/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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
******************************************************************************/

#include "qaccessiblequickwidget_p.h"

#include "qquickwidget_p.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

QAccessibleQuickWidget::QAccessibleQuickWidget(QQuickWidget* widget)
: QAccessibleWidget(widget)
, m_accessibleWindow(QQuickWidgetPrivate::get(widget)->offscreenWindow)
{
    // NOTE: m_accessibleWindow is a QAccessibleQuickWindow, and not a
    // QAccessibleQuickWidgetOffscreenWindow (defined below). This means
    // it will return the Quick item child interfaces, which is what's needed here
    // (unlike QAccessibleQuickWidgetOffscreenWindow, which will report 0 children).
}

QAccessibleInterface *QAccessibleQuickWidget::child(int index) const
{
    return m_accessibleWindow.child(index);
}

int QAccessibleQuickWidget::childCount() const
{
    return m_accessibleWindow.childCount();
}

int QAccessibleQuickWidget::indexOfChild(const QAccessibleInterface *iface) const
{
    return m_accessibleWindow.indexOfChild(iface);
}

QAccessibleInterface *QAccessibleQuickWidget::childAt(int x, int y) const
{
    return m_accessibleWindow.childAt(x, y);
}

QAccessibleQuickWidgetOffscreenWindow::QAccessibleQuickWidgetOffscreenWindow(QQuickWindow *window)
:QAccessibleQuickWindow(window)
{

}

QAccessibleInterface *QAccessibleQuickWidgetOffscreenWindow::child(int index) const
{
    Q_UNUSED(index);
    return nullptr;
}

int QAccessibleQuickWidgetOffscreenWindow::childCount() const
{
    return 0;
}

int QAccessibleQuickWidgetOffscreenWindow::indexOfChild(const QAccessibleInterface *iface) const
{
    Q_UNUSED(iface);
    return -1;
}

QAccessibleInterface *QAccessibleQuickWidgetOffscreenWindow::QAccessibleQuickWidgetOffscreenWindow::childAt(int x, int y) const
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    return nullptr;
}

#endif // accessibility

QT_END_NAMESPACE
