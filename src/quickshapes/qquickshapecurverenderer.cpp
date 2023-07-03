// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshapecurverenderer_p.h"
#include "qquickshapecurverenderer_p_p.h"
#include "qquickshapegenericrenderer_p.h"

#include <QtGui/qvector2d.h>
#include <QtGui/private/qtriangulator_p.h>
#include <QtGui/private/qtriangulatingstroker_p.h>
#include <QtGui/private/qrhi_p.h>

#include <QtQuick/qsgmaterial.h>

#include <QThread>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcShapeCurveRenderer, "qt.shape.curverenderer");

#if !defined(QQUICKSHAPECURVERENDERER_CONVEX_CHECK_ERROR_MARGIN)
#  define QQUICKSHAPECURVERENDERER_CONVEX_CHECK_ERROR_MARGIN (1.0f / 32.0f)
#endif

#define QQUICKSHAPECURVERENDERER_GRADIENTS

namespace {

    class QQuickShapeWireFrameMaterialShader : public QSGMaterialShader
    {
    public:
        QQuickShapeWireFrameMaterialShader()
        {
            setShaderFileName(VertexStage,
                              QStringLiteral(":/qt-project.org/shapes/shaders_ng/wireframe.vert.qsb"));
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/shapes/shaders_ng/wireframe.frag.qsb"));
        }

        bool updateUniformData(RenderState &state, QSGMaterial *, QSGMaterial *) override
        {
            bool changed = false;
            QByteArray *buf = state.uniformData();
            Q_ASSERT(buf->size() >= 64);

            if (state.isMatrixDirty()) {
                const QMatrix4x4 m = state.combinedMatrix();

                memcpy(buf->data(), m.constData(), 64);
                changed = true;
            }

            return changed;
        }
    };

    class QQuickShapeWireFrameMaterial : public QSGMaterial
    {
    public:
        QQuickShapeWireFrameMaterial()
        {
            setFlag(Blending, true);
        }

        int compare(const QSGMaterial *other) const override
        {
            return (type() - other->type());
        }

    protected:
        QSGMaterialType *type() const override
        {
            static QSGMaterialType t;
            return &t;
        }
        QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override
        {
            return new QQuickShapeWireFrameMaterialShader;
        }

    };

    class QQuickShapeWireFrameNode : public QSGGeometryNode
    {
    public:
        struct WireFrameVertex
        {
            float x, y, u, v, w;
        };

        QQuickShapeWireFrameNode()
        {
            setFlag(OwnsGeometry, true);
            setGeometry(new QSGGeometry(attributes(), 0, 0));
            activateMaterial();
        }

        void activateMaterial()
        {
            m_material.reset(new QQuickShapeWireFrameMaterial);
            setMaterial(m_material.data());
        }

        static const QSGGeometry::AttributeSet &attributes()
        {
            static QSGGeometry::Attribute data[] = {
                QSGGeometry::Attribute::createWithAttributeType(0, 2, QSGGeometry::FloatType, QSGGeometry::PositionAttribute),
                QSGGeometry::Attribute::createWithAttributeType(1, 3, QSGGeometry::FloatType, QSGGeometry::TexCoordAttribute),
            };
            static QSGGeometry::AttributeSet attrs = { 2, sizeof(WireFrameVertex), data };
            return attrs;
        }

    protected:
        QScopedPointer<QQuickShapeWireFrameMaterial> m_material;
    };

    class QQuickShapeLoopBlinnNode;
    class QQuickShapeLoopBlinnMaterial : public QSGMaterial
    {
    public:
        QQuickShapeLoopBlinnMaterial(QQuickShapeLoopBlinnNode *node,
                                     QQuickAbstractPathRenderer::FillGradientType gradientType)
            : m_node(node)
            , m_gradientType(gradientType)
        {
            setFlag(Blending, true);
        }
        int compare(const QSGMaterial *other) const override;

        QQuickShapeLoopBlinnNode *node() const
        {
            return m_node;
        }

    protected:
        QSGMaterialType *type() const override;
        QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;

        QQuickShapeLoopBlinnNode *m_node;
        QQuickAbstractPathRenderer::FillGradientType m_gradientType;
    };

    class QQuickShapeLoopBlinnNode : public QSGGeometryNode
    {
    public:
        QQuickShapeLoopBlinnNode(QQuickAbstractPathRenderer::FillGradientType gradientType);

        struct LoopBlinnVertex
        {
            float x, y, u, v, w;
            float r, g, b, a; // Debug color, mixed in proportion to a
        };
        static const QSGGeometry::AttributeSet &attributes()
        {
            static QSGGeometry::Attribute data[] = {
                QSGGeometry::Attribute::createWithAttributeType(0, 2, QSGGeometry::FloatType, QSGGeometry::PositionAttribute),
                QSGGeometry::Attribute::createWithAttributeType(1, 3, QSGGeometry::FloatType, QSGGeometry::TexCoordAttribute),
                QSGGeometry::Attribute::createWithAttributeType(2, 4, QSGGeometry::FloatType, QSGGeometry::ColorAttribute),
            };
            static QSGGeometry::AttributeSet attrs = { 3, sizeof(LoopBlinnVertex), data };
            return attrs;
        }

        QColor color;
        QColor strokeColor = Qt::transparent;
        float strokeWidth = 0.0f;
        QQuickAbstractPathRenderer::GradientDesc fillGradient;

    protected:
        void activateMaterial(QQuickAbstractPathRenderer::FillGradientType gradientType);

        QScopedPointer<QSGMaterial> m_material;
    };

    class QQuickShapeLoopBlinnMaterialShader : public QSGMaterialShader
    {
    public:
        QQuickShapeLoopBlinnMaterialShader(QQuickAbstractPathRenderer::FillGradientType gradientType,
                                           bool includeStroke);

        bool updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
        void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

    private:
        QQuickAbstractPathRenderer::FillGradientType m_gradientType;
    };

    QQuickShapeLoopBlinnMaterialShader::QQuickShapeLoopBlinnMaterialShader(QQuickAbstractPathRenderer::FillGradientType gradientType,
                                                                           bool includeStroke)
        : m_gradientType(gradientType)
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

        setShaderFileName(VertexStage, baseName + QStringLiteral(".vert.qsb"));
        setShaderFileName(FragmentStage, baseName + QStringLiteral(".frag.qsb"));
    }

    void QQuickShapeLoopBlinnMaterialShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                                QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
    {
        Q_UNUSED(oldMaterial);
        if (binding != 1 || m_gradientType == QQuickAbstractPathRenderer::NoGradient)
            return;

        QQuickShapeLoopBlinnMaterial *m = static_cast<QQuickShapeLoopBlinnMaterial *>(newMaterial);
        QQuickShapeLoopBlinnNode *node = m->node();
        const QQuickShapeGradientCacheKey cacheKey(node->fillGradient.stops, node->fillGradient.spread);
        QSGTexture *t = QQuickShapeGradientCache::cacheForRhi(state.rhi())->get(cacheKey);
        t->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
        *texture = t;
    }

    bool QQuickShapeLoopBlinnMaterialShader::updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
    {
        bool changed = false;
        QByteArray *buf = state.uniformData();
        Q_ASSERT(buf->size() >= 64);

        if (state.isMatrixDirty()) {
            const QMatrix4x4 m = state.combinedMatrix();

            memcpy(buf->data(), m.constData(), 64);
            changed = true;
        }
        int offset = 64;

        QQuickShapeLoopBlinnMaterial *newMaterial = static_cast<QQuickShapeLoopBlinnMaterial *>(newEffect);
        QQuickShapeLoopBlinnMaterial *oldMaterial = static_cast<QQuickShapeLoopBlinnMaterial *>(oldEffect);

        QQuickShapeLoopBlinnNode *newNode = newMaterial != nullptr ? newMaterial->node() : nullptr;
        QQuickShapeLoopBlinnNode *oldNode = oldMaterial != nullptr ? oldMaterial->node() : nullptr;

        if (newNode == nullptr)
            return changed;

        if (newNode->strokeColor.alpha() > 0 && newNode->strokeWidth > 0.0f) {
            QVector4D newStrokeColor(newNode->strokeColor.redF(),
                                     newNode->strokeColor.greenF(),
                                     newNode->strokeColor.blueF(),
                                     newNode->strokeColor.alphaF());
            QVector4D oldStrokeColor = oldNode != nullptr
                    ? QVector4D(oldNode->strokeColor.redF(),
                                oldNode->strokeColor.greenF(),
                                oldNode->strokeColor.blueF(),
                                oldNode->strokeColor.alphaF())
                    : QVector4D{};

            if (oldNode == nullptr || oldStrokeColor != newStrokeColor) {
                memcpy(buf->data() + offset, &newStrokeColor, 16);
                changed = true;
            }
            offset += 16;

            if (oldNode == nullptr || !qFuzzyCompare(newNode->strokeWidth, oldNode->strokeWidth || (state.isMatrixDirty() && newNode->strokeWidth > 0 ))) {
                float matrixScale = qSqrt(qAbs(state.determinant())) * state.devicePixelRatio();
                float w = newNode->strokeWidth * matrixScale;
                memcpy(buf->data() + offset, &w, 4);
                changed = true;
            }
            offset += 16;
        }

        if (m_gradientType == QQuickAbstractPathRenderer::NoGradient) {
            Q_ASSERT(buf->size() >= offset + 16);

            QVector4D newColor = QVector4D(newNode->color.redF(),
                                           newNode->color.greenF(),
                                           newNode->color.blueF(),
                                           newNode->color.alphaF());
            QVector4D oldColor = oldNode != nullptr
                    ? QVector4D(oldNode->color.redF(),
                                oldNode->color.greenF(),
                                oldNode->color.blueF(),
                                oldNode->color.alphaF())
                    : QVector4D{};

            if (oldColor != newColor) {
                memcpy(buf->data() + offset, &newColor, 16);
                changed = true;
            }

            offset += 16;
        } else if (m_gradientType == QQuickAbstractPathRenderer::LinearGradient) {
            Q_ASSERT(buf->size() >= offset + 8 + 8);

            QVector2D newGradientStart = QVector2D(newNode->fillGradient.a);
            QVector2D oldGradientStart = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient.a)
                    : QVector2D{};

            if (newGradientStart != oldGradientStart || oldEffect == nullptr) {
                memcpy(buf->data() + offset, &newGradientStart, 8);
                changed = true;
            }
            offset += 8;

            QVector2D newGradientEnd = QVector2D(newNode->fillGradient.b);
            QVector2D oldGradientEnd = oldNode!= nullptr
                    ? QVector2D(oldNode->fillGradient.b)
                    : QVector2D{};

            if (newGradientEnd != oldGradientEnd || oldEffect == nullptr) {
                memcpy(buf->data() + offset, &newGradientEnd, 8);
                changed = true;
            }

            offset += 8;
        } else if (newNode != nullptr && m_gradientType == QQuickAbstractPathRenderer::RadialGradient) {
            Q_ASSERT(buf->size() >= offset + 8 + 8 + 4 + 4);

            QVector2D newFocalPoint = QVector2D(newNode->fillGradient.b);
            QVector2D oldFocalPoint = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient.b)
                    : QVector2D{};
            if (oldNode == nullptr || newFocalPoint != oldFocalPoint) {
                memcpy(buf->data() + offset, &newFocalPoint, 8);
                changed = true;
            }
            offset += 8;

            QVector2D newCenterPoint = QVector2D(newNode->fillGradient.a);
            QVector2D oldCenterPoint = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient.a)
                    : QVector2D{};

            QVector2D newCenterToFocal = newCenterPoint - newFocalPoint;
            QVector2D oldCenterToFocal = oldCenterPoint - oldFocalPoint;
            if (oldNode == nullptr || newCenterToFocal != oldCenterToFocal) {
                memcpy(buf->data() + offset, &newCenterToFocal, 8);
                changed = true;
            }
            offset += 8;

            float newCenterRadius = newNode->fillGradient.v0;
            float oldCenterRadius = oldNode != nullptr
                    ? oldNode->fillGradient.v0
                    : 0.0f;
            if (oldNode == nullptr || !qFuzzyCompare(newCenterRadius, oldCenterRadius)) {
                memcpy(buf->data() + offset, &newCenterRadius, 4);
                changed = true;
            }
            offset += 4;

            float newFocalRadius = newNode->fillGradient.v1;
            float oldFocalRadius = oldNode != nullptr
                    ? oldNode->fillGradient.v1
                    : 0.0f;
            if (oldNode == nullptr || !qFuzzyCompare(newFocalRadius, oldFocalRadius)) {
                memcpy(buf->data() + offset, &newFocalRadius, 4);
                changed = true;
            }
            offset += 4;

        } else if (m_gradientType == QQuickAbstractPathRenderer::ConicalGradient) {
            Q_ASSERT(buf->size() >= offset + 8 + 4);

            QVector2D newFocalPoint = QVector2D(newNode->fillGradient.a);
            QVector2D oldFocalPoint = oldNode != nullptr
                    ? QVector2D(oldNode->fillGradient.a)
                    : QVector2D{};
            if (oldNode == nullptr || newFocalPoint != oldFocalPoint) {
                memcpy(buf->data() + offset, &newFocalPoint, 8);
                changed = true;
            }
            offset += 8;

            float newAngle = newNode->fillGradient.v0;
            float oldAngle = oldNode != nullptr
                    ? oldNode->fillGradient.v0
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

    int QQuickShapeLoopBlinnMaterial::compare(const QSGMaterial *other) const
    {
        if (other->type() != type())
            return (type() - other->type());

        const QQuickShapeLoopBlinnMaterial *otherMaterial =
                static_cast<const QQuickShapeLoopBlinnMaterial *>(other);

        QQuickShapeLoopBlinnNode *a = node();
        QQuickShapeLoopBlinnNode *b = otherMaterial->node();
        if (a == b)
            return 0;

        if (int d = a->strokeColor.rgba() - b->strokeColor.rgba())
            return d;

        if (m_gradientType == QQuickAbstractPathRenderer::NoGradient) {
            if (int d = a->color.red() - b->color.red())
                return d;
            if (int d = a->color.green() - b->color.green())
                return d;
            if (int d = a->color.blue() - b->color.blue())
                return d;
            if (int d = a->color.alpha() - b->color.alpha())
                return d;
        } else {
            const QQuickAbstractPathRenderer::GradientDesc &ga = a->fillGradient;
            const QQuickAbstractPathRenderer::GradientDesc &gb = b->fillGradient;

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

    QSGMaterialType *QQuickShapeLoopBlinnMaterial::type() const
    {
        static QSGMaterialType type[8];
        return &type[m_gradientType + (node()->strokeColor.alpha() > 0 ? 4 : 0)];
    }

    QSGMaterialShader *QQuickShapeLoopBlinnMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
    {
        Q_UNUSED(renderMode);
        return new QQuickShapeLoopBlinnMaterialShader(m_gradientType,
                                                      node()->strokeColor.alpha() > 0);
    }

    QQuickShapeLoopBlinnNode::QQuickShapeLoopBlinnNode(QQuickAbstractPathRenderer::FillGradientType gradientType)
    {
        setFlag(OwnsGeometry, true);
        setGeometry(new QSGGeometry(attributes(), 0, 0));

        activateMaterial(gradientType);
    }

    void QQuickShapeLoopBlinnNode::activateMaterial(QQuickAbstractPathRenderer::FillGradientType gradientType)
    {
        m_material.reset(new QQuickShapeLoopBlinnMaterial(this, gradientType));
        setMaterial(m_material.data());
    }
}

QVector2D QuadPath::Element::pointAtFraction(float t) const
{
    if (isLine()) {
        return sp + t * (ep - sp);
    } else {
        const float r = 1 - t;
        return (r * r * sp) + (2 * t * r * cp) + (t * t * ep);
    }
}

float QuadPath::Element::extent() const
{
    // TBD: cache this value if we start using it a lot
    QVector2D min(qMin(sp.x(), ep.x()), qMin(sp.y(), ep.y()));
    QVector2D max(qMax(sp.x(), ep.x()), qMax(sp.y(), ep.y()));
    if (!isLine()) {
        min = QVector2D(qMin(min.x(), cp.x()), qMin(min.y(), cp.y()));
        max = QVector2D(qMax(max.x(), cp.x()), qMax(max.y(), cp.y()));
    }
    return (max - min).length();
}

// Returns the number of intersections between element and a horizontal line at y.
// The t values of max 2 intersection(s) are stored in the fractions array
int QuadPath::Element::intersectionsAtY(float y, float *fractions) const
{
    const float y0 = startPoint().y() - y;
    const float y1 = controlPoint().y() - y;
    const float y2 = endPoint().y() - y;

    int numRoots = 0;
    const float a = y0 - (2 * y1) + y2;
    if (a) {
        const float b = (y1 * y1) - (y0 * y2);
        if (b >= 0) {
            const float sqr = qSqrt(b);
            const float root1 = -(-y0 + y1 + sqr) / a;
            if (qIsFinite(root1) && root1 >= 0 && root1 <= 1)
                fractions[numRoots++] = root1;
            const float root2 = (y0 - y1 + sqr) / a;
            if (qIsFinite(root2) && root2 != root1 && root2 >= 0 && root2 <= 1)
                fractions[numRoots++] = root2;
        }
    } else if (y1 != y2) {
        const float root1 = (y2 - (2 * y1)) / (2 * (y2 - y1));
        if (qIsFinite(root1) && root1 >= 0 && root1 <= 1)
            fractions[numRoots++] = root1;
    }

    return numRoots;
}

static float crossProduct(const QVector2D &sp, const QVector2D &p, const QVector2D &ep)
{
    QVector2D v1 = ep - p;
    QVector2D v2 = p - sp;
    return (v2.x() * v1.y()) - (v2.y() * v1.x());
}

bool QuadPath::isPointOnLeft(const QVector2D &p, const QVector2D &sp, const QVector2D &ep)
{
    // Use cross product to compare directions of base vector and vector from start to p
    return crossProduct(sp, p, ep) >= 0.0f;
}

bool QuadPath::isPointOnLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep)
{
    return qFuzzyIsNull(crossProduct(p, sp, ep));
}

bool QuadPath::isPointNearLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep)
{
    constexpr float epsilon = 1.0f;
    return qAbs(crossProduct(p, sp, ep)) < epsilon;
}

