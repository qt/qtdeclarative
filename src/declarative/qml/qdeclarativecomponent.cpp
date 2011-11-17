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

#include "qdeclarativecomponent.h"
#include "qdeclarativecomponent_p.h"

#include "qdeclarativecompiler_p.h"
#include "qdeclarativecontext_p.h"
#include "qdeclarativeengine_p.h"
#include "qdeclarativevme_p.h"
#include "qdeclarative.h"
#include "qdeclarativeengine.h"
#include "qdeclarativebinding_p.h"
#include "qdeclarativebinding_p_p.h"
#include "qdeclarativeglobal_p.h"
#include "qdeclarativescript_p.h"
#include <private/qdeclarativedebugtrace_p.h>
#include <private/qdeclarativeenginedebugservice_p.h>
#include "qdeclarativeincubator.h"
#include "qdeclarativeincubator_p.h"

#include <private/qv8engine_p.h>
#include <private/qv8include_p.h>

#include <QStack>
#include <QStringList>
#include <QtCore/qdebug.h>
#include <qdeclarativeinfo.h>

QT_BEGIN_NAMESPACE

class QDeclarativeComponentExtension : public QV8Engine::Deletable
{
public:
    QDeclarativeComponentExtension(QV8Engine *);
    virtual ~QDeclarativeComponentExtension();

    v8::Persistent<v8::Function> incubationConstructor;
    v8::Persistent<v8::Script> initialProperties;
    v8::Persistent<v8::Function> forceCompletion;
};
static V8_DEFINE_EXTENSION(QDeclarativeComponentExtension, componentExtension);

/*
    Try to do what's necessary for a reasonable display of the type
    name, but no more (just enough for the client to do more extensive cleanup).

    Should only be called when debugging is enabled.
*/
static inline QString buildTypeNameForDebug(const QMetaObject *metaObject)
{
    static const QString qmlMarker(QLatin1String("_QML"));
    static const QChar underscore(QLatin1Char('_'));
    static const QChar asterisk(QLatin1Char('*'));
    QDeclarativeType *type = QDeclarativeMetaType::qmlType(metaObject);
    QString typeName = type ? type->qmlTypeName() : QString::fromUtf8(metaObject->className());
    if (!type) {
        //### optimize further?
        int marker = typeName.indexOf(qmlMarker);
        if (marker != -1 && marker < typeName.count() - 1) {
            if (typeName[marker + 1] == underscore) {
                const QString className = typeName.left(marker) + asterisk;
                type = QDeclarativeMetaType::qmlType(QMetaType::type(className.toUtf8()));
                if (type)
                    typeName = type->qmlTypeName();
            }
        }
    }
    return typeName;
}

/*!
    \class QDeclarativeComponent
    \since 4.7
    \brief The QDeclarativeComponent class encapsulates a QML component definition.
    \mainclass

    Components are reusable, encapsulated QML elements with well-defined interfaces.
    They are often defined in \l {qdeclarativedocuments.html}{Component Files}.

    A QDeclarativeComponent instance can be created from a QML file.
    For example, if there is a \c main.qml file like this:

    \qml
    import QtQuick 1.0

    Item {
        width: 200
        height: 200
    }
    \endqml

    The following code loads this QML file as a component, creates an instance of
    this component using create(), and then queries the \l Item's \l {Item::}{width}
    value:

    \code
    QDeclarativeEngine *engine = new QDeclarativeEngine;
    QDeclarativeComponent component(engine, QUrl::fromLocalFile("main.qml"));

    QObject *myObject = component.create();
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(myObject);
    int width = item->width();  // width = 200
    \endcode


    \section2 Network Components

    If the URL passed to QDeclarativeComponent is a network resource, or if the QML document references a
    network resource, the QDeclarativeComponent has to fetch the network data before it is able to create
    objects.  In this case, the QDeclarativeComponent will have a \l {QDeclarativeComponent::Loading}{Loading}
    \l {QDeclarativeComponent::status()}{status}.  An application will have to wait until the component
    is \l {QDeclarativeComponent::Ready}{Ready} before calling \l {QDeclarativeComponent::create()}.

    The following example shows how to load a QML file from a network resource.  After creating
    the QDeclarativeComponent, it tests whether the component is loading.  If it is, it connects to the
    QDeclarativeComponent::statusChanged() signal and otherwise calls the \c {continueLoading()} method
    directly. Note that QDeclarativeComponent::isLoading() may be false for a network component if the
    component has been cached and is ready immediately.

    \code
    MyApplication::MyApplication()
    {
        // ...
        component = new QDeclarativeComponent(engine, QUrl("http://www.example.com/main.qml"));
        if (component->isLoading())
            QObject::connect(component, SIGNAL(statusChanged(QDeclarativeComponent::Status)),
                             this, SLOT(continueLoading()));
        else
            continueLoading();
    }

    void MyApplication::continueLoading()
    {
        if (component->isError()) {
            qWarning() << component->errors();
        } else {
            QObject *myObject = component->create();
        }
    }
    \endcode

    \sa {Using QML Bindings in C++ Applications}, {Integrating QML Code with Existing Qt UI Code}
*/

