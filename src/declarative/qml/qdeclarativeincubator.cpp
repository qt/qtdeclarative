/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativeincubator.h"
#include "qdeclarativecomponent.h"
#include "qdeclarativeincubator_p.h"

#include "qdeclarativecompiler_p.h"
#include "qdeclarativeexpression_p.h"

// XXX TODO 
//   - check that the Component.onCompleted behavior is the same as 4.8 in the synchronous and 
//     async if nested cases
void QDeclarativeEnginePrivate::incubate(QDeclarativeIncubator &i, QDeclarativeContextData *forContext)
{
    QDeclarativeIncubatorPrivate *p = i.d;

    QDeclarativeIncubator::IncubationMode mode = i.incubationMode();

    if (!incubationController)
        mode = QDeclarativeIncubator::Synchronous;

    if (mode == QDeclarativeIncubator::AsynchronousIfNested) {
        mode = QDeclarativeIncubator::Synchronous;

        // Need to find the first constructing context and see if it is asynchronous
        QDeclarativeIncubatorPrivate *parentIncubator = 0;
        QDeclarativeContextData *cctxt = forContext;
        while (cctxt) {
            if (cctxt->activeVMEData) {
                parentIncubator = (QDeclarativeIncubatorPrivate *)cctxt->activeVMEData;
                break;
            }
            cctxt = cctxt->parent;
        }

        if (parentIncubator && parentIncubator->isAsynchronous) {
            mode = QDeclarativeIncubator::Asynchronous;
            p->waitingOnMe = parentIncubator;
            parentIncubator->waitingFor.insert(p);
        }
    }

    p->isAsynchronous = (mode != QDeclarativeIncubator::Synchronous);

    inProgressCreations++;

    if (mode == QDeclarativeIncubator::Synchronous) {
        typedef QDeclarativeIncubatorPrivate IP;
        QRecursionWatcher<IP, &IP::recursion> watcher(p);

        p->changeStatus(QDeclarativeIncubator::Loading);

        if (!watcher.hasRecursed()) {
            QDeclarativeVME::Interrupt i;
            p->incubate(i);
        }
    } else {
        incubatorList.insert(p);
        incubatorCount++;

        p->vmeGuard.guard(&p->vme);
        p->changeStatus(QDeclarativeIncubator::Loading);

        if (incubationController)
            incubationController->incubatingObjectCountChanged(incubatorCount);
    }
}

/*!
Sets the engine's incubation \a controller.  The engine can only have one active controller 
and it does not take ownership of it.

\sa incubationController()
*/
void QDeclarativeEngine::setIncubationController(QDeclarativeIncubationController *controller)
{
    Q_D(QDeclarativeEngine);
    if (d->incubationController)
        d->incubationController->d = 0;
    d->incubationController = controller;
    if (controller) controller->d = d;
}

/*!
Returns the currently set incubation controller, or 0 if no controller has been set.

\sa setIncubationController()
*/
QDeclarativeIncubationController *QDeclarativeEngine::incubationController() const
{
    Q_D(const QDeclarativeEngine);
    return d->incubationController;
}

QDeclarativeIncubatorPrivate::QDeclarativeIncubatorPrivate(QDeclarativeIncubator *q, 
                                                           QDeclarativeIncubator::IncubationMode m)
: q(q), status(QDeclarativeIncubator::Null), mode(m), isAsynchronous(false), progress(Execute),
  result(0), component(0), vme(this), waitingOnMe(0)
{
}

QDeclarativeIncubatorPrivate::~QDeclarativeIncubatorPrivate()
{
}

void QDeclarativeIncubatorPrivate::clear()
{
    if (next.isInList()) {
        next.remove();
        Q_ASSERT(component);
        QDeclarativeEnginePrivate *enginePriv = QDeclarativeEnginePrivate::get(component->engine);
        component->release();
        component = 0;
        enginePriv->incubatorCount--;
        QDeclarativeIncubationController *controller = enginePriv->incubationController;
        if (controller)
            controller->incubatingObjectCountChanged(enginePriv->incubatorCount);
    } else if (component) {
        component->release();
        component = 0;
    }
    if (!rootContext.isNull()) {
        rootContext->activeVMEData = 0;
        rootContext = 0;
    }

    if (nextWaitingFor.isInList()) {
        Q_ASSERT(waitingOnMe);
        nextWaitingFor.remove();
        waitingOnMe = 0;
    }
}

