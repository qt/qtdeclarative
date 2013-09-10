/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4debugservice_p.h"
#include "qqmldebugservice_p_p.h"
#include "qqmlengine.h"
#include "qv4debugging_p.h"
#include "qv4engine_p.h"
#include "qv4function_p.h"

#include <private/qv8engine_p.h>

const char *V4_CONNECT = "connect";
const char *V4_BREAK_ON_SIGNAL = "breakonsignal";
const char *V4_ADD_BREAKPOINT = "addBreakpoint";
const char *V4_REMOVE_BREAKPOINT = "removeBreakpoint";
const char *V4_PAUSE = "interrupt";
const char *V4_ALL = "all";
const char *V4_BREAK = "break";

const char *V4_FILENAME = "filename";
const char *V4_LINENUMBER = "linenumber";

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QV4DebugService, v4ServiceInstance)

class QV4DebuggerAgent : public QV4::Debugging::DebuggerAgent
{
public slots:
    virtual void debuggerPaused(QV4::Debugging::Debugger *debugger);
};

class QV4DebugServicePrivate : public QQmlDebugServicePrivate
{
    Q_DECLARE_PUBLIC(QV4DebugService)

public:
    QV4DebugServicePrivate() : version(1) {}

    static QByteArray packMessage(const QByteArray &command, int querySequence, const QByteArray &message = QByteArray())
    {
        QByteArray reply;
        QQmlDebugStream rs(&reply, QIODevice::WriteOnly);
        const QByteArray cmd("V4DEBUG");
        rs << cmd << QByteArray::number(++sequence) << QByteArray::number(querySequence) << command << message;
        return reply;
    }

    void processCommand(const QByteArray &command, const QByteArray &data);
    void addBreakpoint(const QByteArray &data);
    void removeBreakpoint(const QByteArray &data);

    QMutex initializeMutex;
    QWaitCondition initializeCondition;
    QV4DebuggerAgent debuggerAgent;

    QStringList breakOnSignals;
    QMap<int, QV4::Debugging::Debugger *> debuggerMap;
    static int debuggerIndex;
    static int sequence;
    const int version;
};

int QV4DebugServicePrivate::debuggerIndex = 0;
int QV4DebugServicePrivate::sequence = 0;

QV4DebugService::QV4DebugService(QObject *parent)
    : QQmlDebugService(*(new QV4DebugServicePrivate()),
                       QStringLiteral("V4Debugger"), 1, parent)
{
    Q_D(QV4DebugService);

    // don't execute stateChanged, messageReceived in parallel
    QMutexLocker lock(&d->initializeMutex);

    if (registerService() == Enabled && blockingMode()) {
        // let's wait for first message ...
        d->initializeCondition.wait(&d->initializeMutex);
    }
}

QV4DebugService::~QV4DebugService()
{
}

QV4DebugService *QV4DebugService::instance()
{
    return v4ServiceInstance();
}

void QV4DebugService::addEngine(const QQmlEngine *engine)
{
    Q_D(QV4DebugService);
    if (engine) {
        QV4::ExecutionEngine *ee = QV8Engine::getV4(engine->handle());
        if (ee) {
            ee->enableDebugger();
            QV4::Debugging::Debugger *debugger = ee->debugger;
            d->debuggerMap.insert(d->debuggerIndex++, debugger);
            d->debuggerAgent.addDebugger(debugger);
        }
    }
}

void QV4DebugService::removeEngine(const QQmlEngine *engine)
{
    Q_D(QV4DebugService);
    if (engine){
        const QV4::ExecutionEngine *ee = QV8Engine::getV4(engine->handle());
        if (ee) {
            QV4::Debugging::Debugger *debugger = ee->debugger;
            typedef QMap<int, QV4::Debugging::Debugger *>::const_iterator DebuggerMapIterator;
            const DebuggerMapIterator end = d->debuggerMap.constEnd();
            for (DebuggerMapIterator i = d->debuggerMap.constBegin(); i != end; ++i) {
                if (i.value() == debugger) {
                    d->debuggerMap.remove(i.key());
                    break;
                }
            }
            d->debuggerAgent.removeDebugger(debugger);
        }
    }
}

void QV4DebugService::signalEmitted(const QString &signal)
{
    //This function is only called by QQmlBoundSignal
    //only if there is a slot connected to the signal. Hence, there
    //is no need for additional check.
    Q_D(QV4DebugService);

    //Parse just the name and remove the class info
    //Normalize to Lower case.
    QString signalName = signal.left(signal.indexOf(QLatin1Char('('))).toLower();

    foreach (const QString &signal, d->breakOnSignals) {
        if (signal == signalName) {
            // TODO: pause debugger
            break;
        }
    }
}

void QV4DebugService::stateChanged(QQmlDebugService::State newState)
{
    Q_D(QV4DebugService);
    QMutexLocker lock(&d->initializeMutex);

    if (newState != Enabled) {
        // wake up constructor in blocking mode
        // (we might got disabled before first message arrived)
        d->initializeCondition.wakeAll();
    }
}

