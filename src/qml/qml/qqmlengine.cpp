/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlengine_p.h"
#include "qqmlengine.h"
#include "qqmlcomponentattached_p.h"

#include "qqmlcontext_p.h"
#include "qqml.h"
#include "qqmlcontext.h"
#include "qqmlexpression.h"
#include "qqmlcomponent.h"
#include "qqmlvme_p.h"
#include "qqmlstringconverters_p.h"
#include "qqmlscriptstring.h"
#include "qqmlglobal_p.h"
#include "qqmlcomponent_p.h"
#include "qqmlextensioninterface.h"
#include "qqmllist_p.h"
#include "qqmltypenamecache_p.h"
#include "qqmlnotifier_p.h"
#include "qqmlincubator.h"
#include "qqmlabstracturlinterceptor.h"
#include "qqmlsourcecoordinate_p.h"
#include <private/qqmldirparser_p.h>
#include <private/qqmlboundsignal_p.h>
#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qqmltype_p_p.h>
#include <private/qqmlpluginimporter_p.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qmetaobject.h>
#include <QDebug>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qdir.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <private/qthread_p.h>

#if QT_CONFIG(qml_network)
#include "qqmlnetworkaccessmanagerfactory.h"
#include <QNetworkAccessManager>
#endif

#include <private/qobject_p.h>
#include <private/qmetaobject_p.h>
#if QT_CONFIG(qml_locale)
#include <private/qqmllocale_p.h>
#endif
#include <private/qqmlbind_p.h>
#include <private/qqmlconnections_p.h>
#if QT_CONFIG(qml_animation)
#include <private/qqmltimer_p.h>
#endif
#include <private/qqmlplatform_p.h>
#include <private/qqmlloggingcategory_p.h>

#if QT_CONFIG(qml_sequence_object)
#include <private/qv4sequenceobject_p.h>
#endif

#ifdef Q_OS_WIN // for %APPDATA%
#  include <qt_windows.h>
#  include <shlobj.h>
#  include <qlibrary.h>
#  ifndef CSIDL_APPDATA
#    define CSIDL_APPDATA           0x001a  // <username>\Application Data
#  endif
#endif // Q_OS_WIN

QT_BEGIN_NAMESPACE

/*!
  \qmltype QtObject
    \instantiates QObject
  \inqmlmodule QtQml
  \ingroup qml-utility-elements
  \brief A basic QML type.

  The QtObject type is a non-visual element which contains only the
  objectName property.

  It can be useful to create a QtObject if you need an extremely
  lightweight type to enclose a set of custom properties:

  \snippet qml/qtobject.qml 0

  It can also be useful for C++ integration, as it is just a plain
  QObject. See the QObject documentation for further details.
*/
/*!
  \qmlproperty string QtObject::objectName
  This property holds the QObject::objectName for this specific object instance.

  This allows a C++ application to locate an item within a QML component
  using the QObject::findChild() method. For example, the following C++
  application locates the child \l Rectangle item and dynamically changes its
  \c color value:

    \qml
    // MyRect.qml

    import QtQuick 2.0

    Item {
        width: 200; height: 200

        Rectangle {
            anchors.fill: parent
            color: "red"
            objectName: "myRect"
        }
    }
    \endqml

    \code
    // main.cpp

    QQuickView view;
    view.setSource(QUrl::fromLocalFile("MyRect.qml"));
    view.show();

    QQuickItem *item = view.rootObject()->findChild<QQuickItem*>("myRect");
    if (item)
        item->setProperty("color", QColor(Qt::yellow));
    \endcode
*/

bool QQmlEnginePrivate::qml_debugging_enabled = false;
bool QQmlEnginePrivate::s_designerMode = false;

bool QQmlEnginePrivate::designerMode()
{
    return s_designerMode;
}

void QQmlEnginePrivate::activateDesignerMode()
{
    s_designerMode = true;
}


/*!
    \class QQmlImageProviderBase
    \brief The QQmlImageProviderBase class is used to register image providers in the QML engine.
    \inmodule QtQml

    Image providers must be registered with the QML engine.  The only information the QML
    engine knows about image providers is the type of image data they provide.  To use an
    image provider to acquire image data, you must cast the QQmlImageProviderBase pointer
    to a QQuickImageProvider pointer.

    \sa QQuickImageProvider, QQuickTextureFactory
*/

/*!
    \enum QQmlImageProviderBase::ImageType

    Defines the type of image supported by this image provider.

    \value Image The Image Provider provides QImage images.
        The QQuickImageProvider::requestImage() method will be called for all image requests.
    \value Pixmap The Image Provider provides QPixmap images.
        The QQuickImageProvider::requestPixmap() method will be called for all image requests.
    \value Texture The Image Provider provides QSGTextureProvider based images.
        The QQuickImageProvider::requestTexture() method will be called for all image requests.
    \value ImageResponse The Image provider provides QQuickTextureFactory based images.
        Should only be used in QQuickAsyncImageProvider or its subclasses.
        The QQuickAsyncImageProvider::requestImageResponse() method will be called for all image requests.
        Since Qt 5.6
    \omitvalue Invalid
*/

/*!
    \enum QQmlImageProviderBase::Flag

    Defines specific requirements or features of this image provider.

    \value ForceAsynchronousImageLoading Ensures that image requests to the provider are
        run in a separate thread, which allows the provider to spend as much time as needed
        on producing the image without blocking the main thread.
*/

/*!
    \fn QQmlImageProviderBase::imageType() const

    Implement this method to return the image type supported by this image provider.
*/

/*!
    \fn QQmlImageProviderBase::flags() const

    Implement this to return the properties of this image provider.
*/

/*! \internal */
QQmlImageProviderBase::QQmlImageProviderBase()
{
}

/*! \internal */
QQmlImageProviderBase::~QQmlImageProviderBase()
{
}


/*!
\qmltype Qt
\inqmlmodule QtQml
\instantiates QQmlEnginePrivate
\ingroup qml-utility-elements
\keyword QmlGlobalQtObject
\brief Provides a global object with useful enums and functions from Qt.

The \c Qt object is a global object with utility functions, properties and enums.

It is not instantiable; to use it, call the members of the global \c Qt object directly.
For example:

\qml
import QtQuick 2.0

Text {
    color: Qt.rgba(1, 0, 0, 1)
    text: Qt.md5("hello, world")
}
\endqml


\section1 Enums

The Qt object contains the enums available in the \l [QtCore]{Qt}{Qt Namespace}. For example, you can access
the \l Qt::LeftButton and \l Qt::RightButton enumeration values as \c Qt.LeftButton and \c Qt.RightButton.


\section1 Types

The Qt object also contains helper functions for creating objects of specific
data types. This is primarily useful when setting the properties of an item
when the property has one of the following types:
\list
\li \c rect - use \l{Qt::rect()}{Qt.rect()}
\li \c point - use \l{Qt::point()}{Qt.point()}
\li \c size - use \l{Qt::size()}{Qt.size()}
\endlist

If the \c QtQuick module has been imported, the following helper functions for
creating objects of specific data types are also available for clients to use:
\list
\li \c color - use \l{Qt::rgba()}{Qt.rgba()}, \l{Qt::hsla()}{Qt.hsla()}, \l{Qt::darker()}{Qt.darker()}, \l{Qt::lighter()}{Qt.lighter()} or \l{Qt::tint()}{Qt.tint()}
\li \c font - use \l{Qt::font()}{Qt.font()}
\li \c vector2d - use \l{Qt::vector2d()}{Qt.vector2d()}
\li \c vector3d - use \l{Qt::vector3d()}{Qt.vector3d()}
\li \c vector4d - use \l{Qt::vector4d()}{Qt.vector4d()}
\li \c quaternion - use \l{Qt::quaternion()}{Qt.quaternion()}
\li \c matrix4x4 - use \l{Qt::matrix4x4()}{Qt.matrix4x4()}
\endlist

There are also string based constructors for these types. See \l{qtqml-typesystem-basictypes.html}{QML Basic Types} for more information.

\section1 Date/Time Formatters

The Qt object contains several functions for formatting QDateTime, QDate and QTime values.

\list
    \li \l{Qt::formatDateTime}{string Qt.formatDateTime(datetime date, variant format)}
    \li \l{Qt::formatDate}{string Qt.formatDate(datetime date, variant format)}
    \li \l{Qt::formatTime}{string Qt.formatTime(datetime date, variant format)}
\endlist

The format specification is described at \l{Qt::formatDateTime}{Qt.formatDateTime}.


\section1 Dynamic Object Creation
The following functions on the global object allow you to dynamically create QML
items from files or strings. See \l{Dynamic QML Object Creation from JavaScript} for an overview
of their use.

\list
    \li \l{Qt::createComponent()}{object Qt.createComponent(url)}
    \li \l{Qt::createQmlObject()}{object Qt.createQmlObject(string qml, object parent, string filepath)}
\endlist


\section1 Other Functions

The following functions are also on the Qt object.

\list
    \li \l{Qt::quit()}{Qt.quit()}
    \li \l{Qt::md5()}{Qt.md5(string)}
    \li \l{Qt::btoa()}{string Qt.btoa(string)}
    \li \l{Qt::atob()}{string Qt.atob(string)}
    \li \l{Qt::binding()}{object Qt.binding(function)}
    \li \l{Qt::locale()}{object Qt.locale()}
    \li \l{Qt::resolvedUrl()}{string Qt.resolvedUrl(string)}
    \li \l{Qt::openUrlExternally()}{Qt.openUrlExternally(string)}
    \li \l{Qt::fontFamilies()}{list<string> Qt.fontFamilies()}
\endlist
*/

