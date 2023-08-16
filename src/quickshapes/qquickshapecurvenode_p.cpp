// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshapecurvenode_p_p.h"
#include "qquickshapecurvenode_p.h"

#include "qquickshapegenericrenderer_p.h"

QT_BEGIN_NAMESPACE

namespace {

    class QQuickShapeCurveMaterialShader : public QSGMaterialShader
    {
    public:
        QQuickShapeCurveMaterialShader(QQuickAbstractPathRenderer::FillGradientType gradientType,
                                       bool includeStroke,
                                       bool useDerivatives);

        bool updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
        void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    };

    QQuickShapeCurveMaterialShader::QQuickShapeCurveMaterialShader(QQuickAbstractPathRenderer::FillGradientType gradientType,
                                                                   bool includeStroke,
                                                                   bool useDerivatives)
    {
        QString baseName = QStringLiteral(":/qt-project.org/shapes/shaders_ng/shapecurve");

        if (gradientType == QQuickAbstractPathRenderer::LinearGradient) {
            baseName += QStringLiteral("_lg");
        } else if (gradientType == QQuickAbstractPathRenderer::RadialGradient) {
            baseName += QStringLiteral("_rg");
        } else if (gradientType == QQuickAbstractPathRenderer::ConicalGradient) {
            baseName += QStringLiteral("_cg");
        }

        if (includeStroke)
            baseName += QStringLiteral("_stroke");

        if (useDerivatives)
            baseName += QStringLiteral("_derivatives");

        setShaderFileName(VertexStage, baseName + QStringLiteral(".vert.qsb"));
        setShaderFileName(FragmentStage, baseName + QStringLiteral(".frag.qsb"));
    }

    void QQuickShapeCurveMaterialShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
    {
        Q_UNUSED(oldMaterial);
        const QQuickShapeCurveMaterial *m = static_cast<QQuickShapeCurveMaterial *>(newMaterial);
        const QQuickShapeCurveNode *node = m->node();
        if (binding != 1 || node->gradientType() == QQuickAbstractPathRenderer::NoGradient)
            return;

        const QQuickShapeGradientCacheKey cacheKey(node->fillGradient().stops,
                                                   node->fillGradient().spread);
        QSGTexture *t = QQuickShapeGradientCache::cacheForRhi(state.rhi())->get(cacheKey);
        t->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
        *texture = t;
    }

    bool QQuickShapeCurveMaterialShader::updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
    {
        bool changed = false;
        QByteArray *buf = state.uniformData();
        Q_ASSERT(buf->size() >= 80);

        int offset = 0;
        float matrixScale = 0.0f;
        if (state.isMatrixDirty()) {
            const QMatrix4x4 m = state.combinedMatrix();

            memcpy(buf->data() + offset, m.constData(), 64);

            matrixScale = qSqrt(qAbs(state.determinant()));
            memcpy(buf->data() + offset + 64, &matrixScale, 4);

            changed = true;
        }
        offset += 68;

        if (state.isOpacityDirty()) {
            const float opacity = state.opacity();
            memcpy(buf->data() + offset, &opacity, 4);
            changed = true;
        }
        offset += 4;

        QQuickShapeCurveMaterial *newMaterial = static_cast<QQuickShapeCurveMaterial *>(newEffect);
        QQuickShapeCurveMaterial *oldMaterial = static_cast<QQuickShapeCurveMaterial *>(oldEffect);

        QQuickShapeCurveNode *newNode = newMaterial != nullptr ? newMaterial->node() : nullptr;
        QQuickShapeCurveNode *oldNode = oldMaterial != nullptr ? oldMaterial->node() : nullptr;

        if (newNode == nullptr)
            return changed;

        if (oldNode == nullptr || oldNode->debug() != newNode->debug()) {
            float debug = newNode->debug();
            memcpy(buf->data() + offset, &debug, 4);
            changed = true;
        }
        offset += 8;

        if (newNode->hasStroke()) {
            Q_ASSERT(buf->size() >= offset + 32);
            QVector4D newStrokeColor(newNode->strokeColor().redF(),
                                     newNode->strokeColor().greenF(),
                                     newNode->strokeColor().blueF(),
                                     newNode->strokeColor().alphaF());
            QVector4D oldStrokeColor = oldNode != nullptr
                    ? QVector4D(oldNode->strokeColor().redF(),
                                oldNode->strokeColor().greenF(),
                                oldNode->strokeColor().blueF(),
                                oldNode->strokeColor().alphaF())
                    : QVector4D{};

            if (oldNode == nullptr || oldStrokeColor != newStrokeColor) {
                memcpy(buf->data() + offset, &newStrokeColor, 16);
                changed = true;
            }
            offset += 16;

            if (oldNode == nullptr
                    || !qFuzzyCompare(newNode->strokeWidth(), oldNode->strokeWidth())
                    || (state.isMatrixDirty() && newNode->strokeWidth() > 0.0f)) {
                float w = newNode->strokeWidth() * matrixScale; // matrixScale calculated earlier
                memcpy(buf->data() + offset, &w, 4);
                changed = true;
            }
            offset += 16;
        }

        if (newNode->gradientType() == QQuickAbstractPathRenderer::NoGradient) {
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
        } else if (newNode->gradientType() == QQuickAbstractPathRenderer::LinearGradient) {
            Q_ASSERT(buf->size() >= offset + 8 + 8);

            QVector2D newGradientStart = QVector2D(newNode->fillGradient().a);
            QVector2D oldGradientStart = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient().a)
                    : QVector2D{};

            if (newGradientStart != oldGradientStart || oldEffect == nullptr) {
                memcpy(buf->data() + offset, &newGradientStart, 8);
                changed = true;
            }
            offset += 8;

            QVector2D newGradientEnd = QVector2D(newNode->fillGradient().b);
            QVector2D oldGradientEnd = oldNode!= nullptr
                    ? QVector2D(oldNode->fillGradient().b)
                    : QVector2D{};

            if (newGradientEnd != oldGradientEnd || oldEffect == nullptr) {
                memcpy(buf->data() + offset, &newGradientEnd, 8);
                changed = true;
            }

            offset += 8;
        } else if (newNode->gradientType() == QQuickAbstractPathRenderer::RadialGradient) {
            Q_ASSERT(buf->size() >= offset + 8 + 8 + 4 + 4);

            QVector2D newFocalPoint = QVector2D(newNode->fillGradient().b);
            QVector2D oldFocalPoint = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient().b)
                    : QVector2D{};
            if (oldNode == nullptr || newFocalPoint != oldFocalPoint) {
                memcpy(buf->data() + offset, &newFocalPoint, 8);
                changed = true;
            }
            offset += 8;

