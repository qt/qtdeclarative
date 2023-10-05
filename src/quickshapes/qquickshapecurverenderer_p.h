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
#include <QtQuickShapes/private/qquadpath_p.h>
#include <QtQuickShapes/private/qquickshapeabstractcurvenode_p.h>
#include <qsgnode.h>
#include <qsggeometry.h>
#include <qsgmaterial.h>
#include <qsgrendererinterface.h>
#include <qsgtexture.h>
#include <QtCore/qrunnable.h>

#include <QtGui/private/qtriangulator_p.h>

QT_BEGIN_NAMESPACE

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

    void setRootNode(QSGNode *node);

    using NodeList = QVector<QQuickShapeAbstractCurveNode *>;

    enum DirtyFlag
    {
        PathDirty = 0x01,
        FillDirty = 0x02,
        StrokeDirty = 0x04,
        UniformsDirty = 0x08
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

        FillGradientType gradientType = NoGradient;
        GradientDesc gradient;
        QPainterPath originalPath;
        QQuadPath path;
        QQuadPath fillPath;
        QQuadPath strokePath;
        QColor fillColor;
        Qt::FillRule fillRule = Qt::OddEvenFill;
        QPen pen;
        int m_dirty = 0;
        bool validPenWidth = true;
        bool convexConcaveResolved = false;

        NodeList fillNodes;
        NodeList fillDebugNodes;
        NodeList strokeNodes;
        NodeList strokeDebugNodes;
    };

    void deleteAndClear(NodeList *nodeList);

    NodeList addFillNodes(const PathData &pathData, NodeList *debugNodes);
    NodeList addTriangulatingStrokerNodes(const PathData &pathData, NodeList *debugNodes);
    NodeList addCurveStrokeNodes(const PathData &pathData, NodeList *debugNodes);

    void solveOverlaps(QQuadPath &path);

    QSGNode *m_rootNode;
    QVector<PathData> m_paths;
    static int debugVisualizationFlags;
};

QT_END_NAMESPACE

#endif // QQUICKSHAPECURVERENDERER_P_H