/*!
    \qmlproperty object Qt::platform
    \since 5.1

    The \c platform object provides info about the underlying platform.

    Its properties are:

    \table
    \row
    \li \c platform.os
    \li

    This read-only property contains the name of the operating system.

    Possible values are:

    \list
        \li \c "android" - Android
        \li \c "ios" - iOS
        \li \c "tvos" - tvOS
        \li \c "linux" - Linux
        \li \c "osx" - \macos
        \li \c "qnx" - QNX (since Qt 5.9.3)
        \li \c "unix" - Other Unix-based OS
        \li \c "windows" - Windows
        \li \c "wasm" - WebAssembly
    \endlist

    \row
    \li \c platform.pluginName
    \li This is the name of the platform set on the QGuiApplication instance
        as returned by \l QGuiApplication::platformName()

    \endtable
*/

/*!
    \qmlproperty object Qt::application
    \since 5.1

    The \c application object provides access to global application state
    properties shared by many QML components.

    It is the same as the \l Application singleton.

    The following example uses the \c application object to indicate
    whether the application is currently active:

    \snippet qml/application.qml document

    \note When using QML without a QGuiApplication, the following properties will be undefined:
    \list
    \li application.active
    \li application.state
    \li application.layoutDirection
    \li application.font
    \endlist
*/

/*!
    \qmlproperty object Qt::inputMethod
    \since 5.0

    The \c inputMethod object allows access to application's QInputMethod object
    and all its properties and slots. See the QInputMethod documentation for
    further details.
*/

/*!
    \qmlproperty object Qt::styleHints
    \since 5.5

    The \c styleHints object provides platform-specific style hints and settings.
    See the QStyleHints documentation for further details.

    \note The \c styleHints object is only available when using the Qt Quick module.

    The following example uses the \c styleHints object to determine whether an
    item should gain focus on mouse press or touch release:
    \code
    import QtQuick 2.4

    MouseArea {
        id: button

        onPressed: {
            if (!Qt.styleHints.setFocusOnTouchRelease)
                button.forceActiveFocus()
        }
        onReleased: {
            if (Qt.styleHints.setFocusOnTouchRelease)
                button.forceActiveFocus()
        }
    }
    \endcode
*/

/*!
\qmlmethod object Qt::include(string url, jsobject callback)
\deprecated

This method should not be used. Use ECMAScript modules instead and the native
JavaScript \c import and \c export statements instead.

Includes another JavaScript file. This method can only be used from within JavaScript files,
and not regular QML files.

This imports all functions from \a url into the current script's namespace.

Qt.include() returns an object that describes the status of the operation.  The object has
a single property, \c {status}, that is set to one of the following values:

\table
\header \li Symbol \li Value \li Description
\row \li result.OK \li 0 \li The include completed successfully.
\row \li result.LOADING \li 1 \li Data is being loaded from the network.
\row \li result.NETWORK_ERROR \li 2 \li A network error occurred while fetching the url.
\row \li result.EXCEPTION \li 3 \li A JavaScript exception occurred while executing the included code.
An additional \c exception property will be set in this case.
\endtable

The \c status property will be updated as the operation progresses.

If provided, \a callback is invoked when the operation completes.  The callback is passed
the same object as is returned from the Qt.include() call.
*/
// Qt.include() is implemented in qv4include.cpp

QQmlEnginePrivate::QQmlEnginePrivate(QQmlEngine *e)
: propertyCapture(nullptr), rootContext(nullptr),
#if QT_CONFIG(qml_debug)
  profiler(nullptr),
#endif
  outputWarningsToMsgLog(true),
  erroredBindings(nullptr), inProgressCreations(0),
#if QT_CONFIG(qml_worker_script)
  workerScriptEngine(nullptr),
#endif
  activeObjectCreator(nullptr),
#if QT_CONFIG(qml_network)
  networkAccessManager(nullptr), networkAccessManagerFactory(nullptr),
#endif
  scarceResourcesRefCount(0), importDatabase(e), typeLoader(e),
  uniqueId(1), incubatorCount(0), incubationController(nullptr)
{
}

QQmlEnginePrivate::~QQmlEnginePrivate()
{
    if (inProgressCreations)
        qWarning() << QQmlEngine::tr("There are still \"%1\" items in the process of being created at engine destruction.").arg(inProgressCreations);

    doDeleteInEngineThread();

    if (incubationController) incubationController->d = nullptr;
    incubationController = nullptr;

    QQmlMetaType::freeUnusedTypesAndCaches();

    for (auto iter = m_compositeTypes.cbegin(), end = m_compositeTypes.cend(); iter != end; ++iter)
        iter.value()->isRegisteredWithEngine = false;
#if QT_CONFIG(qml_debug)
    delete profiler;
#endif
}

void QQmlPrivate::qdeclarativeelement_destructor(QObject *o)
{
    if (QQmlData *d = QQmlData::get(o)) {
        if (d->ownContext) {
            for (QQmlRefPointer<QQmlContextData> lc = d->ownContext->linkedContext().data(); lc;
                 lc = lc->linkedContext()) {
                lc->invalidate();
                if (lc->contextObject() == o)
                    lc->setContextObject(nullptr);
            }
            d->ownContext->invalidate();
            if (d->ownContext->contextObject() == o)
                d->ownContext->setContextObject(nullptr);
            d->ownContext = nullptr;
            d->context = nullptr;
        }

        if (d->outerContext && d->outerContext->contextObject() == o)
            d->outerContext->setContextObject(nullptr);

        // Mark this object as in the process of deletion to
        // prevent it resolving in bindings
        QQmlData::markAsDeleted(o);

        // Disconnect the notifiers now - during object destruction this would be too late, since
        // the disconnect call wouldn't be able to call disconnectNotify(), as it isn't possible to
        // get the metaobject anymore.
        d->disconnectNotifiers();
    }
}

QQmlData::QQmlData()
    : ownMemory(true), indestructible(true), explicitIndestructibleSet(false),
      hasTaintedV4Object(false), isQueuedForDeletion(false), rootObjectInCreation(false),
      hasInterceptorMetaObject(false), hasVMEMetaObject(false),
      bindingBitsArraySize(InlineBindingArraySize), notifyList(nullptr),
      bindings(nullptr), signalHandlers(nullptr), nextContextObject(nullptr), prevContextObject(nullptr),
      lineNumber(0), columnNumber(0), jsEngineId(0),
      propertyCache(nullptr), guards(nullptr), extendedData(nullptr)
{
    memset(bindingBitsValue, 0, sizeof(bindingBitsValue));
    init();
}

QQmlData::~QQmlData()
{
}

void QQmlData::destroyed(QAbstractDeclarativeData *d, QObject *o)
{
    QQmlData *ddata = static_cast<QQmlData *>(d);
    ddata->destroyed(o);
}


class QQmlThreadNotifierProxyObject : public QObject
{
public:
    QPointer<QObject> target;

    int qt_metacall(QMetaObject::Call, int methodIndex, void **a) override {
        if (!target)
            return -1;

        QMetaMethod method = target->metaObject()->method(methodIndex);
        Q_ASSERT(method.methodType() == QMetaMethod::Signal);
        int signalIndex = QMetaObjectPrivate::signalIndex(method);
        QQmlData *ddata = QQmlData::get(target, false);
        QQmlNotifierEndpoint *ep = ddata->notify(signalIndex);
        if (ep) QQmlNotifier::emitNotify(ep, a);

        delete this;

        return -1;
    }
};

void QQmlData::signalEmitted(QAbstractDeclarativeData *, QObject *object, int index, void **a)
{
    QQmlData *ddata = QQmlData::get(object, false);
    if (!ddata) return; // Probably being deleted

    // In general, QML only supports QObject's that live on the same thread as the QQmlEngine
    // that they're exposed to.  However, to make writing "worker objects" that calculate data
    // in a separate thread easier, QML allows a QObject that lives in the same thread as the
    // QQmlEngine to emit signals from a different thread.  These signals are then automatically
    // marshalled back onto the QObject's thread and handled by QML from there.  This is tested
    // by the qqmlecmascript::threadSignal() autotest.
    if (!ddata->notifyList)
        return;

    auto objectThreadData = QObjectPrivate::get(object)->threadData.loadRelaxed();
    if (QThread::currentThreadId() != objectThreadData->threadId.loadRelaxed()) {
        if (!objectThreadData->thread.loadAcquire())
            return;

        QMetaMethod m = QMetaObjectPrivate::signal(object->metaObject(), index);
        QList<QByteArray> parameterTypes = m.parameterTypes();

        QScopedPointer<QMetaCallEvent> ev(new QMetaCallEvent(m.methodIndex(), 0, nullptr,
                                                             object, index,
                                                             parameterTypes.count() + 1));

        void **args = ev->args();
        QMetaType *types = ev->types();

        for (int ii = 0; ii < parameterTypes.count(); ++ii) {
            const QByteArray &typeName = parameterTypes.at(ii);
            if (typeName.endsWith('*'))
                types[ii + 1] = QMetaType(QMetaType::VoidStar);
            else
                types[ii + 1] = QMetaType::fromName(typeName);

            if (!types[ii + 1].isValid()) {
                qWarning("QObject::connect: Cannot queue arguments of type '%s'\n"
                         "(Make sure '%s' is registered using qRegisterMetaType().)",
                         typeName.constData(), typeName.constData());
                return;
            }

            args[ii + 1] = types[ii + 1].create(a[ii + 1]);
        }

        QQmlThreadNotifierProxyObject *mpo = new QQmlThreadNotifierProxyObject;
        mpo->target = object;
        mpo->moveToThread(objectThreadData->thread.loadAcquire());
        QCoreApplication::postEvent(mpo, ev.take());

    } else {
        QQmlNotifierEndpoint *ep = ddata->notify(index);
        if (ep) QQmlNotifier::emitNotify(ep, a);
    }
}

