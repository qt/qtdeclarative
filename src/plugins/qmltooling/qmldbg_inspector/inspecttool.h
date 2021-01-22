/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef INSPECTTOOL_H
#define INSPECTTOOL_H

#include <QtCore/QPointF>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QQuickItem;
class QMouseEvent;
class QKeyEvent;
class QTouchEvent;

namespace QmlJSDebugger {

class GlobalInspector;
class QQuickWindowInspector;
class HoverHighlight;

class InspectTool : public QObject
{
    Q_OBJECT
public:
    InspectTool(QQuickWindowInspector *inspector, QQuickWindow *view);

    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void hoverMoveEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *) {}
    void keyReleaseEvent(QKeyEvent *);
    void touchEvent(QTouchEvent *event);

private:
    void selectNextItem();
    void selectItem();
    void showItemName();

    QQuickWindowInspector *inspector() const;
    GlobalInspector *globalInspector() const;

    bool m_tapEvent;
    QPointer<QQuickItem> m_contentItem;
    QPointF m_mousePosition;
    ulong m_touchTimestamp;
    QTimer m_nameDisplayTimer;

    HoverHighlight *m_hoverHighlight;
    QQuickItem *m_lastItem;
    QQuickItem *m_lastClickedItem;
};

} // namespace QmlJSDebugger

QT_END_NAMESPACE

#endif // INSPECTTOOL_H
