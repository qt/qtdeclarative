// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlincubator.h"
#include "qqmlcomponent.h"
#include "qqmlincubator_p.h"

#include "qqmlobjectcreator_p.h"
#include <private/qqmlcomponent_p.h>

void QQmlEnginePrivate::incubate(
        QQmlIncubator &i, const QQmlRefPointer<QQmlContextData> &forContext)
{
    QExplicitlySharedDataPointer<QQmlIncubatorPrivate> p(i.d);

    QQmlIncubator::IncubationMode mode = i.incubationMode();

    if (!incubationController)
        mode = QQmlIncubator::Synchronous;

    if (mode == QQmlIncubator::AsynchronousIfNested) {
        mode = QQmlIncubator::Synchronous;

        // Need to find the first constructing context and see if it is asynchronous
        QExplicitlySharedDataPointer<QQmlIncubatorPrivate> parentIncubator;
        QQmlRefPointer<QQmlContextData> cctxt = forContext;
        while (cctxt) {
            if (QQmlIncubatorPrivate *incubator = cctxt->incubator()) {
                parentIncubator = incubator;
                break;
            }
            cctxt = cctxt->parent();
        }

        if (parentIncubator && parentIncubator->isAsynchronous) {
            mode = QQmlIncubator::Asynchronous;
            p->waitingOnMe = parentIncubator;
            parentIncubator->waitingFor.insert(p.data());
        }
    }

    p->isAsynchronous = (mode != QQmlIncubator::Synchronous);

    inProgressCreations++;

    if (mode == QQmlIncubator::Synchronous) {
        QRecursionWatcher<QQmlIncubatorPrivate, &QQmlIncubatorPrivate::recursion> watcher(p.data());

        p->changeStatus(QQmlIncubator::Loading);

        if (!watcher.hasRecursed()) {
            QQmlInstantiationInterrupt i;
            p->incubate(i);
        }
    } else {
        incubatorList.insert(p.data());
        incubatorCount++;

        p->vmeGuard.guard(p->creator.data());
        p->changeStatus(QQmlIncubator::Loading);

        if (incubationController)
             incubationController->incubatingObjectCountChanged(incubatorCount);
    }
}

/*!
Sets the engine's incubation \a controller.  The engine can only have one active controller
and it does not take ownership of it.

\sa incubationController()
*/
void QQmlEngine::setIncubationController(QQmlIncubationController *controller)
{
    Q_D(QQmlEngine);
    if (d->incubationController)
        d->incubationController->d = nullptr;
    d->incubationController = controller;
    if (controller) controller->d = d;
}

/*!
Returns the currently set incubation controller, or 0 if no controller has been set.

\sa setIncubationController()
*/
QQmlIncubationController *QQmlEngine::incubationController() const
{
    Q_D(const QQmlEngine);
    return d->incubationController;
}

QQmlIncubatorPrivate::QQmlIncubatorPrivate(QQmlIncubator *q, QQmlIncubator::IncubationMode m)
    : q(q), status(QQmlIncubator::Null), mode(m), isAsynchronous(false), progress(Execute),
      result(nullptr), enginePriv(nullptr), waitingOnMe(nullptr)
{
}

QQmlIncubatorPrivate::~QQmlIncubatorPrivate()
{
    clear();
}

void QQmlIncubatorPrivate::clear()
{
    // reset the tagged pointer
    if (requiredPropertiesFromComponent)
        requiredPropertiesFromComponent = decltype(requiredPropertiesFromComponent){};
    compilationUnit.reset();
    if (next.isInList()) {
        next.remove();
        enginePriv->incubatorCount--;
        QQmlIncubationController *controller = enginePriv->incubationController;
        if (controller)
             controller->incubatingObjectCountChanged(enginePriv->incubatorCount);
    }
    enginePriv = nullptr;
    if (!rootContext.isNull()) {
        if (rootContext->incubator())
            rootContext->setIncubator(nullptr);
        rootContext.setContextData({});
    }

    if (nextWaitingFor.isInList()) {
        Q_ASSERT(waitingOnMe);
        nextWaitingFor.remove();
        waitingOnMe = nullptr;
    }

    // if we're waiting on any incubators then they should be cleared too.
    while (waitingFor.first()) {
        QQmlIncubator * i = static_cast<QQmlIncubatorPrivate*>(waitingFor.first())->q;
        if (i)
            i->clear();
    }

    bool guardOk = vmeGuard.isOK();

    vmeGuard.clear();
    if (creator && guardOk)
        creator->clear();
    creator.reset(nullptr);
}

