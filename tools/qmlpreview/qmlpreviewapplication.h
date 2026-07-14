// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLPREVIEWAPPLICATION_H
#define QMLPREVIEWAPPLICATION_H

#include "qmlpreviewfilesystemwatcher.h"

#include <private/qqmljsresourcefilemapper_p.h>
#include <private/qqmlpreviewclient_p.h>
#include <private/qqmldebugconnection_p.h>
#include <private/qqmldebugconsole_p.h>

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
    bool isInteractive() const;
    void startConsole();

private:
    void run();
    void tryToConnect();
    void processHasOutput();
    void processFinished();
    void serveResourceRequest(const QString &path);
    void serveFileRequest(const QString &remotePath, const QString &localPath);

    void logError(const QString &error);
    void logStatus(const QString &status);
    bool argumentsFromCommandLineAndFile(QStringList &allArguments, const QStringList &arguments);

    void serveRequest(const QString &request);
    bool sendFile(const QString &path);
    void sendDirectory(const QString &path);

    void connectQmlPreviewClientSignals();
    void connectConnectionSignals();
    void connectWatcherSignals();

    void killProcess();
    void restartProcess();
    void startProcess();

    void registerCommands();
    void prompt(const QString &line = QString(), bool ready = true);
    void saveEventsToOutput();
    void replayEvents();

    QString m_executablePath;
    QStringList m_arguments;
    QScopedPointer<QProcess> m_process;
    bool m_verbose = false;
    bool m_interactive = false;
    bool m_promptShown = false;

    QString m_outputFile;
    QString m_replayFile;

    QString m_socketFile;

    QScopedPointer<QQmlJSResourceFileMapper> m_resourceFileMapper;
    QHash<QString, QString> m_localToResourcePath;
    QScopedPointer<QQmlDebugConnection> m_connection;
    QScopedPointer<QQmlPreviewClient> m_qmlPreviewClient;
    QmlPreviewFileSystemWatcher m_watcher;
    QQmlDebugConsole m_console;

    QTimer m_loadTimer;
    QTimer m_connectTimer;
    uint m_connectionAttempts = 0;

    QQmlPreviewClient::Settings m_confirmedSettings;
};

#endif // QMLPREVIEWAPPLICATION_H
