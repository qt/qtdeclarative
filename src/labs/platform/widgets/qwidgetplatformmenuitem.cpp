/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Labs Platform module of the Qt Toolkit.
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
******************************************************************************/

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
