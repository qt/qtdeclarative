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
    if (event != v8::Break && event != v8::Exception && event != v8::AfterCompile) {
        return;
    }

    QByteArray response(QV8Engine::toStringStatic(message.GetJSON()).toUtf8());

    QV8DebugService *service = QV8DebugService::instance();
    service->debugMessageHandler(response);

    if (event == v8::Break && !message.WillStartRunning()) {
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

    bool initialized;
    QJSEngine *engine;
    v8::Isolate *isolate;
    QList<QDeclarativeEngine *> engines;
    QEventLoop loop;
    QHash<QString,QString> sourcePath;
    QHash<QString,QByteArray> requestCache;
};

QV8DebugService::QV8DebugService(QObject *parent)
    : QDeclarativeDebugService(*(new QV8DebugServicePrivate()), QLatin1String("V8Debugger"), parent)
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

void QV8DebugService::debugMessageHandler(QByteArray message)
{
    sendMessage(packMessage(message));
}

void QV8DebugService::executionStopped()
{
    Q_D(QV8DebugService);

    if (!d->loop.isRunning()) {
        d->loop.exec(QEventLoop::ExcludeUserInputEvents);
    }
}

void QV8DebugService::appendSourcePath(QByteArray message)
{
    Q_D(QV8DebugService);

    QVariantMap msgMap;
    /* Parse the byte string in a separate isolate
    This will ensure that the debug message handler does not
    receive any messages related to this operation */
    {
        v8::Isolate::Scope i_scope(d->isolate);
        QString req(message);
        QJSValue parser = d->engine->evaluate(QLatin1String("JSON.parse"));
        QJSValue out = parser.call(QJSValue(), QJSValueList() << QJSValue(req));
        msgMap = out.toVariant().toMap();
    }

    QString sourcePath(msgMap.value(QLatin1String("body")).toMap().value(QLatin1String("script")).toMap().value(QLatin1String("name")).toString());
    QString fileName(QFileInfo(sourcePath).fileName());

    d->sourcePath.insert(fileName, sourcePath);

    //Check if there are any pending breakpoint requests for this file
    if (d->requestCache.contains(fileName)) {
        QList<QByteArray> list = d->requestCache.values(fileName);
        d->requestCache.remove(fileName);
        foreach (QByteArray request, list) {
            request.replace(fileName.toUtf8(), sourcePath.toUtf8());
            sendDebugMessage(request);
        }
    }
}

void QV8DebugService::messageReceived(const QByteArray &message)
{
    Q_D(QV8DebugService);

    QDataStream ds(message);
    QByteArray command;
    ds >> command;

    if (command == "V8DEBUG") {
        QByteArray request;
        ds >> request;

        QVariantMap reqMap;
        /* Parse the byte string in a separate isolate
        This will ensure that the debug message handler does not
        receive any messages related to this operation */
        {
            v8::Isolate::Scope i_scope(d->isolate);
            QString req(request);
            QJSValue parser = d->engine->evaluate(QLatin1String("JSON.parse"));
            QJSValue out = parser.call(QJSValue(), QJSValueList() << QJSValue(req));
            reqMap = out.toVariant().toMap();
        }

        QString debugCommand(reqMap.value(QLatin1String("command")).toString());

        if (debugCommand == QLatin1String("connect")) {
            d->initialized = true;

        } else if (debugCommand == QLatin1String("interrupt")) {
            v8::Debug::DebugBreak();

        } else {
            bool ok = true;

            if (debugCommand == QLatin1String("setbreakpoint")){
                QVariantMap arguments = reqMap.value(QLatin1String("arguments")).toMap();
                QString type(arguments.value(QLatin1String("type")).toString());

                if (type == QLatin1String("script")) {
                    QString fileName(arguments.value(QLatin1String("target")).toString());

                    //Check if the filepath has been cached
                    if (d->sourcePath.contains(fileName)) {
                        QString filePath = d->sourcePath.value(fileName);
                        request.replace(fileName.toUtf8(), filePath.toUtf8());
                    } else {
                        //Store the setbreakpoint message till filepath is resolved
                        d->requestCache.insertMulti(fileName, request);
                        ok = false;
                    }
                }
            }
            if (ok)
                sendDebugMessage(request);
        }
    }

    QDeclarativeDebugService::messageReceived(message);
}

void QV8DebugService::sendDebugMessage(const QByteArray &msg)
{
    Q_D(QV8DebugService);

    QString message(msg);
    if (d->loop.isRunning()) {
        d->loop.exit();
    }
    v8::Debug::SendCommand(message.utf16(), message.size());
}

QByteArray QV8DebugService::packMessage(QByteArray &message)
{
    QByteArray reply;
    QDataStream rs(&reply, QIODevice::WriteOnly);
    QByteArray cmd("V8DEBUG");
    rs << cmd << message;
    return reply;
}

QT_END_NAMESPACE
