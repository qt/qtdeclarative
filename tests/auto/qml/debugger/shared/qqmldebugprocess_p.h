// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLDEBUGPROCESS_P_H
#define QQMLDEBUGPROCESS_P_H

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

#include <QtCore/qprocess.h>
#include <QtCore/qtimer.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qmutex.h>

class QQmlDebugProcess : public QObject
{
    Q_OBJECT
public:
    QQmlDebugProcess(const QString &executable, QObject *parent = nullptr);
    ~QQmlDebugProcess();

    QString stateString() const;

    void addEnvironment(const QString &environment);

    void start(const QStringList &arguments);
    bool waitForSessionStart();
    int debugPort() const;

    bool waitForFinished();
    QProcess::ProcessState state() const;
    QProcess::ExitStatus exitStatus() const;

    QString output() const;
    void stop();
    void setMaximumBindErrors(int numErrors);

Q_SIGNALS:
    void readyReadStandardOutput();
    void finished();

private Q_SLOTS:
    void processAppOutput();
    void processError(QProcess::ProcessError error);

private:
    enum SessionState {
        SessionUnknown,
        SessionStarted,
        SessionFailed
    };

    void timeout();

    QString m_executable;
    QProcess m_process;
    QString m_outputBuffer;
    QString m_output;
    SessionState m_state = SessionUnknown;
    QStringList m_environment;
    int m_port = 0;
    int m_maximumBindErrors = 0;
    int m_receivedBindErrors = 0;
};

#endif // QQMLDEBUGPROCESS_P_H
