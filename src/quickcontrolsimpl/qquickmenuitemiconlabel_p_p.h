// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMENUITEMICONLABEL_P_P_H
#define QQUICKMENUITEMICONLABEL_P_P_H

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

#include <QtQuickControls2Impl/private/qquickiconlabel_p_p.h>
#include <QtQuickControls2Impl/private/qquickmenuitemiconlabel_p.h>
#if QT_CONFIG(shortcut)
#  include <QtGui/qkeysequence.h>
#endif

QT_BEGIN_NAMESPACE

class QQuickMenuItem;
class QQuickText;

class Q_AUTOTEST_EXPORT QQuickMenuItemIconLabelPrivate : public QQuickIconLabelPrivate
{
    Q_DECLARE_PUBLIC(QQuickMenuItemIconLabel)

public:
    ~QQuickMenuItemIconLabelPrivate() override;

    bool hasShortcut() const;
#if QT_CONFIG(shortcut)
    QKeySequence shortcut() const;
#endif
    bool createShortcutLabel();
    bool updateShortcutLabel();
    void syncShortcutLabel();
    bool destroyShortcutLabel();
    void updateOrSyncShortcutLabel();

    void updateImplicitSize() override;
    void layout() override;
    void itemDestroyed(QQuickItem *item) override;

    void textChange() override;
    void displayChange() override;

    QQuickMenuItem *menuItem = nullptr;
    QQuickText *shortcutLabel = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKMENUITEMICONLABEL_P_P_H
