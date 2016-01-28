/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickitemdelegate_p.h"
#include "qquickcontrol_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ItemDelegate
    \inherits AbstractButton
    \instantiates QQuickItemDelegate
    \inqmlmodule Qt.labs.controls
    \brief An item delegate.

    ItemDelegate presents a standard view item. It can be used as a delegate
    in various views and controls, such as \l ListView and \l ComboBox.

    ItemDelegate inherits its API from AbstractButton. For instance, you can set
    \l {AbstractButton::text}{text}, make items \l {AbstractButton::checkable}{checkable},
    and react to \l {AbstractButton::clicked}{clicks} using the AbstractButton API.

    \snippet qtlabscontrols-itemdelegate.qml 1

    \labs

    \sa {Customizing ItemDelegate}
*/

QQuickItemDelegate::QQuickItemDelegate(QQuickItem *parent) : QQuickAbstractButton(parent)
{
}

QFont QQuickItemDelegate::defaultFont() const
{
    return QQuickControlPrivate::themeFont(QPlatformTheme::ItemViewFont);
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickItemDelegate::accessibleRole() const
{
    return QAccessible::ListItem;
}
#endif

QT_END_NAMESPACE