/*!
\class QQmlIncubationController
\brief QQmlIncubationController instances drive the progress of QQmlIncubators.
\inmodule QtQml

In order to behave asynchronously and not introduce stutters or freezes in an application,
the process of creating objects a QQmlIncubators must be driven only during the
application's idle time.  QQmlIncubationController allows the application to control
exactly when, how often and for how long this processing occurs.

A QQmlIncubationController derived instance should be created and set on a
QQmlEngine by calling the QQmlEngine::setIncubationController() method.
Processing is then controlled by calling the QQmlIncubationController::incubateFor()
or QQmlIncubationController::incubateWhile() methods as dictated by the application's
requirements.

For example, this is an example of a incubation controller that will incubate for a maximum
of 5 milliseconds out of every 16 milliseconds.

\code
class PeriodicIncubationController : public QObject,
                                     public QQmlIncubationController
{
public:
    PeriodicIncubationController() {
        startTimer(16);
    }

protected:
    void timerEvent(QTimerEvent *) override {
        incubateFor(5);
    }
};
\endcode

Although the example works, it is heavily simplified. Real world incubation controllers
try and maximize the amount of idle time they consume while not disturbing the
application. Using a static amount of 5 milliseconds like above may both leave idle
time on the table in some frames and disturb the application in others.

\l{QQuickWindow}, \l{QQuickView}, and \l{QQuickWidget} all pre-create an incubation
controller that spaces out incubation over multiple frames using a more intelligent
algorithm. You rarely have to write your own.

*/

/*!
Create a new incubation controller.
*/
QQmlIncubationController::QQmlIncubationController()
: d(nullptr)
{
}

/*! \internal */
QQmlIncubationController::~QQmlIncubationController()
{
    if (d) QQmlEnginePrivate::get(d)->setIncubationController(nullptr);
    d = nullptr;
}

/*!
Return the QQmlEngine this incubation controller is set on, or 0 if it
has not been set on any engine.
*/
QQmlEngine *QQmlIncubationController::engine() const
{
    return QQmlEnginePrivate::get(d);
}

/*!
Return the number of objects currently incubating.
*/
int QQmlIncubationController::incubatingObjectCount() const
{
    return d ? d->incubatorCount : 0;
}

/*!
Called when the number of incubating objects changes.  \a incubatingObjectCount is the
new number of incubating objects.

The default implementation does nothing.
*/
void QQmlIncubationController::incubatingObjectCountChanged(int incubatingObjectCount)
{
    Q_UNUSED(incubatingObjectCount);
}

void QQmlIncubatorPrivate::forceCompletion(QQmlInstantiationInterrupt &i)
{
    while (QQmlIncubator::Loading == status) {
        while (QQmlIncubator::Loading == status && !waitingFor.isEmpty())
            waitingFor.first()->forceCompletion(i);
        if (QQmlIncubator::Loading == status)
            incubate(i);
    }
}


