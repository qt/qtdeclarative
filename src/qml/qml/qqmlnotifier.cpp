// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlnotifier_p.h"
#include "qqmlproperty_p.h"
#include <QtCore/qdebug.h>
#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE

typedef void (*Callback)(QQmlNotifierEndpoint *, void **);

void QQmlBoundSignal_callback(QQmlNotifierEndpoint *, void **);
void QQmlJavaScriptExpressionGuard_callback(QQmlNotifierEndpoint *, void **);
void QQmlVMEMetaObjectEndpoint_callback(QQmlNotifierEndpoint *, void **);
void QQmlPropertyGuard_callback(QQmlNotifierEndpoint *, void **);

static Callback QQmlNotifier_callbacks[] = {
    nullptr,
    QQmlBoundSignal_callback,
    QQmlJavaScriptExpressionGuard_callback,
    QQmlVMEMetaObjectEndpoint_callback,
    QQmlPropertyGuard_callback
};

namespace {
    struct NotifyListTraversalData {
        NotifyListTraversalData(QQmlNotifierEndpoint *ep = nullptr)
            : originalSenderPtr(0)
            , disconnectWatch(nullptr)
            , endpoint(ep)
        {}

        qintptr originalSenderPtr;
        qintptr *disconnectWatch;
        QQmlNotifierEndpoint *endpoint;
    };
}

void QQmlNotifier::notify(QQmlData *ddata, int notifierIndex)
{
    if (QQmlNotifierEndpoint *ep = ddata->notify(notifierIndex))
        emitNotify(ep, nullptr);
}

void QQmlNotifier::emitNotify(QQmlNotifierEndpoint *endpoint, void **a)
{
    QVarLengthArray<NotifyListTraversalData> stack;
    while (endpoint) {
        stack.append(NotifyListTraversalData(endpoint));
        endpoint = endpoint->next;
    }

    int i = 0;
    for (; i < stack.size(); ++i) {
        NotifyListTraversalData &data = stack[i];

        if (!data.endpoint->isNotifying()) {
            data.endpoint->startNotifying(&data.originalSenderPtr);
            data.disconnectWatch = &data.originalSenderPtr;
        } else {
            data.disconnectWatch = (qintptr *)(data.endpoint->senderPtr & ~0x1);
        }
    }

    while (--i >= 0) {
        NotifyListTraversalData &data = stack[i];
        if (*data.disconnectWatch) {
            Q_ASSERT(QQmlNotifier_callbacks[data.endpoint->callback]);
            QQmlNotifier_callbacks[data.endpoint->callback](data.endpoint, a);
            if (data.disconnectWatch == &data.originalSenderPtr && data.originalSenderPtr) {
                data.endpoint->stopNotifying(&data.originalSenderPtr);
            }
        }
    }
}

/*! \internal
    \a sourceSignal MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
void QQmlNotifierEndpoint::connect(QObject *source, int sourceSignal, QQmlEngine *engine, bool doNotify)
{
    disconnect();

    Q_ASSERT(engine);
    if (QObjectPrivate::get(source)->threadData.loadRelaxed()->threadId.loadRelaxed() !=
        QObjectPrivate::get(engine)->threadData.loadRelaxed()->threadId.loadRelaxed()) {

        QString sourceName;
        QDebug(&sourceName) << source;
        sourceName = sourceName.left(sourceName.size() - 1);
        QString engineName;
        QDebug(&engineName).nospace() << engine;
        engineName = engineName.left(engineName.size() - 1);

        qFatal("QQmlEngine: Illegal attempt to connect to %s that is in"
               " a different thread than the QML engine %s.", qPrintable(sourceName),
               qPrintable(engineName));
    }

    setSender(qintptr(source));
    this->sourceSignal = sourceSignal;
    QQmlPropertyPrivate::flushSignal(source, sourceSignal);
    QQmlData *ddata = QQmlData::get(source, true);
    ddata->addNotify(sourceSignal, this);
    if (doNotify) {
        needsConnectNotify = doNotify;
        QObjectPrivate * const priv = QObjectPrivate::get(source);
        priv->connectNotify(QMetaObjectPrivate::signal(source->metaObject(), sourceSignal));
    }
}

QT_END_NAMESPACE

