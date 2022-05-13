// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmldebugprocess_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qregularexpression.h>

#include <QtTest/qtest.h>

QQmlDebugProcess::QQmlDebugProcess(const QString &executable, QObject *parent)
    : QObject(parent)
    , m_executable(executable)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &QQmlDebugProcess::processAppOutput);
    connect(&m_process, &QProcess::errorOccurred,
            this, &QQmlDebugProcess::processError);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this]() {
        if (m_state == SessionUnknown)
            m_state = SessionFailed;
        emit finished();
    });
}

QQmlDebugProcess::~QQmlDebugProcess()
{
    stop();
}

QString QQmlDebugProcess::stateString() const
{
    QString stateStr;
    switch (m_process.state()) {
    case QProcess::NotRunning: {
        stateStr = "not running";
        if (m_process.exitStatus() == QProcess::CrashExit)
            stateStr += " (crashed!)";
        else
            stateStr += ", return value " + QString::number(m_process.exitCode());
        break;
    }
    case QProcess::Starting:
        stateStr = "starting";
        break;
    case QProcess::Running:
        stateStr = "running";
        break;
    }
    return stateStr;
}

void QQmlDebugProcess::start(const QStringList &arguments)
{
#ifdef Q_OS_MAC
    // make sure m_executable points to the actual binary even if it's inside an app bundle
    QFileInfo binFile(m_executable);
    if (!binFile.isExecutable()) {
        QDir bundleDir(m_executable + ".app");
        if (bundleDir.exists()) {
            m_executable = bundleDir.absoluteFilePath("Contents/MacOS/" + binFile.baseName());
            //qDebug() << Q_FUNC_INFO << "found bundled binary" << m_executable;
        }
    }
#endif
    m_port = 0;
    m_process.setEnvironment(QProcess::systemEnvironment() + m_environment);
    m_process.start(m_executable, arguments);
    if (!m_process.waitForStarted()) {
        qWarning() << "QML Debug Client: Could not launch app " << m_executable
                   << ": " << m_process.errorString();
    }
}

void QQmlDebugProcess::stop()
{
    if (m_process.state() != QProcess::NotRunning) {
        disconnect(&m_process, &QProcess::errorOccurred, this, &QQmlDebugProcess::processError);
        m_process.kill();
        if (!m_process.waitForFinished(5000))
            qFatal("Debug process refused to be killed. We need to crash now.");
    }
}

void QQmlDebugProcess::setMaximumBindErrors(int ignore)
{
    m_maximumBindErrors = ignore;
}

void QQmlDebugProcess::timeout()
{
    qWarning() << "Timeout while waiting for QML debugging messages "
                  "in application output. Process is in state" << m_process.state()
               << ", Output:" << m_output << ".";
}

bool QQmlDebugProcess::waitForSessionStart()
{
    if (m_process.state() != QProcess::Running) {
        qWarning() << "Could not start up " << m_executable;
        return false;
    } else if (m_state == SessionStarted) {
        return true;
    } else if (m_state == SessionFailed) {
        return false;
    }

    if (!QTest::qWaitFor(
                [this]() { return m_state == SessionFailed || m_state == SessionStarted; },
                15000)) {
        timeout();
    }

    return m_state == SessionStarted;
}

int QQmlDebugProcess::debugPort() const
{
    return m_port;
}

bool QQmlDebugProcess::waitForFinished()
{
    return m_process.waitForFinished();
}

QProcess::ProcessState QQmlDebugProcess::state() const
{
    return m_process.state();
}

QProcess::ExitStatus QQmlDebugProcess::exitStatus() const
{
    return m_process.exitStatus();
}

void QQmlDebugProcess::addEnvironment(const QString &environment)
{
    m_environment.append(environment);
}

QString QQmlDebugProcess::output() const
{
    return m_output;
}

void QQmlDebugProcess::processAppOutput()
{
    bool outputFromAppItself = false;

    QString newOutput = m_process.readAll();
    m_output.append(newOutput);
    m_outputBuffer.append(newOutput);

    while (true) {
        const int nlIndex = m_outputBuffer.indexOf(QLatin1Char('\n'));
        if (nlIndex < 0) // no further complete lines
            break;
        const QString line = m_outputBuffer.left(nlIndex);
        m_outputBuffer = m_outputBuffer.right(m_outputBuffer.size() - nlIndex - 1);

        if (line.contains("QML Debugger:")) {
            auto portRx = QRegularExpression("Waiting for connection on port (\\d+)").match(line);
            if (portRx.hasMatch()) {
                m_port = portRx.captured(1).toInt();
                m_state = SessionStarted;
                continue;
            }
            if (line.contains("Unable to listen")) {
                if (++m_receivedBindErrors >= m_maximumBindErrors) {
                    if (m_maximumBindErrors == 0)
                        qWarning() << "App was unable to bind to port!";
                    m_state = SessionFailed;
                 }
                 continue;
            }
        }

        // set to true if there is output not coming from the debugger or we don't understand it
        outputFromAppItself = true;
    }

    if (outputFromAppItself)
        emit readyReadStandardOutput();
}

void QQmlDebugProcess::processError(QProcess::ProcessError error)
{
    qDebug() << "An error occurred while waiting for debug process to become available:";
    switch (error) {
    case QProcess::FailedToStart:
        qDebug() << "Process failed to start.";
        break;
    case QProcess::Crashed:
        qDebug() << "Process crashed.";
        break;
    case QProcess::Timedout:
        qDebug() << "Process timed out.";
        break;
    case QProcess::WriteError:
        qDebug() << "Error while writing to process.";
        break;
    case QProcess::ReadError:
        qDebug() << "Error while reading from process.";
        break;
    case QProcess::UnknownError:
        qDebug() << "Unknown process error.";
        break;
    }
}
