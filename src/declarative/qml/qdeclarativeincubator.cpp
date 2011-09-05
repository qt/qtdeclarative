/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativeincubator.h"
#include "qdeclarativeincubator_p.h"

#include <private/qdeclarativecompiler_p.h>
#include <private/qdeclarativeexpression_p.h>

// XXX TODO 
//   - check that the Component.onCompleted behavior is the same as 4.8 in the synchronous and 
//     async if nested cases
void QDeclarativeEnginePrivate::incubate(QDeclarativeIncubator &i, QDeclarativeContextData *forContext)
{
    QDeclarativeIncubatorPrivate *p = i.d;

    QDeclarativeIncubator::IncubationMode mode = i.incubationMode();

    if (mode == QDeclarativeIncubator::AsynchronousIfNested) {
        mode = QDeclarativeIncubator::Synchronous;

        // Need to find the first constructing context and see if it is asynchronous
        QDeclarativeIncubatorPrivate *parentIncubator = 0;
        QDeclarativeContextData *cctxt = forContext;
        while (cctxt) {
            if (cctxt->activeVME) {
                parentIncubator = (QDeclarativeIncubatorPrivate *)cctxt->activeVME->data;
                break;
            }
            cctxt = cctxt->parent;
        }

        if (parentIncubator && parentIncubator->mode != QDeclarativeIncubator::Synchronous) {
            mode = QDeclarativeIncubator::Asynchronous;
            p->waitingOnMe = parentIncubator;
            parentIncubator->waitingFor.insert(p);
        }
    }

    if (mode == QDeclarativeIncubator::Synchronous) {
        QDeclarativeVME::Interrupt i;
        p->incubate(i);
    } else {
        inProgressCreations++;
        incubatorList.insert(p);
        incubatorCount++;

        if (incubationController)
            incubationController->incubatingObjectCountChanged(incubatorCount);
    }
}

void QDeclarativeEngine::setIncubationController(QDeclarativeIncubationController *i)
{
    Q_D(QDeclarativeEngine);
    d->incubationController = i;
    if (i) i->d = d;
}

QDeclarativeIncubationController *QDeclarativeEngine::incubationController() const
{
    Q_D(const QDeclarativeEngine);
    return d->incubationController;
}

QDeclarativeIncubatorPrivate::QDeclarativeIncubatorPrivate(QDeclarativeIncubator *q, 
                                                           QDeclarativeIncubator::IncubationMode m)
: q(q), mode(m), progress(Execute), result(0), component(0), vme(this), waitingOnMe(0)
{
}

QDeclarativeIncubatorPrivate::~QDeclarativeIncubatorPrivate()
{
    clear();
}

void QDeclarativeIncubatorPrivate::clear()
{
    if (next.isInList()) {
        next.remove();
        Q_ASSERT(component);
        QDeclarativeEnginePrivate *enginePriv = QDeclarativeEnginePrivate::get(component->engine);
        component->release();

        enginePriv->incubatorCount--;
        QDeclarativeIncubationController *controller = enginePriv->incubationController;
        if (controller)
            controller->incubatingObjectCountChanged(enginePriv->incubatorCount);
    }

    if (nextWaitingFor.isInList()) {
        Q_ASSERT(waitingOnMe);
        nextWaitingFor.remove();
        waitingOnMe = 0;
    }

}

QDeclarativeIncubationController::QDeclarativeIncubationController()
: d(0)
{
}

QDeclarativeIncubationController::~QDeclarativeIncubationController()
{
    if (d) QDeclarativeEnginePrivate::get(d)->setIncubationController(0);
    d = 0;
}

QDeclarativeEngine *QDeclarativeIncubationController::engine() const
{
    return QDeclarativeEnginePrivate::get(d);
}

int QDeclarativeIncubationController::incubatingObjectCount() const
{
    if (d)
        return d->incubatorCount;
    else 
        return 0;
}

void QDeclarativeIncubationController::incubatingObjectCountChanged(int)
{
}