bool QuadPath::isControlPointOnLeft(const QuadPath::Element &element)
{
    return isPointOnLeft(element.cp, element.sp, element.ep);
}

// NOTE: it is assumed that subpaths are closed
bool QuadPath::contains(const QVector2D &pt) const
{
    // if (!controlPointRect().contains(pt) : good opt when we add cpr caching
    //     return false;

    int winding_number = 0;
    for (const Element &e : m_elements) {
        int dir = 1;
        float y1 = e.startPoint().y();
        float y2 = e.endPoint().y();
        if (y2 < y1) {
            qSwap(y1, y2);
            dir = -1;
        }
        if (e.m_isLine) {
            if (pt.y() < y1 || pt.y() >= y2 || y1 == y2)
                continue;
            const float t = (pt.y() - e.startPoint().y()) / (e.endPoint().y() - e.startPoint().y());
            const float x = e.startPoint().x() + t * (e.endPoint().x() - e.startPoint().x());
            if (x <= pt.x())
                winding_number += dir;
        } else {
            y1 = qMin(y1, e.controlPoint().y());
            y2 = qMax(y2, e.controlPoint().y());
            if (pt.y() < y1 || pt.y() >= y2)
                continue;
            float ts[2];
            const int numRoots = e.intersectionsAtY(pt.y(), ts);
            // Count if there is exactly one intersection to the left
            bool oneHit = false;
            float tForHit = -1;
            for (int i = 0; i < numRoots; i++) {
                if (e.pointAtFraction(ts[i]).x() <= pt.x()) {
                    oneHit = !oneHit;
                    tForHit = ts[i];
                }
            }
            if (oneHit) {
                dir = e.tangentAtFraction(tForHit).y() < 0 ? -1 : 1;
                winding_number += dir;
            }
        }
    };

    return (fillRule() == Qt::WindingFill ? (winding_number != 0) : ((winding_number % 2) != 0));
}

void QuadPath::addElement(const QVector2D &control, const QVector2D &endPoint, bool isLine)
{
    if (qFuzzyCompare(currentPoint, endPoint))
        return; // 0 length element, skip

    isLine = isLine || isPointNearLine(control, currentPoint, endPoint); // Turn flat quad into line

    m_elements.resize(m_elements.size() + 1);
    Element &elem = m_elements.last();
    elem.sp = currentPoint;
    elem.cp = isLine ? (0.5f * (currentPoint + endPoint)) : control;
    elem.ep = endPoint;
    elem.m_isLine = isLine;
    elem.m_isSubpathStart = subPathToStart;
    subPathToStart = false;
    currentPoint = endPoint;
}

QuadPath::Element::CurvatureFlags QuadPath::coordinateOrderOfElement(const QuadPath::Element &element) const
{
    QVector2D baseLine = element.endPoint() - element.startPoint();
    QVector2D midPoint = element.midPoint();
    // At the midpoint, the tangent of a quad is parallel to the baseline
    QVector2D normal = QVector2D(-baseLine.y(), baseLine.x()).normalized();
    float delta = qMin(element.extent() / 100, QQUICKSHAPECURVERENDERER_CONVEX_CHECK_ERROR_MARGIN);
    QVector2D justRightOfMid = midPoint + (normal * delta);
    bool pathContainsPoint = contains(justRightOfMid);
    return pathContainsPoint ? Element::FillOnRight : Element::CurvatureFlags(0);
}

QVector2D QuadPath::closestPointOnLine(const QVector2D &start,
                                       const QVector2D &end,
                                       const QVector2D &p)
{
    QVector2D line = end - start;
    float t = QVector2D::dotProduct(p - start, line) / QVector2D::dotProduct(line, line);
    return start + qBound(0.0f, t, 1.0f) * line;
}

QuadPath QuadPath::fromPainterPath(const QPainterPath &path)
{
    QuadPath res;
    res.reserve(path.elementCount());
    res.setFillRule(path.fillRule());

    QPolygonF quads;
    QPointF sp;
    for (int i = 0; i < path.elementCount(); ++i) {
        QPainterPath::Element element = path.elementAt(i);

        QPointF ep(element);
        switch (element.type) {
        case QPainterPath::MoveToElement:
            res.moveTo(QVector2D(ep));
            break;
        case QPainterPath::LineToElement:
            res.lineTo(QVector2D(ep));
            break;
        case QPainterPath::CurveToElement: {
            QPointF cp1 = ep;
            QPointF cp2(path.elementAt(++i));
            ep = path.elementAt(++i);
            QBezier b = QBezier::fromPoints(sp, cp1, cp2, ep);
#ifndef USE_TOQUADRATICS_IN_QBEZIER
            qt_toQuadratics(b, &quads);
#else
            quads = b.toQuadratics();
#endif
            for (int i = 1; i < quads.size(); i += 2) {
                QVector2D cp(quads[i]);
                QVector2D ep(quads[i + 1]);
                res.quadTo(cp, ep);
            }
            break;
        }
        default:
            Q_UNREACHABLE();
            break;
        }
        sp = ep;
    }

    return res;
}

void QuadPath::addCurvatureData()
{
    // We use the convention that the inside of a curve is on the *right* side of the
    // direction of the baseline.Thus, as long as this is true: if the control point is
    // on the left side of the baseline, the curve is convex  and otherwise it is
    // concave. The paths we get can be arbitrary order, but each subpath will have a
    // consistent order. Therefore, for the first curve element in a subpath, we can
    // determine if the direction already follows the convention or not, and then we
    // can easily detect curvature of all subsequent elements in the subpath.

    static bool checkAnomaly = qEnvironmentVariableIntValue("QT_QUICKSHAPES_CHECK_ALL_CURVATURE") != 0;

    Element::CurvatureFlags flags = Element::CurvatureUndetermined;
    for (QuadPath::Element &element : m_elements) {
        Q_ASSERT(element.childCount() == 0);
        if (element.isSubpathStart()) {
            flags = coordinateOrderOfElement(element);
        } else if (checkAnomaly) {
            Element::CurvatureFlags newFlags = coordinateOrderOfElement(element);
            if (flags != newFlags) {
                qDebug() << "Curvature anomaly detected:" << element
                         << "Subpath fill on right:" << (flags & Element::FillOnRight)
                         << "Element fill on right:" << (newFlags & Element::FillOnRight);
            }
        }

        if (element.isLine()) {
            element.m_curvatureFlags = flags;

            // Set the control point to an arbitrary point on the inside side of the line
            // (doesn't need to actually be inside the shape: it just makes our calculations
            // easier later if it is at the same side as the fill).
            const QVector2D &sp = element.sp;
            const QVector2D &ep = element.ep;
            QVector2D v = ep - sp;
            element.cp = flags & Element::FillOnRight ? sp + QVector2D(-v.y(), v.x()) : sp + QVector2D(v.y(), -v.x());
        } else {
            bool controlPointOnLeft = isControlPointOnLeft(element);
            bool isFillOnRight = flags & Element::FillOnRight;
            bool isConvex = controlPointOnLeft == isFillOnRight;

            if (isConvex)
                element.m_curvatureFlags = Element::CurvatureFlags(flags | Element::Convex);
            else
                element.m_curvatureFlags = flags;
        }
    }
}

QRectF QuadPath::controlPointRect() const
{
    QRectF res;
    if (elementCount()) {
        QVector2D min, max;
        min = max = m_elements.constFirst().sp;
        // No need to recurse, as split curve's controlpoints are within the parent curve's
        for (const QuadPath::Element &e : std::as_const(m_elements)) {
            min.setX(std::min({ min.x(), e.sp.x(), e.cp.x(), e.ep.x() }));
            min.setY(std::min({ min.y(), e.sp.y(), e.cp.y(), e.ep.y() }));
            max.setX(std::max({ max.x(), e.sp.x(), e.cp.x(), e.ep.x() }));
            max.setY(std::max({ max.y(), e.sp.y(), e.cp.y(), e.ep.y() }));
        }
        res = QRectF(min.toPointF(), max.toPointF());
    }
    return res;
}

// Count leaf elements
qsizetype QuadPath::elementCountRecursive() const
{
    qsizetype count = 0;
    iterateElements([&](const QuadPath::Element &) { count++; });
    return count;
}

QPainterPath QuadPath::toPainterPath() const
{
    // Currently only converts the main, unsplit path; no need for the split path identified
    QPainterPath res;
    res.reserve(elementCount());
    res.setFillRule(fillRule());
    for (const Element &element : m_elements) {
        if (element.m_isSubpathStart)
            res.moveTo(element.startPoint().toPointF());
        if (element.m_isLine)
            res.lineTo(element.endPoint().toPointF());
        else
            res.quadTo(element.controlPoint().toPointF(), element.endPoint().toPointF());
    };
    return res;
}

