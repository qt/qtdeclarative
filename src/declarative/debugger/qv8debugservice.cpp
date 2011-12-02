/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv8debugservice_p.h"
#include "qdeclarativedebugservice_p_p.h"
#include <private/qv8debug_p.h>
#include <private/qjsconverter_impl_p.h>

#include <QtCore/QHash>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>

#define DEBUGGER_SCRIPT "(function(){"\
        "var DebuggerScript = {};"\
        "DebuggerScript.getScriptName = function(eventData){"\
        "return eventData.script_.script_.nameOrSourceURL();"\
        "};"\
        "DebuggerScript.getScripts = function(){"\
        "var result = [];"\
        "var scripts = Debug.scripts();"\
        "for (var i = 0; i < scripts.length; ++i) {"\
        "var script = scripts[i];"\
        "if (script.type == Debug.ScriptType.Native) continue;"\
        "result.push(script.nameOrSourceURL());"\
        "}"\
        "return result;"\
        "};"\
        "return DebuggerScript;"\
        "})();"

#define DEBUGGER_UTILITY "(function(){"\
        "var DebuggerUtility = {};"\
        "DebuggerUtility.parseRequest = function(request){"\
        "return JSON.parse(request);"\
        "};"\
        "DebuggerUtility.stringifyRequest = function(request){"\
        "return JSON.stringify(request);"\
        "};"\
        "return DebuggerUtility;"\
        "})();"

const char *V8_DEBUGGER_KEY_CONNECT = "connect";
const char *V8_DEBUGGER_KEY_INTERRUPT = "interrupt";
const char *V8_DEBUGGER_KEY_DISCONNECT = "disconnect";
const char *V8_DEBUGGER_KEY_CLEARBREAKPOINT = "clearbreakpoint";
const char *V8_DEBUGGER_KEY_CHANGEBREAKPOINT = "changebreakpoint";
const char *V8_DEBUGGER_KEY_SETBREAKPOINT = "setbreakpoint";
const char *V8_DEBUGGER_KEY_TARGET = "target";
const char *V8_DEBUGGER_KEY_SCRIPT = "script";
const char *V8_DEBUGGER_KEY_COMMAND = "command";
const char *V8_DEBUGGER_KEY_SEQ = "seq";
const char *V8_DEBUGGER_KEY_REQUEST_SEQ = "request_seq";
const char *V8_DEBUGGER_KEY_SUCCESS = "success";
const char *V8_DEBUGGER_KEY_BREAKPOINT = "breakpoint";
const char *V8_DEBUGGER_KEY_ARGUMENTS = "arguments";
const char *V8_DEBUGGER_KEY_ENABLED = "enabled";
const char *V8_DEBUGGER_KEY_TYPE = "type";
const char *V8_DEBUGGER_KEY_EVENT = "event";
const char *V8_DEBUGGER_KEY_BODY = "body";
const char *V8_DEBUGGER_KEY_RUNNING = "running";

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

void DebugMessageHandler(const v8::Debug::Message& message)
{
    v8::DebugEvent event = message.GetEvent();

    if (event != v8::Break && event != v8::Exception &&
            event != v8::AfterCompile && event != v8::BeforeCompile) {
        return;
    }

    const QString response(QJSConverter::toString(message.GetJSON()));

    v8ServiceInstancePtr->debugMessageHandler(response, message.WillStartRunning());

    if (event == v8::AfterCompile) {
        v8ServiceInstancePtr->appendSourcePath(message.GetEventData());
    } //TODO::v8::Exception
}

class QV8DebugServicePrivate : public QDeclarativeDebugServicePrivate
{
public:
    QV8DebugServicePrivate()
        : connectReceived(false)
        , scheduleBreak(false)
        , debuggerThreadIsolate(0)
        , isRunning(true)
    {
    }

    ~QV8DebugServicePrivate()
    {
        debuggerScript.Dispose();
        debuggerUtility.Dispose();
        debuggerUtilityContext.Dispose();
        if (debuggerThreadIsolate)
            debuggerThreadIsolate->Dispose();
    }

    void initializeDebuggerThread();

