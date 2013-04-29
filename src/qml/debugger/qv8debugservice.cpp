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

#include "qv8debugservice_p.h"
#include "qqmldebugservice_p_p.h"
#include <private/qjsconverter_impl_p.h>
#include <private/qv4compiler_p.h>
#include <private/qv8engine_p.h>

#include <QtCore/QHash>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

//V8 DEBUG SERVICE PROTOCOL
// <HEADER><COMMAND><DATA>
// <HEADER> : "V8DEBUG"
// <COMMAND> : ["connect", "disconnect", "interrupt",
//              "v8request", "v8message", "breakonsignal",
//              "breakaftercompile"]
// <DATA> : connect, disconnect, interrupt: empty
//          v8request, v8message: <JSONrequest_string>
//          breakonsignal: <signalname_string><enabled_bool>
//          breakaftercompile: <enabled_bool>

const char *V8_DEBUGGER_KEY_VERSION = "version";
const char *V8_DEBUGGER_KEY_CONNECT = "connect";
const char *V8_DEBUGGER_KEY_INTERRUPT = "interrupt";
const char *V8_DEBUGGER_KEY_DISCONNECT = "disconnect";
const char *V8_DEBUGGER_KEY_REQUEST = "v8request";
const char *V8_DEBUGGER_KEY_V8MESSAGE = "v8message";
const char *V8_DEBUGGER_KEY_BREAK_ON_SIGNAL = "breakonsignal";

QT_BEGIN_NAMESPACE

struct SignalHandlerData
{
    QString functionName;
    bool enabled;
};

Q_GLOBAL_STATIC(QV8DebugService, v8ServiceInstance)

// DebugMessageHandler will call back already when the QV8DebugService constructor is
// running, we therefore need a plain pointer.
static QV8DebugService *v8ServiceInstancePtr = 0;

void DebugMessageDispatchHandler()
{
    QMetaObject::invokeMethod(v8ServiceInstancePtr, "processDebugMessages", Qt::QueuedConnection);
}

void DebugMessageHandler(const v8::Debug::Message& message)
{
    v8::DebugEvent event = message.GetEvent();

    if (message.IsEvent()) {
        if (event == v8::AfterCompile || event == v8::BeforeCompile)
            return;
    } else if (event != v8::Break && event != v8::Exception &&
               event != v8::AfterCompile && event != v8::BeforeCompile) {
        return;
    }

    v8ServiceInstancePtr->debugMessageHandler(QJSConverter::toString(message.GetJSON()));
}

class QV8DebugServicePrivate : public QQmlDebugServicePrivate
{
public:
    QV8DebugServicePrivate()
        : engine(0),
          debugIsolate(0)
    {
    }

    void initializeDebuggerThread();

    static QByteArray packMessage(const QString &type, const QString &message = QString());

    QMutex initializeMutex;
    QWaitCondition initializeCondition;
    QStringList breakOnSignals;
    const QV8Engine *engine;
    v8::Isolate *debugIsolate;
};

QV8DebugService::QV8DebugService(QObject *parent)
    : QQmlDebugService(*(new QV8DebugServicePrivate()),
                               QStringLiteral("V8Debugger"), 2, parent)
{
    Q_D(QV8DebugService);
    v8ServiceInstancePtr = this;
    // don't execute stateChanged, messageReceived in parallel
    QMutexLocker lock(&d->initializeMutex);

    if (registerService() == Enabled) {
        init();
        if (blockingMode())
            d->initializeCondition.wait(&d->initializeMutex);
    }
}

QV8DebugService::~QV8DebugService()
{
}

QV8DebugService *QV8DebugService::instance()
{
    return v8ServiceInstance();
}

void QV8DebugService::initialize(const QV8Engine *engine)
{
    // just make sure that the service is properly registered
    v8ServiceInstance()->setEngine(engine);
}

void QV8DebugService::setEngine(const QV8Engine *engine)
{
    Q_D(QV8DebugService);

    d->engine = engine;
}

void QV8DebugService::debugMessageHandler(const QString &message)
{
    sendMessage(QV8DebugServicePrivate::packMessage(QLatin1String(V8_DEBUGGER_KEY_V8MESSAGE), message));
}