// Returns a new path since doing it inline would probably be less efficient
// (technically changing it from O(n) to O(n^2))
// Note that this function should be called before splitting any elements,
// so we can assume that the structure is a list and not a tree
QuadPath QuadPath::subPathsClosed() const
{
    QuadPath res = *this;
    res.m_elements = {};
    res.m_elements.reserve(elementCount());
    qsizetype subStart = -1;
    qsizetype prevElement = -1;
    for (qsizetype i = 0; i < elementCount(); i++) {
        const auto &element = m_elements.at(i);
        if (element.m_isSubpathStart) {
            if (subStart >= 0 && m_elements[i - 1].ep != m_elements[subStart].sp) {
                res.currentPoint = m_elements[i - 1].ep;
                res.lineTo(m_elements[subStart].sp);
                auto &endElement = res.m_elements.last();
                endElement.m_isSubpathEnd = true;
                // lineTo() can bail out if the points are too close.
                // In that case, just change the end point to be equal to the start
                // (No need to test because the assignment is a no-op otherwise).
                endElement.ep = m_elements[subStart].sp;
            } else if (prevElement >= 0) {
                res.m_elements[prevElement].m_isSubpathEnd = true;
            }
            subStart = i;
        }
        res.m_elements.append(element);
        prevElement = res.m_elements.size() - 1;
    }

    if (subStart >= 0 && m_elements.last().ep != m_elements[subStart].sp) {
        res.currentPoint = m_elements.last().ep;
        res.lineTo(m_elements[subStart].sp);
    }
    if (!res.m_elements.isEmpty()) {
        auto &endElement = res.m_elements.last();
        endElement.m_isSubpathEnd = true;
        endElement.ep = m_elements[subStart].sp;
    }

    // ### Workaround for triangulator issue: Avoid 3-element paths
    if (res.elementCount() == 3) {
        res.splitElementAt(2);
        res = res.flattened();
        Q_ASSERT(res.elementCount() == 4);
    }

    return res;
}

QuadPath QuadPath::flattened() const
{
    QuadPath res;
    res.reserve(elementCountRecursive());
    iterateElements([&](const QuadPath::Element &element) { res.m_elements.append(element); });
    return res;
}

void QuadPath::splitElementAt(qsizetype index)
{
    const qsizetype newChildIndex = m_childElements.size();
    m_childElements.resize(newChildIndex + 2);
    Element &parent = elementAt(index);
    parent.m_numChildren = 2;
    parent.m_firstChildIndex = newChildIndex;

    Element &quad1 = m_childElements[newChildIndex];
    const QVector2D mp = parent.midPoint();
    quad1.sp = parent.sp;
    quad1.cp = 0.5f * (parent.sp + parent.cp);
    quad1.ep = mp;
    quad1.m_isSubpathStart = parent.m_isSubpathStart;
    quad1.m_isSubpathEnd = false;
    quad1.m_curvatureFlags = parent.m_curvatureFlags;
    quad1.m_isLine = parent.m_isLine; //### || isPointNearLine(quad1.cp, quad1.sp, quad1.ep);

    Element &quad2 = m_childElements[newChildIndex + 1];
    quad2.sp = mp;
    quad2.cp = 0.5f * (parent.ep + parent.cp);
    quad2.ep = parent.ep;
    quad2.m_isSubpathStart = false;
    quad2.m_isSubpathEnd = parent.m_isSubpathEnd;
    quad2.m_curvatureFlags = parent.m_curvatureFlags;
    quad2.m_isLine = parent.m_isLine; //### || isPointNearLine(quad2.cp, quad2.sp, quad2.ep);

#ifndef QT_NO_DEBUG
    if (qFuzzyCompare(quad1.sp, quad1.ep) || qFuzzyCompare(quad2.sp, quad2.ep))
        qDebug() << "###FIXME: quad splitting has yielded ~null quad.";
#endif
}

static void printElement(QDebug stream, const QuadPath::Element &element)
{
    auto printPoint = [&](QVector2D p) { stream << "(" << p.x() << ", " << p.y() << ") "; };
    stream << "{ ";
    printPoint(element.startPoint());
    printPoint(element.controlPoint());
    printPoint(element.endPoint());
    stream << "} " << (element.isLine() ? "L " : "C ") << (element.isConvex() ? "X " : "O ")
           << (element.isSubpathStart() ? "S" : element.isSubpathEnd() ? "E" : "");
}

QDebug operator<<(QDebug stream, const QuadPath::Element &element)
{
    QDebugStateSaver saver(stream);
    stream.nospace();
    stream << "QuadPath::Element( ";
    printElement(stream, element);
    stream << " )";
    return stream;
}

QDebug operator<<(QDebug stream, const QuadPath &path)
{
    QDebugStateSaver saver(stream);
    stream.nospace();
    stream << "QuadPath(" << path.elementCount() << " main elements, "
           << path.elementCountRecursive() << " leaf elements, "
           << (path.fillRule() == Qt::OddEvenFill ? "OddEven" : "Winding") << Qt::endl;
    qsizetype count = 0;
    path.iterateElements([&](const QuadPath::Element &e) {
        stream << " " << count++ << (e.isSubpathStart() ? " >" : " ");
        printElement(stream, e);
        stream << Qt::endl;
    });
    stream << ")";
    return stream;
}

QQuickShapeCurveNode::QQuickShapeCurveNode() { }

QQuickShapeCurveRenderer::~QQuickShapeCurveRenderer() { }

void QQuickShapeCurveRenderer::beginSync(int totalCount, bool *countChanged)
{
    if (countChanged != nullptr && totalCount != m_paths.size())
        *countChanged = true;
    m_paths.resize(totalCount);
}

void QQuickShapeCurveRenderer::setPath(int index, const QQuickPath *path)
{
    auto &pathData = m_paths[index];
    pathData.originalPath = path->path();
    pathData.m_dirty |= GeometryDirty;

}

void QQuickShapeCurveRenderer::setStrokeColor(int index, const QColor &color)
{
    auto &pathData = m_paths[index];
    const bool wasVisible = pathData.isStrokeVisible();
    pathData.pen.setColor(color);
    if (pathData.isStrokeVisible() != wasVisible)
        pathData.m_dirty |= GeometryDirty;
    else
        pathData.m_dirty |= UniformsDirty;
}

void QQuickShapeCurveRenderer::setStrokeWidth(int index, qreal w)
{
    auto &pathData = m_paths[index];
    if (w > 0) {
        pathData.validPenWidth = true;
        pathData.pen.setWidthF(w);
    } else {
        pathData.validPenWidth = false;
    }
    pathData.m_dirty |= GeometryDirty;
}

void QQuickShapeCurveRenderer::setFillColor(int index, const QColor &color)
{
    auto &pathData = m_paths[index];
    const bool wasVisible = pathData.isFillVisible();
    pathData.fillColor = color;
    if (pathData.isFillVisible() != wasVisible)
        pathData.m_dirty |= GeometryDirty;
    else
        pathData.m_dirty |= UniformsDirty;
}

void QQuickShapeCurveRenderer::setFillRule(int index, QQuickShapePath::FillRule fillRule)
{
    auto &pathData = m_paths[index];
    pathData.fillRule = Qt::FillRule(fillRule);
    pathData.m_dirty |= GeometryDirty;
}

void QQuickShapeCurveRenderer::setJoinStyle(int index,
                                          QQuickShapePath::JoinStyle joinStyle,
                                          int miterLimit)
{
    auto &pathData = m_paths[index];
    pathData.pen.setJoinStyle(Qt::PenJoinStyle(joinStyle));
    pathData.pen.setMiterLimit(miterLimit);
    pathData.m_dirty |= GeometryDirty;
}

void QQuickShapeCurveRenderer::setCapStyle(int index, QQuickShapePath::CapStyle capStyle)
{
    auto &pathData = m_paths[index];
    pathData.pen.setCapStyle(Qt::PenCapStyle(capStyle));
    pathData.m_dirty |= GeometryDirty;
}

void QQuickShapeCurveRenderer::setStrokeStyle(int index,
                                            QQuickShapePath::StrokeStyle strokeStyle,
                                            qreal dashOffset,
                                            const QVector<qreal> &dashPattern)
{
    auto &pathData = m_paths[index];
    pathData.pen.setStyle(Qt::PenStyle(strokeStyle));
    if (strokeStyle == QQuickShapePath::DashLine) {
        pathData.pen.setDashPattern(dashPattern);
        pathData.pen.setDashOffset(dashOffset);
    }
    pathData.m_dirty |= GeometryDirty;
}

void QQuickShapeCurveRenderer::setFillGradient(int index, QQuickShapeGradient *gradient)
{
#if defined(QQUICKSHAPECURVERENDERER_GRADIENTS)
    PathData &pd(m_paths[index]);
    pd.gradientType = NoGradient;
    if (QQuickShapeLinearGradient *g  = qobject_cast<QQuickShapeLinearGradient *>(gradient)) {
        pd.gradientType = LinearGradient;
        pd.gradient.stops = gradient->gradientStops();
        pd.gradient.spread = gradient->spread();
        pd.gradient.a = QPointF(g->x1(), g->y1());
        pd.gradient.b = QPointF(g->x2(), g->y2());
    } else if (QQuickShapeRadialGradient *g = qobject_cast<QQuickShapeRadialGradient *>(gradient)) {
        pd.gradientType = RadialGradient;
        pd.gradient.a = QPointF(g->centerX(), g->centerY());
        pd.gradient.b = QPointF(g->focalX(), g->focalY());
        pd.gradient.v0 = g->centerRadius();
        pd.gradient.v1 = g->focalRadius();
    } else if (QQuickShapeConicalGradient *g = qobject_cast<QQuickShapeConicalGradient *>(gradient)) {
        pd.gradientType = ConicalGradient;
        pd.gradient.a = QPointF(g->centerX(), g->centerY());
        pd.gradient.v0 = g->angle();
    } else
#endif
    if (gradient != nullptr) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            qCWarning(lcShapeCurveRenderer) << "Unsupported gradient fill";
        }
    }

    if (pd.gradientType != NoGradient) {
        pd.gradient.stops = gradient->gradientStops();
        pd.gradient.spread = gradient->spread();
    }

    pd.m_dirty |= GeometryDirty;
}

void QQuickShapeCurveRenderer::setAsyncCallback(void (*callback)(void *), void *data)
{
    qCWarning(lcShapeCurveRenderer) << "Asynchronous creation not supported by CurveRenderer";
    Q_UNUSED(callback);
    Q_UNUSED(data);
}

void QQuickShapeCurveRenderer::endSync(bool async)
{
    Q_UNUSED(async);
}

bool QQuickShapeCurveRenderer::PathData::useFragmentShaderStroker() const
{
    static bool useStrokeShader = qEnvironmentVariableIntValue("QT_QUICKSHAPES_STROKER");
    return useStrokeShader && pen.style() == Qt::SolidLine && isFillVisible();
}

void QQuickShapeCurveRenderer::updateNode()
{
    if (!m_rootNode)
        return;
    static const int codePath = qEnvironmentVariableIntValue("QT_QUICKSHAPES_ALTERNATIVE_CODE_PATH");
    static const int overlapSolving = !qEnvironmentVariableIntValue("QT_QUICKSHAPES_DISABLE_OVERLAP_SOLVER");
    static bool newStrokeShader = qEnvironmentVariableIntValue("QT_QUICKSHAPES_EXPERIMENTAL_STROKER");

    auto addNodes = [&](const PathData &pathData, NodeList *debugNodes) {
        if (codePath == 1)
            return addPathNodesBasic(pathData, debugNodes);
        else
            return addPathNodesLineShader(pathData, debugNodes);
    };

    for (PathData &pathData : m_paths) {
        if (pathData.m_dirty & GeometryDirty) {
            deleteAndClear(&pathData.strokeNodes);
            deleteAndClear(&pathData.fillNodes);
            deleteAndClear(&pathData.debugNodes);

            static bool useTriangulatingStroker = !qEnvironmentVariableIntValue("QT_QUICKSHAPES_PAINTERPATH_STROKER");
            bool createStrokePath = pathData.isStrokeVisible() && !pathData.useFragmentShaderStroker();

            static bool simplifyPath = qEnvironmentVariableIntValue("QT_QUICKSHAPES_SIMPLIFY_PATHS") != 0;
            pathData.path = simplifyPath ? QuadPath::fromPainterPath(pathData.originalPath.simplified()) : QuadPath::fromPainterPath(pathData.originalPath);
            pathData.path.setFillRule(pathData.fillRule);

            bool solidStroke = pathData.pen.style() == Qt::SolidLine && pathData.pen.isSolid();
            if (newStrokeShader && solidStroke) {
                pathData.qPath = pathData.path;
                pathData.qPath.addCurvatureData(); // ### Can we unify with fill below?
            } else if (createStrokePath) {
                pathData.fillPath = pathData.path.toPainterPath(); // Without subpath closing etc.
            }

            if (pathData.isFillVisible()) {
                pathData.path = pathData.path.subPathsClosed();
                pathData.path.addCurvatureData();
                if (overlapSolving)
                    solveOverlaps(pathData.path);
                pathData.fillNodes = addNodes(pathData, &pathData.debugNodes);
            }

            if (createStrokePath) {
                if (newStrokeShader && solidStroke) {
                    pathData.strokeNodes = addNodesStrokeShader(pathData, &pathData.debugNodes);
                } else if (useTriangulatingStroker && solidStroke) {
                    pathData.strokeNodes = addStrokeNodes(pathData, &pathData.debugNodes);
                } else {
                    QPainterPathStroker stroker(pathData.pen);
                    QPainterPath strokePath = stroker.createStroke(pathData.fillPath);

                    // Solid strokes are sometimes created with self-overlaps in the joins,
                    // causing the overlap detection to freak out. So while this is expensive
                    // we have to make sure the overlaps are removed first.
                    static bool simplifyStroke = qEnvironmentVariableIntValue("QT_QUICKSHAPES_DISABLE_SIMPLIFY_STROKE") == 0;
                    if (pathData.pen.style() == Qt::SolidLine && simplifyStroke)
                        strokePath = strokePath.simplified();
                    QuadPath strokeQuadPath = QuadPath::fromPainterPath(strokePath);
                    strokeQuadPath.addCurvatureData();
                    strokePath = strokeQuadPath.toPainterPath();
                    if (overlapSolving)
                        solveOverlaps(strokeQuadPath);

                    PathData strokeData = pathData;
                    strokeData.path = strokeQuadPath;
                    strokeData.fillPath = strokePath;
                    strokeData.gradientType = NoGradient;
                    strokeData.fillColor = pathData.pen.color();
                    pathData.strokeNodes = addNodes(strokeData, &pathData.debugNodes);
                }
            }
        } else if (pathData.m_dirty & UniformsDirty) {
            for (auto &pathNode : std::as_const(pathData.fillNodes))
                static_cast<QQuickShapeLoopBlinnNode *>(pathNode)->color = pathData.fillColor;

            if (pathData.pen.style() != Qt::NoPen && pathData.pen.color().alpha() > 0) {
                for (auto &strokeNode : std::as_const(pathData.strokeNodes))
                    if (newStrokeShader) // #### Remove this when we have a dedicated stroke shader
                        static_cast<QQuickShapeLoopBlinnNode *>(strokeNode)->strokeColor = pathData.pen.color();
                    else
                        static_cast<QQuickShapeLoopBlinnNode *>(strokeNode)->color = pathData.pen.color();
            }
        }
        pathData.m_dirty &= ~(GeometryDirty | UniformsDirty);
    }
}

