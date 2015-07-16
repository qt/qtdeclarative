/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4debugservice_p.h"
#include "qqmlengine.h"
#include "qv4engine_p.h"
#include "qv4function_p.h"
#include "qqmldebugconnector_p.h"

#include <private/qv8engine_p.h>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

const char *const V4_CONNECT = "connect";
const char *const V4_DISCONNECT = "disconnect";
const char *const V4_BREAK_ON_SIGNAL = "breakonsignal";
const char *const V4_PAUSE = "interrupt";

#define NO_PROTOCOL_TRACING
#ifdef NO_PROTOCOL_TRACING
#  define TRACE_PROTOCOL(x)
#else
#include <QtCore/QDebug>
#  define TRACE_PROTOCOL(x) x
#endif

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QV4DebugService, v4ServiceInstance)

class V8CommandHandler;
class UnknownV8CommandHandler;

class VariableCollector: public QV4::Debugging::Debugger::Collector
{
public:
    VariableCollector(QV4::ExecutionEngine *engine)
        : Collector(engine)
        , destination(0)
    {}

    virtual ~VariableCollector() {}

    void collectScope(QJsonArray *dest, QV4::Debugging::Debugger *debugger, int frameNr, int scopeNr)
    {
        qSwap(destination, dest);
        bool oldIsProp = isProperty();
        setIsProperty(true);
        debugger->collectArgumentsInContext(this, frameNr, scopeNr);
        debugger->collectLocalsInContext(this, frameNr, scopeNr);
        setIsProperty(oldIsProp);
        qSwap(destination, dest);
    }

    void setDestination(QJsonArray *dest)
    { destination = dest; }

    QJsonArray retrieveRefsToInclude()
    {
        QJsonArray result;
        qSwap(refsToInclude, result);
        return result;
    }

    QJsonValue lookup(int handle, bool addRefs = true)
    {
        if (handle < 0)
            handle = -handle;

        if (addRefs)
            foreach (int ref, refsByHandle[handle])
                refsToInclude.append(lookup(ref, false));
        return refs[handle];
    }

    QJsonObject makeRef(int refId)
    {
        QJsonObject ref;
        ref[QLatin1String("ref")] = refId;
        return ref;
    }

    QJsonObject addFunctionRef(const QString &name)
    {
        const int refId = newRefId();

        QJsonObject func;
        func[QLatin1String("handle")] = refId;
        func[QLatin1String("type")] = QStringLiteral("function");
        func[QLatin1String("className")] = QStringLiteral("Function");
        func[QLatin1String("name")] = name;
        insertRef(func, refId);

        return makeRef(refId);
    }

    QJsonObject addScriptRef(const QString &name)
    {
        const int refId = newRefId();

        QJsonObject func;
        func[QLatin1String("handle")] = refId;
        func[QLatin1String("type")] = QStringLiteral("script");
        func[QLatin1String("name")] = name;
        insertRef(func, refId);

        return makeRef(refId);
    }

    QJsonObject addObjectRef(QJsonObject obj, bool anonymous)
    {
        int ref = newRefId();

        if (anonymous)
            ref = -ref;
        obj[QLatin1String("handle")] = ref;
        obj[QLatin1String("type")] = QStringLiteral("object");
        insertRef(obj, ref);
        QSet<int> used;
        qSwap(usedRefs, used);
        refsByHandle.insert(ref, used);

        return makeRef(ref);
    }

protected:
    virtual void addUndefined(const QString &name)
    {
        QJsonObject o;
        addHandle(name, o, QStringLiteral("undefined"));
    }

    virtual void addNull(const QString &name)
    {
        QJsonObject o;
        addHandle(name, o, QStringLiteral("null"));
    }

    virtual void addBoolean(const QString &name, bool value)
    {
        QJsonObject o;
        o[QLatin1String("value")] = value;
        addHandle(name, o, QStringLiteral("boolean"));
    }

    virtual void addString(const QString &name, const QString &value)
    {
        QJsonObject o;
        o[QLatin1String("value")] = value;
        addHandle(name, o, QStringLiteral("string"));
    }

    virtual void addObject(const QString &name, const QV4::Value &value)
    {
        QV4::Scope scope(engine());
        QV4::ScopedObject obj(scope, value.as<QV4::Object>());

        int ref = cachedObjectRef(obj);
        if (ref != -1) {
            addNameRefPair(name, ref);
        } else {
            int ref = newRefId();
            cacheObjectRef(obj, ref);

            QJsonArray properties, *prev = &properties;
            QSet<int> used;
            qSwap(usedRefs, used);
            qSwap(destination, prev);
            collect(obj);
            qSwap(destination, prev);
            qSwap(usedRefs, used);

            QJsonObject o;
            o[QLatin1String("properties")] = properties;
            addHandle(name, o, QStringLiteral("object"), ref);
            refsByHandle.insert(ref, used);
        }
    }

