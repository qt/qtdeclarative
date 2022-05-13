// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
class QEnterEvent;

namespace QmlJSDebugger {

class GlobalInspector;
class QQuickWindowInspector;
class HoverHighlight;

class InspectTool : public QObject
{
    Q_OBJECT
public:
    InspectTool(QQuickWindowInspector *inspector, QQuickWindow *view);

    void enterEvent(QEnterEvent *);
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
