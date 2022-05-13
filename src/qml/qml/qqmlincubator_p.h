// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLINCUBATOR_P_H
#define QQMLINCUBATOR_P_H

#include "qqmlincubator.h"

#include <private/qintrusivelist_p.h>
#include <private/qqmlvme_p.h>
#include <private/qrecursionwatcher_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlguardedcontextdata_p.h>

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
    bool hadTopLevelRequiredProperties() const;
};

QT_END_NAMESPACE

#endif // QQMLINCUBATOR_P_H

