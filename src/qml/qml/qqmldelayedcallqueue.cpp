// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldelayedcallqueue_p.h"
#include <private/qqmlengine_p.h>
#include <private/qqmljavascriptexpression_p.h>
#include <private/qv4value_p.h>
#include <private/qv4jscall_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4qmlcontext_p.h>

#include <QQmlError>

QT_BEGIN_NAMESPACE

//
// struct QQmlDelayedCallQueue::DelayedFunctionCall
//

void QQmlDelayedCallQueue::DelayedFunctionCall::execute(QV4::ExecutionEngine *engine) const
{
    if (!m_guarded ||
            (!m_objectGuard.isNull() &&
             !QQmlData::wasDeleted(m_objectGuard) &&
             QQmlData::get(m_objectGuard) &&
             !QQmlData::get(m_objectGuard)->isQueuedForDeletion)) {

        QV4::Scope scope(engine);

        QV4::ArrayObject *array = m_args.as<QV4::ArrayObject>();
        const QV4::FunctionObject *callback = m_function.as<QV4::FunctionObject>();
        Q_ASSERT(callback);
        const int argCount = array ? array->getLength() : 0;
        QV4::JSCallArguments jsCallData(scope, argCount);
        *jsCallData.thisObject = QV4::Encode::undefined();

        for (int i = 0; i < argCount; i++) {
            jsCallData.args[i] = array->get(i);
        }

        callback->call(jsCallData);

        if (scope.hasException()) {
            QQmlError error = scope.engine->catchExceptionAsQmlError();
            error.setDescription(error.description() + QLatin1String(" (exception occurred during delayed function evaluation)"));
            QQmlEnginePrivate::warning(QQmlEnginePrivate::get(scope.engine->qmlEngine()), error);
        }
    }
}

//
// class QQmlDelayedCallQueue
//

QQmlDelayedCallQueue::QQmlDelayedCallQueue()
    : QObject(nullptr), m_engine(nullptr), m_callbackOutstanding(false)
{
}

QQmlDelayedCallQueue::~QQmlDelayedCallQueue()
{
}

void QQmlDelayedCallQueue::init(QV4::ExecutionEngine* engine)
{
    m_engine = engine;

    const QMetaObject &metaObject = QQmlDelayedCallQueue::staticMetaObject;
    int methodIndex = metaObject.indexOfSlot("ticked()");
    m_tickedMethod = metaObject.method(methodIndex);
}

QV4::ReturnedValue QQmlDelayedCallQueue::addUniquelyAndExecuteLater(QV4::ExecutionEngine *engine, QQmlV4Function *args)
{
    QQmlDelayedCallQueue *self = engine->delayedCallQueue();

    QV4::Scope scope(engine);
    if (args->length() == 0)
        THROW_GENERIC_ERROR("Qt.callLater: no arguments given");

    QV4::ScopedValue firstArgument(scope, (*args)[0]);

    const QV4::FunctionObject *func = firstArgument->as<QV4::FunctionObject>();

    if (!func)
        THROW_GENERIC_ERROR("Qt.callLater: first argument not a function or signal");

    QPair<QObject *, int> functionData = QV4::QObjectMethod::extractQtMethod(func);

    QVector<DelayedFunctionCall>::Iterator iter;
    if (functionData.second != -1) {
        // This is a QObject function wrapper
        iter = self->m_delayedFunctionCalls.begin();
        while (iter != self->m_delayedFunctionCalls.end()) {
            DelayedFunctionCall& dfc = *iter;
            QPair<QObject *, int> storedFunctionData = QV4::QObjectMethod::extractQtMethod(dfc.m_function.as<QV4::FunctionObject>());
            if (storedFunctionData == functionData) {
                break; // Already stored!
            }
            ++iter;
        }
    } else {
        // This is a JavaScript function (dynamic slot on VMEMO)
        iter = self->m_delayedFunctionCalls.begin();
        while (iter != self->m_delayedFunctionCalls.end()) {
            DelayedFunctionCall& dfc = *iter;
            if (firstArgument->asReturnedValue() == dfc.m_function.value()) {
                break; // Already stored!
            }
            ++iter;
        }
    }

    const bool functionAlreadyStored = (iter != self->m_delayedFunctionCalls.end());
    if (functionAlreadyStored) {
        DelayedFunctionCall dfc = *iter;
        self->m_delayedFunctionCalls.erase(iter);
        self->m_delayedFunctionCalls.append(dfc);
    } else {
        self->m_delayedFunctionCalls.append(QV4::PersistentValue(engine, firstArgument));
    }

    DelayedFunctionCall& dfc = self->m_delayedFunctionCalls.last();
    if (dfc.m_objectGuard.isNull()) {
        if (functionData.second != -1) {
            // if it's a qobject function wrapper, guard against qobject deletion
            dfc.m_objectGuard = QQmlGuard<QObject>(functionData.first);
            dfc.m_guarded = true;
        } else if (func->scope()->type == QV4::Heap::ExecutionContext::Type_QmlContext) {
            QV4::QmlContext::Data *g = static_cast<QV4::QmlContext::Data *>(func->scope());
            Q_ASSERT(g->qml()->scopeObject);
            dfc.m_objectGuard = QQmlGuard<QObject>(g->qml()->scopeObject);
            dfc.m_guarded = true;
        }
    }
    self->storeAnyArguments(dfc, args, 1, engine);

    if (!self->m_callbackOutstanding) {
        self->m_tickedMethod.invoke(self, Qt::QueuedConnection);
        self->m_callbackOutstanding = true;
    }
    return QV4::Encode::undefined();
}

void QQmlDelayedCallQueue::storeAnyArguments(DelayedFunctionCall &dfc, QQmlV4Function *args, int offset, QV4::ExecutionEngine *engine)
{
    const int length = args->length() - offset;
    if (length == 0) {
        dfc.m_args.clear();
        return;
    }
    QV4::Scope scope(engine);
    QV4::ScopedArrayObject array(scope, engine->newArrayObject(length));
    uint i = 0;
    for (int j = offset, ej = args->length(); j < ej; ++i, ++j)
        array->put(i, (*args)[j]);
    dfc.m_args.set(engine, array);
}

void QQmlDelayedCallQueue::executeAllExpired_Later()
{
    // Make a local copy of the list and clear m_delayedFunctionCalls
    // This ensures correct behavior in the case of recursive calls to Qt.callLater()
    QVector<DelayedFunctionCall> delayedCalls = m_delayedFunctionCalls;
    m_delayedFunctionCalls.clear();

    QVector<DelayedFunctionCall>::Iterator iter = delayedCalls.begin();
    while (iter != delayedCalls.end()) {
        DelayedFunctionCall& dfc = *iter;
        dfc.execute(m_engine);
        ++iter;
    }
}

void QQmlDelayedCallQueue::ticked()
{
    m_callbackOutstanding = false;
    executeAllExpired_Later();
}

QT_END_NAMESPACE

#include "moc_qqmldelayedcallqueue_p.cpp"
