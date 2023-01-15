// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#if QT_CONFIG(menu)
    return new QWidgetPlatformMenu;
#else
    return nullptr;
#endif
}

QT_END_NAMESPACE

#include "moc_qwidgetplatformsystemtrayicon_p.cpp"