    virtual void addInteger(const QString &name, int value)
    {
        QJsonObject o;
        o[QLatin1String("value")] = value;
        addHandle(name, o, QStringLiteral("number"));
    }

    virtual void addDouble(const QString &name, double value)
    {
        QJsonObject o;
        o[QLatin1String("value")] = value;
        addHandle(name, o, QStringLiteral("number"));
    }

private:
    int addHandle(const QString &name, QJsonObject object, const QString &type, int suppliedRef = -1)
    {
        Q_ASSERT(destination);

        object[QLatin1String("type")] = type;

        QJsonDocument tmp;
        tmp.setObject(object);
        QByteArray key = tmp.toJson(QJsonDocument::Compact);

        int ref;
        if (suppliedRef == -1) {
            ref = refCache.value(key, -1);
            if (ref == -1) {
                ref = newRefId();
                object[QLatin1String("handle")] = ref;
                insertRef(object, ref);
                refCache.insert(key, ref);
            }
        } else {
            ref = suppliedRef;
            object[QLatin1String("handle")] = ref;
            insertRef(object, ref);
            refCache.insert(key, ref);
        }

        addNameRefPair(name, ref);
        return ref;
    }

    void addNameRefPair(const QString &name, int ref)
    {
        QJsonObject nameValuePair;
        nameValuePair[QLatin1String("name")] = name;
        if (isProperty()) {
            nameValuePair[QLatin1String("ref")] = ref;
        } else {
            QJsonObject refObj;
            refObj[QLatin1String("ref")] = ref;
            nameValuePair[QLatin1String("value")] = refObj;
        }
        destination->append(nameValuePair);
        usedRefs.insert(ref);
    }

    int newRefId()
    {
        int ref = refs.count();
        refs.insert(ref, QJsonValue());
        return ref;
    }

    void insertRef(const QJsonValue &value, int refId)
    {
        if (refId < 0)
            refId = -refId;

        refs.insert(refId, value);
        refsToInclude.append(value);
    }

    void cacheObjectRef(QV4::Object *obj, int ref)
    {
        objectRefs.insert(obj, ref);
    }

    int cachedObjectRef(QV4::Object *obj) const
    {
        return objectRefs.value(obj, -1);
    }

private:
    QJsonArray refsToInclude;
    QHash<int, QJsonValue> refs;
    QHash<QByteArray, int> refCache;
    QJsonArray *destination;
    QSet<int> usedRefs;
    QHash<int, QSet<int> > refsByHandle;
    QHash<QV4::Object *, int> objectRefs;
};

int QV4DebugService::debuggerIndex = 0;
int QV4DebugService::sequence = 0;

class V8CommandHandler
{
public:
    V8CommandHandler(const QString &command)
        : cmd(command)
    {}

    virtual ~V8CommandHandler()
    {}

    QString command() const { return cmd; }

    void handle(const QJsonObject &request, QV4DebugService *s)
    {
        TRACE_PROTOCOL(qDebug() << "handling command" << command() << "...");

        req = request;
        seq = req.value(QStringLiteral("seq"));
        debugService = s;

        handleRequest();
        if (!response.isEmpty()) {
            response[QLatin1String("type")] = QStringLiteral("response");
            debugService->send(response);
        }

        debugService = 0;
        seq = QJsonValue();
        req = QJsonObject();
        response = QJsonObject();
    }

    virtual void handleRequest() = 0;

protected:
    void addCommand() { response.insert(QStringLiteral("command"), cmd); }
    void addRequestSequence() { response.insert(QStringLiteral("request_seq"), seq); }
    void addSuccess(bool success) { response.insert(QStringLiteral("success"), success); }
    void addBody(const QJsonObject &body)
    {
        response.insert(QStringLiteral("body"), body);
    }

    void addRunning()
    {
        response.insert(QStringLiteral("running"), debugService->debuggerAgent.isRunning());
    }

    void addRefs()
    {
        response.insert(QStringLiteral("refs"), debugService->buildRefs());
    }

    void createErrorResponse(const QString &msg)
    {
        QJsonValue command = req.value(QStringLiteral("command"));
        response.insert(QStringLiteral("command"), command);
        addRequestSequence();
        addSuccess(false);
        addRunning();
        response.insert(QStringLiteral("message"), msg);
    }

