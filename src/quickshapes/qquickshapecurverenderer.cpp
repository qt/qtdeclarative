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

class QQuickShapeWireFrameNode : public QQuickShapeAbstractCurveNode
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

    void setColor(QColor col) override
    {
        Q_UNUSED(col);
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
                pathData.path = QQuadPath::fromPainterPath(pathData.originalPath.simplified());
            else
                pathData.path = QQuadPath::fromPainterPath(pathData.originalPath);
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
                    pathNode->setColor(pathData.fillColor);
            }
            if (!(dirtyFlags & StrokeDirty)) {
                for (auto &strokeNode : std::as_const(pathData.strokeNodes))
                    strokeNode->setColor(pathData.pen.color());
            }
        }

        pathData.m_dirty &= ~(PathDirty | FillDirty | StrokeDirty | UniformsDirty);
    }
}

namespace {
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

static QVector3D elementUvForPoint(const QQuadPath::Element& e, QVector2D p)
{
    auto uv = curveUv(e.startPoint(), e.controlPoint(), e.endPoint(), p);
    if (e.isLine())
        return { uv.x(), uv.y(), 0.0f };
    else
        return { uv.x(), uv.y(), e.isConvex() ? -1.0f : 1.0f };
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
void iteratePath(const QQuadPath &path, int index, Func &&lambda)
{
    const auto &element = path.elementAt(index);
    if (element.childCount() == 0) {
        lambda(element, index);
    } else {
        for (int i = 0; i < element.childCount(); ++i)
            iteratePath(path, element.indexOfChild(i), lambda);
    }
}

static inline float determinant(const QVector2D &p1, const QVector2D &p2, const QVector2D &p3)
{
    return p1.x() * (p2.y() - p3.y())
           + p2.x() * (p3.y() - p1.y())
           + p3.x() * (p1.y() - p2.y());
}
}

QQuickShapeCurveRenderer::NodeList QQuickShapeCurveRenderer::addFillNodes(const PathData &pathData,
                                                                  NodeList *debugNodes)
{
    auto *node = new QQuickShapeCurveNode;
    node->setGradientType(pathData.gradientType);

    NodeList ret;
    const QColor &color = pathData.fillColor;
    QPainterPath internalHull;
    internalHull.setFillRule(pathData.fillPath.fillRule());

    bool visualizeDebug = debugVisualization() & DebugCurves;
    const float dbg = visualizeDebug  ? 0.5f : 0.0f;
    node->setDebug(dbg);
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

    auto addCurveTriangle = [&](const QQuadPath::Element &element,
                                const QVector2D &sp,
                                const QVector2D &ep,
                                const QVector2D &cp) {
        node->appendTriangle(sp, cp, ep,
                             [&element](QVector2D v) { return elementUvForPoint(element, v); });

        wfVertices.append({sp.x(), sp.y(), 1.0f, 0.0f, 0.0f}); // 0
        wfVertices.append({cp.x(), cp.y(), 0.0f, 1.0f, 0.0f}); // 1
        wfVertices.append({ep.x(), ep.y(), 0.0f, 0.0f, 1.0f}); // 2
    };

    auto addCurveTriangleWithNormals = [&](const QQuadPath::Element &element,
                                           const std::array<QVector2D, 3> &v,
                                           const std::array<QVector2D, 3> &n) {
        node->appendTriangle(v, n, [&element](QVector2D v) { return elementUvForPoint(element, v); });
        wfVertices.append({v[0].x(), v[0].y(), 1.0f, 0.0f, 0.0f}); // 0
        wfVertices.append({v[1].x(), v[1].y(), 0.0f, 1.0f, 0.0f}); // 1
        wfVertices.append({v[2].x(), v[2].y(), 0.0f, 0.0f, 1.0f}); // 2
    };

    auto outsideNormal = [](const QVector2D &startPoint,
                                 const QVector2D &endPoint,
                                 const QVector2D &insidePoint) {

        QVector2D baseLine = endPoint - startPoint;
        QVector2D insideVector = insidePoint - startPoint;
        QVector2D normal = QVector2D(-baseLine.y(), baseLine.x()).normalized();

        bool swap = QVector2D::dotProduct(insideVector, normal) < 0;

        return swap ? normal : -normal;
    };

    auto addTriangleForLine = [&](const QQuadPath::Element &element,
                               const QVector2D &sp,
                               const QVector2D &ep,
                               const QVector2D &cp) {
        addCurveTriangle(element, sp, ep, cp);

        // Add triangles on the outer side to make room for AA
        const QVector2D normal = outsideNormal(sp, ep, cp);
        constexpr QVector2D null;
        addCurveTriangleWithNormals(element, {sp, sp, ep}, {null, normal, null});
        addCurveTriangleWithNormals(element, {sp, ep, ep}, {normal, normal, null});
    };

    auto addTriangleForConcave = [&](const QQuadPath::Element &element,
                                  const QVector2D &sp,
                                  const QVector2D &ep,
                                  const QVector2D &cp) {
        addTriangleForLine(element, sp, ep, cp);
    };

    auto addTriangleForConvex = [&](const QQuadPath::Element &element,
                                 const QVector2D &sp,
                                 const QVector2D &ep,
                                 const QVector2D &cp) {
        addCurveTriangle(element, sp, ep, cp);
        // Add two triangles on the outer side to get some more AA

        constexpr QVector2D null;
        // First triangle on the line sp-cp, replacing ep
        {
            const QVector2D normal = outsideNormal(sp, cp, ep);
            addCurveTriangleWithNormals(element, {sp, sp, cp}, {null, normal, null});
        }

        // Second triangle on the line ep-cp, replacing sp
        {
            const QVector2D normal = outsideNormal(ep, cp, sp);
            addCurveTriangleWithNormals(element, {ep, ep, cp}, {null, normal, null});
        }
    };

    auto addFillTriangle = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &p3) {
        constexpr QVector3D uv(0.0, 1.0, -1.0);
        node->appendTriangle(p1, p2, p3, [&uv](QVector2D) { return uv; } );

        wfVertices.append({p1.x(), p1.y(), 1.0f, 0.0f, 0.0f}); // 0
        wfVertices.append({p3.x(), p3.y(), 0.0f, 1.0f, 0.0f}); // 1
        wfVertices.append({p2.x(), p2.y(), 0.0f, 0.0f, 1.0f}); // 2
    };