            QVector2D newCenterPoint = QVector2D(newNode->fillGradient().a);
            QVector2D oldCenterPoint = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient().a)
                    : QVector2D{};

            QVector2D newCenterToFocal = newCenterPoint - newFocalPoint;
            QVector2D oldCenterToFocal = oldCenterPoint - oldFocalPoint;
            if (oldNode == nullptr || newCenterToFocal != oldCenterToFocal) {
                memcpy(buf->data() + offset, &newCenterToFocal, 8);
                changed = true;
            }
            offset += 8;

            float newCenterRadius = newNode->fillGradient().v0;
            float oldCenterRadius = oldNode != nullptr
                    ? oldNode->fillGradient().v0
                    : 0.0f;
            if (oldNode == nullptr || !qFuzzyCompare(newCenterRadius, oldCenterRadius)) {
                memcpy(buf->data() + offset, &newCenterRadius, 4);
                changed = true;
            }
            offset += 4;

            float newFocalRadius = newNode->fillGradient().v1;
            float oldFocalRadius = oldNode != nullptr
                    ? oldNode->fillGradient().v1
                    : 0.0f;
            if (oldNode == nullptr || !qFuzzyCompare(newFocalRadius, oldFocalRadius)) {
                memcpy(buf->data() + offset, &newFocalRadius, 4);
                changed = true;
            }
            offset += 4;

        } else if (newNode->gradientType() == QQuickAbstractPathRenderer::ConicalGradient) {
            Q_ASSERT(buf->size() >= offset + 8 + 4);

            QVector2D newFocalPoint = QVector2D(newNode->fillGradient().a);
            QVector2D oldFocalPoint = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient().a)
                    : QVector2D{};
            if (oldNode == nullptr || newFocalPoint != oldFocalPoint) {
                memcpy(buf->data() + offset, &newFocalPoint, 8);
                changed = true;
            }
            offset += 8;

            float newAngle = newNode->fillGradient().v0;
            float oldAngle = oldNode != nullptr
                    ? oldNode->fillGradient().v0
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

QQuickShapeCurveMaterial::QQuickShapeCurveMaterial(QQuickShapeCurveNode *node)
    : m_node(node)
{
    setFlag(Blending, true);
    setFlag(RequiresDeterminant, true);
}

int QQuickShapeCurveMaterial::compare(const QSGMaterial *other) const
{
    if (other->type() != type())
        return (type() - other->type());

    const QQuickShapeCurveMaterial *otherMaterial =
            static_cast<const QQuickShapeCurveMaterial *>(other);

    QQuickShapeCurveNode *a = node();
    QQuickShapeCurveNode *b = otherMaterial->node();
    if (a == b)
        return 0;

    if (int d = a->strokeColor().rgba() - b->strokeColor().rgba())
        return d;

    if (a->gradientType() == QQuickAbstractPathRenderer::NoGradient) {
        if (int d = a->color().red() - b->color().red())
            return d;
        if (int d = a->color().green() - b->color().green())
            return d;
        if (int d = a->color().blue() - b->color().blue())
            return d;
        if (int d = a->color().alpha() - b->color().alpha())
            return d;
    } else {
        const QQuickAbstractPathRenderer::GradientDesc &ga = a->fillGradient();
        const QQuickAbstractPathRenderer::GradientDesc &gb = b->fillGradient();

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

    return 0;
}

QSGMaterialType *QQuickShapeCurveMaterial::type() const
{
    static QSGMaterialType type[8];
    uint index = node()->gradientType();
    Q_ASSERT((index & ~3) == 0); // Only two first bits for gradient type

    if (node()->hasStroke())
        index |= 4;

    return &type[index];
}

QSGMaterialShader *QQuickShapeCurveMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    return new QQuickShapeCurveMaterialShader(node()->gradientType(),
                                              node()->hasStroke(),
                                              renderMode == QSGRendererInterface::RenderMode3D);
}


QT_END_NAMESPACE