int QQmlData::receivers(QAbstractDeclarativeData *d, const QObject *, int index)
{
    QQmlData *ddata = static_cast<QQmlData *>(d);
    return ddata->endpointCount(index);
}

bool QQmlData::isSignalConnected(QAbstractDeclarativeData *d, const QObject *, int index)
{
    QQmlData *ddata = static_cast<QQmlData *>(d);
    return ddata->signalHasEndpoint(index);
}

int QQmlData::endpointCount(int index)
{
    int count = 0;
    QQmlNotifierEndpoint *ep = notify(index);
    if (!ep)
        return count;
    ++count;
    while (ep->next) {
        ++count;
        ep = ep->next;
    }
    return count;
}

void QQmlData::markAsDeleted(QObject *o)
{
    QQmlData::setQueuedForDeletion(o);

    QObjectPrivate *p = QObjectPrivate::get(o);
    for (QList<QObject *>::const_iterator it = p->children.constBegin(), end = p->children.constEnd(); it != end; ++it) {
        QQmlData::markAsDeleted(*it);
    }
}

void QQmlData::setQueuedForDeletion(QObject *object)
{
    if (object) {
        if (QQmlData *ddata = QQmlData::get(object)) {
            if (ddata->ownContext) {
                Q_ASSERT(ddata->ownContext.data() == ddata->context);
                ddata->context->emitDestruction();
                if (ddata->ownContext->contextObject() == object)
                    ddata->ownContext->setContextObject(nullptr);
                ddata->ownContext = nullptr;
                ddata->context = nullptr;
            }
            ddata->isQueuedForDeletion = true;
        }
    }
}

void QQmlData::flushPendingBinding(int coreIndex)
{
    clearPendingBindingBit(coreIndex);

    // Find the binding
    QQmlAbstractBinding *b = bindings;
    while (b && (b->targetPropertyIndex().coreIndex() != coreIndex ||
                 b->targetPropertyIndex().hasValueTypeIndex()))
        b = b->nextBinding();

    if (b && b->targetPropertyIndex().coreIndex() == coreIndex &&
            !b->targetPropertyIndex().hasValueTypeIndex())
        b->setEnabled(true, QQmlPropertyData::BypassInterceptor |
                            QQmlPropertyData::DontRemoveBinding);
}

QQmlData::DeferredData::DeferredData()
{
}

QQmlData::DeferredData::~DeferredData()
{
}

bool QQmlEnginePrivate::baseModulesUninitialized = true;
void QQmlEnginePrivate::init()
{
    Q_Q(QQmlEngine);

    if (baseModulesUninitialized) {

        // required for the Compiler.
        qmlRegisterType<QObject>("QML", 1, 0, "QtObject");
        qmlRegisterType<QQmlComponent>("QML", 1, 0, "Component");

        QQmlData::init();
        baseModulesUninitialized = false;
    }

    qRegisterMetaType<QVariant>();
    qRegisterMetaType<QQmlScriptString>();
    qRegisterMetaType<QJSValue>();
    qRegisterMetaType<QQmlComponent::Status>();
    qRegisterMetaType<QList<QObject*> >();
    qRegisterMetaType<QList<int> >();
    qRegisterMetaType<QQmlBinding*>();

    q->handle()->setQmlEngine(q);

    rootContext = new QQmlContext(q,true);
}

/*!
  \class QQmlEngine
  \since 5.0
  \inmodule QtQml
  \brief The QQmlEngine class provides an environment for instantiating QML components.

  Each QML component is instantiated in a QQmlContext.
  QQmlContext's are essential for passing data to QML
  components.  In QML, contexts are arranged hierarchically and this
  hierarchy is managed by the QQmlEngine.

  Prior to creating any QML components, an application must have
  created a QQmlEngine to gain access to a QML context.  The
  following example shows how to create a simple Text item.

  \code
  QQmlEngine engine;
  QQmlComponent component(&engine);
  component.setData("import QtQuick 2.0\nText { text: \"Hello world!\" }", QUrl());
  QQuickItem *item = qobject_cast<QQuickItem *>(component.create());

  //add item to view, etc
  ...
  \endcode

  In this case, the Text item will be created in the engine's
  \l {QQmlEngine::rootContext()}{root context}.

  \sa QQmlComponent, QQmlContext, {QML Global Object}
*/

/*!
  Create a new QQmlEngine with the given \a parent.
*/
QQmlEngine::QQmlEngine(QObject *parent)
: QJSEngine(*new QQmlEnginePrivate(this), parent)
{
    Q_D(QQmlEngine);
    d->init();
    QJSEnginePrivate::addToDebugServer(this);
}

/*!
* \internal
*/
QQmlEngine::QQmlEngine(QQmlEnginePrivate &dd, QObject *parent)
: QJSEngine(dd, parent)
{
    Q_D(QQmlEngine);
    d->init();
}

/*!
  Destroys the QQmlEngine.

  Any QQmlContext's created on this engine will be
  invalidated, but not destroyed (unless they are parented to the
  QQmlEngine object).

  See QJSEngine docs for details on cleaning up the JS engine.
*/
QQmlEngine::~QQmlEngine()
{
    Q_D(QQmlEngine);
    QJSEnginePrivate::removeFromDebugServer(this);

    // Emit onDestruction signals for the root context before
    // we destroy the contexts, engine, Singleton Types etc. that
    // may be required to handle the destruction signal.
    QQmlContextPrivate::get(rootContext())->emitDestruction();

    // clean up all singleton type instances which we own.
    // we do this here and not in the private dtor since otherwise a crash can
    // occur (if we are the QObject parent of the QObject singleton instance)
    // XXX TODO: performance -- store list of singleton types separately?
    const QList<QQmlType> singletonTypes = QQmlMetaType::qmlSingletonTypes();
    for (const QQmlType &currType : singletonTypes)
        d->destroySingletonInstance(currType);

    delete d->rootContext;
    d->rootContext = nullptr;

    d->typeLoader.invalidate();
}

/*! \fn void QQmlEngine::quit()
    This signal is emitted when the QML loaded by the engine would like to quit.

    \sa exit()
 */

/*! \fn void QQmlEngine::exit(int retCode)
    This signal is emitted when the QML loaded by the engine would like to exit
    from the event loop with the specified return code \a retCode.

    \since 5.8
    \sa quit()
 */


/*! \fn void QQmlEngine::warnings(const QList<QQmlError> &warnings)
    This signal is emitted when \a warnings messages are generated by QML.
 */

/*!
  Clears the engine's internal component cache.

  This function causes the property metadata of all components previously
  loaded by the engine to be destroyed.  All previously loaded components and
  the property bindings for all extant objects created from those components will
  cease to function.

  This function returns the engine to a state where it does not contain any loaded
  component data.  This may be useful in order to reload a smaller subset of the
  previous component set, or to load a new version of a previously loaded component.

  Once the component cache has been cleared, components must be loaded before
  any new objects can be created.

  \note Any existing objects created from QML components retain their types,
  even if you clear the component cache. This includes singleton objects. If you
  create more objects from the same QML code after clearing the cache, the new
  objects will be of different types than the old ones. Assigning such a new
  object to a property of its declared type belonging to an object created
  before clearing the cache won't work.

  As a general rule of thumb, make sure that no objects created from QML
  components are alive when you clear the component cache.

  \sa trimComponentCache()
 */
void QQmlEngine::clearComponentCache()
{
    Q_D(QQmlEngine);
    d->typeLoader.lock();
    d->typeLoader.clearCache();
    d->typeLoader.unlock();
}

/*!
  Trims the engine's internal component cache.

  This function causes the property metadata of any loaded components which are
  not currently in use to be destroyed.

  A component is considered to be in use if there are any extant instances of
  the component itself, any instances of other components that use the component,
  or any objects instantiated by any of those components.

  \sa clearComponentCache()
 */
void QQmlEngine::trimComponentCache()
{
    Q_D(QQmlEngine);
    d->typeLoader.trimCache();
}

/*!
  Returns the engine's root context.

  The root context is automatically created by the QQmlEngine.
  Data that should be available to all QML component instances
  instantiated by the engine should be put in the root context.

  Additional data that should only be available to a subset of
  component instances should be added to sub-contexts parented to the
  root context.
*/
QQmlContext *QQmlEngine::rootContext() const
{
    Q_D(const QQmlEngine);
    return d->rootContext;
}

