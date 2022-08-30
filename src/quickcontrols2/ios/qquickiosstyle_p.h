// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKIOSSTYLE_P_H
#define QQUICKIOSSTYLE_P_H

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

#include <QtCore/qvariant.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2Impl/private/qquickattachedobject_p.h>

QT_BEGIN_NAMESPACE

class QQuickIOSStyle : public QQuickAttachedObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url CONSTANT)
    Q_PROPERTY(Theme theme READ theme NOTIFY themeChanged FINAL)
    QML_NAMED_ELEMENT(IOS)
    QML_ATTACHED(QQuickIOSStyle)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(2, 3)

public:
    enum Theme {
        Light,
        Dark
    };
    Q_ENUM(Theme)

    explicit QQuickIOSStyle(QObject *parent = nullptr);

    static QQuickIOSStyle *qmlAttachedProperties(QObject *object);

    Theme theme() const;

    QUrl url() const;

Q_SIGNALS:
    void themeChanged();

private:
    void init();

    Theme m_theme = Light;
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickIOSStyle, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKIOSSTYLE_P_H
