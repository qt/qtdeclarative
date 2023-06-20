// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSHAPECURVERENDERER_P_H
#define QQUICKSHAPECURVERENDERER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuickShapes/private/qquickshapesglobal_p.h>
#include <QtQuickShapes/private/qquickshape_p_p.h>
#include <qsgnode.h>
#include <qsggeometry.h>
#include <qsgmaterial.h>
#include <qsgrendererinterface.h>
#include <qsgtexture.h>
#include <QtCore/qrunnable.h>

#include <QtGui/private/qtriangulator_p.h>

QT_BEGIN_NAMESPACE

class QuadPath
{
public:
    void moveTo(const QVector2D &to)
    {
        subPathToStart = true;
        currentPoint = to;
    }

    void lineTo(const QVector2D &to)
    {
        addElement({}, to, true);
    }

    void quadTo(const QVector2D &control, const QVector2D &to)
    {
        addElement(control, to);
    }

    QRectF controlPointRect() const;

    Qt::FillRule fillRule() const { return m_fillRule; }
    void setFillRule(Qt::FillRule rule) { m_fillRule = rule; }

    void reserve(qsizetype size) { m_elements.reserve(size); }
    qsizetype elementCount() const { return m_elements.size(); }
    qsizetype elementCountRecursive() const;

    static QuadPath fromPainterPath(const QPainterPath &path);
    QPainterPath toPainterPath() const;
    QuadPath subPathsClosed() const;
    QuadPath flattened() const;

    class Element
    {
    public:
        Element ()
            : m_isSubpathStart(false), m_isSubpathEnd(false), m_isLine(false)
        {
        }

        bool isSubpathStart() const
        {
            return m_isSubpathStart;
        }

        bool isSubpathEnd() const
        {
            return m_isSubpathEnd;
        }

        bool isLine() const
        {
            return m_isLine;
        }

        bool isConvex() const
        {
            return m_curvatureFlags & Convex;
        }

        QVector2D startPoint() const
        {
            return sp;
        }

        QVector2D controlPoint() const
        {
            return cp;
        }

        QVector2D endPoint() const
        {
            return ep;
        }

        QVector2D midPoint() const
        {
            return isLine() ? 0.5f * (sp + ep) : (0.25f * sp) + (0.5f * cp) + (0.25 * ep);
        }

        QVector3D uvForPoint(QVector2D p) const;

        qsizetype childCount() const { return m_numChildren; }

        qsizetype indexOfChild(qsizetype childNumber) const
        {
            Q_ASSERT(childNumber >= 0 && childNumber < childCount());
            return -(m_firstChildIndex + 1 + childNumber);
        }

        QVector2D pointAtFraction(float t) const;

        QVector2D tangentAtFraction(float t) const
        {
            return isLine() ? (ep - sp) : ((1 - t) * 2 * (cp - sp)) + (t * 2 * (ep - cp));
        }

        QVector2D normalAtFraction(float t) const
        {
            const QVector2D tan = tangentAtFraction(t);
            return QVector2D(-tan.y(), tan.x());
        }

        float extent() const;

    private:
        int intersectionsAtY(float y, float *fractions) const;

        enum CurvatureFlags : quint8 {
            CurvatureUndetermined = 0,
            FillOnRight = 1,
            Convex = 2
        };

        QVector2D sp;
        QVector2D cp;
        QVector2D ep;
        int m_firstChildIndex = 0;
        quint8 m_numChildren = 0;
        CurvatureFlags m_curvatureFlags = CurvatureUndetermined;
        quint8 m_isSubpathStart : 1;
        quint8 m_isSubpathEnd : 1;
        quint8 m_isLine : 1;
        friend class QuadPath;
        friend QDebug operator<<(QDebug, const QuadPath::Element &);
    };

    template<typename Func>
    void iterateChildrenOf(Element &e, Func &&lambda)
    {
        const qsizetype lastChildIndex = e.m_firstChildIndex + e.childCount() - 1;
        for (qsizetype i = e.m_firstChildIndex; i <= lastChildIndex; i++) {
            Element &c = m_childElements[i];
            if (c.childCount() > 0)
                iterateChildrenOf(c, lambda);
            else
                lambda(c);
        }
    }

    template<typename Func>
    void iterateChildrenOf(const Element &e, Func &&lambda) const
    {
        const qsizetype lastChildIndex = e.m_firstChildIndex + e.childCount() - 1;
        for (qsizetype i = e.m_firstChildIndex; i <= lastChildIndex; i++) {
            const Element &c = m_childElements[i];
            if (c.childCount() > 0)
                iterateChildrenOf(c, lambda);
            else
                lambda(c);
        }
    }

    template<typename Func>
    void iterateElements(Func &&lambda)
    {
        for (auto &e : m_elements) {
            if (e.childCount() > 0)
                iterateChildrenOf(e, lambda);
            else
                lambda(e);
        }
    }

    template<typename Func>
    void iterateElements(Func &&lambda) const
    {
        for (auto &e : m_elements) {
            if (e.childCount() > 0)
                iterateChildrenOf(e, lambda);
            else
                lambda(e);
        }
    }

