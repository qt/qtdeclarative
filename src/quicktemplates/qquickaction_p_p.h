// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKACTION_P_P_H
#define QQUICKACTION_P_P_H

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

#include <QtCore/private/qobject_p.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstring.h>
#if QT_CONFIG(shortcut)
#  include <QtGui/qkeysequence.h>
#endif
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

class QShortcutEvent;
class QQuickActionGroup;

class QQuickActionPrivate : public QObjectPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickAction)

public:
    static QQuickActionPrivate *get(QQuickAction *action)
    {
        return action->d_func();
    }

#if QT_CONFIG(shortcut)
    QVariant shortcut() const;
    void setShortcut(const QVariant &shortcut);
#endif

    void setEnabled(bool enable);

    bool watchItem(QQuickItem *item);
    bool unwatchItem(QQuickItem *item);

    void registerItem(QQuickItem *item);
    void unregisterItem(QQuickItem *item);

    void itemVisibilityChanged(QQuickItem *item) override;
    void itemDestroyed(QQuickItem *item) override;

    bool handleShortcutEvent(QObject *object, QShortcutEvent *event);

    void trigger(QObject*, bool doToggle);

#if QT_CONFIG(shortcut)
    class ShortcutEntry
    {
    public:
        explicit ShortcutEntry(QObject *target);
        ~ShortcutEntry();

        QObject *target() const;
        int shortcutId() const;

        void grab(const QKeySequence &vshortcut, bool enabled);
        void ungrab();

        void setEnabled(bool enabled);

    private:
        int m_shortcutId = 0;
        QObject *m_target = nullptr;
    };

    ShortcutEntry *findShortcutEntry(QObject *target) const;
    void updateDefaultShortcutEntry();
#endif // QT_CONFIG(shortcut)

    bool explicitEnabled = false;
    bool enabled = true;
    bool checked = false;
    bool checkable = false;
    QString text;
    QQuickIcon icon;
#if QT_CONFIG(shortcut)
    QKeySequence keySequence;
    QVariant vshortcut;
    ShortcutEntry *defaultShortcutEntry = nullptr;
    QList<ShortcutEntry *> shortcutEntries;
#endif
    QQuickActionGroup *group = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKACTION_P_P_H
