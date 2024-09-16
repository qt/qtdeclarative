// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQMLDEBUGTESTSERVICE_H
#define QQMLDEBUGTESTSERVICE_H

#include <private/qqmldebugservice_p.h>

class QQmlDebugTestService : public QQmlDebugService
{
    Q_OBJECT
public:
    QQmlDebugTestService(const QString &s, float version = 1, QObject *parent = nullptr);

protected:
    void messageReceived(const QByteArray &ba) override;
    void stateAboutToBeChanged(State state) override;
    void stateChanged(State state) override;
};

#endif // QQMLDEBUGTESTSERVICE_H
