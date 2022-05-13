// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4include_p.h"
#include "qv4scopedvalue_p.h"
#include "qv4jscall_p.h"

#include <QtQml/qjsengine.h>
#if QT_CONFIG(qml_network)
#include <QtNetwork/qnetworkrequest.h>
#include <QtNetwork/qnetworkreply.h>
#endif
#include <QtCore/qfile.h>
#include <QtQml/qqmlfile.h>

#include <private/qqmlengine_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4script_p.h>
#include <private/qv4context_p.h>

QT_BEGIN_NAMESPACE

QV4Include::QV4Include(const QUrl &url, QV4::ExecutionEngine *engine,
                       QV4::QmlContext *qmlContext, const QV4::Value &callback)
    : QObject(engine->jsEngine())
    , v4(engine), m_url(url)
#if QT_CONFIG(qml_network)
    , m_redirectCount(0), m_network(nullptr) , m_reply(nullptr)
#endif
{
    if (qmlContext)
        m_qmlContext.set(v4, *qmlContext);
    if (callback.as<QV4::FunctionObject>())
        m_callbackFunction.set(v4, callback);

    m_resultObject.set(v4, resultValue(v4));

#if QT_CONFIG(qml_network)
    if (QQmlEngine *qmlEngine = v4->qmlEngine()) {
        m_network = qmlEngine->networkAccessManager();

        QNetworkRequest request;
        request.setUrl(url);

        m_reply = m_network->get(request);
        QObject::connect(m_reply, SIGNAL(finished()), this, SLOT(finished()));
    } else {
        finished();
    }
#else
    finished();
#endif
}

QV4Include::~QV4Include()
{
#if QT_CONFIG(qml_network)
    delete m_reply;
    m_reply = nullptr;
#endif
}

QV4::ReturnedValue QV4Include::resultValue(QV4::ExecutionEngine *v4, Status status,
                                           const QString &statusText)
{
    QV4::Scope scope(v4);

    // XXX It seems inefficient to create this object from scratch each time.
    QV4::ScopedObject o(scope, v4->newObject());
    QV4::ScopedString s(scope);
    QV4::ScopedValue v(scope);
    o->put((s = v4->newString(QStringLiteral("OK"))), (v = QV4::Value::fromInt32(Ok)));
    o->put((s = v4->newString(QStringLiteral("LOADING"))), (v = QV4::Value::fromInt32(Loading)));
    o->put((s = v4->newString(QStringLiteral("NETWORK_ERROR"))), (v = QV4::Value::fromInt32(NetworkError)));
    o->put((s = v4->newString(QStringLiteral("EXCEPTION"))), (v = QV4::Value::fromInt32(Exception)));
    o->put((s = v4->newString(QStringLiteral("status"))), (v = QV4::Value::fromInt32(status)));
    if (!statusText.isEmpty())
        o->put((s = v4->newString(QStringLiteral("statusText"))), (v = v4->newString(statusText)));

    return o.asReturnedValue();
}

void QV4Include::callback(const QV4::Value &callback, const QV4::Value &status)
{
    if (!callback.isObject())
        return;
    QV4::ExecutionEngine *v4 = callback.as<QV4::Object>()->engine();
    QV4::Scope scope(v4);
    QV4::ScopedFunctionObject f(scope, callback);
    if (!f)
        return;

    QV4::JSCallArguments jsCallData(scope, 1);
    *jsCallData.thisObject = v4->globalObject->asReturnedValue();
    jsCallData.args[0] = status;
    f->call(jsCallData);
    if (scope.hasException())
        scope.engine->catchException();
}

QV4::ReturnedValue QV4Include::result()
{
    return m_resultObject.value();
}

