/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qsgrhishadereffectnode_p.h"
#include "qsgdefaultrendercontext_p.h"
#include "qsgrhisupport_p.h"
#include <qsgmaterialrhishader.h>
#include <qsgtextureprovider.h>
#include <private/qsgplaintexture_p.h>
#include <QtGui/private/qshaderdescription_p.h>
#include <QQmlFile>
#include <QFile>
#include <QFileSelector>

QT_BEGIN_NAMESPACE

void QSGRhiShaderLinker::reset(const QShader &vs, const QShader &fs)
{
    Q_ASSERT(vs.isValid() && fs.isValid());
    m_vs = vs;
    m_fs = fs;

    m_error = false;

    m_constantBufferSize = 0;
    m_constants.clear();
    m_samplers.clear();
    m_samplerNameMap.clear();
}

void QSGRhiShaderLinker::feedConstants(const QSGShaderEffectNode::ShaderData &shader, const QSet<int> *dirtyIndices)
{
    Q_ASSERT(shader.shaderInfo.variables.count() == shader.varData.count());
    if (!dirtyIndices) {
        m_constantBufferSize = qMax(m_constantBufferSize, shader.shaderInfo.constantDataSize);
        for (int i = 0; i < shader.shaderInfo.variables.count(); ++i) {
            const QSGGuiThreadShaderEffectManager::ShaderInfo::Variable &var(shader.shaderInfo.variables.at(i));
            if (var.type == QSGGuiThreadShaderEffectManager::ShaderInfo::Constant) {
                const QSGShaderEffectNode::VariableData &vd(shader.varData.at(i));
                Constant c;
                c.size = var.size;
                c.specialType = vd.specialType;
                if (c.specialType != QSGShaderEffectNode::VariableData::SubRect) {
                    c.value = vd.value;
                    if (QSGRhiSupport::instance()->isShaderEffectDebuggingRequested()) {
                        if (c.specialType == QSGShaderEffectNode::VariableData::None) {
                            qDebug() << "cbuf prepare" << shader.shaderInfo.name << var.name
                                     << "offset" << var.offset << "value" << c.value;
                        } else {
                            qDebug() << "cbuf prepare" << shader.shaderInfo.name << var.name
                                     << "offset" << var.offset << "special" << c.specialType;
                        }
                    }
                } else {
                    Q_ASSERT(var.name.startsWith(QByteArrayLiteral("qt_SubRect_")));
                    c.value = var.name.mid(11);
                }
                m_constants[var.offset] = c;
            }
        }
    } else {
        for (int idx : *dirtyIndices) {
            const int offset = shader.shaderInfo.variables.at(idx).offset;
            const QVariant value = shader.varData.at(idx).value;
            m_constants[offset].value = value;
            if (QSGRhiSupport::instance()->isShaderEffectDebuggingRequested()) {
                qDebug() << "cbuf update" << shader.shaderInfo.name
                         << "offset" << offset << "value" << value;
            }
        }
    }
}

void QSGRhiShaderLinker::feedSamplers(const QSGShaderEffectNode::ShaderData &shader, const QSet<int> *dirtyIndices)
{
    if (!dirtyIndices) {
        for (int i = 0; i < shader.shaderInfo.variables.count(); ++i) {
            const QSGGuiThreadShaderEffectManager::ShaderInfo::Variable &var(shader.shaderInfo.variables.at(i));
            const QSGShaderEffectNode::VariableData &vd(shader.varData.at(i));
            if (var.type == QSGGuiThreadShaderEffectManager::ShaderInfo::Sampler) {
                Q_ASSERT(vd.specialType == QSGShaderEffectNode::VariableData::Source);
                m_samplers.insert(var.bindPoint, vd.value);
                m_samplerNameMap.insert(var.name, var.bindPoint);
            }
        }
    } else {
        for (int idx : *dirtyIndices) {
            const QSGGuiThreadShaderEffectManager::ShaderInfo::Variable &var(shader.shaderInfo.variables.at(idx));
            const QSGShaderEffectNode::VariableData &vd(shader.varData.at(idx));
            m_samplers.insert(var.bindPoint, vd.value);
            m_samplerNameMap.insert(var.name, var.bindPoint);
        }
    }
}