/*!
\class QDeclarativeIncubationController
\brief QDeclarativeIncubationController instances drive the progress of QDeclarativeIncubators

In order to behave asynchronously and not introduce stutters or freezes in an application,
the process of creating objects a QDeclarativeIncubators must be driven only during the
application's idle time.  QDeclarativeIncubationController allows the application to control
exactly when, how often and for how long this processing occurs.

A QDeclarativeIncubationController derived instance should be created and set on a 
QDeclarativeEngine by calling the QDeclarativeEngine::setIncubationController() method.
Processing is then controlled by calling the QDeclarativeIncubationController::incubateFor()
or QDeclarativeIncubationController::incubateWhile() methods as dictated by the application's
requirements.

For example, this is an example of a incubation controller that will incubate for a maximum
of 5 milliseconds out of every 16 milliseconds.

\code
class PeriodicIncubationController : public QObject, 
                                     public QDeclarativeIncubationController 
{
public:
    PeriodicIncubationController() { 
        startTimer(16); 
    }

protected:
    virtual void timerEvent(QTimerEvent *) {
        incubateFor(5);
    }
};
\endcode

Although the previous example would work, it is not optimal.  Real world incubation
controllers should try and maximize the amount of idle time they consume - rather
than a static amount like 5 milliseconds - while not disturbing the application.  
*/

/*!
Create a new incubation controller.
*/
QDeclarativeIncubationController::QDeclarativeIncubationController()
: d(0)
{
}

/*! \internal */
QDeclarativeIncubationController::~QDeclarativeIncubationController()
{
    if (d) QDeclarativeEnginePrivate::get(d)->setIncubationController(0);
    d = 0;
}

/*!
Return the QDeclarativeEngine this incubation controller is set on, or 0 if it
has not been set on any engine.
*/
QDeclarativeEngine *QDeclarativeIncubationController::engine() const
{
    return QDeclarativeEnginePrivate::get(d);
}

/*!
Return the number of objects currently incubating.
*/
int QDeclarativeIncubationController::incubatingObjectCount() const
{
    if (d)
        return d->incubatorCount;
    else 
        return 0;
}

/*!
Called when the number of incubating objects changes.  \a incubatingObjectCount is the 
new number of incubating objects.

The default implementation does nothing.
*/
void QDeclarativeIncubationController::incubatingObjectCountChanged(int incubatingObjectCount)
{
    Q_UNUSED(incubatingObjectCount);
}

