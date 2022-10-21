/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
