/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qquickloader_p_p.h"

#include <QtDeclarative/qdeclarativeinfo.h>

#include <private/qdeclarativeengine_p.h>
#include <private/qdeclarativeglobal_p.h>

#include <private/qdeclarativecomponent_p.h>

#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

QQuickLoaderPrivate::QQuickLoaderPrivate()
    : item(0), component(0), itemContext(0), incubator(0), updatingSize(false),
      itemWidthValid(false), itemHeightValid(false),
      active(true), loadingFromSource(false), asynchronous(false)
{
}

QQuickLoaderPrivate::~QQuickLoaderPrivate()
{
    delete incubator;
    disposeInitialPropertyValues();
}

void QQuickLoaderPrivate::itemGeometryChanged(QQuickItem *resizeItem, const QRectF &newGeometry, const QRectF &oldGeometry)
{
    if (resizeItem == item) {
        if (!updatingSize && newGeometry.width() != oldGeometry.width())
            itemWidthValid = true;
        if (!updatingSize && newGeometry.height() != oldGeometry.height())
            itemHeightValid = true;
        _q_updateSize(false);
    }
    QQuickItemChangeListener::itemGeometryChanged(resizeItem, newGeometry, oldGeometry);
}

void QQuickLoaderPrivate::clear()
{
    disposeInitialPropertyValues();

    if (incubator)
        incubator->clear();

    if (loadingFromSource && component) {
        component->deleteLater();
        component = 0;
    }
    source = QUrl();

    if (item) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(item);
        p->removeItemChangeListener(this, QQuickItemPrivate::Geometry);

        // We can't delete immediately because our item may have triggered
        // the Loader to load a different item.
        item->setParentItem(0);
        item->setVisible(false);
        item->deleteLater();
        item = 0;
    }
}

void QQuickLoaderPrivate::initResize()
{
    QQuickItemPrivate *p = QQuickItemPrivate::get(item);
    p->addItemChangeListener(this, QQuickItemPrivate::Geometry);
    // We may override the item's size, so we need to remember
    // whether the item provided its own valid size.
    itemWidthValid = p->widthValid;
    itemHeightValid = p->heightValid;
    _q_updateSize();
}

/*!
    \qmlclass Loader QQuickLoader
    \inqmlmodule QtQuick 2
    \ingroup qml-utility-elements
    \inherits Item

    \brief The Loader item allows dynamically loading an Item-based
    subtree from a URL or Component.

    Loader is used to dynamically load visual QML components. It can load a
    QML file (using the \l source property) or a \l Component object (using
    the \l sourceComponent property). It is useful for delaying the creation
    of a component until it is required: for example, when a component should
    be created on demand, or when a component should not be created
    unnecessarily for performance reasons.

    Here is a Loader that loads "Page1.qml" as a component when the
    \l MouseArea is clicked:

    \snippet doc/src/snippets/declarative/loader/simple.qml 0

    The loaded item can be accessed using the \l item property.

    If the \l source or \l sourceComponent changes, any previously instantiated
    items are destroyed. Setting \l source to an empty string or setting
    \l sourceComponent to \c undefined destroys the currently loaded item,
    freeing resources and leaving the Loader empty.

    \section2 Loader sizing behavior

    Loader is like any other visual item and must be positioned and sized
    accordingly to become visible.

    \list
    \o If an explicit size is not specified for the Loader, the Loader
    is automatically resized to the size of the loaded item once the
    component is loaded.
    \o If the size of the Loader is specified explicitly by setting
    the width, height or by anchoring, the loaded item will be resized
    to the size of the Loader.
    \endlist

    In both scenarios the size of the item and the Loader are identical.
    This ensures that anchoring to the Loader is equivalent to anchoring
    to the loaded item.

    \table
    \row
    \o sizeloader.qml
    \o sizeitem.qml
    \row
    \o \snippet doc/src/snippets/declarative/loader/sizeloader.qml 0
    \o \snippet doc/src/snippets/declarative/loader/sizeitem.qml 0
    \row
    \o The red rectangle will be sized to the size of the root item.
    \o The red rectangle will be 50x50, centered in the root item.
    \endtable


    \section2 Receiving signals from loaded items

    Any signals emitted from the loaded item can be received using the
    \l Connections element. For example, the following \c application.qml
    loads \c MyItem.qml, and is able to receive the \c message signal from
    the loaded item through a \l Connections object:

    \table
    \row
    \o application.qml
    \o MyItem.qml
    \row
    \o \snippet doc/src/snippets/declarative/loader/connections.qml 0
    \o \snippet doc/src/snippets/declarative/loader/MyItem.qml 0
    \endtable

    Alternatively, since \c MyItem.qml is loaded within the scope of the
    Loader, it could also directly call any function defined in the Loader or
    its parent \l Item.


    \section2 Focus and key events

    Loader is a focus scope. Its \l {Item::}{focus} property must be set to
    \c true for any of its children to get the \e {active focus}. (See
    \l{qmlfocus#Acquiring Focus and Focus Scopes}{the focus documentation page}
    for more details.) Any key events received in the loaded item should likely
    also be \l {KeyEvent::}{accepted} so they are not propagated to the Loader.

    For example, the following \c application.qml loads \c KeyReader.qml when
    the \l MouseArea is clicked.  Notice the \l {Item::}{focus} property is
    set to \c true for the Loader as well as the \l Item in the dynamically
    loaded object:

    \table
    \row
    \o application.qml
    \o KeyReader.qml
    \row
    \o \snippet doc/src/snippets/declarative/loader/focus.qml 0
    \o \snippet doc/src/snippets/declarative/loader/KeyReader.qml 0
    \endtable

    Once \c KeyReader.qml is loaded, it accepts key events and sets
    \c event.accepted to \c true so that the event is not propagated to the
    parent \l Rectangle.

    \sa {dynamic-object-creation}{Dynamic Object Creation}
*/