#if QT_DEPRECATED_SINCE(6, 0)
/*!
  \internal
  \deprecated
  This API is private for 5.1

  Returns the last QQmlAbstractUrlInterceptor. It must not be modified outside
  the GUI thread.
*/
QQmlAbstractUrlInterceptor *QQmlEngine::urlInterceptor() const
{
    Q_D(const QQmlEngine);
    return d->urlInterceptors.last();
}
#endif

/*!
  Adds a \a urlInterceptor to be used when resolving URLs in QML.
  This also applies to URLs used for loading script files and QML types.
  The URL interceptors should not be modifed while the engine is loading files,
  or URL selection may be inconsistent. Multiple URL interceptors, when given,
  will be called in the order they were added for each URL.

  QQmlEngine does not take ownership of the interceptor and won't delete it.
*/
void QQmlEngine::addUrlInterceptor(QQmlAbstractUrlInterceptor *urlInterceptor)
{
    Q_D(QQmlEngine);
    d->urlInterceptors.append(urlInterceptor);
}

/*!
  Remove a \a urlInterceptor that was previously added using
  \l addUrlInterceptor. The URL interceptors should not be modifed while the
  engine is loading files, or URL selection may be inconsistent.

  This does not delete the interceptor, but merely removes it from the engine.
  You can re-use it on the same or a different engine afterwards.
*/
void QQmlEngine::removeUrlInterceptor(QQmlAbstractUrlInterceptor *urlInterceptor)
{
    Q_D(QQmlEngine);
    d->urlInterceptors.removeOne(urlInterceptor);
}

/*!
  Run the current URL interceptors on the given \a url of the given \a type and
  return the result.
 */
QUrl QQmlEngine::interceptUrl(const QUrl &url, QQmlAbstractUrlInterceptor::DataType type) const
{
    Q_D(const QQmlEngine);
    QUrl result = url;
    for (QQmlAbstractUrlInterceptor *interceptor : d->urlInterceptors)
        result = interceptor->intercept(result, type);
    return result;
}

/*!
  Returns the list of currently active URL interceptors.
 */
QList<QQmlAbstractUrlInterceptor *> QQmlEngine::urlInterceptors() const
{
    Q_D(const QQmlEngine);
    return d->urlInterceptors;
}

void QQmlEnginePrivate::registerFinalizeCallback(QObject *obj, int index)
{
    if (activeObjectCreator) {
        activeObjectCreator->finalizeCallbacks()->append(qMakePair(QPointer<QObject>(obj), index));
    } else {
        void *args[] = { nullptr };
        QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod, index, args);
    }
}

QSharedPointer<QQmlImageProviderBase> QQmlEnginePrivate::imageProvider(const QString &providerId) const
{
    const QString providerIdLower = providerId.toLower();
    QMutexLocker locker(&mutex);
    return imageProviders.value(providerIdLower);
}

#if QT_CONFIG(qml_network)
/*!
  Sets the \a factory to use for creating QNetworkAccessManager(s).

  QNetworkAccessManager is used for all network access by QML.  By
  implementing a factory it is possible to create custom
  QNetworkAccessManager with specialized caching, proxy and cookie
  support.

  The factory must be set before executing the engine.

  \note QQmlEngine does not take ownership of the factory.
*/
void QQmlEngine::setNetworkAccessManagerFactory(QQmlNetworkAccessManagerFactory *factory)
{
    Q_D(QQmlEngine);
    QMutexLocker locker(&d->networkAccessManagerMutex);
    d->networkAccessManagerFactory = factory;
}

/*!
  Returns the current QQmlNetworkAccessManagerFactory.

  \sa setNetworkAccessManagerFactory()
*/
QQmlNetworkAccessManagerFactory *QQmlEngine::networkAccessManagerFactory() const
{
    Q_D(const QQmlEngine);
    return d->networkAccessManagerFactory;
}

QNetworkAccessManager *QQmlEnginePrivate::createNetworkAccessManager(QObject *parent) const
{
    QMutexLocker locker(&networkAccessManagerMutex);
    QNetworkAccessManager *nam;
    if (networkAccessManagerFactory) {
        nam = networkAccessManagerFactory->create(parent);
    } else {
        nam = new QNetworkAccessManager(parent);
    }

    return nam;
}

QNetworkAccessManager *QQmlEnginePrivate::getNetworkAccessManager() const
{
    Q_Q(const QQmlEngine);
    if (!networkAccessManager)
        networkAccessManager = createNetworkAccessManager(const_cast<QQmlEngine*>(q));
    return networkAccessManager;
}

/*!
  Returns a common QNetworkAccessManager which can be used by any QML
  type instantiated by this engine.

  If a QQmlNetworkAccessManagerFactory has been set and a
  QNetworkAccessManager has not yet been created, the
  QQmlNetworkAccessManagerFactory will be used to create the
  QNetworkAccessManager; otherwise the returned QNetworkAccessManager
  will have no proxy or cache set.

  \sa setNetworkAccessManagerFactory()
*/
QNetworkAccessManager *QQmlEngine::networkAccessManager() const
{
    Q_D(const QQmlEngine);
    return d->getNetworkAccessManager();
}
#endif // qml_network

/*!

  Sets the \a provider to use for images requested via the \e
  image: url scheme, with host \a providerId. The QQmlEngine
  takes ownership of \a provider.

  Image providers enable support for pixmap and threaded image
  requests. See the QQuickImageProvider documentation for details on
  implementing and using image providers.

  All required image providers should be added to the engine before any
  QML sources files are loaded.

  \sa removeImageProvider(), QQuickImageProvider, QQmlImageProviderBase
*/
void QQmlEngine::addImageProvider(const QString &providerId, QQmlImageProviderBase *provider)
{
    Q_D(QQmlEngine);
    QString providerIdLower = providerId.toLower();
    QSharedPointer<QQmlImageProviderBase> sp(provider);
    QMutexLocker locker(&d->mutex);
    d->imageProviders.insert(std::move(providerIdLower), std::move(sp));
}

/*!
  Returns the image provider set for \a providerId if found; otherwise returns \nullptr.

  \sa QQuickImageProvider
*/
QQmlImageProviderBase *QQmlEngine::imageProvider(const QString &providerId) const
{
    Q_D(const QQmlEngine);
    const QString providerIdLower = providerId.toLower();
    QMutexLocker locker(&d->mutex);
    return d->imageProviders.value(providerIdLower).data();
}

/*!
  Removes the image provider for \a providerId.

  \sa addImageProvider(), QQuickImageProvider
*/
void QQmlEngine::removeImageProvider(const QString &providerId)
{
    Q_D(QQmlEngine);
    const QString providerIdLower = providerId.toLower();
    QMutexLocker locker(&d->mutex);
    d->imageProviders.take(providerIdLower);
}

/*!
  Return the base URL for this engine.  The base URL is only used to
  resolve components when a relative URL is passed to the
  QQmlComponent constructor.

  If a base URL has not been explicitly set, this method returns the
  application's current working directory.

  \sa setBaseUrl()
*/
QUrl QQmlEngine::baseUrl() const
{
    Q_D(const QQmlEngine);
    if (d->baseUrl.isEmpty()) {
        const QString currentPath = QDir::currentPath();
        const QString rootPath = QDir::rootPath();
        return QUrl::fromLocalFile((currentPath == rootPath) ? rootPath : (currentPath + QDir::separator()));
    } else {
        return d->baseUrl;
    }
}

/*!
  Set the  base URL for this engine to \a url.

  \sa baseUrl()
*/
void QQmlEngine::setBaseUrl(const QUrl &url)
{
    Q_D(QQmlEngine);
    d->baseUrl = url;
}

/*!
  Returns true if warning messages will be output to stderr in addition
  to being emitted by the warnings() signal, otherwise false.

  The default value is true.
*/
bool QQmlEngine::outputWarningsToStandardError() const
{
    Q_D(const QQmlEngine);
    return d->outputWarningsToMsgLog;
}

/*!
  Set whether warning messages will be output to stderr to \a enabled.

  If \a enabled is true, any warning messages generated by QML will be
  output to stderr and emitted by the warnings() signal.  If \a enabled
  is false, only the warnings() signal will be emitted.  This allows
  applications to handle warning output themselves.

  The default value is true.
*/
void QQmlEngine::setOutputWarningsToStandardError(bool enabled)
{
    Q_D(QQmlEngine);
    d->outputWarningsToMsgLog = enabled;
}

/*!
  \internal

  Capture the given property as part of a binding.
 */
void QQmlEngine::captureProperty(QObject *object, const QMetaProperty &property) const
{
    Q_D(const QQmlEngine);
    if (d->propertyCapture && !property.isConstant()) {
        d->propertyCapture->captureProperty(
                    object, property.propertyIndex(),
                    QMetaObjectPrivate::signalIndex(property.notifySignal()));
    }
}

/*!
  \qmlproperty string Qt::uiLanguage
  \since 5.15

  The uiLanguage holds the name of the language to be used for user interface
  string translations. It is exposed in C++ as QQmlEngine::uiLanguage property.

  You can set the value freely and use it in bindings. It is recommended to set it
  after installing translators in your application. By convention, an empty string
  means no translation from the language used in the source code is intended to occur.

  If you're using QQmlApplicationEngine and the value changes, QQmlEngine::retranslate()
  will be called.
*/

