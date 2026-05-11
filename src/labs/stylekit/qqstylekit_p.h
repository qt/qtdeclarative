// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKIT_P_H
#define QQSTYLEKIT_P_H

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

#include "qqstylekitdebug_p.h"

#include <QtQml/QtQml>

QT_BEGIN_NAMESPACE

class QQStyleKitAttached;

class QQStyleKit : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(StyleKit)
    QML_ATTACHED(QQStyleKitAttached)

public:
    QQStyleKit(QObject *parent = nullptr);
    static QQStyleKitAttached *qmlAttachedProperties(QObject *obj = nullptr);

private:
    Q_DISABLE_COPY(QQStyleKit)
};

class QQStyleKitAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQStyleKitStyle *style READ style WRITE setStyle NOTIFY styleChanged FINAL)
    Q_PROPERTY(QUrl styleUrl READ styleUrl WRITE setStyleUrl NOTIFY styleUrlChanged FINAL)
    Q_PROPERTY(bool transitionsEnabled READ transitionsEnabled WRITE setTransitionsEnabled NOTIFY transitionsEnabledChanged FINAL)
    Q_PROPERTY(QQStyleKitDebug *debug READ debug FINAL)

public:
    QQStyleKitAttached(QObject *parent);
    ~QQStyleKitAttached();

    QQStyleKitStyle *style() const;
    void setStyle(QQStyleKitStyle *style);

    QUrl styleUrl() const;
    void setStyleUrl(const QUrl &styleUrl);

    bool transitionsEnabled() const;
    void setTransitionsEnabled(bool enabled);

    QQStyleKitDebug *debug() const;

    Q_INVOKABLE bool styleLoaded() const;

signals:
    void styleChanged();
    void styleUrlChanged();
    void transitionsEnabledChanged();

private:
    bool m_ownsStyle = false;
    QUrl m_styleUrl;
    QPointer<QQmlEngine> m_engine;
    QPointer<QQStyleKitStyle> m_style;
    QQStyleKitDebug m_debug;

    static QPointer<QQStyleKitAttached> s_instance;
    static bool s_transitionsEnabled;

    friend class QQStyleKit;
};

QT_END_NAMESPACE

#endif // QQSTYLEKIT_P_H
