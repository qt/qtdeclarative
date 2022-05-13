// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