void QDeclarativeIncubatorPrivate::incubate(QDeclarativeVME::Interrupt &i)
{
    if (!component)
        return;
    typedef QDeclarativeIncubatorPrivate IP;
    QRecursionWatcher<IP, &IP::recursion> watcher(this);

    QDeclarativeEngine *engine = component->engine;
    QDeclarativeEnginePrivate *enginePriv = QDeclarativeEnginePrivate::get(engine);

    bool guardOk = vmeGuard.isOK();
    vmeGuard.clear();

    if (!guardOk) {
        QDeclarativeError error;
        error.setUrl(component->url);
        error.setDescription(QDeclarativeComponent::tr("Object destroyed during incubation"));
        errors << error;
        progress = QDeclarativeIncubatorPrivate::Completed;

        goto finishIncubate;
    }

    if (progress == QDeclarativeIncubatorPrivate::Execute) {
        enginePriv->referenceScarceResources();
        QObject *tresult = vme.execute(&errors, i);
        enginePriv->dereferenceScarceResources();

        if (watcher.hasRecursed())
            return;

        result = tresult;
        if (errors.isEmpty() && result == 0) 
            goto finishIncubate;

        if (result) {
            QDeclarativeData *ddata = QDeclarativeData::get(result);
            Q_ASSERT(ddata);
            ddata->indestructible = true;

            q->setInitialState(result);
        }

        if (watcher.hasRecursed())
            return;

        if (errors.isEmpty())
            progress = QDeclarativeIncubatorPrivate::Completing;
        else
            progress = QDeclarativeIncubatorPrivate::Completed;

        changeStatus(calculateStatus());

        if (watcher.hasRecursed())
            return;

        if (i.shouldInterrupt())
            goto finishIncubate;
    }

    if (progress == QDeclarativeIncubatorPrivate::Completing) {
        do {
            if (watcher.hasRecursed())
                return;

            QDeclarativeContextData *ctxt = vme.complete(i);
            if (ctxt) {
                rootContext = ctxt;
                progress = QDeclarativeIncubatorPrivate::Completed;
                goto finishIncubate;
            }
        } while (!i.shouldInterrupt());
    }

finishIncubate:
    if (progress == QDeclarativeIncubatorPrivate::Completed && waitingFor.isEmpty()) {
        typedef QDeclarativeIncubatorPrivate IP;

        QDeclarativeIncubatorPrivate *isWaiting = waitingOnMe;
        clear();

        if (isWaiting) {
            QRecursionWatcher<IP, &IP::recursion> watcher(isWaiting);
            changeStatus(calculateStatus());
            if (!watcher.hasRecursed())
                isWaiting->incubate(i);
        } else {
            changeStatus(calculateStatus());
        }

        enginePriv->inProgressCreations--;

        if (0 == enginePriv->inProgressCreations) {
            while (enginePriv->erroredBindings) {
                enginePriv->warning(enginePriv->erroredBindings->error);
                enginePriv->erroredBindings->removeError();
            }
        }
    } else {
        vmeGuard.guard(&vme);
    }
}

/*!
Incubate objects for \a msecs, or until there are no more objects to incubate.
*/
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

/*!
Incubate objects while the bool pointed to by \a flag is true, or until there are no
more objects to incubate.

Generally this method is used in conjunction with a thread or a UNIX signal that sets
the bool pointed to by \a flag to false when it wants incubation to be interrupted.
*/
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

