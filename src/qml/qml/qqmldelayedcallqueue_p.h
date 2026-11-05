// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDELAYEDCALLQUEUE_P_H
#define QQMLDELAYEDCALLQUEUE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmetatype.h>
#include <private/qqmlguard_p.h>
#include <private/qv4context_p.h>

QT_BEGIN_NAMESPACE

class QQmlDelayedCallQueue : public QObject
{
    Q_OBJECT
public:
    QQmlDelayedCallQueue();
    ~QQmlDelayedCallQueue() override;

    void init(QV4::ExecutionEngine *);

    static QV4::ReturnedValue addUniquelyAndExecuteLater(QV4::ExecutionEngine *engine,
            QQmlV4FunctionPtr args);

public Q_SLOTS:
    void ticked();

private:
    struct DelayedFunctionCall
    {
        DelayedFunctionCall() {}
        DelayedFunctionCall(QV4::PersistentValue function, QObject *explicitObject)
            : m_function(function), m_explicitObject(explicitObject), m_guarded(false),
              m_hasExplicitObject(explicitObject != nullptr) { }

        bool matchesExplicitObject(const QObject *explicitObject) const
        {
            return m_hasExplicitObject == (explicitObject != nullptr)
                    && m_explicitObject.data() == explicitObject;
        }

        bool argumentsEqual(QQmlV4FunctionPtr args, int offset) const;

        void execute(QV4::ExecutionEngine *engine) const;

        QV4::PersistentValue m_function;
        QV4::PersistentValue m_args;
        QQmlGuard<QObject> m_objectGuard;
        // The object passed to Qt.callLater(object, function, ...)
        QQmlGuard<QObject> m_explicitObject;
        bool m_guarded;
        bool m_hasExplicitObject = false;
    };

    void storeAnyArguments(DelayedFunctionCall& dfc, QQmlV4FunctionPtr args, int offset, QV4::ExecutionEngine *engine);
    void executeAllExpired_Later();

    QV4::ExecutionEngine *m_engine;
    QList<DelayedFunctionCall> m_delayedFunctionCalls;
    QMetaMethod m_tickedMethod;
    bool m_callbackOutstanding;
};

QT_END_NAMESPACE

#endif // QQMLDELAYEDCALLQUEUE_P_H