/*!
  \fn template<typename T> T QQmlEngine::singletonInstance(int qmlTypeId)

  Returns the instance of a singleton type that was registered under \a qmlTypeId.

  The template argument \e T may be either QJSValue or a pointer to a QObject-derived
  type and depends on how the singleton was registered. If no instance of \e T has been
  created yet, it is created now. If \a qmlTypeId does not represent a valid singleton
  type, either a default constructed QJSValue or a \c nullptr is returned.

  QObject* example:

  \snippet code/src_qml_qqmlengine.cpp 0
  \codeline
  \snippet code/src_qml_qqmlengine.cpp 1
  \codeline
  \snippet code/src_qml_qqmlengine.cpp 2

  QJSValue example:

  \snippet code/src_qml_qqmlengine.cpp 3
  \codeline
  \snippet code/src_qml_qqmlengine.cpp 4

  It is recommended to store the QML type id, e.g. as a static member in the
  singleton class. The lookup via qmlTypeId() is costly.

  \sa QML_SINGLETON, qmlRegisterSingletonType(), qmlTypeId()
  \since 5.12
*/
template<>
QJSValue QQmlEngine::singletonInstance<QJSValue>(int qmlTypeId)
{
    Q_D(QQmlEngine);
    QQmlType type = QQmlMetaType::qmlType(qmlTypeId, QQmlMetaType::TypeIdCategory::QmlType);

    if (!type.isValid() || !type.isSingleton())
        return QJSValue();

    return d->singletonInstance<QJSValue>(type);
}

/*!
  Refreshes all binding expressions that use strings marked for translation.

  Call this function after you have installed a new translator with
  QCoreApplication::installTranslator, to ensure that your user-interface
  shows up-to-date translations.

  \since 5.10
*/
void QQmlEngine::retranslate()
{
    Q_D(QQmlEngine);
    d->translationLanguage.notify();
}

/*!
  Returns the QQmlContext for the \a object, or 0 if no
  context has been set.

  When the QQmlEngine instantiates a QObject, the context is
  set automatically.

  \sa qmlContext(), qmlEngine()
  */
QQmlContext *QQmlEngine::contextForObject(const QObject *object)
{
    if(!object)
        return nullptr;

    QQmlData *data = QQmlData::get(object);
    if (data && data->outerContext)
        return data->outerContext->asQQmlContext();

    return nullptr;
}

/*!
  Sets the QQmlContext for the \a object to \a context.
  If the \a object already has a context, a warning is
  output, but the context is not changed.

  When the QQmlEngine instantiates a QObject, the context is
  set automatically.
 */
void QQmlEngine::setContextForObject(QObject *object, QQmlContext *context)
{
    if (!object || !context)
        return;

    QQmlData *data = QQmlData::get(object, true);
    if (data->context) {
        qWarning("QQmlEngine::setContextForObject(): Object already has a QQmlContext");
        return;
    }

    QQmlRefPointer<QQmlContextData> contextData = QQmlContextData::get(context);
    Q_ASSERT(data->context == nullptr);
    data->context = contextData.data();
    contextData->addOwnedObject(data);
}

/*!
   \reimp
*/
bool QQmlEngine::event(QEvent *e)
{
    Q_D(QQmlEngine);
    if (e->type() == QEvent::User)
        d->doDeleteInEngineThread();
    else if (e->type() == QEvent::LanguageChange) {
        retranslate();
    }

    return QJSEngine::event(e);
}

void QQmlEnginePrivate::doDeleteInEngineThread()
{
    QFieldList<Deletable, &Deletable::next> list;
    mutex.lock();
    list.copyAndClear(toDeleteInEngineThread);
    mutex.unlock();

    while (Deletable *d = list.takeFirst())
        delete d;
}

class QQmlDataExtended {
public:
    QQmlDataExtended();
    ~QQmlDataExtended();

    QHash<QQmlAttachedPropertiesFunc, QObject *> attachedProperties;
};

QQmlDataExtended::QQmlDataExtended()
{
}

QQmlDataExtended::~QQmlDataExtended()
{
}

void QQmlData::NotifyList::layout(QQmlNotifierEndpoint *endpoint)
{
    // Add a temporary sentinel at beginning of list. This will be overwritten
    // when the end point is inserted into the notifies further down.
    endpoint->prev = nullptr;

    while (endpoint->next) {
        Q_ASSERT(reinterpret_cast<QQmlNotifierEndpoint *>(endpoint->next->prev) == endpoint);
        endpoint = endpoint->next;
    }

    while (endpoint) {
        QQmlNotifierEndpoint *ep = (QQmlNotifierEndpoint *) endpoint->prev;

        int index = endpoint->sourceSignal;
        index = qMin(index, 0xFFFF - 1);

        endpoint->next = notifies[index];
        if (endpoint->next) endpoint->next->prev = &endpoint->next;
        endpoint->prev = &notifies[index];
        notifies[index] = endpoint;

        endpoint = ep;
    }
}

void QQmlData::NotifyList::layout()
{
    Q_ASSERT(maximumTodoIndex >= notifiesSize);

    if (todo) {
        QQmlNotifierEndpoint **old = notifies;
        const int reallocSize = (maximumTodoIndex + 1) * sizeof(QQmlNotifierEndpoint*);
        notifies = (QQmlNotifierEndpoint**)realloc(notifies, reallocSize);
        const int memsetSize = (maximumTodoIndex - notifiesSize + 1) *
                               sizeof(QQmlNotifierEndpoint*);
        memset(notifies + notifiesSize, 0, memsetSize);

        if (notifies != old) {
            for (int ii = 0; ii < notifiesSize; ++ii)
                if (notifies[ii])
                    notifies[ii]->prev = &notifies[ii];
        }

        notifiesSize = maximumTodoIndex + 1;

        layout(todo);
    }

    maximumTodoIndex = 0;
    todo = nullptr;
}

void QQmlData::deferData(
        int objectIndex, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
        const QQmlRefPointer<QQmlContextData> &context)
{
    QQmlData::DeferredData *deferData = new QQmlData::DeferredData;
    deferData->deferredIdx = objectIndex;
    deferData->compilationUnit = compilationUnit;
    deferData->context = context;

    const QV4::CompiledData::Object *compiledObject = compilationUnit->objectAt(objectIndex);
    const QV4::BindingPropertyData &propertyData = compilationUnit->bindingPropertyDataPerObject.at(objectIndex);

    const QV4::CompiledData::Binding *binding = compiledObject->bindingTable();
    for (quint32 i = 0; i < compiledObject->nBindings; ++i, ++binding) {
        const QQmlPropertyData *property = propertyData.at(i);
        if (property && binding->flags & QV4::CompiledData::Binding::IsDeferredBinding)
            deferData->bindings.insert(property->coreIndex(), binding);
    }

    deferredData.append(deferData);
}

void QQmlData::releaseDeferredData()
{
    auto it = deferredData.begin();
    while (it != deferredData.end()) {
        DeferredData *deferData = *it;
        if (deferData->bindings.isEmpty()) {
            delete deferData;
            it = deferredData.erase(it);
        } else {
            ++it;
        }
    }
}

void QQmlData::addNotify(int index, QQmlNotifierEndpoint *endpoint)
{
    if (!notifyList) {
        notifyList = (NotifyList *)malloc(sizeof(NotifyList));
        notifyList->connectionMask = 0;
        notifyList->maximumTodoIndex = 0;
        notifyList->notifiesSize = 0;
        notifyList->todo = nullptr;
        notifyList->notifies = nullptr;
    }

    Q_ASSERT(!endpoint->isConnected());

    index = qMin(index, 0xFFFF - 1);
    notifyList->connectionMask |= (1ULL << quint64(index % 64));

    if (index < notifyList->notifiesSize) {

        endpoint->next = notifyList->notifies[index];
        if (endpoint->next) endpoint->next->prev = &endpoint->next;
        endpoint->prev = &notifyList->notifies[index];
        notifyList->notifies[index] = endpoint;

    } else {
        notifyList->maximumTodoIndex = qMax(int(notifyList->maximumTodoIndex), index);

        endpoint->next = notifyList->todo;
        if (endpoint->next) endpoint->next->prev = &endpoint->next;
        endpoint->prev = &notifyList->todo;
        notifyList->todo = endpoint;
    }
}

void QQmlData::disconnectNotifiers()
{
    if (notifyList) {
        while (notifyList->todo)
            notifyList->todo->disconnect();
        for (int ii = 0; ii < notifyList->notifiesSize; ++ii) {
            while (QQmlNotifierEndpoint *ep = notifyList->notifies[ii])
                ep->disconnect();
        }
        free(notifyList->notifies);
        free(notifyList);
        notifyList = nullptr;
    }
}

QHash<QQmlAttachedPropertiesFunc, QObject *> *QQmlData::attachedProperties() const
{
    if (!extendedData) extendedData = new QQmlDataExtended;
    return &extendedData->attachedProperties;
}

