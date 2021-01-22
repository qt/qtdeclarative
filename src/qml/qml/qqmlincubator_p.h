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

#ifndef QQMLINCUBATOR_P_H
#define QQMLINCUBATOR_P_H

#include "qqmlincubator.h"

#include <private/qintrusivelist_p.h>
#include <private/qqmlvme_p.h>
#include <private/qrecursionwatcher_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlcontext_p.h>

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

QT_BEGIN_NAMESPACE

class RequiredProperties;

class QQmlIncubator;
class Q_QML_PRIVATE_EXPORT QQmlIncubatorPrivate : public QQmlEnginePrivate::Incubator
{
public:
    QQmlIncubatorPrivate(QQmlIncubator *q, QQmlIncubator::IncubationMode m);
    ~QQmlIncubatorPrivate();

    inline static QQmlIncubatorPrivate *get(QQmlIncubator *incubator) { return incubator->d; }

    QQmlIncubator *q;

    QQmlIncubator::Status calculateStatus() const;
    void changeStatus(QQmlIncubator::Status);
    QQmlIncubator::Status status;

    QQmlIncubator::IncubationMode mode;
    bool isAsynchronous;

    QList<QQmlError> errors;

    enum Progress { Execute, Completing, Completed };
    Progress progress;

    QPointer<QObject> result;
    QQmlGuardedContextData rootContext;
    QQmlEnginePrivate *enginePriv;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;
    QScopedPointer<QQmlObjectCreator> creator;
    int subComponentToCreate;
    QQmlVMEGuard vmeGuard;

    QExplicitlySharedDataPointer<QQmlIncubatorPrivate> waitingOnMe;
    typedef QQmlEnginePrivate::Incubator QIPBase;
    QIntrusiveList<QIPBase, &QIPBase::nextWaitingFor> waitingFor;

    QRecursionNode recursion;
    QVariantMap initialProperties;

    void clear();

    void forceCompletion(QQmlInstantiationInterrupt &i);
    void incubate(QQmlInstantiationInterrupt &i);
    RequiredProperties &requiredProperties();
    bool hadRequiredProperties() const;
};

QT_END_NAMESPACE

#endif // QQMLINCUBATOR_P_H