/*!
    \qmlclass Component QDeclarativeComponent
    \ingroup qml-utility-elements
    \since 4.7
    \brief The Component element encapsulates a QML component definition.

    Components are reusable, encapsulated QML elements with well-defined interfaces.

    Components are often defined by \l {qdeclarativedocuments.html}{component files} -
    that is, \c .qml files. The \e Component element essentially allows QML components
    to be defined inline, within a \l {QML Document}{QML document}, rather than as a separate QML file.
    This may be useful for reusing a small component within a QML file, or for defining
    a component that logically belongs with other QML components within a file.

    For example, here is a component that is used by multiple \l Loader objects.
    It contains a single item, a \l Rectangle:

    \snippet doc/src/snippets/declarative/component.qml 0

    Notice that while a \l Rectangle by itself would be automatically 
    rendered and displayed, this is not the case for the above rectangle
    because it is defined inside a \c Component. The component encapsulates the
    QML elements within, as if they were defined in a separate QML
    file, and is not loaded until requested (in this case, by the
    two \l Loader objects).

    Defining a \c Component is similar to defining a \l {QML Document}{QML document}.
    A QML document has a single top-level item that defines the behaviors and
    properties of that component, and cannot define properties or behaviors outside
    of that top-level item. In the same way, a \c Component definition contains a single
    top level item (which in the above example is a \l Rectangle) and cannot define any
    data outside of this item, with the exception of an \e id (which in the above example
    is \e redSquare).

    The \c Component element is commonly used to provide graphical components
    for views. For example, the ListView::delegate property requires a \c Component
    to specify how each list item is to be displayed.

    \c Component objects can also be created dynamically using
    \l{QML:Qt::createComponent()}{Qt.createComponent()}.
*/

/*!
    \qmlattachedsignal Component::onCompleted()

    Emitted after component "startup" has completed.  This can be used to
    execute script code at startup, once the full QML environment has been
    established.

    The \c {Component::onCompleted} attached property can be applied to
    any element.  The order of running the \c onCompleted scripts is
    undefined.

    \qml
    Rectangle {
        Component.onCompleted: console.log("Completed Running!")
        Rectangle {
            Component.onCompleted: console.log("Nested Completed Running!")
        }
    }
    \endqml
*/

/*!
    \qmlattachedsignal Component::onDestruction()

    Emitted as the component begins destruction.  This can be used to undo
    work done in the onCompleted signal, or other imperative code in your
    application.

    The \c {Component::onDestruction} attached property can be applied to
    any element.  However, it applies to the destruction of the component as
    a whole, and not the destruction of the specific object.  The order of
    running the \c onDestruction scripts is undefined.

    \qml
    Rectangle {
        Component.onDestruction: console.log("Destruction Beginning!")
        Rectangle {
            Component.onDestruction: console.log("Nested Destruction Beginning!")
        }
    }
    \endqml

    \sa QtDeclarative
*/

/*!
    \enum QDeclarativeComponent::Status
    
    Specifies the loading status of the QDeclarativeComponent.

    \value Null This QDeclarativeComponent has no data.  Call loadUrl() or setData() to add QML content.
    \value Ready This QDeclarativeComponent is ready and create() may be called.
    \value Loading This QDeclarativeComponent is loading network data.
    \value Error An error has occurred.  Call errors() to retrieve a list of \{QDeclarativeError}{errors}.
*/

void QDeclarativeComponentPrivate::typeDataReady(QDeclarativeTypeData *)
{
    Q_Q(QDeclarativeComponent);

    Q_ASSERT(typeData);

    fromTypeData(typeData);
    typeData = 0;

    emit q->statusChanged(q->status());
}

void QDeclarativeComponentPrivate::typeDataProgress(QDeclarativeTypeData *, qreal p)
{
    Q_Q(QDeclarativeComponent);

    progress = p;

    emit q->progressChanged(p);
}

void QDeclarativeComponentPrivate::fromTypeData(QDeclarativeTypeData *data)
{
    url = data->finalUrl();
    QDeclarativeCompiledData *c = data->compiledData();

    if (!c) {
        Q_ASSERT(data->isError());
        state.errors = data->errors();
    } else {
        cc = c;
    }

    data->release();
}

void QDeclarativeComponentPrivate::clear()
{
    if (typeData) {
        typeData->unregisterCallback(this);
        typeData->release();
        typeData = 0;
    }
        
    if (cc) { 
        cc->release();
        cc = 0;
    }
}

/*!
    \internal
*/
QDeclarativeComponent::QDeclarativeComponent(QObject *parent)
    : QObject(*(new QDeclarativeComponentPrivate), parent)
{
}

/*!
    Destruct the QDeclarativeComponent.
*/
QDeclarativeComponent::~QDeclarativeComponent()
{
    Q_D(QDeclarativeComponent);

    if (d->state.completePending) {
        qWarning("QDeclarativeComponent: Component destroyed while completion pending");
        d->completeCreate();
    }

    if (d->typeData) {
        d->typeData->unregisterCallback(d);
        d->typeData->release();
    }
    if (d->cc)
        d->cc->release();
}

/*!
    \qmlproperty enumeration Component::status
    This property holds the status of component loading.  It can be one of:
    \list
    \o Component.Null - no data is available for the component
    \o Component.Ready - the component has been loaded, and can be used to create instances.
    \o Component.Loading - the component is currently being loaded
    \o Component.Error - an error occurred while loading the component.
               Calling errorString() will provide a human-readable description of any errors.
    \endlist
 */

/*!
    \property QDeclarativeComponent::status
    The component's current \l{QDeclarativeComponent::Status} {status}.
 */