void QQmlData::destroyed(QObject *object)
{
    if (nextContextObject)
        nextContextObject->prevContextObject = prevContextObject;
    if (prevContextObject)
        *prevContextObject = nextContextObject;
    else if (outerContext && outerContext->ownedObjects() == this)
        outerContext->setOwnedObjects(nextContextObject);

    QQmlAbstractBinding *binding = bindings;
    while (binding) {
        binding->setAddedToObject(false);
        binding = binding->nextBinding();
    }
    if (bindings && !bindings->ref.deref())
        delete bindings;

    compilationUnit = nullptr;

    qDeleteAll(deferredData);
    deferredData.clear();

    QQmlBoundSignal *signalHandler = signalHandlers;
    while (signalHandler) {
        if (signalHandler->isNotifying()) {
            // The object is being deleted during signal handler evaluation.
            // This will cause a crash due to invalid memory access when the
            // evaluation has completed.
            // Abort with a friendly message instead.
            QString locationString;
            QQmlBoundSignalExpression *expr = signalHandler->expression();
            if (expr) {
                QQmlSourceLocation location = expr->sourceLocation();
                if (location.sourceFile.isEmpty())
                    location.sourceFile = QStringLiteral("<Unknown File>");
                locationString.append(location.sourceFile);
                locationString.append(QStringLiteral(":%0: ").arg(location.line));
                QString source = expr->expression();
                if (source.size() > 100) {
                    source.truncate(96);
                    source.append(QLatin1String(" ..."));
                }
                locationString.append(source);
            } else {
                locationString = QStringLiteral("<Unknown Location>");
            }
            qFatal("Object %p destroyed while one of its QML signal handlers is in progress.\n"
                   "Most likely the object was deleted synchronously (use QObject::deleteLater() "
                   "instead), or the application is running a nested event loop.\n"
                   "This behavior is NOT supported!\n"
                   "%s", object, qPrintable(locationString));
        }

        QQmlBoundSignal *next = signalHandler->m_nextSignal;
        signalHandler->m_prevSignal = nullptr;
        signalHandler->m_nextSignal = nullptr;
        delete signalHandler;
        signalHandler = next;
    }

    if (bindingBitsArraySize > InlineBindingArraySize)
        free(bindingBits);

    if (propertyCache)
        propertyCache->release();

    ownContext = nullptr;

    while (guards) {
        QQmlGuard<QObject> *guard = static_cast<QQmlGuard<QObject> *>(guards);
        *guard = (QObject *)nullptr;
        guard->objectDestroyed(object);
    }

    disconnectNotifiers();

    if (extendedData)
        delete extendedData;

    // Dispose the handle.
    jsWrapper.clear();

    if (ownMemory)
        delete this;
    else
        this->~QQmlData();
}

QQmlData::BindingBitsType *QQmlData::growBits(QObject *obj, int bit)
{
    BindingBitsType *bits = (bindingBitsArraySize == InlineBindingArraySize) ? bindingBitsValue : bindingBits;
    int props = QQmlMetaObject(obj).propertyCount();
    Q_ASSERT(bit < 2 * props);
    Q_UNUSED(bit); // .. for Q_NO_DEBUG mode when the assert above expands to empty

    uint arraySize = (2 * static_cast<uint>(props) + BitsPerType - 1) / BitsPerType;
    Q_ASSERT(arraySize > 1);
    Q_ASSERT(arraySize <= 0xffff); // max for bindingBitsArraySize

    BindingBitsType *newBits = static_cast<BindingBitsType *>(malloc(arraySize*sizeof(BindingBitsType)));
    memcpy(newBits, bits, bindingBitsArraySize * sizeof(BindingBitsType));
    memset(newBits + bindingBitsArraySize, 0, sizeof(BindingBitsType) * (arraySize - bindingBitsArraySize));

    if (bindingBitsArraySize > InlineBindingArraySize)
        free(bits);
    bindingBits = newBits;
    bits = newBits;
    bindingBitsArraySize = arraySize;
    return bits;
}

QQmlData *QQmlData::createQQmlData(QObjectPrivate *priv)
{
    Q_ASSERT(priv);
    Q_ASSERT(!priv->isDeletingChildren);
    priv->declarativeData = new QQmlData;
    return static_cast<QQmlData *>(priv->declarativeData);
}

QQmlPropertyCache *QQmlData::createPropertyCache(QJSEngine *engine, QObject *object)
{
    QQmlData *ddata = QQmlData::get(object, /*create*/true);
    ddata->propertyCache = QJSEnginePrivate::get(engine)->cache(object, QTypeRevision {}, true);
    return ddata->propertyCache;
}

void QQmlEnginePrivate::sendQuit()
{
    Q_Q(QQmlEngine);
    emit q->quit();
    if (q->receivers(SIGNAL(quit())) == 0) {
        qWarning("Signal QQmlEngine::quit() emitted, but no receivers connected to handle it.");
    }
}

void QQmlEnginePrivate::sendExit(int retCode)
{
    Q_Q(QQmlEngine);
    if (q->receivers(SIGNAL(exit(int))) == 0)
        qWarning("Signal QQmlEngine::exit() emitted, but no receivers connected to handle it.");
    emit q->exit(retCode);
}

static void dumpwarning(const QQmlError &error)
{
    switch (error.messageType()) {
    case QtDebugMsg:
        QMessageLogger(error.url().toString().toLatin1().constData(),
                       error.line(), nullptr).debug().noquote().nospace()
                << error.toString();
        break;
    case QtInfoMsg:
        QMessageLogger(error.url().toString().toLatin1().constData(),
                       error.line(), nullptr).info().noquote().nospace()
                << error.toString();
        break;
    case QtWarningMsg:
    case QtFatalMsg: // fatal does not support streaming, and furthermore, is actually fatal. Probably not desirable for QML.
        QMessageLogger(error.url().toString().toLatin1().constData(),
                       error.line(), nullptr).warning().noquote().nospace()
                << error.toString();
        break;
    case QtCriticalMsg:
        QMessageLogger(error.url().toString().toLatin1().constData(),
                       error.line(), nullptr).critical().noquote().nospace()
                << error.toString();
        break;
    }
}

static void dumpwarning(const QList<QQmlError> &errors)
{
    for (int ii = 0; ii < errors.count(); ++ii)
        dumpwarning(errors.at(ii));
}

void QQmlEnginePrivate::warning(const QQmlError &error)
{
    Q_Q(QQmlEngine);
    q->warnings(QList<QQmlError>() << error);
    if (outputWarningsToMsgLog)
        dumpwarning(error);
}

void QQmlEnginePrivate::warning(const QList<QQmlError> &errors)
{
    Q_Q(QQmlEngine);
    q->warnings(errors);
    if (outputWarningsToMsgLog)
        dumpwarning(errors);
}

void QQmlEnginePrivate::warning(QQmlEngine *engine, const QQmlError &error)
{
    if (engine)
        QQmlEnginePrivate::get(engine)->warning(error);
    else
        dumpwarning(error);
}

void QQmlEnginePrivate::warning(QQmlEngine *engine, const QList<QQmlError> &error)
{
    if (engine)
        QQmlEnginePrivate::get(engine)->warning(error);
    else
        dumpwarning(error);
}

void QQmlEnginePrivate::warning(QQmlEnginePrivate *engine, const QQmlError &error)
{
    if (engine)
        engine->warning(error);
    else
        dumpwarning(error);
}

void QQmlEnginePrivate::warning(QQmlEnginePrivate *engine, const QList<QQmlError> &error)
{
    if (engine)
        engine->warning(error);
    else
        dumpwarning(error);
}

QList<QQmlError> QQmlEnginePrivate::qmlErrorFromDiagnostics(
        const QString &fileName, const QList<QQmlJS::DiagnosticMessage> &diagnosticMessages)
{
    QList<QQmlError> errors;
    for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
        if (m.isWarning()) {
            qWarning("%s:%d : %s", qPrintable(fileName), m.loc.startLine, qPrintable(m.message));
            continue;
        }

        QQmlError error;
        error.setUrl(QUrl(fileName));
        error.setDescription(m.message);
        error.setLine(qmlConvertSourceCoordinate<quint32, int>(m.loc.startLine));
        error.setColumn(qmlConvertSourceCoordinate<quint32, int>(m.loc.startColumn));
        errors << error;
    }
    return errors;
}

void QQmlEnginePrivate::cleanupScarceResources()
{
    // iterate through the list and release them all.
    // note that the actual SRD is owned by the JS engine,
    // so we cannot delete the SRD; but we can free the
    // memory used by the variant in the SRD.
    QV4::ExecutionEngine *engine = v4engine();
    while (QV4::ExecutionEngine::ScarceResourceData *sr = engine->scarceResources.first()) {
        sr->data = QVariant();
        engine->scarceResources.remove(sr);
    }
}

/*!
  Adds \a path as a directory where the engine searches for
  installed modules in a URL-based directory structure.

  The \a path may be a local filesystem directory, a
  \l {The Qt Resource System}{Qt Resource} path (\c {:/imports}), a
  \l {The Qt Resource System}{Qt Resource} url (\c {qrc:/imports}) or a URL.

  The \a path will be converted into canonical form before it
  is added to the import path list.

  The newly added \a path will be first in the importPathList().

  \sa setImportPathList(), {QML Modules}
*/
void QQmlEngine::addImportPath(const QString& path)
{
    Q_D(QQmlEngine);
    d->importDatabase.addImportPath(path);
}

