// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLPROFILERCLIENT_H
#define QMLPROFILERCLIENT_H

#include <private/qqmlprofilerclient_p.h>
#include <private/qqmlprofilerclientdefinitions_p.h>
#include <private/qqmlprofilereventlocation_p.h>

class QmlProfilerData;
class QmlProfilerClientPrivate;
class QmlProfilerClient : public QQmlProfilerClient
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QmlProfilerClient)

public:
    QmlProfilerClient(QQmlDebugConnection *connection, QmlProfilerData *data);

Q_SIGNALS:
    void enabledChanged(bool enabled);
    void error(const QString &error);

private:
    void onStateChanged(State state);
};

#endif // QMLPROFILERCLIENT_H