void QQmlIncubatorPrivate::incubate(QQmlInstantiationInterrupt &i)
{
    if (!compilationUnit)
        return;

    QExplicitlySharedDataPointer<QQmlIncubatorPrivate> protectThis(this);

    QRecursionWatcher<QQmlIncubatorPrivate, &QQmlIncubatorPrivate::recursion> watcher(this);
    // get a copy of the engine pointer as it might get reset;
    QQmlEnginePrivate *enginePriv = this->enginePriv;

    // Incubating objects takes quite a bit more stack space than our usual V4 function
    enum { EstimatedSizeInV4Frames = 2 };
    QV4::ExecutionEngineCallDepthRecorder<EstimatedSizeInV4Frames> callDepthRecorder(
                compilationUnit->engine);
    if (callDepthRecorder.hasOverflow()) {
        QQmlError error;
        error.setMessageType(QtCriticalMsg);
        error.setUrl(compilationUnit->url());
        error.setDescription(QQmlComponent::tr("Maximum call stack size exceeded."));
        errors << error;
        progress = QQmlIncubatorPrivate::Completed;
        goto finishIncubate;
    }

    if (!vmeGuard.isOK()) {
        QQmlError error;
        error.setMessageType(QtInfoMsg);
        error.setUrl(compilationUnit->url());
        error.setDescription(QQmlComponent::tr("Object or context destroyed during incubation"));
        errors << error;
        progress = QQmlIncubatorPrivate::Completed;

        goto finishIncubate;
    }

    vmeGuard.clear();

    if (progress == QQmlIncubatorPrivate::Execute) {
        enginePriv->referenceScarceResources();
        QObject *tresult = nullptr;
        tresult = creator->create(subComponentToCreate, /*parent*/nullptr, &i);
        if (!tresult)
            errors = creator->errors;
        else {
           RequiredProperties* requiredProperties = creator->requiredProperties();
           for (auto it = initialProperties.cbegin(); it != initialProperties.cend(); ++it) {
               auto component = tresult;
               auto name = it.key();
               QQmlProperty prop = QQmlComponentPrivate::removePropertyFromRequired(
                           component, name, requiredProperties, QQmlEnginePrivate::get(enginePriv));
               if (!prop.isValid() || !prop.write(it.value())) {
                   QQmlError error{};
                   error.setUrl(compilationUnit->url());
                   error.setDescription(QLatin1String("Could not set property %1").arg(name));
                   errors.push_back(error);
               }
           }
        }
        enginePriv->dereferenceScarceResources();

        if (watcher.hasRecursed())
            return;

        result = tresult;
        if (errors.isEmpty() && result == nullptr)
            goto finishIncubate;

        if (result) {
            QQmlData *ddata = QQmlData::get(result);
            Q_ASSERT(ddata);
            //see QQmlComponent::beginCreate for explanation of indestructible
            ddata->indestructible = true;
            ddata->explicitIndestructibleSet = true;
            ddata->rootObjectInCreation = false;
            if (q) {
                q->setInitialState(result);
                if (creator && !creator->requiredProperties()->empty()) {
                    const RequiredProperties *unsetRequiredProperties = creator->requiredProperties();
                    for (const auto& unsetRequiredProperty: *unsetRequiredProperties)
                        errors << QQmlComponentPrivate::unsetRequiredPropertyToQQmlError(unsetRequiredProperty);
                }
            }
        }

        if (watcher.hasRecursed())
            return;

        if (errors.isEmpty())
            progress = QQmlIncubatorPrivate::Completing;
        else
            progress = QQmlIncubatorPrivate::Completed;

        changeStatus(calculateStatus());

        if (watcher.hasRecursed())
            return;

        if (i.shouldInterrupt())
            goto finishIncubate;
    }

    if (progress == QQmlIncubatorPrivate::Completing) {
        do {
            if (watcher.hasRecursed())
                return;

            if (creator->finalize(i)) {
                rootContext = creator->rootContext();
                progress = QQmlIncubatorPrivate::Completed;
                goto finishIncubate;
            }
        } while (!i.shouldInterrupt());
    }

finishIncubate:
    if (progress == QQmlIncubatorPrivate::Completed && waitingFor.isEmpty()) {
        QExplicitlySharedDataPointer<QQmlIncubatorPrivate> isWaiting = waitingOnMe;
        clear();

        if (isWaiting) {
            QRecursionWatcher<QQmlIncubatorPrivate, &QQmlIncubatorPrivate::recursion> watcher(isWaiting.data());
            changeStatus(calculateStatus());
            if (!watcher.hasRecursed())
                isWaiting->incubate(i);
        } else {
            changeStatus(calculateStatus());
        }

        enginePriv->inProgressCreations--;

        if (0 == enginePriv->inProgressCreations) {
            while (enginePriv->erroredBindings)
                enginePriv->warning(enginePriv->erroredBindings->removeError());
        }
    } else if (!creator.isNull()) {
        vmeGuard.guard(creator.data());
    }
}

/*!
    \internal
    This is used to mimic the behavior of incubate when the
    Component we want to incubate refers to a creatable
    QQmlType (i.e., it is the result of loadFromModule).
 */
