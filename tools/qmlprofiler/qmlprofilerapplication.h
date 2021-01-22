/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QMLPROFILERAPPLICATION_H
#define QMLPROFILERAPPLICATION_H

#include "qmlprofilerclient.h"
#include "qmlprofilerdata.h"

#include <private/qqmldebugconnection_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qprocess.h>
#include <QtCore/qtimer.h>
#include <QtNetwork/qabstractsocket.h>

enum PendingRequest {
    REQUEST_QUIT,
    REQUEST_FLUSH_FILE,
    REQUEST_FLUSH,
    REQUEST_OUTPUT_FILE,
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
    void userCommand(const QString &command);
    void notifyTraceStarted();
    void outputData();

signals:
    void readyForCommand();

private:
    void run();
    void tryToConnect();
    void connected();
    void disconnected();
    void processHasOutput();
    void processFinished();

    void traceClientEnabledChanged(bool enabled);
    void traceFinished();

    void prompt(const QString &line = QString(), bool ready = true);
    void logError(const QString &error);
    void logStatus(const QString &status);

    quint64 parseFeatures(const QStringList &featureList, const QString &values, bool exclude);
    bool checkOutputFile(PendingRequest pending);
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
    QScopedPointer<QmlProfilerData> m_profilerData;
    QTimer m_connectTimer;
    uint m_connectionAttempts;
};

#endif // QMLPROFILERAPPLICATION_H