/*!
  Returns the list of directories where the engine searches for
  installed modules in a URL-based directory structure.

  For example, if \c /opt/MyApp/lib/imports is in the path, then QML that
  imports \c com.mycompany.Feature will cause the QQmlEngine to look
  in \c /opt/MyApp/lib/imports/com/mycompany/Feature/ for the components
  provided by that module. A \c qmldir file is required for defining the
  type version mapping and possibly QML extensions plugins.

  By default, the list contains the directory of the application executable,
  paths specified in the \c QML_IMPORT_PATH environment variable,
  and the builtin \c QmlImportsPath from QLibraryInfo.

  \sa addImportPath(), setImportPathList()
*/
QStringList QQmlEngine::importPathList() const
{
    Q_D(const QQmlEngine);
    return d->importDatabase.importPathList();
}

/*!
  Sets \a paths as the list of directories where the engine searches for
  installed modules in a URL-based directory structure.

  By default, the list contains the directory of the application executable,
  paths specified in the \c QML_IMPORT_PATH environment variable,
  and the builtin \c QmlImportsPath from QLibraryInfo.

  \sa importPathList(), addImportPath()
  */
void QQmlEngine::setImportPathList(const QStringList &paths)
{
    Q_D(QQmlEngine);
    d->importDatabase.setImportPathList(paths);
}


/*!
  Adds \a path as a directory where the engine searches for
  native plugins for imported modules (referenced in the \c qmldir file).

  By default, the list contains only \c .,  i.e. the engine searches
  in the directory of the \c qmldir file itself.

  The newly added \a path will be first in the pluginPathList().

  \sa setPluginPathList()
*/
void QQmlEngine::addPluginPath(const QString& path)
{
    Q_D(QQmlEngine);
    d->importDatabase.addPluginPath(path);
}

/*!
  Returns the list of directories where the engine searches for
  native plugins for imported modules (referenced in the \c qmldir file).

  By default, the list contains only \c .,  i.e. the engine searches
  in the directory of the \c qmldir file itself.

  \sa addPluginPath(), setPluginPathList()
*/
QStringList QQmlEngine::pluginPathList() const
{
    Q_D(const QQmlEngine);
    return d->importDatabase.pluginPathList();
}

/*!
  Sets the list of directories where the engine searches for
  native plugins for imported modules (referenced in the \c qmldir file)
  to \a paths.

  By default, the list contains only \c .,  i.e. the engine searches
  in the directory of the \c qmldir file itself.

  \sa pluginPathList(), addPluginPath()
  */
void QQmlEngine::setPluginPathList(const QStringList &paths)
{
    Q_D(QQmlEngine);
    d->importDatabase.setPluginPathList(paths);
}

#if QT_CONFIG(library)
/*!
  Imports the plugin named \a filePath with the \a uri provided.
  Returns true if the plugin was successfully imported; otherwise returns false.

  On failure and if non-null, the \a errors list will have any errors which occurred prepended to it.

  The plugin has to be a Qt plugin which implements the QQmlEngineExtensionPlugin interface.
*/
bool QQmlEngine::importPlugin(const QString &filePath, const QString &uri, QList<QQmlError> *errors)
{
    Q_D(QQmlEngine);
    QQmlTypeLoaderQmldirContent qmldir;
    QQmlPluginImporter importer(
                uri, QTypeRevision(), &d->importDatabase, &qmldir, &d->typeLoader, errors);
    return importer.importDynamicPlugin(filePath, uri, false).isValid();
}
#endif

/*!
  \property QQmlEngine::offlineStoragePath
  \brief the directory for storing offline user data

  Returns the directory where SQL and other offline
  storage is placed.

  The SQL databases created with \c openDatabaseSync() are stored here.
  \sa \l{Qt Quick Local Storage QML Types}

  The default is QML/OfflineStorage in the platform-standard
  user application data directory.

  Note that the path may not currently exist on the filesystem, so
  callers wanting to \e create new files at this location should create
  it first - see QDir::mkpath().
*/
void QQmlEngine::setOfflineStoragePath(const QString& dir)
{
    Q_D(QQmlEngine);
    d->offlineStoragePath = dir;
}

QString QQmlEngine::offlineStoragePath() const
{
    Q_D(const QQmlEngine);

    if (d->offlineStoragePath.isEmpty()) {
        QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QQmlEnginePrivate *e = const_cast<QQmlEnginePrivate *>(d);
        if (!dataLocation.isEmpty())
            e->offlineStoragePath = dataLocation.replace(QLatin1Char('/'), QDir::separator())
                                  + QDir::separator() + QLatin1String("QML")
                                  + QDir::separator() + QLatin1String("OfflineStorage");
    }

    return d->offlineStoragePath;
}

/*!
  Returns the file path where a \l{QtQuick.LocalStorage}{Local Storage}
  database with the identifier \a databaseName is (or would be) located.

  \sa {openDatabaseSync}{LocalStorage.openDatabaseSync()}
  \since 5.9
*/
QString QQmlEngine::offlineStorageDatabaseFilePath(const QString &databaseName) const
{
    Q_D(const QQmlEngine);
    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(databaseName.toUtf8());
    return d->offlineStorageDatabaseDirectory() + QLatin1String(md5.result().toHex());
}

QString QQmlEnginePrivate::offlineStorageDatabaseDirectory() const
{
    Q_Q(const QQmlEngine);
    return q->offlineStoragePath() + QDir::separator() + QLatin1String("Databases") + QDir::separator();
}

static QQmlPropertyCache *propertyCacheForPotentialInlineComponentType(int t, const QHash<int, QV4::ExecutableCompilationUnit *>::const_iterator &iter) {
    if (t != (*iter)->typeIds.id.id()) {
        // this is an inline component, and what we have in the iterator is currently the parent compilation unit
        for (auto &&icDatum: (*iter)->inlineComponentData)
            if (icDatum.typeIds.id.id() == t)
                return (*iter)->propertyCaches.at(icDatum.objectIndex);
    }
    return (*iter)->rootPropertyCache().data();
}

/*!
 * \internal
 *
 * Look up by type's baseMetaObject.
 */
QQmlMetaObject QQmlEnginePrivate::rawMetaObjectForType(int t) const
{
    if (QQmlPropertyCache *composite = findPropertyCacheInCompositeTypes(t))
        return QQmlMetaObject(composite);

    QQmlType type = QQmlMetaType::qmlType(t);
    return QQmlMetaObject(type.baseMetaObject());
}

/*!
 * \internal
 *
 * Look up by type's metaObject.
 */
QQmlMetaObject QQmlEnginePrivate::metaObjectForType(int t) const
{
    if (QQmlPropertyCache *composite = findPropertyCacheInCompositeTypes(t))
        return QQmlMetaObject(composite);

    QQmlType type = QQmlMetaType::qmlType(t);
    return QQmlMetaObject(type.metaObject());
}

/*!
 * \internal
 *
 * Look up by type's metaObject and version.
 */
QQmlPropertyCache *QQmlEnginePrivate::propertyCacheForType(int t)
{
    if (QQmlPropertyCache *composite = findPropertyCacheInCompositeTypes(t))
        return composite;

    QQmlType type = QQmlMetaType::qmlType(t);
    return type.isValid() ? cache(type.metaObject(), type.version()) : nullptr;
}

/*!
 * \internal
 *
 * Look up by type's baseMetaObject and unspecified/any version.
 * TODO: Is this correct? Passing a plain QTypeRevision() rather than QTypeRevision::zero() or
 *       the actual type's version seems strange. The behavior has been in place for a while.
 */
QQmlPropertyCache *QQmlEnginePrivate::rawPropertyCacheForType(int t)
{
    if (QQmlPropertyCache *composite = findPropertyCacheInCompositeTypes(t))
        return composite;

    QQmlType type = QQmlMetaType::qmlType(t);
    return type.isValid() ? cache(type.baseMetaObject(), QTypeRevision()) : nullptr;
}

/*!
 * \internal
 *
 * Look up by QQmlType and version. We only fall back to lookup by metaobject if the type
 * has no revisiononed attributes here. Unspecified versions are interpreted as "any".
 */
QQmlPropertyCache *QQmlEnginePrivate::rawPropertyCacheForType(int t, QTypeRevision version)
{
    if (QQmlPropertyCache *composite = findPropertyCacheInCompositeTypes(t))
        return composite;

    QQmlType type = QQmlMetaType::qmlType(t);
    if (!type.isValid())
        return nullptr;

    if (type.containsRevisionedAttributes())
        return QQmlMetaType::propertyCache(type, version);

    if (const QMetaObject *metaObject = type.metaObject())
        return cache(metaObject, version);

    return nullptr;
}

QQmlPropertyCache *QQmlEnginePrivate::findPropertyCacheInCompositeTypes(int t) const
{
    Locker locker(this);
    auto iter = m_compositeTypes.constFind(t);
    return (iter == m_compositeTypes.constEnd())
            ? nullptr
            : propertyCacheForPotentialInlineComponentType(t, iter);
}