    void sendDebugMessage(const QString &message);
    void updateSourcePath(const QString &sourcePath);
    static QByteArray packMessage(const QString &message);
    QString createResponse(const QVariantMap &response) const;
    v8::Handle<v8::Object> createV8Object(const QVariantMap &map) const;

    bool connectReceived;
    bool scheduleBreak;

    v8::Isolate *debuggerThreadIsolate;
    v8::Persistent<v8::Object> debuggerScript;
    v8::Persistent<v8::Object> debuggerUtility;
    v8::Persistent<v8::Context> debuggerUtilityContext;
    // keep messageReceived() from running until initialize() has finished
    QMutex initializeMutex;

    bool isRunning;
    QHash<QString, QString> sourcePath;
    QHash<QString, QString> requestCache;
    QHash<int, SignalHandlerData> handlersList;
};

QV8DebugService::QV8DebugService(QObject *parent)
    : QDeclarativeDebugService(*(new QV8DebugServicePrivate()),
                               QLatin1String("V8Debugger"), parent)
{
    Q_D(QV8DebugService);
    v8ServiceInstancePtr = this;
    // wait for statusChanged() -> initialize()
    d->initializeMutex.lock();
    if (registerService() == Enabled) {
        initialize(false);
        // ,block mode, client attached
        while (!d->connectReceived) {
            waitForMessage();
        }
    } else {
        d->initializeMutex.unlock();
    }
}

QV8DebugService::~QV8DebugService()
{
}

QV8DebugService *QV8DebugService::instance()
{
    return v8ServiceInstance();
}

void QV8DebugService::initialize()
{
    // just make sure that the service is properly registered
    v8ServiceInstance();
}

void QV8DebugService::debugMessageHandler(const QString &message, bool willStartRunning)
{
    Q_D(QV8DebugService);
    d->isRunning = willStartRunning;

    if (d->scheduleBreak)
        scheduledDebugBreak();

    sendMessage(QV8DebugServicePrivate::packMessage(message));
}


void QV8DebugService::appendSourcePath(const v8::Handle<v8::Object> &eventData)
{
    Q_D(QV8DebugService);

    Q_ASSERT(!d->debuggerScript.IsEmpty());
    v8::HandleScope handleScope;
    v8::Local<v8::Context> debuggerContext = v8::Debug::GetDebugContext();
    v8::Context::Scope contextScope(debuggerContext);
    v8::Handle<v8::Function> getScriptNameFn =
            v8::Local<v8::Function>::Cast(d->debuggerScript->Get(v8::String::New("getScriptName")));
    v8::Handle<v8::Value> argv[] = { eventData };
    v8::Handle<v8::Value> scriptName = getScriptNameFn->Call(d->debuggerScript, 1, argv);
    Q_ASSERT(scriptName->IsString());

    d->updateSourcePath(QJSConverter::toString(scriptName->ToString()));
}

void QV8DebugService::signalEmitted(const QString &signal)
{
    //This function is only called by QDeclarativeBoundSignal
    //only if there is a slot connected to the signal. Hence, there
    //is no need for additional check.
    Q_D(QV8DebugService);

    //Parse just the name and remove the class info
    //Normalize to Lower case.
    QString signalName = signal.left(signal.indexOf(QLatin1String("("))).toLower();
    foreach (const SignalHandlerData &data, d->handlersList) {
        if (data.functionName == signalName
                && data.enabled) {
            d->scheduleBreak = true;
        }
    }
    if (d->scheduleBreak)
        scheduledDebugBreak();
}

