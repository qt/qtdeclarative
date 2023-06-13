// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgrhishadereffectnode_p.h"
#include "qsgdefaultrendercontext_p.h"
#include "qsgrhisupport_p.h"
#include <qsgmaterialshader.h>
#include <qsgtextureprovider.h>
#include <private/qsgplaintexture_p.h>
#include <rhi/qshaderdescription.h>
#include <QQmlFile>
#include <QFile>
#include <QFileSelector>
#include <QMutexLocker>

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
    m_subRectBindings.clear();

    m_constants.reserve(8);
    m_samplers.reserve(4);
    m_samplerNameMap.reserve(4);
}

void QSGRhiShaderLinker::feedConstants(const QSGShaderEffectNode::ShaderData &shader, const QSet<int> *dirtyIndices)
{
    Q_ASSERT(shader.shaderInfo.variables.size() == shader.varData.size());
    static bool shaderEffectDebug = qEnvironmentVariableIntValue("QSG_RHI_SHADEREFFECT_DEBUG");
    if (!dirtyIndices) {
        m_constantBufferSize = qMax(m_constantBufferSize, shader.shaderInfo.constantDataSize);
        for (int i = 0; i < shader.shaderInfo.variables.size(); ++i) {
            const QSGGuiThreadShaderEffectManager::ShaderInfo::Variable &var(shader.shaderInfo.variables.at(i));
            if (var.type == QSGGuiThreadShaderEffectManager::ShaderInfo::Constant) {
                const QSGShaderEffectNode::VariableData &vd(shader.varData.at(i));
                Constant c;
                c.size = var.size;
                c.specialType = vd.specialType;
                if (c.specialType != QSGShaderEffectNode::VariableData::SubRect) {
                    c.value = vd.value;
                    if (shaderEffectDebug) {
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
            if (shaderEffectDebug) {
                qDebug() << "cbuf update" << shader.shaderInfo.name
                         << "offset" << offset << "value" << value;
            }
        }
    }
}

void QSGRhiShaderLinker::feedSamplers(const QSGShaderEffectNode::ShaderData &shader, const QSet<int> *dirtyIndices)
{
    if (!dirtyIndices) {
        for (int i = 0; i < shader.shaderInfo.variables.size(); ++i) {
            const QSGGuiThreadShaderEffectManager::ShaderInfo::Variable &var(shader.shaderInfo.variables.at(i));
            const QSGShaderEffectNode::VariableData &vd(shader.varData.at(i));
            if (var.type == QSGGuiThreadShaderEffectManager::ShaderInfo::Sampler) {
                Q_ASSERT(vd.specialType == QSGShaderEffectNode::VariableData::Source);

#ifndef QT_NO_DEBUG
                int existingBindPoint = m_samplerNameMap.value(var.name, -1);
                Q_ASSERT(existingBindPoint < 0 || existingBindPoint == var.bindPoint);
#endif

                m_samplers.insert(var.bindPoint, vd.value);
                m_samplerNameMap.insert(var.name, var.bindPoint);
            }
        }
    } else {
        for (int idx : *dirtyIndices) {
            const QSGGuiThreadShaderEffectManager::ShaderInfo::Variable &var(shader.shaderInfo.variables.at(idx));
            const QSGShaderEffectNode::VariableData &vd(shader.varData.at(idx));

#ifndef QT_NO_DEBUG
            int existingBindPoint = m_samplerNameMap.value(var.name, -1);
            Q_ASSERT(existingBindPoint < 0 || existingBindPoint == var.bindPoint);
#endif

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
                const int binding = m_samplerNameMap[name];
                c.value = binding;
                m_subRectBindings.insert(binding);
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
    QSGMaterialType *ref(const QShader &vs, const QShader &fs);
    void unref(const QShader &vs, const QShader &fs);
    void reset() {
        for (auto it = m_types.begin(), end = m_types.end(); it != end; ++it)
            delete it->type;
        m_types.clear();
        clearGraveyard();
    }
    void clearGraveyard() {
        qDeleteAll(m_graveyard);
        m_graveyard.clear();
    }
    struct Key {
        QShader vs;
        QShader fs;
        size_t hash;
        Key(const QShader &vs, const QShader &fs)
            : vs(vs),
              fs(fs)
        {
            QtPrivate::QHashCombine hashGen;
            hash = hashGen(hashGen(0, vs), fs);
        }
        bool operator==(const Key &other) const {
            return vs == other.vs && fs == other.fs;
        }
    };
    struct MaterialType {
        int ref;
        QSGMaterialType *type;
    };
    QHash<Key, MaterialType> m_types;
    QVector<QSGMaterialType *> m_graveyard;
};

size_t qHash(const QSGRhiShaderMaterialTypeCache::Key &key, size_t seed = 0)
{
    return seed ^ key.hash;
}

static QMutex shaderMaterialTypeCacheMutex;

QSGMaterialType *QSGRhiShaderMaterialTypeCache::ref(const QShader &vs, const QShader &fs)
{
    QMutexLocker lock(&shaderMaterialTypeCacheMutex);
    const Key k(vs, fs);
    auto it = m_types.find(k);
    if (it != m_types.end()) {
        it->ref += 1;
        return it->type;
    }

    QSGMaterialType *t = new QSGMaterialType;
    m_types.insert(k, { 1, t });
    return t;
}

void QSGRhiShaderMaterialTypeCache::unref(const QShader &vs, const QShader &fs)
{
    QMutexLocker lock(&shaderMaterialTypeCacheMutex);
    const Key k(vs, fs);
    auto it = m_types.find(k);
    if (it != m_types.end()) {
        if (!--it->ref) {
            m_graveyard.append(it->type);
            m_types.erase(it);
        }
    }
}

static QHash<void *, QSGRhiShaderMaterialTypeCache> shaderMaterialTypeCache;

void QSGRhiShaderEffectNode::resetMaterialTypeCache(void *materialTypeCacheKey)
{
    QMutexLocker lock(&shaderMaterialTypeCacheMutex);
    shaderMaterialTypeCache[materialTypeCacheKey].reset();
}

void QSGRhiShaderEffectNode::garbageCollectMaterialTypeCache(void *materialTypeCacheKey)
{
    QMutexLocker lock(&shaderMaterialTypeCacheMutex);
    shaderMaterialTypeCache[materialTypeCacheKey].clearGraveyard();
}

class QSGRhiShaderEffectMaterialShader : public QSGMaterialShader
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
    float r, g, b, a;
    c.getRgbF(&r, &g, &b, &a);
    return QColor::fromRgbF(r * a, g * a, b * a, a);
}

template<typename T>
static inline void fillUniformBlockMember(char *dst, const T *value, int valueCount, int fieldSizeBytes)
{
    const size_t valueBytes = sizeof(T) * valueCount;
    const size_t fieldBytes = fieldSizeBytes;
    if (valueBytes <= fieldBytes) {
        memcpy(dst, value, valueBytes);
        if (valueBytes < fieldBytes)
            memset(dst + valueBytes, 0, fieldBytes - valueBytes);
    } else {
        memcpy(dst, value, fieldBytes);
    }
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
                fillUniformBlockMember<float>(dst, &f, 1, c.size);
                changed = true;
            }
        } else if (c.specialType == QSGShaderEffectNode::VariableData::Matrix) {
            if (state.isMatrixDirty()) {
                const QMatrix4x4 m = state.combinedMatrix();
                fillUniformBlockMember<float>(dst, m.constData(), 16, c.size);
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
            fillUniformBlockMember<float>(dst, f, 4, c.size);
        } else if (c.specialType == QSGShaderEffectNode::VariableData::None) {
            changed = true;
            switch (int(c.value.userType())) {
            case QMetaType::QColor: {
                const QColor v = qsg_premultiply_color(qvariant_cast<QColor>(c.value)).toRgb();
                const float f[4] = { float(v.redF()), float(v.greenF()), float(v.blueF()), float(v.alphaF()) };
                fillUniformBlockMember<float>(dst, f, 4, c.size);
                break;
            }
            case QMetaType::Float: {
                const float f = qvariant_cast<float>(c.value);
                fillUniformBlockMember<float>(dst, &f, 1, c.size);
                break;
            }
            case QMetaType::Double: {
                const float f = float(qvariant_cast<double>(c.value));
                fillUniformBlockMember<float>(dst, &f, 1, c.size);
                break;
            }
            case QMetaType::Int: {
                const qint32 i = c.value.toInt();
                fillUniformBlockMember<qint32>(dst, &i, 1, c.size);
                break;
            }
            case QMetaType::Bool: {
                const qint32 b = c.value.toBool();
                fillUniformBlockMember<qint32>(dst, &b, 1, c.size);
                break;
            }
            case QMetaType::QTransform: { // mat3
                const QTransform v = qvariant_cast<QTransform>(c.value);
                const float m[3][3] = {
                    { float(v.m11()), float(v.m12()), float(v.m13()) },
                    { float(v.m21()), float(v.m22()), float(v.m23()) },
                    { float(v.m31()), float(v.m32()), float(v.m33()) }
                };
                // stored as 4 floats per column, 1 unused
                memset(dst, 0, c.size);
                const size_t bytesPerColumn = 4 * sizeof(float);
                if (c.size >= bytesPerColumn)
                    fillUniformBlockMember<float>(dst, m[0], 3, 3 * sizeof(float));
                if (c.size >= 2 * bytesPerColumn)
                    fillUniformBlockMember<float>(dst + bytesPerColumn, m[1], 3, 3 * sizeof(float));
                if (c.size >= 3 * bytesPerColumn)
                    fillUniformBlockMember<float>(dst + 2 * bytesPerColumn, m[2], 3, 3 * sizeof(float));
                break;
            }
            case QMetaType::QSize:
            case QMetaType::QSizeF: { // vec2
                const QSizeF v = c.value.toSizeF();
                const float f[2] = { float(v.width()), float(v.height()) };
                fillUniformBlockMember<float>(dst, f, 2, c.size);
                break;
            }
            case QMetaType::QPoint:
            case QMetaType::QPointF: { // vec2
                const QPointF v = c.value.toPointF();
                const float f[2] = { float(v.x()), float(v.y()) };
                fillUniformBlockMember<float>(dst, f, 2, c.size);
                break;
            }
            case QMetaType::QRect:
            case QMetaType::QRectF: { // vec4
                const QRectF v = c.value.toRectF();
                const float f[4] = { float(v.x()), float(v.y()), float(v.width()), float(v.height()) };
                fillUniformBlockMember<float>(dst, f, 4, c.size);
                break;
            }
            case QMetaType::QVector2D: { // vec2
                const QVector2D v = qvariant_cast<QVector2D>(c.value);
                const float f[2] = { float(v.x()), float(v.y()) };
                fillUniformBlockMember<float>(dst, f, 2, c.size);
                break;
            }
            case QMetaType::QVector3D: { // vec3
                const QVector3D v = qvariant_cast<QVector3D>(c.value);
                const float f[3] = { float(v.x()), float(v.y()), float(v.z()) };
                fillUniformBlockMember<float>(dst, f, 3, c.size);
                break;
            }
            case QMetaType::QVector4D: { // vec4
                const QVector4D v = qvariant_cast<QVector4D>(c.value);
                const float f[4] = { float(v.x()), float(v.y()), float(v.z()), float(v.w()) };
                fillUniformBlockMember<float>(dst, f, 4, c.size);
                break;
            }
            case QMetaType::QQuaternion: { // vec4
                const QQuaternion v = qvariant_cast<QQuaternion>(c.value);
                const float f[4] = { float(v.x()), float(v.y()), float(v.z()), float(v.scalar()) };
                fillUniformBlockMember<float>(dst, f, 4, c.size);
                break;
            }
            case QMetaType::QMatrix4x4: { // mat4
                const QMatrix4x4 m = qvariant_cast<QMatrix4x4>(c.value);
                fillUniformBlockMember<float>(dst, m.constData(), 16, c.size);
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
            t->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());

            if (t->isAtlasTexture() && !mat->m_geometryUsesTextureSubRect && !mat->usesSubRectUniform(binding)) {
                // Why the hassle with the batch: while removedFromAtlas() is
                // able to operate with its own resource update batch (which is
                // then committed immediately), that approach is wrong when the
                // atlas enqueued (in the updateRhiTexture() above) not yet
                // committed operations to state.resourceUpdateBatch()... The
                // only safe way then is to use the same batch the atlas'
                // updateRhiTexture() used.
                QSGTexture *newTexture = t->removedFromAtlas(state.resourceUpdateBatch());
                if (newTexture) {
                    t = newTexture;
                    t->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
                }
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
        mat->m_dummyTexture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
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
    setFlag(Blending | RequiresFullMatrix, true); // may be changed in syncMaterial()
}

QSGRhiShaderEffectMaterial::~QSGRhiShaderEffectMaterial()
{
    if (m_materialType && m_materialTypeCacheKey)
        shaderMaterialTypeCache[m_materialTypeCacheKey].unref(m_vertexShader, m_fragmentShader);

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

    if (int diff = m_textureProviders.size() - o->m_textureProviders.size())
        return diff;

    if (m_linker.m_constants != o->m_linker.m_constants)
        return 1;

    if (hasAtlasTexture(m_textureProviders) && !m_geometryUsesTextureSubRect)
        return -1;

    if (hasAtlasTexture(o->m_textureProviders) && !o->m_geometryUsesTextureSubRect)
        return 1;

    for (int binding = 0, count = m_textureProviders.size(); binding != count; ++binding) {
        QSGTextureProvider *tp1 = m_textureProviders.at(binding);
        QSGTextureProvider *tp2 = o->m_textureProviders.at(binding);
        if (tp1 && tp2) {
            QSGTexture *t1 = tp1->texture();
            QSGTexture *t2 = tp2->texture();
            if (t1 && t2) {
                const qint64 diff = t1->comparisonKey() - t2->comparisonKey();
                if (diff != 0)
                    return diff < 0 ? -1 : 1;
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

QSGMaterialShader *QSGRhiShaderEffectMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
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

QSGRhiShaderEffectNode::QSGRhiShaderEffectNode(QSGDefaultRenderContext *rc)
    : m_material(this)
{
    Q_UNUSED(rc);
    setFlag(UsePreprocess, true);
    setMaterial(&m_material);
}

QRectF QSGRhiShaderEffectNode::updateNormalizedTextureSubRect(bool supportsAtlasTextures)
{
    QRectF srcRect(0, 0, 1, 1);
    bool geometryUsesTextureSubRect = false;
    if (supportsAtlasTextures) {
        QSGTextureProvider *tp = nullptr;
        for (int binding = 0, count = m_material.m_textureProviders.size(); binding != count; ++binding) {
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

static QShader loadShaderFromFile(const QString &filename)
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
        if (m_material.m_materialType) {
            shaderMaterialTypeCache[m_material.m_materialTypeCacheKey].unref(m_material.m_vertexShader,
                                                                             m_material.m_fragmentShader);
        }

        m_material.m_hasCustomVertexShader = syncData->vertex.shader->hasShaderCode;
        if (m_material.m_hasCustomVertexShader) {
            m_material.m_vertexShader = syncData->vertex.shader->shaderInfo.rhiShader;
        } else {
            if (!defaultVertexShader.isValid())
                defaultVertexShader = loadShaderFromFile(QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/shadereffect.vert.qsb"));
            m_material.m_vertexShader = defaultVertexShader;
        }

        m_material.m_hasCustomFragmentShader = syncData->fragment.shader->hasShaderCode;
        if (m_material.m_hasCustomFragmentShader) {
            m_material.m_fragmentShader = syncData->fragment.shader->shaderInfo.rhiShader;
        } else {
            if (!defaultFragmentShader.isValid())
                defaultFragmentShader = loadShaderFromFile(QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/shadereffect.frag.qsb"));
            m_material.m_fragmentShader = defaultFragmentShader;
        }

        m_material.m_materialType = shaderMaterialTypeCache[syncData->materialTypeCacheKey].ref(m_material.m_vertexShader,
                                                                                                m_material.m_fragmentShader);
        m_material.m_materialTypeCacheKey = syncData->materialTypeCacheKey;

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
            for (const QSGShaderEffectNode::VariableData &extVarData : std::as_const(syncData->fragment.shader->varData)) {
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
    emit textureChanged();
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

void QSGRhiGuiThreadShaderEffectManager::prepareShaderCode(ShaderInfo::Type typeHint, const QUrl &src, ShaderInfo *result)
{
    if (!src.scheme().compare(QLatin1String("qrc"), Qt::CaseInsensitive) || src.isLocalFile()) {
        if (!m_fileSelector) {
            m_fileSelector = new QFileSelector(this);
            m_fileSelector->setExtraSelectors(QStringList() << QStringLiteral("qsb"));
        }
        const QString fn = m_fileSelector->select(QQmlFile::urlToLocalFileOrQrc(src));
        const QShader s = loadShaderFromFile(fn);
        if (!s.isValid()) {
            qWarning("ShaderEffect: Failed to deserialize QShader from %s. "
                     "Either the filename is incorrect, or it is not a valid .qsb file. "
                     "In Qt 6 shaders must be preprocessed using the Qt Shader Tools infrastructure. "
                     "The vertexShader and fragmentShader properties are now URLs that are expected to point to .qsb files generated by the qsb tool. "
                     "See https://doc.qt.io/qt-6/qtshadertools-index.html for more information.",
                     qPrintable(fn));
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
    const int ubufCount = ubufs.size();
    for (int i = 0; i < ubufCount; ++i) {
        const QShaderDescription::UniformBlock &ubuf(ubufs[i]);
        if (ubufBinding == -1 && ubuf.binding >= 0) {
            ubufBinding = ubuf.binding;
            result->constantDataSize = ubuf.size;
            for (const QShaderDescription::BlockVariable &member : ubuf.members) {
                ShaderInfo::Variable v;
                v.type = ShaderInfo::Constant;
                v.name = member.name;
                v.offset = member.offset;
                v.size = member.size;
                result->variables.append(v);
            }
        } else {
            qWarning("Uniform block %s (binding %d) ignored", ubuf.blockName.constData(),
                     ubuf.binding);
        }
    }

    const QVector<QShaderDescription::InOutVariable> combinedImageSamplers = desc.combinedImageSamplers();
    const int samplerCount = combinedImageSamplers.size();
    for (int i = 0; i < samplerCount; ++i) {
        const QShaderDescription::InOutVariable &combinedImageSampler(combinedImageSamplers[i]);
        ShaderInfo::Variable v;
        v.type = ShaderInfo::Sampler;
        v.name = combinedImageSampler.name;
        v.bindPoint = combinedImageSampler.binding;
        result->variables.append(v);
    }

    return true;
}

QT_END_NAMESPACE

#include "moc_qsgrhishadereffectnode_p.cpp"