#define INCLUDE_MAXIMUM_REDIRECT_RECURSION 15
void QV4Include::finished()
{
#if QT_CONFIG(qml_network)
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
    QV4::ScopedString status(scope, v4->newString(QStringLiteral("status")));
    if (m_reply->error() == QNetworkReply::NoError) {
        QByteArray data = m_reply->readAll();

        QString code = QString::fromUtf8(data);

        QV4::Scoped<QV4::QmlContext> qml(scope, m_qmlContext.value());
        QV4::Script script(v4, qml, /*parse as QML binding*/false, code, m_url.toString());

        script.parse();
        if (!scope.hasException())
            script.run();
        if (scope.hasException()) {
            QV4::ScopedValue ex(scope, scope.engine->catchException());
            resultObj->put(status, QV4::ScopedValue(scope, QV4::Value::fromInt32(Exception)));
            QV4::ScopedString exception(scope, v4->newString(QStringLiteral("exception")));
            resultObj->put(exception, ex);
        } else {
            resultObj->put(status, QV4::ScopedValue(scope, QV4::Value::fromInt32(Ok)));
        }
    } else {
        resultObj->put(status, QV4::ScopedValue(scope, QV4::Value::fromInt32(NetworkError)));
    }
#else
    QV4::Scope scope(v4);
    QV4::ScopedObject resultObj(scope, m_resultObject.value());
    QV4::ScopedString status(scope, v4->newString(QStringLiteral("status")));
    resultObj->put(status, QV4::ScopedValue(scope, QV4::Value::fromInt32(NetworkError)));
#endif // qml_network

    QV4::ScopedValue cb(scope, m_callbackFunction.value());
    callback(cb, resultObj);

    disconnect();
    deleteLater();
}

/*
    Documented in qv4engine.cpp
*/
QJSValue QV4Include::method_include(QV4::ExecutionEngine *engine, const QUrl &url,
                                    const QJSValue &callbackFunction)
{
    QQmlRefPointer<QQmlContextData> context = engine->callingQmlContext();

    if ((!context || !context->isJSContext()) && engine->qmlEngine()) {
        return QJSValuePrivate::fromReturnedValue(
                    engine->throwError(
                        QString::fromUtf8(
                            "Qt.include(): Can only be called from JavaScript files")));
    }


    QV4::Scope scope(engine);
    QV4::ScopedValue scopedCallbackFunction(scope, QV4::Value::undefinedValue());
    if (auto function = QJSValuePrivate::asManagedType<QV4::FunctionObject>(&callbackFunction))
        scopedCallbackFunction = *function;

    const QQmlEngine *qmlEngine = engine->qmlEngine();
    const QUrl intercepted = qmlEngine
            ? qmlEngine->interceptUrl(url, QQmlAbstractUrlInterceptor::JavaScriptFile)
            : url;
    QString localFile = QQmlFile::urlToLocalFileOrQrc(intercepted);

    QV4::ScopedValue result(scope);
    QV4::Scoped<QV4::QmlContext> qmlcontext(scope, scope.engine->qmlContext());

    if (localFile.isEmpty()) {
#if QT_CONFIG(qml_network)
        QV4Include *i = new QV4Include(url, engine, qmlcontext, scopedCallbackFunction);
        result = i->result();
#else
        result = resultValue(scope.engine, NetworkError);
        callback(scopedCallbackFunction, result);
#endif
    } else {
        QScopedPointer<QV4::Script> script;
        QString error;
        script.reset(QV4::Script::createFromFileOrCache(scope.engine, qmlcontext, localFile, url, &error));

        if (!script.isNull()) {
            script->parse();
            if (!scope.hasException())
                script->run();
            if (scope.hasException()) {
                QV4::ScopedValue ex(scope, scope.engine->catchException());
                result = resultValue(scope.engine, Exception);
                QV4::ScopedString exception(scope, scope.engine->newString(QStringLiteral("exception")));
                result->as<QV4::Object>()->put(exception, ex);
            } else {
                result = resultValue(scope.engine, Ok);
            }
        } else {
            result = resultValue(scope.engine, NetworkError, error);
        }

        callback(scopedCallbackFunction, result);
    }

    return QJSValuePrivate::fromReturnedValue(result->asReturnedValue());
}

QT_END_NAMESPACE

#include "moc_qv4include_p.cpp"
