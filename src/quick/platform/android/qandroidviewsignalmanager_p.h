// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDVIEWSIGNALMANAGER_P_H
#define QANDROIDVIEWSIGNALMANAGER_P_H

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

#include <QtCore/qhash.h>
#include <QtCore/qjnitypes.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qmap.h>
#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>
#include <QtQuick/qquickview.h>

QT_BEGIN_NAMESPACE

class QAndroidViewSignalManager : public QObject
{
    using connection_key_t = int;

public:
    explicit QAndroidViewSignalManager(QQuickView *view, QObject *parent = nullptr);

    int qt_metacall(QMetaObject::Call call, int methodId, void **args) override;

    void removeConnection(connection_key_t signalIdx);
    bool addConnection(const QString &signalName,
                      const QJniArray<jclass> &argTypes,
                      const QJniObject &listener,
                      int id);

private:
    /*
        This will store the necessary information to call the listener
        when the signal is emitted, including the Java function name and
        signature so that we can call it quickly without recalculating those.
    */
    struct ConnectionInfo
    {
        QMetaObject::Connection connection;
        QJniObject listenerObject;
        QString qmlSignalName;

        QList<QMetaType::Type> qmlArgumentTypes;
        bool isPropertySignal;
        std::optional<int> qmlPropertyIndex; // Only filled if isPropertySignal
        int connectionId;
    };

    struct QueuedConnectionInfo
    {
        connection_key_t id;
        QString signalName;
        QJniArray<jclass> argTypes;
        QJniObject listener;
    };

    bool hasConnection(connection_key_t key) const;
    connection_key_t createNewSignalKey() const;
    void onViewStatusChanged(QQuickView::Status status);
    bool queueConnection(const QString &signalName,
                         const QJniArray<jclass> &argTypes,
                         const QJniObject &listener,
                         int id);

    QMap<connection_key_t, ConnectionInfo> m_connections;
    QQuickView *m_view;
    QList<QueuedConnectionInfo> m_queuedConnections;
    QMutex m_queueMutex;
};

QT_END_NAMESPACE

#endif // QANDROIDVIEWSIGNALMANAGER_P_H