QQuickLoader::QQuickLoader(QQuickItem *parent)
  : QQuickImplicitSizeItem(*(new QQuickLoaderPrivate), parent)
{
    setFlag(ItemIsFocusScope);
}

QQuickLoader::~QQuickLoader()
{
    Q_D(QQuickLoader);
    if (d->item) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->item);
        p->removeItemChangeListener(d, QQuickItemPrivate::Geometry);
    }
}

/*!
    \qmlproperty bool QtQuick2::Loader::active
    This property is \c true if the Loader is currently active.
    The default value for the \l active property is \c true.

    If the Loader is inactive, changing the \l source or \l sourceComponent
    will not cause the item to be instantiated until the Loader is made active.

    Setting the value to inactive will cause any \l item loaded by the loader
    to be released, but will not affect the \l source or \l sourceComponent.

    The \l status of an inactive loader is always \c Null.

    \sa source, sourceComponent
 */
bool QQuickLoader::active() const
{
    Q_D(const QQuickLoader);
    return d->active;
}

void QQuickLoader::setActive(bool newVal)
{
    Q_D(QQuickLoader);
    if (d->active != newVal) {
        d->active = newVal;
        if (newVal == true) {
            if (d->loadingFromSource) {
                loadFromSource();
            } else {
                loadFromSourceComponent();
            }
        } else {
            if (d->item) {
                QQuickItemPrivate *p = QQuickItemPrivate::get(d->item);
                p->removeItemChangeListener(d, QQuickItemPrivate::Geometry);

                // We can't delete immediately because our item may have triggered
                // the Loader to load a different item.
                d->item->setParentItem(0);
                d->item->setVisible(false);
                d->item->deleteLater();
                d->item = 0;
                emit itemChanged();
            }
            emit statusChanged();
        }
        emit activeChanged();
    }
}


/*!
    \qmlproperty url QtQuick2::Loader::source
    This property holds the URL of the QML component to instantiate.

    Note the QML component must be an \l{Item}-based component. The loader
    cannot load non-visual components.

    To unload the currently loaded item, set this property to an empty string,
    or set \l sourceComponent to \c undefined. Setting \c source to a
    new URL will also cause the item created by the previous URL to be unloaded.

    \sa sourceComponent, status, progress
*/
QUrl QQuickLoader::source() const
{
    Q_D(const QQuickLoader);
    return d->source;
}

void QQuickLoader::setSource(const QUrl &url)
{
    setSource(url, true); // clear previous values
}

void QQuickLoader::setSource(const QUrl &url, bool needsClear)
{
    Q_D(QQuickLoader);
    if (d->source == url)
        return;

    if (needsClear)
        d->clear();

    d->source = url;
    d->loadingFromSource = true;

    if (d->active)
        loadFromSource();
    else
        emit sourceChanged();
}

void QQuickLoader::loadFromSource()
{
    Q_D(QQuickLoader);
    if (d->source.isEmpty()) {
        emit sourceChanged();
        emit statusChanged();
        emit progressChanged();
        emit itemChanged();
        return;
    }

    if (isComponentComplete()) {
        d->component = new QDeclarativeComponent(qmlEngine(this), d->source, this);
        d->load();
    }
}

