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

#include "qv8include_p.h"

#include <QtQml/qjsengine.h>
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtCore/qfile.h>
#include <QtQml/qqmlfile.h>

#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

QV8Include::QV8Include(const QUrl &url, QV8Engine *engine, QQmlContextData *context,
                       v8::Handle<v8::Object> qmlglobal, v8::Handle<v8::Function> callback)
: m_engine(engine), m_network(0), m_reply(0), m_url(url), m_redirectCount(0), m_context(context)
{
    m_qmlglobal = qPersistentNew<v8::Object>(qmlglobal);
    if (!callback.IsEmpty())
        m_callbackFunction = qPersistentNew<v8::Function>(callback);

    m_resultObject = qPersistentNew<v8::Object>(resultValue());

    m_network = engine->networkAccessManager();

    QNetworkRequest request;
    request.setUrl(url);

    m_reply = m_network->get(request);
    QObject::connect(m_reply, SIGNAL(finished()), this, SLOT(finished()));
}

QV8Include::~QV8Include()
{
    delete m_reply; m_reply = 0;
    qPersistentDispose(m_callbackFunction);
    qPersistentDispose(m_resultObject);
}

v8::Local<v8::Object> QV8Include::resultValue(Status status)
{
    // XXX It seems inefficient to create this object from scratch each time.
    v8::Local<v8::Object> result = v8::Object::New();
    result->Set(v8::String::New("OK"), v8::Integer::New(Ok));
    result->Set(v8::String::New("LOADING"), v8::Integer::New(Loading));
    result->Set(v8::String::New("NETWORK_ERROR"), v8::Integer::New(NetworkError));
    result->Set(v8::String::New("EXCEPTION"), v8::Integer::New(Exception));

    result->Set(v8::String::New("status"), v8::Integer::New(status));

    return result;
}

void QV8Include::callback(QV8Engine *engine, v8::Handle<v8::Function> callback, v8::Handle<v8::Object> status)
{
    if (!callback.IsEmpty()) {
        v8::Handle<v8::Value> args[] = { status };
        v8::TryCatch tc;
        callback->Call(engine->global(), 1, args);
    }
}

v8::Handle<v8::Object> QV8Include::result()
{
    return m_resultObject;
}

#define INCLUDE_MAXIMUM_REDIRECT_RECURSION 15
void QV8Include::finished()
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

    v8::HandleScope handle_scope;

    if (m_reply->error() == QNetworkReply::NoError) {
        QByteArray data = m_reply->readAll();

        QString code = QString::fromUtf8(data);
        QQmlScript::Parser::extractPragmas(code);

        QQmlContextData *importContext = new QQmlContextData;
        importContext->isInternal = true;
        importContext->isJSContext = true;
        importContext->url = m_url;
        importContext->isPragmaLibraryContext = m_context->isPragmaLibraryContext;
        importContext->setParent(m_context, true);

        v8::Context::Scope ctxtscope(m_engine->context());
        v8::TryCatch try_catch;

        v8::Local<v8::Script> script = m_engine->qmlModeCompile(code, m_url.toString());

        if (!try_catch.HasCaught()) {
            m_engine->contextWrapper()->addSubContext(m_qmlglobal, script, importContext);
            script->Run(m_qmlglobal);
        }

        if (try_catch.HasCaught()) {
            m_resultObject->Set(v8::String::New("status"), v8::Integer::New(Exception));
            m_resultObject->Set(v8::String::New("exception"), try_catch.Exception());
        } else {
            m_resultObject->Set(v8::String::New("status"), v8::Integer::New(Ok));
        }
    } else {
        m_resultObject->Set(v8::String::New("status"), v8::Integer::New(NetworkError));
    }

    callback(m_engine, m_callbackFunction, m_resultObject);

    disconnect();
    deleteLater();
}

/*
    Documented in qv8engine.cpp
*/
v8::Handle<v8::Value> QV8Include::include(const v8::Arguments &args)
{
    if (args.Length() == 0)
        return v8::Undefined();

    QV8Engine *engine = V8ENGINE();
    QQmlContextData *context = engine->callingContext();

    if (!context || !context->isJSContext) 
        V8THROW_ERROR("Qt.include(): Can only be called from JavaScript files");

    QUrl url(context->resolvedUrl(QUrl(engine->toString(args[0]->ToString()))));
    
    v8::Local<v8::Function> callbackFunction;
    if (args.Length() >= 2 && args[1]->IsFunction())
        callbackFunction = v8::Local<v8::Function>::Cast(args[1]);

    QString localFile = QQmlFile::urlToLocalFileOrQrc(url);

    v8::Local<v8::Object> result;

    if (localFile.isEmpty()) {

        QV8Include *i = new QV8Include(url, engine, context, 
                                       v8::Context::GetCallingQmlGlobal(), 
                                       callbackFunction);
        result = v8::Local<v8::Object>::New(i->result());

    } else { 

        QFile f(localFile);

        if (f.open(QIODevice::ReadOnly)) {
            QByteArray data = f.readAll();
            QString code = QString::fromUtf8(data);
            QQmlScript::Parser::extractPragmas(code);

            QQmlContextData *importContext = new QQmlContextData;
            importContext->isInternal = true;
            importContext->isJSContext = true;
            importContext->url = url;
            importContext->setParent(context, true);

            v8::TryCatch try_catch;

            v8::Local<v8::Script> script = engine->qmlModeCompile(code, url.toString());

            if (!try_catch.HasCaught()) {
                v8::Local<v8::Object> qmlglobal = v8::Context::GetCallingQmlGlobal();
                engine->contextWrapper()->addSubContext(qmlglobal, script, importContext);
                script->Run(qmlglobal);
            }

            if (try_catch.HasCaught()) {
                result = resultValue(Exception);
                result->Set(v8::String::New("exception"), try_catch.Exception());
            } else {
                result = resultValue(Ok);
            }

        } else {
            result = resultValue(NetworkError);
        }

        callback(engine, callbackFunction, result);
    }

    if (result.IsEmpty())
        return v8::Undefined();
    else 
        return result;
}

QT_END_NAMESPACE