void QV8DebugService::initialize(bool getCompiledScripts)
{
    Q_D(QV8DebugService);
    if (d->debuggerScript.IsEmpty()) {
        v8::HandleScope handleScope;
        v8::Local<v8::Context> debuggerContext = v8::Debug::GetDebugContext();
        v8::Context::Scope contextScope(debuggerContext);
        v8::Handle<v8::Script> script = v8::Script::New(v8::String::New(DEBUGGER_SCRIPT), 0, 0, v8::Handle<v8::String>(), v8::Script::NativeMode);
        d->debuggerScript = v8::Persistent<v8::Object>::New(v8::Local<v8::Object>::Cast(script->Run()));
    }
    v8::Debug::SetMessageHandler2(DebugMessageHandler);

    if (getCompiledScripts) {
        v8::HandleScope handleScope;
        v8::Local<v8::Context> debuggerContext = v8::Debug::GetDebugContext();
        v8::Context::Scope contextScope(debuggerContext);
        v8::Handle<v8::Function> getScriptsFn =
                v8::Local<v8::Function>::Cast(d->debuggerScript->Get(v8::String::New("getScripts")));
        v8::Handle<v8::Value> result = getScriptsFn->Call(d->debuggerScript, 0, 0);
        if (result.IsEmpty())
            return;

        Q_ASSERT(!result->IsUndefined() && result->IsArray());
        v8::Handle<v8::Array> scripts = v8::Handle<v8::Array>::Cast(result);
        uint len = scripts->Length();
        for (uint i = 0; i < len; ++i) {
             d->updateSourcePath(QJSConverter::toString(scripts->Get(i)->ToString()));
        }
    }

    d->initializeMutex.unlock();
}

void QV8DebugService::scheduledDebugBreak()
{
    Q_D(QV8DebugService);
    if (d->scheduleBreak) {
        v8::Debug::DebugBreak();
        d->scheduleBreak = false;
    }
}

void QV8DebugService::cancelDebugBreak()
{
    v8::Debug::CancelDebugBreak();
}

// executed in the debugger thread
void QV8DebugService::statusChanged(QDeclarativeDebugService::Status newStatus)
{
    Q_D(QV8DebugService);
    if (newStatus == Enabled) {
        if (!d->debuggerThreadIsolate)
            d->initializeDebuggerThread();

        // execute in GUI thread
        d->initializeMutex.lock();
        QMetaObject::invokeMethod(this, "initialize", Qt::QueuedConnection, Q_ARG(bool, true));
    }
}

