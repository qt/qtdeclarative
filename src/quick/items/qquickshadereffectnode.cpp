/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qquickshadereffectnode_p.h>

#include "qquickshadereffect_p.h"
#include <QtQuick/qsgtextureprovider.h>
#include <QtQuick/private/qsgrenderer_p.h>
#include <QtQuick/private/qsgshadersourcebuilder_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

static bool hasAtlasTexture(const QVector<QSGTextureProvider *> &textureProviders)
{
    for (int i = 0; i < textureProviders.size(); ++i) {
        QSGTextureProvider *t = textureProviders.at(i);
        if (t && t->texture() && t->texture()->isAtlasTexture())
            return true;
    }
    return false;
}

class QQuickCustomMaterialShader : public QSGMaterialShader
{
public:
    QQuickCustomMaterialShader(const QQuickShaderEffectMaterialKey &key, const QVector<QByteArray> &attributes);
    void deactivate() Q_DECL_OVERRIDE;
    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) Q_DECL_OVERRIDE;
    char const *const *attributeNames() const Q_DECL_OVERRIDE;

protected:
    friend class QQuickShaderEffectNode;

    void compile() Q_DECL_OVERRIDE;
    const char *vertexShader() const Q_DECL_OVERRIDE;
    const char *fragmentShader() const Q_DECL_OVERRIDE;

    const QQuickShaderEffectMaterialKey m_key;
    QVector<QByteArray> m_attributes;
    QVector<const char *> m_attributeNames;
    QString m_log;
    bool m_compiled;

    QVector<int> m_uniformLocs[QQuickShaderEffectMaterialKey::ShaderTypeCount];
    uint m_initialized : 1;
};

QQuickCustomMaterialShader::QQuickCustomMaterialShader(const QQuickShaderEffectMaterialKey &key, const QVector<QByteArray> &attributes)
    : m_key(key)
    , m_attributes(attributes)
    , m_compiled(false)
    , m_initialized(false)
{
    const int attributesCount = m_attributes.count();
    m_attributeNames.reserve(attributesCount + 1);
    for (int i = 0; i < attributesCount; ++i)
        m_attributeNames.append(m_attributes.at(i).constData());
    m_attributeNames.append(0);
}

void QQuickCustomMaterialShader::deactivate()
{
    QSGMaterialShader::deactivate();
    QOpenGLContext::currentContext()->functions()->glDisable(GL_CULL_FACE);
}

void QQuickCustomMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    typedef QQuickShaderEffectMaterial::UniformData UniformData;

    Q_ASSERT(newEffect != 0);

    QQuickShaderEffectMaterial *material = static_cast<QQuickShaderEffectMaterial *>(newEffect);
    if (!material->m_emittedLogChanged && material->m_node) {
        material->m_emittedLogChanged = true;
        emit material->m_node->logAndStatusChanged(m_log, m_compiled ? QQuickShaderEffect::Compiled
                                                                     : QQuickShaderEffect::Error);
    }

    int textureProviderIndex = 0;
    if (!m_initialized) {
        for (int shaderType = 0; shaderType < QQuickShaderEffectMaterialKey::ShaderTypeCount; ++shaderType) {
            Q_ASSERT(m_uniformLocs[shaderType].isEmpty());
            m_uniformLocs[shaderType].reserve(material->uniforms[shaderType].size());
            for (int i = 0; i < material->uniforms[shaderType].size(); ++i) {
                const UniformData &d = material->uniforms[shaderType].at(i);
                QByteArray name = d.name;
                if (d.specialType == UniformData::Sampler) {
                    program()->setUniformValue(d.name.constData(), textureProviderIndex++);
                    // We don't need to store the sampler uniform locations, since their values
                    // only need to be set once. Look for the "qt_SubRect_" uniforms instead.
                    // These locations are used when binding the textures later.
                    name = "qt_SubRect_" + name;
                }
                m_uniformLocs[shaderType].append(program()->uniformLocation(name.constData()));
            }
        }
        m_initialized = true;
        textureProviderIndex = 0;
    }

    QOpenGLFunctions *functions = state.context()->functions();
    for (int shaderType = 0; shaderType < QQuickShaderEffectMaterialKey::ShaderTypeCount; ++shaderType) {
        for (int i = 0; i < material->uniforms[shaderType].size(); ++i) {
            const UniformData &d = material->uniforms[shaderType].at(i);
            int loc = m_uniformLocs[shaderType].at(i);
            if (d.specialType == UniformData::Sampler) {
                int idx = textureProviderIndex++;
                functions->glActiveTexture(GL_TEXTURE0 + idx);
                if (QSGTextureProvider *provider = material->textureProviders.at(idx)) {
                    if (QSGTexture *texture = provider->texture()) {

#ifndef QT_NO_DEBUG
                        if (!qsg_safeguard_texture(texture))
                            continue;
#endif

                        if (loc >= 0) {
                            QRectF r = texture->normalizedTextureSubRect();
                            program()->setUniformValue(loc, r.x(), r.y(), r.width(), r.height());
                        } else if (texture->isAtlasTexture() && !material->geometryUsesTextureSubRect) {
                            texture = texture->removedFromAtlas();
                        }
                        texture->bind();
                        continue;
                    }
                }
                functions->glBindTexture(GL_TEXTURE_2D, 0);
            } else if (d.specialType == UniformData::Opacity) {
                program()->setUniformValue(loc, state.opacity());
            } else if (d.specialType == UniformData::Matrix) {
                if (state.isMatrixDirty())
                    program()->setUniformValue(loc, state.combinedMatrix());
            } else if (d.specialType == UniformData::None) {
                switch (int(d.value.type())) {
                case QMetaType::QColor:
                    program()->setUniformValue(loc, qt_premultiply_color(qvariant_cast<QColor>(d.value)));
                    break;
                case QMetaType::Float:
                    program()->setUniformValue(loc, qvariant_cast<float>(d.value));
                    break;
                case QMetaType::Double:
                    program()->setUniformValue(loc, (float) qvariant_cast<double>(d.value));
                    break;
                case QMetaType::QTransform:
                    program()->setUniformValue(loc, qvariant_cast<QTransform>(d.value));
                    break;
                case QMetaType::Int:
                    program()->setUniformValue(loc, d.value.toInt());
                    break;
                case QMetaType::Bool:
                    program()->setUniformValue(loc, GLint(d.value.toBool()));
                    break;
                case QMetaType::QSize:
                case QMetaType::QSizeF:
                    program()->setUniformValue(loc, d.value.toSizeF());
                    break;
                case QMetaType::QPoint:
                case QMetaType::QPointF:
                    program()->setUniformValue(loc, d.value.toPointF());
                    break;
                case QMetaType::QRect:
                case QMetaType::QRectF:
                    {
                        QRectF r = d.value.toRectF();
                        program()->setUniformValue(loc, r.x(), r.y(), r.width(), r.height());
                    }
                    break;
                case QMetaType::QVector2D:
                    program()->setUniformValue(loc, qvariant_cast<QVector2D>(d.value));
                    break;
                case QMetaType::QVector3D:
                    program()->setUniformValue(loc, qvariant_cast<QVector3D>(d.value));
                    break;
                case QMetaType::QVector4D:
                    program()->setUniformValue(loc, qvariant_cast<QVector4D>(d.value));
                    break;
                case QMetaType::QQuaternion:
                    {
                        QQuaternion q = qvariant_cast<QQuaternion>(d.value);
                        program()->setUniformValue(loc, q.x(), q.y(), q.z(), q.scalar());
                    }
                    break;
                case QMetaType::QMatrix4x4:
                    program()->setUniformValue(loc, qvariant_cast<QMatrix4x4>(d.value));
                    break;
                default:
                    break;
                }
            }
        }
    }
    functions->glActiveTexture(GL_TEXTURE0);

    const QQuickShaderEffectMaterial *oldMaterial = static_cast<const QQuickShaderEffectMaterial *>(oldEffect);
    if (oldEffect == 0 || material->cullMode != oldMaterial->cullMode) {
        switch (material->cullMode) {
        case QQuickShaderEffectMaterial::FrontFaceCulling:
            functions->glEnable(GL_CULL_FACE);
            functions->glCullFace(GL_FRONT);
            break;
        case QQuickShaderEffectMaterial::BackFaceCulling:
            functions->glEnable(GL_CULL_FACE);
            functions->glCullFace(GL_BACK);
            break;
        default:
            functions->glDisable(GL_CULL_FACE);
            break;
        }
    }
}

char const *const *QQuickCustomMaterialShader::attributeNames() const
{
    return m_attributeNames.constData();
}

