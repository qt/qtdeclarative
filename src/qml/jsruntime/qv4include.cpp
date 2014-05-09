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

#include "qv4include_p.h"
#include "qv4scopedvalue_p.h"

#include <QtQml/qjsengine.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtCore/qfile.h>
#include <QtQml/qqmlfile.h>

#include <private/qqmlengine_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4script_p.h>
#include <private/qv4context_p.h>
#include <private/qqmlcontextwrapper_p.h>

QT_BEGIN_NAMESPACE

QV4Include::QV4Include(const QUrl &url, QV8Engine *engine, QQmlContextData *context,
                       const QV4::ValueRef qmlglobal, const QV4::ValueRef callback)
    : v4(QV8Engine::getV4(engine)), m_network(0), m_reply(0), m_url(url), m_redirectCount(0), m_context(context)
{
    m_qmlglobal = qmlglobal;
    if (callback->asFunctionObject())
        m_callbackFunction = callback;

    m_resultObject = resultValue(v4);

    m_network = engine->networkAccessManager();

    QNetworkRequest request;
    request.setUrl(url);

    m_reply = m_network->get(request);
    QObject::connect(m_reply, SIGNAL(finished()), this, SLOT(finished()));
}

QV4Include::~QV4Include()
{
    delete m_reply; m_reply = 0;
}

QV4::ReturnedValue QV4Include::resultValue(QV4::ExecutionEngine *v4, Status status)
{
    QV4::Scope scope(v4);

    // XXX It seems inefficient to create this object from scratch each time.
    QV4::ScopedObject o(scope, v4->newObject());
    QV4::ScopedString s(scope);
    QV4::ScopedValue v(scope);
    o->put((s = v4->newString(QStringLiteral("OK"))), (v = QV4::Primitive::fromInt32(Ok)));
    o->put((s = v4->newString(QStringLiteral("LOADING"))), (v = QV4::Primitive::fromInt32(Loading)));
    o->put((s = v4->newString(QStringLiteral("NETWORK_ERROR"))), (v = QV4::Primitive::fromInt32(NetworkError)));
    o->put((s = v4->newString(QStringLiteral("EXCEPTION"))), (v = QV4::Primitive::fromInt32(Exception)));
    o->put((s = v4->newString(QStringLiteral("status"))), (v = QV4::Primitive::fromInt32(status)));

    return o.asReturnedValue();
}

void QV4Include::callback(const QV4::ValueRef callback, const QV4::ValueRef status)
{
    QV4::ExecutionEngine *v4 = callback->engine();
    if (!v4)
        return;
    QV4::Scope scope(v4);
    QV4::ScopedFunctionObject f(scope, callback);
    if (!f)
        return;

    QV4::ExecutionContext *ctx = v4->currentContext();
    QV4::ScopedCallData callData(scope, 1);
    callData->thisObject = v4->globalObject->asReturnedValue();
    callData->args[0] = status;
    f->call(callData);
    if (scope.hasException())
        ctx->catchException();
}

QV4::ReturnedValue QV4Include::result()
{
    return m_resultObject.value();
}

#define INCLUDE_MAXIMUM_REDIRECT_RECURSION 15
void QV4Include::finished()
{
    m_redirectCount++;

    if (m_redirectCount < INCLUDE_MAXIMUM_REDIRECT_RECURSION) {
        QVariant redirect = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            m_url = m_url.resolved(redirect.toUrl());
            delete m_reply;

            QNetworkRequest request;
            request.setUrl(m_url);

            m_reply = m_network->get(request);
            QObject::connect(m_reply, SIGNAL(finished()), this, SLOT(finished()));
            return;
        }
    }

    QV4::Scope scope(v4);
    QV4::ScopedObject resultObj(scope, m_resultObject.value());
    if (m_reply->error() == QNetworkReply::NoError) {
        QByteArray data = m_reply->readAll();

        QString code = QString::fromUtf8(data);
        QmlIR::Document::removeScriptPragmas(code);

        QV4::ScopedObject qmlglobal(scope, m_qmlglobal.value());
        QV4::Script script(v4, qmlglobal, code, m_url.toString());

        QV4::ExecutionContext *ctx = v4->currentContext();
        QV4::ScopedString status(scope, v4->newString(QStringLiteral("status")));
        script.parse();
        if (!scope.engine->hasException)
            script.run();
        if (scope.engine->hasException) {
            QV4::ScopedValue ex(scope, ctx->catchException());
            resultObj->put(status, QV4::ScopedValue(scope, QV4::Primitive::fromInt32(Exception)));
            resultObj->put(QV4::ScopedString(scope, v4->newString(QStringLiteral("exception"))), ex);
        } else {
            resultObj->put(status, QV4::ScopedValue(scope, QV4::Primitive::fromInt32(Ok)));
        }
    } else {
        resultObj->put(QV4::ScopedString(scope, v4->newString(QStringLiteral("status"))), QV4::ScopedValue(scope, QV4::Primitive::fromInt32(NetworkError)));
    }

    QV4::ScopedValue cb(scope, m_callbackFunction.value());
    callback(cb, resultObj);

    disconnect();
    deleteLater();
}

/*
    Documented in qv8engine.cpp
*/
QV4::ReturnedValue QV4Include::method_include(QV4::CallContext *ctx)
{
    if (!ctx->callData->argc)
        return QV4::Encode::undefined();

    QV4::ExecutionEngine *v4 = ctx->engine;
    QV4::Scope scope(v4);
    QV8Engine *engine = v4->v8Engine;
    QQmlContextData *context = QV4::QmlContextWrapper::callingContext(v4);

    if (!context || !context->isJSContext)
        V4THROW_ERROR("Qt.include(): Can only be called from JavaScript files");

    QUrl url(ctx->engine->resolvedUrl(ctx->callData->args[0].toQStringNoThrow()));

    QV4::ScopedValue callbackFunction(scope, QV4::Primitive::undefinedValue());
    if (ctx->callData->argc >= 2 && ctx->callData->args[1].asFunctionObject())
        callbackFunction = ctx->callData->args[1];

    QString localFile = QQmlFile::urlToLocalFileOrQrc(url);

    QV4::ScopedValue result(scope);
    QV4::ScopedObject qmlcontextobject(scope, v4->qmlContextObject());

    if (localFile.isEmpty()) {
        QV4Include *i = new QV4Include(url, engine, context,
                                       qmlcontextobject,
                                       callbackFunction);
        result = i->result();

    } else {

        QFile f(localFile);

        if (f.open(QIODevice::ReadOnly)) {
            QByteArray data = f.readAll();
            QString code = QString::fromUtf8(data);
            QmlIR::Document::removeScriptPragmas(code);

            QV4::Script script(v4, qmlcontextobject, code, url.toString());

            QV4::ExecutionContext *ctx = v4->currentContext();
            script.parse();
            if (!v4->hasException)
                script.run();
            if (v4->hasException) {
                QV4::ScopedValue ex(scope, ctx->catchException());
                result = resultValue(v4, Exception);
                result->asObject()->put(QV4::ScopedString(scope, v4->newString(QStringLiteral("exception"))), ex);
            } else {
                result = resultValue(v4, Ok);
            }
        } else {
            result = resultValue(v4, NetworkError);
        }

        callback(callbackFunction, result);
    }

    return result.asReturnedValue();
}

QT_END_NAMESPACE
