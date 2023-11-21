// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKNATIVEMENUITEM_P_H
#define QQUICKNATIVEMENUITEM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQuickAction;
class QQuickNativeIconLoader;
class QQuickMenu;
class QPlatformMenuItem;

class QQuickNativeMenuItem : public QObject
{
    Q_OBJECT

public:
    explicit QQuickNativeMenuItem(QQuickMenu *parentMenu, QQuickAction *action);
    explicit QQuickNativeMenuItem(QQuickMenu *parentMenu, QQuickMenu *subMenu);
    ~QQuickNativeMenuItem();

    QQuickAction *action() const;
    QQuickMenu *subMenu() const;
    void clearSubMenu();
    QPlatformMenuItem *handle() const;
    QPlatformMenuItem *create();
    void sync();

    QQuickNativeIconLoader *iconLoader() const;

    QString debugText() const;

private Q_SLOTS:
    void updateIcon();

private:
    void addShortcut();
    void removeShortcut();

    QQuickMenu *m_parentMenu = nullptr;
    QQuickMenu *m_subMenu = nullptr;
    QQuickAction *m_action = nullptr;
    mutable QQuickNativeIconLoader *m_iconLoader = nullptr;
    std::unique_ptr<QPlatformMenuItem> m_handle = nullptr;
    int m_shortcutId = -1;
};

QT_END_NAMESPACE

#endif // QQUICKNATIVEMENUITEM_P_H
