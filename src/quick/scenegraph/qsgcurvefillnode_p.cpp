// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgcurvefillnode_p_p.h"
#include "qsgcurvefillnode_p.h"
#include "util/qsggradientcache_p.h"

#include <private/qsgtexture_p.h>
#include <private/qsgplaintexture_p.h>

QT_BEGIN_NAMESPACE

namespace {

    class QSGCurveFillMaterialShader : public QSGMaterialShader
    {
    public:
        QSGCurveFillMaterialShader(QGradient::Type gradientType,
                                   bool useTextureFill,
                                   bool useDerivatives,
                                   int viewCount);

        bool updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
        void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    };

    QSGCurveFillMaterialShader::QSGCurveFillMaterialShader(QGradient::Type gradientType,
                                                           bool useTextureFill,
                                                           bool useDerivatives,
                                                           int viewCount)
    {
        QString baseName = QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/shapecurve");

        if (gradientType == QGradient::LinearGradient) {
            baseName += QStringLiteral("_lg");
        } else if (gradientType == QGradient::RadialGradient) {
            baseName += QStringLiteral("_rg");
        } else if (gradientType == QGradient::ConicalGradient) {
            baseName += QStringLiteral("_cg");
        } else if (useTextureFill) {
            baseName += QStringLiteral("_tf");
        }

        if (useDerivatives)
            baseName += QStringLiteral("_derivatives");

        setShaderFileName(VertexStage, baseName + QStringLiteral(".vert.qsb"), viewCount);
        setShaderFileName(FragmentStage, baseName + QStringLiteral(".frag.qsb"), viewCount);
    }

    void QSGCurveFillMaterialShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                        QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
    {
        Q_UNUSED(oldMaterial);
        QSGCurveFillMaterial *m = static_cast<QSGCurveFillMaterial *>(newMaterial);
        const QSGCurveFillNode *node = m->node();
        if (binding != 1
            || (node->gradientType() == QGradient::NoGradient && node->fillTextureProvider() == nullptr)) {
            return;
        }

        QSGTexture *t = nullptr;
        if (node->gradientType() != QGradient::NoGradient) {
            const QSGGradientCacheKey cacheKey(node->fillGradient()->stops,
                                               node->fillGradient()->spread);
            t = QSGGradientCache::cacheForRhi(state.rhi())->get(cacheKey);
        } else if (node->fillTextureProvider() != nullptr) {
            t = node->fillTextureProvider()->texture();
            if (t != nullptr && t->isAtlasTexture()) {
                // Create a non-atlas copy to make texture coordinate wrapping work. This
                // texture copy is owned by the QSGTexture so memory is managed with the original
                // texture provider.
                QSGTexture *newTexture = t->removedFromAtlas(state.resourceUpdateBatch());
                if (newTexture != nullptr)
                    t = newTexture;
            }

        }

        if (t != nullptr) {
            t->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
        } else {
            if (m->dummyTexture() == nullptr) {
                QSGPlainTexture *dummyTexture = new QSGPlainTexture;
                dummyTexture->setFiltering(QSGTexture::Nearest);
                dummyTexture->setHorizontalWrapMode(QSGTexture::Repeat);
                dummyTexture->setVerticalWrapMode(QSGTexture::Repeat);
                QImage img(128, 128, QImage::Format_ARGB32_Premultiplied);
                img.fill(0);
                dummyTexture->setImage(img);
                dummyTexture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());

                m->setDummyTexture(dummyTexture);
            }

            t = m->dummyTexture();
        }