QVector<QSGGeometryNode *> QQuickShapeCurveRenderer::addPathNodesBasic(const PathData &pathData, NodeList *debugNodes)
{
    QVector<QSGGeometryNode *> ret;

    QList<QPolygonF> quadraticCurvesConvex;
    QList<QPolygonF> quadraticCurvesConcave;
    QList<QLineF> lineSegments;
    QPainterPath internalHull;
    internalHull.setFillRule(pathData.path.fillRule());

    pathData.path.iterateElements([&](const QuadPath::Element &element){
        QPointF sp(element.startPoint().toPointF());
        QPointF cp(element.controlPoint().toPointF());
        QPointF ep(element.endPoint().toPointF());
        if (element.isSubpathStart())
            internalHull.moveTo(sp);
        if (element.isLine()) {
            internalHull.lineTo(ep);
            lineSegments.append(QLineF(sp, ep));
        } else if (element.isConvex()) {
            internalHull.lineTo(ep);
            quadraticCurvesConvex.append(QPolygonF() << sp << cp << ep);
        } else {
            internalHull.lineTo(cp);
            internalHull.lineTo(ep);
            quadraticCurvesConcave.append(QPolygonF() << sp << cp << ep);
        }
    });

    QTriangleSet triangles = qTriangulate(internalHull);

    // Add triangles for curves. Note: These have to be adapted to 1/32 grid, since this
    // is the resolution of the triangulation
    QVarLengthArray<quint32> extraIndices;
    for (const QPolygonF &quadraticCurve : quadraticCurvesConvex) {
        QPointF v1 = quadraticCurve.at(0);
        QPointF v2 = quadraticCurve.at(1);
        QPointF v3 = quadraticCurve.at(2);

        extraIndices.append(triangles.vertices.size() / 2);
        triangles.vertices.append(qRound(v1.x() * 32.0) / 32.0);
        triangles.vertices.append(qRound(v1.y() * 32.0) / 32.0);

        extraIndices.append(triangles.vertices.size() / 2);
        triangles.vertices.append(qRound(v2.x() * 32.0) / 32.0);
        triangles.vertices.append(qRound(v2.y() * 32.0) / 32.0);

        extraIndices.append(triangles.vertices.size() / 2);
        triangles.vertices.append(qRound(v3.x() * 32.0) / 32.0);
        triangles.vertices.append(qRound(v3.y() * 32.0) / 32.0);
    }

    int startConcaveCurves = triangles.vertices.size() / 2;
    for (const QPolygonF &quadraticCurve : quadraticCurvesConcave) {
        QPointF v1 = quadraticCurve.at(0);
        QPointF v2 = quadraticCurve.at(1);
        QPointF v3 = quadraticCurve.at(2);

        extraIndices.append(triangles.vertices.size() / 2);
        triangles.vertices.append(qRound(v1.x() * 32.0) / 32.0);
        triangles.vertices.append(qRound(v1.y() * 32.0) / 32.0);

        extraIndices.append(triangles.vertices.size() / 2);
        triangles.vertices.append(qRound(v2.x() * 32.0) / 32.0);
        triangles.vertices.append(qRound(v2.y() * 32.0) / 32.0);

        extraIndices.append(triangles.vertices.size() / 2);
        triangles.vertices.append(qRound(v3.x() * 32.0) / 32.0);
        triangles.vertices.append(qRound(v3.y() * 32.0) / 32.0);
    }

    ret.append(addLoopBlinnNodes(triangles,
                                 extraIndices,
                                 startConcaveCurves,
                                 pathData,
                                 debugNodes));
    return ret;
}

QSGGeometryNode *QQuickShapeCurveRenderer::addLoopBlinnNodes(const QTriangleSet &triangles,
                                                           const QVarLengthArray<quint32> &extraIndices,
                                                           int startConcaveCurves,
                                                           const PathData &pathData, NodeList *debugNodes)
{
    const QColor &color = pathData.fillColor;

    // Basically we here need a way to determine if each triangle is:
    // 1. A convex curve
    // 2. A concave curve
    // 3. A filled triangle
    //
    // For filled triangles, we make all texture coordinates (1.0, 0.0)
    // For curves, we use (0, 0), (0.5, 0), (1, 1)
    // We use a third texture coordinate for curves: 0 means convex curve and 1 means concave

    QQuickShapeLoopBlinnNode *node = new QQuickShapeLoopBlinnNode(pathData.gradientType);
    node->fillGradient = pathData.gradient;
    node->color = color;

    if (pathData.isStrokeVisible() && pathData.useFragmentShaderStroker()) {
        node->strokeColor = pathData.pen.color();
        node->strokeWidth = pathData.pen.widthF();
    }

    QSGGeometry *g = node->geometry();

    QSGGeometry::Type indexType = triangles.indices.type() == QVertexIndexVector::UnsignedInt
            ? QSGGeometry::UnsignedIntType
            : QSGGeometry::UnsignedShortType;
    Q_ASSERT(indexType == QSGGeometry::UnsignedIntType); // Needs some code to support shorts
                                                         // since extraIndices is int32

    QVector<QQuickShapeLoopBlinnNode::LoopBlinnVertex> vertices;
    for (int i = 0; i < triangles.vertices.size(); i += 2) {
        QVector2D v(triangles.vertices.at(i), triangles.vertices.at(i + 1));
        QVector3D uv(0.0f, 1.0f, -1.0f);
        bool visualizeDebug = debugVisualization() & DebugCurves;
        vertices.append( { v.x(), v.y(), uv.x(), uv.y(), uv.z(), 0.0f, 1.0f, 0.0f, visualizeDebug ? 0.5f : 0.0f });
    }

    for (int i = 0; i < extraIndices.size(); ++i) {
        int idx = extraIndices.at(i);

        QVector2D uv;
        switch (i % 3) {
        case 0:
            uv = QVector2D(0.0f, 0.0f);
            break;
        case 1:
            uv = QVector2D(0.5f, 0.0f);
            break;
        case 2:
            uv = QVector2D(1.0f, 1.0f);
            break;
        }

        vertices[idx].u = uv.x();
        vertices[idx].v = uv.y();
        vertices[idx].w = idx >= startConcaveCurves ? 1.0f : -1.0f;
        vertices[idx].r = idx >= startConcaveCurves ? 0.0f : 1.0f;
        vertices[idx].g = 0.0f;
        vertices[idx].b = idx >= startConcaveCurves ? 1.0f : 0.0f;
    }

    if (g->indexType() != indexType) {
        g = new QSGGeometry(QQuickShapeLoopBlinnNode::attributes(),
                            vertices.size(),
                            triangles.indices.size() + extraIndices.size(),
                            indexType);
        node->setGeometry(g);
    } else {
        g->allocate(vertices.size(), triangles.indices.size() + extraIndices.size());
    }

    g->setDrawingMode(QSGGeometry::DrawTriangles);
    memcpy(g->vertexData(),
           vertices.data(),
           g->vertexCount() * g->sizeOfVertex());
    memcpy(g->indexData(),
           triangles.indices.data(),
           triangles.indices.size() * g->sizeOfIndex());
    memcpy((uchar*)g->indexData() + triangles.indices.size() * g->sizeOfIndex(),
           extraIndices.data(),
           extraIndices.size() * g->sizeOfIndex());

    m_rootNode->appendChildNode(node);

    if (debugVisualization() & DebugWireframe) {
        QQuickShapeWireFrameNode *wfNode = new QQuickShapeWireFrameNode;
        QSGGeometry *wfg = wfNode->geometry();

        QVarLengthArray<quint32> indices;
        QVarLengthArray<QQuickShapeWireFrameNode::WireFrameVertex> wfVertices;
        for (int i = 0; i < triangles.indices.size() + extraIndices.size(); ++i) {
            quint32 index = i < triangles.indices.size()
                    ? ((quint32*)triangles.indices.data())[i]
                    : extraIndices[i - triangles.indices.size()];

            const QQuickShapeLoopBlinnNode::LoopBlinnVertex &vertex = vertices.at(index);

            float u = i % 3 == 0 ? 1.0f : 0.0f;
            float v = i % 3 == 1 ? 1.0f : 0.0f;
            float w = i % 3 == 2 ? 1.0f : 0.0f;

            indices.append(i);
            wfVertices.append({ vertex.x, vertex.y, u, v, w });
        }

        if (wfg->indexType() != indexType) {
            wfg = new QSGGeometry(QQuickShapeWireFrameNode::attributes(),
                                  wfVertices.size(),
                                  indices.size(),
                                  indexType);
            wfNode->setGeometry(wfg);
        } else {
            wfg->allocate(wfVertices.size(), indices.size());
        }

        wfg->setDrawingMode(QSGGeometry::DrawTriangles);
        memcpy(wfg->indexData(),
               indices.data(),
               indices.size() * g->sizeOfIndex());
        memcpy(wfg->vertexData(),
               wfVertices.data(),
               wfg->vertexCount() * wfg->sizeOfVertex());

        m_rootNode->appendChildNode(wfNode);
        debugNodes->append(wfNode);
    }

    return node;
}

// Input coordinate space is pre-mapped so that (0, 0) maps to [0, 0] in uv space.
// v1 maps to [1,0], v2 maps to [0,1]. p is the point to be mapped to uv in this space (i.e. vector from p0)
static inline QVector2D uvForPoint(QVector2D v1, QVector2D v2, QVector2D p)
{
    double divisor = v1.x() * v2.y() - v2.x() * v1.y();

    float u = (p.x() * v2.y() - p.y() * v2.x()) / divisor;
    float v = (p.y() * v1.x() - p.x() * v1.y()) / divisor;

    return {u, v};
}

// Find uv coordinates for the point p, for a quadratic curve from p0 to p2 with control point p1
// also works for a line from p0 to p2, where p1 is on the inside of the path relative to the line
static inline QVector2D curveUv(QVector2D p0, QVector2D p1, QVector2D p2, QVector2D p)
{
    QVector2D v1 = 2 * (p1 - p0);
    QVector2D v2 = p2 - v1 - p0;
    return uvForPoint(v1, v2, p - p0);
}

QVector3D QuadPath::Element::uvForPoint(QVector2D p) const
{
    auto uv = curveUv(sp, cp, ep, p);
    if (m_isLine)
        return { uv.x(), uv.y(), 0.0f };
    else
        return { uv.x(), uv.y(), (m_curvatureFlags & Convex) ? -1.0f : 1.0f };
}

static inline QVector2D calcNormalVector(QVector2D a, QVector2D b)
{
    auto v = b - a;
    return {v.y(), -v.x()};
}

// The sign of the return value indicates which side of the line defined by a and n the point p falls
static inline float testSideOfLineByNormal(QVector2D a, QVector2D n, QVector2D p)
{
    float dot = QVector2D::dotProduct(p - a, n);
    return dot;
};