void QQmlIncubatorPrivate::incubateCppBasedComponent(QQmlComponent *component, QQmlContext *context)
{
    auto compPriv = QQmlComponentPrivate::get(component);
    Q_ASSERT(compPriv->loadedType.isCreatable());
    std::unique_ptr<QObject> object(component->beginCreate(context));
    component->setInitialProperties(object.get(), initialProperties);
    if (auto props = compPriv->state.requiredProperties()) {
        requiredPropertiesFromComponent = props;
        requiredPropertiesFromComponent.setTag(HadTopLevelRequired::Yes);
    }
    q->setInitialState(object.get());
    if (requiredPropertiesFromComponent && !requiredPropertiesFromComponent->isEmpty()) {
        for (const RequiredPropertyInfo &unsetRequiredProperty :
             std::as_const(*requiredPropertiesFromComponent)) {
            errors << QQmlComponentPrivate::unsetRequiredPropertyToQQmlError(unsetRequiredProperty);
        }
    } else {
        compPriv->completeCreate();
        result = object.release();
        progress = QQmlIncubatorPrivate::Completed;
    }
    changeStatus(calculateStatus());

}

/*!
Incubate objects for \a msecs, or until there are no more objects to incubate.
*/
void QQmlIncubationController::incubateFor(int msecs)
{
    if (!d || !d->incubatorCount)
        return;

    QDeadlineTimer deadline(msecs);
    QQmlInstantiationInterrupt i(deadline);
    do {
        static_cast<QQmlIncubatorPrivate*>(d->incubatorList.first())->incubate(i);
    } while (d && d->incubatorCount != 0 && !i.shouldInterrupt());
}

/*!
\since 5.15

Incubate objects while the atomic bool pointed to by \a flag is true,
or until there are no more objects to incubate, or up to \a msecs if \a
msecs is not zero.

Generally this method is used in conjunction with a thread or a UNIX signal that sets
the bool pointed to by \a flag to false when it wants incubation to be interrupted.

\note \a flag is read using acquire memory ordering.
*/
void QQmlIncubationController::incubateWhile(std::atomic<bool> *flag, int msecs)
{
    if (!d || !d->incubatorCount)
        return;

    QQmlInstantiationInterrupt i(flag, msecs ? QDeadlineTimer(msecs) : QDeadlineTimer::Forever);
    do {
        static_cast<QQmlIncubatorPrivate*>(d->incubatorList.first())->incubate(i);
    } while (d && d->incubatorCount != 0 && !i.shouldInterrupt());
}

/*!
\class QQmlIncubator
\brief The QQmlIncubator class allows QML objects to be created asynchronously.
\inmodule QtQml

Creating QML objects - like delegates in a view, or a new page in an application - can take
a noticeable amount of time, especially on resource constrained mobile devices.  When an
application uses QQmlComponent::create() directly, the QML object instance is created
synchronously which, depending on the complexity of the object,  can cause noticeable pauses or
stutters in the application.

The use of QQmlIncubator gives more control over the creation of a QML object,
including allowing it to be created asynchronously using application idle time. The following
example shows a simple use of QQmlIncubator.

\code
// Initialize the incubator
QQmlIncubator incubator;
component->create(incubator);
\endcode

Let the incubator run for a while (normally by returning control to the event loop),
then poll it. There are a number of ways to get back to the incubator later. You may
want to connect to one of the signals sent by \l{QQuickWindow}, or you may want to run
a \l{QTimer} especially for that. You may also need the object for some specific
purpose and poll the incubator when that purpose arises.

\code
// Poll the incubator
if (incubator.isReady()) {
    QObject *object = incubator.object();
    // Use created object
}
\endcode

Asynchronous incubators are controlled by a \l{QQmlIncubationController} that is
set on the \l{QQmlEngine}, which lets the engine know when the application is idle and
incubating objects should be processed.  If an incubation controller is not set on the
\l{QQmlEngine}, \l{QQmlIncubator} creates objects synchronously regardless of the
specified IncubationMode. By default, no incubation controller is set. However,
\l{QQuickView}, \l{QQuickWindow} and \l{QQuickWidget} all set incubation controllers
on their respective \l{QQmlEngine}s. These incubation controllers space out incubations
across multiple frames while the view is being rendered.

QQmlIncubator supports three incubation modes:
\list
\li Synchronous The creation occurs synchronously.  That is, once the
QQmlComponent::create() call returns, the incubator will already be in either the
Error or Ready state.  A synchronous incubator has no real advantage compared to using
the synchronous creation methods on QQmlComponent directly, but it may simplify an
application's implementation to use the same API for both synchronous and asynchronous
creations.

\li Asynchronous (default) The creation occurs asynchronously, assuming a
QQmlIncubatorController is set on the QQmlEngine.

The incubator will remain in the Loading state until either the creation is complete or an error
occurs.  The statusChanged() callback can be used to be notified of status changes.

Applications should use the Asynchronous incubation mode to create objects that are not needed
immediately.  For example, the ListView type uses Asynchronous incubation to create objects
that are slightly off screen while the list is being scrolled.  If, during asynchronous creation,
the object is needed immediately the QQmlIncubator::forceCompletion() method can be called
to complete the creation process synchronously.

\li AsynchronousIfNested The creation will occur asynchronously if part of a nested asynchronous
creation, or synchronously if not.

In most scenarios where a QML component wants the appearance of a synchronous
instantiation, it should use this mode.

This mode is best explained with an example.  When the ListView type is first created, it needs
to populate itself with an initial set of delegates to show.  If the ListView was 400 pixels high,
and each delegate was 100 pixels high, it would need to create four initial delegate instances.  If
the ListView used the Asynchronous incubation mode, the ListView would always be created empty and
then, sometime later, the four initial items would appear.

Conversely, if the ListView was to use the Synchronous incubation mode it would behave correctly
but it may introduce stutters into the application.  As QML would have to stop and instantiate the
ListView's delegates synchronously, if the ListView was part of a QML component that was being
instantiated asynchronously this would undo much of the benefit of asynchronous instantiation.

The AsynchronousIfNested mode reconciles this problem.  By using AsynchronousIfNested, the ListView
delegates are instantiated asynchronously if the ListView itself is already part of an asynchronous
instantiation, and synchronously otherwise.  In the case of a nested asynchronous instantiation, the
outer asynchronous instantiation will not complete until after all the nested instantiations have also
completed.  This ensures that by the time the outer asynchronous instantitation completes, inner
items like ListView have already completed loading their initial delegates.

It is almost always incorrect to use the Synchronous incubation mode - elements or components that
want the appearance of synchronous instantiation, but without the downsides of introducing freezes
or stutters into the application, should use the AsynchronousIfNested incubation mode.
\endlist
*/

