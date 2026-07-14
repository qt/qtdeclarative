// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLPROFILERAPPLICATION_H
#define QMLPROFILERAPPLICATION_H

#include "qmlprofilerclient.h"

#include <private/qqmldebugconnection_p.h>
#include <private/qqmldebugconsole_p.h>
#include <private/qqmlprofilereventreceiver_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qprocess.h>
#include <QtCore/qtimer.h>
#include <QtNetwork/qabstractsocket.h>

enum PendingRequest {
    REQUEST_FLUSH,
    REQUEST_TOGGLE_RECORDING,
    REQUEST_NONE
};

class QmlProfilerApplication : public QCoreApplication
{
    Q_OBJECT
public:
    QmlProfilerApplication(int &argc, char **argv);
    ~QmlProfilerApplication();

    void parseArguments();
    int exec();
    bool isInteractive() const;
    void startConsole();
    void notifyTraceStarted();
    void outputData();

private:
    void run();
    void tryToConnect();
    void connected();
    void disconnected();
    void processHasOutput();
    void processFinished();

    void traceClientEnabledChanged(bool enabled);
    void traceFinished();

    void registerCommands();
    void prompt(const QString &line = QString(), bool ready = true);
    void logError(const QString &error);
    void logWarning(const QString &warning);
    void logStatus(const QString &status);

    quint64 parseFeatures(const QStringList &featureList, const QString &values, bool exclude);
    void confirmOverwriteThen(std::function<void()> action);
    void flush();
    void output();

    enum ApplicationMode {
        LaunchMode,
        AttachMode
    } m_runMode;

    // LaunchMode
    QString m_executablePath;
    QStringList m_arguments;
    QProcess *m_process;

    QString m_socketFile;
    QString m_hostName;
    quint16 m_port;
    QString m_outputFile;
    QString m_interactiveOutputFile;

    PendingRequest m_pendingRequest;
    bool m_verbose;
    bool m_recording;
    bool m_interactive;

    QScopedPointer<QQmlDebugConnection> m_connection;
    QScopedPointer<QmlProfilerClient> m_qmlProfilerClient;
    QScopedPointer<QQmlProfilerEventReceiver> m_profilerData;
    QQmlDebugConsole m_console;
    QTimer m_connectTimer;
    uint m_connectionAttempts;
};

#endif // QMLPROFILERAPPLICATION_H