    int requestSequenceNr() const
    { return seq.toInt(-1); }

protected:
    QString cmd;
    QJsonObject req;
    QJsonValue seq;
    QV4DebugService *debugService;
    QJsonObject response;
};

class UnknownV8CommandHandler: public V8CommandHandler
{
public:
    UnknownV8CommandHandler(): V8CommandHandler(QString()) {}

    virtual void handleRequest()
    {
        QString msg = QStringLiteral("unimplemented command \"");
        msg += req.value(QStringLiteral("command")).toString();
        msg += QStringLiteral("\"");
        createErrorResponse(msg);
    }
};

namespace {
class V8VersionRequest: public V8CommandHandler
{
public:
    V8VersionRequest(): V8CommandHandler(QStringLiteral("version")) {}

    virtual void handleRequest()
    {
        addCommand();
        addRequestSequence();
        addSuccess(true);
        addRunning();
        QJsonObject body;
        body.insert(QStringLiteral("V8Version"),
                    QLatin1String("this is not V8, this is V4 in Qt " QT_VERSION_STR));
        addBody(body);
    }
};

class V8SetBreakPointRequest: public V8CommandHandler
{
public:
    V8SetBreakPointRequest(): V8CommandHandler(QStringLiteral("setbreakpoint")) {}

    virtual void handleRequest()
    {
        // decypher the payload:
        QJsonObject args = req.value(QStringLiteral("arguments")).toObject();
        if (args.isEmpty())
            return;

        QString type = args.value(QStringLiteral("type")).toString();
        if (type != QStringLiteral("scriptRegExp")) {
            createErrorResponse(QStringLiteral("breakpoint type \"%1\" is not implemented").arg(type));
            return;
        }

        QString fileName = args.value(QStringLiteral("target")).toString();
        if (fileName.isEmpty()) {
            createErrorResponse(QStringLiteral("breakpoint has no file name"));
            return;
        }

        int line = args.value(QStringLiteral("line")).toInt(-1);
        if (line < 0) {
            createErrorResponse(QStringLiteral("breakpoint has an invalid line number"));
            return;
        }

        bool enabled = args.value(QStringLiteral("enabled")).toBool(true);
        QString condition = args.value(QStringLiteral("condition")).toString();

        // set the break point:
        int id = debugService->debuggerAgent.addBreakPoint(fileName, line + 1, enabled, condition);

        // response:
        addCommand();
        addRequestSequence();
        addSuccess(true);
        addRunning();
        QJsonObject body;
        body.insert(QStringLiteral("type"), type);
        body.insert(QStringLiteral("breakpoint"), id);
        // It's undocumented, but V8 sends back an actual_locations array too. However, our
        // Debugger currently doesn't tell us when it resolved a breakpoint, so we'll leave them
        // pending until the breakpoint is hit for the first time.
        addBody(body);
    }
};

class V8ClearBreakPointRequest: public V8CommandHandler
{
public:
    V8ClearBreakPointRequest(): V8CommandHandler(QStringLiteral("clearbreakpoint")) {}

    virtual void handleRequest()
    {
        // decypher the payload:
        QJsonObject args = req.value(QStringLiteral("arguments")).toObject();
        if (args.isEmpty())
            return;

        int id = args.value(QStringLiteral("breakpoint")).toInt(-1);
        if (id < 0) {
            createErrorResponse(QStringLiteral("breakpoint has an invalid number"));
            return;
        }

        // remove the break point:
        debugService->debuggerAgent.removeBreakPoint(id);

        // response:
        addCommand();
        addRequestSequence();
        addSuccess(true);
        addRunning();
        QJsonObject body;
        body.insert(QStringLiteral("type"), QStringLiteral("scriptRegExp"));
        body.insert(QStringLiteral("breakpoint"), id);
        addBody(body);
    }
};

class V8BacktraceRequest: public V8CommandHandler
{
public:
    V8BacktraceRequest(): V8CommandHandler(QStringLiteral("backtrace")) {}