/*!
Create a new incubator with the specified \a mode
*/
QQmlIncubator::QQmlIncubator(IncubationMode mode)
    : d(new QQmlIncubatorPrivate(this, mode))
{
    d->ref.ref();
}

/*! \internal */
QQmlIncubator::~QQmlIncubator()
{
    d->q = nullptr;

    if (!d->ref.deref()) {
        delete d;
    }
    d = nullptr;
}

/*!
\enum QQmlIncubator::IncubationMode

Specifies the mode the incubator operates in.  Regardless of the incubation mode, a
QQmlIncubator will behave synchronously if the QQmlEngine does not have
a QQmlIncubationController set.

\value Asynchronous The object will be created asynchronously.
\value AsynchronousIfNested If the object is being created in a context that is already part
of an asynchronous creation, this incubator will join that existing incubation and execute
asynchronously.  The existing incubation will not become Ready until both it and this
incubation have completed.  Otherwise, the incubation will execute synchronously.
\value Synchronous The object will be created synchronously.
*/

/*!
\enum QQmlIncubator::Status

Specifies the status of the QQmlIncubator.

\value Null Incubation is not in progress.  Call QQmlComponent::create() to begin incubating.
\value Ready The object is fully created and can be accessed by calling object().
\value Loading The object is in the process of being created.
\value Error An error occurred.  The errors can be access by calling errors().
*/

/*!
Clears the incubator.  Any in-progress incubation is aborted.  If the incubator is in the
Ready state, the created object is \b not deleted.
*/
void QQmlIncubator::clear()
{
    QRecursionWatcher<QQmlIncubatorPrivate, &QQmlIncubatorPrivate::recursion> watcher(d);

    Status s = status();

    if (s == Null)
        return;

    QQmlEnginePrivate *enginePriv = d->enginePriv;
    if (s == Loading) {
        Q_ASSERT(d->compilationUnit);
        if (d->result) d->result->deleteLater();
        d->result = nullptr;
    }

    d->clear();

    Q_ASSERT(d->compilationUnit.isNull());
    Q_ASSERT(d->waitingOnMe.data() == nullptr);
    Q_ASSERT(d->waitingFor.isEmpty());

    d->errors.clear();
    d->progress = QQmlIncubatorPrivate::Execute;
    d->result = nullptr;

    if (s == Loading) {
        Q_ASSERT(enginePriv);

        enginePriv->inProgressCreations--;
        if (0 == enginePriv->inProgressCreations) {
            while (enginePriv->erroredBindings)
                enginePriv->warning(enginePriv->erroredBindings->removeError());
        }
    }

    d->changeStatus(Null);
}

