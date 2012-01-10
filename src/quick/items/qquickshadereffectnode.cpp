/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
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
    virtual void initialize();
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;

    const QQuickShaderEffectMaterialKey m_key;
    QVector<const char *> m_attributeNames;
    const QVector<QByteArray> m_attributes;
    QString m_log;
    bool m_compiled;

    QVector<int> m_uniformLocs;
    int m_opacityLoc;
    int m_matrixLoc;
    uint m_textureIndicesSet;
};

QQuickCustomMaterialShader::QQuickCustomMaterialShader(const QQuickShaderEffectMaterialKey &key, const QVector<QByteArray> &attributes)
    : m_key(key)
    , m_attributes(attributes)
    , m_compiled(false)
    , m_textureIndicesSet(false)
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
    Q_ASSERT(newEffect != 0);

    QQuickShaderEffectMaterial *material = static_cast<QQuickShaderEffectMaterial *>(newEffect);
    if (!material->m_emittedLogChanged && material->m_node) {
        material->m_emittedLogChanged = true;
        emit material->m_node->logAndStatusChanged(m_log, m_compiled ? QQuickShaderEffect::Compiled
                                                                     : QQuickShaderEffect::Error);
    }

    if (!m_textureIndicesSet) {
        for (int i = 0; i < material->m_textures.size(); ++i)
            program()->setUniformValue(material->m_textures.at(i).first.constData(), i);
        m_textureIndicesSet = true;
    }

    if (m_uniformLocs.size() != material->m_uniformValues.size()) {
        m_uniformLocs.reserve(material->m_uniformValues.size());
        for (int i = 0; i < material->m_uniformValues.size(); ++i) {
            const QByteArray &name = material->m_uniformValues.at(i).first;
            m_uniformLocs.append(program()->uniformLocation(name.constData()));
        }
    }

    QOpenGLFunctions *functions = state.context()->functions();
    for (int i = material->m_textures.size() - 1; i >= 0; --i) {
        functions->glActiveTexture(GL_TEXTURE0 + i);
        if (QSGTextureProvider *provider = material->m_textures.at(i).second) {
            if (QSGTexture *texture = provider->texture()) {
                texture->bind();
                continue;
            }
        }
        qWarning("ShaderEffect: source or provider missing when binding textures");
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (material->m_source.respectsOpacity)
        program()->setUniformValue(m_opacityLoc, state.opacity());

    for (int i = 0; i < material->m_uniformValues.count(); ++i) {
        const QVariant &v = material->m_uniformValues.at(i).second;

        switch (v.type()) {
        case QVariant::Color:
            program()->setUniformValue(m_uniformLocs.at(i), qt_premultiply_color(qvariant_cast<QColor>(v)));
            break;
        case QVariant::Double:
            program()->setUniformValue(m_uniformLocs.at(i), (float) qvariant_cast<double>(v));
            break;
        case QVariant::Transform:
            program()->setUniformValue(m_uniformLocs.at(i), qvariant_cast<QTransform>(v));
            break;
        case QVariant::Int:
            program()->setUniformValue(m_uniformLocs.at(i), v.toInt());
            break;
        case QVariant::Bool:
            program()->setUniformValue(m_uniformLocs.at(i), GLint(v.toBool()));
            break;
        case QVariant::Size:
        case QVariant::SizeF:
            program()->setUniformValue(m_uniformLocs.at(i), v.toSizeF());
            break;
        case QVariant::Point:
        case QVariant::PointF:
            program()->setUniformValue(m_uniformLocs.at(i), v.toPointF());
            break;
        case QVariant::Rect:
        case QVariant::RectF:
            {
                QRectF r = v.toRectF();
                program()->setUniformValue(m_uniformLocs.at(i), r.x(), r.y(), r.width(), r.height());
            }
            break;
        case QVariant::Vector3D:
            program()->setUniformValue(m_uniformLocs.at(i), qvariant_cast<QVector3D>(v));
            break;
        default:
            break;
        }
    }

    const QQuickShaderEffectMaterial *oldMaterial = static_cast<const QQuickShaderEffectMaterial *>(oldEffect);
    if (oldEffect == 0 || material->cullMode() != oldMaterial->cullMode()) {
        switch (material->cullMode()) {
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

    if ((state.isMatrixDirty()) && material->m_source.respectsMatrix)
        program()->setUniformValue(m_matrixLoc, state.combinedMatrix());
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

    static const char *fallbackVertexShader =
            "uniform highp mat4 qt_Matrix;"
            "attribute highp vec4 v;"
            "void main() { gl_Position = qt_Matrix * v; }";
    static const char *fallbackFragmentShader =
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

void QQuickCustomMaterialShader::initialize()
{
    m_opacityLoc = program()->uniformLocation("qt_Opacity");
    m_matrixLoc = program()->uniformLocation("qt_Matrix");
}

const char *QQuickCustomMaterialShader::vertexShader() const
{
    return m_key.vertexCode.constData();
}

const char *QQuickCustomMaterialShader::fragmentShader() const
{
    return m_key.fragmentCode.constData();
}


bool QQuickShaderEffectMaterialKey::operator == (const QQuickShaderEffectMaterialKey &other) const
{
    return vertexCode == other.vertexCode && fragmentCode == other.fragmentCode && className == other.className;
}

uint qHash(const QQuickShaderEffectMaterialKey &key)
{
    return qHash(qMakePair(qMakePair(key.vertexCode, key.fragmentCode), key.className));
}


QHash<QQuickShaderEffectMaterialKey, QSharedPointer<QSGMaterialType> > QQuickShaderEffectMaterial::materialMap;

QQuickShaderEffectMaterial::QQuickShaderEffectMaterial(QQuickShaderEffectNode *node)
    : m_cullMode(NoCulling)
    , m_node(node)
    , m_emittedLogChanged(false)
{
    setFlag(Blending, true);
}

QSGMaterialType *QQuickShaderEffectMaterial::type() const
{
    return m_type.data();
}

QSGMaterialShader *QQuickShaderEffectMaterial::createShader() const
{
    return new QQuickCustomMaterialShader(m_source, m_source.attributeNames);
}

int QQuickShaderEffectMaterial::compare(const QSGMaterial *other) const
{
    return this - static_cast<const QQuickShaderEffectMaterial *>(other);
}

void QQuickShaderEffectMaterial::setCullMode(QQuickShaderEffectMaterial::CullMode face)
{
    m_cullMode = face;
}

QQuickShaderEffectMaterial::CullMode QQuickShaderEffectMaterial::cullMode() const
{
    return m_cullMode;
}

void QQuickShaderEffectMaterial::setProgramSource(const QQuickShaderEffectProgram &source)
{
    m_source = source;
    m_emittedLogChanged = false;
    m_type = materialMap.value(m_source);
    if (m_type.isNull()) {
        m_type = QSharedPointer<QSGMaterialType>(new QSGMaterialType);
        materialMap.insert(m_source, m_type);
    }
}

void QQuickShaderEffectMaterial::setUniforms(const QVector<QPair<QByteArray, QVariant> > &uniformValues)
{
    m_uniformValues = uniformValues;
}

void QQuickShaderEffectMaterial::setTextureProviders(const QVector<QPair<QByteArray, QSGTextureProvider *> > &textures)
{
    m_textures = textures;
}

const QVector<QPair<QByteArray, QSGTextureProvider *> > &QQuickShaderEffectMaterial::textureProviders() const
{
    return m_textures;
}

void QQuickShaderEffectMaterial::updateTextures() const
{
    for (int i = 0; i < m_textures.size(); ++i) {
        if (QSGTextureProvider *provider = m_textures.at(i).second) {
            if (QSGDynamicTexture *texture = qobject_cast<QSGDynamicTexture *>(provider->texture()))
                texture->updateTexture();
        }
    }
}


QQuickShaderEffectNode::QQuickShaderEffectNode()
    : m_material(this)
{
    QSGNode::setFlag(UsePreprocess, true);
    setMaterial(&m_material);

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

void QQuickShaderEffectNode::preprocess()
{
    Q_ASSERT(material());
    static_cast<QQuickShaderEffectMaterial *>(material())->updateTextures();
}

QT_END_NAMESPACE