template<typename Func>
void iteratePath(const QuadPath &path, int index, Func &&lambda)
{
    const auto &element = path.elementAt(index);
    if (element.childCount() == 0) {
        lambda(element, index);
    } else {
        for (int i = 0; i < element.childCount(); ++i)
            iteratePath(path, element.indexOfChild(i), lambda);
    }
}
QVector<QSGGeometryNode *> QQuickShapeCurveRenderer::addPathNodesLineShader(const PathData &pathData, NodeList *debugNodes)
{
    //qDebug() << "========= STARTING ===========" << pathData.path;


    QVector<QSGGeometryNode *> ret;
    const QColor &color = pathData.fillColor;
    QPainterPath internalHull;
    internalHull.setFillRule(pathData.path.fillRule());


    QVector<QQuickShapeLoopBlinnNode::LoopBlinnVertex> vertexBuffer;
    QVector<quint32> indices;
    bool visualizeDebug = debugVisualization() & DebugCurves;
    const float dbg = visualizeDebug  ? 0.5f : 0.0f;
    QVector<QQuickShapeWireFrameNode::WireFrameVertex> wfVertices;

    QHash<QPair<float, float>, int> linePointHash;
    QHash<QPair<float, float>, int> concaveControlPointHash;
    QHash<QPair<float, float>, int> convexPointHash;


    auto toRoundedPair = [](const QPointF &p) -> QPair<float, float> {
        return qMakePair(qRound(p.x() * 32.0f) / 32.0f, qRound(p.y() * 32.0f) / 32.0f);
    };

    auto toRoundedVec2D = [](const QPointF &p) -> QVector2D {
        return { qRound(p.x() * 32.0f) / 32.0f, qRound(p.y() * 32.0f) / 32.0f };
    };

    auto roundVec2D = [](const QVector2D &p) -> QVector2D {
        return { qRound(p.x() * 32.0f) / 32.0f, qRound(p.y() * 32.0f) / 32.0f };
    };

    auto addVertexDataForPoint = [&vertexBuffer, dbg](const QuadPath::Element &element, const QVector2D &p) {
        auto uv = element.uvForPoint(p);

        float r = 0.0f, g = 0.0f, b = 0.0f;
        if (element.isLine()) {
            g = b = 1.0f;
        } else if (element.isConvex()) {
            r = 1.0;
        } else { // concave
            b = 1.0;
        }

        vertexBuffer.append({p.x(), p.y(), uv.x(), uv.y(), uv.z(), r, g, b, dbg});
    };

    auto addCurveTriangle = [&](const QuadPath::Element &element, const QVector2D &sp, const QVector2D &ep, const QVector2D &cp){
        int currentVertex = vertexBuffer.count();
        addVertexDataForPoint(element, sp);
        addVertexDataForPoint(element, cp);
        addVertexDataForPoint(element, ep);
        indices << currentVertex << currentVertex + 1 << currentVertex + 2;
        wfVertices.append({sp.x(), sp.y(), 1.0f, 0.0f, 0.0f}); // 0
        wfVertices.append({cp.x(), cp.y(), 0.0f, 1.0f, 0.0f}); // 1
        wfVertices.append({ep.x(), ep.y(), 0.0f, 0.0f, 1.0f}); // 2
    };

    // Find a point on the other side of the line
    auto findPointOtherSide = [](const QVector2D &startPoint, const QVector2D &endPoint, const QVector2D &referencePoint){

        QVector2D baseLine = endPoint - startPoint;
        QVector2D insideVector = referencePoint - startPoint;
        //QVector2D midPoint = (endPoint + startPoint) / 2; // ??? Should we use midPoint instead of startPoint??
        QVector2D normal = QVector2D(-baseLine.y(), baseLine.x()); // TODO: limit size of triangle

        bool swap = QVector2D::dotProduct(insideVector, normal) < 0;

        return swap ? startPoint + normal : startPoint - normal;
    };

    auto addLineTriangle = [&](const QuadPath::Element &element, const QVector2D &sp, const QVector2D &ep, const QVector2D &cp){
        int currentVertex = vertexBuffer.count();
        addCurveTriangle(element, sp, ep, cp);
        // Add a triangle on the outer side of the line to get some more AA
        // The new point replaces cp (currentVertex+1)
        QVector2D op = findPointOtherSide(sp, ep, cp); //sp - (cp - ep);
        wfVertices.append({op.x(), op.y(), 0.0f, 1.0f, 0.0f}); // replacing cp (1)
        auto uv = element.uvForPoint(op);
        vertexBuffer.append({op.x(), op.y(), uv.x(), uv.y(), uv.z(), 0.0f, 1.0f, 1.0f, dbg});
        indices << currentVertex << currentVertex + 3 << currentVertex + 2;
    };

    auto addConvexTriangle = [&](const QuadPath::Element &element, const QVector2D &sp, const QVector2D &ep, const QVector2D &cp){
        int currentVertex = vertexBuffer.count();
        addCurveTriangle(element, sp, ep, cp);
        // Add two triangles on the outer side to get some more AA

        // First triangle on the line sp-cp, replacing ep (currentVertex+2)
        {
        QVector2D op = findPointOtherSide(sp, cp, ep); //sp - (ep - cp);
        wfVertices.append({op.x(), op.y(), 0.0f, 0.0f, 1.0f}); // replacing ep (2)
        auto uv = element.uvForPoint(op);
        vertexBuffer.append({op.x(), op.y(), uv.x(), uv.y(), uv.z(), 0.0f, 1.0f, 1.0f, dbg});
        indices << currentVertex << currentVertex + 1 << currentVertex + 3;
        }

        // Second triangle on the line ep-cp, replacing sp (currentVertex+0)
        {
        QVector2D op = findPointOtherSide(ep, cp, sp); //ep - (sp - cp);
        wfVertices.append({op.x(), op.y(), 1.0f, 0.0f, 0.0f}); // replacing sp (0)
        auto uv = element.uvForPoint(op);
        vertexBuffer.append({op.x(), op.y(), uv.x(), uv.y(), uv.z(), 0.0f, 1.0f, 1.0f, dbg});
        indices << currentVertex + 4 << currentVertex + 1 << currentVertex + 2;
        }

    };


    // This is guaranteed to be in safe space (the curve will never enter the triangle)
    // ### This is the opposite of what we really want: it's going to be extremely thin when we need it,
    // and big when it's completely pointless, but a thicker triangle could be going into negative space
    auto oppositePoint = [](const QVector2D &startPoint, const QVector2D &endPoint, const QVector2D &controlPoint) -> QVector2D {
        return startPoint + 2 * (endPoint - controlPoint);
    };

    // Identical to addLineTriangle, except for how op is calculated
    auto addConcaveTriangle = [&](const QuadPath::Element &element, const QVector2D &sp, const QVector2D &ep, const QVector2D &cp){
        int currentVertex = vertexBuffer.count();
        addCurveTriangle(element, sp, ep, cp);
        // Add an outer triangle to give extra AA for very flat curves
        QVector2D op = oppositePoint(sp, ep, cp);
        // The new point replaces cp (currentVertex+1)
        wfVertices.append({op.x(), op.y(), 0.0f, 1.0f, 0.0f}); // replacing cp (1)
        auto uv = element.uvForPoint(op);
        vertexBuffer.append({op.x(), op.y(), uv.x(), uv.y(), uv.z(), 0.0f, 1.0f, 1.0f, dbg});
        indices << currentVertex << currentVertex + 3 << currentVertex + 2;
    };

    auto addFillTriangle = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &p3){
        int currentVertex = vertexBuffer.count();
         constexpr QVector3D uv(0.0, 1.0, -1.0);
        vertexBuffer.append({p1.x(), p1.y(), uv.x(), uv.y(), uv.z(), 0.0f, 1.0f, 0.0f, dbg});
        vertexBuffer.append({p2.x(), p2.y(), uv.x(), uv.y(), uv.z(), 0.0f, 1.0f, 0.0f, dbg});
        vertexBuffer.append({p3.x(), p3.y(), uv.x(), uv.y(), uv.z(), 0.0f, 1.0f, 0.0f, dbg});
        indices << currentVertex << currentVertex + 1 << currentVertex + 2;
        wfVertices.append({p1.x(), p1.y(), 1.0f, 0.0f, 0.0f}); // 0
        wfVertices.append({p3.x(), p3.y(), 0.0f, 1.0f, 0.0f}); // 1
        wfVertices.append({p2.x(), p2.y(), 0.0f, 0.0f, 1.0f}); // 2
    };



    for (int i = 0; i < pathData.path.elementCount(); ++i)
        iteratePath(pathData.path, i, [&](const QuadPath::Element &element, int index){
            QPointF sp(element.startPoint().toPointF());  //### to much conversion to and from pointF
            QPointF cp(element.controlPoint().toPointF());
            QPointF ep(element.endPoint().toPointF());
            if (element.isSubpathStart())
                internalHull.moveTo(sp);
            if (element.isLine()) {
                internalHull.lineTo(ep);
                //lineSegments.append(QLineF(sp, ep));
                linePointHash.insert(toRoundedPair(sp), index);
            } else {
                if (element.isConvex()) {
                    internalHull.lineTo(ep);
                    addConvexTriangle(element, toRoundedVec2D(sp), toRoundedVec2D(ep), toRoundedVec2D(cp));
                    convexPointHash.insert(toRoundedPair(sp), index);
                } else {
                    internalHull.lineTo(cp);
                    internalHull.lineTo(ep);
                    addConcaveTriangle(element, toRoundedVec2D(sp), toRoundedVec2D(ep), toRoundedVec2D(cp));
                    concaveControlPointHash.insert(toRoundedPair(cp), index);
                }
            }
        });
    //qDebug() << "Curves: i" << indices.size() << "v" << vertexBuffer.size() << "w" << wfVertices.size();

    auto makeHashable = [](const QVector2D &p) -> QPair<float, float> {
        return qMakePair(qRound(p.x() * 32.0f) / 32.0f, qRound(p.y() * 32.0f) / 32.0f);
    };
    // Points in p are already rounded do 1/32
    // Returns false if the triangle needs to be split. Adds the triangle to the graphics buffers and returns true otherwise.

    // TODO: Does not handle ambiguous vertices that are on multiple unrelated lines/curves

    auto onSameSideOfLine = [](const QVector2D &p1, const QVector2D &p2, const QVector2D &linePoint, const QVector2D &lineNormal) {
        float side1 = testSideOfLineByNormal(linePoint, lineNormal, p1);
        float side2 = testSideOfLineByNormal(linePoint, lineNormal, p2);
        return side1 * side2 >= 0;
    };

    auto pointInSafeSpace = [&](const QVector2D &p, const QuadPath::Element &element) -> bool {
        const QVector2D a = element.startPoint();
        const QVector2D b = element.endPoint();
        const QVector2D c = element.controlPoint();
        // There are "safe" areas of the curve also across the baseline: the curve can never cross:
        // line1: the line through A and B'
        // line2: the line through B and A'
        // Where A' = A "mirrored" through C and B' = B "mirrored" through C
        const QVector2D n1 = calcNormalVector(a, c + (c - b));
        const QVector2D n2 = calcNormalVector(b, c + (c - a));
        bool safeSideOf1 = onSameSideOfLine(p, c, a, n1);
        bool safeSideOf2 = onSameSideOfLine(p, c, b, n2);
        return safeSideOf1 && safeSideOf2;
    };

    auto handleTriangle = [&](const QVector2D (&p)[3]) -> bool {
        int lineElementIndex = -1;
        int concaveElementIndex = -1;
        int convexElementIndex = -1;

        bool foundElement = false;
        int si = -1;
        int ei = -1;
        for (int i = 0; i < 3; ++i) {
            if (auto found = linePointHash.constFind(makeHashable(p[i])); found != linePointHash.constEnd()) {
                // check if this triangle is on a line, i.e. if one point is the sp and another is the ep of the same path element
                const auto &element = pathData.path.elementAt(*found);
                //qDebug() << "    " << element;
                for (int j = 0; j < 3; ++j) {
                    if (i != j && roundVec2D(element.endPoint()) == p[j]) {
                        if (foundElement)
                            return false; // More than one edge on path: must split
                        lineElementIndex = *found;
                        si = i;
                        ei = j;
                        //qDebug() << "FOUND IT!!!!" << p[i] << p[j] << lineElementIndex;
                        foundElement = true;
                    }
                }
            } else if (auto found = concaveControlPointHash.constFind(makeHashable(p[i])); found != concaveControlPointHash.constEnd()) {
                // check if this triangle is on the tangent line of a concave curve,
                // i.e if one point is the cp, and the other is sp or ep
                // TODO: clean up duplicated code (almost the same as the lineElement path above)
                const auto &element = pathData.path.elementAt(*found);
                for (int j = 0; j < 3; ++j) {
                    if (i == j)
                        continue;
                    if (roundVec2D(element.startPoint()) == p[j] || roundVec2D(element.endPoint()) == p[j]) {
                        if (foundElement)
                            return false; // More than one edge on path: must split
                        concaveElementIndex = *found;
                        // The tangent line is p[i] - p[j]
                        si = i; // we may not need these
                        ei = j;
                        //qDebug() << "FOUND IT!!!!" << p[i] << p[j] << lineElementIndex;
                        foundElement = true;
                    }
                }
            } else if (auto found = convexPointHash.constFind(makeHashable(p[i])); found != convexPointHash.constEnd()) {
                // check if this triangle is on a curve, i.e. if one point is the sp and another is the ep of the same path element
                const auto &element = pathData.path.elementAt(*found);
                for (int j = 0; j < 3; ++j) {
                    if (i != j && roundVec2D(element.endPoint()) == p[j]) {
                        if (foundElement)
                            return false; // More than one edge on path: must split
                        convexElementIndex = *found;
                        si = i;
                        ei = j;
                        //qDebug() << "FOUND IT!!!!" << p[i] << p[j] << convexElementIndex;
                        foundElement = true;
                    }
                }
            }
        }
        if (lineElementIndex != -1) {
            int ci = (6 - si - ei) % 3; // 1+2+3 is 6, so missing number is 6-n1-n2
            addLineTriangle(pathData.path.elementAt(lineElementIndex), p[si], p[ei], p[ci]);
        } else if (concaveElementIndex != -1) {
            addCurveTriangle(pathData.path.elementAt(concaveElementIndex), p[0], p[1], p[2]);
        } else if (convexElementIndex != -1) {
            int oi = (6 - si - ei) % 3;
            const auto &otherPoint = p[oi];
            const auto &element = pathData.path.elementAt(convexElementIndex);
            // We have to test whether the triangle can cross the line TODO: use the toplevel element's safe space
            bool safeSpace = pointInSafeSpace(otherPoint, element);
            if (safeSpace) {
                addCurveTriangle(element, p[0], p[1], p[2]);
            } else {
                //qDebug() << "Point" << otherPoint << "element" << element << "safe" << safeSpace;
                // Find a point inside the triangle that's also in the safe space
                QVector2D newPoint = (p[0] +  p[1] + p[2]) / 3;
                // We should calculate the point directly, but just do a lazy implementation for now:
                for (int i = 0; i < 7; ++i) {
                    safeSpace = pointInSafeSpace(newPoint, element);
                    //qDebug() << "UNSAFE space round" << i << "checking" << newPoint << safeSpace;
                    if (safeSpace)
                        break;
                    newPoint = (p[si] +  p[ei] + newPoint) / 3;
                }
                if (safeSpace) {
                    // Split triangle. We know the original triangle is only on one path element, so the other triangles are both fill.
                    // Curve triangle is (sp, ep, np)
                    addCurveTriangle(element, p[si], p[ei], newPoint);
                    // The other two are (sp, op, np) and (ep, op, np)
                    addFillTriangle(p[si], p[oi], newPoint);
                    addFillTriangle(p[ei], p[oi], newPoint);
                } else {
                    // fallback to fill if we can't find a point in safe space
                    addFillTriangle(p[0], p[1], p[2]);
                }
            }

        } else {
            addFillTriangle(p[0], p[1], p[2]);
        }
        return true;
    };

    QTriangleSet triangles = qTriangulate(internalHull);

    const quint32 *idxTable = static_cast<const quint32 *>(triangles.indices.data());
    for (int triangle = 0; triangle < triangles.indices.size() / 3; ++triangle) {
        const quint32 *idx = &idxTable[triangle * 3];

        QVector2D p[3];
        for (int i = 0; i < 3; ++i) {
            p[i] = toRoundedVec2D(QPointF(triangles.vertices.at(idx[i]*2), triangles.vertices.at(idx[i]*2 + 1)));
        }
        bool needsSplit = !handleTriangle(p);
        if (needsSplit) {
            QVector2D c = (p[0] + p[1] + p[2]) / 3;
            //qDebug() << "Split!!! New point" << c;
            for (int i = 0; i < 3; ++i) {
                qSwap(c, p[i]);
                //qDebug() << "Adding split triangle" << p[0] << p[1] << p[2];
                handleTriangle(p);
                qSwap(c, p[i]);
            }
        }
    }

    if (indices.size() > 0) {
        auto *node = new QQuickShapeLoopBlinnNode(pathData.gradientType);
        node->color = color;
        node->fillGradient = pathData.gradient;

        if (pathData.useFragmentShaderStroker() && pathData.isStrokeVisible() && pathData.pen.isSolid()) {
            node->strokeColor = pathData.pen.color();
            node->strokeWidth = pathData.pen.widthF();
        }

        QSGGeometry *g = new QSGGeometry(QQuickShapeLoopBlinnNode::attributes(),
                                         vertexBuffer.size(), indices.size(), QSGGeometry::UnsignedIntType);
        node->setGeometry(g);
        g->setDrawingMode(QSGGeometry::DrawTriangles);

        //qDebug() << "gvc" << g->vertexCount();

        memcpy(g->vertexData(),
               vertexBuffer.data(),
               g->vertexCount() * g->sizeOfVertex());
        memcpy(g->indexData(),
               indices.data(),
               indices.size() * g->sizeOfIndex());

        m_rootNode->appendChildNode(node);
        ret.append(node);
    }
     const bool wireFrame = debugVisualization() & DebugWireframe;
    if (wireFrame) {
        QQuickShapeWireFrameNode *wfNode = new QQuickShapeWireFrameNode;

        //QVarLengthArray<quint32> indices;

        QSGGeometry *wfg = new QSGGeometry(QQuickShapeWireFrameNode::attributes(),
                                           wfVertices.size(),
                                           indices.size(),
                                           QSGGeometry::UnsignedIntType);
        wfNode->setGeometry(wfg);

        wfg->setDrawingMode(QSGGeometry::DrawTriangles);
        memcpy(wfg->indexData(),
               indices.data(),
               indices.size() * wfg->sizeOfIndex());
        memcpy(wfg->vertexData(),
               wfVertices.data(),
               wfg->vertexCount() * wfg->sizeOfVertex());

        m_rootNode->appendChildNode(wfNode);
        debugNodes->append(wfNode);
    }

    return ret;
}