    virtual void handleRequest()
    {
        // decypher the payload:

        QJsonObject arguments = req.value(QStringLiteral("arguments")).toObject();
        int fromFrame = arguments.value(QStringLiteral("fromFrame")).toInt(0);
        int toFrame = arguments.value(QStringLiteral("toFrame")).toInt(fromFrame + 10);
        // no idea what the bottom property is for, so we'll ignore it.

        QV4::Debugging::Debugger *debugger = debugService->debuggerAgent.firstDebugger();

        QJsonArray frameArray;
        QVector<QV4::StackFrame> frames = debugger->stackTrace(toFrame);
        for (int i = fromFrame; i < toFrame && i < frames.size(); ++i)
            frameArray.push_back(debugService->buildFrame(frames[i], i, debugger));

        // response:
        addCommand();
        addRequestSequence();
        addSuccess(true);
        addRunning();
        QJsonObject body;
        if (frameArray.isEmpty()) {
            body.insert(QStringLiteral("totalFrames"), 0);
        } else {
            body.insert(QStringLiteral("fromFrame"), fromFrame);
            body.insert(QStringLiteral("toFrame"), fromFrame + frameArray.size());
            body.insert(QStringLiteral("frames"), frameArray);
        }
        addBody(body);
        addRefs();
    }
};

class V8FrameRequest: public V8CommandHandler
{
public:
    V8FrameRequest(): V8CommandHandler(QStringLiteral("frame")) {}

    virtual void handleRequest()
    {
        // decypher the payload:
        QJsonObject arguments = req.value(QStringLiteral("arguments")).toObject();
        const int frameNr = arguments.value(QStringLiteral("number")).toInt(
                    debugService->selectedFrame());

        QV4::Debugging::Debugger *debugger = debugService->debuggerAgent.firstDebugger();
        QVector<QV4::StackFrame> frames = debugger->stackTrace(frameNr + 1);
        if (frameNr < 0 || frameNr >= frames.size()) {
            createErrorResponse(QStringLiteral("frame command has invalid frame number"));
            return;
        }

        debugService->selectFrame(frameNr);
        QJsonObject frame = debugService->buildFrame(frames[frameNr], frameNr, debugger);

        // response:
        addCommand();
        addRequestSequence();
        addSuccess(true);
        addRunning();
        addBody(frame);
        addRefs();
    }
};

class V8ScopeRequest: public V8CommandHandler
{
public:
    V8ScopeRequest(): V8CommandHandler(QStringLiteral("scope")) {}

    virtual void handleRequest()
    {
        // decypher the payload:
        QJsonObject arguments = req.value(QStringLiteral("arguments")).toObject();
        const int frameNr = arguments.value(QStringLiteral("frameNumber")).toInt(
                    debugService->selectedFrame());
        const int scopeNr = arguments.value(QStringLiteral("number")).toInt(0);

        QV4::Debugging::Debugger *debugger = debugService->debuggerAgent.firstDebugger();
        QVector<QV4::StackFrame> frames = debugger->stackTrace(frameNr + 1);
        if (frameNr < 0 || frameNr >= frames.size()) {
            createErrorResponse(QStringLiteral("scope command has invalid frame number"));
            return;
        }
        if (scopeNr < 0) {
            createErrorResponse(QStringLiteral("scope command has invalid scope number"));
            return;
        }

        QJsonObject scope = debugService->buildScope(frameNr, scopeNr, debugger);

        // response:
        addCommand();
        addRequestSequence();
        addSuccess(true);
        addRunning();
        addBody(scope);
        addRefs();
    }
};

class V8LookupRequest: public V8CommandHandler
{
public:
    V8LookupRequest(): V8CommandHandler(QStringLiteral("lookup")) {}

    virtual void handleRequest()
    {
        // decypher the payload:
        QJsonObject arguments = req.value(QStringLiteral("arguments")).toObject();
        QJsonArray handles = arguments.value(QStringLiteral("handles")).toArray();

        QJsonObject body;
        foreach (const QJsonValue &handle, handles)
            body[QString::number(handle.toInt())] = debugService->lookup(handle.toInt());

        // response:
        addCommand();
        addRequestSequence();
        addSuccess(true);
        addRunning();
        addBody(body);
        addRefs();
    }
};

class V8ContinueRequest: public V8CommandHandler
{
public:
    V8ContinueRequest(): V8CommandHandler(QStringLiteral("continue")) {}

    virtual void handleRequest()
    {
        // decypher the payload:
        QJsonObject arguments = req.value(QStringLiteral("arguments")).toObject();

        QV4::Debugging::Debugger *debugger = debugService->debuggerAgent.firstDebugger();

        if (arguments.empty()) {
            debugger->resume(QV4::Debugging::Debugger::FullThrottle);
        } else {
            QJsonObject arguments = req.value(QStringLiteral("arguments")).toObject();
            QString stepAction = arguments.value(QStringLiteral("stepaction")).toString();
            const int stepcount = arguments.value(QStringLiteral("stepcount")).toInt(1);
            if (stepcount != 1)
                qWarning() << "Step count other than 1 is not supported.";

            if (stepAction == QStringLiteral("in")) {
                debugger->resume(QV4::Debugging::Debugger::StepIn);
            } else if (stepAction == QStringLiteral("out")) {
                debugger->resume(QV4::Debugging::Debugger::StepOut);
            } else if (stepAction == QStringLiteral("next")) {
                debugger->resume(QV4::Debugging::Debugger::StepOver);
            } else {
                createErrorResponse(QStringLiteral("continue command has invalid stepaction"));
                return;
            }
        }

        // response:
        addCommand();
        addRequestSequence();
        addSuccess(true);
        addRunning();
    }
};

class V8DisconnectRequest: public V8CommandHandler
{
public:
    V8DisconnectRequest(): V8CommandHandler(QStringLiteral("disconnect")) {}

