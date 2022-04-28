/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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

#include "qwidgetplatformmenuitem_p.h"
#include "qwidgetplatformmenu_p.h"

#include <QtGui/qaction.h>
#include <QtWidgets/qmenu.h>

QT_BEGIN_NAMESPACE

QWidgetPlatformMenuItem::QWidgetPlatformMenuItem(QObject *parent)
    : m_action(new QAction)
{
    setParent(parent);
    connect(m_action.data(), &QAction::hovered, this, &QPlatformMenuItem::hovered);
    connect(m_action.data(), &QAction::triggered, this, &QPlatformMenuItem::activated);
}

QWidgetPlatformMenuItem::~QWidgetPlatformMenuItem()
{
}

QAction *QWidgetPlatformMenuItem::action() const
{
    return m_action.data();
}

void QWidgetPlatformMenuItem::setText(const QString &text)
{
    m_action->setText(text);
}

void QWidgetPlatformMenuItem::setIcon(const QIcon &icon)
{
    m_action->setIcon(icon);
}

void QWidgetPlatformMenuItem::setMenu(QPlatformMenu *menu)
{
    QWidgetPlatformMenu *widgetMenu = qobject_cast<QWidgetPlatformMenu *>(menu);
    m_action->setMenu(widgetMenu ? widgetMenu->menu() : nullptr);
}

void QWidgetPlatformMenuItem::setVisible(bool visible)
{
    m_action->setVisible(visible);
}

void QWidgetPlatformMenuItem::setIsSeparator(bool separator)
{
    m_action->setSeparator(separator);
}

void QWidgetPlatformMenuItem::setFont(const QFont &font)
{
    m_action->setFont(font);
}

void QWidgetPlatformMenuItem::setRole(MenuRole role)
{
    m_action->setMenuRole(static_cast<QAction::MenuRole>(role));
}

void QWidgetPlatformMenuItem::setCheckable(bool checkable)
{
    m_action->setCheckable(checkable);
}

void QWidgetPlatformMenuItem::setChecked(bool checked)
{
    m_action->setChecked(checked);
}

#if QT_CONFIG(shortcut)
void QWidgetPlatformMenuItem::setShortcut(const QKeySequence &shortcut)
{
    m_action->setShortcut(shortcut);
}
#endif

void QWidgetPlatformMenuItem::setEnabled(bool enabled)
{
    m_action->setEnabled(enabled);
}

void QWidgetPlatformMenuItem::setIconSize(int size)
{
    Q_UNUSED(size);
}

QT_END_NAMESPACE

#include "moc_qwidgetplatformmenuitem_p.cpp"