/*!
\class QDeclarativeIncubator
\brief The QDeclarativeIncubator class allows QML objects to be created asynchronously.

Creating QML objects - like delegates in a view, or a new page in an application - can take
a noticable amount of time, especially on resource constrained mobile devices.  When an
application uses QDeclarativeComponent::create() directly, the QML object instance is created
synchronously which, depending on the complexity of the object,  can cause noticable pauses or 
stutters in the application.

The use of QDeclarativeIncubator gives more control over the creation of a QML object, 
including allowing it to be created asynchronously using application idle time.  The following 
example shows a simple use of QDeclarativeIncubator.

\code
QDeclarativeIncubator incubator;
component->create(incubator);

while (incubator.isReady()) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
}

QObject *object = incubator.object();
\endcode

Asynchronous incubators are controlled by a QDeclarativeIncubationController that is 
set on the QDeclarativeEngine, which lets the engine know when the application is idle and
incubating objects should be processed.  If an incubation controller is not set on the
QDeclarativeEngine, QDeclarativeIncubator creates objects synchronously regardless of the
specified IncubationMode.  

QDeclarativeIncubator supports three incubation modes:
\list
\i Synchronous The creation occurs synchronously.  That is, once the 
QDeclarativeComponent::create() call returns, the incubator will already be in either the
Error or Ready state.  A synchronous incubator has no real advantage compared to using
the synchronous creation methods on QDeclarativeComponent directly, but it may simplify an 
application's implementation to use the same API for both synchronous and asynchronous 
creations.

\i Asynchronous (default) The creation occurs asynchronously, assuming a 
QDeclarativeIncubatorController is set on the QDeclarativeEngine.  

The incubator will remain in the Loading state until either the creation is complete or an error 
occurs.  The statusChanged() callback can be used to be notified of status changes.

Applications should use the Asynchronous incubation mode to create objects that are not needed
immediately.  For example, the ListView element uses Asynchronous incubation to create objects
that are slightly off screen while the list is being scrolled.  If, during asynchronous creation,
the object is needed immediately the QDeclarativeIncubator::forceCompletion() method can be called
to complete the creation process synchronously.

\i AsynchronousIfNested The creation will occur asynchronously if part of a nested asynchronous 
creation, or synchronously if not.  

In most scenarios where a QML element or component wants the appearance of a synchronous 
instantiation, it should use this mode.  

This mode is best explained with an example.  When the ListView element is first created, it needs
to populate itself with an initial set of delegates to show.  If the ListView was 400 pixels high, 
and each delegate was 100 pixels high, it would need to create four initial delegate instances.  If
the ListView used the Asynchronous incubation mode, the ListView would always be created empty and
then, sometime later, the four initial elements would appear.  

Conversely, if the ListView was to use the Synchronous incubation mode it would behave correctly 
but it may introduce stutters into the application.  As QML would have to stop and instantiate the 
ListView's delegates synchronously, if the ListView was part of a QML component that was being 
instantiated asynchronously this would undo much of the benefit of asynchronous instantiation.

The AsynchronousIfNested mode reconciles this problem.  By using AsynchronousIfNested, the ListView
delegates are instantiated asynchronously if the ListView itself is already part of an asynchronous
instantiation, and synchronously otherwise.  In the case of a nested asynchronous instantiation, the
outer asynchronous instantiation will not complete until after all the nested instantiations have also
completed.  This ensures that by the time the outer asynchronous instantitation completes, inner 
elements like ListView have already completed loading their initial delegates.

It is almost always incorrect to use the Synchronous incubation mode - elements or components that 
want the appearance of synchronous instantiation, but without the downsides of introducing freezes 
or stutters into the application, should use the AsynchronousIfNested incubation mode.
\endlist
*/

/*!
Create a new incubator with the specified \a mode
*/
QDeclarativeIncubator::QDeclarativeIncubator(IncubationMode mode)
: d(new QDeclarativeIncubatorPrivate(this, mode))
{
}

/*! \internal */
QDeclarativeIncubator::~QDeclarativeIncubator()
{
    clear();

    delete d; d = 0;
}

/*!
\enum QDeclarativeIncubator::IncubationMode

Specifies the mode the incubator operates in.  Regardless of the incubation mode, a 
QDeclarativeIncubator will behave synchronously if the QDeclarativeEngine does not have
a QDeclarativeIncubationController set.

\value Asynchronous The object will be created asynchronously.
\value AsynchronousIfNested If the object is being created in a context that is already part
of an asynchronous creation, this incubator will join that existing incubation and execute 
asynchronously.  The existing incubation will not become Ready until both it and this 
incubation have completed.  Otherwise, the incubation will execute synchronously.
\value Synchronous The object will be created synchronously.
*/

/*!
\enum QDeclarativeIncubator::Status

Specifies the status of the QDeclarativeIncubator.

\value Null Incubation is not in progress.  Call QDeclarativeComponent::create() to begin incubating.
\value Ready The object is fully created and can be accessed by calling object().
\value Loading The object is in the process of being created.
\value Error An error occurred.  The errors can be access by calling errors().
*/

