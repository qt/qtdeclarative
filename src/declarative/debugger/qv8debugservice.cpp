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
#include "qv8debug_p.h"
#include "qv8engine_p.h"
#include "qdeclarativeengine_p.h"

#include <QtCore/QEventLoop>
#include <QtCore/QHash>
#include <QtCore/QFileInfo>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QV8DebugService, v8ServiceInstance)

void DebugMessageHandler(const v8::Debug::Message& message)
{
    v8::DebugEvent event = message.GetEvent();

    if (event != v8::Break && event != v8::Exception &&
            event != v8::AfterCompile && event != v8::BeforeCompile) {
        return;
    }

    const QString response(QV8Engine::toStringStatic(
                                  message.GetJSON()));

    QV8DebugService *service = QV8DebugService::instance();
    service->debugMessageHandler(response);

    if ((event == v8::Break || event == v8::Exception) &&
            !message.WillStartRunning()) {
        service->executionStopped();
    } else if (event == v8::AfterCompile) {
        service->appendSourcePath(response);
    } //TODO::v8::Exception
}

class QV8DebugServicePrivate : public QDeclarativeDebugServicePrivate
{
public:
    QV8DebugServicePrivate()
        :initialized(false)
    {
        //Create a new isolate
        isolate = v8::Isolate::New();

        //Enter the isolate and initialize
        v8::Isolate::Scope i_scope(isolate);
        v8::V8::Initialize();

        //Create an instance in the new isolate
        engine = new QJSEngine();
    }

    ~QV8DebugServicePrivate()
    {
        delete engine;
        isolate->Dispose();
    }

    void sendDebugMessage(const QString &message);
    static QByteArray packMessage(const QString &message);

    bool initialized;
    QJSEngine *engine;
    v8::Isolate *isolate;
    QList<QDeclarativeEngine *> engines;
    QEventLoop loop;
    QHash<QString,QString> sourcePath;
    QHash<QString,QString> requestCache;
    QHash<int,QString> eventList;
};

QV8DebugService::QV8DebugService(QObject *parent)
    : QDeclarativeDebugService(*(new QV8DebugServicePrivate()),
                               QLatin1String("V8Debugger"), parent)
{
    Q_D(QV8DebugService);
    v8::Debug::SetMessageHandler2(DebugMessageHandler);
    if (status() == Enabled) {
        // ,block mode, client attached
        while (!d->initialized) {
            waitForMessage();
        }
    }
}

QV8DebugService::~QV8DebugService()
{
}

QV8DebugService *QV8DebugService::instance()
{
    return v8ServiceInstance();
}

void QV8DebugService::addEngine(QDeclarativeEngine *engine)
{
    Q_D(QV8DebugService);
    Q_ASSERT(engine);
    Q_ASSERT(!d->engines.contains(engine));

    d->engines.append(engine);
}

void QV8DebugService::removeEngine(QDeclarativeEngine *engine)
{
    Q_D(QV8DebugService);
    Q_ASSERT(engine);
    Q_ASSERT(d->engines.contains(engine));

    d->engines.removeAll(engine);
}

void QV8DebugService::debugMessageHandler(const QString &message)
{
    sendMessage(QV8DebugServicePrivate::packMessage(message));
}

void QV8DebugService::executionStopped()
{
    Q_D(QV8DebugService);

    if (!d->loop.isRunning()) {
        d->loop.exec(QEventLoop::ExcludeUserInputEvents);
    }
}

void QV8DebugService::appendSourcePath(const QString &message)
{
    Q_D(QV8DebugService);

    QVariantMap msgMap;
    /* Parse the byte string in a separate isolate
    This will ensure that the debug message handler does not
    receive any messages related to this operation */
    {
        v8::Isolate::Scope i_scope(d->isolate);
        QJSValue parser = d->engine->evaluate(QLatin1String("JSON.parse"));
        QJSValue out = parser.call(QJSValue(), QJSValueList() << QJSValue(message));
        msgMap = out.toVariant().toMap();
    }

    const QString sourcePath(msgMap.value(QLatin1String("body")).toMap().value(
                                 QLatin1String("script")).toMap().value(
                                 QLatin1String("name")).toString());
    const QString fileName(QFileInfo(sourcePath).fileName());

    d->sourcePath.insert(fileName, sourcePath);

    //Check if there are any pending breakpoint requests for this file
    if (d->requestCache.contains(fileName)) {
        QList<QString> list = d->requestCache.values(fileName);
        d->requestCache.remove(fileName);
        foreach (QString request, list) {
            request.replace(fileName, sourcePath);
            d->sendDebugMessage(request);
        }
    }
}

void QV8DebugService::signalEmitted(const QString &signal)
{
    //This function is only called by QDeclarativeBoundSignal
    //only if there is a slot connected to the signal. Hence, there
    //is no need for additional check.
    Q_D(QV8DebugService);

    //Parse just the name and remove the class info
    if (d->eventList.key(signal.left(signal.indexOf(QLatin1String("("))))) {
        v8::Debug::DebugBreak();
    }
}