QDeclarativeComponent::Status QDeclarativeComponent::status() const
{
    Q_D(const QDeclarativeComponent);

    if (d->typeData)
        return Loading;
    else if (!d->state.errors.isEmpty())
        return Error;
    else if (d->engine && d->cc)
        return Ready;
    else
        return Null;
}

/*!
    Returns true if status() == QDeclarativeComponent::Null.
*/
bool QDeclarativeComponent::isNull() const
{
    return status() == Null;
}

/*!
    Returns true if status() == QDeclarativeComponent::Ready.
*/
bool QDeclarativeComponent::isReady() const
{
    return status() == Ready;
}

/*!
    Returns true if status() == QDeclarativeComponent::Error.
*/
bool QDeclarativeComponent::isError() const
{
    return status() == Error;
}

/*!
    Returns true if status() == QDeclarativeComponent::Loading.
*/
bool QDeclarativeComponent::isLoading() const
{
    return status() == Loading;
}

/*!
    \qmlproperty real Component::progress
    The progress of loading the component, from 0.0 (nothing loaded)
    to 1.0 (finished).
*/

/*!
    \property QDeclarativeComponent::progress
    The progress of loading the component, from 0.0 (nothing loaded)
    to 1.0 (finished).
*/
qreal QDeclarativeComponent::progress() const
{
    Q_D(const QDeclarativeComponent);
    return d->progress;
}

/*!
    \fn void QDeclarativeComponent::progressChanged(qreal progress)

    Emitted whenever the component's loading progress changes.  \a progress will be the
    current progress between 0.0 (nothing loaded) and 1.0 (finished).
*/

/*!
    \fn void QDeclarativeComponent::statusChanged(QDeclarativeComponent::Status status)

    Emitted whenever the component's status changes.  \a status will be the
    new status.
*/

/*!
    Create a QDeclarativeComponent with no data and give it the specified
    \a engine and \a parent. Set the data with setData().
*/
QDeclarativeComponent::QDeclarativeComponent(QDeclarativeEngine *engine, QObject *parent)
    : QObject(*(new QDeclarativeComponentPrivate), parent)
{
    Q_D(QDeclarativeComponent);
    d->engine = engine;
}

/*!
    Create a QDeclarativeComponent from the given \a url and give it the
    specified \a parent and \a engine.

    Ensure that the URL provided is full and correct, in particular, use
    \l QUrl::fromLocalFile() when loading a file from the local filesystem.

    \sa loadUrl()
*/
QDeclarativeComponent::QDeclarativeComponent(QDeclarativeEngine *engine, const QUrl &url, QObject *parent)
: QObject(*(new QDeclarativeComponentPrivate), parent)
{
    Q_D(QDeclarativeComponent);
    d->engine = engine;
    loadUrl(url);
}

/*!
    Create a QDeclarativeComponent from the given \a fileName and give it the specified 
    \a parent and \a engine.

    \sa loadUrl()
*/
QDeclarativeComponent::QDeclarativeComponent(QDeclarativeEngine *engine, const QString &fileName, 
                           QObject *parent)
: QObject(*(new QDeclarativeComponentPrivate), parent)
{
    Q_D(QDeclarativeComponent);
    d->engine = engine;
    loadUrl(d->engine->baseUrl().resolved(QUrl::fromLocalFile(fileName)));
}

/*!
    \internal
*/
QDeclarativeComponent::QDeclarativeComponent(QDeclarativeEngine *engine, QDeclarativeCompiledData *cc, int start, QObject *parent)
    : QObject(*(new QDeclarativeComponentPrivate), parent)
{
    Q_D(QDeclarativeComponent);
    d->engine = engine;
    d->cc = cc;
    cc->addref();
    d->start = start;
    d->url = cc->url;
    d->progress = 1.0;
}

/*!
    Sets the QDeclarativeComponent to use the given QML \a data.  If \a url
    is provided, it is used to set the component name and to provide
    a base path for items resolved by this component.
*/
void QDeclarativeComponent::setData(const QByteArray &data, const QUrl &url)
{
    Q_D(QDeclarativeComponent);

    d->clear();

    d->url = url;

    QDeclarativeTypeData *typeData = QDeclarativeEnginePrivate::get(d->engine)->typeLoader.get(data, url);
    
    if (typeData->isCompleteOrError()) {
        d->fromTypeData(typeData);
    } else {
        d->typeData = typeData;
        d->typeData->registerCallback(d);
    }

    d->progress = 1.0;
    emit statusChanged(status());
    emit progressChanged(d->progress);
}

/*!
Returns the QDeclarativeContext the component was created in.  This is only
valid for components created directly from QML.
*/
QDeclarativeContext *QDeclarativeComponent::creationContext() const
{
    Q_D(const QDeclarativeComponent);
    if(d->creationContext)
        return d->creationContext->asQDeclarativeContext();

    return qmlContext(this);
}