    virtual void handleRequest()
    {
        debugService->debuggerAgent.removeAllBreakPoints();
        debugService->debuggerAgent.resumeAll();

        // response:
        addCommand();
        addRequestSequence();
        addSuccess(true);
        addRunning();
    }
};

class V8SetExceptionBreakRequest: public V8CommandHandler
{
public:
    V8SetExceptionBreakRequest(): V8CommandHandler(QStringLiteral("setexceptionbreak")) {}

    virtual void handleRequest()
    {
        bool wasEnabled = debugService->debuggerAgent.breakOnThrow();

        //decypher the payload:
        QJsonObject arguments = req.value(QStringLiteral("arguments")).toObject();
        QString type = arguments.value(QStringLiteral("type")).toString();
        bool enabled = arguments.value(QStringLiteral("number")).toBool(!wasEnabled);

        if (type == QStringLiteral("all")) {
            // that's fine
        } else if (type == QStringLiteral("uncaught")) {
            createErrorResponse(QStringLiteral("breaking only on uncaught exceptions is not supported yet"));
            return;
        } else {
            createErrorResponse(QStringLiteral("invalid type for break on exception"));
            return;
        }

        // do it:
        debugService->debuggerAgent.setBreakOnThrow(enabled);

        QJsonObject body;
        body[QLatin1String("type")] = type;
        body[QLatin1String("enabled")] = debugService->debuggerAgent.breakOnThrow();

        // response:
        addBody(body);
        addRunning();
        addSuccess(true);
        addRequestSequence();
        addCommand();
    }
};

class V8ScriptsRequest: public V8CommandHandler
{
public:
    V8ScriptsRequest(): V8CommandHandler(QStringLiteral("scripts")) {}

    virtual void handleRequest()
    {
        //decypher the payload:
        QJsonObject arguments = req.value(QStringLiteral("arguments")).toObject();
        int types = arguments.value(QStringLiteral("types")).toInt(-1);
        if (types < 0 || types > 7) {
            createErrorResponse(QStringLiteral("invalid types value in scripts command"));
            return;
        } else if (types != 4) {
            createErrorResponse(QStringLiteral("unsupported types value in scripts command"));
            return;
        }

        // do it:
        debugService->debuggerAgent.firstDebugger()->gatherSources(requestSequenceNr());

        // response will be send by
    }
};

// Request:
// {
//   "seq": 4,
//   "type": "request",
//   "command": "evaluate",
//   "arguments": {
//     "expression": "a",
//     "frame": 0
//   }
// }
//
// Response:
// {
//   "body": {
//     "handle": 3,
//     "type": "number",
//     "value": 1
//   },
//   "command": "evaluate",
//   "refs": [],
//   "request_seq": 4,
//   "running": false,
//   "seq": 5,
//   "success": true,
//   "type": "response"
// }
//
// The "value" key in "body" is the result of evaluating the expression in the request.
class V8EvaluateRequest: public V8CommandHandler
{
public:
    V8EvaluateRequest(): V8CommandHandler(QStringLiteral("evaluate")) {}

    virtual void handleRequest()
    {
        //decypher the payload:
        QJsonObject arguments = req.value(QStringLiteral("arguments")).toObject();
        QString expression = arguments.value(QStringLiteral("expression")).toString();
        const int frame = arguments.value(QStringLiteral("frame")).toInt(0);

        QV4::Debugging::Debugger *debugger = debugService->debuggerAgent.firstDebugger();
        Q_ASSERT(debugger->state() == QV4::Debugging::Debugger::Paused);

        VariableCollector *collector = debugService->collector();
        QJsonArray dest;
        collector->setDestination(&dest);
        debugger->evaluateExpression(frame, expression, collector);

        const int ref = dest.at(0).toObject().value(QStringLiteral("value")).toObject()
                .value(QStringLiteral("ref")).toInt();

        // response:
        addCommand();
        addRequestSequence();
        addSuccess(true);
        addRunning();
        addBody(collector->lookup(ref).toObject());
        addRefs();
    }
};
} // anonymous namespace