/*!
    \qmlproperty Component QtQuick2::Loader::sourceComponent
    This property holds the \l{Component} to instantiate.

    \qml
    Item {
        Component {
            id: redSquare
            Rectangle { color: "red"; width: 10; height: 10 }
        }

        Loader { sourceComponent: redSquare }
        Loader { sourceComponent: redSquare; x: 10 }
    }
    \endqml

    To unload the currently loaded item, set this property to an empty string
    or \c undefined.

    \sa source, progress
*/

QDeclarativeComponent *QQuickLoader::sourceComponent() const
{
    Q_D(const QQuickLoader);
    return d->component;
}

void QQuickLoader::setSourceComponent(QDeclarativeComponent *comp)
{
    Q_D(QQuickLoader);
    if (comp == d->component)
        return;

    d->clear();

    d->component = comp;
    d->loadingFromSource = false;

    if (d->active)
        loadFromSourceComponent();
    else
        emit sourceComponentChanged();
}

void QQuickLoader::resetSourceComponent()
{
    setSourceComponent(0);
}

void QQuickLoader::loadFromSourceComponent()
{
    Q_D(QQuickLoader);
    if (!d->component) {
        emit sourceComponentChanged();
        emit statusChanged();
        emit progressChanged();
        emit itemChanged();
        return;
    }

    if (isComponentComplete())
        d->load();
}

/*!
    \qmlmethod object QtQuick2::Loader::setSource(url source, object properties)

    Creates an object instance of the given \a source component that will have
    the given \a properties. The \a properties argument is optional.  The instance
    will be accessible via the \l item property once loading and instantiation
    is complete.

    If the \l active property is \c false at the time when this function is called,
    the given \a source component will not be loaded but the \a source and initial
    \a properties will be cached.  When the loader is made \l active, an instance of
    the \a source component will be created with the initial \a properties set.

    Setting the initial property values of an instance of a component in this manner
    will \bold{not} trigger any associated \l{Behavior}s.

    Note that the cached \a properties will be cleared if the \l source or \l sourceComponent
    is changed after calling this function but prior to setting the loader \l active.

    Example:
    \table
    \row
    \o
    \qml
    // ExampleComponent.qml
    import QtQuick 2.0
    Rectangle {
        id: rect
        color: "red"
        width: 10
        height: 10

        Behavior on color {
            NumberAnimation {
                target: rect
                property: "width"
                to: (rect.width + 20)
                duration: 0
            }
        }
    }
    \endqml
    \o
    \qml
    // example.qml
    import QtQuick 2.0
    Item {
        Loader {
            id: squareLoader
            onLoaded: console.log(squareLoader.item.width); // prints [10], not [30]
        }

        Component.onCompleted: {
            squareLoader.setSource("ExampleComponent.qml", { "color": "blue" });
            // will trigger the onLoaded code when complete.
        }
    }
    \endqml
    \endtable

    \sa source, active
*/
void QQuickLoader::setSource(QDeclarativeV8Function *args)
{
    Q_ASSERT(args);
    Q_D(QQuickLoader);

    bool ipvError = false;
    args->returnValue(v8::Undefined());
    v8::Handle<v8::Object> ipv = d->extractInitialPropertyValues(args, this, &ipvError);
    if (ipvError)
        return;

    d->clear();
    QUrl sourceUrl = d->resolveSourceUrl(args);
    if (!ipv.IsEmpty()) {
        d->disposeInitialPropertyValues();
        d->initialPropertyValues = qPersistentNew(ipv);
        d->qmlGlobalForIpv = qPersistentNew(args->qmlGlobal());
    }

    setSource(sourceUrl, false); // already cleared and set ipv above.
}

void QQuickLoaderPrivate::disposeInitialPropertyValues()
{
    if (!initialPropertyValues.IsEmpty())
        qPersistentDispose(initialPropertyValues);
    if (!qmlGlobalForIpv.IsEmpty())
        qPersistentDispose(qmlGlobalForIpv);
}

void QQuickLoaderPrivate::load()
{
    Q_Q(QQuickLoader);

    if (!q->isComponentComplete() || !component)
        return;

    if (!component->isLoading()) {
        _q_sourceLoaded();
    } else {
        QObject::connect(component, SIGNAL(statusChanged(QDeclarativeComponent::Status)),
                q, SLOT(_q_sourceLoaded()));
        QObject::connect(component, SIGNAL(progressChanged(qreal)),
                q, SIGNAL(progressChanged()));
        emit q->statusChanged();
        emit q->progressChanged();
        if (loadingFromSource)
            emit q->sourceChanged();
        else
            emit q->sourceComponentChanged();
        emit q->itemChanged();
    }
}