// executed in the debugger thread
void QV8DebugService::messageReceived(const QByteArray &message)
{
    Q_D(QV8DebugService);

    QDataStream ds(message);
    QByteArray command;
    ds >> command;

    QMutexLocker locker(&d->initializeMutex);

    if (command == "V8DEBUG") {
        if (!d->debuggerThreadIsolate)
            d->initializeDebuggerThread();


        QString request;
        {
            QByteArray requestArray;
            ds >> requestArray;
            request = QString::fromUtf8(requestArray);
        }

        bool forwardRequestToV8 = false;
        Q_ASSERT(!d->debuggerUtility.IsEmpty());
        {
            v8::Isolate::Scope i_scope(d->debuggerThreadIsolate);

            v8::HandleScope handleScope;
            v8::Context::Scope contextScope(d->debuggerUtilityContext);

            v8::Handle<v8::Function> parseFn =
                    v8::Local<v8::Function>::Cast(d->debuggerUtility->Get(v8::String::New("parseRequest")));
            v8::Handle<v8::Value> argv[] = { QJSConverter::toString(request) };
            v8::Handle<v8::Value> result = parseFn->Call(d->debuggerUtility, 1, argv);
            Q_ASSERT(result->IsObject());

            v8::Handle<v8::Object> requestObj = v8::Handle<v8::Object>::Cast(result);
            const QString debugCommand = QJSConverter::toString(requestObj->Get(v8::String::New(V8_DEBUGGER_KEY_COMMAND))->ToString());
            const int sequence = requestObj->Get(v8::String::New(V8_DEBUGGER_KEY_SEQ))->Int32Value();

            if (debugCommand == QLatin1String(V8_DEBUGGER_KEY_CONNECT)) {
                d->connectReceived = true;
                QVariantMap response;
                response.insert(QLatin1String(V8_DEBUGGER_KEY_COMMAND), QVariant(debugCommand));
                response.insert(QLatin1String(V8_DEBUGGER_KEY_REQUEST_SEQ), QVariant(sequence));
                response.insert(QLatin1String(V8_DEBUGGER_KEY_SUCCESS), QVariant(true));
                sendMessage(QV8DebugServicePrivate::packMessage(d->createResponse(response)));

            } else if (debugCommand == QLatin1String(V8_DEBUGGER_KEY_INTERRUPT)) {
                // break has to be executed in gui thread
                d->scheduleBreak = true;
                QMetaObject::invokeMethod(this, "scheduledDebugBreak", Qt::QueuedConnection);
                QVariantMap response;
                response.insert(QLatin1String(V8_DEBUGGER_KEY_COMMAND), QVariant(debugCommand));
                response.insert(QLatin1String(V8_DEBUGGER_KEY_REQUEST_SEQ), QVariant(sequence));
                response.insert(QLatin1String(V8_DEBUGGER_KEY_SUCCESS), QVariant(true));
                sendMessage(QV8DebugServicePrivate::packMessage(d->createResponse(response)));

            } else if (debugCommand == QLatin1String(V8_DEBUGGER_KEY_DISCONNECT)) {
                // cancel break has to be executed in gui thread
                QMetaObject::invokeMethod(this, "cancelDebugBreak", Qt::QueuedConnection);
                forwardRequestToV8 = true;

            } else if (debugCommand == QLatin1String(V8_DEBUGGER_KEY_CLEARBREAKPOINT)) {
                //check if the breakpoint is a negative integer (event breakpoint)
                v8::Handle<v8::Object> argsObj = requestObj->Get(v8::String::New(V8_DEBUGGER_KEY_ARGUMENTS))->ToObject();
                const int bp = argsObj->Get(v8::String::New(V8_DEBUGGER_KEY_BREAKPOINT))->Int32Value();

                if (bp < 0) {
                    d->handlersList.remove(bp);
                } else {
                    forwardRequestToV8 = true;
                }

            } else if (debugCommand == QLatin1String(V8_DEBUGGER_KEY_CHANGEBREAKPOINT)) {
                //check if the breakpoint is a negative integer (event breakpoint)
                v8::Handle<v8::Object> argsObj = requestObj->Get(v8::String::New(V8_DEBUGGER_KEY_ARGUMENTS))->ToObject();
                const int bp = argsObj->Get(v8::String::New(V8_DEBUGGER_KEY_BREAKPOINT))->Int32Value();

                if (bp < 0) {
                    SignalHandlerData data = d->handlersList.value(bp);
                    data.enabled = argsObj->Get(v8::String::New(V8_DEBUGGER_KEY_ENABLED))->BooleanValue();
                    d->handlersList.insert(bp, data);

                } else {
                    forwardRequestToV8 = true;
                }

            } else if (debugCommand == QLatin1String(V8_DEBUGGER_KEY_SETBREAKPOINT)) {
                v8::Handle<v8::Object> argsObj = requestObj->Get(v8::String::New(V8_DEBUGGER_KEY_ARGUMENTS))->ToObject();
                const QString type = QJSConverter::toString(argsObj->Get(v8::String::New(V8_DEBUGGER_KEY_TYPE))->ToString());
                QString target = QJSConverter::toString(argsObj->Get(v8::String::New(V8_DEBUGGER_KEY_TARGET))->ToString());

                if (type == QLatin1String(V8_DEBUGGER_KEY_SCRIPT)) {
                    //check if the script associated with the breakpoint is cached
                    if (d->sourcePath.contains(target)) {
                        //replace fileName with correct filePath
                        request.replace(target, d->sourcePath.value(target));
                        forwardRequestToV8 = true;

                    } else {
                        //store the message till file is compiled
                        d->requestCache.insertMulti(target, request);
                    }

                } else if (type == QLatin1String(V8_DEBUGGER_KEY_EVENT)) {
                    //Handle this internally
                    bool success = true;
                    if (target.startsWith(QLatin1String("on"))) {
                        SignalHandlerData data;
                        //Only store the probable signal name.
                        //Normalize to lower case.
                        data.functionName = target.remove(0,2).toLower();
                        data.enabled = argsObj->Get(v8::String::New(V8_DEBUGGER_KEY_ENABLED))->BooleanValue();
                        d->handlersList.insert(-sequence, data);
                    } else {
                        success = false;
                    }
                    //TODO::have to send back response
                    QVariantMap body;
                    body.insert(QLatin1String(V8_DEBUGGER_KEY_TYPE), QVariant(QLatin1String(V8_DEBUGGER_KEY_EVENT)));
                    body.insert(QLatin1String(V8_DEBUGGER_KEY_BREAKPOINT), QVariant(-sequence));
                    QVariantMap response;
                    response.insert(QLatin1String(V8_DEBUGGER_KEY_COMMAND), QVariant(debugCommand));
                    response.insert(QLatin1String(V8_DEBUGGER_KEY_REQUEST_SEQ), QVariant(sequence));
                    response.insert(QLatin1String(V8_DEBUGGER_KEY_SUCCESS), QVariant(success));
                    response.insert(QLatin1String(V8_DEBUGGER_KEY_BODY), body);

                    sendMessage(QV8DebugServicePrivate::packMessage(d->createResponse(response)));
                }
            } else {
                //Forward all other requests
                forwardRequestToV8 = true;
            }
        }

        if (forwardRequestToV8)
            d->sendDebugMessage(request);
    }
}