QVector<QSGGeometryNode *> QQuickShapeCurveRenderer::addStrokeNodes(const PathData &pathData, NodeList *debugNodes)
{
    QVector<QSGGeometryNode *> ret;
    const QColor &color = pathData.pen.color();

    QVector<QQuickShapeLoopBlinnNode::LoopBlinnVertex> vertexBuffer;
    QVector<quint32> indices;
    bool visualizeDebug = debugVisualization() & DebugCurves;
    const float dbg = visualizeDebug  ? 0.5f : 0.0f;
    QVector<QQuickShapeWireFrameNode::WireFrameVertex> wfVertices;

    QTriangulatingStroker stroker;
    const QVectorPath &vp = qtVectorPathForPath(pathData.fillPath);
    QPen pen = pathData.pen;
    stroker.process(vp, pen, {}, {});

    auto findPointOtherSide = [](const QVector2D &startPoint, const QVector2D &endPoint, const QVector2D &referencePoint){

        QVector2D baseLine = endPoint - startPoint;
        QVector2D insideVector = referencePoint - startPoint;
        //QVector2D midPoint = (endPoint + startPoint) / 2; // ??? Should we use midPoint instead of startPoint??
        QVector2D normal = QVector2D(-baseLine.y(), baseLine.x()); // TODO: limit size of triangle

        bool swap = QVector2D::dotProduct(insideVector, normal) < 0;

        return swap ? startPoint + normal : startPoint - normal;
    };

    static bool disableExtraTriangles = qEnvironmentVariableIntValue("QT_QUICKSHAPES_WIP_DISABLE_EXTRA_STROKE_TRIANGLES");

    auto addStrokeTriangle = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &p3, bool){
        int currentVertex = vertexBuffer.count();
        //constexpr QVector3D uv(0.0, 1.0, -1.0);


        if (p1 == p2 || p2 == p3) {
            //qDebug() << "Skipping triangle" << p1 << p2 << p3;
            return;
        }


        vertexBuffer.append({p1.x(), p1.y(), 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, dbg}); // edge start
        vertexBuffer.append({p2.x(), p2.y(), 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, dbg}); // inner
        vertexBuffer.append({p3.x(), p3.y(), 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, dbg}); // edge end
        indices << currentVertex << currentVertex + 1 << currentVertex + 2;
        wfVertices.append({p1.x(), p1.y(), 1.0f, 0.0f, 0.0f}); // 0
        wfVertices.append({p2.x(), p2.y(), 0.0f, 0.1f, 0.0f}); // 1
        wfVertices.append({p3.x(), p3.y(), 0.0f, 0.0f, 1.0f}); // 2

        if (!disableExtraTriangles) {
            // Add a triangle on the outer side of the line to get some more AA
            // The new point replaces p2 (currentVertex+1)
            QVector2D op = findPointOtherSide(p1, p3, p2);
            wfVertices.append({op.x(), op.y(), 0.0f, 1.0f, 0.0f}); // replacing p2
            vertexBuffer.append({op.x(), op.y(), 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, dbg});
            indices << currentVertex << currentVertex + 3 << currentVertex + 2;
        }
    };

    const int vertCount = stroker.vertexCount() / 2;
    const float *verts = stroker.vertices();
    for (int i = 0; i < vertCount - 2; ++i) {
        QVector2D p[3];
        for (int j = 0; j < 3; ++j) {
            p[j] = QVector2D(verts[(i+j)*2], verts[(i+j)*2 + 1]);
        }
        bool isOdd = i % 2;
        addStrokeTriangle(p[0], p[1], p[2], isOdd);
    }

    if (indices.size() > 0) {
        auto *node = new QQuickShapeLoopBlinnNode(pathData.gradientType);
        node->color = color;
        node->fillGradient = pathData.gradient;

        if (pathData.useFragmentShaderStroker() && pathData.isStrokeVisible() && pathData.pen.isSolid()) {
            node->strokeColor = pathData.pen.color();
            node->strokeWidth = pathData.pen.widthF();
        }

        QSGGeometry *g = new QSGGeometry(QQuickShapeLoopBlinnNode::attributes(),
                                         vertexBuffer.size(), indices.size(), QSGGeometry::UnsignedIntType);
        node->setGeometry(g);
        g->setDrawingMode(QSGGeometry::DrawTriangles);

        //qDebug() << "gvc" << g->vertexCount();

        memcpy(g->vertexData(),
               vertexBuffer.data(),
               g->vertexCount() * g->sizeOfVertex());
        memcpy(g->indexData(),
               indices.data(),
               indices.size() * g->sizeOfIndex());

        m_rootNode->appendChildNode(node);
        ret.append(node);
    }
    const bool wireFrame = debugVisualization() & DebugWireframe;
    if (wireFrame) {
        QQuickShapeWireFrameNode *wfNode = new QQuickShapeWireFrameNode;

        //QVarLengthArray<quint32> indices;

        QSGGeometry *wfg = new QSGGeometry(QQuickShapeWireFrameNode::attributes(),
                                           wfVertices.size(),
                                           indices.size(),
                                           QSGGeometry::UnsignedIntType);
        wfNode->setGeometry(wfg);

        wfg->setDrawingMode(QSGGeometry::DrawTriangles);
        memcpy(wfg->indexData(),
               indices.data(),
               indices.size() * wfg->sizeOfIndex());
        memcpy(wfg->vertexData(),
               wfVertices.data(),
               wfg->vertexCount() * wfg->sizeOfVertex());

        m_rootNode->appendChildNode(wfNode);
        debugNodes->append(wfNode);
    }

    return ret;
}

void QQuickShapeCurveRenderer::setRootNode(QQuickShapeCurveNode *node)
{
    m_rootNode = node;
}

int QQuickShapeCurveRenderer::debugVisualizationFlags = QQuickShapeCurveRenderer::NoDebug;

int QQuickShapeCurveRenderer::debugVisualization()
{
    static const int envFlags = qEnvironmentVariableIntValue("QT_QUICKSHAPES_DEBUG");
    return debugVisualizationFlags | envFlags;
}

void QQuickShapeCurveRenderer::setDebugVisualization(int options)
{
    if (debugVisualizationFlags == options)
        return;
    debugVisualizationFlags = options;
}

void QQuickShapeCurveRenderer::deleteAndClear(NodeList *nodeList)
{
    for (QSGNode *node : std::as_const(*nodeList))
        delete node;
    nodeList->clear();
}