/*!
    Load the QDeclarativeComponent from the provided \a url.

    Ensure that the URL provided is full and correct, in particular, use
    \l QUrl::fromLocalFile() when loading a file from the local filesystem.
*/
void QDeclarativeComponent::loadUrl(const QUrl &url)
{
    Q_D(QDeclarativeComponent);

    d->clear();

    if ((url.isRelative() && !url.isEmpty())
    || url.scheme() == QLatin1String("file")) // Workaround QTBUG-11929
        d->url = d->engine->baseUrl().resolved(url);
    else
        d->url = url;

    if (url.isEmpty()) {
        QDeclarativeError error;
        error.setDescription(tr("Invalid empty URL"));
        d->state.errors << error;
        return;
    }

    QDeclarativeTypeData *data = QDeclarativeEnginePrivate::get(d->engine)->typeLoader.get(d->url);

    if (data->isCompleteOrError()) {
        d->fromTypeData(data);
        d->progress = 1.0;
    } else {
        d->typeData = data;
        d->typeData->registerCallback(d);
        d->progress = data->progress();
    }

    emit statusChanged(status());
    emit progressChanged(d->progress);
}

/*!
    Return the list of errors that occurred during the last compile or create
    operation.  An empty list is returned if isError() is not set.
*/
QList<QDeclarativeError> QDeclarativeComponent::errors() const
{
    Q_D(const QDeclarativeComponent);
    if (isError())
        return d->state.errors;
    else
        return QList<QDeclarativeError>();
}

/*!
    \qmlmethod string Component::errorString()

    Returns a human-readable description of any errors.

    The string includes the file, location, and description of each error.
    If multiple errors are present they are separated by a newline character.

    If no errors are present, an empty string is returned.
*/

/*!
    \internal
    errorString is only meant as a way to get the errors in script
*/
QString QDeclarativeComponent::errorString() const
{
    Q_D(const QDeclarativeComponent);
    QString ret;
    if(!isError())
        return ret;
    foreach(const QDeclarativeError &e, d->state.errors) {
        ret += e.url().toString() + QLatin1Char(':') +
               QString::number(e.line()) + QLatin1Char(' ') +
               e.description() + QLatin1Char('\n');
    }
    return ret;
}

/*!
    \qmlproperty url Component::url
    The component URL.  This is the URL that was used to construct the component.
*/

/*!
    \property QDeclarativeComponent::url
    The component URL.  This is the URL passed to either the constructor,
    or the loadUrl() or setData() methods.
*/
QUrl QDeclarativeComponent::url() const
{
    Q_D(const QDeclarativeComponent);
    return d->url;
}

/*!
    \internal
*/
QDeclarativeComponent::QDeclarativeComponent(QDeclarativeComponentPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    Create an object instance from this component.  Returns 0 if creation
    failed.  \a context specifies the context within which to create the object
    instance.  

    If \a context is 0 (the default), it will create the instance in the
    engine' s \l {QDeclarativeEngine::rootContext()}{root context}.
*/
QObject *QDeclarativeComponent::create(QDeclarativeContext *context)
{
    Q_D(QDeclarativeComponent);

    if (!context)
        context = d->engine->rootContext();

    QObject *rv = beginCreate(context);
    completeCreate();
    return rv;
}

/*!
    This method provides more advanced control over component instance creation.
    In general, programmers should use QDeclarativeComponent::create() to create a 
    component.

    Create an object instance from this component.  Returns 0 if creation
    failed.  \a publicContext specifies the context within which to create the object
    instance.  

    When QDeclarativeComponent constructs an instance, it occurs in three steps:
    \list 1
    \i The object hierarchy is created, and constant values are assigned.
    \i Property bindings are evaluated for the the first time.
    \i If applicable, QDeclarativeParserStatus::componentComplete() is called on objects.
    \endlist 
    QDeclarativeComponent::beginCreate() differs from QDeclarativeComponent::create() in that it
    only performs step 1.  QDeclarativeComponent::completeCreate() must be called to 
    complete steps 2 and 3.

    This breaking point is sometimes useful when using attached properties to
    communicate information to an instantiated component, as it allows their
    initial values to be configured before property bindings take effect.
*/
QObject *QDeclarativeComponent::beginCreate(QDeclarativeContext *publicContext)
{
    Q_D(QDeclarativeComponent);

    Q_ASSERT(publicContext);
    QDeclarativeContextData *context = QDeclarativeContextData::get(publicContext);

    return d->beginCreate(context);
}

QObject *
QDeclarativeComponentPrivate::beginCreate(QDeclarativeContextData *context)
{
    Q_Q(QDeclarativeComponent);
    if (!context) {
        qWarning("QDeclarativeComponent: Cannot create a component in a null context");
        return 0;
    }

    if (!context->isValid()) {
        qWarning("QDeclarativeComponent: Cannot create a component in an invalid context");
        return 0;
    }

    if (context->engine != engine) {
        qWarning("QDeclarativeComponent: Must create component in context from the same QDeclarativeEngine");
        return 0;
    }

    if (state.completePending) {
        qWarning("QDeclarativeComponent: Cannot create new component instance before completing the previous");
        return 0;
    }

    if (!q->isReady()) {
        qWarning("QDeclarativeComponent: Component is not ready");
        return 0;
    }

    QDeclarativeEnginePrivate *enginePriv = QDeclarativeEnginePrivate::get(engine);

    bool isRoot = enginePriv->inProgressCreations == 0;
    enginePriv->inProgressCreations++;
    state.errors.clear();
    state.completePending = true;

    if (isRoot) 
        QDeclarativeDebugTrace::startRange(QDeclarativeDebugTrace::Creating);

    enginePriv->referenceScarceResources();
    state.vme.init(context, cc, start, creationContext);
    QObject *rv = state.vme.execute(&state.errors);
    enginePriv->dereferenceScarceResources();

    if (rv) {
        QDeclarativeData *ddata = QDeclarativeData::get(rv);
        Q_ASSERT(ddata);
        ddata->indestructible = true;
    }

    if (enginePriv->isDebugging && rv) {
        if (!context->isInternal)
            context->asQDeclarativeContextPrivate()->instances.append(rv);
        QDeclarativeEngineDebugService::instance()->objectCreated(engine, rv);
        if (isRoot) {
            QDeclarativeDebugTrace::rangeData(QDeclarativeDebugTrace::Creating, 
                                              buildTypeNameForDebug(rv->metaObject()));
            QDeclarativeData *data = QDeclarativeData::get(rv);
            Q_ASSERT(data);
            QDeclarativeDebugTrace::rangeLocation(QDeclarativeDebugTrace::Creating, 
                                                  cc->url, data->lineNumber);
        }
    }

    return rv;
}

void QDeclarativeComponentPrivate::beginDeferred(QDeclarativeEnginePrivate *enginePriv,
                                                 QObject *object, ConstructionState *state)
{
    enginePriv->inProgressCreations++;
    state->errors.clear();
    state->completePending = true;

    state->vme.initDeferred(object);
    state->vme.execute(&state->errors);
}

void QDeclarativeComponentPrivate::complete(QDeclarativeEnginePrivate *enginePriv, ConstructionState *state)
{
    if (state->completePending) {
        state->vme.complete();

        state->completePending = false;

        enginePriv->inProgressCreations--;

        if (0 == enginePriv->inProgressCreations) {
            while (enginePriv->erroredBindings) {
                enginePriv->warning(enginePriv->erroredBindings->error);
                enginePriv->erroredBindings->removeError();
            }
        }
    }
}

/*!
    This method provides more advanced control over component instance creation.
    In general, programmers should use QDeclarativeComponent::create() to create a 
    component.

    Complete a component creation begin with QDeclarativeComponent::beginCreate().
*/
void QDeclarativeComponent::completeCreate()
{
    Q_D(QDeclarativeComponent);
    d->completeCreate();
}

void QDeclarativeComponentPrivate::completeCreate()
{
    if (state.completePending) {
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);
        complete(ep, &state);

        QDeclarativeDebugTrace::endRange(QDeclarativeDebugTrace::Creating);
    }
}