void QDeclarativeIncubatorPrivate::incubate(QDeclarativeVME::Interrupt &i)
{
    QDeclarativeEngine *engine = component->engine;
    QDeclarativeEnginePrivate *enginePriv = QDeclarativeEnginePrivate::get(engine);

    if (progress == QDeclarativeIncubatorPrivate::Execute) {
        enginePriv->referenceScarceResources();
        result = vme.execute(&errors, i);
        enginePriv->dereferenceScarceResources();

        if (errors.isEmpty() && result == 0) 
            return; // Interrupted

        if (result) {
            QDeclarativeData *ddata = QDeclarativeData::get(result);
            Q_ASSERT(ddata);
            ddata->indestructible = true;

            q->setInitialState(result);
        }

        if (errors.isEmpty())
            progress = QDeclarativeIncubatorPrivate::Completing;
        else
            progress = QDeclarativeIncubatorPrivate::Completed;

        q->statusChanged(q->status());

        if (i.shouldInterrupt())
            goto finishIncubate;
    }

    if (progress == QDeclarativeIncubatorPrivate::Completing) {
        do {
            if (vme.complete(i)) {
                progress = QDeclarativeIncubatorPrivate::Completed;
                goto finishIncubate;
            }
        } while (!i.shouldInterrupt());
    }

finishIncubate:
    if (progress == QDeclarativeIncubatorPrivate::Completed && waitingFor.isEmpty()) {
        QDeclarativeIncubatorPrivate *isWaiting = waitingOnMe;
        clear();
        if (isWaiting) isWaiting->incubate(i); 

        enginePriv->inProgressCreations--;

        q->statusChanged(q->status());

        if (0 == enginePriv->inProgressCreations) {
            while (enginePriv->erroredBindings) {
                enginePriv->warning(enginePriv->erroredBindings->error);
                enginePriv->erroredBindings->removeError();
            }
        }
    }
}

void QDeclarativeIncubationController::incubateFor(int msecs)
{
    if (!d || d->incubatorCount == 0)
        return;

    QDeclarativeVME::Interrupt i(msecs * 1000000);
    i.reset();
    do {
        QDeclarativeIncubatorPrivate *p = (QDeclarativeIncubatorPrivate*)d->incubatorList.first();
        p->incubate(i);
    } while (d && d->incubatorCount != 0 && !i.shouldInterrupt());
}

void QDeclarativeIncubationController::incubateWhile(bool *flag)
{
    if (!d || d->incubatorCount == 0)
        return;

    QDeclarativeVME::Interrupt i(flag);
    do {
        QDeclarativeIncubatorPrivate *p = (QDeclarativeIncubatorPrivate*)d->incubatorList.first();
        p->incubate(i);
    } while (d && d->incubatorCount != 0 && !i.shouldInterrupt());
}

QDeclarativeIncubator::QDeclarativeIncubator(IncubationMode m)
: d(new QDeclarativeIncubatorPrivate(this, m))
{
}

QDeclarativeIncubator::~QDeclarativeIncubator()
{
    delete d; d = 0;
}

void QDeclarativeIncubator::clear()
{
}

void QDeclarativeIncubator::forceIncubation()
{
    QDeclarativeVME::Interrupt i;
    while (Loading == status()) {
        while (Loading == status() && !d->waitingFor.isEmpty())
            d->waitingFor.first()->incubate(i);
        if (Loading == status())
            d->incubate(i);
    }

}

bool QDeclarativeIncubator::isNull() const
{
    return status() == Null;
}

bool QDeclarativeIncubator::isReady() const
{
    return status() == Ready;
}

bool QDeclarativeIncubator::isError() const
{
    return status() == Error;
}

bool QDeclarativeIncubator::isLoading() const
{
    return status() == Loading;
}

QList<QDeclarativeError> QDeclarativeIncubator::errors() const
{
    return d->errors;
}

QDeclarativeIncubator::IncubationMode QDeclarativeIncubator::incubationMode() const
{
    return d->mode;
}

QDeclarativeIncubator::Status QDeclarativeIncubator::status() const
{
    if (!d->errors.isEmpty()) return Error;
    else if (d->result && d->progress == QDeclarativeIncubatorPrivate::Completed && 
             d->waitingFor.isEmpty()) return Ready;
    else if (d->component) return Loading;
    else return Null;
}

QObject *QDeclarativeIncubator::object() const
{
    if (status() != Ready) return 0;
    else return d->result;
}

void QDeclarativeIncubator::statusChanged(Status)
{
}

void QDeclarativeIncubator::setInitialState(QObject *)
{
}
