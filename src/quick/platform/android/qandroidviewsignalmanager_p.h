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
#include <QtCore/qmap.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QAndroidViewSignalManager : public QObject
{
    Q_OBJECT
public:
    struct ConnectionInfo
    {
        int id;
        QJniObject listener;
        QByteArray javaArgType;
        QByteArray signalSignature;
        int propertyIndex{ -1 };
    };

    explicit QAndroidViewSignalManager()
        : QObject(), connectionHandleCounter(0)
    {
    }
    void invokeListener(QObject *sender, int senderSignalIndex, QVariant signalValue);

    int connectionHandleCounter;
    QMultiMap<QByteArray, ConnectionInfo> connectionInfoMap;
    QHash<int, QMetaObject::Connection> connections;

public slots:
    void forwardSignal();
    void forwardSignal(int);
    void forwardSignal(double);
    void forwardSignal(float);
    void forwardSignal(bool);
    void forwardSignal(QString);
};

QT_END_NAMESPACE

#endif // QANDROIDVIEWSIGNALMANAGER_P_H