void QQuickCustomMaterialShader::compile()
{
    Q_ASSERT_X(!program()->isLinked(), "QQuickCustomMaterialShader::compile()", "Compile called multiple times!");

    m_log.clear();
    m_compiled = true;
    if (!program()->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShader())) {
        m_log += QLatin1String("*** Vertex shader ***\n");
        m_log += program()->log();
        m_compiled = false;
    }
    if (!program()->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShader())) {
        m_log += QLatin1String("*** Fragment shader ***\n");
        m_log += program()->log();
        m_compiled = false;
    }

    char const *const *attr = attributeNames();
#ifndef QT_NO_DEBUG
    int maxVertexAttribs = 0;
    QOpenGLContext::currentContext()->functions()->glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    int attrCount = 0;
    while (attrCount < maxVertexAttribs && attr[attrCount])
        ++attrCount;
    if (attr[attrCount]) {
        qWarning("List of attribute names is too long.\n"
                 "Maximum number of attributes on this hardware is %i.\n"
                 "Vertex shader:\n%s\n"
                 "Fragment shader:\n%s\n",
                 maxVertexAttribs, vertexShader(), fragmentShader());
    }
#endif

    if (m_compiled) {
#ifndef QT_NO_DEBUG
        for (int i = 0; i < attrCount; ++i) {
#else
        for (int i = 0; attr[i]; ++i) {
#endif
            if (*attr[i])
                program()->bindAttributeLocation(attr[i], i);
        }
        m_compiled = program()->link();
        m_log += program()->log();
    }

    if (!m_compiled) {
        qWarning("QQuickCustomMaterialShader: Shader compilation failed:");
        qWarning() << program()->log();

        QSGShaderSourceBuilder::initializeProgramFromFiles(
            program(),
            QStringLiteral(":/qt-project.org/items/shaders/shadereffectfallback.vert"),
            QStringLiteral(":/qt-project.org/items/shaders/shadereffectfallback.frag"));

#ifndef QT_NO_DEBUG
        for (int i = 0; i < attrCount; ++i) {
#else
        for (int i = 0; attr[i]; ++i) {
#endif
            if (qstrcmp(attr[i], qtPositionAttributeName()) == 0)
                program()->bindAttributeLocation("v", i);
        }
        program()->link();
    }
}

const char *QQuickCustomMaterialShader::vertexShader() const
{
    return m_key.sourceCode[QQuickShaderEffectMaterialKey::VertexShader].constData();
}

const char *QQuickCustomMaterialShader::fragmentShader() const
{
    return m_key.sourceCode[QQuickShaderEffectMaterialKey::FragmentShader].constData();
}


bool QQuickShaderEffectMaterialKey::operator == (const QQuickShaderEffectMaterialKey &other) const
{
    for (int shaderType = 0; shaderType < ShaderTypeCount; ++shaderType) {
        if (sourceCode[shaderType] != other.sourceCode[shaderType])
            return false;
    }
    return true;
}

bool QQuickShaderEffectMaterialKey::operator != (const QQuickShaderEffectMaterialKey &other) const
{
    return !(*this == other);
}

uint qHash(const QQuickShaderEffectMaterialKey &key)
{
    uint hash = 1;
    typedef QQuickShaderEffectMaterialKey Key;
    for (int shaderType = 0; shaderType < Key::ShaderTypeCount; ++shaderType)
        hash = hash * 31337 + qHash(key.sourceCode[shaderType]);
    return hash;
}

class QQuickShaderEffectMaterialCache : public QObject
{
    Q_OBJECT
public:
    static QQuickShaderEffectMaterialCache *get(bool create = true) {
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        QQuickShaderEffectMaterialCache *me = ctx->findChild<QQuickShaderEffectMaterialCache *>(QStringLiteral("__qt_ShaderEffectCache"), Qt::FindDirectChildrenOnly);
        if (!me && create) {
            me = new QQuickShaderEffectMaterialCache();
            me->setObjectName(QStringLiteral("__qt_ShaderEffectCache"));
            me->setParent(ctx);
        }
        return me;
    }
    QHash<QQuickShaderEffectMaterialKey, QSGMaterialType *> cache;
};

QQuickShaderEffectMaterial::QQuickShaderEffectMaterial(QQuickShaderEffectNode *node)
    : cullMode(NoCulling)
    , geometryUsesTextureSubRect(false)
    , m_node(node)
    , m_emittedLogChanged(false)
{
    setFlag(Blending | RequiresFullMatrix, true);
}

QSGMaterialType *QQuickShaderEffectMaterial::type() const
{
    return m_type;
}

QSGMaterialShader *QQuickShaderEffectMaterial::createShader() const
{
    return new QQuickCustomMaterialShader(m_source, attributes);
}

bool QQuickShaderEffectMaterial::UniformData::operator == (const UniformData &other) const
{
    if (specialType != other.specialType)
        return false;
    if (name != other.name)
        return false;

    if (specialType == UniformData::Sampler) {
        // We can't check the source objects as these live in the GUI thread,
        // so return true here and rely on the textureProvider check for
        // equality of these..
        return true;
    } else {
        return value == other.value;
    }
}

int QQuickShaderEffectMaterial::compare(const QSGMaterial *o) const
{
    const QQuickShaderEffectMaterial *other = static_cast<const QQuickShaderEffectMaterial *>(o);
    if ((hasAtlasTexture(textureProviders) && !geometryUsesTextureSubRect) || (hasAtlasTexture(other->textureProviders) && !other->geometryUsesTextureSubRect))
        return 1;
    if (cullMode != other->cullMode)
        return 1;
    for (int shaderType = 0; shaderType < QQuickShaderEffectMaterialKey::ShaderTypeCount; ++shaderType) {
        if (uniforms[shaderType] != other->uniforms[shaderType])
            return 1;
    }

    // Check the texture providers..
    if (textureProviders.size() != other->textureProviders.size())
        return 1;
    for (int i=0; i<textureProviders.size(); ++i) {
        QSGTextureProvider *tp1 = textureProviders.at(i);
        QSGTextureProvider *tp2 = other->textureProviders.at(i);
        if (!tp1 || !tp2)
            return tp1 == tp2 ? 0 : 1;
        QSGTexture *t1 = tp1->texture();
        QSGTexture *t2 = tp2->texture();
        if (!t1 || !t2)
            return t1 == t2 ? 0 : 1;
        // Check texture id's as textures may be in the same atlas.
        if (t1->textureId() != t2->textureId())
            return 1;
    }
    return 0;
}

void QQuickShaderEffectMaterial::setProgramSource(const QQuickShaderEffectMaterialKey &source)
{
    m_source = source;
    m_emittedLogChanged = false;

    QQuickShaderEffectMaterialCache *cache = QQuickShaderEffectMaterialCache::get();
    m_type = cache->cache.value(m_source);
    if (!m_type) {
        m_type = new QSGMaterialType();
        cache->cache.insert(source, m_type);
    }
}

void QQuickShaderEffectMaterial::cleanupMaterialCache()
{
    QQuickShaderEffectMaterialCache *cache = QQuickShaderEffectMaterialCache::get(false);
    if (cache) {
        qDeleteAll(cache->cache.values());
        delete cache;
    }
}

void QQuickShaderEffectMaterial::updateTextures() const
{
    for (int i = 0; i < textureProviders.size(); ++i) {
        if (QSGTextureProvider *provider = textureProviders.at(i)) {
            if (QSGDynamicTexture *texture = qobject_cast<QSGDynamicTexture *>(provider->texture()))
                texture->updateTexture();
        }
    }
}

void QQuickShaderEffectMaterial::invalidateTextureProvider(QSGTextureProvider *provider)
{
    for (int i = 0; i < textureProviders.size(); ++i) {
        if (provider == textureProviders.at(i))
            textureProviders[i] = 0;
    }
}


QQuickShaderEffectNode::QQuickShaderEffectNode()
{
    QSGNode::setFlag(UsePreprocess, true);

#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(this, QLatin1String("shadereffect"));
#endif
}

QQuickShaderEffectNode::~QQuickShaderEffectNode()
{
}

void QQuickShaderEffectNode::markDirtyTexture()
{
    markDirty(DirtyMaterial);
    Q_EMIT dirtyTexture();
}

void QQuickShaderEffectNode::textureProviderDestroyed(QObject *object)
{
    Q_ASSERT(material());
    static_cast<QQuickShaderEffectMaterial *>(material())->invalidateTextureProvider(static_cast<QSGTextureProvider *>(object));
}

void QQuickShaderEffectNode::preprocess()
{
    Q_ASSERT(material());
    static_cast<QQuickShaderEffectMaterial *>(material())->updateTextures();
}

#include "qquickshadereffectnode.moc"

QT_END_NAMESPACE
