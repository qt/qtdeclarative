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

QT_BEGIN_NAMESPACE

QV4Include::QV4Include(const QUrl &url, QV8Engine *engine, QQmlContextData *context,
                       const QV4::Value &qmlglobal, const QV4::Value &callback)
    : v4(QV8Engine::getV4(engine)), m_network(0), m_reply(0), m_url(url), m_redirectCount(0), m_context(context)
{
    m_qmlglobal = qmlglobal;
    if (callback.asFunctionObject())
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

QV4::Value QV4Include::resultValue(QV4::ExecutionEngine *v4, Status status)
{

    // XXX It seems inefficient to create this object from scratch each time.
    QV4::Object *o = v4->newObject();
    o->put(v4->newString("OK"), QV4::Value::fromInt32(Ok));
    o->put(v4->newString("LOADING"), QV4::Value::fromInt32(Loading));
    o->put(v4->newString("NETWORK_ERROR"), QV4::Value::fromInt32(NetworkError));
    o->put(v4->newString("EXCEPTION"), QV4::Value::fromInt32(Exception));

    o->put(v4->newString("status"), QV4::Value::fromInt32(status));

    return QV4::Value::fromObject(o);
}

void QV4Include::callback(const QV4::Value &callback, const QV4::Value &status)
{
    QV4::FunctionObject *f = callback.asFunctionObject();
    if (!f)
        return;

    QV4::Value args[] = { status };
    QV4::ExecutionContext *ctx = f->engine()->current;
    try {
        f->call(QV4::Value::fromObject(f->engine()->globalObject), args, 1);
    } catch (QV4::Exception &e) {
        e.accept(ctx);
    }
}

QV4::Value QV4Include::result()
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

    if (m_reply->error() == QNetworkReply::NoError) {
        QByteArray data = m_reply->readAll();

        QString code = QString::fromUtf8(data);
        QQmlScript::Parser::extractPragmas(code);

        QV4::Script script(v4, m_qmlglobal.value().asObject(), code, m_url.toString());

        QV4::ExecutionContext *ctx = v4->current;
        QV4::Object *o = m_resultObject.value().asObject();
        try {
            script.parse();
            script.run();
            o->put(v4->newString("status"), QV4::Value::fromInt32(Ok));
        } catch (QV4::Exception &e) {
            e.accept(ctx);
            o->put(v4->newString("status"), QV4::Value::fromInt32(Exception));
            o->put(v4->newString("exception"), e.value());
        }
    } else {
        m_resultObject.value().asObject()->put(v4->newString("status"), QV4::Value::fromInt32(NetworkError));
    }

    callback(m_callbackFunction.value(), m_resultObject.value());

    disconnect();
    deleteLater();
}

/*
    Documented in qv8engine.cpp
*/
QV4::Value QV4Include::include(QV4::SimpleCallContext *ctx)
{
    if (!ctx->argumentCount)
        return QV4::Value::undefinedValue();

    QV4::ExecutionEngine *v4 = ctx->engine;
    QV8Engine *engine = v4->publicEngine->handle();
    QQmlContextData *context = engine->callingContext();

    if (!context || !context->isJSContext)
        V4THROW_ERROR("Qt.include(): Can only be called from JavaScript files");

    QUrl url(ctx->engine->resolvedUrl(ctx->arguments[0].toQString()));

    QV4::Value callbackFunction;
    if (ctx->argumentCount >= 2 && ctx->arguments[1].asFunctionObject())
        callbackFunction = ctx->arguments[1];

    QString localFile = QQmlFile::urlToLocalFileOrQrc(url);

    QV4::Value result = QV4::Value::undefinedValue();

    if (localFile.isEmpty()) {

        QV4Include *i = new QV4Include(url, engine, context,
                                       QV4::Value::fromObject(v4->qmlContextObject()),
                                       callbackFunction);
        result = i->result();

    } else { 

        QFile f(localFile);

        if (f.open(QIODevice::ReadOnly)) {
            QByteArray data = f.readAll();
            QString code = QString::fromUtf8(data);
            QQmlScript::Parser::extractPragmas(code);

            QV4::Object *qmlglobal = v4->qmlContextObject();
            QV4::Script script(v4, qmlglobal, code, url.toString());

            QV4::ExecutionContext *ctx = v4->current;
            try {
                script.parse();
                script.run();
                result = resultValue(v4, Ok);
            } catch (QV4::Exception &e) {
                e.accept(ctx);
                result = resultValue(v4, Exception);
                result.asObject()->put(v4->newString("exception"), e.value());
            }
        } else {
            result = resultValue(v4, NetworkError);
        }

        callback(callbackFunction, result);
    }

    return result;
}

QT_END_NAMESPACE