QDeclarativeComponentAttached::QDeclarativeComponentAttached(QObject *parent)
: QObject(parent), prev(0), next(0)
{
}

QDeclarativeComponentAttached::~QDeclarativeComponentAttached()
{
    if (prev) *prev = next;
    if (next) next->prev = prev;
    prev = 0;
    next = 0;
}

/*!
    \internal
*/
QDeclarativeComponentAttached *QDeclarativeComponent::qmlAttachedProperties(QObject *obj)
{
    QDeclarativeComponentAttached *a = new QDeclarativeComponentAttached(obj);

    QDeclarativeEngine *engine = qmlEngine(obj);
    if (!engine)
        return a;

    if (QDeclarativeEnginePrivate::get(engine)->activeVME) { // XXX should only be allowed during begin
        QDeclarativeEnginePrivate *p = QDeclarativeEnginePrivate::get(engine);
        a->add(&p->activeVME->componentAttached);
    } else {
        QDeclarativeData *d = QDeclarativeData::get(obj);
        Q_ASSERT(d);
        Q_ASSERT(d->context);
        a->add(&d->context->componentAttached);
    }

    return a;
}

void QDeclarativeComponent::create(QDeclarativeIncubator &i, QDeclarativeContext *context,
                                   QDeclarativeContext *forContext)
{
    Q_D(QDeclarativeComponent);

    if (!context) 
        context = d->engine->rootContext();

    QDeclarativeContextData *contextData = QDeclarativeContextData::get(context);
    QDeclarativeContextData *forContextData = contextData;
    if (forContext) forContextData = QDeclarativeContextData::get(forContext);

    if (!contextData->isValid()) {
        qWarning("QDeclarativeComponent: Cannot create a component in an invalid context");
        return;
    }

    if (contextData->engine != d->engine) {
        qWarning("QDeclarativeComponent: Must create component in context from the same QDeclarativeEngine");
        return;
    }

    if (!isReady()) {
        qWarning("QDeclarativeComponent: Component is not ready");
        return;
    }

    i.clear();
    QDeclarativeIncubatorPrivate *p = i.d;

    QDeclarativeEnginePrivate *enginePriv = QDeclarativeEnginePrivate::get(d->engine);

    p->component = d->cc; p->component->addref();
    p->vme.init(contextData, d->cc, d->start, d->creationContext);

    enginePriv->incubate(i, forContextData);
}

class QV8IncubatorResource : public QV8ObjectResource,
                             public QDeclarativeIncubator
{
V8_RESOURCE_TYPE(IncubatorType)
public:
    QV8IncubatorResource(QV8Engine *engine, IncubationMode = Asynchronous);

    static v8::Handle<v8::Value> StatusChangedGetter(v8::Local<v8::String>, 
                                                     const v8::AccessorInfo& info);
    static v8::Handle<v8::Value> StatusGetter(v8::Local<v8::String>, 
                                              const v8::AccessorInfo& info);
    static v8::Handle<v8::Value> ObjectGetter(v8::Local<v8::String>, 
                                              const v8::AccessorInfo& info);
    static v8::Handle<v8::Value> ForceCompletionGetter(v8::Local<v8::String>, 
                                                       const v8::AccessorInfo& info);
    static v8::Handle<v8::Value> ForceCompletion(const v8::Arguments &args);

    static void StatusChangedSetter(v8::Local<v8::String>, v8::Local<v8::Value> value, 
                                    const v8::AccessorInfo& info);

    void dispose();

    v8::Persistent<v8::Object> me;
    QDeclarativeGuard<QObject> parent;
    v8::Persistent<v8::Value> valuemap;
    v8::Persistent<v8::Object> qmlGlobal;
protected:
    virtual void statusChanged(Status);
    virtual void setInitialState(QObject *);
};

