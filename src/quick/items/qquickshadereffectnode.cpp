/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qquickshadereffectnode_p.h>

#include "qquickshadereffectmesh_p.h"
#include "qquickshadereffect_p.h"
#include <QtQuick/qsgtextureprovider.h>
#include <QtQuick/private/qsgrenderer_p.h>

QT_BEGIN_NAMESPACE

class QQuickCustomMaterialShader : public QSGMaterialShader
{
public:
    QQuickCustomMaterialShader(const QQuickShaderEffectMaterialKey &key, const QVector<QByteArray> &attributes);
    virtual void deactivate();
    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect);
    virtual char const *const *attributeNames() const;

protected:
    friend class QQuickShaderEffectNode;

    virtual void compile();
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;

    const QQuickShaderEffectMaterialKey m_key;
    QVector<const char *> m_attributeNames;
    const QVector<QByteArray> m_attributes;
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
    for (int i = 0; i < attributes.count(); ++i)
        m_attributeNames.append(attributes.at(i).constData());
    m_attributeNames.append(0);
}

void QQuickCustomMaterialShader::deactivate()
{
    QSGMaterialShader::deactivate();
    glDisable(GL_CULL_FACE);
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
                        if (loc >= 0) {
                            QRectF r = texture->normalizedTextureSubRect();
                            program()->setUniformValue(loc, r.x(), r.y(), r.width(), r.height());
                        } else if (texture->isAtlasTexture()) {
                            texture = texture->removedFromAtlas();
                        }
                        texture->bind();
                        continue;
                    }
                }
                qWarning("ShaderEffect: source or provider missing when binding textures");
                glBindTexture(GL_TEXTURE_2D, 0);
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
                case QMetaType::QVector3D:
                    program()->setUniformValue(loc, qvariant_cast<QVector3D>(d.value));
                    break;
                case QMetaType::QVector4D:
                    program()->setUniformValue(loc, qvariant_cast<QVector4D>(d.value));
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
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case QQuickShaderEffectMaterial::BackFaceCulling:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        default:
            glDisable(GL_CULL_FACE);
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
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
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

    static const char fallbackVertexShader[] =
            "uniform highp mat4 qt_Matrix;"
            "attribute highp vec4 v;"
            "void main() { gl_Position = qt_Matrix * v; }";
    static const char fallbackFragmentShader[] =
            "void main() { gl_FragColor = vec4(1., 0., 1., 1.); }";

    if (!m_compiled) {
        qWarning("QQuickCustomMaterialShader: Shader compilation failed:");
        qWarning() << program()->log();

        program()->removeAllShaders();
        program()->addShaderFromSourceCode(QOpenGLShader::Vertex, fallbackVertexShader);
        program()->addShaderFromSourceCode(QOpenGLShader::Fragment, fallbackFragmentShader);
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
    if (className != other.className)
        return false;
    for (int shaderType = 0; shaderType < ShaderTypeCount; ++shaderType) {
        if (sourceCode[shaderType] != other.sourceCode[shaderType])
            return false;
    }
    return true;
}

uint qHash(const QQuickShaderEffectMaterialKey &key)
{
    uint hash = qHash((void *)key.className);
    typedef QQuickShaderEffectMaterialKey Key;
    for (int shaderType = 0; shaderType < Key::ShaderTypeCount; ++shaderType)
        hash = hash * 31337 + qHash(key.sourceCode[shaderType]);
    return hash;
}


QHash<QQuickShaderEffectMaterialKey, QSharedPointer<QSGMaterialType> > QQuickShaderEffectMaterial::materialMap;

QQuickShaderEffectMaterial::QQuickShaderEffectMaterial(QQuickShaderEffectNode *node)
    : cullMode(NoCulling)
    , m_node(node)
    , m_emittedLogChanged(false)
{
    setFlag(Blending | RequiresFullMatrix, true);
}

QSGMaterialType *QQuickShaderEffectMaterial::type() const
{
    return m_type.data();
}

QSGMaterialShader *QQuickShaderEffectMaterial::createShader() const
{
    return new QQuickCustomMaterialShader(m_source, attributes);
}

int QQuickShaderEffectMaterial::compare(const QSGMaterial *other) const
{
    return this - static_cast<const QQuickShaderEffectMaterial *>(other);
}

void QQuickShaderEffectMaterial::setProgramSource(const QQuickShaderEffectMaterialKey &source)
{
    m_source = source;
    m_emittedLogChanged = false;
    m_type = materialMap.value(m_source);
    if (m_type.isNull()) {
        m_type = QSharedPointer<QSGMaterialType>(new QSGMaterialType);
        materialMap.insert(m_source, m_type);
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

#ifdef QML_RUNTIME_TESTING
    description = QLatin1String("shadereffect");
#endif
}

QQuickShaderEffectNode::~QQuickShaderEffectNode()
{
}

void QQuickShaderEffectNode::markDirtyTexture()
{
    markDirty(DirtyMaterial);
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

QT_END_NAMESPACE
