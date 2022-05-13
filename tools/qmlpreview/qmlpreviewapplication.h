// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLPREVIEWAPPLICATION_H
#define QMLPREVIEWAPPLICATION_H

#include "qmlpreviewfilesystemwatcher.h"

#include <private/qqmlpreviewclient_p.h>
#include <private/qqmldebugconnection_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qprocess.h>
#include <QtCore/qtimer.h>

#include <QtNetwork/qabstractsocket.h>

class QmlPreviewApplication : public QCoreApplication
{
    Q_OBJECT
public:
    QmlPreviewApplication(int &argc, char **argv);
    ~QmlPreviewApplication();

    void parseArguments();
    int exec();

private:
    void run();
    void tryToConnect();
    void processHasOutput();
    void processFinished();

    void logError(const QString &error);
    void logStatus(const QString &status);

    void serveRequest(const QString &request);
    bool sendFile(const QString &path);
    void sendDirectory(const QString &path);

    QString m_executablePath;
    QStringList m_arguments;
    QScopedPointer<QProcess> m_process;
    bool m_verbose;

    QString m_socketFile;

    QScopedPointer<QQmlDebugConnection> m_connection;
    QScopedPointer<QQmlPreviewClient> m_qmlPreviewClient;
    QmlPreviewFileSystemWatcher m_watcher;

    QTimer m_loadTimer;
    QTimer m_connectTimer;
    uint m_connectionAttempts;
};

#endif // QMLPREVIEWAPPLICATION_H