namespace {

/*
  Clever triangle overlap algorithm. Stack Overflow says:

  You can prove that the two triangles do not collide by finding an edge (out of the total 6
  edges that make up the two triangles) that acts as a separating line where all the vertices
  of one triangle lie on one side and the vertices of the other triangle lie on the other side.
  If you can find such an edge then it means that the triangles do not intersect otherwise the
  triangles are colliding.
*/

// The sign of the determinant tells the winding order
static inline double determinant(const QVector2D &p1, const QVector2D &p2, const QVector2D &p3)
{
    return p1.x() * (p2.y() - p3.y())
           + p2.x() * (p3.y() - p1.y())
           + p3.x() * (p1.y() - p2.y());
}

// Fix the triangle so that the determinant is positive
static void fixWinding(QVector2D &p1, QVector2D &p2, QVector2D &p3)
{
    double det = determinant(p1, p2, p3);
    if (det < 0.0) {
        qSwap(p1, p2);
    }
}

// Return true if the determinant is negative, i.e. if the winding order is opposite of the triangle p1,p2,p3.
// This means that p is strictly on the other side of p1-p2 relative to p3 [where p1,p2,p3 is a triangle with
// a positive determinant].
bool checkEdge(QVector2D &p1, QVector2D &p2, QVector2D &p, float epsilon)
{
    return determinant(p1, p2, p) <= epsilon;
}

bool checkTriangleOverlap(QVector2D *triangle1, QVector2D *triangle2, float epsilon = 1.0/32)
{
    // See if there is an edge of triangle1 such that all vertices in triangle2 are on the opposite side
    fixWinding(triangle1[0], triangle1[1], triangle1[2]);
    for (int i = 0; i < 3; i++) {
        int ni = (i + 1) % 3;
        if (checkEdge(triangle1[i], triangle1[ni], triangle2[0], epsilon) &&
            checkEdge(triangle1[i], triangle1[ni], triangle2[1], epsilon) &&
            checkEdge(triangle1[i], triangle1[ni], triangle2[2], epsilon))
            return false;
    }

    // See if there is an edge of triangle2 such that all vertices in triangle1 are on the opposite side
    fixWinding(triangle2[0], triangle2[1], triangle2[2]);
    for (int i = 0; i < 3; i++) {
        int ni = (i + 1) % 3;

        if (checkEdge(triangle2[i], triangle2[ni], triangle1[0], epsilon) &&
            checkEdge(triangle2[i], triangle2[ni], triangle1[1], epsilon) &&
            checkEdge(triangle2[i], triangle2[ni], triangle1[2], epsilon))
            return false;
    }

    return true;
}

bool checkLineTriangleOverlap(QVector2D *triangle, QVector2D *line, float epsilon = 1.0/32)
{
    // See if all vertices of the triangle are on the same side of the line
    bool s1 = determinant(line[0], line[1], triangle[0]) < 0;
    auto s2 = determinant(line[0], line[1], triangle[1]) < 0;
    auto s3 = determinant(line[0], line[1], triangle[2]) < 0;
    // If all determinants have the same sign, then there is no overlap
    if (s1 == s2 && s2 == s3) {
        return false;
    }
    // See if there is an edge of triangle1 such that both vertices in line are on the opposite side
    fixWinding(triangle[0], triangle[1], triangle[2]);
    for (int i = 0; i < 3; i++) {
        int ni = (i + 1) % 3;
        if (checkEdge(triangle[i], triangle[ni], line[0], epsilon) &&
            checkEdge(triangle[i], triangle[ni], line[1], epsilon))
            return false;
    }

    return true;
}

// We could slightly optimize this if we did fixWinding in advance
bool checkTriangleContains (QVector2D pt, QVector2D v1, QVector2D v2, QVector2D v3, float epsilon = 1.0/32)
{
    float d1, d2, d3;
    d1 = determinant(pt, v1, v2);
    d2 = determinant(pt, v2, v3);
    d3 = determinant(pt, v3, v1);

    bool allNegative = d1 < epsilon && d2 < epsilon && d3 < epsilon;
    bool allPositive = d1 > epsilon && d2 > epsilon && d3 > epsilon;

    return allNegative || allPositive;
}

// e1 is always a concave curve. e2 can be curve or line
static bool isOverlap(const QuadPath &path, qsizetype e1, qsizetype e2)
{
    const QuadPath::Element &element1 = path.elementAt(e1);
    const QuadPath::Element &element2 = path.elementAt(e2);

    QVector2D t1[3] = { element1.startPoint(), element1.controlPoint(), element1.endPoint() };

    if (element2.isLine()) {
        QVector2D line[2] = { element2.startPoint(), element2.endPoint() };
        return checkLineTriangleOverlap(t1, line);
    } else {
        QVector2D t2[3] = { element2.startPoint(), element2.controlPoint(), element2.endPoint() };
        return checkTriangleOverlap(t1, t2);
    }

    return false;
}

static bool isOverlap(const QuadPath &path, qsizetype index, const QVector2D &vertex)
{
    const QuadPath::Element &elem = path.elementAt(index);
    return checkTriangleContains(vertex, elem.startPoint(), elem.controlPoint(), elem.endPoint());
}

struct TriangleData
{
    QVector2D points[3];
    int pathElementIndex;
    QVector3D debugColor;
    bool specialDebug = false;
};

QVector2D normalVector(QVector2D baseLine)
{
    QVector2D normal = QVector2D(-baseLine.y(), baseLine.x()).normalized();
    return normal;
}

QVector2D normalVector(const QuadPath::Element &element, bool endSide = false)
{
    if (element.isLine())
        return normalVector(element.endPoint() - element.startPoint());
    else if (!endSide)
        return normalVector(element.controlPoint() - element.startPoint());
    else
        return normalVector(element.endPoint() - element.controlPoint());
}

QVector2D angleBisector(const QuadPath::Element &element1, const QuadPath::Element &element2, float strokeMargin)
{
    Q_ASSERT(element1.endPoint() == element2.startPoint());


    const auto p1 = element1.isLine() ? element1.startPoint() : element1.controlPoint();
    const auto p2 = element1.endPoint();
    const auto p3 = element2.isLine() ? element2.endPoint() : element2.controlPoint();

    const auto v1 = (p1 - p2).normalized();
    const auto v2 = (p3 - p2).normalized();
    const auto bisector = v1 + v2;

    if (qFuzzyIsNull(bisector.x()) && qFuzzyIsNull(bisector.y())) {
        // v1 and v2 are almost parallel, and pointing in opposite directions
        // angle bisector formula will give an almost null vector: use normal of bisector of normals instead
        QVector2D n1(-v1.y(), v1.x());
        QVector2D n2(-v2.y(), v2.x());
        return (n2 - n1).normalized() * strokeMargin;
    } else {
        // We need to increase the length, so that the result covers the whole miter
        // Using the identity sin(x/2) == sqrt((1 - cos(x)) / 2), and the fact that the
        // dot product of two unit vectors is the cosine of the angle between them
        // The length of the miter is w/sin(x/2) where x is the angle between the two elements

        float cos2x = QVector2D::dotProduct(v1, v2);
        cos2x = qMin(1.0f, cos2x); // Allow for float inaccuracy
        float sine = sqrt((1.0f - cos2x) / 2);
        sine = qMax(sine, 0.1f); // Avoid divide by zero

        return bisector.normalized() * strokeMargin / sine;
        // TODO: Make a proper bevel at that point
        // ### This is *not* the right place to do a miter limit: The inner join needs to be as long as
        // it needs to be, independent of the miter on the other side. (We still need to avoid a divide
        // by zero, of course.)
    }
}

// Returns true if p and q are on the same side of the line AB
bool onSameSideOfLine(const QVector2D &a, const QVector2D &b, const QVector2D &p, const QVector2D &q)
{
    auto baseLine = a - b;
    QVector2D n(-baseLine.y(), baseLine.x());
    float pSide = testSideOfLineByNormal(a, n, p);
    float qSide = testSideOfLineByNormal(a, n, q);
    return pSide * qSide >= 0;
}

// Returns true if the entire curve triangle is on the same side of the line p1-p2
bool controlPointInsideLine(const QuadPath::Element &element, const QVector2D &p1, const QVector2D &p2)
{
    bool s1 = determinant(p1, p2, element.startPoint()) < 0;
    auto s2 = determinant(p1, p2, element.endPoint()) < 0;
    auto s3 = determinant(p1, p2, element.controlPoint()) < 0;
    // If all determinants have the same sign, all the points are on the same side
    return (s1 == s2 && s2 == s3);
}

inline bool lineIntersects(const QVector2D &v1_, const QVector2D &v2_,
                           const QVector2D &u1_, const QVector2D &u2_)
{
#if 1
    QPointF v1 = v1_.toPointF();
    QPointF v2 = v2_.toPointF();
    QPointF u1 = u1_.toPointF();
    QPointF u2 = u2_.toPointF();

    if ( (v1.x() < u1.x() && v1.x() < u2.x() && v2.x() < u1.x() && v2.x() < u2.x())
        || (v1.x() > u1.x() && v1.x() > u2.x() && v2.x() > u1.x() && v2.x() > u2.x())
        || (v1.y() < u1.y() && v1.y() < u2.y() && v2.y() < u1.y() && v2.y() < u2.y())
        || (v1.y() > u1.y() && v1.y() > u2.y() && v2.y() > u1.y() && v2.y() > u2.y())) {
        return false;
    }

    // Copied from QLineF::intersects() with some changes to ordering to speed
    // up for our use case
    const QPointF a = v2 - v1;
    const QPointF b = u1 - u2;
    const QPointF c = v1 - u1;

    const qreal denominator = a.y() * b.x() - a.x() * b.y();
    if (denominator == 0.0 || !qt_is_finite(denominator))
        return false;

    const qreal reciprocal = 1.0 / denominator;
    qreal na = (b.y() * c.x() - b.x() * c.y()) * reciprocal;
    if (na < 0.0 || na > 1.0)
        return false;

    qreal nb = (a.x() * c.y() - a.y() * c.x()) * reciprocal;
    if (nb < 0.0 || nb > 1.0)
        return false;

    const QPointF intersectionPoint = v1 + a * na;
    return intersectionPoint != v1 && intersectionPoint != v2 && intersectionPoint != u1 && intersectionPoint != u2;
    return true;
#else
    QLineF v(v1.toPointF(), v2.toPointF());
    QLineF u(u1.toPointF(), u2.toPointF());

    QPointF intersectionPoint;
    if (v.intersects(u, &intersectionPoint) == QLineF::BoundedIntersection) {
        return (intersectionPoint != v.p1() && intersectionPoint != v.p2()
                && intersectionPoint != u.p1() && intersectionPoint != u.p2());
    }

    return false;
#endif
}


static QList<TriangleData> customTriangulator(const QuadPath &path, float margin, bool miterJoin)
{
    QList<TriangleData> ret;

    // TODO: We repeat the same calculations for each side of each join: try to remember the values from the previous
    // element. (The difficult part is path closures)

    auto handleElement = [&ret, margin, miterJoin](const QuadPath::Element *prev, const QuadPath::Element *current, const QuadPath::Element *next, const int elementIndex) {
        QVector2D p1, p2, p3, p4;


        auto startNormal = normalVector(*current) * margin;
        auto endNormal = normalVector(*current, true) * margin;
        auto startBisector = prev ? angleBisector(*prev, *current, margin) : startNormal;
        auto endBisector = next ? angleBisector(*current, *next, margin) : endNormal;

        const auto &sp = current->startPoint();
        const auto &ep = current->endPoint();

        // p1: bisect on the "inside" of the join, p2: normal on the "outside"
        // guess which side is which: (TODO: we should be able to know this)
        bool bevelPointReversed = false;

        if (prev) {
            const auto &prevPoint = prev->isLine() ? prev->startPoint() : prev->controlPoint();
            const auto &lineStart = current->startPoint();
            const auto &lineEnd = current->isLine() ? current->endPoint() : current->controlPoint();
            if (miterJoin) {
                p1 = sp + startBisector;
                p2 = sp - startBisector;
            } else {
                p1 = sp - startBisector;
                p2 = sp - startNormal;
                // p1 should be on the same side as the start point of the previous element
                if (!onSameSideOfLine(lineStart, lineEnd, p1, prevPoint)) {
                    p1 = sp + startBisector;
                }
                // p2 should be on the opposite side ### TODO: optimize test, reusing intermediates from test above
                if (onSameSideOfLine(lineStart, lineEnd, p2, prevPoint)) {
                    p2 = sp + startNormal;
                    bevelPointReversed = true;
                }
            }
        } else {
            p1 = sp + startNormal;
            p2 = sp - startNormal;
        }
        auto bevelPoint = p2; // This is one corner of the "gap" triangle

        // p3: bisect on the "inside" of the join, p4: normal on the "outside"
        // For curves, we use the tangent line of the next element
        if (next) {
            const auto &lineStart = current->isLine() ? current->startPoint() : current->controlPoint();
            const auto &lineEnd = current->endPoint();
            const auto &nextPoint = next->isLine() ? next->endPoint() : next->controlPoint();
            if (miterJoin) {
                p3 = ep + endBisector;
                p4 = ep - endBisector;
            } else {
                p3 = ep - endBisector;
                p4 = ep - endNormal;
                // p3 should be on the same side of our line as the end point of the next element
                if (!onSameSideOfLine(lineStart, lineEnd, p3, nextPoint)) {
                    p3 = ep + endBisector;
                }
                // p4 should be on the opposite side ### TODO: optimize test, reusing intermediates from test above
                if (onSameSideOfLine(lineStart, lineEnd, p4, nextPoint)) {
                    p4 = ep + endNormal;
                }
            }
        } else {
            p3 = ep + endNormal;
            p4 = ep - endNormal;
        }

        //bool intersect = lineIntersects(p1, p2, p3, p4); // Check if our quadrilateral is self-intersecting
        //### Not enough! It's also wrong if p1-p2 is completely on the wrong side of p3-p4
        //bool ok = onSameSideOfLine(p1, p2, p3, p4) && onSameSideOfLine(p1, p2, p3, current->endPoint());
        //### And that's still not enough, since the line can turn almost 180 degrees

        // New attempt: see if the bisector intersects the normal...
        bool notOK = lineIntersects(sp + startNormal, sp - startNormal, ep + endBisector, ep - endBisector)
                     || lineIntersects(sp + startBisector, sp - startBisector, ep + endNormal, ep - endNormal);

        // TESTING: Avoid the inner bisector extending past the stroke. We need to move the points around,
        // but for now, just accept overpainting. Only implemented for BevelJoin
        if (notOK && !miterJoin) {
            p1 = current->startPoint() + startNormal;
            p2 = current->startPoint() - startNormal;
            p3 = current->endPoint() + endNormal;
            p4 = current->endPoint() - endNormal;
        }

        if (current->isLine()) {
            // Rearrange so p1, p3 is on the same side of the path, and p2, p4 is on the other side
            if (!onSameSideOfLine(current->startPoint(), current->endPoint(), p1, p3))
                qSwap(p3, p4);
        } else {
            // Rearrange so that p1 and p3 are on the outside of the curve triangle, i.e. p2 and p4 are on the same side
            // of the respective tangent line as the path


            // p1 should be on the outside of sp-cp
            if (onSameSideOfLine(current->startPoint(), current->controlPoint(), p1, current->endPoint()))
                qSwap(p1, p2);
            // p3 should be on the outside of ep-cp
            if (onSameSideOfLine(current->endPoint(), current->controlPoint(), p3, current->startPoint()))
                qSwap(p3, p4);
        }


        bool special = notOK;

        /*
                         p
            p1                           p3

            sp                           ep

            p2                           p4

        */

        //Note: p1-sp-p2 is no longer guaranteed to be on a line, so we need to add extra triangles

        if (current->isLine() || controlPointInsideLine(*current, p1, p3)) {
            float r = current->isLine() ? 0.0f : 0.5f;
            ret.append({{p1, p2, p3}, elementIndex, {r, 1.0f, 0.0f}, special});
            ret.append({{p2, p3, p4}, elementIndex, {r, 0.0f, 1.0f}, special});
            if (!miterJoin) { // TODO: handle bevel for overlong miter
                ret.append({{p1, sp, p2}, elementIndex, {r, 0.5f, 1.0f}, special});
                ret.append({{p3, ep, p4}, elementIndex, {r, 0.5f, 1.0f}, special});
            }
            //qDebug() << "adding triangles" << p1 << p2 << p3 << "and" << p2 << p3 << p4;
        } else {

            const auto &p = current->controlPoint();
            ret.append({{p1, p2, p}, elementIndex, {1.0f, 0.0f, 0.0f}, special});
            ret.append({{p, p2, p4}, elementIndex, {1.0f, 1.0f, 0.0f}, special});
            ret.append({{p, p3, p4}, elementIndex, {1.0f, 0.0f, 1.0f}, special});
            if (!miterJoin) { // TODO: handle bevel for overlong miter
                ret.append({{p1, sp, p2}, elementIndex, {0.5f, 1.0f, 0.0f}, special});
                ret.append({{p3, ep, p4}, elementIndex, {0.5f, 1.0f, 0.0f}, special});
            }
            //qDebug() << "adding triangle1" << p1 << p2 << p << "triangle2" << p << p2 << p4 << "triangle3" << p << p3 << p4 ;


            //            qDebug() << "Curve element" << p1 << p2 << p << p3 << p4;
        }

        if (prev && !miterJoin) {
            // Try to do the bevel triangle: prev bevelpoint - sp - bevelPoint
            auto prevNormal = normalVector(*prev, true) * margin; // Do we have normals in different directions???
            auto pbp = bevelPointReversed ? sp + prevNormal : sp - prevNormal;
            //qDebug() << "bevel triangle" << pbp << sp << bevelPoint;
            ret.append({{pbp, sp, bevelPoint}, -1, {}, true});
        }
    };


    /*
        Iterate through the path. Each start element has to be postponed until the end of the sub-path.
        At that point, we have to process both the start and the end elements.

        If the end element's end point is equal to the start element's start point, we treat them as connected,
        otherwise we add null pointers
    */

    const QuadPath::Element *startElement = nullptr;
    int startElementIndex = -1;
    const QuadPath::Element *prevElement = nullptr;
    //### For now, we know that this path has not been split
    for (int i = 0; i < path.elementCount(); ++i) {
        const auto *element = &path.elementAt(i);
        // ### isSubPathEnd() doesn't work unless subPathsClosed() has been called
        bool isSubPathEnd = i == path.elementCount() - 1 || path.elementAt(i + 1).isSubpathStart();
        bool isSubPathStart = element->isSubpathStart() || i == 0; // 0 should always be subPathStart, but just to be sure
        if (isSubPathStart) {
            if (isSubPathEnd) {
                // If the element is both start and end, it's a standalone
                handleElement(nullptr, element, nullptr, i);
                startElement = nullptr; // These should be ignored, but reset just in case
                prevElement = nullptr;
            } else {
                // this element will be handled later, when we know whether the sub-path is closed
                startElement = element;
                startElementIndex = i;
                prevElement = element;
            }
            continue;
        }

        const QuadPath::Element *nextElement = nullptr;
        if (i != 0 && !element->isSubpathStart())
            prevElement = &path.elementAt(i - 1);

        if (isSubPathEnd) {
            //handle the previous start element: check if it is connected to this element
            Q_ASSERT(startElement);
            bool subPathClosed = element->endPoint() == startElement->startPoint();
            handleElement(subPathClosed ? element : nullptr, startElement, &path.elementAt(startElementIndex + 1), startElementIndex);
            if (subPathClosed)
                nextElement = startElement;
            startElement = nullptr;
         } else {
            nextElement = &path.elementAt(i + 1);
         }
         handleElement(prevElement, element, nextElement, i);
         prevElement = isSubPathEnd ? nullptr : element;
    }
    Q_ASSERT(!startElement); // Should have been handled in the loop

    return ret;
}

};