void QQuickLoaderIncubator::setInitialState(QObject *o)
{
    loader->setInitialState(o);
}

void QQuickLoaderPrivate::setInitialState(QObject *obj)
{
    Q_Q(QQuickLoader);

    QQuickItem *item = qobject_cast<QQuickItem*>(obj);
    if (item) {
        QDeclarative_setParent_noEvent(itemContext, obj);
        QDeclarative_setParent_noEvent(item, q);
        item->setParentItem(q);
    }

    if (initialPropertyValues.IsEmpty())
        return;

    QDeclarativeComponentPrivate *d = QDeclarativeComponentPrivate::get(component);
    Q_ASSERT(d && d->engine);
    d->initializeObjectWithInitialProperties(qmlGlobalForIpv, initialPropertyValues, obj);
}

void QQuickLoaderIncubator::statusChanged(Status status)
{
    loader->incubatorStateChanged(status);
}

void QQuickLoaderPrivate::incubatorStateChanged(QDeclarativeIncubator::Status status)
{
    Q_Q(QQuickLoader);
    if (status == QDeclarativeIncubator::Loading || status == QDeclarativeIncubator::Null)
        return;

    if (status == QDeclarativeIncubator::Ready) {
        QObject *obj = incubator->object();
        item = qobject_cast<QQuickItem*>(obj);
        if (item) {
            initResize();
        } else {
            qmlInfo(q) << QQuickLoader::tr("Loader does not support loading non-visual elements.");
            delete itemContext;
            itemContext = 0;
            delete obj;
        }
        incubator->clear();
    } else if (status == QDeclarativeIncubator::Error) {
        if (!incubator->errors().isEmpty())
            QDeclarativeEnginePrivate::warning(qmlEngine(q), incubator->errors());
        delete itemContext;
        itemContext = 0;
        delete incubator->object();
        source = QUrl();
    }
    if (loadingFromSource)
        emit q->sourceChanged();
    else
        emit q->sourceComponentChanged();
    emit q->statusChanged();
    emit q->progressChanged();
    emit q->itemChanged();
    emit q->loaded();
    disposeInitialPropertyValues(); // cleanup
}

void QQuickLoaderPrivate::_q_sourceLoaded()
{
    Q_Q(QQuickLoader);
    if (!component || !component->errors().isEmpty()) {
        if (component)
            QDeclarativeEnginePrivate::warning(qmlEngine(q), component->errors());
        if (loadingFromSource)
            emit q->sourceChanged();
        else
            emit q->sourceComponentChanged();
        emit q->statusChanged();
        emit q->progressChanged();
        disposeInitialPropertyValues(); // cleanup
        return;
    }

    QDeclarativeContext *creationContext = component->creationContext();
    if (!creationContext) creationContext = qmlContext(q);
    itemContext = new QDeclarativeContext(creationContext);
    itemContext->setContextObject(q);

    delete incubator;
    incubator = new QQuickLoaderIncubator(this, asynchronous ? QDeclarativeIncubator::Asynchronous : QDeclarativeIncubator::AsynchronousIfNested);

    component->create(*incubator, itemContext);

    if (incubator && incubator->status() == QDeclarativeIncubator::Loading)
        emit q->statusChanged();
}

/*!
    \qmlproperty enumeration QtQuick2::Loader::status

    This property holds the status of QML loading.  It can be one of:
    \list
    \o Loader.Null - the loader is inactive or no QML source has been set
    \o Loader.Ready - the QML source has been loaded
    \o Loader.Loading - the QML source is currently being loaded
    \o Loader.Error - an error occurred while loading the QML source
    \endlist

    Use this status to provide an update or respond to the status change in some way.
    For example, you could:

    \list
    \o Trigger a state change:
    \qml
        State { name: 'loaded'; when: loader.status == Loader.Ready }
    \endqml

    \o Implement an \c onStatusChanged signal handler:
    \qml
        Loader {
            id: loader
            onStatusChanged: if (loader.status == Loader.Ready) console.log('Loaded')
        }
    \endqml

    \o Bind to the status value:
    \qml
        Text { text: loader.status == Loader.Ready ? 'Loaded' : 'Not loaded' }
    \endqml
    \endlist

    Note that if the source is a local file, the status will initially be Ready (or Error). While
    there will be no onStatusChanged signal in that case, the onLoaded will still be invoked.

    \sa progress
*/