/*!
Force any in-progress incubation to finish synchronously.  Once this call
returns, the incubator will not be in the Loading state.
*/
void QQmlIncubator::forceCompletion()
{
    QQmlInstantiationInterrupt i;
    d->forceCompletion(i);
}

/*!
Returns true if the incubator's status() is Null.
*/
bool QQmlIncubator::isNull() const
{
    return status() == Null;
}

/*!
Returns true if the incubator's status() is Ready.
*/
bool QQmlIncubator::isReady() const
{
    return status() == Ready;
}

/*!
Returns true if the incubator's status() is Error.
*/
bool QQmlIncubator::isError() const
{
    return status() == Error;
}

/*!
Returns true if the incubator's status() is Loading.
*/
bool QQmlIncubator::isLoading() const
{
    return status() == Loading;
}

/*!
Return the list of errors encountered while incubating the object.
*/
QList<QQmlError> QQmlIncubator::errors() const
{
    return d->errors;
}

/*!
Return the incubation mode passed to the QQmlIncubator constructor.
*/
QQmlIncubator::IncubationMode QQmlIncubator::incubationMode() const
{
    return d->mode;
}

/*!
Return the current status of the incubator.
*/
QQmlIncubator::Status QQmlIncubator::status() const
{
    return d->status;
}

/*!
Return the incubated object if the status is Ready, otherwise 0.
*/
QObject *QQmlIncubator::object() const
{
    if (status() != Ready)
        return nullptr;
    else
        return d->result;
}

/*!
Return a pointer to a list of properties which are required but haven't
been set yet.
This list can be modified, so that subclasses which implement special logic
setInitialProperties can mark properties set there as no longer required.

\sa QQmlIncubator::setInitialProperties
\since 5.15
*/
RequiredProperties *QQmlIncubatorPrivate::requiredProperties()
{
    if (creator)
        return creator->requiredProperties();
    else
        return requiredPropertiesFromComponent.data();
}

bool QQmlIncubatorPrivate::hadTopLevelRequiredProperties() const
{
    if (creator)
        return creator->componentHadTopLevelRequiredProperties();
    else
        return requiredPropertiesFromComponent.tag() == HadTopLevelRequired::Yes;
}

/*!
Stores a mapping from property names to initial values, contained in
\a initialProperties, with which the incubated component will be initialized.

\sa QQmlComponent::setInitialProperties
\since 5.15
*/
void QQmlIncubator::setInitialProperties(const QVariantMap &initialProperties)
{
    d->initialProperties = initialProperties;
}

/*!
Called when the status of the incubator changes.  \a status is the new status.

The default implementation does nothing.
*/
void QQmlIncubator::statusChanged(Status status)
{
    Q_UNUSED(status);
}

/*!
Called after the \a object is first created, but before complex property
bindings are evaluated and, if applicable, QQmlParserStatus::componentComplete()
is called. This is equivalent to the point between QQmlComponent::beginCreate()
and QQmlComponent::completeCreate(), and can be used to assign initial values
to the object's properties.

The default implementation does nothing.

\note Simple bindings such as numeric literals are evaluated before
setInitialState() is called. The categorization of bindings into simple and
complex ones is intentionally unspecified and may change between versions of
Qt and depending on whether and how you are using \l{qmlcachegen}. You should
not rely on any particular binding to be evaluated either before or after
setInitialState() is called. For example, a constant expression like
\e{MyType.EnumValue} may be recognized as such at compile time or deferred
to be executed as binding. The same holds for constant expressions like
\e{-(5)} or \e{"a" + " constant string"}.
*/
void QQmlIncubator::setInitialState(QObject *object)
{
    Q_UNUSED(object);
}

void QQmlIncubatorPrivate::changeStatus(QQmlIncubator::Status s)
{
    if (s == status)
        return;

    status = s;
    if (q)
        q->statusChanged(status);
}

QQmlIncubator::Status QQmlIncubatorPrivate::calculateStatus() const
{
    if (!errors.isEmpty())
        return QQmlIncubator::Error;
    else if (result && progress == QQmlIncubatorPrivate::Completed && waitingFor.isEmpty())
        return QQmlIncubator::Ready;
    else if (compilationUnit)
        return QQmlIncubator::Loading;
    else
        return QQmlIncubator::Null;
}