QVector<QSGGeometryNode *> QQuickShapeCurveRenderer::addNodesStrokeShader(const PathData &pathData, NodeList *debugNodes)
{
    QVector<QSGGeometryNode *> ret;

    Q_UNUSED(debugNodes);

    const QColor &color = pathData.pen.color();
    Q_UNUSED(color)

    QVector<QQuickShapeLoopBlinnNode::LoopBlinnVertex> vertexBuffer;
    QVector<quint32> indices;
    bool visualizeDebug = debugVisualization() & DebugCurves;
    const float dbg = visualizeDebug  ? 0.5f : 0.0f;
    QVector<QQuickShapeWireFrameNode::WireFrameVertex> wfVertices;

    const auto &thePath = pathData.qPath;

   // qDebug() << "Stroking" << thePath;


    // TODO: calculate margin correctly for narrow angles. If it's too big we'll get into unsafe space more often
    auto triangles = customTriangulator(thePath, float(pathData.pen.widthF()) * 1, pathData.pen.joinStyle() == Qt::MiterJoin); //### SvgMiterJoin ???

    //### TODO: reduce code duplication
    auto addVertexDataForPoint = [&vertexBuffer, &wfVertices, dbg](const QuadPath::Element &element, const QVector2D &p, const QVector3D &color, int wfIndex) {
        auto uv = element.uvForPoint(p);

        float r = color.x(), g = color.y(), b = color.z();

        vertexBuffer.append({p.x(), p.y(), uv.x(), uv.y(), uv.z(), r, g, b, dbg});
        switch (wfIndex % 3) {
        case 0:
            wfVertices.append({p.x(), p.y(), 1.0f, 0.0f, 0.0f});
            break;
        case 1:
            wfVertices.append({p.x(), p.y(), 0.0f, 1.0f, 0.0f});
            break;
        case 2:
            wfVertices.append({p.x(), p.y(), 0.0f, 0.0f, 1.0f});
            break;
        }
    };

    auto addCurveTriangle = [&](const QuadPath::Element &element, const QVector2D &sp, const QVector2D &ep, const QVector2D &cp, const QVector3D &color){
        int currentVertex = vertexBuffer.count();
        addVertexDataForPoint(element, sp, color, 0);
        addVertexDataForPoint(element, cp, color, 1);
        addVertexDataForPoint(element, ep, color, 2);
        indices << currentVertex << currentVertex + 1 << currentVertex + 2;
    };

    // TESTING
    auto addBevelTriangle = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &p3)
    {
        float r = 0.0f;
        float g = 1.0f;
        float b = 0.5f;


        // MEGA HACK!!! Invent a curve based on the triangle, since we know that:
        // p2 is the path join point
        // the stroke passes through the mid points of p2-p1 and p2-p3

        // we have static inline QVector2D curveUv(QVector2D p0, QVector2D p1, QVector2D p2, QVector2D p)
        // which finds uv coordinates for the point p, for a quadratic curve from p0 to p2 with control point p1

        QVector2D fp1 = p2 + (p3 - p1);
        QVector2D fp2 = p2 - (p3 - p1);
        QVector2D fcp = (p1 + 3 * p2 + p3) / 5; // does not matter for line

        // The bevel is not strokeWidth/2 wide, it's half this triangle's height
        // We need to shift everything down by the difference (definitely hackish)

        QVector2D vh = ((p1 - p2) + (p3 - p2)) / 2;
        float triangleHeight = vh.length();
        float delta = (pathData.pen.widthF() - triangleHeight) / 2;

        QVector2D offset = vh.normalized() * delta;
        fp1 -= offset;
        fp2 -= offset;
        fcp -= offset;


        const auto uv1 = curveUv(fp1, fcp, fp2, p1);
        const auto uv2 = curveUv(fp1, fcp, fp2, p2);
        const auto uv3 = curveUv(fp1, fcp, fp2, p3);

        const int currentVertex = vertexBuffer.count();
        vertexBuffer.append({p1.x(), p1.y(), uv1.x(), uv1.y(), 0.0f, r, g, b, dbg});
        vertexBuffer.append({p2.x(), p2.y(), uv2.x(), uv2.y(), 0.0f, r, g, b, dbg});
        vertexBuffer.append({p3.x(), p3.y(), uv3.x(), uv3.y(), 0.0f, r, g, b, dbg});
        indices << currentVertex << currentVertex + 1 << currentVertex + 2;
        wfVertices.append({p1.x(), p1.y(), 1.0f, 0.0f, 0.0f});
        wfVertices.append({p2.x(), p2.y(), 0.0f, 1.0f, 0.0f});
        wfVertices.append({p3.x(), p3.y(), 0.0f, 0.0f, 1.0f});
        //qDebug() << "bev" << p1 << p2 << p3;
    };

    static constexpr QVector3D specialDebug{0.0f, 1.0f, 1.0f};

    for (const auto &triangle : triangles) {
         if (triangle.pathElementIndex < 0) {
            //TESTING
            addBevelTriangle(triangle.points[0], triangle.points[1], triangle.points[2]);
            continue;
         }
        const auto &element = thePath.elementAt(triangle.pathElementIndex);
        addCurveTriangle(element, triangle.points[0], triangle.points[1], triangle.points[2],
                         triangle.specialDebug ? specialDebug : triangle.debugColor);
    }



    if (indices.size() > 0) {
        auto *node = new QQuickShapeLoopBlinnNode(NoGradient); // TODO: dedicated node
        node->color = Qt::transparent;
        //node->fillGradient = pathData.gradient;

        node->strokeColor = pathData.pen.color(); // this is all you need to use the stroke shader
        node->strokeWidth = pathData.pen.widthF();

        QSGGeometry *g = new QSGGeometry(QQuickShapeLoopBlinnNode::attributes(),
                                         vertexBuffer.size(), indices.size(), QSGGeometry::UnsignedIntType);
        node->setGeometry(g);
        g->setDrawingMode(QSGGeometry::DrawTriangles);

        //qDebug() << "gvc" << g->vertexCount();

        memcpy(g->vertexData(),
               vertexBuffer.data(),
               g->vertexCount() * g->sizeOfVertex());
        memcpy(g->indexData(),
               indices.data(),
               indices.size() * g->sizeOfIndex());

        m_rootNode->appendChildNode(node);
        ret.append(node);
    }
    const bool wireFrame = debugVisualization() & DebugWireframe;
    if (wireFrame) {
        QQuickShapeWireFrameNode *wfNode = new QQuickShapeWireFrameNode;

        //QVarLengthArray<quint32> indices;

        QSGGeometry *wfg = new QSGGeometry(QQuickShapeWireFrameNode::attributes(),
                                           wfVertices.size(),
                                           indices.size(),
                                           QSGGeometry::UnsignedIntType);
        wfNode->setGeometry(wfg);

        wfg->setDrawingMode(QSGGeometry::DrawTriangles);
        memcpy(wfg->indexData(),
               indices.data(),
               indices.size() * wfg->sizeOfIndex());
        memcpy(wfg->vertexData(),
               wfVertices.data(),
               wfg->vertexCount() * wfg->sizeOfVertex());

        m_rootNode->appendChildNode(wfNode);
        debugNodes->append(wfNode);
    }
    return ret;
}

//TODO: we could optimize by preprocessing e1, since we call this function multiple times on the same
// elements
static void handleOverlap(QuadPath &path, qsizetype e1, qsizetype e2, int recursionLevel = 0)
{
    if (!isOverlap(path, e1, e2)) {
        return;
    }

    if (recursionLevel > 8) {
        qDebug() << "Triangle overlap: recursion level" << recursionLevel << "aborting!";
        return;
    }

    if (path.elementAt(e1).childCount() > 1) {
        // qDebug() << "Case 1, recursion level" << recursionLevel;
        auto e11 = path.indexOfChildAt(e1, 0);
        auto e12 = path.indexOfChildAt(e1, 1);
        handleOverlap(path, e11, e2, recursionLevel + 1);
        handleOverlap(path, e12, e2, recursionLevel + 1);
    } else if (path.elementAt(e2).childCount() > 1) {
        // qDebug() << "Case 2, recursion level" << recursionLevel;
        auto e21 = path.indexOfChildAt(e2, 0);
        auto e22 = path.indexOfChildAt(e2, 1);
        handleOverlap(path, e1, e21, recursionLevel + 1);
        handleOverlap(path, e1, e22, recursionLevel + 1);
    } else {
        path.splitElementAt(e1);
        auto e11 = path.indexOfChildAt(e1, 0);
        auto e12 = path.indexOfChildAt(e1, 1);
        bool overlap1 = isOverlap(path, e11, e2);
        bool overlap2 = isOverlap(path, e12, e2);
        // qDebug() << "actual split, recursion level" << recursionLevel << "new overlaps" <<
        // overlap1 << overlap2;

        if (!overlap1 && !overlap2)
            return; // no more overlap: success!

        // We need to split more:
        if (path.elementAt(e2).isLine()) {
            // Splitting a line won't help, so we just split e1 further
            if (overlap1)
                handleOverlap(path, e11, e2, recursionLevel + 1);
            if (overlap2)
                handleOverlap(path, e12, e2, recursionLevel + 1);
        } else {
            // See if splitting e2 works:
            path.splitElementAt(e2);
            auto e21 = path.indexOfChildAt(e2, 0);
            auto e22 = path.indexOfChildAt(e2, 1);
            if (overlap1) {
                handleOverlap(path, e11, e21, recursionLevel + 1);
                handleOverlap(path, e11, e22, recursionLevel + 1);
            }
            if (overlap2) {
                handleOverlap(path, e12, e21, recursionLevel + 1);
                handleOverlap(path, e12, e22, recursionLevel + 1);
            }
        }
    }
}

// Test if element contains a start point of another element
static void handleOverlap(QuadPath &path, qsizetype e1, const QVector2D vertex,
                          int recursionLevel = 0)
{
    // First of all: Ignore the next element: it trivially overlaps (maybe not necessary: we do check for strict containment)
    if (vertex == path.elementAt(e1).endPoint() || !isOverlap(path, e1, vertex))
        return;
    if (recursionLevel > 8) {
        qDebug() << "Vertex overlap: recursion level" << recursionLevel << "aborting!";
        return;
    }

    // Don't split if we're already split
    if (path.elementAt(e1).childCount() == 0)
        path.splitElementAt(e1);

    handleOverlap(path, path.indexOfChildAt(e1, 0), vertex, recursionLevel + 1);
    handleOverlap(path, path.indexOfChildAt(e1, 1), vertex, recursionLevel + 1);
}

void QQuickShapeCurveRenderer::solveOverlaps(QuadPath &path)
{
    for (qsizetype i = 0; i < path.elementCount(); i++) {
        auto &element = path.elementAt(i);
        // only concave curve overlap is problematic, as long as we don't allow self-intersecting curves
        if (element.isLine() || element.isConvex())
            continue;

        for (qsizetype j = 0; j < path.elementCount(); j++) {
            if (i == j)
                continue; // Would be silly to test overlap with self
            auto &other = path.elementAt(j);
            if (!other.isConvex() && !other.isLine() && j < i)
                continue; // We have already tested this combination, so no need to test again
            handleOverlap(path, i, j);
        }
    }

    static const int handleConcaveJoint = qEnvironmentVariableIntValue("QT_QUICKSHAPES_WIP_CONCAVE_JOINT");
    if (handleConcaveJoint) {
        // Note that the joint between two non-concave elements can also be concave, so we have to
        // test all convex elements to see if there is a vertex in any of them. We could do it the other way
        // by identifying concave joints, but then we would have to know which side is the inside
        // TODO: optimization potential! Maybe do that at the same time as we identify concave curves?

        // We do this in a separate loop, since the triangle/triangle test above is more expensive, and
        // if we did this first, there would be more triangles to test
        for (qsizetype i = 0; i < path.elementCount(); i++) {
            auto &element = path.elementAt(i);
            if (!element.isConvex())
                continue;

            for (qsizetype j = 0; j < path.elementCount(); j++) {
                // We only need to check one point per element, since all subpaths are closed
                // Could do smartness to skip elements that cannot overlap, but let's do it the easy way first
                if (i == j)
                    continue;
                const auto &other = path.elementAt(j);
                handleOverlap(path, i, other.startPoint());
            }
        }
    }
}

QT_END_NAMESPACE