void QV4DebugService::addHandler(V8CommandHandler* handler)
{
    handlers[handler->command()] = handler;
}

V8CommandHandler *QV4DebugService::v8CommandHandler(const QString &command) const
{
    V8CommandHandler *handler = handlers.value(command, 0);
    if (handler)
        return handler;
    else
        return unknownV8CommandHandler.data();
}

QV4DebugService::QV4DebugService(QObject *parent) :
    QQmlConfigurableDebugService(QStringLiteral("V8Debugger"), 1, parent), debuggerAgent(this),
    version(1), theSelectedFrame(0), unknownV8CommandHandler(new UnknownV8CommandHandler)
{
    addHandler(new V8VersionRequest);
    addHandler(new V8SetBreakPointRequest);
    addHandler(new V8ClearBreakPointRequest);
    addHandler(new V8BacktraceRequest);
    addHandler(new V8FrameRequest);
    addHandler(new V8ScopeRequest);
    addHandler(new V8LookupRequest);
    addHandler(new V8ContinueRequest);
    addHandler(new V8DisconnectRequest);
    addHandler(new V8SetExceptionBreakRequest);
    addHandler(new V8ScriptsRequest);
    addHandler(new V8EvaluateRequest);
}

QV4DebugService::~QV4DebugService()
{
    qDeleteAll(handlers);
}

QV4DebugService *QV4DebugService::instance()
{
    return v4ServiceInstance();
}

void QV4DebugService::engineAboutToBeAdded(QQmlEngine *engine)
{
    QMutexLocker lock(&m_configMutex);
    if (engine) {
        QV4::ExecutionEngine *ee = QV8Engine::getV4(engine->handle());
        if (QQmlDebugConnector *server = QQmlDebugConnector::instance()) {
            if (ee) {
                ee->enableDebugger();
                QV4::Debugging::Debugger *debugger = ee->debugger;
                debuggerMap.insert(debuggerIndex++, debugger);
                debuggerAgent.addDebugger(debugger);
                debuggerAgent.moveToThread(server->thread());
                moveToThread(server->thread());
            }
        }
    }
    QQmlConfigurableDebugService::engineAboutToBeAdded(engine);
}

void QV4DebugService::engineAboutToBeRemoved(QQmlEngine *engine)
{
    QMutexLocker lock(&m_configMutex);
    if (engine){
        const QV4::ExecutionEngine *ee = QV8Engine::getV4(engine->handle());
        if (ee) {
            QV4::Debugging::Debugger *debugger = ee->debugger;
            typedef QMap<int, QV4::Debugging::Debugger *>::const_iterator DebuggerMapIterator;
            const DebuggerMapIterator end = debuggerMap.constEnd();
            for (DebuggerMapIterator i = debuggerMap.constBegin(); i != end; ++i) {
                if (i.value() == debugger) {
                    debuggerMap.remove(i.key());
                    break;
                }
            }
            debuggerAgent.removeDebugger(debugger);
        }
    }
    QQmlConfigurableDebugService::engineAboutToBeRemoved(engine);
}

void QV4DebugService::signalEmitted(const QString &signal)
{
    //This function is only called by QQmlBoundSignal
    //only if there is a slot connected to the signal. Hence, there
    //is no need for additional check.

    //Parse just the name and remove the class info
    //Normalize to Lower case.
    QString signalName = signal.left(signal.indexOf(QLatin1Char('('))).toLower();

    foreach (const QString &signal, breakOnSignals) {
        if (signal == signalName) {
            // TODO: pause debugger
            break;
        }
    }
}

void QV4DebugService::messageReceived(const QByteArray &message)
{
    QMutexLocker lock(&m_configMutex);

    QQmlDebugStream ms(message);
    QByteArray header;
    ms >> header;

    TRACE_PROTOCOL(qDebug() << "received message with header" << header);

    if (header == "V8DEBUG") {
        QByteArray type;
        QByteArray payload;
        ms >> type >> payload;
        TRACE_PROTOCOL(qDebug() << "... type:" << type);

        if (type == V4_CONNECT) {
            emit messageToClient(name(), packMessage(type));
            stopWaiting();
        } else if (type == V4_PAUSE) {
            debuggerAgent.pauseAll();
            sendSomethingToSomebody(type);
        } else if (type == V4_BREAK_ON_SIGNAL) {
            QByteArray signal;
            bool enabled;
            ms >> signal >> enabled;
             //Normalize to lower case.
            QString signalName(QString::fromUtf8(signal).toLower());
            if (enabled)
                breakOnSignals.append(signalName);
            else
                breakOnSignals.removeOne(signalName);
        } else if (type == "v8request") {
            handleV8Request(payload);
        } else if (type == V4_DISCONNECT) {
            TRACE_PROTOCOL(qDebug() << "... payload:" << payload.constData());
            handleV8Request(payload);
        } else {
            sendSomethingToSomebody(type, 0);
        }
    }
}