static void QDeclarativeComponent_setQmlParent(QObject *me, QObject *parent)
{
    if (parent) {
        me->setParent(parent);
        typedef QDeclarativePrivate::AutoParentFunction APF;
        QList<APF> functions = QDeclarativeMetaType::parentFunctions();

        bool needParent = false;
        for (int ii = 0; ii < functions.count(); ++ii) {
            QDeclarativePrivate::AutoParentResult res = functions.at(ii)(me, parent);
            if (res == QDeclarativePrivate::Parented) {
                needParent = false;
                break;
            } else if (res == QDeclarativePrivate::IncompatibleParent) {
                needParent = true;
            }
        }
        if (needParent) 
            qWarning("QDeclarativeComponent: Created graphical object was not "
                     "placed in the graphics scene.");
    }
}

/*!
    \qmlmethod object Component::createObject(Item parent, object properties)

    Creates and returns an object instance of this component that will have
    the given \a parent and \a properties. The \a properties argument is optional.
    Returns null if object creation fails.

    The object will be created in the same context as the one in which the component
    was created. This function will always return null when called on components
    which were not created in QML.

    If you wish to create an object without setting a parent, specify \c null for
    the \a parent value. Note that if the returned object is to be displayed, you 
    must provide a valid \a parent value or set the returned object's \l{Item::parent}{parent} 
    property, or else the object will not be visible.

    If a \a parent is not provided to createObject(), a reference to the returned object must be held so that
    it is not destroyed by the garbage collector.  This is true regardless of whether \l{Item::parent} is set afterwards,
    since setting the Item parent does not change object ownership; only the graphical parent is changed.

    As of QtQuick 1.1, this method accepts an optional \a properties argument that specifies a
    map of initial property values for the created object. These values are applied before object
    creation is finalized. (This is more efficient than setting property values after object creation,
    particularly where large sets of property values are defined, and also allows property bindings
    to be set up before the object is created.)

    The \a properties argument is specified as a map of property-value items. For example, the code
    below creates an object with initial \c x and \c y values of 100 and 200, respectively:

    \js
        var component = Qt.createComponent("Button.qml");
        if (component.status == Component.Ready)
            component.createObject(parent, {"x": 100, "y": 100});
    \endjs

    Dynamically created instances can be deleted with the \c destroy() method.
    See \l {Dynamic Object Management in QML} for more information.
*/
void QDeclarativeComponent::createObject(QDeclarativeV8Function *args)
{
    Q_D(QDeclarativeComponent);
    Q_ASSERT(d->engine);
    Q_ASSERT(args);

    QObject *parent = 0;
    v8::Local<v8::Object> valuemap;

    if (args->Length() >= 1) 
        parent = args->engine()->toQObject((*args)[0]);

    if (args->Length() >= 2) {
        v8::Local<v8::Value> v = (*args)[1];
        if (!v->IsObject() || v->IsArray()) {
            qmlInfo(this) << tr("createObject: value is not an object");
            args->returnValue(v8::Null());
            return;
        }
        valuemap = v8::Local<v8::Object>::Cast(v);
    }

    QV8Engine *v8engine = args->engine();

    QDeclarativeContext *ctxt = creationContext();
    if (!ctxt) ctxt = d->engine->rootContext();

    QObject *rv = beginCreate(ctxt);

    if (!rv) {
        args->returnValue(v8::Null());
        return;
    }

    QDeclarativeComponent_setQmlParent(rv, parent);

    v8::Handle<v8::Value> ov = v8engine->newQObject(rv);
    Q_ASSERT(ov->IsObject());
    v8::Handle<v8::Object> object = v8::Handle<v8::Object>::Cast(ov);

    if (!valuemap.IsEmpty()) {
        QDeclarativeComponentExtension *e = componentExtension(v8engine);
        // Try catch isn't needed as the function itself is loaded with try/catch
        v8::Handle<v8::Value> function = e->initialProperties->Run(args->qmlGlobal());
        v8::Handle<v8::Value> args[] = { object, valuemap };
        v8::Handle<v8::Function>::Cast(function)->Call(v8engine->global(), 2, args);
    }

    d->completeCreate();

    Q_ASSERT(QDeclarativeData::get(rv));
    QDeclarativeData::get(rv)->setImplicitDestructible();

    if (!rv)
        args->returnValue(v8::Null());
    else
        args->returnValue(object);
}