    for (int i = 0; i < pathData.fillPath.elementCount(); ++i) {
        iteratePath(pathData.fillPath, i, [&](const QQuadPath::Element &element, int index) {
            QPointF sp(element.startPoint().toPointF());  //### to much conversion to and from pointF
            QPointF cp(element.controlPoint().toPointF());
            QPointF ep(element.endPoint().toPointF());
            if (element.isSubpathStart())
                internalHull.moveTo(sp);
            if (element.isLine()) {
                internalHull.lineTo(ep);
                linePointHash.insert(toRoundedPair(sp), index);
            } else {
                if (element.isConvex()) {
                    internalHull.lineTo(ep);
                    addTriangleForConvex(element, toRoundedVec2D(sp), toRoundedVec2D(ep), toRoundedVec2D(cp));
                    convexPointHash.insert(toRoundedPair(sp), index);
                } else {
                    internalHull.lineTo(cp);
                    internalHull.lineTo(ep);
                    addTriangleForConcave(element, toRoundedVec2D(sp), toRoundedVec2D(ep), toRoundedVec2D(cp));
                    concaveControlPointHash.insert(toRoundedPair(cp), index);
                }
            }
        });
    }

    auto makeHashable = [](const QVector2D &p) -> QPair<float, float> {
        return qMakePair(qRound(p.x() * 32.0f) / 32.0f, qRound(p.y() * 32.0f) / 32.0f);
    };
    // Points in p are already rounded do 1/32
    // Returns false if the triangle needs to be split. Adds the triangle to the graphics buffers and returns true otherwise.
    // (Does not handle ambiguous vertices that are on multiple unrelated lines/curves)
    auto onSameSideOfLine = [](const QVector2D &p1,
                               const QVector2D &p2,
                               const QVector2D &linePoint,
                               const QVector2D &lineNormal) {
        float side1 = testSideOfLineByNormal(linePoint, lineNormal, p1);
        float side2 = testSideOfLineByNormal(linePoint, lineNormal, p2);
        return side1 * side2 >= 0;
    };

