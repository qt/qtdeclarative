/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

    QV4::ReturnedValue addUniquelyAndExecuteLater(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);

public Q_SLOTS:
    void ticked();

private:
    struct DelayedFunctionCall
    {
        DelayedFunctionCall() {}
        DelayedFunctionCall(QV4::PersistentValue function)
            : m_function(function), m_guarded(false) { }

        void execute(QV4::ExecutionEngine *engine) const;

        QV4::PersistentValue m_function;
        QV4::PersistentValue m_args;
        QQmlGuard<QObject> m_objectGuard;
        bool m_guarded;
    };

    void storeAnyArguments(DelayedFunctionCall& dfc, const QV4::Value *argv, int argc, int offset, QV4::ExecutionEngine *engine);
    void executeAllExpired_Later();

    QV4::ExecutionEngine *m_engine;
    QVector<DelayedFunctionCall> m_delayedFunctionCalls;
    QMetaMethod m_tickedMethod;
    bool m_callbackOutstanding;
};

QT_END_NAMESPACE

#endif // QQMLDELAYEDCALLQUEUE_P_H