/*!
    \qmlmethod object Component::incubateObject(Item parent, object properties, enum mode)

    Creates an incubator for instance of this component.  Incubators allow new component 
    instances to be instantiated asynchronously and not cause freezes in the UI.

    The \a parent argument specifies the parent the created instance will have.  Omitting the 
    parameter or passing null will create anobject with no parent.  In this case, a reference
    to the created object must be maintained by the application of the object will eventually
    be garbage collected.

    The \a properties argument is specified as a map of property-value items which will be
    set on the created object during its construction.  \a mode may be Qt.Synchronous or 
    Qt.Asynchronous and controls whether the instance is created synchronously or asynchronously. 
    The default is asynchronously.  In some circumstances, even if Qt.Synchronous is specified,
    the incubator may create the object asynchronously.  This happens if the component calling
    incubateObject() is itself being created asynchronously.

    All three arguments are optional.

    If successful, the method returns an incubator, otherwise null.  The incubator has the following
    properties:

    \list
    \i status The status of the incubator.  Valid values are Component.Ready, Component.Loading and
       Component.Error.
    \i object The created object instance.  Will only be available once the incubator is in the 
       Ready status.
    \i onStatusChanged Specifies a callback function to be invoked when the status changes.  The
       status is passed as a parameter to the callback.
    \i forceCompletion() Call to complete incubation synchronously.
    \endlist

    The following example demonstrates how to use an incubator:

    \js
        var component = Qt.createComponent("Button.qml");

        var incubator = component.incubateObject(parent, { x: 10, y: 10 });
        if (incubator.status != Component.Ready) {
            incubator.onStatusChanged = function(status) {
                if (status == Component.Ready) {
                    print ("Object", incubator.object, "is now ready!");
                }
            }
        } else {
            print ("Object", incubator.object, "is ready immediately!");
        }
    \endjs
*/

void QDeclarativeComponent::incubateObject(QDeclarativeV8Function *args)
{
    Q_D(QDeclarativeComponent);
    Q_ASSERT(d->engine);
    Q_UNUSED(d);
    Q_ASSERT(args);

    QObject *parent = 0;
    v8::Local<v8::Object> valuemap;
    QDeclarativeIncubator::IncubationMode mode = QDeclarativeIncubator::Asynchronous;

    if (args->Length() >= 1) 
        parent = args->engine()->toQObject((*args)[0]);

    if (args->Length() >= 2) {
        v8::Local<v8::Value> v = (*args)[1];
        if (v->IsNull()) {
        } else if (!v->IsObject() || v->IsArray()) {
            qmlInfo(this) << tr("createObject: value is not an object");
            args->returnValue(v8::Null());
            return;
        } else {
            valuemap = v8::Local<v8::Object>::Cast(v);
        }
    }

    if (args->Length() >= 3) {
        quint32 v = (*args)[2]->Uint32Value();
        if (v == 0)
            mode = QDeclarativeIncubator::Asynchronous;
        else if (v == 1)
            mode = QDeclarativeIncubator::AsynchronousIfNested;
    }

    QDeclarativeComponentExtension *e = componentExtension(args->engine());
    
    QV8IncubatorResource *r = new QV8IncubatorResource(args->engine(), mode);
    v8::Local<v8::Object> o = e->incubationConstructor->NewInstance();
    o->SetExternalResource(r);

    if (!valuemap.IsEmpty()) {
        r->valuemap = qPersistentNew(valuemap);
        r->qmlGlobal = qPersistentNew(args->qmlGlobal());
    }
    r->parent = parent;
    r->me = qPersistentNew(o);

    create(*r, creationContext());

    if (r->status() == QDeclarativeIncubator::Null) {
        r->dispose();
        args->returnValue(v8::Null());
    } else {
        args->returnValue(o);
    }
}

// XXX used by QSGLoader
void QDeclarativeComponentPrivate::initializeObjectWithInitialProperties(v8::Handle<v8::Object> qmlGlobal, v8::Handle<v8::Object> valuemap, QObject *toCreate)
{
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);
    QV8Engine *v8engine = ep->v8engine();

    v8::HandleScope handle_scope;
    v8::Context::Scope scope(v8engine->context());
    v8::Handle<v8::Value> ov = v8engine->newQObject(toCreate);
    Q_ASSERT(ov->IsObject());
    v8::Handle<v8::Object> object = v8::Handle<v8::Object>::Cast(ov);

    if (!valuemap.IsEmpty()) {
        QDeclarativeComponentExtension *e = componentExtension(v8engine);
        // Try catch isn't needed as the function itself is loaded with try/catch
        v8::Handle<v8::Value> function = e->initialProperties->Run(qmlGlobal);
        v8::Handle<v8::Value> args[] = { object, valuemap };
        v8::Handle<v8::Function>::Cast(function)->Call(v8engine->global(), 2, args);
    }

    QDeclarativeData *ddata = QDeclarativeData::get(toCreate);
    Q_ASSERT(ddata);
    ddata->setImplicitDestructible();
}