    auto pointInSafeSpace = [&](const QVector2D &p, const QQuadPath::Element &element) -> bool {
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
                for (int j = 0; j < 3; ++j) {
                    if (i != j && roundVec2D(element.endPoint()) == p[j]) {
                        if (foundElement)
                            return false; // More than one edge on path: must split
                        lineElementIndex = *found;
                        si = i;
                        ei = j;
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
                        si = i;
                        ei = j;
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
                        foundElement = true;
                    }
                }
            }
        }
        if (lineElementIndex != -1) {
            int ci = (6 - si - ei) % 3; // 1+2+3 is 6, so missing number is 6-n1-n2
            addTriangleForLine(pathData.fillPath.elementAt(lineElementIndex), p[si], p[ei], p[ci]);
        } else if (concaveElementIndex != -1) {
            addCurveTriangle(pathData.fillPath.elementAt(concaveElementIndex), p[0], p[1], p[2]);
        } else if (convexElementIndex != -1) {
            int oi = (6 - si - ei) % 3;
            const auto &otherPoint = p[oi];
            const auto &element = pathData.fillPath.elementAt(convexElementIndex);
            // We have to test whether the triangle can cross the line
            // TODO: use the toplevel element's safe space
            bool safeSpace = pointInSafeSpace(otherPoint, element);
            if (safeSpace) {
                addCurveTriangle(element, p[0], p[1], p[2]);
            } else {
                // Find a point inside the triangle that's also in the safe space
                QVector2D newPoint = (p[0] +  p[1] + p[2]) / 3;
                // We should calculate the point directly, but just do a lazy implementation for now:
                for (int i = 0; i < 7; ++i) {
                    safeSpace = pointInSafeSpace(newPoint, element);
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
            p[i] = toRoundedVec2D(QPointF(triangles.vertices.at(idx[i] * 2),
                                          triangles.vertices.at(idx[i] * 2 + 1)));
        }
        if (qFuzzyIsNull(determinant(p[0], p[1], p[2])))
            continue; // Skip degenerate triangles
        bool needsSplit = !handleTriangle(p);
        if (needsSplit) {
            QVector2D c = (p[0] + p[1] + p[2]) / 3;
            for (int i = 0; i < 3; ++i) {
                qSwap(c, p[i]);
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

QQuickShapeCurveRenderer::NodeList QQuickShapeCurveRenderer::addTriangulatingStrokerNodes(const PathData &pathData, NodeList *debugNodes)
{
    NodeList ret;
    const QColor &color = pathData.pen.color();

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
        QVector2D normal = QVector2D(-baseLine.y(), baseLine.x()); // TODO: limit size of triangle

        bool swap = QVector2D::dotProduct(insideVector, normal) < 0;

        return swap ? startPoint + normal : startPoint - normal;
    };

    static bool disableExtraTriangles = qEnvironmentVariableIntValue("QT_QUICKSHAPES_WIP_DISABLE_EXTRA_STROKE_TRIANGLES");

    auto addStrokeTriangle = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &p3, bool){
        if (p1 == p2 || p2 == p3) {
            return;
        }

        auto uvForPoint = [&p1, &p2, &p3](QVector2D p) {
            auto uv = curveUv(p1, p2, p3, p);
            return QVector3D(uv.x(), uv.y(), 0.0f); // Line
        };

        node->appendTriangle(p1, p2, p3, uvForPoint);


        wfVertices.append({p1.x(), p1.y(), 1.0f, 0.0f, 0.0f}); // 0
        wfVertices.append({p2.x(), p2.y(), 0.0f, 0.1f, 0.0f}); // 1
        wfVertices.append({p3.x(), p3.y(), 0.0f, 0.0f, 1.0f}); // 2

        if (!disableExtraTriangles) {
            // Add a triangle on the outer side of the line to get some more AA
            // The new point replaces p2 (currentVertex+1)
            QVector2D op = findPointOtherSide(p1, p3, p2);
            node->appendTriangle(p1, op, p3, uvForPoint);

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
using TrianglePoints = std::array<QVector2D, 3>;
using LinePoints = std::array<QVector2D, 2>;

// The sign of the determinant tells the winding order: positive means counter-clockwise

static inline double determinant(const TrianglePoints &p)
{
    return determinant(p[0], p[1], p[2]);
}

// Fix the triangle so that the determinant is positive
static void fixWinding(TrianglePoints &p)
{
    double det = determinant(p);
    if (det < 0.0) {
        qSwap(p[0], p[1]);
    }
}

// Return true if the determinant is negative, i.e. if the winding order is opposite of the triangle p1,p2,p3.
// This means that p is strictly on the other side of p1-p2 relative to p3 [where p1,p2,p3 is a triangle with
// a positive determinant].
bool checkEdge(QVector2D &p1, QVector2D &p2, QVector2D &p, float epsilon)
{
    return determinant(p1, p2, p) <= epsilon;
}


bool checkTriangleOverlap(TrianglePoints &triangle1, TrianglePoints &triangle2, float epsilon = 1.0/32)
{
    // See if there is an edge of triangle1 such that all vertices in triangle2 are on the opposite side
    fixWinding(triangle1);
    for (int i = 0; i < 3; i++) {
        int ni = (i + 1) % 3;
        if (checkEdge(triangle1[i], triangle1[ni], triangle2[0], epsilon) &&
            checkEdge(triangle1[i], triangle1[ni], triangle2[1], epsilon) &&
            checkEdge(triangle1[i], triangle1[ni], triangle2[2], epsilon))
            return false;
    }

    // See if there is an edge of triangle2 such that all vertices in triangle1 are on the opposite side
    fixWinding(triangle2);
    for (int i = 0; i < 3; i++) {
        int ni = (i + 1) % 3;

        if (checkEdge(triangle2[i], triangle2[ni], triangle1[0], epsilon) &&
            checkEdge(triangle2[i], triangle2[ni], triangle1[1], epsilon) &&
            checkEdge(triangle2[i], triangle2[ni], triangle1[2], epsilon))
            return false;
    }

    return true;
}

bool checkLineTriangleOverlap(TrianglePoints &triangle, LinePoints &line, float epsilon = 1.0/32)
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
    fixWinding(triangle);
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

    bool allNegative = d1 < -epsilon && d2 < -epsilon && d3 < -epsilon;
    bool allPositive = d1 > epsilon && d2 > epsilon && d3 > epsilon;

    return allNegative || allPositive;
}

// e1 is always a concave curve. e2 can be curve or line
static bool isOverlap(const QQuadPath &path, int e1, int e2)
{
    const QQuadPath::Element &element1 = path.elementAt(e1);
    const QQuadPath::Element &element2 = path.elementAt(e2);

    TrianglePoints t1{ element1.startPoint(), element1.controlPoint(), element1.endPoint() };

    if (element2.isLine()) {
        LinePoints line{ element2.startPoint(), element2.endPoint() };
        return checkLineTriangleOverlap(t1, line);
    } else {
        TrianglePoints t2{ element2.startPoint(), element2.controlPoint(), element2.endPoint() };
        return checkTriangleOverlap(t1, t2);
    }

    return false;
}

static bool isOverlap(const QQuadPath &path, int index, const QVector2D &vertex)
{
    const QQuadPath::Element &elem = path.elementAt(index);
    return checkTriangleContains(vertex, elem.startPoint(), elem.controlPoint(), elem.endPoint());
}

struct TriangleData
{
    TrianglePoints points;
    int pathElementIndex;
    TrianglePoints normals;
};

// Returns a vector that is normal to baseLine, pointing to the right
QVector2D normalVector(QVector2D baseLine)
{
    QVector2D normal = QVector2D(-baseLine.y(), baseLine.x()).normalized();
    return normal;
}

// Returns a vector that is normal to the path and pointing to the right. If endSide is false
// the vector is normal to the start point, otherwise to the end point
QVector2D normalVector(const QQuadPath::Element &element, bool endSide = false)
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
QVector2D tangentVector(const QQuadPath::Element &element, bool endSide = false)
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

// Really simplistic O(n^2) triangulator - only intended for five points
QList<TriangleData> simplePointTriangulator(const QList<QVector2D> &pts, const QList<QVector2D> &normals, int elementIndex)
{
    int count = pts.size();
    Q_ASSERT(count >= 3);
    Q_ASSERT(normals.size() == count);

    // First we find the convex hull: it's always in positive determinant winding order
    QList<int> hull;
    float det1 = determinant(pts[0], pts[1], pts[2]);
    if (det1 > 0)
        hull << 0 << 1 << 2;
    else
        hull << 2 << 1 << 0;
    auto connectableInHull = [&](int idx) -> QList<int> {
        QList<int> r;
        const int n = hull.size();
        const auto &pt = pts[idx];
        for (int i = 0; i < n; ++i) {
            const auto &i1 = hull.at(i);
            const auto &i2 = hull.at((i+1) % n);
            if (determinant(pts[i1], pts[i2], pt) < 0.0f)
                r << i;
        }
        return r;
    };
    for (int i = 3; i < count; ++i) {
        auto visible = connectableInHull(i);
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
        // Finally replace the points that are now inside the hull
        // We insert the new point after boundaryStart, and before boundaryStart + visCount (modulo...)
        // and remove the points in between
        int pointsToKeep = hullCount - visCount + 1;
        QList<int> newHull;
        newHull << i;
        for (int j = 0; j < pointsToKeep; ++j) {
            newHull << hull.at((j + boundaryStart + visCount) % hullCount);
        }
        hull = newHull;
    }

    // Now that we have a convex hull, we can trivially triangulate it
    QList<TriangleData> ret;
    for (int i = 1; i < hull.size() - 1; ++i) {
        int i0 = hull[0];
        int i1 = hull[i];
        int i2 = hull[i+1];
        ret.append({{pts[i0], pts[i1], pts[i2]}, elementIndex, {normals[i0], normals[i1], normals[i2]}});
    }
    return ret;
}

static bool needsSplit(const QQuadPath::Element &el, float penWidth)
{
    Q_UNUSED(penWidth)
    const auto v1 = el.controlPoint() - el.startPoint();
    const auto v2 = el.endPoint() - el.controlPoint();
    float cos = QVector2D::dotProduct(v1, v2) / (v1.length() * v2.length());
    return cos < 0.9;
}
static void splitElementIfNecessary(QQuadPath &path, int index, float penWidth)
{
    auto &e = path.elementAt(index);
    if (e.isLine())
        return;
    if (e.childCount() == 0) {
        if (needsSplit(e, penWidth))
            path.splitElementAt(index);
    } else {
        for (int i = 0; i < e.childCount(); ++i)
            splitElementIfNecessary(path, e.indexOfChild(i), penWidth);
    }
}

static QQuadPath subdivide(const QQuadPath &path, int subdivisions, float penWidth)
{
    QQuadPath newPath = path;

    for (int i = 0; i < subdivisions; ++i)
        for (int j = 0; j < newPath.elementCount(); j++)
            splitElementIfNecessary(newPath, j, penWidth);
    return newPath;
}

static QList<TriangleData> customTriangulator2(const QQuadPath &path, float penWidth, Qt::PenJoinStyle joinStyle, Qt::PenCapStyle capStyle, float miterLimit)
{
    const bool bevelJoin = joinStyle == Qt::BevelJoin;
    const bool roundJoin = joinStyle == Qt::RoundJoin;
    const bool miterJoin = !bevelJoin && !roundJoin;

    const bool roundCap = capStyle == Qt::RoundCap;
    const bool squareCap = capStyle == Qt::SquareCap;
    // We can't use the simple miter for miter joins, since the shader currently only supports round joins
    const bool simpleMiter = joinStyle == Qt::RoundJoin;

    Q_ASSERT(miterLimit > 0 || !miterJoin);
    float inverseMiterLimit = miterJoin ? 1.0f / miterLimit : 1.0;

    const float penFactor = penWidth / 2;

    // Returns {inner1, inner2, outer1, outer2, outerMiter}
    // where foo1 is for the end of element1 and foo2 is for the start of element2
    // and inner1 == inner2 unless we had to give up finding a decent point
    auto calculateJoin = [&](const QQuadPath::Element *element1, const QQuadPath::Element *element2,
                             bool &outerBisectorWithinMiterLimit, bool &innerIsRight, bool &giveUp) -> std::array<QVector2D, 5>
    {
        outerBisectorWithinMiterLimit = true;
        innerIsRight = true;
        giveUp = false;
        if (!element1) {
            Q_ASSERT(element2);
            QVector2D n = normalVector(*element2).normalized();
            return {n, n, -n, -n, -n};
        }
        if (!element2) {
            Q_ASSERT(element1);
            QVector2D n = normalVector(*element1, true).normalized();
            return {n, n, -n, -n, -n};
        }

        Q_ASSERT(element1->endPoint() == element2->startPoint());

        const auto p1 = element1->isLine() ? element1->startPoint() : element1->controlPoint();
        const auto p2 = element1->endPoint();
        const auto p3 = element2->isLine() ? element2->endPoint() : element2->controlPoint();

        const auto v1 = (p1 - p2).normalized();
        const auto v2 = (p3 - p2).normalized();
        const auto b = (v1 + v2);

        constexpr float epsilon = 1.0f / 32.0f;
        bool smoothJoin = qAbs(b.x()) < epsilon && qAbs(b.y()) < epsilon;

        if (smoothJoin) {
            // v1 and v2 are almost parallel and pointing in opposite directions
            // angle bisector formula will give an almost null vector: use normal of bisector of normals instead
            QVector2D n1(-v1.y(), v1.x());
            QVector2D n2(-v2.y(), v2.x());
            QVector2D n = (n2 - n1).normalized();
            return {n, n, -n, -n, -n};
        }
        // Calculate the length of the bisector, so it will cover the entire miter.
        // Using the identity sin(x/2) == sqrt((1 - cos(x)) / 2), and the fact that the
        // dot product of two unit vectors is the cosine of the angle between them
        // The length of the miter is w/sin(x/2) where x is the angle between the two elements

        const auto bisector = b.normalized();
        float cos2x = QVector2D::dotProduct(v1, v2);
        cos2x = qMin(1.0f, cos2x); // Allow for float inaccuracy
        float sine = sqrt((1.0f - cos2x) / 2);
        innerIsRight = determinant(p1, p2, p3) > 0;
        sine = qMax(sine, 0.01f); // Avoid divide by zero
        float length = penFactor / sine;

        // Check if bisector is longer than one of the lines it's trying to bisect

        auto tooLong = [](QVector2D p1, QVector2D p2, QVector2D n, float length, float margin) -> bool {
            auto v = p2 - p1;
            // It's too long if the projection onto the bisector is longer than the bisector
            // and the projection onto the normal to the bisector is shorter
            // than the pen margin (that projection is just v - proj)
            // (we're adding a 10% safety margin to make room for AA -- not exact)
            auto projLen = QVector2D::dotProduct(v, n);
            return projLen * 0.9f < length && (v - n * projLen).length() * 0.9 < margin;
        };


        // The angle bisector of the tangent lines is not correct for curved lines. We could fix this by calculating
        // the exact intersection point, but for now just give up and use the normals.

        giveUp = !element1->isLine() || !element2->isLine()
                 || tooLong(p1, p2, bisector, length, penFactor)
                 || tooLong(p3, p2, bisector, length, penFactor);
        outerBisectorWithinMiterLimit = sine >= inverseMiterLimit / 2.0f;
        bool simpleJoin = simpleMiter && outerBisectorWithinMiterLimit && !giveUp;
        const QVector2D bn = bisector / sine;

        if (simpleJoin)
            return {bn, bn, -bn, -bn, -bn}; // We only have one inner and one outer point TODO: change inner point when conflict/curve
        const QVector2D n1 = normalVector(*element1, true).normalized();
        const QVector2D n2 = normalVector(*element2).normalized();
        if (giveUp) {
            if (innerIsRight)
                return {n1, n2, -n1, -n2, -bn};
            else
                return {-n1, -n2, n1, n2, -bn};

        } else {
            if (innerIsRight)
                return {bn, bn, -n1, -n2, -bn};
            else
                return {bn, bn, n1, n2, -bn};
        }
    };

    QList<TriangleData> ret;

    auto triangulateCurve = [&](int idx, const QVector2D &p1, const QVector2D &p2, const QVector2D &p3, const QVector2D &p4,
                                const QVector2D &n1, const QVector2D &n2, const QVector2D &n3, const QVector2D &n4)
    {
        const auto &element = path.elementAt(idx);
        const auto &s = element.startPoint();
        const auto &c = element.controlPoint();
        const auto &e = element.endPoint();
        // TODO: Don't flatten the path in addCurveStrokeNodes, but iterate over the children here instead
        bool controlPointOnRight = determinant(s, c, e) > 0;
        QVector2D startNormal = normalVector(element).normalized();
        QVector2D endNormal = normalVector(element, true).normalized();
        QVector2D controlPointNormal = (startNormal + endNormal).normalized();
        if (controlPointOnRight)
            controlPointNormal = -controlPointNormal;
        QVector2D p5 = c + controlPointNormal * penFactor; // This is too simplistic
        TrianglePoints t1{p1, p2, p5};
        TrianglePoints t2{p3, p4, p5};
        bool simpleCase = !checkTriangleOverlap(t1, t2);

        if (simpleCase) {
            ret.append({{p1, p2, p5}, idx, {n1, n2, controlPointNormal}});
            ret.append({{p3, p4, p5}, idx, {n3, n4, controlPointNormal}});
            if (controlPointOnRight) {
                ret.append({{p1, p3, p5}, idx, {n1, n3, controlPointNormal}});
            } else {
                ret.append({{p2, p4, p5}, idx, {n2, n4, controlPointNormal}});
            }
        } else {
            ret.append(simplePointTriangulator({p1, p2, p5, p3, p4}, {n1, n2, controlPointNormal, n3, n4}, idx));
        }
    };

    // Each element is calculated independently, so we don't have to special-case closed sub-paths.
    // Take care so the end points of one element are precisely equal to the start points of the next.
    // Any additional triangles needed for joining are added at the end of the current element.

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

         auto addIdx = [&](int idx, int delta) -> int {
             int subIdx = idx - subStart;
             if (closed)
                 subIdx = (subIdx + subCount + delta) % subCount;
             else
                 subIdx += delta;
             return subStart + subIdx;
         };
         auto elementAt = [&](int idx, int delta) -> const QQuadPath::Element * {
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
            const auto &e = element.endPoint();

            bool startInnerIsRight;
            bool startBisectorWithinMiterLimit; // Not used
            bool giveUpOnStartJoin; // Not used
            auto startJoin = calculateJoin(prevElement, &element,
                                           startBisectorWithinMiterLimit, startInnerIsRight,
                                           giveUpOnStartJoin);
            const QVector2D &startInner = startJoin[1];
            const QVector2D &startOuter = startJoin[3];

            bool endInnerIsRight;
            bool endBisectorWithinMiterLimit;
            bool giveUpOnEndJoin;
            auto endJoin = calculateJoin(&element, nextElement,
                                         endBisectorWithinMiterLimit, endInnerIsRight,
                                         giveUpOnEndJoin);
            QVector2D endInner = endJoin[0];
            QVector2D endOuter = endJoin[2];
            QVector2D nextOuter = endJoin[3];
            QVector2D outerB = endJoin[4];

            QVector2D p1, p2, p3, p4;
            QVector2D n1, n2, n3, n4;

            if (startInnerIsRight) {
                n1 = startInner;
                n2 = startOuter;
            } else {
                n1 = startOuter;
                n2 = startInner;
            }

            p1 = s + n1 * penFactor;
            p2 = s + n2 * penFactor;

            // repeat logic above for the other end:
            if (endInnerIsRight) {
                n3 = endInner;
                n4 = endOuter;
            } else {
                n3 = endOuter;
                n4 = endInner;
            }

            p3 = e + n3 * penFactor;
            p4 = e + n4 * penFactor;

            // End caps

            if (!prevElement) {
                QVector2D capSpace = tangentVector(element).normalized() * -penFactor;
                if (roundCap) {
                    p1 += capSpace;
                    p2 += capSpace;
                } else if (squareCap) {
                    QVector2D c1 = p1 + capSpace;
                    QVector2D c2 = p2 + capSpace;
                    ret.append({{p1, s, c1}, -1, {}});
                    ret.append({{c1, s, c2}, -1, {}});
                    ret.append({{p2, s, c2}, -1, {}});
                }
            }
            if (!nextElement) {
                QVector2D capSpace = tangentVector(element, true).normalized() * -penFactor;
                if (roundCap) {
                    p3 += capSpace;
                    p4 += capSpace;
                } else if (squareCap) {
                    QVector2D c3 = p3 + capSpace;
                    QVector2D c4 = p4 + capSpace;
                    ret.append({{p3, e, c3}, -1, {}});
                    ret.append({{c3, e, c4}, -1, {}});
                    ret.append({{p4, e, c4}, -1, {}});
                }
            }

            if (element.isLine()) {
                ret.append({{p1, p2, p3}, i, {n1, n2, n3}});
                ret.append({{p2, p3, p4}, i, {n2, n3, n4}});
            } else {
                triangulateCurve(i, p1, p2, p3, p4, n1, n2, n3, n4);
            }

            bool trivialJoin = simpleMiter && endBisectorWithinMiterLimit && !giveUpOnEndJoin;
            if (!trivialJoin && nextElement) {
                // inside of join (opposite of bevel) is defined by
                // triangle s, e, next.e
                bool innerOnRight = endInnerIsRight;

                const auto outer1 = e + endOuter * penFactor;
                const auto outer2 = e + nextOuter * penFactor;
                //const auto inner = e + endInner * penFactor;

                if (bevelJoin || (miterJoin && !endBisectorWithinMiterLimit)) {
                    ret.append({{outer1, e, outer2}, -1, {}});
                } else if (roundJoin) {
                    ret.append({{outer1, e, outer2}, i, {}});
                    QVector2D nn = calcNormalVector(outer1, outer2).normalized() * penFactor;
                    if (!innerOnRight)
                        nn = -nn;
                    ret.append({{outer1, outer1 + nn, outer2}, i, {}});
                    ret.append({{outer1 + nn, outer2, outer2 + nn}, i, {}});

                } else if (miterJoin) {
                    QVector2D outer = e + outerB * penFactor;
                    ret.append({{outer1, e, outer}, -2, {}});
                    ret.append({{outer, e, outer2}, -2, {}});
                }

                if (!giveUpOnEndJoin) {
                    QVector2D inner = e + endInner * penFactor;
                    ret.append({{inner, e, outer1}, i, {endInner, {}, endOuter}});
                    // The remaining triangle ought to be done by nextElement, but we don't have start join logic there (yet)
                    int nextIdx = addIdx(i, +1);
                    ret.append({{inner, e, outer2}, nextIdx, {endInner, {}, nextOuter}});
                }
            }
         }
         subStart = subEnd + 1;
    }
    return ret;
}

};

QQuickShapeCurveRenderer::NodeList QQuickShapeCurveRenderer::addCurveStrokeNodes(const PathData &pathData, NodeList *debugNodes)
{
    NodeList ret;
    const QColor &color = pathData.pen.color();

    const bool debug = debugVisualization() & DebugCurves;
    auto *node = new QQuickShapeStrokeNode;
    node->setDebug(0.2f * debug);
    QVector<QQuickShapeWireFrameNode::WireFrameVertex> wfVertices;

    float miterLimit = pathData.pen.miterLimit();
    float penWidth = pathData.pen.widthF();

    static const int subdivisions = qEnvironmentVariable("QT_QUICKSHAPES_STROKE_SUBDIVISIONS", QStringLiteral("3")).toInt();
    auto thePath = subdivide(pathData.strokePath, subdivisions, penWidth).flattened(); // TODO: don't flatten, but handle it in the triangulator
    auto triangles = customTriangulator2(thePath, penWidth, pathData.pen.joinStyle(), pathData.pen.capStyle(), miterLimit);

    auto addCurveTriangle = [&](const QQuadPath::Element &element, const TriangleData &t){
        const QVector2D &p0 = t.points[0];
        const QVector2D &p1 = t.points[1];
        const QVector2D &p2 = t.points[2];
        if (element.isLine()) {
            node->appendTriangle(t.points,
                                 LinePoints{element.startPoint(), element.endPoint()},
                                 t.normals);
        } else {
            node->appendTriangle(t.points,
                                 { element.startPoint(), element.controlPoint(), element.endPoint()},
                                 t.normals);
        }
        wfVertices.append({p0.x(), p0.y(), 1.0f, 0.0f, 0.0f});
        wfVertices.append({p1.x(), p1.y(), 0.0f, 1.0f, 0.0f});
        wfVertices.append({p2.x(), p2.y(), 0.0f, 0.0f, 1.0f});
    };

    auto addBevelTriangle = [&](const TrianglePoints &p)
    {
        QVector2D fp1 = p[0];
        QVector2D fp2 = p[2];

        // That describes a path that passes through those points. We want the stroke
        // edge, so we need to shift everything down by the stroke offset

        QVector2D nn = calcNormalVector(p[0], p[2]);
        if (determinant(p) < 0)
            nn = -nn;
        float delta = penWidth / 2;

        QVector2D offset = nn.normalized() * delta;
        fp1 += offset;
        fp2 += offset;

        TrianglePoints n;
        // p1 is inside, so n[1] is {0,0}
        n[0] = (p[0] - p[1]).normalized();
        n[2] = (p[2] - p[1]).normalized();

        node->appendTriangle(p, LinePoints{fp1, fp2}, n);

        wfVertices.append({p[0].x(), p[0].y(), 1.0f, 0.0f, 0.0f});
        wfVertices.append({p[1].x(), p[1].y(), 0.0f, 1.0f, 0.0f});
        wfVertices.append({p[2].x(), p[2].y(), 0.0f, 0.0f, 1.0f});
    };

    for (const auto &triangle : triangles) {
         if (triangle.pathElementIndex < 0) {
            addBevelTriangle(triangle.points);
            continue;
         }
        const auto &element = thePath.elementAt(triangle.pathElementIndex);
        addCurveTriangle(element, triangle);
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

// TODO: we could optimize by preprocessing e1, since we call this function multiple times on the same
// elements
static void handleOverlap(QQuadPath &path, int e1, int e2, int recursionLevel = 0)
{
    if (!isOverlap(path, e1, e2)) {
        return;
    }

    if (recursionLevel > 8) {
        qCDebug(lcShapeCurveRenderer) << "Triangle overlap: recursion level" << recursionLevel << "aborting!";
        return;
    }

    if (path.elementAt(e1).childCount() > 1) {
        auto e11 = path.indexOfChildAt(e1, 0);
        auto e12 = path.indexOfChildAt(e1, 1);
        handleOverlap(path, e11, e2, recursionLevel + 1);
        handleOverlap(path, e12, e2, recursionLevel + 1);
    } else if (path.elementAt(e2).childCount() > 1) {
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
static void handleOverlap(QQuadPath &path, int e1, const QVector2D vertex, int recursionLevel = 0)
{
    // First of all: Ignore the next element: it trivially overlaps (maybe not necessary: we do check for strict containment)
    if (vertex == path.elementAt(e1).endPoint() || !isOverlap(path, e1, vertex))
        return;
    if (recursionLevel > 8) {
        qCDebug(lcShapeCurveRenderer) << "Vertex overlap: recursion level" << recursionLevel << "aborting!";
        return;
    }

    // Don't split if we're already split
    if (path.elementAt(e1).childCount() == 0)
        path.splitElementAt(e1);

    handleOverlap(path, path.indexOfChildAt(e1, 0), vertex, recursionLevel + 1);
    handleOverlap(path, path.indexOfChildAt(e1, 1), vertex, recursionLevel + 1);
}

void QQuickShapeCurveRenderer::solveOverlaps(QQuadPath &path)
{
    for (int i = 0; i < path.elementCount(); i++) {
        auto &element = path.elementAt(i);
        // only concave curve overlap is problematic, as long as we don't allow self-intersecting curves
        if (element.isLine() || element.isConvex())
            continue;

        for (int j = 0; j < path.elementCount(); j++) {
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
        for (int i = 0; i < path.elementCount(); i++) {
            auto &element = path.elementAt(i);
            if (!element.isConvex())
                continue;

            for (int j = 0; j < path.elementCount(); j++) {
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