void QV8DebugService::messageReceived(const QByteArray &message)
{
    Q_D(QV8DebugService);

    QDataStream ds(message);
    QByteArray command;
    ds >> command;

    if (command == "V8DEBUG") {
        QString request;
        {
            QByteArray requestArray;
            ds >> requestArray;
            request = QString::fromUtf8(requestArray);
        }

        QVariantMap reqMap;
        /* Parse the byte string in a separate isolate
        This will ensure that the debug message handler does not
        receive any messages related to this operation */
        {
            v8::Isolate::Scope i_scope(d->isolate);
            QJSValue parser = d->engine->evaluate(QLatin1String("JSON.parse"));
            QJSValue out = parser.call(QJSValue(), QJSValueList() << QJSValue(request));
            reqMap = out.toVariant().toMap();
        }

        const QString debugCommand(reqMap.value(QLatin1String("command")).toString());

        if (debugCommand == QLatin1String("connect")) {
            d->initialized = true;

        } else if (debugCommand == QLatin1String("interrupt")) {
            v8::Debug::DebugBreak();

        } else {
            bool forwardRequestToV8 = true;

            if (debugCommand == QLatin1String("setbreakpoint")) {
                const QVariantMap arguments = reqMap.value(QLatin1String("arguments")).toMap();
                const QString type(arguments.value(QLatin1String("type")).toString());

                if (type == QLatin1String("script")) {
                    QString fileName(arguments.value(QLatin1String("target")).toString());

                    //Check if the filepath has been cached
                    if (d->sourcePath.contains(fileName)) {
                        QString filePath = d->sourcePath.value(fileName);
                        request.replace(fileName, filePath);
                    } else {
                        //Store the setbreakpoint message till filepath is resolved
                        d->requestCache.insertMulti(fileName, request);
                        forwardRequestToV8 = false;
                    }
                } else if (type == QLatin1String("event")) {
                    //Do not send this request to v8
                    forwardRequestToV8 = false;

                    //Prepare the response string
                    //Create a json message using v8 debugging protocol
                    //and send it to client

                    // { "seq"         : <number>,
                    //   "type"        : "response",
                    //   "request_seq" : <number>,
                    //   "command"     : "setbreakpoint",
                    //   "body"        : { "type"       : <"function" or "script">
                    //                     "breakpoint" : <break point number of the new break point>
                    //                   }
                    //   "running"     : <is the VM running after sending this response>
                    //   "success"     : true
                    // }
                    {
                        v8::Isolate::Scope(d->isolate);
                        const QString obj(QLatin1String("{}"));
                        QJSValue parser = d->engine->evaluate(QLatin1String("JSON.parse"));
                        QJSValue jsonVal = parser.call(QJSValue(), QJSValueList() << obj);
                        jsonVal.setProperty(QLatin1String("type"), QJSValue(QLatin1String("response")));

                        const int sequence = reqMap.value(QLatin1String("seq")).toInt();
                        jsonVal.setProperty(QLatin1String("request_seq"), QJSValue(sequence));
                        jsonVal.setProperty(QLatin1String("command"), QJSValue(debugCommand));

                        //Check that the function starts with 'on'
                        QString eventName(arguments.value(QLatin1String("target")).toString());


                        if (eventName.startsWith(QLatin1String("on"))) {
                            d->eventList.insert(-sequence, eventName.remove(0,2).toLower());

                            QJSValue args = parser.call(QJSValue(), QJSValueList() << obj);

                            args.setProperty(QLatin1String("type"), QJSValue(QLatin1String("event")));
                            args.setProperty(QLatin1String("breakpoint"), QJSValue(-sequence));

                            jsonVal.setProperty(QLatin1String("body"), args);
                            jsonVal.setProperty(QLatin1String("success"), QJSValue(true));

                        } else {
                            jsonVal.setProperty(QLatin1String("success"), QJSValue(false));
                        }


                        jsonVal.setProperty(QLatin1String("running"), QJSValue(!d->loop.isRunning()));

                        QJSValue stringify = d->engine->evaluate(QLatin1String("JSON.stringify"));
                        QJSValue json = stringify.call(QJSValue(), QJSValueList() << jsonVal);
                        debugMessageHandler(json.toString());

                    }
                }
            } else if (debugCommand == QLatin1String("clearbreakpoint")) {
                //check if the breakpoint is a negative integer (event breakpoint)
                const QVariantMap arguments = reqMap.value(QLatin1String("arguments")).toMap();
                const int bp = arguments.value(QLatin1String("breakpoint")).toInt();

                if (bp < 0) {
                    d->eventList.remove(bp);
                    forwardRequestToV8 = false;
                }
            }
            if (forwardRequestToV8)
                d->sendDebugMessage(request);
        }
    }

    QDeclarativeDebugService::messageReceived(message);
}

void QV8DebugServicePrivate::sendDebugMessage(const QString &message)
{
    if (loop.isRunning())
        loop.exit();

    v8::Debug::SendCommand(message.utf16(), message.size());
}

QByteArray QV8DebugServicePrivate::packMessage(const QString &message)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd("V8DEBUG");
    rs << cmd << message.toUtf8();
    return reply;
}

QT_END_NAMESPACE