void QV4DebugService::sendSomethingToSomebody(const char *type, int magicNumber)
{
    QByteArray response;
    QQmlDebugStream rs(&response, QIODevice::WriteOnly);
    rs << QByteArray(type)
       << QByteArray::number(version) << QByteArray::number(magicNumber);
    emit messageToClient(name(), packMessage(type, response));
}

QV4DebuggerAgent::QV4DebuggerAgent(QV4DebugService *debugService)
    : debugService(debugService)
{}

QV4::Debugging::Debugger *QV4DebuggerAgent::firstDebugger() const
{
    // Currently only 1 single engine is supported, so:
    if (m_debuggers.isEmpty())
        return 0;
    else
        return m_debuggers.first();
}

bool QV4DebuggerAgent::isRunning() const
{
    // Currently only 1 single engine is supported, so:
    if (QV4::Debugging::Debugger *debugger = firstDebugger())
        return debugger->state() == QV4::Debugging::Debugger::Running;
    else
        return false;
}

void QV4DebuggerAgent::debuggerPaused(QV4::Debugging::Debugger *debugger, QV4::Debugging::PauseReason reason)
{
    Q_UNUSED(reason);

    debugService->clearHandles(debugger->engine());

    QJsonObject event, body, script;
    event.insert(QStringLiteral("type"), QStringLiteral("event"));

    switch (reason) {
    case QV4::Debugging::Step:
    case QV4::Debugging::PauseRequest:
    case QV4::Debugging::BreakPoint: {
        event.insert(QStringLiteral("event"), QStringLiteral("break"));
        QVector<QV4::StackFrame> frames = debugger->stackTrace(1);
        if (frames.isEmpty())
            break;

        const QV4::StackFrame &topFrame = frames.first();
        body.insert(QStringLiteral("invocationText"), topFrame.function);
        body.insert(QStringLiteral("sourceLine"), topFrame.line - 1);
        if (topFrame.column > 0)
            body.insert(QStringLiteral("sourceColumn"), topFrame.column);
        QJsonArray breakPoints;
        foreach (int breakPointId, breakPointIds(topFrame.source, topFrame.line))
            breakPoints.push_back(breakPointId);
        body.insert(QStringLiteral("breakpoints"), breakPoints);
        script.insert(QStringLiteral("name"), topFrame.source);
    } break;
    case QV4::Debugging::Throwing:
        // TODO: complete this!
        event.insert(QStringLiteral("event"), QStringLiteral("exception"));
        break;
    }

    if (!script.isEmpty())
        body.insert(QStringLiteral("script"), script);
    if (!body.isEmpty())
        event.insert(QStringLiteral("body"), body);
    debugService->send(event);
}

void QV4DebuggerAgent::sourcesCollected(QV4::Debugging::Debugger *debugger, QStringList sources, int requestSequenceNr)
{
    QJsonArray body;
    foreach (const QString &source, sources) {
        QJsonObject src;
        src[QLatin1String("name")] = source;
        src[QLatin1String("scriptType")] = 4;
        body.append(src);
    }

    QJsonObject response;
    response[QLatin1String("success")] = true;
    response[QLatin1String("running")] = debugger->state() == QV4::Debugging::Debugger::Running;
    response[QLatin1String("body")] = body;
    response[QLatin1String("command")] = QStringLiteral("scripts");
    response[QLatin1String("request_seq")] = requestSequenceNr;
    response[QLatin1String("type")] = QStringLiteral("response");
    debugService->send(response);
}

void QV4DebugService::handleV8Request(const QByteArray &payload)
{
    TRACE_PROTOCOL(qDebug() << "v8request, payload:" << payload.constData());

    QJsonDocument request = QJsonDocument::fromJson(payload);
    QJsonObject o = request.object();
    QJsonValue type = o.value(QStringLiteral("type"));
    if (type.toString() == QStringLiteral("request")) {
        QJsonValue command = o.value(QStringLiteral("command"));
        V8CommandHandler *h = v8CommandHandler(command.toString());
        if (h)
            h->handle(o, this);
    }
}