void QV8DebugServicePrivate::initializeDebuggerThread()
{
    Q_ASSERT(!debuggerThreadIsolate);

    //Create an isolate & engine in debugger thread
    debuggerThreadIsolate = v8::Isolate::New();
    v8::Isolate::Scope i_scope(debuggerThreadIsolate);
    if (debuggerUtility.IsEmpty()) {
        v8::HandleScope handleScope;
        debuggerUtilityContext = v8::Context::New();
        v8::Context::Scope contextScope(debuggerUtilityContext);
        v8::Handle<v8::Script> script = v8::Script::New(v8::String::New(DEBUGGER_UTILITY), 0, 0, v8::Handle<v8::String>(), v8::Script::NativeMode);
        debuggerUtility = v8::Persistent<v8::Object>::New(v8::Local<v8::Object>::Cast(script->Run()));
    }
}

void QV8DebugServicePrivate::sendDebugMessage(const QString &message)
{
    v8::Debug::SendCommand(message.utf16(), message.size());
}

void QV8DebugServicePrivate::updateSourcePath(const QString &source)
{
    const QString fileName(QFileInfo(source).fileName());
    sourcePath.insert(fileName, source);

    //Check if there are any pending breakpoint requests for this file
    if (requestCache.contains(fileName)) {
        QList<QString> list = requestCache.values(fileName);
        requestCache.remove(fileName);
        foreach (QString request, list) {
            request.replace(fileName, source);
            sendDebugMessage(request);
        }
    }
}

QByteArray QV8DebugServicePrivate::packMessage(const QString &message)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd("V8DEBUG");
    rs << cmd << message.toUtf8();
    return reply;
}

QString QV8DebugServicePrivate::createResponse(const QVariantMap &response) const
{
    v8::HandleScope handleScope;
    v8::Context::Scope contextScope(debuggerUtilityContext);

    v8::Handle<v8::Object> respObj = createV8Object(response);
    respObj->Set(v8::String::New(V8_DEBUGGER_KEY_TYPE), v8::String::New("response"));
    respObj->Set(v8::String::New(V8_DEBUGGER_KEY_RUNNING), v8::Boolean::New(isRunning));
    v8::Handle<v8::Function> stringifyFn =
            v8::Local<v8::Function>::Cast(debuggerUtility->Get(v8::String::New("stringifyRequest")));
    v8::Handle<v8::Value> argv[] = { respObj };
    v8::Handle<v8::Value> result = stringifyFn->Call(debuggerScript, 1, argv);
    Q_ASSERT(result->IsString());

    return QJSConverter::toString(result->ToString());

}

v8::Handle<v8::Object> QV8DebugServicePrivate::createV8Object(const QVariantMap &map) const
{
    v8::Handle<v8::Object> obj = v8::Object::New();
    foreach (const QVariant &var, map) {
        v8::Handle<v8::String> key = QJSConverter::toString(map.key(var));
        v8::Handle<v8::Value> val;
        switch (var.type()) {
        case QVariant::Bool:
            val = v8::Boolean::New(var.toBool());
            break;
        case QVariant::Int:
            val = v8::Int32::New(var.toInt());
            break;
        case QVariant::String:
            val = QJSConverter::toString(var.toString());
            break;
        case QVariant::Map:
            createV8Object(var.toMap());
            break;
        default:
            //Add other types when required.
            //Not handled currently
            break;
        }
        if (!val.IsEmpty())
            obj->Set(key, val);
    }
    return obj;
}

QT_END_NAMESPACE
