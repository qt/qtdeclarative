// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshapecurverenderer_p.h"
#include "qquickshapecurverenderer_p_p.h"
#include "qquickshapecurvenode_p.h"
#include "qquickshapestrokenode_p.h"

#include <QtGui/qvector2d.h>
#include <QtGui/qvector4d.h>
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

// Assumes sp != ep
bool QuadPath::isPointNearLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep)
{
    // epsilon is max length of p-to-baseline relative to length of baseline. So 0.01 means that
    // the distance from p to the baseline must be less than 1% of the length of the baseline.
    constexpr float epsilon = 0.01f;
    QVector2D bv = ep - sp;
    float bl2 = QVector2D::dotProduct(bv, bv);
    float t = QVector2D::dotProduct(p - sp, bv) / bl2;
    QVector2D pv = p - (sp + t * bv);
    return (QVector2D::dotProduct(pv, pv) / bl2) < (epsilon * epsilon);
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
                flags = newFlags;
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

class ElementCutter
{
public:
    ElementCutter(const QuadPath::Element &element)
        : m_element(element)
    {
        m_currentPoint = m_element.startPoint();
        if (m_element.isLine())
            m_lineLength = (m_element.endPoint() - m_element.startPoint()).length();
        else
            fillLUT();
    }

    bool consume(float length)
    {
        m_lastT = m_currentT;
        m_lastPoint = m_currentPoint;
        float nextBreak = m_consumed + length;
        float breakT = m_element.isLine() ? nextBreak / m_lineLength : tForLength(nextBreak);
        if (breakT < 1) {
            m_currentT = breakT;
            m_currentPoint = m_element.pointAtFraction(m_currentT);
            m_consumed = nextBreak;
            return true;
        } else {
            m_currentT = 1;
            m_currentPoint = m_element.endPoint();
            return false;
        }
    }

    QVector2D currentCutPoint()
    {
        return m_currentPoint;
    }

    QVector2D currentControlPoint()
    {
        Q_ASSERT(!m_element.isLine());
        // Split curve right at lastT, yields { lastPoint, rcp, endPoint } quad segment
        QVector2D rcp = (1 - m_lastT) * m_element.controlPoint() + m_lastT * m_element.endPoint();
        // Split that left at currentT, yields  { lastPoint, lcp, currentPoint } quad segment
        float segmentT = (m_currentT - m_lastT) / (1 - m_lastT);
        QVector2D lcp = (1 - segmentT) * m_lastPoint + segmentT * rcp;
        return lcp;
    }

    float lastLength()
    {
        float elemLength = m_element.isLine() ? m_lineLength : m_lut.last();
        return elemLength - m_consumed;
    }

private:
    void fillLUT()
    {
        Q_ASSERT(!m_element.isLine());
        QVector2D ap = m_element.startPoint() - 2 * m_element.controlPoint() + m_element.endPoint();
        QVector2D bp = 2 * m_element.controlPoint() - 2 * m_element.startPoint();
        float A = 4 * QVector2D::dotProduct(ap, ap);
        float B = 4 * QVector2D::dotProduct(ap, bp);
        float C = QVector2D::dotProduct(bp, bp);
        float b = B / (2 * A);
        float c = C / A;
        float k = c - (b * b);
        float l2 = b * std::sqrt(b * b + k);
        float lnom = b + std::sqrt(b * b + k);
        float l0 = 0.5f * std::sqrt(A);

        m_lut.resize(LUTSize, 0);
        for (int i = 1; i < LUTSize; i++) {
            float t = float(i) / (LUTSize - 1);
            float u = t + b;
            float w = std::sqrt(u * u + k);
            float l1 = u * w;
            float lden = u + w;
            float l3 = k * std::log(std::fabs(lden / lnom));
            float res = l0 * (l1 - l2 + l3);
            m_lut[i] = res;
        }
    }

    float tForLength(float length)
    {
        Q_ASSERT(!m_element.isLine());
        Q_ASSERT(!m_lut.isEmpty());

        float res = 2; // I.e. invalid, outside [0, 1] range
        auto it = std::upper_bound(m_lut.cbegin(), m_lut.cend(), length);
        if (it != m_lut.cend()) {
            float nextLength = *it--;
            float prevLength = *it;
            int prevIndex = std::distance(m_lut.cbegin(), it);
            float fraction = (length - prevLength) / (nextLength - prevLength);
            res = (prevIndex + fraction) / (LUTSize - 1);
        }
        return res;
    }

    const QuadPath::Element &m_element;
    float m_lastT = 0;
    float m_currentT = 0;
    QVector2D m_lastPoint;
    QVector2D m_currentPoint;
    float m_consumed = 0;
    // For line elements:
    float m_lineLength;
    // For quadratic curve elements:
    static constexpr int LUTSize = 21;
    QVarLengthArray<float, LUTSize> m_lut;
};

QuadPath QuadPath::dashed(qreal lineWidth, const QList<qreal> &dashPattern, qreal dashOffset) const
{
    QVarLengthArray<float, 16> pattern;
    float patternLength = 0;
    for (int i = 0; i < 2 * (dashPattern.length() / 2); i++) {
        float dashLength = qMax(lineWidth * dashPattern[i], qreal(0));
        pattern.append(dashLength);
        patternLength += dashLength;
    }
    if (patternLength == 0)
        return {};

    int startIndex = 0;
    float startOffset = std::fmod(lineWidth * dashOffset, patternLength);
    if (startOffset < 0)
        startOffset += patternLength;
    for (float dashLength : pattern) {
        if (dashLength > startOffset)
            break;
        startIndex++;
        startOffset -= dashLength;
    }

    int dashIndex = startIndex;
    float offset = startOffset;
    QuadPath res;
    for (int i = 0; i < elementCount(); i++) {
        const Element &element = elementAt(i);
        if (element.isSubpathStart()) {
            res.moveTo(element.startPoint());
            dashIndex = startIndex;
            offset = startOffset;
        }
        ElementCutter cutter(element);
        while (true) {
            bool gotAll = cutter.consume(pattern.at(dashIndex) - offset);
            QVector2D nextPoint = cutter.currentCutPoint();
            if (dashIndex & 1)
                res.moveTo(nextPoint); // gap
            else if (element.isLine())
                res.lineTo(nextPoint); // dash in line
            else
                res.quadTo(cutter.currentControlPoint(), nextPoint); // dash in curve
            if (gotAll) {
                offset = 0;
                dashIndex = (dashIndex + 1) % pattern.size();
            } else {
                offset += cutter.lastLength();
                break;
            }
        }
    }
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
    pathData.m_dirty |= PathDirty;
}

void QQuickShapeCurveRenderer::setStrokeColor(int index, const QColor &color)
{
    auto &pathData = m_paths[index];
    const bool wasVisible = pathData.isStrokeVisible();
    pathData.pen.setColor(color);
    if (pathData.isStrokeVisible() != wasVisible)
        pathData.m_dirty |= StrokeDirty;
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
    pathData.m_dirty |= StrokeDirty;
}

void QQuickShapeCurveRenderer::setFillColor(int index, const QColor &color)
{
    auto &pathData = m_paths[index];
    const bool wasVisible = pathData.isFillVisible();
    pathData.fillColor = color;
    if (pathData.isFillVisible() != wasVisible)
        pathData.m_dirty |= FillDirty;
    else
        pathData.m_dirty |= UniformsDirty;
}

void QQuickShapeCurveRenderer::setFillRule(int index, QQuickShapePath::FillRule fillRule)
{
    auto &pathData = m_paths[index];
    pathData.fillRule = Qt::FillRule(fillRule);
    pathData.m_dirty |= PathDirty;
}

void QQuickShapeCurveRenderer::setJoinStyle(int index,
                                          QQuickShapePath::JoinStyle joinStyle,
                                          int miterLimit)
{
    auto &pathData = m_paths[index];
    pathData.pen.setJoinStyle(Qt::PenJoinStyle(joinStyle));
    pathData.pen.setMiterLimit(miterLimit);
    pathData.m_dirty |= StrokeDirty;
}

void QQuickShapeCurveRenderer::setCapStyle(int index, QQuickShapePath::CapStyle capStyle)
{
    auto &pathData = m_paths[index];
    pathData.pen.setCapStyle(Qt::PenCapStyle(capStyle));
    pathData.m_dirty |= StrokeDirty;
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
    pathData.m_dirty |= StrokeDirty;
}

void QQuickShapeCurveRenderer::setFillGradient(int index, QQuickShapeGradient *gradient)
{
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

    pd.m_dirty |= FillDirty;
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

void QQuickShapeCurveRenderer::updateNode()
{
    if (!m_rootNode)
        return;
    static const bool doOverlapSolving = !qEnvironmentVariableIntValue("QT_QUICKSHAPES_DISABLE_OVERLAP_SOLVER");
    static const bool useTriangulatingStroker = qEnvironmentVariableIntValue("QT_QUICKSHAPES_TRIANGULATING_STROKER");
    static const bool simplifyPath = qEnvironmentVariableIntValue("QT_QUICKSHAPES_SIMPLIFY_PATHS");

    for (PathData &pathData : m_paths) {
        int dirtyFlags = pathData.m_dirty;

        if (dirtyFlags & PathDirty) {
            if (simplifyPath)
                pathData.path = QuadPath::fromPainterPath(pathData.originalPath.simplified());
            else
                pathData.path = QuadPath::fromPainterPath(pathData.originalPath);
            pathData.path.setFillRule(pathData.fillRule);
            pathData.fillPath = {};
            dirtyFlags |= (FillDirty | StrokeDirty);
        }

        if (dirtyFlags & FillDirty) {
            deleteAndClear(&pathData.fillNodes);
            deleteAndClear(&pathData.fillDebugNodes);
            if (pathData.isFillVisible()) {
                if (pathData.fillPath.isEmpty()) {
                    pathData.fillPath = pathData.path.subPathsClosed();
                    pathData.fillPath.addCurvatureData();
                    if (doOverlapSolving)
                        solveOverlaps(pathData.fillPath);
                }
                pathData.fillNodes = addFillNodes(pathData, &pathData.fillDebugNodes);
                dirtyFlags |= StrokeDirty;
            }
        }

        if (dirtyFlags & StrokeDirty) {
            deleteAndClear(&pathData.strokeNodes);
            deleteAndClear(&pathData.strokeDebugNodes);
            if (pathData.isStrokeVisible()) {
                const QPen &pen = pathData.pen;
                if (pen.style() == Qt::SolidLine)
                    pathData.strokePath = pathData.path;
                else
                    pathData.strokePath = pathData.path.dashed(pen.widthF(), pen.dashPattern(), pen.dashOffset());

                if (useTriangulatingStroker)
                    pathData.strokeNodes = addTriangulatingStrokerNodes(pathData, &pathData.strokeDebugNodes);
                else
                    pathData.strokeNodes = addCurveStrokeNodes(pathData, &pathData.strokeDebugNodes);
            }
        }

        if (dirtyFlags & UniformsDirty) {
            if (!(dirtyFlags & FillDirty)) {
                for (auto &pathNode : std::as_const(pathData.fillNodes))
                    static_cast<QQuickShapeCurveNode *>(pathNode)->setColor(pathData.fillColor);
            }
            if (!(dirtyFlags & StrokeDirty)) {
                for (auto &strokeNode : std::as_const(pathData.strokeNodes))
                    static_cast<QQuickShapeCurveNode *>(strokeNode)->setColor(pathData.pen.color());
            }
        }

        pathData.m_dirty &= ~(PathDirty | FillDirty | StrokeDirty | UniformsDirty);
    }
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
QVector<QSGGeometryNode *> QQuickShapeCurveRenderer::addFillNodes(const PathData &pathData, NodeList *debugNodes)
{
    //qDebug() << "========= STARTING ===========" << pathData.path;
    auto *node = new QQuickShapeCurveNode;
    node->setGradientType(pathData.gradientType);

    QVector<QSGGeometryNode *> ret;
    const QColor &color = pathData.fillColor;
    QPainterPath internalHull;
    internalHull.setFillRule(pathData.fillPath.fillRule());


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

    auto addCurveTriangle = [&](const QuadPath::Element &element, const QVector2D &sp, const QVector2D &ep, const QVector2D &cp) {
        float r = 0.0f, g = 0.0f, b = 0.0f;
        if (element.isLine()) {
            g = b = 1.0f;
        } else if (element.isConvex()) {
            r = 1.0;
        } else { // concave
            b = 1.0;
        }

        QVector4D dbgColor(r, g, b, dbg);

        node->appendTriangle(sp, cp, ep,
                             [&element](QVector2D v) { return element.uvForPoint(v); },
                             dbgColor, dbgColor, dbgColor);

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
        addCurveTriangle(element, sp, ep, cp);
        // Add a triangle on the outer side of the line to get some more AA
        // The new point replaces cp
        QVector2D op = findPointOtherSide(sp, ep, cp); //sp - (cp - ep);
        addCurveTriangle(element, sp, op, ep);
    };

    auto addConvexTriangle = [&](const QuadPath::Element &element, const QVector2D &sp, const QVector2D &ep, const QVector2D &cp){
        addCurveTriangle(element, sp, ep, cp);
        // Add two triangles on the outer side to get some more AA

        // First triangle on the line sp-cp, replacing ep
        {
            QVector2D op = findPointOtherSide(sp, cp, ep); //sp - (ep - cp);
            addCurveTriangle(element, sp, cp, op);
        }

        // Second triangle on the line ep-cp, replacing sp
        {
            QVector2D op = findPointOtherSide(ep, cp, sp); //ep - (sp - cp);
            addCurveTriangle(element, op, cp, ep);
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
        addCurveTriangle(element, sp, ep, cp);
        // Add an outer triangle to give extra AA for very flat curves
        QVector2D op = oppositePoint(sp, ep, cp);
        // The new point replaces cp
        addCurveTriangle(element, sp, op, ep);
    };

    auto addFillTriangle = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &p3){
        constexpr QVector3D uv(0.0, 1.0, -1.0);
        QVector4D dbgColor(0.0f, 1.0f, 0.0f, dbg);
        node->appendTriangle(p1, p2, p3,
                             [&uv](QVector2D) { return uv; },
                             dbgColor, dbgColor, dbgColor);

        wfVertices.append({p1.x(), p1.y(), 1.0f, 0.0f, 0.0f}); // 0
        wfVertices.append({p3.x(), p3.y(), 0.0f, 1.0f, 0.0f}); // 1
        wfVertices.append({p2.x(), p2.y(), 0.0f, 0.0f, 1.0f}); // 2
    };



    for (int i = 0; i < pathData.fillPath.elementCount(); ++i)
        iteratePath(pathData.fillPath, i, [&](const QuadPath::Element &element, int index){
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
                const auto &element = pathData.fillPath.elementAt(*found);
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
                const auto &element = pathData.fillPath.elementAt(*found);
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
                const auto &element = pathData.fillPath.elementAt(*found);
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
            addLineTriangle(pathData.fillPath.elementAt(lineElementIndex), p[si], p[ei], p[ci]);
        } else if (concaveElementIndex != -1) {
            addCurveTriangle(pathData.fillPath.elementAt(concaveElementIndex), p[0], p[1], p[2]);
        } else if (convexElementIndex != -1) {
            int oi = (6 - si - ei) % 3;
            const auto &otherPoint = p[oi];
            const auto &element = pathData.fillPath.elementAt(convexElementIndex);
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

    QVector<quint32> indices = node->uncookedIndexes();
    if (indices.size() > 0) {
        node->setColor(color);
        node->setFillGradient(pathData.gradient);

        node->cookGeometry();
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

QVector<QSGGeometryNode *> QQuickShapeCurveRenderer::addTriangulatingStrokerNodes(const PathData &pathData, NodeList *debugNodes)
{
    QVector<QSGGeometryNode *> ret;
    const QColor &color = pathData.pen.color();

    bool visualizeDebug = debugVisualization() & DebugCurves;
    const float dbg = visualizeDebug  ? 0.5f : 0.0f;
    QVector<QQuickShapeWireFrameNode::WireFrameVertex> wfVertices;

    QTriangulatingStroker stroker;
    const auto painterPath = pathData.strokePath.toPainterPath();
    const QVectorPath &vp = qtVectorPathForPath(painterPath);
    QPen pen = pathData.pen;
    stroker.process(vp, pen, {}, {});

    auto *node = new QQuickShapeCurveNode;
    node->setGradientType(pathData.gradientType);

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
        //constexpr QVector3D uv(0.0, 1.0, -1.0);


        if (p1 == p2 || p2 == p3) {
            //qDebug() << "Skipping triangle" << p1 << p2 << p3;
            return;
        }

        auto uvForPoint = [&p1, &p2, &p3](QVector2D p) {
            auto uv = curveUv(p1, p2, p3, p);
            return QVector3D(uv.x(), uv.y(), 0.0f); // Line
        };

        node->appendTriangle(p1, p2, p3,
                             uvForPoint,
                             QVector4D(1.0f, 0.0, 0.0, dbg),
                             QVector4D(0.0f, 1.0, 0.0, dbg),
                             QVector4D(0.0f, 0.0, 1.0, dbg));


        wfVertices.append({p1.x(), p1.y(), 1.0f, 0.0f, 0.0f}); // 0
        wfVertices.append({p2.x(), p2.y(), 0.0f, 0.1f, 0.0f}); // 1
        wfVertices.append({p3.x(), p3.y(), 0.0f, 0.0f, 1.0f}); // 2

        if (!disableExtraTriangles) {
            // Add a triangle on the outer side of the line to get some more AA
            // The new point replaces p2 (currentVertex+1)
            QVector2D op = findPointOtherSide(p1, p3, p2);
            node->appendTriangle(p1, op, p3,
                                 uvForPoint,
                                 QVector4D(1.0f, 0.0, 0.0, dbg),
                                 QVector4D(1.0f, 1.0, 0.0, dbg),
                                 QVector4D(0.0f, 0.0, 1.0, dbg));

            wfVertices.append({p1.x(), p1.y(), 1.0f, 0.0f, 0.0f});
            wfVertices.append({op.x(), op.y(), 0.0f, 1.0f, 0.0f}); // replacing p2
            wfVertices.append({p3.x(), p3.y(), 0.0f, 0.0f, 1.0f});
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

    QVector<quint32> indices = node->uncookedIndexes();
    if (indices.size() > 0) {
        node->setColor(color);
        node->setFillGradient(pathData.gradient);

        node->cookGeometry();
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

void QQuickShapeCurveRenderer::setRootNode(QSGNode *node)
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

// The sign of the determinant tells the winding order: positive means counter-clockwise
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
    bool specialDebug = false; // Quick way of debugging special cases without having to change debugColor
};

// Returns a vector that is normal to baseLine, pointing to the right
QVector2D normalVector(QVector2D baseLine)
{
    QVector2D normal = QVector2D(-baseLine.y(), baseLine.x()).normalized();
    return normal;
}

// Returns a vector that is normal to the path and pointing to the right. If endSide is false
// the vector is normal to the start point, otherwise to the end point
QVector2D normalVector(const QuadPath::Element &element, bool endSide = false)
{
    if (element.isLine())
        return normalVector(element.endPoint() - element.startPoint());
    else if (!endSide)
        return normalVector(element.controlPoint() - element.startPoint());
    else
        return normalVector(element.endPoint() - element.controlPoint());
}

// Returns a vector that is parallel to the path. If endSide is false
// the vector starts at the start point and points forward,
// otherwise it starts at the end point and points backward
QVector2D tangentVector(const QuadPath::Element &element, bool endSide = false)
{
    if (element.isLine()) {
        if (!endSide)
            return element.endPoint() - element.startPoint();
        else
            return element.startPoint() - element.endPoint();
    } else {
        if (!endSide)
            return element.controlPoint() - element.startPoint();
        else
            return element.controlPoint() - element.endPoint();
    }
}


QVector2D miterBisector(const QuadPath::Element &element1, const QuadPath::Element &element2, float strokeMargin, float inverseMiterLimit,
                   bool *ok = nullptr, bool *pointingRight = nullptr)
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
        if (ok)
            *ok = true;
        if (pointingRight)
            *pointingRight = true;
        return (n2 - n1).normalized() * strokeMargin;
    } else {
        // We need to increase the length, so that the result covers the whole miter
        // Using the identity sin(x/2) == sqrt((1 - cos(x)) / 2), and the fact that the
        // dot product of two unit vectors is the cosine of the angle between them
        // The length of the miter is w/sin(x/2) where x is the angle between the two elements

        float cos2x = QVector2D::dotProduct(v1, v2);
        cos2x = qMin(1.0f, cos2x); // Allow for float inaccuracy
        float sine = sqrt((1.0f - cos2x) / 2);
        if (ok)
            *ok = sine >= inverseMiterLimit / 2.0f;
        if (pointingRight)
            *pointingRight = determinant(p1, p2, p3) > 0;
        sine = qMax(sine, 0.01f); // Avoid divide by zero
        return bisector.normalized() * strokeMargin / sine;
    }
}

// Really simplistic O(n^2) triangulator - only intended for five points
QList<TriangleData> simplePointTriangulator(const QList<QVector2D> &pts, int elementIndex)
{
    int count = pts.size();
    Q_ASSERT(count >= 3);
    QList<TriangleData> ret;

    ret.append({{pts[0], pts[1], pts[2]}, elementIndex, {1, 0, 0}});

    // hull is always in positive determinant winding order
    QList<QVector2D> hull;
    float det1 = determinant(pts[0], pts[1], pts[2]);
    if (det1 > 0)
        hull << pts[0] << pts[1] << pts[2];
    else
        hull << pts[2] << pts[1] << pts[0];
    auto connectableInHull = [&](const QVector2D &pt) -> QList<int> {
        QList<int> r;
        const int n = hull.size();
        for (int i = 0; i < n; ++i) {
            const auto &p1 = hull.at(i);
            const auto &p2 = hull.at((i+1) % n);
            if (determinant(p1, p2, pt) < 0.0f)
                r << i;
        }
        return r;
    };
    for (int i = 3; i < count; ++i) {
        const auto &p = pts[i];
        auto visible = connectableInHull(p);
        if (visible.isEmpty())
            continue;
        int visCount = visible.count();
        int hullCount = hull.count();
        // Find where the visible part of the hull starts. (This is the part we need to triangulate to,
        // and the part we're going to replace. "visible" contains the start point of the line segments that are visible from p.
        int boundaryStart = visible[0];
        for (int j = 0; j < visCount - 1; ++j) {
            if ((visible[j] + 1) % hullCount != visible[j+1]) {
                boundaryStart = visible[j + 1];
                break;
            }
        }
        // Now add those triangles
        for (int j = 0; j < visCount; ++j) {
            const auto &p1 = hull.at((boundaryStart + j) % hullCount);
            const auto &p2 = hull.at((boundaryStart + j+1) % hullCount);
            ret.append({{p1, p2, p}, elementIndex, {1,1,0}});
        }
        // Finally replace the points that are now inside the hull
        // We insert p after boundaryStart, and before boundaryStart + visCount (modulo...)
        // and remove the points inbetween
        int pointsToKeep = hullCount - visCount + 1;
        QList<QVector2D> newHull;
        newHull << p;
        for (int j = 0; j < pointsToKeep; ++j) {
            newHull << hull.at((j + boundaryStart + visCount) % hullCount);
        }
        hull = newHull;
    }
    return ret;
}


static QList<TriangleData> customTriangulator2(const QuadPath &path, float penWidth, Qt::PenJoinStyle joinStyle, Qt::PenCapStyle capStyle, float miterLimit)
{
    const bool bevelJoin = joinStyle == Qt::BevelJoin;
    const bool roundJoin = joinStyle == Qt::RoundJoin;
    const bool miterJoin = !bevelJoin && !roundJoin;

    const bool roundCap = capStyle == Qt::RoundCap;
    const bool squareCap = capStyle == Qt::SquareCap;

    Q_ASSERT(miterLimit > 0 || !miterJoin);
    float inverseMiterLimit = miterJoin ? 1.0f / miterLimit : 1.0;


    static const int additionalSpace = qEnvironmentVariableIntValue("QT_QUICKSHAPES_EXTRA_SPACE");

    const float extraFactor = roundJoin && additionalSpace ? (penWidth + additionalSpace) / penWidth : 2.0;

    QList<TriangleData> ret;
    int count = path.elementCount();
    int subStart = 0;
    while (subStart < count) {
         int subEnd = subStart;
         for (int i = subStart + 1; i < count; ++i) {
            const auto &e = path.elementAt(i);
            if (e.isSubpathStart()) {
                subEnd = i - 1;
                break;
            }
            if (i == count - 1) {
                subEnd = i;
                break;
            }
         }
         bool closed = path.elementAt(subStart).startPoint() == path.elementAt(subEnd).endPoint();
         const int subCount = subEnd - subStart + 1;

         auto elementAt = [&](int idx, int delta) -> const QuadPath::Element * {
             int subIdx = idx - subStart;
             if (closed) {
                 subIdx = (subIdx + subCount + delta) % subCount;
                 return &path.elementAt(subStart + subIdx);
             }
             subIdx += delta;
             if (subIdx >= 0 && subIdx < subCount)
                 return &path.elementAt(subStart + subIdx);
             return nullptr;
         };

         for (int i = subStart; i <= subEnd; ++i) {
            const auto &element = path.elementAt(i);
            const auto *nextElement = elementAt(i, +1);
            const auto *prevElement = elementAt(i, -1);

            const auto &s = element.startPoint();
            const auto &c = element.controlPoint();
            const auto &e = element.endPoint();

            // Normals point to the right
            QVector2D startNormal = normalVector(element).normalized() * (penWidth / 2);
            QVector2D endNormal = normalVector(element, true).normalized() * (penWidth / 2);

            // Bisectors point to the inside of the curve: make them point the same way as normals
            bool startBisectorPointsRight = true;
            bool startBisectorWithinMiterLimit = true;
            QVector2D startBisector = prevElement ? miterBisector(*prevElement, element, penWidth / 2, inverseMiterLimit, &startBisectorWithinMiterLimit, &startBisectorPointsRight) : startNormal;
            if (!startBisectorPointsRight)
                startBisector = -startBisector;

            bool endBisectorPointsRight = true;
            bool endBisectorWithinMiterLimit = true;
            QVector2D endBisector = nextElement ? miterBisector(element, *nextElement, penWidth / 2, inverseMiterLimit, &endBisectorWithinMiterLimit, &endBisectorPointsRight) : endNormal;
            if (!endBisectorPointsRight)
                endBisector = -endBisector;

            // We can't use the simple miter for miter joins, since the shader currently only supports round joins
            bool simpleMiter = joinStyle == Qt::RoundJoin;

            // TODO: miterLimit
            bool startMiter = simpleMiter && startBisectorWithinMiterLimit;
            bool endMiter = simpleMiter && endBisectorWithinMiterLimit;

            QVector2D p1, p2, p3, p4;
            if (startMiter) {
                //### bisector on inside can extend further than element: must limit it somehow
                p1 = s + startBisector * extraFactor;
                p2 = s - startBisector * extraFactor;
            } else {
                // TODO: remove the overdraw by using the intersection point on the inside (for lines, this is the bisector, but
                // it's more complex for non-smooth curve joins)

                // For now, simple bevel: just use normals and live with overdraw
                p1 = s + startNormal * extraFactor;
                p2 = s - startNormal * extraFactor;
            }
            // repeat logic above for the other end:
            if (endMiter) {
                p3 = e + endBisector * extraFactor;
                p4 = e - endBisector * extraFactor;
            } else {
                p3 = e + endNormal * extraFactor;
                p4 = e - endNormal * extraFactor;
            }

            // End caps

            if (!prevElement) {
                QVector2D capSpace = tangentVector(element).normalized() * -penWidth;
                if (roundCap) {
                    p1 += capSpace;
                    p2 += capSpace;
                } else if (squareCap) {
                    QVector2D c1 = p1 + capSpace;
                    QVector2D c2 = p2 + capSpace;
                    ret.append({{p1, s, c1}, -1, {1, 1, 0}, true});
                    ret.append({{c1, s, c2}, -1, {1, 1, 0}, true});
                    ret.append({{p2, s, c2}, -1, {1, 1, 0}, true});
                }
            }
            if (!nextElement) {
                QVector2D capSpace = tangentVector(element, true).normalized() * -penWidth;
                if (roundCap) {
                    p3 += capSpace;
                    p4 += capSpace;
                } else if (squareCap) {
                    QVector2D c3 = p3 + capSpace;
                    QVector2D c4 = p4 + capSpace;
                    ret.append({{p3, e, c3}, -1, {1, 1, 0}, true});
                    ret.append({{c3, e, c4}, -1, {1, 1, 0}, true});
                    ret.append({{p4, e, c4}, -1, {1, 1, 0}, true});
                }
            }

            if (element.isLine()) {
                ret.append({{p1, p2, p3}, i, {0,1,0}, false});
                ret.append({{p2, p3, p4}, i, {0.5,1,0}, false});
            } else {
                bool controlPointOnRight = determinant(s, c, e) > 0;
                QVector2D controlPointOffset = (startNormal + endNormal).normalized() * penWidth;
                QVector2D p5 = controlPointOnRight ? c - controlPointOffset : c + controlPointOffset;
                ret.append(simplePointTriangulator({p1, p2, p5, p3, p4}, i));
            }

            if (!endMiter && nextElement) {
                // inside of join (opposite of bevel) is defined by
                // triangle s, e, next.e
                QVector2D outer1, outer2;
                const auto &np = nextElement->isLine() ? nextElement->endPoint() : nextElement->controlPoint();
                bool innerOnRight = endBisectorPointsRight;
                auto nextNormal = calcNormalVector(e, np).normalized() * penWidth / 2;

                if (innerOnRight) {
                    outer1 = e - 2 * endNormal;
                    outer2 = e + 2 * nextNormal;
                } else {
                    outer1 = e + 2 * endNormal;
                    outer2 = e - 2 * nextNormal;
                }

                if (bevelJoin || (miterJoin && !endBisectorWithinMiterLimit)) {
                    ret.append({{outer1, e, outer2}, -1, {1,1,0}, false});
                } else if (roundJoin) {
                    ret.append({{outer1, e, outer2}, i, {1,1,0}, false});
                    QVector2D nn = calcNormalVector(outer1, outer2).normalized() * penWidth / 2;
                    if (!innerOnRight)
                        nn = -nn;
                    ret.append({{outer1, outer1 + nn, outer2}, i, {1,1,0}, false});
                    ret.append({{outer1 + nn, outer2, outer2 + nn}, i, {1,1,0}, false});

                } else if (miterJoin) {
                    QVector2D outer = innerOnRight ? e - endBisector * 2 : e + endBisector * 2;
                    ret.append({{outer1, e, outer}, -2, {1,1,0}, false});
                    ret.append({{outer, e, outer2}, -2, {1,1,0}, false});
                }
            }
         }
         subStart = subEnd + 1;
    }
    return ret;
}

};

QVector<QSGGeometryNode *> QQuickShapeCurveRenderer::addCurveStrokeNodes(const PathData &pathData, NodeList *debugNodes)
{
    QVector<QSGGeometryNode *> ret;
    const QColor &color = pathData.pen.color();

    auto *node = new QQuickShapeStrokeNode;

    QVector<QQuickShapeWireFrameNode::WireFrameVertex> wfVertices;

    float miterLimit = pathData.pen.miterLimit();
    float penWidth = pathData.pen.widthF();

    auto thePath = pathData.strokePath;
    auto triangles = customTriangulator2(thePath, penWidth, pathData.pen.joinStyle(), pathData.pen.capStyle(), miterLimit);

    auto addCurveTriangle = [&](const QuadPath::Element &element, const QVector2D &p0, const QVector2D &p1, const QVector2D &p2){

        if (element.isLine()) {
            node->appendTriangle(p0, p1, p2,
                                 element.startPoint(), element.endPoint());
        } else {
            node->appendTriangle(p0, p1, p2,
                                 element.startPoint(), element.controlPoint(), element.endPoint());
        }
        wfVertices.append({p0.x(), p0.y(), 1.0f, 0.0f, 0.0f});
        wfVertices.append({p1.x(), p1.y(), 0.0f, 1.0f, 0.0f});
        wfVertices.append({p2.x(), p2.y(), 0.0f, 0.0f, 1.0f});
    };

    // TESTING
    auto addBevelTriangle = [&](const QVector2D &p0, const QVector2D &p1, const QVector2D &p2)
    {
        QVector2D fp1 = p1 + (p0 - p1) / 2;
        QVector2D fp2 = p1 + (p2 - p1) / 2;
        QVector2D fcp = p1; // does not matter for line

        // That describes a path that passes through those points. We want the stroke
        // edge, so we need to shift everything down by the stroke offset

        QVector2D nn = calcNormalVector(p0, p2);
        if (determinant(p0, p1, p2) < 0)
            nn = -nn;
        float delta = penWidth / 2;

        QVector2D offset = nn.normalized() * delta;
        fp1 += offset;
        fp2 += offset;
        fcp += offset;

        node->appendTriangle(p0, p1, p2, fp1, fp2);

        wfVertices.append({p0.x(), p0.y(), 1.0f, 0.0f, 0.0f});
        wfVertices.append({p1.x(), p1.y(), 0.0f, 1.0f, 0.0f});
        wfVertices.append({p2.x(), p2.y(), 0.0f, 0.0f, 1.0f});
        //qDebug() << "bev" << p1 << p2 << p3;
    };

    for (const auto &triangle : triangles) {
         if (triangle.pathElementIndex < 0) {
            // TODO: improve bevel logic
            addBevelTriangle(triangle.points[0], triangle.points[1], triangle.points[2]);
            continue;
         }
        const auto &element = thePath.elementAt(triangle.pathElementIndex);
        addCurveTriangle(element, triangle.points[0], triangle.points[1], triangle.points[2]);
    }

    auto indexCopy = node->uncookedIndexes(); // uncookedIndexes get delete on cooking

    node->setColor(color);
    node->setStrokeWidth(pathData.pen.widthF());
    node->cookGeometry();
    m_rootNode->appendChildNode(node);
    ret.append(node);


    const bool wireFrame = debugVisualization() & DebugWireframe;
    if (wireFrame) {
        QQuickShapeWireFrameNode *wfNode = new QQuickShapeWireFrameNode;

        QSGGeometry *wfg = new QSGGeometry(QQuickShapeWireFrameNode::attributes(),
                                           wfVertices.size(),
                                           indexCopy.size(),
                                           QSGGeometry::UnsignedIntType);
        wfNode->setGeometry(wfg);

        wfg->setDrawingMode(QSGGeometry::DrawTriangles);
        memcpy(wfg->indexData(),
               indexCopy.data(),
               indexCopy.size() * wfg->sizeOfIndex());
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