QQuickLoader::Status QQuickLoader::status() const
{
    Q_D(const QQuickLoader);

    if (!d->active)
        return Null;

    if (d->component) {
        switch (d->component->status()) {
        case QDeclarativeComponent::Loading:
            return Loading;
        case QDeclarativeComponent::Error:
            return Error;
        case QDeclarativeComponent::Null:
            return Null;
        default:
            break;
        }
    }

    if (d->incubator) {
        switch (d->incubator->status()) {
        case QDeclarativeIncubator::Loading:
            return Loading;
        case QDeclarativeIncubator::Error:
            return Error;
        default:
            break;
        }
    }

    if (d->item)
        return Ready;

    return d->source.isEmpty() ? Null : Error;
}

void QQuickLoader::componentComplete()
{
    Q_D(QQuickLoader);
    QQuickItem::componentComplete();
    if (active()) {
        if (d->loadingFromSource) {
            d->component = new QDeclarativeComponent(qmlEngine(this), d->source, this);
        }
        d->load();
    }
}

/*!
    \qmlsignal QtQuick2::Loader::onLoaded()

    This handler is called when the \l status becomes \c Loader.Ready, or on successful
    initial load.
*/


/*!
\qmlproperty real QtQuick2::Loader::progress

This property holds the progress of loading QML data from the network, from
0.0 (nothing loaded) to 1.0 (finished).  Most QML files are quite small, so
this value will rapidly change from 0 to 1.

\sa status
*/
qreal QQuickLoader::progress() const
{
    Q_D(const QQuickLoader);

    if (d->item)
        return 1.0;

    if (d->component)
        return d->component->progress();

    return 0.0;
}

/*!
\qmlproperty bool QtQuick2::Loader::asynchronous

This property holds whether the component will be instantiated asynchronously.

Loading asynchronously creates the objects declared by the component
across multiple frames, and reduces the
likelihood of glitches in animation.  When loading asynchronously the status
will change to Loader.Loading.  Once the entire component has been created, the
\l item will be available and the status will change to Loader.Ready.

To avoid seeing the items loading progressively set \c visible appropriately, e.g.

\code
Loader {
    source: "mycomponent.qml"
    asynchronous: true
    visible: status == Loader.Ready
}
\endcode

Note that this property affects object instantiation only; it is unrelated to
loading a component asynchronously via a network.
*/
bool QQuickLoader::asynchronous() const
{
    Q_D(const QQuickLoader);
    return d->asynchronous;
}

void QQuickLoader::setAsynchronous(bool a)
{
    Q_D(QQuickLoader);
    if (d->asynchronous == a)
        return;

    d->asynchronous = a;
    emit asynchronousChanged();
}

void QQuickLoaderPrivate::_q_updateSize(bool loaderGeometryChanged)
{
    Q_Q(QQuickLoader);
    if (!item || updatingSize)
        return;

    updatingSize = true;

    qreal iWidth = !itemWidthValid ? item->implicitWidth() : item->width();
    qreal iHeight = !itemHeightValid ? item->implicitHeight() : item->height();
    q->setImplicitSize(iWidth, iHeight);

    if (loaderGeometryChanged && q->widthValid())
        item->setWidth(q->width());
    if (loaderGeometryChanged && q->heightValid())
        item->setHeight(q->height());

    updatingSize = false;
}

/*!
    \qmlproperty Item QtQuick2::Loader::item
    This property holds the top-level item that is currently loaded.
*/
QQuickItem *QQuickLoader::item() const
{
    Q_D(const QQuickLoader);
    return d->item;
}

void QQuickLoader::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickLoader);
    if (newGeometry != oldGeometry) {
        d->_q_updateSize();
    }
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

QUrl QQuickLoaderPrivate::resolveSourceUrl(QDeclarativeV8Function *args)
{
    QV8Engine *v8engine = args->engine();
    QString arg = v8engine->toString((*args)[0]->ToString());
    if (arg.isEmpty())
        return QUrl();

    QDeclarativeContextData *context = args->context();
    Q_ASSERT(context);
    return context->resolvedUrl(QUrl(arg));
}

v8::Handle<v8::Object> QQuickLoaderPrivate::extractInitialPropertyValues(QDeclarativeV8Function *args, QObject *loader, bool *error)
{
    v8::Local<v8::Object> valuemap;
    if (args->Length() >= 2) {
        v8::Local<v8::Value> v = (*args)[1];
        if (!v->IsObject() || v->IsArray()) {
            *error = true;
            qmlInfo(loader) << loader->tr("setSource: value is not an object");
        } else {
            *error = false;
            valuemap = v8::Local<v8::Object>::Cast(v);
        }
    }

    return valuemap;
}

#include <moc_qquickloader_p.cpp>

QT_END_NAMESPACE