void QSGRhiShaderLinker::linkTextureSubRects()
{
    // feedConstants stores <name> in Constant::value for subrect entries. Now
    // that both constants and textures are known, replace the name with the
    // texture binding point.
    for (Constant &c : m_constants) {
        if (c.specialType == QSGShaderEffectNode::VariableData::SubRect) {
            if (c.value.userType() == QMetaType::QByteArray) {
                const QByteArray name = c.value.toByteArray();
                if (!m_samplerNameMap.contains(name))
                    qWarning("ShaderEffect: qt_SubRect_%s refers to unknown source texture", name.constData());
                c.value = m_samplerNameMap[name];
            }
        }
    }
}

void QSGRhiShaderLinker::dump()
{
    if (m_error) {
        qDebug() << "Failed to generate program data";
        return;
    }
    qDebug() << "Combined shader data" << m_vs << m_fs << "cbuffer size" << m_constantBufferSize;
    qDebug() << " - constants" << m_constants;
    qDebug() << " - samplers" << m_samplers;
}

QDebug operator<<(QDebug debug, const QSGRhiShaderLinker::Constant &c)
{
    QDebugStateSaver saver(debug);
    debug.space();
    debug << "size" << c.size;
    if (c.specialType != QSGShaderEffectNode::VariableData::None)
        debug << "special" << c.specialType;
    else
        debug << "value" << c.value;
    return debug;
}

struct QSGRhiShaderMaterialTypeCache
{
    QSGMaterialType *get(const QShader &vs, const QShader &fs);
    void reset() { qDeleteAll(m_types); m_types.clear(); }

    struct Key {
        QShader blob[2];
        Key() { }
        Key(const QShader &vs, const QShader &fs) { blob[0] = vs; blob[1] = fs; }
        bool operator==(const Key &other) const {
            return blob[0] == other.blob[0] && blob[1] == other.blob[1];
        }
    };
    QHash<Key, QSGMaterialType *> m_types;
};

uint qHash(const QSGRhiShaderMaterialTypeCache::Key &key, uint seed = 0)
{
    uint hash = seed;
    for (int i = 0; i < 2; ++i)
        hash = hash * 31337 + qHash(key.blob[i]);
    return hash;
}

QSGMaterialType *QSGRhiShaderMaterialTypeCache::get(const QShader &vs, const QShader &fs)
{
    const Key k(vs, fs);
    if (m_types.contains(k))
        return m_types.value(k);

    QSGMaterialType *t = new QSGMaterialType;
    m_types.insert(k, t);
    return t;
}

static QSGRhiShaderMaterialTypeCache shaderMaterialTypeCache;