        *texture = t;
    }

    bool QSGCurveFillMaterialShader::updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
    {
        bool changed = false;
        QByteArray *buf = state.uniformData();
        Q_ASSERT(buf->size() >= 80);
        const int matrixCount = qMin(state.projectionMatrixCount(), newEffect->viewCount());

        int offset = 0;
        float matrixScale = 0.0f;
        if (state.isMatrixDirty()) {
            for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
                const QMatrix4x4 m = state.combinedMatrix(viewIndex);
                memcpy(buf->data() + offset + viewIndex * 64, m.constData(), 64);
            }

            matrixScale = qSqrt(qAbs(state.determinant()));
            memcpy(buf->data() + offset + newEffect->viewCount() * 64, &matrixScale, 4);

            changed = true;
        }
        offset += newEffect->viewCount() * 64 + 4;

        if (state.isOpacityDirty()) {
            const float opacity = state.opacity();
            memcpy(buf->data() + offset, &opacity, 4);
            changed = true;
        }
        offset += 4;

        QSGCurveFillMaterial *newMaterial = static_cast<QSGCurveFillMaterial *>(newEffect);
        QSGCurveFillMaterial *oldMaterial = static_cast<QSGCurveFillMaterial *>(oldEffect);

        QSGCurveFillNode *newNode = newMaterial != nullptr ? newMaterial->node() : nullptr;
        QSGCurveFillNode *oldNode = oldMaterial != nullptr ? oldMaterial->node() : nullptr;

        if (newNode == nullptr)
            return changed;

        if (oldNode == nullptr || oldNode->debug() != newNode->debug()) {
            float debug = newNode->debug();
            memcpy(buf->data() + offset, &debug, 4);
            changed = true;
        }
        offset += 8;

        if (newNode->gradientType() == QGradient::NoGradient
            && newNode->fillTextureProvider() == nullptr) {
            Q_ASSERT(buf->size() >= offset + 16);

            QVector4D newColor = QVector4D(newNode->color().redF(),
                                           newNode->color().greenF(),
                                           newNode->color().blueF(),
                                           newNode->color().alphaF());
            QVector4D oldColor = oldNode != nullptr
                    ? QVector4D(oldNode->color().redF(),
                                oldNode->color().greenF(),
                                oldNode->color().blueF(),
                                oldNode->color().alphaF())
                    : QVector4D{};

            if (oldNode == nullptr || oldColor != newColor) {
                memcpy(buf->data() + offset, &newColor, 16);
                changed = true;
            }

            offset += 16;
        } else {
            Q_ASSERT(buf->size() >= offset + 64);

            if (!oldNode || *oldNode->fillTransform() != *newNode->fillTransform()) {
                memcpy(buf->data() + offset, newNode->fillTransform()->invertedData(), 64);
                changed = true;
            }

            offset += 64;
        }

        if (newNode->gradientType() == QGradient::NoGradient
            && newNode->fillTextureProvider() != nullptr) {
            Q_ASSERT(buf->size() >= offset + 8);
            const QSizeF newTextureSize = newNode->fillTextureProvider()->texture() != nullptr
                                             ? newNode->fillTextureProvider()->texture()->textureSize()
                                             : QSizeF(0, 0);
            const QVector2D newBoundsSize(newTextureSize.width() / state.devicePixelRatio(),
                                          newTextureSize.height() / state.devicePixelRatio());
            const QVector2D oldBoundsSize = oldNode != nullptr
                                            ? oldNode->boundsSize()
                                            : QVector2D{};

            if (oldEffect == nullptr || newBoundsSize != oldBoundsSize) {
                newNode->setBoundsSize(newBoundsSize);
                memcpy(buf->data() + offset, &newBoundsSize, 8);
                changed = true;
            }
            offset += 8;

        } else if (newNode->gradientType() == QGradient::LinearGradient) {
            Q_ASSERT(buf->size() >= offset + 8 + 8);

            QVector2D newGradientStart = QVector2D(newNode->fillGradient()->a);
            QVector2D oldGradientStart = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient()->a)
                    : QVector2D{};

            if (newGradientStart != oldGradientStart || oldEffect == nullptr) {
                memcpy(buf->data() + offset, &newGradientStart, 8);
                changed = true;
            }
            offset += 8;

            QVector2D newGradientEnd = QVector2D(newNode->fillGradient()->b);
            QVector2D oldGradientEnd = oldNode!= nullptr
                    ? QVector2D(oldNode->fillGradient()->b)
                    : QVector2D{};

            if (newGradientEnd != oldGradientEnd || oldEffect == nullptr) {
                memcpy(buf->data() + offset, &newGradientEnd, 8);
                changed = true;
            }

            offset += 8;
        } else if (newNode->gradientType() == QGradient::RadialGradient) {
            Q_ASSERT(buf->size() >= offset + 8 + 8 + 4 + 4);

            QVector2D newFocalPoint = QVector2D(newNode->fillGradient()->b);
            QVector2D oldFocalPoint = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient()->b)
                    : QVector2D{};
            if (oldNode == nullptr || newFocalPoint != oldFocalPoint) {
                memcpy(buf->data() + offset, &newFocalPoint, 8);
                changed = true;
            }
            offset += 8;

            QVector2D newCenterPoint = QVector2D(newNode->fillGradient()->a);
            QVector2D oldCenterPoint = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient()->a)
                    : QVector2D{};

            QVector2D newCenterToFocal = newCenterPoint - newFocalPoint;
            QVector2D oldCenterToFocal = oldCenterPoint - oldFocalPoint;
            if (oldNode == nullptr || newCenterToFocal != oldCenterToFocal) {
                memcpy(buf->data() + offset, &newCenterToFocal, 8);
                changed = true;
            }
            offset += 8;

            float newCenterRadius = newNode->fillGradient()->v0;
            float oldCenterRadius = oldNode != nullptr
                    ? oldNode->fillGradient()->v0
                    : 0.0f;
            if (oldNode == nullptr || !qFuzzyCompare(newCenterRadius, oldCenterRadius)) {
                memcpy(buf->data() + offset, &newCenterRadius, 4);
                changed = true;
            }
            offset += 4;

            float newFocalRadius = newNode->fillGradient()->v1;
            float oldFocalRadius = oldNode != nullptr
                    ? oldNode->fillGradient()->v1
                    : 0.0f;
            if (oldNode == nullptr || !qFuzzyCompare(newFocalRadius, oldFocalRadius)) {
                memcpy(buf->data() + offset, &newFocalRadius, 4);
                changed = true;
            }
            offset += 4;

        } else if (newNode->gradientType() == QGradient::ConicalGradient) {
            Q_ASSERT(buf->size() >= offset + 8 + 4);

            QVector2D newFocalPoint = QVector2D(newNode->fillGradient()->a);
            QVector2D oldFocalPoint = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient()->a)
                    : QVector2D{};
            if (oldNode == nullptr || newFocalPoint != oldFocalPoint) {
                memcpy(buf->data() + offset, &newFocalPoint, 8);
                changed = true;
            }
            offset += 8;

            float newAngle = newNode->fillGradient()->v0;
            float oldAngle = oldNode != nullptr
                    ? oldNode->fillGradient()->v0
                    : 0.0f;
            if (oldNode == nullptr || !qFuzzyCompare(newAngle, oldAngle)) {
                newAngle = -qDegreesToRadians(newAngle);
                memcpy(buf->data() + offset, &newAngle, 4);
                changed = true;
            }
            offset += 4;
        }

        return changed;
    }

}

