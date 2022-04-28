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

#include "qwidgetplatformsystemtrayicon_p.h"
#include "qwidgetplatformmenu_p.h"

#include <QtWidgets/qsystemtrayicon.h>

QT_BEGIN_NAMESPACE

QWidgetPlatformSystemTrayIcon::QWidgetPlatformSystemTrayIcon(QObject *parent)
    : m_systray(new QSystemTrayIcon)
{
    setParent(parent);

    connect(m_systray.data(), &QSystemTrayIcon::messageClicked, this, &QPlatformSystemTrayIcon::messageClicked);
    connect(m_systray.data(), &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        emit activated(static_cast<ActivationReason>(reason));
    });
}

QWidgetPlatformSystemTrayIcon::~QWidgetPlatformSystemTrayIcon()
{
}

void QWidgetPlatformSystemTrayIcon::init()
{
    m_systray->show();
}

void QWidgetPlatformSystemTrayIcon::cleanup()
{
    m_systray->hide();
}

void QWidgetPlatformSystemTrayIcon::updateIcon(const QIcon &icon)
{
    m_systray->setIcon(icon);
}

void QWidgetPlatformSystemTrayIcon::updateToolTip(const QString &tooltip)
{
    m_systray->setToolTip(tooltip);
}

void QWidgetPlatformSystemTrayIcon::updateMenu(QPlatformMenu *menu)
{
#if QT_CONFIG(menu)
    QWidgetPlatformMenu *widgetMenu = qobject_cast<QWidgetPlatformMenu *>(menu);
    if (!widgetMenu)
        return;

    m_systray->setContextMenu(widgetMenu->menu());
#else
    Q_UNUSED(menu);
#endif
}

QRect QWidgetPlatformSystemTrayIcon::geometry() const
{
    return m_systray->geometry();
}

void QWidgetPlatformSystemTrayIcon::showMessage(const QString &title, const QString &msg, const QIcon &icon, MessageIcon iconType, int msecs)
{
    Q_UNUSED(icon);
    m_systray->showMessage(title, msg, static_cast<QSystemTrayIcon::MessageIcon>(iconType), msecs);
}

bool QWidgetPlatformSystemTrayIcon::isSystemTrayAvailable() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

bool QWidgetPlatformSystemTrayIcon::supportsMessages() const
{
    return QSystemTrayIcon::supportsMessages();
}

QPlatformMenu *QWidgetPlatformSystemTrayIcon::createMenu() const
{
    return new QWidgetPlatformMenu;
}

QT_END_NAMESPACE

#include "moc_qwidgetplatformsystemtrayicon_p.cpp"
