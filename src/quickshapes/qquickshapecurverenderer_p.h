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
#include <QtQuick/private/qquadpath_p.h>
#include <QtQuick/private/qsgcurveabstractnode_p.h>
#include <QtQuick/private/qsggradientcache_p.h>
#include <qsgnode.h>
#include <qsggeometry.h>
#include <qsgmaterial.h>
#include <qsgrendererinterface.h>
#include <qsgtexture.h>
#include <QtCore/qrunnable.h>
#include <QRunnable>

#include <QtGui/private/qtriangulator_p.h>
#include <QtQuick/private/qsgcurvefillnode_p.h>

QT_BEGIN_NAMESPACE

class QQuickShapeCurveRunnable;

class Q_QUICKSHAPES_EXPORT QQuickShapeCurveRenderer : public QQuickAbstractPathRenderer
{
public:
    QQuickShapeCurveRenderer(QQuickItem *item)
        : m_item(item)
    { }
    ~QQuickShapeCurveRenderer() override;

    void beginSync(int totalCount, bool *countChanged) override;
    void setPath(int index, const QQuickPath *path) override;
    void setPath(int index, const QPainterPath &path, QQuickShapePath::PathHints pathHints = {});
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
    Flags flags() const override { return SupportsAsync; }

    void updateNode() override;

    void setRootNode(QSGNode *node);
    void clearNodeReferences();

    using NodeList = QVector<QSGCurveAbstractNode *>;

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

    static int debugVisualization();
    static void setDebugVisualization(int options);

private:
    struct PathData {

        bool isFillVisible() const { return fillColor.alpha() > 0 || gradientType != QGradient::NoGradient; }

        bool isStrokeVisible() const
        {
            return validPenWidth && pen.color().alpha() > 0 && pen.style() != Qt::NoPen;
        }

        QGradient::Type gradientType = QGradient::NoGradient;
        QSGGradientCache::GradientDesc gradient;
        QColor fillColor;
        Qt::FillRule fillRule = Qt::OddEvenFill;
        QPen pen;
        bool validPenWidth = true;
        int m_dirty = 0;
        QQuickShapePath::PathHints pathHints;

        QPainterPath originalPath;
        QQuadPath path;
        QQuadPath fillPath;
        QQuadPath strokePath;

        NodeList fillNodes;
        NodeList strokeNodes;

        QQuickShapeCurveRunnable *currentRunner = nullptr;
    };

    void createRunner(PathData *pathData);
    void maybeUpdateAsyncItem();

    static void processPath(PathData *pathData);
    static NodeList addFillNodes(const PathData &pathData);
    static NodeList addTriangulatingStrokerNodes(const PathData &pathData);
    static NodeList addCurveStrokeNodes(const PathData &pathData);

    void solveIntersections(QQuadPath &path);
    QQuickItem *m_item;
    QSGNode *m_rootNode = nullptr;
    QVector<PathData> m_paths;
    void (*m_asyncCallback)(void *) = nullptr;
    void *m_asyncCallbackData = nullptr;
    static int debugVisualizationFlags;

    friend class QQuickShapeCurveRunnable;
};

class QQuickShapeCurveRunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:
    void run() override;

    bool isAsync = false;
    bool isDone = false;
    bool orphaned = false;

    // input / output
    QQuickShapeCurveRenderer::PathData pathData;

Q_SIGNALS:
    void done(QQuickShapeCurveRunnable *self);
};

QT_END_NAMESPACE

#endif // QQUICKSHAPECURVERENDERER_P_H