/*!
Clears the incubator.  Any in-progress incubation is aborted.  If the incubator is in the 
Ready state, the created object is \b not deleted.
*/
void QDeclarativeIncubator::clear()
{
    typedef QDeclarativeIncubatorPrivate IP;
    QRecursionWatcher<IP, &IP::recursion> watcher(d);

    Status s = status();

    if (s == Null)
        return;

    QDeclarativeEnginePrivate *enginePriv = 0;
    if (s == Loading) {
        Q_ASSERT(d->component);
        enginePriv = QDeclarativeEnginePrivate::get(d->component->engine);
        if (d->result) d->result->deleteLater();
        d->result = 0;
    }

    d->clear();

    d->vme.reset();
    d->vmeGuard.clear();

    Q_ASSERT(d->component == 0);
    Q_ASSERT(d->waitingOnMe == 0);
    Q_ASSERT(d->waitingFor.isEmpty());
    Q_ASSERT(!d->nextWaitingFor.isInList());

    d->errors.clear();
    d->progress = QDeclarativeIncubatorPrivate::Execute;
    d->result = 0;

    if (s == Loading) {
        Q_ASSERT(enginePriv);

        enginePriv->inProgressCreations--;
        if (0 == enginePriv->inProgressCreations) {
            while (enginePriv->erroredBindings) {
                enginePriv->warning(enginePriv->erroredBindings->error);
                enginePriv->erroredBindings->removeError();
            }
        }
    }

    d->changeStatus(Null);
}

/*!
Force any in-progress incubation to finish synchronously.  Once this call
returns, the incubator will not be in the Loading state.
*/
void QDeclarativeIncubator::forceCompletion()
{
    QDeclarativeVME::Interrupt i;
    while (Loading == status()) {
        while (Loading == status() && !d->waitingFor.isEmpty())
            static_cast<QDeclarativeIncubatorPrivate *>(d->waitingFor.first())->incubate(i);
        if (Loading == status())
            d->incubate(i);
    }
}

/*!
Returns true if the incubator's status() is Null.
*/
bool QDeclarativeIncubator::isNull() const
{
    return status() == Null;
}

/*!
Returns true if the incubator's status() is Ready.
*/
bool QDeclarativeIncubator::isReady() const
{
    return status() == Ready;
}

/*!
Returns true if the incubator's status() is Error.
*/
bool QDeclarativeIncubator::isError() const
{
    return status() == Error;
}

/*!
Returns true if the incubator's status() is Loading.
*/
bool QDeclarativeIncubator::isLoading() const
{
    return status() == Loading;
}

/*!
Return the list of errors encountered while incubating the object.
*/
QList<QDeclarativeError> QDeclarativeIncubator::errors() const
{
    return d->errors;
}

/*!
Return the incubation mode passed to the QDeclarativeIncubator constructor.
*/
QDeclarativeIncubator::IncubationMode QDeclarativeIncubator::incubationMode() const
{
    return d->mode;
}

/*!
Return the current status of the incubator.
*/
QDeclarativeIncubator::Status QDeclarativeIncubator::status() const
{
    return d->status;
}

/*!
Return the incubated object if the status is Ready, otherwise 0.
*/
QObject *QDeclarativeIncubator::object() const
{
    if (status() != Ready) return 0;
    else return d->result;
}

/*!
Called when the status of the incubator changes.  \a status is the new status.

The default implementation does nothing.
*/
void QDeclarativeIncubator::statusChanged(Status status)
{
    Q_UNUSED(status);
}

/*!
Called after the object is first created, but before property bindings are
evaluated and, if applicable, QDeclarativeParserStatus::componentComplete() is
called.  This is equivalent to the point between QDeclarativeComponent::beginCreate()
and QDeclarativeComponent::endCreate(), and can be used to assign initial values
to the object's properties.

The default implementation does nothing.
*/
void QDeclarativeIncubator::setInitialState(QObject *object)
{
    Q_UNUSED(object);
}

void QDeclarativeIncubatorPrivate::changeStatus(QDeclarativeIncubator::Status s)
{
    if (s == status) 
        return;

    status = s;
    q->statusChanged(status);
}

QDeclarativeIncubator::Status QDeclarativeIncubatorPrivate::calculateStatus() const
{
    if (!errors.isEmpty()) 
        return QDeclarativeIncubator::Error;
    else if (result && progress == QDeclarativeIncubatorPrivate::Completed && 
             waitingFor.isEmpty()) 
        return QDeclarativeIncubator::Ready;
    else if (component) 
        return QDeclarativeIncubator::Loading;
    else 
        return QDeclarativeIncubator::Null;
}