void QQmlEnginePrivate::registerInternalCompositeType(QV4::ExecutableCompilationUnit *compilationUnit)
{
    compilationUnit->isRegisteredWithEngine = true;

    Locker locker(this);
    // The QQmlCompiledData is not referenced here, but it is removed from this
    // hash in the QQmlCompiledData destructor
    m_compositeTypes.insert(compilationUnit->typeIds.id.id(), compilationUnit);
    for (auto &&data: compilationUnit->inlineComponentData)
        m_compositeTypes.insert(data.typeIds.id.id(), compilationUnit);
}

void QQmlEnginePrivate::unregisterInternalCompositeType(QV4::ExecutableCompilationUnit *compilationUnit)
{
    compilationUnit->isRegisteredWithEngine = false;

    Locker locker(this);
    m_compositeTypes.remove(compilationUnit->typeIds.id.id());
    for (auto&& icDatum: compilationUnit->inlineComponentData)
        m_compositeTypes.remove(icDatum.typeIds.id.id());
}

QV4::ExecutableCompilationUnit *QQmlEnginePrivate::obtainExecutableCompilationUnit(int typeId)
{
    Locker locker(this);
    return m_compositeTypes.value(typeId, nullptr);
}

template<>
QJSValue QQmlEnginePrivate::singletonInstance<QJSValue>(const QQmlType &type)
{
    Q_Q(QQmlEngine);

    QJSValue value = singletonInstances.value(type);
    if (!value.isUndefined()) {
        return value;
    }

    QQmlType::SingletonInstanceInfo *siinfo = type.singletonInstanceInfo();
    Q_ASSERT(siinfo != nullptr);

    if (siinfo->scriptCallback) {
        value = siinfo->scriptCallback(q, q);
        if (value.isQObject()) {
            QObject *o = value.toQObject();
            // even though the object is defined in C++, qmlContext(obj) and qmlEngine(obj)
            // should behave identically to QML singleton types.
            q->setContextForObject(o, new QQmlContext(q->rootContext(), q));
        }
        singletonInstances.convertAndInsert(v4engine(), type, &value);

    } else if (siinfo->qobjectCallback) {
        QObject *o = siinfo->qobjectCallback(q, q);
        if (!o) {
            QQmlError error;
            error.setMessageType(QtMsgType::QtCriticalMsg);
            error.setDescription(QString::asprintf("qmlRegisterSingletonType(): \"%s\" is not available because the callback function returns a null pointer.",
                                                   qPrintable(QString::fromUtf8(type.typeName()))));
            warning(error);
        } else {
            type.createProxy(o);

            // if this object can use a property cache, create it now
            QQmlData::ensurePropertyCache(q, o);
        }
        // even though the object is defined in C++, qmlContext(obj) and qmlEngine(obj)
        // should behave identically to QML singleton types.
        q->setContextForObject(o, new QQmlContext(q->rootContext(), q));
        value = q->newQObject(o);
        singletonInstances.convertAndInsert(v4engine(), type, &value);
    } else if (!siinfo->url.isEmpty()) {
        QQmlComponent component(q, siinfo->url, QQmlComponent::PreferSynchronous);
        if (component.isError()) {
            warning(component.errors());
            v4engine()->throwError(QLatin1String("Due to the preceding error(s), Singleton \"%1\" could not be loaded.").arg(QString::fromUtf8(type.typeName())));

            return QJSValue(QJSValue::UndefinedValue);
        }
        QObject *o = component.beginCreate(q->rootContext());
        value = q->newQObject(o);
        singletonInstances.convertAndInsert(v4engine(), type, &value);
        component.completeCreate();
    }

    return value;
}

void QQmlEnginePrivate::destroySingletonInstance(const QQmlType &type)
{
    Q_ASSERT(type.isSingleton() || type.isCompositeSingleton());

    QObject* o = singletonInstances.take(type).toQObject();
    if (o) {
        QQmlData *ddata = QQmlData::get(o, false);
        if (type.singletonInstanceInfo()->url.isEmpty() && ddata && ddata->indestructible && ddata->explicitIndestructibleSet)
            return;
        delete o;
    }
}

bool QQmlEnginePrivate::isTypeLoaded(const QUrl &url) const
{
    return typeLoader.isTypeLoaded(url);
}

bool QQmlEnginePrivate::isScriptLoaded(const QUrl &url) const
{
    return typeLoader.isScriptLoaded(url);
}

void QQmlEnginePrivate::executeRuntimeFunction(const QUrl &url, qsizetype functionIndex,
                                               QObject *thisObject, int argc, void **args,
                                               QMetaType *types)
{
    Q_Q(QQmlEngine);
    const auto unit = typeLoader.getType(url)->compilationUnit();
    if (!unit)
        return;

    Q_ASSERT(functionIndex >= 0);
    Q_ASSERT(thisObject);

    if (!unit->engine)
        unit->linkToEngine(q->handle());

    if (unit->runtimeFunctions.length() <= functionIndex)
        return;

    QQmlContext *ctx = q->contextForObject(thisObject);
    if (!ctx)
        ctx = q->rootContext();

    // implicitly sets the return value, if it is present
    q->handle()->callInContext(unit->runtimeFunctions[functionIndex], thisObject,
                               QQmlContextData::get(ctx), argc, args, types);
}

#if defined(Q_OS_WIN)
// Normalize a file name using Shell API. As opposed to converting it
// to a short 8.3 name and back, this also works for drives where 8.3 notation
// is disabled (see 8dot3name options of fsutil.exe).
static inline QString shellNormalizeFileName(const QString &name)
{
    const QString nativeSeparatorName(QDir::toNativeSeparators(name));
    const LPCTSTR nameC = reinterpret_cast<LPCTSTR>(nativeSeparatorName.utf16());
// The correct declaration of the SHGetPathFromIDList symbol is
// being used in mingw-w64 as of r6215, which is a v3 snapshot.
#if defined(Q_CC_MINGW) && (!defined(__MINGW64_VERSION_MAJOR) || __MINGW64_VERSION_MAJOR < 3)
    ITEMIDLIST *file;
    if (FAILED(SHParseDisplayName(nameC, NULL, reinterpret_cast<LPITEMIDLIST>(&file), 0, NULL)))
        return name;
#else
    PIDLIST_ABSOLUTE file;
    if (FAILED(SHParseDisplayName(nameC, NULL, &file, 0, NULL)))
        return name;
#endif
    TCHAR buffer[MAX_PATH];
    bool gotPath = SHGetPathFromIDList(file, buffer);
    ILFree(file);

    if (!gotPath)
        return name;

    QString canonicalName = QString::fromWCharArray(buffer);
    // Upper case drive letter
    if (canonicalName.size() > 2 && canonicalName.at(1) == QLatin1Char(':'))
        canonicalName[0] = canonicalName.at(0).toUpper();
    return QDir::cleanPath(canonicalName);
}
#endif // Q_OS_WIN

bool QQml_isFileCaseCorrect(const QString &fileName, int lengthIn /* = -1 */)
{
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    QFileInfo info(fileName);
    const QString absolute = info.absoluteFilePath();

#if defined(Q_OS_DARWIN)
    const QString canonical = info.canonicalFilePath();
#elif defined(Q_OS_WIN)
    // No difference if the path is qrc based
    if (absolute[0] == QLatin1Char(':'))
        return true;
    const QString canonical = shellNormalizeFileName(absolute);
#endif

    const int absoluteLength = absolute.length();
    const int canonicalLength = canonical.length();

    int length = qMin(absoluteLength, canonicalLength);
    if (lengthIn >= 0) {
        length = qMin(lengthIn, length);
    } else {
        // No length given: Limit to file name. Do not trigger
        // on drive letters or folder names.
        int lastSlash = absolute.lastIndexOf(QLatin1Char('/'));
        if (lastSlash < 0)
            lastSlash = absolute.lastIndexOf(QLatin1Char('\\'));
        if (lastSlash >= 0) {
            const int fileNameLength = absoluteLength - 1 - lastSlash;
            length = qMin(length, fileNameLength);
        }
    }

    for (int ii = 0; ii < length; ++ii) {
        const QChar &a = absolute.at(absoluteLength - 1 - ii);
        const QChar &c = canonical.at(canonicalLength - 1 - ii);

        if (a.toLower() != c.toLower())
            return true;
        if (a != c)
            return false;
    }
#else
    Q_UNUSED(lengthIn);
    Q_UNUSED(fileName);
#endif
    return true;
}

/*!
    \fn QQmlEngine *qmlEngine(const QObject *object)
    \relates QQmlEngine

    Returns the QQmlEngine associated with \a object, if any.  This is equivalent to
    QQmlEngine::contextForObject(object)->engine(), but more efficient.

    \note Add \c{#include <QtQml>} to use this function.

    \sa {QQmlEngine::contextForObject()}{contextForObject()}, qmlContext()
*/

/*!
    \fn QQmlContext *qmlContext(const QObject *object)
    \relates QQmlEngine

    Returns the QQmlContext associated with \a object, if any.  This is equivalent to
    QQmlEngine::contextForObject(object).

    \note Add \c{#include <QtQml>} to use this function.

    \sa {QQmlEngine::contextForObject()}{contextForObject()}, qmlEngine()
*/

QT_END_NAMESPACE

#include "moc_qqmlengine.cpp"
