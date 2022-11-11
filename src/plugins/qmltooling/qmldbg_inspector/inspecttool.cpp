// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "inspecttool.h"
#include "highlight.h"
#include "qquickwindowinspector.h"
#include "globalinspector.h"

#include <QtCore/QLineF>

#include <QtGui/QMouseEvent>
#include <QtGui/QTouchEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QGuiApplication>
#include <QtGui/QStyleHints>

#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>

QT_BEGIN_NAMESPACE

namespace QmlJSDebugger {

InspectTool::InspectTool(QQuickWindowInspector *inspector, QQuickWindow *view) :
    QObject(inspector),
    m_contentItem(view->contentItem()),
    m_touchTimestamp(0),
    m_hoverHighlight(new HoverHighlight(inspector->overlay())),
    m_lastItem(nullptr),
    m_lastClickedItem(nullptr)
{
    //Timer to display selected item's name
    m_nameDisplayTimer.setSingleShot(true);
    m_nameDisplayTimer.setInterval(QGuiApplication::styleHints()->mouseDoubleClickInterval());
    connect(&m_nameDisplayTimer, &QTimer::timeout, this, &InspectTool::showItemName);
}

void InspectTool::enterEvent(QEnterEvent *)
{
    m_hoverHighlight->setVisible(true);
}

void InspectTool::leaveEvent(QEvent *)
{
    m_hoverHighlight->setVisible(false);
}

void InspectTool::mousePressEvent(QMouseEvent *event)
{
    m_mousePosition = event->position();
    if (event->button() == Qt::LeftButton) {
        selectItem();
        m_hoverHighlight->setVisible(false);
    }
}

void InspectTool::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_mousePosition = event->position();
    if (event->button() == Qt::LeftButton) {
        selectNextItem();
        m_hoverHighlight->setVisible(false);
    }
}

void InspectTool::mouseMoveEvent(QMouseEvent *event)
{
    hoverMoveEvent(event);
}

void InspectTool::hoverMoveEvent(QMouseEvent *event)
{
    m_mousePosition = event->position();
    QQuickItem *item = inspector()->topVisibleItemAt(event->position().toPoint());
    if (!item || item == m_lastClickedItem) {
        m_hoverHighlight->setVisible(false);
    } else {
        m_hoverHighlight->setItem(item);
        m_hoverHighlight->setVisible(true);
    }
}

void InspectTool::touchEvent(QTouchEvent *event)
{
    const auto &touchPoints = event->points();

    switch (event->type()) {
    case QEvent::TouchBegin:
        if (touchPoints.size() == 1 && (event->touchPointStates() & QEventPoint::State::Pressed)) {
            m_mousePosition = touchPoints.first().position();
            m_tapEvent = true;
        } else {
            m_tapEvent = false;
        }
        break;
    case QEvent::TouchUpdate: {
        if (touchPoints.size() > 1)
            m_tapEvent = false;
        else if ((touchPoints.size() == 1) && (event->touchPointStates() & QEventPoint::State::Updated))
            m_mousePosition = touchPoints.first().position();
        break;
    }
    case QEvent::TouchEnd: {
        if (touchPoints.size() == 1 && m_tapEvent) {
            m_tapEvent = false;
            bool doubleTap = event->timestamp() - m_touchTimestamp
                    < static_cast<ulong>(QGuiApplication::styleHints()->mouseDoubleClickInterval());
            if (doubleTap) {
                m_nameDisplayTimer.stop();
                selectNextItem();
            } else {
                selectItem();
            }
            m_touchTimestamp = event->timestamp();
        }
        break;
    }
    default:
        break;
    }
}

void InspectTool::selectNextItem()
{
    if (m_lastClickedItem != inspector()->topVisibleItemAt(m_mousePosition))
        return;
    QList<QQuickItem*> items = inspector()->itemsAt(m_mousePosition);
    for (int i = 0; i < items.size(); i++) {
        if (m_lastItem == items[i]) {
            if (i + 1 < items.size())
                m_lastItem = items[i+1];
            else
                m_lastItem = items[0];
            globalInspector()->setSelectedItems(QList<QQuickItem*>() << m_lastItem);
            showItemName();
            break;
        }
    }
}

void InspectTool::selectItem()
{
    if (!inspector()->topVisibleItemAt(m_mousePosition))
        return;
    m_lastClickedItem = inspector()->topVisibleItemAt(m_mousePosition);
    m_lastItem = m_lastClickedItem;
    globalInspector()->setSelectedItems(QList<QQuickItem*>() << m_lastClickedItem);
    if (m_lastClickedItem == inspector()->topVisibleItemAt(m_mousePosition)) {
        m_nameDisplayTimer.start();
    } else {
        showItemName();
    }
}

void InspectTool::showItemName()
{
    globalInspector()->showSelectedItemName(m_lastItem, m_mousePosition);
}

QQuickWindowInspector *InspectTool::inspector() const
{
    return static_cast<QQuickWindowInspector *>(parent());
}

GlobalInspector *InspectTool::globalInspector() const
{
    return static_cast<GlobalInspector *>(parent()->parent());
}

} // namespace QmlJSDebugger

QT_END_NAMESPACE

#include "moc_inspecttool.cpp"