    void splitElementAt(qsizetype index);
    Element &elementAt(qsizetype i) { return i < 0 ? m_childElements[-(i + 1)] : m_elements[i]; }
    const Element &elementAt(qsizetype i) const
    {
        return i < 0 ? m_childElements[-(i + 1)] : m_elements[i];
    }

    qsizetype indexOfChildAt(qsizetype i, qsizetype childNumber) const
    {
        return elementAt(i).indexOfChild(childNumber);
    }

    void addCurvatureData();
    bool contains(const QVector2D &v) const;

private:
    void addElement(const QVector2D &control, const QVector2D &to, bool isLine = false);
    Element::CurvatureFlags coordinateOrderOfElement(const Element &element) const;
    static bool isControlPointOnLeft(const Element &element);
    static QVector2D closestPointOnLine(const QVector2D &start,
                                        const QVector2D &end,
                                        const QVector2D &p);
    static bool isPointOnLeft(const QVector2D &p, const QVector2D &sp, const QVector2D &ep);
    static bool isPointOnLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep);
    static bool isPointNearLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep);

    friend QDebug operator<<(QDebug, const QuadPath &);

    bool subPathToStart = true;
    Qt::FillRule m_fillRule = Qt::OddEvenFill;
    QVector2D currentPoint;
    QList<Element> m_elements;
    QList<Element> m_childElements;
};

QDebug operator<<(QDebug, const QuadPath::Element &);
QDebug operator<<(QDebug, const QuadPath &);

class QQuickShapeCurveNode : public QSGNode
{
public:
    QQuickShapeCurveNode();
};

class QQuickShapeCurveRenderer : public QQuickAbstractPathRenderer
{
public:
    QQuickShapeCurveRenderer(QQuickItem *)
        : m_rootNode(nullptr)
    { }
    ~QQuickShapeCurveRenderer() override;

    void beginSync(int totalCount, bool *countChanged) override;
    void setPath(int index, const QQuickPath *path) override;
    void setStrokeColor(int index, const QColor &color) override;
    void setStrokeWidth(int index, qreal w) override;
    void setFillColor(int index, const QColor &color) override;
    void setFillRule(int index, QQuickShapePath::FillRule fillRule) override;
    void setJoinStyle(int index, QQuickShapePath::JoinStyle joinStyle, int miterLimit) override;
    void setCapStyle(int index, QQuickShapePath::CapStyle capStyle) override;
    void setStrokeStyle(int index, QQuickShapePath::StrokeStyle strokeStyle,
                        qreal dashOffset, const QVector<qreal> &dashPattern) override;
    void setFillGradient(int index, QQuickShapeGradient *gradient) override;
    void endSync(bool async) override;
    void setAsyncCallback(void (*)(void *), void *) override;
    Flags flags() const override { return Flags{}; }

    void updateNode() override;

    void setRootNode(QQuickShapeCurveNode *node);

    using NodeList = QVector<QSGGeometryNode *>;

    enum DirtyFlag
    {
        GeometryDirty = 1,
        UniformsDirty = 2
    };

    enum DebugVisualizationOption {
        NoDebug = 0,
        DebugCurves = 0x01,
        DebugWireframe = 0x02
    };

    Q_QUICKSHAPES_PRIVATE_EXPORT static int debugVisualization();
    Q_QUICKSHAPES_PRIVATE_EXPORT static void setDebugVisualization(int options);

private:
    struct PathData {

        bool isFillVisible() const { return fillColor.alpha() > 0 || gradientType != NoGradient; }

        bool isStrokeVisible() const
        {
            return validPenWidth && pen.color().alpha() > 0 && pen.style() != Qt::NoPen;
        }

        bool useFragmentShaderStroker() const;

        FillGradientType gradientType = NoGradient;
        GradientDesc gradient;
        QPainterPath fillPath;
        QPainterPath originalPath;
        QuadPath path;
        QuadPath qPath; // TODO: better name
        QColor fillColor;
        Qt::FillRule fillRule = Qt::OddEvenFill;
        QPen pen;
        int m_dirty = 0;
        bool validPenWidth = true;
        bool convexConcaveResolved = false;

        NodeList fillNodes;
        NodeList strokeNodes;
        NodeList debugNodes;
    };

    void deleteAndClear(NodeList *nodeList);

    QVector<QSGGeometryNode *> addPathNodesBasic(const PathData &pathData, NodeList *debugNodes);
    QVector<QSGGeometryNode *> addPathNodesLineShader(const PathData &pathData, NodeList *debugNodes);
    QVector<QSGGeometryNode *> addStrokeNodes(const PathData &pathData, NodeList *debugNodes);
    QVector<QSGGeometryNode *> addNodesStrokeShader(const PathData &pathData, NodeList *debugNodes);

    QSGGeometryNode *addLoopBlinnNodes(const QTriangleSet &triangles,
                                       const QVarLengthArray<quint32> &extraIndices,
                                       int startConcaveCurves,
                                       const PathData &pathData,
                                       NodeList *debugNodes);

    void solveOverlaps(QuadPath &path);

    QQuickShapeCurveNode *m_rootNode;
    QVector<PathData> m_paths;
    static int debugVisualizationFlags;
};

QT_END_NAMESPACE

#endif // QQUICKSHAPECURVERENDERER_P_H