class QSGRhiShaderEffectMaterialShader : public QSGMaterialRhiShader
{
public:
    QSGRhiShaderEffectMaterialShader(const QSGRhiShaderEffectMaterial *material);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    bool updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

QSGRhiShaderEffectMaterialShader::QSGRhiShaderEffectMaterialShader(const QSGRhiShaderEffectMaterial *material)
{
    setFlag(UpdatesGraphicsPipelineState, true);
    setShader(VertexStage, material->m_vertexShader);
    setShader(FragmentStage, material->m_fragmentShader);
}

static inline QColor qsg_premultiply_color(const QColor &c)
{
    return QColor::fromRgbF(c.redF() * c.alphaF(), c.greenF() * c.alphaF(), c.blueF() * c.alphaF(), c.alphaF());
}

bool QSGRhiShaderEffectMaterialShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);
    QSGRhiShaderEffectMaterial *mat = static_cast<QSGRhiShaderEffectMaterial *>(newMaterial);

    bool changed = false;
    QByteArray *buf = state.uniformData();

    for (auto it = mat->m_linker.m_constants.constBegin(), itEnd = mat->m_linker.m_constants.constEnd(); it != itEnd; ++it) {
        const int offset = it.key();
        char *dst = buf->data() + offset;
        const QSGRhiShaderLinker::Constant &c(it.value());
        if (c.specialType == QSGShaderEffectNode::VariableData::Opacity) {
            if (state.isOpacityDirty()) {
                const float f = state.opacity();
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, &f, sizeof(f));
                changed = true;
            }
        } else if (c.specialType == QSGShaderEffectNode::VariableData::Matrix) {
            if (state.isMatrixDirty()) {
                const int sz = 16 * sizeof(float);
                Q_ASSERT(sz == c.size);
                memcpy(dst, state.combinedMatrix().constData(), sz);
                changed = true;
            }
        } else if (c.specialType == QSGShaderEffectNode::VariableData::SubRect) {
            // vec4
            QRectF subRect(0, 0, 1, 1);
            const int binding = c.value.toInt(); // filled in by linkTextureSubRects
            if (binding < QSGRhiShaderEffectMaterial::MAX_BINDINGS) {
                if (QSGTextureProvider *tp = mat->m_textureProviders.at(binding)) {
                    if (QSGTexture *t = tp->texture())
                        subRect = t->normalizedTextureSubRect();
                }
            }
            const float f[4] = { float(subRect.x()), float(subRect.y()),
                                 float(subRect.width()), float(subRect.height()) };
            Q_ASSERT(sizeof(f) == c.size);
            memcpy(dst, f, sizeof(f));
        } else if (c.specialType == QSGShaderEffectNode::VariableData::None) {
            changed = true;
            switch (int(c.value.userType())) {
            case QMetaType::QColor: {
                const QColor v = qsg_premultiply_color(qvariant_cast<QColor>(c.value));
                const float f[4] = { float(v.redF()), float(v.greenF()), float(v.blueF()), float(v.alphaF()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::Float: {
                const float f = qvariant_cast<float>(c.value);
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, &f, sizeof(f));
                break;
            }
            case QMetaType::Double: {
                const float f = float(qvariant_cast<double>(c.value));
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, &f, sizeof(f));
                break;
            }
            case QMetaType::Int: {
                const int i = c.value.toInt();
                Q_ASSERT(sizeof(i) == c.size);
                memcpy(dst, &i, sizeof(i));
                break;
            }
            case QMetaType::Bool: {
                const bool b = c.value.toBool();
                Q_ASSERT(sizeof(b) == c.size);
                memcpy(dst, &b, sizeof(b));
                break;
            }
            case QMetaType::QTransform: { // mat3
                const QTransform v = qvariant_cast<QTransform>(c.value);
                const float m[3][3] = {
                    { float(v.m11()), float(v.m12()), float(v.m13()) },
                    { float(v.m21()), float(v.m22()), float(v.m23()) },
                    { float(v.m31()), float(v.m32()), float(v.m33()) }
                };
                Q_ASSERT(sizeof(m) == c.size);
                memcpy(dst, m[0], sizeof(m));
                break;
            }
            case QMetaType::QSize:
            case QMetaType::QSizeF: { // vec2
                const QSizeF v = c.value.toSizeF();
                const float f[2] = { float(v.width()), float(v.height()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QPoint:
            case QMetaType::QPointF: { // vec2
                const QPointF v = c.value.toPointF();
                const float f[2] = { float(v.x()), float(v.y()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QRect:
            case QMetaType::QRectF: { // vec4
                const QRectF v = c.value.toRectF();
                const float f[4] = { float(v.x()), float(v.y()), float(v.width()), float(v.height()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QVector2D: { // vec2
                const QVector2D v = qvariant_cast<QVector2D>(c.value);
                const float f[2] = { float(v.x()), float(v.y()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QVector3D: { // vec3
                const QVector3D v = qvariant_cast<QVector3D>(c.value);
                const float f[3] = { float(v.x()), float(v.y()), float(v.z()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QVector4D: { // vec4
                const QVector4D v = qvariant_cast<QVector4D>(c.value);
                const float f[4] = { float(v.x()), float(v.y()), float(v.z()), float(v.w()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QQuaternion: { // vec4
                const QQuaternion v = qvariant_cast<QQuaternion>(c.value);
                const float f[4] = { float(v.x()), float(v.y()), float(v.z()), float(v.scalar()) };
                Q_ASSERT(sizeof(f) == c.size);
                memcpy(dst, f, sizeof(f));
                break;
            }
            case QMetaType::QMatrix4x4: { // mat4
                const QMatrix4x4 v = qvariant_cast<QMatrix4x4>(c.value);
                const int sz = 16 * sizeof(float);
                Q_ASSERT(sz == c.size);
                memcpy(dst, v.constData(), sz);
                break;
            }
            default:
                break;
            }
        }
    }

    return changed;
}

void QSGRhiShaderEffectMaterialShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                          QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);
    QSGRhiShaderEffectMaterial *mat = static_cast<QSGRhiShaderEffectMaterial *>(newMaterial);

    if (binding >= QSGRhiShaderEffectMaterial::MAX_BINDINGS)
        return;

    QSGTextureProvider *tp = mat->m_textureProviders.at(binding);
    if (tp) {
        if (QSGTexture *t = tp->texture()) {
            t->updateRhiTexture(state.rhi(), state.resourceUpdateBatch());
            if (t->isAtlasTexture() && !mat->m_geometryUsesTextureSubRect) {
                // Why the hassle with the batch: while removedFromAtlas() is
                // able to operate with its own resource update batch (which is
                // then committed immediately), that approach is wrong when the
                // atlas enqueued (in the updateRhiTexture() above) not yet
                // committed operations to state.resourceUpdateBatch()... The
                // only safe way then is to use the same batch the atlas'
                // updateRhiTexture() used.
                t->setWorkResourceUpdateBatch(state.resourceUpdateBatch());
                QSGTexture *newTexture = t->removedFromAtlas();
                t->setWorkResourceUpdateBatch(nullptr);
                if (newTexture)
                    t = newTexture;
            }
            *texture = t;
            return;
        }
    }

    if (!mat->m_dummyTexture) {
        mat->m_dummyTexture = new QSGPlainTexture;
        mat->m_dummyTexture->setFiltering(QSGTexture::Nearest);
        mat->m_dummyTexture->setHorizontalWrapMode(QSGTexture::Repeat);
        mat->m_dummyTexture->setVerticalWrapMode(QSGTexture::Repeat);
        QImage img(128, 128, QImage::Format_ARGB32_Premultiplied);
        img.fill(0);
        mat->m_dummyTexture->setImage(img);
        mat->m_dummyTexture->updateRhiTexture(state.rhi(), state.resourceUpdateBatch());
    }
    *texture = mat->m_dummyTexture;
}

bool QSGRhiShaderEffectMaterialShader::updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                                                   QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(state);
    Q_UNUSED(oldMaterial);
    QSGRhiShaderEffectMaterial *mat = static_cast<QSGRhiShaderEffectMaterial *>(newMaterial);

    switch (mat->m_cullMode) {
    case QSGShaderEffectNode::FrontFaceCulling:
        ps->cullMode = GraphicsPipelineState::CullFront;
        return true;
    case QSGShaderEffectNode::BackFaceCulling:
        ps->cullMode = GraphicsPipelineState::CullBack;
        return true;
    default:
        return false;
    }
}

QSGRhiShaderEffectMaterial::QSGRhiShaderEffectMaterial(QSGRhiShaderEffectNode *node)
    : m_node(node)
{
    setFlag(SupportsRhiShader | Blending | RequiresFullMatrix, true); // may be changed in syncMaterial()
}

QSGRhiShaderEffectMaterial::~QSGRhiShaderEffectMaterial()
{
    delete m_dummyTexture;
}

static bool hasAtlasTexture(const QVector<QSGTextureProvider *> &textureProviders)
{
    for (QSGTextureProvider *tp : textureProviders) {
        if (tp && tp->texture() && tp->texture()->isAtlasTexture())
            return true;
    }
    return false;
}

int QSGRhiShaderEffectMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    const QSGRhiShaderEffectMaterial *o = static_cast<const QSGRhiShaderEffectMaterial *>(other);

    if (int diff = m_cullMode - o->m_cullMode)
        return diff;

    if (int diff = m_textureProviders.count() - o->m_textureProviders.count())
        return diff;

    if (m_linker.m_constants != o->m_linker.m_constants)
        return 1;

    if (hasAtlasTexture(m_textureProviders) && !m_geometryUsesTextureSubRect)
        return -1;

    if (hasAtlasTexture(o->m_textureProviders) && !o->m_geometryUsesTextureSubRect)
        return 1;

    for (int binding = 0, count = m_textureProviders.count(); binding != count; ++binding) {
        QSGTextureProvider *tp1 = m_textureProviders.at(binding);
        QSGTextureProvider *tp2 = o->m_textureProviders.at(binding);
        if (tp1 && tp2) {
            QSGTexture *t1 = tp1->texture();
            QSGTexture *t2 = tp2->texture();
            if (t1 && t2) {
                if (int diff = t1->comparisonKey() - t2->comparisonKey())
                    return diff;
            } else {
                if (!t1 && t2)
                    return -1;
                if (t1 && !t2)
                    return 1;
            }
        } else {
            if (!tp1 && tp2)
                return -1;
            if (tp1 && !tp2)
                return 1;
        }
    }

    return 0;
}

QSGMaterialType *QSGRhiShaderEffectMaterial::type() const
{
    return m_materialType;
}

QSGMaterialShader *QSGRhiShaderEffectMaterial::createShader() const
{
    Q_ASSERT(flags().testFlag(RhiShaderWanted));
    return new QSGRhiShaderEffectMaterialShader(this);
}

void QSGRhiShaderEffectMaterial::updateTextureProviders(bool layoutChange)
{
    if (layoutChange) {
        for (QSGTextureProvider *tp : m_textureProviders) {
            if (tp) {
                QObject::disconnect(tp, SIGNAL(textureChanged()), m_node,
                                    SLOT(handleTextureChange()));
                QObject::disconnect(tp, SIGNAL(destroyed(QObject*)), m_node,
                                    SLOT(handleTextureProviderDestroyed(QObject*)));
            }
        }
        m_textureProviders.fill(nullptr, MAX_BINDINGS);
    }

    for (auto it = m_linker.m_samplers.constBegin(), itEnd = m_linker.m_samplers.constEnd(); it != itEnd; ++it) {
        const int binding = it.key();
        QQuickItem *source = qobject_cast<QQuickItem *>(qvariant_cast<QObject *>(it.value()));
        QSGTextureProvider *newProvider = source && source->isTextureProvider() ? source->textureProvider() : nullptr;
        if (binding >= MAX_BINDINGS) {
            qWarning("Sampler at binding %d exceeds the available ShaderEffect binding slots; ignored",
                     binding);
            continue;
        }
        QSGTextureProvider *&activeProvider(m_textureProviders[binding]);
        if (newProvider != activeProvider) {
            if (activeProvider) {
                QObject::disconnect(activeProvider, SIGNAL(textureChanged()), m_node,
                                    SLOT(handleTextureChange()));
                QObject::disconnect(activeProvider, SIGNAL(destroyed(QObject*)), m_node,
                                    SLOT(handleTextureProviderDestroyed(QObject*)));
            }
            if (newProvider) {
                Q_ASSERT_X(newProvider->thread() == QThread::currentThread(),
                           "QSGRhiShaderEffectMaterial::updateTextureProviders",
                           "Texture provider must belong to the rendering thread");
                QObject::connect(newProvider, SIGNAL(textureChanged()), m_node, SLOT(handleTextureChange()));
                QObject::connect(newProvider, SIGNAL(destroyed(QObject*)), m_node,
                                 SLOT(handleTextureProviderDestroyed(QObject*)));
            } else {
                const char *typeName = source ? source->metaObject()->className() : it.value().typeName();
                qWarning("ShaderEffect: Texture t%d is not assigned a valid texture provider (%s).",
                         binding, typeName);
            }
            activeProvider = newProvider;
        }
    }
}

QSGRhiShaderEffectNode::QSGRhiShaderEffectNode(QSGDefaultRenderContext *rc, QSGRhiGuiThreadShaderEffectManager *mgr)
    : QSGShaderEffectNode(mgr),
      m_rc(rc),
      m_mgr(mgr),
      m_material(this)
{
    setFlag(UsePreprocess, true);
    setMaterial(&m_material);
}

QRectF QSGRhiShaderEffectNode::updateNormalizedTextureSubRect(bool supportsAtlasTextures)
{
    QRectF srcRect(0, 0, 1, 1);
    bool geometryUsesTextureSubRect = false;
    if (supportsAtlasTextures) {
        QSGTextureProvider *tp = nullptr;
        for (int binding = 0, count = m_material.m_textureProviders.count(); binding != count; ++binding) {
            if (QSGTextureProvider *candidate = m_material.m_textureProviders.at(binding)) {
                if (!tp) {
                    tp = candidate;
                } else { // there can only be one...
                    tp = nullptr;
                    break;
                }
            }
        }
        if (tp && tp->texture()) {
            srcRect = tp->texture()->normalizedTextureSubRect();
            geometryUsesTextureSubRect = true;
        }
    }

    if (m_material.m_geometryUsesTextureSubRect != geometryUsesTextureSubRect) {
        m_material.m_geometryUsesTextureSubRect = geometryUsesTextureSubRect;
        markDirty(QSGNode::DirtyMaterial);
    }

    return srcRect;
}

static QShader loadShader(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to find shader" << filename;
        return QShader();
    }
    return QShader::fromSerialized(f.readAll());
}

void QSGRhiShaderEffectNode::syncMaterial(SyncData *syncData)
{
    static QShader defaultVertexShader;
    static QShader defaultFragmentShader;

    if (bool(m_material.flags() & QSGMaterial::Blending) != syncData->blending) {
        m_material.setFlag(QSGMaterial::Blending, syncData->blending);
        markDirty(QSGNode::DirtyMaterial);
    }

    if (m_material.m_cullMode != syncData->cullMode) {
        m_material.m_cullMode = syncData->cullMode;
        markDirty(QSGNode::DirtyMaterial);
    }

    if (syncData->dirty & QSGShaderEffectNode::DirtyShaders) {
        m_material.m_hasCustomVertexShader = syncData->vertex.shader->hasShaderCode;
        if (m_material.m_hasCustomVertexShader) {
            m_material.m_vertexShader = syncData->vertex.shader->shaderInfo.rhiShader;
        } else {
            if (!defaultVertexShader.isValid())
                defaultVertexShader = loadShader(QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/shadereffect.vert.qsb"));
            m_material.m_vertexShader = defaultVertexShader;
        }

        m_material.m_hasCustomFragmentShader = syncData->fragment.shader->hasShaderCode;
        if (m_material.m_hasCustomFragmentShader) {
            m_material.m_fragmentShader = syncData->fragment.shader->shaderInfo.rhiShader;
        } else {
            if (!defaultFragmentShader.isValid())
                defaultFragmentShader = loadShader(QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/shadereffect.frag.qsb"));
            m_material.m_fragmentShader = defaultFragmentShader;
        }

        m_material.m_materialType = shaderMaterialTypeCache.get(m_material.m_vertexShader, m_material.m_fragmentShader);
        m_material.m_linker.reset(m_material.m_vertexShader, m_material.m_fragmentShader);

        if (m_material.m_hasCustomVertexShader) {
            m_material.m_linker.feedConstants(*syncData->vertex.shader);
            m_material.m_linker.feedSamplers(*syncData->vertex.shader);
        } else {
            QSGShaderEffectNode::ShaderData defaultSD;
            defaultSD.shaderInfo.name = QLatin1String("Default ShaderEffect vertex shader");
            defaultSD.shaderInfo.rhiShader = m_material.m_vertexShader;
            defaultSD.shaderInfo.type = QSGGuiThreadShaderEffectManager::ShaderInfo::TypeVertex;

            // { mat4 qt_Matrix; float qt_Opacity; } where only the matrix is used
            QSGGuiThreadShaderEffectManager::ShaderInfo::Variable v;
            v.name = QByteArrayLiteral("qt_Matrix");
            v.offset = 0;
            v.size = 16 * sizeof(float);
            defaultSD.shaderInfo.variables.append(v);
            QSGShaderEffectNode::VariableData vd;
            vd.specialType = QSGShaderEffectNode::VariableData::Matrix;
            defaultSD.varData.append(vd);
            defaultSD.shaderInfo.constantDataSize = (16 + 1) * sizeof(float);
            m_material.m_linker.feedConstants(defaultSD);
        }

        if (m_material.m_hasCustomFragmentShader) {
            m_material.m_linker.feedConstants(*syncData->fragment.shader);
            m_material.m_linker.feedSamplers(*syncData->fragment.shader);
        } else {
            QSGShaderEffectNode::ShaderData defaultSD;
            defaultSD.shaderInfo.name = QLatin1String("Default ShaderEffect fragment shader");
            defaultSD.shaderInfo.rhiShader = m_material.m_fragmentShader;
            defaultSD.shaderInfo.type = QSGGuiThreadShaderEffectManager::ShaderInfo::TypeFragment;

            // { mat4 qt_Matrix; float qt_Opacity; } where only the opacity is used
            QSGGuiThreadShaderEffectManager::ShaderInfo::Variable v;
            v.name = QByteArrayLiteral("qt_Opacity");
            v.offset = 16 * sizeof(float);
            v.size = sizeof(float);
            defaultSD.shaderInfo.variables.append(v);
            QSGShaderEffectNode::VariableData vd;
            vd.specialType = QSGShaderEffectNode::VariableData::Opacity;
            defaultSD.varData.append(vd);

            v.name = QByteArrayLiteral("source");
            v.bindPoint = 1;
            v.type = QSGGuiThreadShaderEffectManager::ShaderInfo::Sampler;
            defaultSD.shaderInfo.variables.append(v);
            for (const QSGShaderEffectNode::VariableData &extVarData : qAsConst(syncData->fragment.shader->varData)) {
                if (extVarData.specialType == QSGShaderEffectNode::VariableData::Source) {
                    vd.value = extVarData.value;
                    break;
                }
            }
            vd.specialType = QSGShaderEffectNode::VariableData::Source;
            defaultSD.varData.append(vd);

            defaultSD.shaderInfo.constantDataSize = (16 + 1) * sizeof(float);

            m_material.m_linker.feedConstants(defaultSD);
            m_material.m_linker.feedSamplers(defaultSD);
        }

        m_material.m_linker.linkTextureSubRects();
        m_material.updateTextureProviders(true);
        markDirty(QSGNode::DirtyMaterial);

    } else  {

        if (syncData->dirty & QSGShaderEffectNode::DirtyShaderConstant) {
            if (!syncData->vertex.dirtyConstants->isEmpty())
                m_material.m_linker.feedConstants(*syncData->vertex.shader, syncData->vertex.dirtyConstants);
            if (!syncData->fragment.dirtyConstants->isEmpty())
                m_material.m_linker.feedConstants(*syncData->fragment.shader, syncData->fragment.dirtyConstants);
            markDirty(QSGNode::DirtyMaterial);
        }

        if (syncData->dirty & QSGShaderEffectNode::DirtyShaderTexture) {
            if (!syncData->vertex.dirtyTextures->isEmpty())
                m_material.m_linker.feedSamplers(*syncData->vertex.shader, syncData->vertex.dirtyTextures);
            if (!syncData->fragment.dirtyTextures->isEmpty())
                m_material.m_linker.feedSamplers(*syncData->fragment.shader, syncData->fragment.dirtyTextures);
            m_material.m_linker.linkTextureSubRects();
            m_material.updateTextureProviders(false);
            markDirty(QSGNode::DirtyMaterial);
        }
    }

    if (bool(m_material.flags() & QSGMaterial::RequiresFullMatrix) != m_material.m_hasCustomVertexShader) {
        m_material.setFlag(QSGMaterial::RequiresFullMatrix, m_material.m_hasCustomVertexShader);
        markDirty(QSGNode::DirtyMaterial);
    }
}

void QSGRhiShaderEffectNode::handleTextureChange()
{
    markDirty(QSGNode::DirtyMaterial);
    emit m_mgr->textureChanged();
}

void QSGRhiShaderEffectNode::handleTextureProviderDestroyed(QObject *object)
{
    for (QSGTextureProvider *&tp : m_material.m_textureProviders) {
        if (tp == object)
            tp = nullptr;
    }
}

void QSGRhiShaderEffectNode::preprocess()
{
    for (QSGTextureProvider *tp : m_material.m_textureProviders) {
        if (tp) {
            if (QSGDynamicTexture *texture = qobject_cast<QSGDynamicTexture *>(tp->texture()))
                texture->updateTexture();
        }
    }
}

void QSGRhiShaderEffectNode::cleanupMaterialTypeCache()
{
    shaderMaterialTypeCache.reset();
}

bool QSGRhiGuiThreadShaderEffectManager::hasSeparateSamplerAndTextureObjects() const
{
    return false; // because SPIR-V and QRhi make it look so, regardless of the underlying API
}

QString QSGRhiGuiThreadShaderEffectManager::log() const
{
    return QString();
}

QSGGuiThreadShaderEffectManager::Status QSGRhiGuiThreadShaderEffectManager::status() const
{
    return m_status;
}

void QSGRhiGuiThreadShaderEffectManager::prepareShaderCode(ShaderInfo::Type typeHint, const QByteArray &src, ShaderInfo *result)
{
    QUrl srcUrl(QString::fromUtf8(src));
    if (!srcUrl.scheme().compare(QLatin1String("qrc"), Qt::CaseInsensitive) || srcUrl.isLocalFile()) {
        if (!m_fileSelector) {
            m_fileSelector = new QFileSelector(this);
            m_fileSelector->setExtraSelectors(QStringList() << QStringLiteral("qsb"));
        }
        const QString fn = m_fileSelector->select(QQmlFile::urlToLocalFileOrQrc(srcUrl));
        QFile f(fn);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning("ShaderEffect: Failed to read %s", qPrintable(fn));
            m_status = Error;
            emit shaderCodePrepared(false, typeHint, src, result);
            emit logAndStatusChanged();
            return;
        }
        const QShader s = QShader::fromSerialized(f.readAll());
        f.close();
        if (!s.isValid()) {
            qWarning("ShaderEffect: Failed to deserialize QShader from %s", qPrintable(fn));
            m_status = Error;
            emit shaderCodePrepared(false, typeHint, src, result);
            emit logAndStatusChanged();
            return;
        }
        result->name = fn;
        result->rhiShader = s;
        const bool ok = reflect(result);
        m_status = ok ? Compiled : Error;
        emit shaderCodePrepared(ok, typeHint, src, result);
        emit logAndStatusChanged();
    } else {
        qWarning("rhi shader effect only supports files (qrc or local) at the moment");
        emit shaderCodePrepared(false, typeHint, src, result);
    }
}

bool QSGRhiGuiThreadShaderEffectManager::reflect(ShaderInfo *result)
{
    switch (result->rhiShader.stage()) {
    case QShader::VertexStage:
        result->type = ShaderInfo::TypeVertex;
        break;
    case QShader::FragmentStage:
        result->type = ShaderInfo::TypeFragment;
        break;
    default:
        result->type = ShaderInfo::TypeOther;
        qWarning("Unsupported shader stage (%d)", result->rhiShader.stage());
        return false;
    }

    const QShaderDescription desc = result->rhiShader.description();
    result->constantDataSize = 0;

    int ubufBinding = -1;
    const QVector<QShaderDescription::UniformBlock> ubufs = desc.uniformBlocks();
    const int ubufCount = ubufs.count();
    for (int i = 0; i < ubufCount; ++i) {
        const QShaderDescription::UniformBlock &ubuf(ubufs[i]);
        if (ubufBinding == -1 && ubuf.binding >= 0) {
            ubufBinding = ubuf.binding;
            result->constantDataSize = ubuf.size;
            for (const QShaderDescription::BlockVariable &member : ubuf.members) {
                ShaderInfo::Variable v;
                v.type = ShaderInfo::Constant;
                v.name = member.name.toUtf8();
                v.offset = member.offset;
                v.size = member.size;
                result->variables.append(v);
            }
        } else {
            qWarning("Uniform block %s (binding %d) ignored", qPrintable(ubuf.blockName), ubuf.binding);
        }
    }

    const QVector<QShaderDescription::InOutVariable> combinedImageSamplers = desc.combinedImageSamplers();
    const int samplerCount = combinedImageSamplers.count();
    for (int i = 0; i < samplerCount; ++i) {
        const QShaderDescription::InOutVariable &combinedImageSampler(combinedImageSamplers[i]);
        ShaderInfo::Variable v;
        v.type = ShaderInfo::Sampler;
        v.name = combinedImageSampler.name.toUtf8();
        v.bindPoint = combinedImageSampler.binding;
        result->variables.append(v);
    }

    return true;
}

QT_END_NAMESPACE