QSGCurveFillMaterial::QSGCurveFillMaterial(QSGCurveFillNode *node)
    : m_node(node)
{
    setFlag(Blending, true);
    setFlag(RequiresDeterminant, true);
}

QSGCurveFillMaterial::~QSGCurveFillMaterial()
{
    delete m_dummyTexture;
}

int QSGCurveFillMaterial::compare(const QSGMaterial *other) const
{
    if (other->type() != type())
        return (type() - other->type());

    const QSGCurveFillMaterial *otherMaterial =
            static_cast<const QSGCurveFillMaterial *>(other);

    QSGCurveFillNode *a = node();
    QSGCurveFillNode *b = otherMaterial->node();
    if (a == b)
        return 0;

    if (a->gradientType() == QGradient::NoGradient && a->fillTextureProvider() == nullptr) {
        if (int d = a->color().red() - b->color().red())
            return d;
        if (int d = a->color().green() - b->color().green())
            return d;
        if (int d = a->color().blue() - b->color().blue())
            return d;
        if (int d = a->color().alpha() - b->color().alpha())
            return d;
    } else {
        if (a->gradientType() != QGradient::NoGradient) {
            const QSGGradientCache::GradientDesc &ga = *a->fillGradient();
            const QSGGradientCache::GradientDesc &gb = *b->fillGradient();

            if (int d = ga.a.x() - gb.a.x())
                return d;
            if (int d = ga.a.y() - gb.a.y())
                return d;
            if (int d = ga.b.x() - gb.b.x())
                return d;
            if (int d = ga.b.y() - gb.b.y())
                return d;

            if (int d = ga.v0 - gb.v0)
                return d;
            if (int d = ga.v1 - gb.v1)
                return d;

            if (int d = ga.spread - gb.spread)
                return d;

            if (int d = ga.stops.size() - gb.stops.size())
                return d;

            for (int i = 0; i < ga.stops.size(); ++i) {
                if (int d = ga.stops[i].first - gb.stops[i].first)
                    return d;
                if (int d = ga.stops[i].second.rgba() - gb.stops[i].second.rgba())
                    return d;
            }
        }

        if (int d = a->fillTransform()->compareTo(*b->fillTransform()))
            return d;
    }

    const qintptr diff = qintptr(a->fillTextureProvider()) - qintptr(b->fillTextureProvider());
    return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
}

QSGMaterialType *QSGCurveFillMaterial::type() const
{
    static QSGMaterialType type[5];
    uint index = node()->gradientType();
    Q_ASSERT((index & ~3) == 0); // Only two first bits for gradient type

    if (node()->gradientType() == QGradient::NoGradient && node()->fillTextureProvider() != nullptr)
        index = 5;

    return &type[index];
}

QSGMaterialShader *QSGCurveFillMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    return new QSGCurveFillMaterialShader(node()->gradientType(),
                                          node()->gradientType() == QGradient::NoGradient
                                              && node()->fillTextureProvider() != nullptr,
                                          renderMode == QSGRendererInterface::RenderMode3D,
                                          viewCount());
}

QT_END_NAMESPACE