void QV8DebugService::signalEmitted(const QString &signal)
{
    //This function is only called by QQmlBoundSignal
    //only if there is a slot connected to the signal. Hence, there
    //is no need for additional check.
    Q_D(QV8DebugService);

    //Parse just the name and remove the class info
    //Normalize to Lower case.
    QString signalName = signal.left(signal.indexOf(QLatin1Char('('))).toLower();

    foreach (const QString &signal, d->breakOnSignals) {
        if (signal == signalName) {
            scheduledDebugBreak(true);
            break;
        }
    }
}

// executed in the gui thread
void QV8DebugService::init()
{
    Q_D(QV8DebugService);
    if (!d->debugIsolate)
        d->debugIsolate = v8::Isolate::GetCurrent();
    v8::Debug::SetMessageHandler2(DebugMessageHandler);
    v8::Debug::SetDebugMessageDispatchHandler(DebugMessageDispatchHandler);
    QV4Compiler::enableV4(false);
}

// executed in the gui thread
void QV8DebugService::scheduledDebugBreak(bool schedule)
{
    if (schedule)
        v8::Debug::DebugBreak();
    else
        v8::Debug::CancelDebugBreak();
}

// executed in the debugger thread
void QV8DebugService::stateChanged(QQmlDebugService::State newState)
{
    Q_D(QV8DebugService);
    QMutexLocker lock(&d->initializeMutex);

    if (newState == Enabled) {
        // execute in GUI thread, bock to make sure messageReceived isn't called
        // before it finished.
        QMetaObject::invokeMethod(this, "init", Qt::BlockingQueuedConnection);
    } else {
        // wake up constructor in blocking mode
        // (we might got disabled before first message arrived)
        d->initializeCondition.wakeAll();
    }
}

// executed in the debugger thread
void QV8DebugService::messageReceived(const QByteArray &message)
{
    Q_D(QV8DebugService);
    QMutexLocker lock(&d->initializeMutex);

    QQmlDebugStream ds(message);
    QByteArray header;
    ds >> header;

    if (header == "V8DEBUG") {
        QByteArray command;
        QByteArray data;
        ds >> command >> data;

        if (command == V8_DEBUGGER_KEY_CONNECT) {
            sendMessage(QV8DebugServicePrivate::packMessage(QLatin1String(V8_DEBUGGER_KEY_CONNECT)));
            // wake up constructor in blocking mode
            d->initializeCondition.wakeAll();
        } else if (command == V8_DEBUGGER_KEY_INTERRUPT) {
            // break has to be executed in gui thread
            QMetaObject::invokeMethod(this, "scheduledDebugBreak", Qt::QueuedConnection, Q_ARG(bool, true));
            sendMessage(QV8DebugServicePrivate::packMessage(QLatin1String(V8_DEBUGGER_KEY_INTERRUPT)));

        } else if (command == V8_DEBUGGER_KEY_DISCONNECT) {
            // cancel break has to be executed in gui thread
            QMetaObject::invokeMethod(this, "scheduledDebugBreak", Qt::QueuedConnection, Q_ARG(bool, false));
            sendDebugMessage(QString::fromUtf8(data));

        } else if (command == V8_DEBUGGER_KEY_REQUEST) {
            sendDebugMessage(QString::fromUtf8(data));

        } else if (command == V8_DEBUGGER_KEY_BREAK_ON_SIGNAL) {
            QQmlDebugStream rs(data);
            QByteArray signal;
            bool enabled;
            rs >> signal >> enabled;
             //Normalize to lower case.
            QString signalName(QString::fromUtf8(signal).toLower());
            if (enabled)
                d->breakOnSignals.append(signalName);
            else
                d->breakOnSignals.removeOne(signalName);
            sendMessage(QV8DebugServicePrivate::packMessage(QLatin1String(V8_DEBUGGER_KEY_BREAK_ON_SIGNAL)));
        }
    }
}

void QV8DebugService::sendDebugMessage(const QString &message)
{
    Q_D(QV8DebugService);
    v8::Debug::SendCommand(message.utf16(), message.size(), 0, d->debugIsolate);
}

void QV8DebugService::processDebugMessages()
{
    Q_D(QV8DebugService);
    v8::HandleScope handleScope;
    v8::Context::Scope contextScope(d->engine->context());
    v8::Debug::ProcessDebugMessages();
}

QByteArray QV8DebugServicePrivate::packMessage(const QString &type, const QString &message)
{
    QByteArray reply;
    QQmlDebugStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd("V8DEBUG");
    rs << cmd << type.toUtf8() << message.toUtf8();
    return reply;
}

QT_END_NAMESPACE