QByteArray QV4DebugService::packMessage(const QByteArray &command, const QByteArray &message)
{
    QByteArray reply;
    QQmlDebugStream rs(&reply, QIODevice::WriteOnly);
    static const QByteArray cmd("V8DEBUG");
    rs << cmd << command << message;
    return reply;
}

void QV4DebugService::send(QJsonObject v8Payload)
{
    v8Payload[QLatin1String("seq")] = sequence++;
    QJsonDocument doc;
    doc.setObject(v8Payload);
#ifdef NO_PROTOCOL_TRACING
    QByteArray responseData = doc.toJson(QJsonDocument::Compact);
#else
    QByteArray responseData = doc.toJson(QJsonDocument::Indented);
#endif

    TRACE_PROTOCOL(qDebug() << "sending response for:" << responseData.constData() << endl);

    emit messageToClient(name(), packMessage("v8message", responseData));
}

void QV4DebugService::clearHandles(QV4::ExecutionEngine *engine)
{
    theCollector.reset(new VariableCollector(engine));
}

QJsonObject QV4DebugService::buildFrame(const QV4::StackFrame &stackFrame, int frameNr,
                                        QV4::Debugging::Debugger *debugger)
{
    QJsonObject frame;
    frame[QLatin1String("index")] = frameNr;
    frame[QLatin1String("debuggerFrame")] = false;
    frame[QLatin1String("func")] = theCollector->addFunctionRef(stackFrame.function);
    frame[QLatin1String("script")] = theCollector->addScriptRef(stackFrame.source);
    frame[QLatin1String("line")] = stackFrame.line - 1;
    if (stackFrame.column >= 0)
        frame[QLatin1String("column")] = stackFrame.column;

    QJsonArray properties;
    theCollector->setDestination(&properties);
    if (debugger->collectThisInContext(theCollector.data(), frameNr)) {
        QJsonObject obj;
        obj[QLatin1String("properties")] = properties;
        frame[QLatin1String("receiver")] = theCollector->addObjectRef(obj, false);
    }

    QJsonArray scopes;
    // Only type and index are used by Qt Creator, so we keep it easy:
    QVector<QV4::Heap::ExecutionContext::ContextType> scopeTypes = debugger->getScopeTypes(frameNr);
    for (int i = 0, ei = scopeTypes.count(); i != ei; ++i) {
        int type = encodeScopeType(scopeTypes[i]);
        if (type == -1)
            continue;

        QJsonObject scope;
        scope[QLatin1String("index")] = i;
        scope[QLatin1String("type")] = type;
        scopes.push_back(scope);
    }
    frame[QLatin1String("scopes")] = scopes;

    return frame;
}

int QV4DebugService::encodeScopeType(QV4::Heap::ExecutionContext::ContextType scopeType)
{
    switch (scopeType) {
    case QV4::Heap::ExecutionContext::Type_GlobalContext:
        return 0;
        break;
    case QV4::Heap::ExecutionContext::Type_CatchContext:
        return 4;
        break;
    case QV4::Heap::ExecutionContext::Type_WithContext:
        return 2;
        break;
    case QV4::Heap::ExecutionContext::Type_SimpleCallContext:
    case QV4::Heap::ExecutionContext::Type_CallContext:
        return 1;
        break;
    case QV4::Heap::ExecutionContext::Type_QmlContext:
    default:
        return -1;
    }
}

QJsonObject QV4DebugService::buildScope(int frameNr, int scopeNr,
                                        QV4::Debugging::Debugger *debugger)
{
    QJsonObject scope;

    QJsonArray properties;
    theCollector->collectScope(&properties, debugger, frameNr, scopeNr);

    QJsonObject anonymous;
    anonymous[QLatin1String("properties")] = properties;

    QVector<QV4::Heap::ExecutionContext::ContextType> scopeTypes = debugger->getScopeTypes(frameNr);
    scope[QLatin1String("type")] = encodeScopeType(scopeTypes[scopeNr]);
    scope[QLatin1String("index")] = scopeNr;
    scope[QLatin1String("frameIndex")] = frameNr;
    scope[QLatin1String("object")] = theCollector->addObjectRef(anonymous, true);

    return scope;
}

QJsonValue QV4DebugService::lookup(int refId) const
{
    return theCollector->lookup(refId);
}

QJsonArray QV4DebugService::buildRefs()
{
    return theCollector->retrieveRefsToInclude();
}

VariableCollector *QV4DebugService::collector() const
{
    return theCollector.data();
}

void QV4DebugService::selectFrame(int frameNr)
{
    theSelectedFrame = frameNr;
}

int QV4DebugService::selectedFrame() const
{
    return theSelectedFrame;
}

QT_END_NAMESPACE