void QV4DebugService::messageReceived(const QByteArray &message)
{
    Q_D(QV4DebugService);
    QMutexLocker lock(&d->initializeMutex);

    QQmlDebugStream ms(message);
    QByteArray header;
    ms >> header;

    if (header == "V4DEBUG") {
        QByteArray sequenceValue;
        QByteArray command;
        QByteArray data;
        ms >> sequenceValue >> command >> data;

        QQmlDebugStream ds(data);

        QByteArray versionValue;
        QByteArray debuggerValue;
        ds >> versionValue >> debuggerValue; // unused for now

        int querySequence = sequenceValue.toInt();
        if (command == V4_BREAK_ON_SIGNAL) {
            QByteArray signal;
            bool enabled;
            ds >> signal >> enabled;
             //Normalize to lower case.
            QString signalName(QString::fromUtf8(signal).toLower());
            if (enabled)
                d->breakOnSignals.append(signalName);
            else
                d->breakOnSignals.removeOne(signalName);
        } else if (command == V4_ADD_BREAKPOINT) {
            QMetaObject::invokeMethod(this, "addBreakpoint", Qt::QueuedConnection,
                                      Q_ARG(QByteArray, data), Q_ARG(int, querySequence));
        } else if (command == V4_REMOVE_BREAKPOINT) {
            QMetaObject::invokeMethod(this, "removeBreakpoint", Qt::QueuedConnection,
                                      Q_ARG(QByteArray, data), Q_ARG(int, querySequence));
        } else if (command == V4_PAUSE) {
            int id = ds.atEnd() ? debuggerValue.toInt() : -1;
            QMetaObject::invokeMethod(this, "pause", Qt::QueuedConnection, Q_ARG(int, id),
                                      Q_ARG(int, querySequence));
        } else if (command == V4_CONNECT) {
            QByteArray response;
            QQmlDebugStream rs(&response, QIODevice::WriteOnly);
            rs << QByteArray::number(d->version) << QByteArray::number(1);
            sendMessage(d->packMessage(command, sequenceValue.toInt(), response));

            d->initializeCondition.wakeAll();
        } else {
            QByteArray response;
            QQmlDebugStream rs(&response, QIODevice::WriteOnly);
            rs << QByteArray::number(d->version) << QByteArray::number(0);
            sendMessage(d->packMessage(command, sequenceValue.toInt(), response));
        }
    }
}

void QV4DebuggerAgent::debuggerPaused(QV4::Debugging::Debugger *debugger)
{
    QByteArray data;
    QQmlDebugStream message(&data, QIODevice::WriteOnly);

    QV4::Debugging::Debugger::ExecutionState state = debugger->currentExecutionState();
    message << V4_FILENAME << state.fileName.toLatin1();
    message << V4_LINENUMBER << QByteArray().number(state.lineNumber);

    QV4DebugService::instance()->sendMessage(QV4DebugServicePrivate::packMessage(V4_BREAK, -1, data));

    qDebug() << Q_FUNC_INFO;
}

void QV4DebugService::pause(int debuggerId, int querySequence)
{
    Q_D(QV4DebugService);

    debuggerId == -1 ? d->debuggerAgent.pauseAll()
                     : d->debuggerAgent.pause(d->debuggerMap.value(debuggerId));
    QByteArray response;
    QQmlDebugStream rs(&response, QIODevice::WriteOnly);
    rs << QByteArray::number(d->version) << QByteArray::number(1);
    sendMessage(d->packMessage(V4_PAUSE, querySequence, response));
}

void QV4DebugService::addBreakpoint(const QByteArray &data, int querySequence)
{
    Q_D(QV4DebugService);

    QQmlDebugStream ds(data);
    QString fileName;
    int lineNumber = -1;
    while (!ds.atEnd()) {
        QByteArray key;
        QByteArray value;
        ds >> key >> value;
        if (key == V4_FILENAME)
            fileName = QString::fromLatin1(value);
        else if (key == V4_LINENUMBER)
            lineNumber = value.toInt();
    }
    d->debuggerAgent.addBreakPoint(fileName, lineNumber);
    QByteArray response;
    QQmlDebugStream rs(&response, QIODevice::WriteOnly);
    rs << QByteArray::number(d->version) << QByteArray::number(1);
    sendMessage(d->packMessage(V4_ADD_BREAKPOINT, querySequence, response));
}

void QV4DebugService::removeBreakpoint(const QByteArray &data, int querySequence)
{
    Q_D(QV4DebugService);

    QQmlDebugStream ds(data);
    QString fileName;
    int lineNumber = -1;
    while (!ds.atEnd()) {
        QByteArray key;
        QByteArray value;
        ds >> key >> value;
        if (key == V4_FILENAME)
            fileName = QString::fromLatin1(value);
        else if (key == V4_LINENUMBER)
            lineNumber = value.toInt();
    }
    d->debuggerAgent.removeBreakPoint(fileName, lineNumber);
    QByteArray response;
    QQmlDebugStream rs(&response, QIODevice::WriteOnly);
    rs << QByteArray::number(d->version) << QByteArray::number(1);
    sendMessage(d->packMessage(V4_REMOVE_BREAKPOINT, querySequence, response));
}

QT_END_NAMESPACE