QDeclarativeComponentExtension::QDeclarativeComponentExtension(QV8Engine *engine)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope scope(engine->context());

    forceCompletion = qPersistentNew(V8FUNCTION(QV8IncubatorResource::ForceCompletion, engine));

    {
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->InstanceTemplate()->SetInternalFieldCount(1);
    ft->InstanceTemplate()->SetAccessor(v8::String::New("onStatusChanged"), 
                                        QV8IncubatorResource::StatusChangedGetter, 
                                        QV8IncubatorResource::StatusChangedSetter);
    ft->InstanceTemplate()->SetAccessor(v8::String::New("status"),
                                        QV8IncubatorResource::StatusGetter);
    ft->InstanceTemplate()->SetAccessor(v8::String::New("object"), 
                                        QV8IncubatorResource::ObjectGetter); 
    ft->InstanceTemplate()->SetAccessor(v8::String::New("forceCompletion"), 
                                        QV8IncubatorResource::ForceCompletionGetter); 
    incubationConstructor = qPersistentNew(ft->GetFunction());
    }

    {
#define INITIALPROPERTIES_SOURCE \
        "(function(object, values) {"\
            "try {"\
                "for(var property in values) {" \
                    "try {"\
                        "var properties = property.split(\".\");"\
                        "var o = object;"\
                        "for (var ii = 0; ii < properties.length - 1; ++ii) {"\
                            "o = o[properties[ii]];"\
                        "}"\
                        "o[properties[properties.length - 1]] = values[property];"\
                    "} catch(e) {}"\
                "}"\
            "} catch(e) {}"\
        "})"
    initialProperties = qPersistentNew(engine->qmlModeCompile(QLatin1String(INITIALPROPERTIES_SOURCE)));
#undef INITIALPROPERTIES_SOURCE
    }
}

v8::Handle<v8::Value> QV8IncubatorResource::ObjectGetter(v8::Local<v8::String>, 
                                                          const v8::AccessorInfo& info)
{
    QV8IncubatorResource *r = v8_resource_check<QV8IncubatorResource>(info.This());
    return r->engine->newQObject(r->object());
}

v8::Handle<v8::Value> QV8IncubatorResource::ForceCompletionGetter(v8::Local<v8::String>, 
                                                                  const v8::AccessorInfo& info)
{
    QV8IncubatorResource *r = v8_resource_check<QV8IncubatorResource>(info.This());
    return componentExtension(r->engine)->forceCompletion;
}

v8::Handle<v8::Value> QV8IncubatorResource::ForceCompletion(const v8::Arguments &args) 
{
    QV8IncubatorResource *r = v8_resource_cast<QV8IncubatorResource>(args.This());
    if (!r)
        V8THROW_TYPE("Not an incubator object");

    r->forceCompletion();

    return v8::Undefined();
}

v8::Handle<v8::Value> QV8IncubatorResource::StatusGetter(v8::Local<v8::String>, 
                                                         const v8::AccessorInfo& info)
{
    QV8IncubatorResource *r = v8_resource_check<QV8IncubatorResource>(info.This());
    return v8::Integer::NewFromUnsigned(r->status());
}

v8::Handle<v8::Value> QV8IncubatorResource::StatusChangedGetter(v8::Local<v8::String>, 
                                                                 const v8::AccessorInfo& info)
{
    return info.This()->GetInternalField(0);
}

void QV8IncubatorResource::StatusChangedSetter(v8::Local<v8::String>, v8::Local<v8::Value> value, 
                                                const v8::AccessorInfo& info)
{
    info.This()->SetInternalField(0, value);
}

QDeclarativeComponentExtension::~QDeclarativeComponentExtension()
{
    qPersistentDispose(incubationConstructor);
    qPersistentDispose(initialProperties);
    qPersistentDispose(forceCompletion);
}

QV8IncubatorResource::QV8IncubatorResource(QV8Engine *engine, IncubationMode m)
: QV8ObjectResource(engine), QDeclarativeIncubator(m)
{
}

void QV8IncubatorResource::setInitialState(QObject *o)
{
    QDeclarativeComponent_setQmlParent(o, parent);

    if (!valuemap.IsEmpty()) {
        QDeclarativeComponentExtension *e = componentExtension(engine);

        v8::HandleScope handle_scope;
        v8::Context::Scope scope(engine->context());

        v8::Handle<v8::Value> function = e->initialProperties->Run(qmlGlobal);
        v8::Handle<v8::Value> args[] = { engine->newQObject(o), valuemap };
        v8::Handle<v8::Function>::Cast(function)->Call(engine->global(), 2, args);

        qPersistentDispose(valuemap);
        qPersistentDispose(qmlGlobal);
    }
}
    
void QV8IncubatorResource::dispose()
{
    qPersistentDispose(valuemap);
    qPersistentDispose(qmlGlobal);
    // No further status changes are forthcoming, so we no long need a self reference
    qPersistentDispose(me);
}

void QV8IncubatorResource::statusChanged(Status s)
{
    if (s == Ready) {
        Q_ASSERT(QDeclarativeData::get(object()));
        QDeclarativeData::get(object())->setImplicitDestructible();
    }

    if (!me.IsEmpty()) { // Will be false in synchronous mode
        v8::HandleScope scope;
        v8::Local<v8::Value> callback = me->GetInternalField(0);

        if (!callback.IsEmpty() && !callback->IsUndefined()) {

            if (callback->IsFunction()) {
                v8::Context::Scope context_scope(engine->context());
                v8::Local<v8::Function> f = v8::Local<v8::Function>::Cast(callback);
                v8::Handle<v8::Value> args[] = { v8::Integer::NewFromUnsigned(s) };
                v8::TryCatch tc;
                f->Call(me, 1, args);
                if (tc.HasCaught()) {
                    QDeclarativeError error;
                    QDeclarativeExpressionPrivate::exceptionToError(tc.Message(), error);
                    QDeclarativeEnginePrivate::warning(QDeclarativeEnginePrivate::get(engine->engine()),
                                                       error);
                }
            }
        }
    }

    if (s == Ready || s == Error) 
        dispose();
}

QT_END_NAMESPACE
