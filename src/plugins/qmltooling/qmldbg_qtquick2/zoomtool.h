/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ZOOMTOOL_H
#define ZOOMTOOL_H

#include "abstracttool.h"

#include <QtCore/QPointF>

QT_FORWARD_DECLARE_CLASS(QQuickView)
QT_FORWARD_DECLARE_CLASS(QQuickItem)

namespace QmlJSDebugger {
namespace QtQuick2 {

class QQuickViewInspector;

class ZoomTool : public AbstractTool
{
    Q_OBJECT

public:
    enum ZoomDirection {
        ZoomIn,
        ZoomOut
    };

    explicit ZoomTool(QQuickViewInspector *inspector, QQuickView *view);
    virtual ~ZoomTool();
    void leaveEvent(QEvent *) {}

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *) {}
    void mouseDoubleClickEvent(QMouseEvent *event);

    void hoverMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

    void keyPressEvent(QKeyEvent *) {}
    void keyReleaseEvent(QKeyEvent *event);

    void touchEvent(QTouchEvent *event);

private:
    qreal nextZoomScale(ZoomDirection direction);
    void scaleView(const qreal &factor, const QPointF &newcenter, const QPointF &oldcenter);
    void zoomTo100();
    void zoomIn();
    void zoomOut();

private:
    bool m_originalSmooth;
    bool m_dragStarted;
    bool m_pinchStarted;
    QQuickItem *m_rootItem;
    QPointF m_adjustedOrigin;
    QPointF m_dragStartPosition;
    QPointF m_mousePosition;
    QPointF m_originalPosition;
    qreal m_currentScale;
    qreal m_smoothScaleFactor;
    qreal m_minScale;
    qreal m_maxScale;
    qreal m_tapScaleCounter;
    qreal m_originalScale;
};

} // namespace QtQuick2
} // namespace QmlJSDebugger

#endif // ZOOMTOOL_H
