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

#include "private/qdeclarativeengine_p.h"
#include "qdeclarativeengine.h"

#include "private/qdeclarativecontext_p.h"
#include "private/qdeclarativecompiler_p.h"
#include "qdeclarative.h"
#include "qdeclarativecontext.h"
#include "qdeclarativeexpression.h"
#include "qdeclarativecomponent.h"
#include "private/qdeclarativebinding_p_p.h"
#include "private/qdeclarativevme_p.h"
#include "private/qdeclarativeenginedebug_p.h"
#include "private/qdeclarativestringconverters_p.h"
#include "private/qdeclarativexmlhttprequest_p.h"
#include "private/qdeclarativesqldatabase_p.h"
#include "qdeclarativescriptstring.h"
#include "private/qdeclarativeglobal_p.h"
#include "private/qdeclarativeworkerscript_p.h"
#include "private/qdeclarativecomponent_p.h"
#include "qdeclarativenetworkaccessmanagerfactory.h"
#include "qdeclarativeimageprovider.h"
#include "private/qdeclarativedirparser_p.h"
#include "qdeclarativeextensioninterface.h"
#include "private/qdeclarativelist_p.h"
#include "private/qdeclarativetypenamecache_p.h"
#include "private/qdeclarativenotifier_p.h"
#include "private/qdeclarativedebugtrace_p.h"
#include "private/qdeclarativeapplication_p.h"
#include "private/qv8debugservice_p.h"

#include <QtCore/qmetaobject.h>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QDesktopServices>
#include <QTimer>
#include <QList>
#include <QPair>
#include <QDebug>
#include <QMetaObject>
#include <QStack>
#include <QMap>
#include <QPluginLoader>
#include <QtGui/qfontdatabase.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qthreadstorage.h>
#include <QtCore/qthread.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qmutex.h>
#include <QtGui/qcolor.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qsound.h>
#include <QtCore/qcryptographichash.h>

#include <private/qobject_p.h>

#include <private/qdeclarativeutilmodule_p.h>
#include <private/qsgitemsmodule_p.h>
#include <private/qsgparticlesmodule_p.h>
#include <qsgtexture.h>

#ifdef Q_OS_WIN // for %APPDATA%
#include <qt_windows.h>
#include <qlibrary.h>
#include <windows.h>

#define CSIDL_APPDATA		0x001a	// <username>\Application Data
#endif

Q_DECLARE_METATYPE(QDeclarativeProperty)

QT_BEGIN_NAMESPACE

void qmlRegisterBaseTypes(const char *uri, int versionMajor, int versionMinor)
{
    QDeclarativeEnginePrivate::registerBaseTypes(uri, versionMajor, versionMinor);
    QDeclarativeValueTypeFactory::registerBaseTypes(uri, versionMajor, versionMinor);
    QDeclarativeUtilModule::registerBaseTypes(uri, versionMajor, versionMinor);
}

/*!
  \qmlclass QtObject QObject
  \ingroup qml-utility-elements
  \since 4.7
  \brief The QtObject element is the most basic element in QML.

  The QtObject element is a non-visual element which contains only the
  objectName property.

  It can be useful to create a QtObject if you need an extremely
  lightweight element to enclose a set of custom properties:

  \snippet doc/src/snippets/declarative/qtobject.qml 0

  It can also be useful for C++ integration, as it is just a plain
  QObject. See the QObject documentation for further details.
*/
/*!
  \qmlproperty string QML:QtObject::objectName
  This property holds the QObject::objectName for this specific object instance.

  This allows a C++ application to locate an item within a QML component
  using the QObject::findChild() method. For example, the following C++
  application locates the child \l Rectangle item and dynamically changes its
  \c color value:

    \qml
    // MyRect.qml

    import QtQuick 1.0

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

    QDeclarativeView view;
    view.setSource(QUrl::fromLocalFile("MyRect.qml"));
    view.show();

    QDeclarativeItem *item = view.rootObject()->findChild<QDeclarativeItem*>("myRect");
    if (item)
        item->setProperty("color", QColor(Qt::yellow));
    \endcode
*/

static bool qt_QmlQtModule_registered = false;
bool QDeclarativeEnginePrivate::qml_debugging_enabled = false;

void QDeclarativeEnginePrivate::registerBaseTypes(const char *uri, int versionMajor, int versionMinor)
{
    qmlRegisterType<QDeclarativeComponent>(uri,versionMajor,versionMinor,"Component");
    qmlRegisterType<QObject>(uri,versionMajor,versionMinor,"QtObject");
    qmlRegisterType<QDeclarativeWorkerScript>(uri,versionMajor,versionMinor,"WorkerScript");
}

void QDeclarativeEnginePrivate::defineModule()
{
    registerBaseTypes("QtQuick", 2, 0);
    qmlRegisterType<QDeclarativeBinding>();
}

/*!
\qmlclass QML:Qt QDeclarativeEnginePrivate
  \ingroup qml-utility-elements
\brief The QML global Qt object provides useful enums and functions from Qt.

\keyword QmlGlobalQtObject

\brief The \c Qt object provides useful enums and functions from Qt, for use in all QML files.

The \c Qt object is a global object with utility functions, properties and enums.

It is not instantiable; to use it, call the members of the global \c Qt object directly.
For example:

\qml
import QtQuick 1.0

Text {
    color: Qt.rgba(1, 0, 0, 1)
    text: Qt.md5("hello, world")
}
\endqml


\section1 Enums

The Qt object contains the enums available in the \l {Qt Namespace}. For example, you can access
the \l Qt::LeftButton and \l Qt::RightButton enum values as \c Qt.LeftButton and \c Qt.RightButton.


\section1 Types
The Qt object also contains helper functions for creating objects of specific
data types. This is primarily useful when setting the properties of an item
when the property has one of the following types:

\list
\o \c color - use \l{QML:Qt::rgba()}{Qt.rgba()}, \l{QML:Qt::hsla()}{Qt.hsla()}, \l{QML:Qt::darker()}{Qt.darker()}, \l{QML:Qt::lighter()}{Qt.lighter()} or \l{QML:Qt::tint()}{Qt.tint()}
\o \c rect - use \l{QML:Qt::rect()}{Qt.rect()}
\o \c point - use \l{QML:Qt::point()}{Qt.point()}
\o \c size - use \l{QML:Qt::size()}{Qt.size()}
\o \c vector3d - use \l{QML:Qt::vector3d()}{Qt.vector3d()}
\endlist

There are also string based constructors for these types. See \l{qdeclarativebasictypes.html}{QML Basic Types} for more information.

\section1 Date/Time Formatters

The Qt object contains several functions for formatting QDateTime, QDate and QTime values.

\list
    \o \l{QML:Qt::formatDateTime}{string Qt.formatDateTime(datetime date, variant format)}
    \o \l{QML:Qt::formatDate}{string Qt.formatDate(datetime date, variant format)}
    \o \l{QML:Qt::formatTime}{string Qt.formatTime(datetime date, variant format)}
\endlist

The format specification is described at \l{QML:Qt::formatDateTime}{Qt.formatDateTime}.


\section1 Dynamic Object Creation
The following functions on the global object allow you to dynamically create QML
items from files or strings. See \l{Dynamic Object Management in QML} for an overview
of their use.

\list
    \o \l{QML:Qt::createComponent()}{object Qt.createComponent(url)}
    \o \l{QML:Qt::createQmlObject()}{object Qt.createQmlObject(string qml, object parent, string filepath)}
\endlist
*/


/*!
    \qmlproperty object QML:Qt::application
    \since QtQuick 1.1

    The \c application object provides access to global application state
    properties shared by many QML components.

    Its properties are:

    \table
    \row
    \o \c application.active
    \o
    This read-only property indicates whether the application is the top-most and focused
    application, and the user is able to interact with the application. The property
    is false when the application is in the background, the device keylock or screen
    saver is active, the screen backlight is turned off, or the global system dialog
    is being displayed on top of the application. It can be used for stopping and
    pausing animations, timers and active processing of data in order to save device
    battery power and free device memory and processor load when the application is not
    active.

    \row
    \o \c application.layoutDirection
    \o
    This read-only property can be used to query the default layout direction of the
    application. On system start-up, the default layout direction depends on the
    application's language. The property has a value of \c Qt.RightToLeft in locales
    where text and graphic elements are read from right to left, and \c Qt.LeftToRight
    where the reading direction flows from left to right. You can bind to this
    property to customize your application layouts to support both layout directions.

    Possible values are:

    \list
    \o Qt.LeftToRight - Text and graphics elements should be positioned
                        from left to right.
    \o Qt.RightToLeft - Text and graphics elements should be positioned
                        from right to left.
    \endlist
    \endtable

    The following example uses the \c application object to indicate
    whether the application is currently active:

    \snippet doc/src/snippets/declarative/application.qml document

*/


/*!
\qmlmethod object Qt::include(string url, jsobject callback)

Includes another JavaScript file. This method can only be used from within JavaScript files,
and not regular QML files.

This imports all functions from \a url into the current script's namespace.

Qt.include() returns an object that describes the status of the operation.  The object has
a single property, \c {status}, that is set to one of the following values:

\table
\header \o Symbol \o Value \o Description
\row \o result.OK \o 0 \o The include completed successfully.
\row \o result.LOADING \o 1 \o Data is being loaded from the network.
\row \o result.NETWORK_ERROR \o 2 \o A network error occurred while fetching the url.
\row \o result.EXCEPTION \o 3 \o A JavaScript exception occurred while executing the included code.
An additional \c exception property will be set in this case.
\endtable

The \c status property will be updated as the operation progresses.

If provided, \a callback is invoked when the operation completes.  The callback is passed
the same object as is returned from the Qt.include() call.
*/
// Qt.include() is implemented in qv8include.cpp


QDeclarativeEnginePrivate::QDeclarativeEnginePrivate(QDeclarativeEngine *e)
: captureProperties(false), rootContext(0), isDebugging(false),
  outputWarningsToStdErr(true), sharedContext(0), sharedScope(0),
  cleanup(0), erroredBindings(0), inProgressCreations(0), 
  workerScriptEngine(0), componentAttached(0), inBeginCreate(false), 
  networkAccessManager(0), networkAccessManagerFactory(0),
  scarceResourcesRefCount(0), typeLoader(e), importDatabase(e), uniqueId(1),
  sgContext(0)
{
    if (!qt_QmlQtModule_registered) {
        qt_QmlQtModule_registered = true;
        QDeclarativeUtilModule::defineModule();
        QDeclarativeEnginePrivate::defineModule();
        QSGItemsModule::defineModule();
        QSGParticlesModule::defineModule();
        QDeclarativeValueTypeFactory::registerValueTypes();
    }
}

QDeclarativeEnginePrivate::~QDeclarativeEnginePrivate()
{
    Q_ASSERT(inProgressCreations == 0);
    Q_ASSERT(bindValues.isEmpty());
    Q_ASSERT(parserStatus.isEmpty());

    while (cleanup) {
        QDeclarativeCleanup *c = cleanup;
        cleanup = c->next;
        if (cleanup) cleanup->prev = &cleanup;
        c->next = 0;
        c->prev = 0;
        c->clear();
    }

    delete rootContext;
    rootContext = 0;

    for(QHash<int, QDeclarativeCompiledData*>::ConstIterator iter = m_compositeTypes.constBegin(); iter != m_compositeTypes.constEnd(); ++iter)
        (*iter)->release();
    for(QHash<const QMetaObject *, QDeclarativePropertyCache *>::Iterator iter = propertyCache.begin(); iter != propertyCache.end(); ++iter)
        (*iter)->release();
    for(QHash<QPair<QDeclarativeType *, int>, QDeclarativePropertyCache *>::Iterator iter = typePropertyCache.begin(); iter != typePropertyCache.end(); ++iter)
        (*iter)->release();
    for(QHash<QDeclarativeMetaType::ModuleApi, QDeclarativeMetaType::ModuleApiInstance *>::Iterator iter = moduleApiInstances.begin(); iter != moduleApiInstances.end(); ++iter) {
        delete (*iter)->qobjectApi;
        delete *iter;
    }
}

void QDeclarativeEnginePrivate::clear(SimpleList<QDeclarativeAbstractBinding> &bvs)
{
    bvs.clear();
}

void QDeclarativeEnginePrivate::clear(SimpleList<QDeclarativeParserStatus> &pss)
{
    for (int ii = 0; ii < pss.count; ++ii) {
        QDeclarativeParserStatus *ps = pss.at(ii);
        if(ps)
            ps->d = 0;
    }
    pss.clear();
}

void QDeclarativePrivate::qdeclarativeelement_destructor(QObject *o)
{
    QObjectPrivate *p = QObjectPrivate::get(o);
    if (p->declarativeData) {
        QDeclarativeData *d = static_cast<QDeclarativeData*>(p->declarativeData);
        if (d->ownContext && d->context) {
            d->context->destroy();
            d->context = 0;
        }
    }
}

void QDeclarativeData::destroyed(QAbstractDeclarativeData *d, QObject *o)
{
    static_cast<QDeclarativeData *>(d)->destroyed(o);
}

void QDeclarativeData::parentChanged(QAbstractDeclarativeData *d, QObject *o, QObject *p)
{
    static_cast<QDeclarativeData *>(d)->parentChanged(o, p);
}

void QDeclarativeData::objectNameChanged(QAbstractDeclarativeData *d, QObject *o)
{
    static_cast<QDeclarativeData *>(d)->objectNameChanged(o);
}

void QDeclarativeEnginePrivate::init()
{
    Q_Q(QDeclarativeEngine);
    qRegisterMetaType<QVariant>("QVariant");
    qRegisterMetaType<QDeclarativeScriptString>("QDeclarativeScriptString");
    qRegisterMetaType<QJSValue>("QJSValue");
    qRegisterMetaType<QDeclarativeComponent::Status>("QDeclarativeComponent::Status");
    qRegisterMetaType<QList<QObject*> >("QList<QObject*>");
    qRegisterMetaType<QList<int> >("QList<int>");
    qRegisterMetaType<QDeclarativeV8Handle>("QDeclarativeV8Handle");

    QDeclarativeData::init();

    v8engine()->setEngine(q);

    rootContext = new QDeclarativeContext(q,true);

    if (QCoreApplication::instance()->thread() == q->thread() &&
        QDeclarativeEngineDebugServer::isDebuggingEnabled()) {
        isDebugging = true;
        QDeclarativeEngineDebugServer::instance()->addEngine(q);
        QV8DebugService::instance()->addEngine(q);
    }
}

QDeclarativeWorkerScriptEngine *QDeclarativeEnginePrivate::getWorkerScriptEngine()
{
    Q_Q(QDeclarativeEngine);
    if (!workerScriptEngine)
        workerScriptEngine = new QDeclarativeWorkerScriptEngine(q);
    return workerScriptEngine;
}

/*!
  \class QDeclarativeEngine
  \since 4.7
  \brief The QDeclarativeEngine class provides an environment for instantiating QML components.
  \mainclass

  Each QML component is instantiated in a QDeclarativeContext.
  QDeclarativeContext's are essential for passing data to QML
  components.  In QML, contexts are arranged hierarchically and this
  hierarchy is managed by the QDeclarativeEngine.

  Prior to creating any QML components, an application must have
  created a QDeclarativeEngine to gain access to a QML context.  The
  following example shows how to create a simple Text item.

  \code
  QDeclarativeEngine engine;
  QDeclarativeComponent component(&engine);
  component.setData("import QtQuick 1.0\nText { text: \"Hello world!\" }", QUrl());
  QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(component.create());

  //add item to view, etc
  ...
  \endcode

  In this case, the Text item will be created in the engine's
  \l {QDeclarativeEngine::rootContext()}{root context}.

  \sa QDeclarativeComponent QDeclarativeContext
*/

/*!
  Create a new QDeclarativeEngine with the given \a parent.
*/
QDeclarativeEngine::QDeclarativeEngine(QObject *parent)
: QJSEngine(*new QDeclarativeEnginePrivate(this), parent)
{
    Q_D(QDeclarativeEngine);
    d->init();
}

/*!
  Destroys the QDeclarativeEngine.

  Any QDeclarativeContext's created on this engine will be
  invalidated, but not destroyed (unless they are parented to the
  QDeclarativeEngine object).
*/
QDeclarativeEngine::~QDeclarativeEngine()
{
    Q_D(QDeclarativeEngine);
    if (d->isDebugging)
        QDeclarativeEngineDebugServer::instance()->remEngine(this);

    // if we are the parent of any of the qobject module api instances,
    // we need to remove them from our internal list, in order to prevent
    // a segfault in engine private dtor.
    QList<QDeclarativeMetaType::ModuleApi> keys = d->moduleApiInstances.keys();
    QObject *currQObjectApi = 0;
    QDeclarativeMetaType::ModuleApiInstance *currInstance = 0;
    foreach (const QDeclarativeMetaType::ModuleApi &key, keys) {
        currInstance = d->moduleApiInstances.value(key);
        currQObjectApi = currInstance->qobjectApi;
        if (this->children().contains(currQObjectApi)) {
            delete currQObjectApi;
            delete currInstance;
            d->moduleApiInstances.remove(key);
        }
    }
}

/*! \fn void QDeclarativeEngine::quit()
    This signal is emitted when the QML loaded by the engine would like to quit.
 */

/*! \fn void QDeclarativeEngine::warnings(const QList<QDeclarativeError> &warnings)
    This signal is emitted when \a warnings messages are generated by QML.
 */

/*!
  Clears the engine's internal component cache.

  Normally the QDeclarativeEngine caches components loaded from qml
  files.  This method clears this cache and forces the component to be
  reloaded.
 */
void QDeclarativeEngine::clearComponentCache()
{
    Q_D(QDeclarativeEngine);
    d->typeLoader.clearCache();
}

/*!
  Returns the engine's root context.

  The root context is automatically created by the QDeclarativeEngine.
  Data that should be available to all QML component instances
  instantiated by the engine should be put in the root context.

  Additional data that should only be available to a subset of
  component instances should be added to sub-contexts parented to the
  root context.
*/
QDeclarativeContext *QDeclarativeEngine::rootContext() const
{
    Q_D(const QDeclarativeEngine);
    return d->rootContext;
}

/*!
  Sets the \a factory to use for creating QNetworkAccessManager(s).

  QNetworkAccessManager is used for all network access by QML.  By
  implementing a factory it is possible to create custom
  QNetworkAccessManager with specialized caching, proxy and cookie
  support.

  The factory must be set before executing the engine.
*/
void QDeclarativeEngine::setNetworkAccessManagerFactory(QDeclarativeNetworkAccessManagerFactory *factory)
{
    Q_D(QDeclarativeEngine);
    QMutexLocker locker(&d->mutex);
    d->networkAccessManagerFactory = factory;
}

/*!
  Returns the current QDeclarativeNetworkAccessManagerFactory.

  \sa setNetworkAccessManagerFactory()
*/
QDeclarativeNetworkAccessManagerFactory *QDeclarativeEngine::networkAccessManagerFactory() const
{
    Q_D(const QDeclarativeEngine);
    return d->networkAccessManagerFactory;
}

QNetworkAccessManager *QDeclarativeEnginePrivate::createNetworkAccessManager(QObject *parent) const
{
    QMutexLocker locker(&mutex);
    QNetworkAccessManager *nam;
    if (networkAccessManagerFactory) {
        nam = networkAccessManagerFactory->create(parent);
    } else {
        nam = new QNetworkAccessManager(parent);
    }

    return nam;
}

QNetworkAccessManager *QDeclarativeEnginePrivate::getNetworkAccessManager() const
{
    Q_Q(const QDeclarativeEngine);
    if (!networkAccessManager)
        networkAccessManager = createNetworkAccessManager(const_cast<QDeclarativeEngine*>(q));
    return networkAccessManager;
}

/*!
  Returns a common QNetworkAccessManager which can be used by any QML
  element instantiated by this engine.

  If a QDeclarativeNetworkAccessManagerFactory has been set and a
  QNetworkAccessManager has not yet been created, the
  QDeclarativeNetworkAccessManagerFactory will be used to create the
  QNetworkAccessManager; otherwise the returned QNetworkAccessManager
  will have no proxy or cache set.

  \sa setNetworkAccessManagerFactory()
*/
QNetworkAccessManager *QDeclarativeEngine::networkAccessManager() const
{
    Q_D(const QDeclarativeEngine);
    return d->getNetworkAccessManager();
}

/*!

  Sets the \a provider to use for images requested via the \e
  image: url scheme, with host \a providerId. The QDeclarativeEngine
  takes ownership of \a provider.

  Image providers enable support for pixmap and threaded image
  requests. See the QDeclarativeImageProvider documentation for details on
  implementing and using image providers.

  All required image providers should be added to the engine before any
  QML sources files are loaded.

  \sa removeImageProvider()
*/
void QDeclarativeEngine::addImageProvider(const QString &providerId, QDeclarativeImageProvider *provider)
{
    Q_D(QDeclarativeEngine);
    QMutexLocker locker(&d->mutex);
    d->imageProviders.insert(providerId.toLower(), QSharedPointer<QDeclarativeImageProvider>(provider));
}

/*!
  Returns the QDeclarativeImageProvider set for \a providerId.
*/
QDeclarativeImageProvider *QDeclarativeEngine::imageProvider(const QString &providerId) const
{
    Q_D(const QDeclarativeEngine);
    QMutexLocker locker(&d->mutex);
    return d->imageProviders.value(providerId).data();
}

/*!
  Removes the QDeclarativeImageProvider for \a providerId.

  Returns the provider if it was found; otherwise returns 0.

  \sa addImageProvider()
*/
void QDeclarativeEngine::removeImageProvider(const QString &providerId)
{
    Q_D(QDeclarativeEngine);
    QMutexLocker locker(&d->mutex);
    d->imageProviders.take(providerId);
}

QDeclarativeImageProvider::ImageType QDeclarativeEnginePrivate::getImageProviderType(const QUrl &url)
{
    QMutexLocker locker(&mutex);
    QSharedPointer<QDeclarativeImageProvider> provider = imageProviders.value(url.host());
    locker.unlock();
    if (provider)
        return provider->imageType();
    return QDeclarativeImageProvider::Invalid;
}

QSGTexture *QDeclarativeEnginePrivate::getTextureFromProvider(const QUrl &url, QSize *size, const QSize& req_size)
{
    QMutexLocker locker(&mutex);
    QSharedPointer<QDeclarativeImageProvider> provider = imageProviders.value(url.host());
    locker.unlock();
    if (provider) {
        QString imageId = url.toString(QUrl::RemoveScheme | QUrl::RemoveAuthority).mid(1);
        return provider->requestTexture(imageId, size, req_size);
    }
    return 0;
}

QImage QDeclarativeEnginePrivate::getImageFromProvider(const QUrl &url, QSize *size, const QSize& req_size)
{
    QMutexLocker locker(&mutex);
    QImage image;
    QSharedPointer<QDeclarativeImageProvider> provider = imageProviders.value(url.host());
    locker.unlock();
    if (provider) {
        QString imageId = url.toString(QUrl::RemoveScheme | QUrl::RemoveAuthority).mid(1);
        image = provider->requestImage(imageId, size, req_size);
    }
    return image;
}

QPixmap QDeclarativeEnginePrivate::getPixmapFromProvider(const QUrl &url, QSize *size, const QSize& req_size)
{
    QMutexLocker locker(&mutex);
    QPixmap pixmap;
    QSharedPointer<QDeclarativeImageProvider> provider = imageProviders.value(url.host());
    locker.unlock();
    if (provider) {
        QString imageId = url.toString(QUrl::RemoveScheme | QUrl::RemoveAuthority).mid(1);
        pixmap = provider->requestPixmap(imageId, size, req_size);
    }
    return pixmap;
}

/*!
  Return the base URL for this engine.  The base URL is only used to
  resolve components when a relative URL is passed to the
  QDeclarativeComponent constructor.

  If a base URL has not been explicitly set, this method returns the
  application's current working directory.

  \sa setBaseUrl()
*/
QUrl QDeclarativeEngine::baseUrl() const
{
    Q_D(const QDeclarativeEngine);
    if (d->baseUrl.isEmpty()) {
        return QUrl::fromLocalFile(QDir::currentPath() + QDir::separator());
    } else {
        return d->baseUrl;
    }
}

/*!
  Set the  base URL for this engine to \a url.

  \sa baseUrl()
*/
void QDeclarativeEngine::setBaseUrl(const QUrl &url)
{
    Q_D(QDeclarativeEngine);
    d->baseUrl = url;
}

/*!
  Returns true if warning messages will be output to stderr in addition
  to being emitted by the warnings() signal, otherwise false.

  The default value is true.
*/
bool QDeclarativeEngine::outputWarningsToStandardError() const
{
    Q_D(const QDeclarativeEngine);
    return d->outputWarningsToStdErr;
}

/*!
  Set whether warning messages will be output to stderr to \a enabled.

  If \a enabled is true, any warning messages generated by QML will be
  output to stderr and emitted by the warnings() signal.  If \a enabled
  is false, on the warnings() signal will be emitted.  This allows
  applications to handle warning output themselves.

  The default value is true.
*/
void QDeclarativeEngine::setOutputWarningsToStandardError(bool enabled)
{
    Q_D(QDeclarativeEngine);
    d->outputWarningsToStdErr = enabled;
}

/*!
  Attempt to free unused memory.
*/
void QDeclarativeEngine::collectGarbage()
{
    QV8Engine::gc();
}

/*!
  Returns the QDeclarativeContext for the \a object, or 0 if no
  context has been set.

  When the QDeclarativeEngine instantiates a QObject, the context is
  set automatically.
  */
QDeclarativeContext *QDeclarativeEngine::contextForObject(const QObject *object)
{
    if(!object)
        return 0;

    QObjectPrivate *priv = QObjectPrivate::get(const_cast<QObject *>(object));

    QDeclarativeData *data =
        static_cast<QDeclarativeData *>(priv->declarativeData);

    if (!data)
        return 0;
    else if (data->outerContext)
        return data->outerContext->asQDeclarativeContext();
    else
        return 0;
}

/*!
  Sets the QDeclarativeContext for the \a object to \a context.
  If the \a object already has a context, a warning is
  output, but the context is not changed.

  When the QDeclarativeEngine instantiates a QObject, the context is
  set automatically.
 */
void QDeclarativeEngine::setContextForObject(QObject *object, QDeclarativeContext *context)
{
    if (!object || !context)
        return;

    QDeclarativeData *data = QDeclarativeData::get(object, true);
    if (data->context) {
        qWarning("QDeclarativeEngine::setContextForObject(): Object already has a QDeclarativeContext");
        return;
    }

    QDeclarativeContextData *contextData = QDeclarativeContextData::get(context);
    contextData->addObject(object);
}

/*!
  \enum QDeclarativeEngine::ObjectOwnership

  Ownership controls whether or not QML automatically destroys the
  QObject when the object is garbage collected by the JavaScript
  engine.  The two ownership options are:

  \value CppOwnership The object is owned by C++ code, and will
  never be deleted by QML.  The JavaScript destroy() method cannot be
  used on objects with CppOwnership.  This option is similar to
  QScriptEngine::QtOwnership.

  \value JavaScriptOwnership The object is owned by JavaScript.
  When the object is returned to QML as the return value of a method
  call or property access, QML will delete the object if there are no
  remaining JavaScript references to it and it has no
  QObject::parent().  This option is similar to
  QScriptEngine::ScriptOwnership.

  Generally an application doesn't need to set an object's ownership
  explicitly.  QML uses a heuristic to set the default object
  ownership.  By default, an object that is created by QML has
  JavaScriptOwnership.  The exception to this are the root objects
  created by calling QDeclarativeCompnent::create() or
  QDeclarativeComponent::beginCreate() which have CppOwnership by
  default.  The ownership of these root-level objects is considered to
  have been transferred to the C++ caller.

  Objects not-created by QML have CppOwnership by default.  The
  exception to this is objects returned from a C++ method call.  The
  ownership of these objects is passed to JavaScript.

  Calling setObjectOwnership() overrides the default ownership
  heuristic used by QML.
*/

/*!
  Sets the \a ownership of \a object.
*/
void QDeclarativeEngine::setObjectOwnership(QObject *object, ObjectOwnership ownership)
{
    if (!object)
        return;

    QDeclarativeData *ddata = QDeclarativeData::get(object, true);
    if (!ddata)
        return;

    ddata->indestructible = (ownership == CppOwnership)?true:false;
    ddata->explicitIndestructibleSet = true;
}

/*!
  Returns the ownership of \a object.
*/
QDeclarativeEngine::ObjectOwnership QDeclarativeEngine::objectOwnership(QObject *object)
{
    if (!object)
        return CppOwnership;

    QDeclarativeData *ddata = QDeclarativeData::get(object, false);
    if (!ddata)
        return CppOwnership;
    else
        return ddata->indestructible?CppOwnership:JavaScriptOwnership;
}

Q_AUTOTEST_EXPORT void qmlExecuteDeferred(QObject *object)
{
    QDeclarativeData *data = QDeclarativeData::get(object);

    if (data && data->deferredComponent) {
        if (QDeclarativeDebugService::isDebuggingEnabled()) {
            QDeclarativeDebugTrace::startRange(QDeclarativeDebugTrace::Creating);
            QDeclarativeType *type = QDeclarativeMetaType::qmlType(object->metaObject());
            QString typeName = type ? QLatin1String(type->qmlTypeName()) : QString::fromLatin1(object->metaObject()->className());
            QDeclarativeDebugTrace::rangeData(QDeclarativeDebugTrace::Creating, typeName);
            if (data->outerContext)
                QDeclarativeDebugTrace::rangeLocation(QDeclarativeDebugTrace::Creating, data->outerContext->url, data->lineNumber);
        }
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(data->context->engine);

        QDeclarativeComponentPrivate::ConstructionState state;
        QDeclarativeComponentPrivate::beginDeferred(ep, object, &state);

        data->deferredComponent->release();
        data->deferredComponent = 0;

        QDeclarativeComponentPrivate::complete(ep, &state);
        QDeclarativeDebugTrace::endRange(QDeclarativeDebugTrace::Creating);
    }
}

QDeclarativeContext *qmlContext(const QObject *obj)
{
    return QDeclarativeEngine::contextForObject(obj);
}

QDeclarativeEngine *qmlEngine(const QObject *obj)
{
    QDeclarativeContext *context = QDeclarativeEngine::contextForObject(obj);
    return context?context->engine():0;
}

QObject *qmlAttachedPropertiesObjectById(int id, const QObject *object, bool create)
{
    QDeclarativeData *data = QDeclarativeData::get(object);
    if (!data)
        return 0; // Attached properties are only on objects created by QML

    QObject *rv = data->hasExtendedData()?data->attachedProperties()->value(id):0;
    if (rv || !create)
        return rv;

    QDeclarativeAttachedPropertiesFunc pf = QDeclarativeMetaType::attachedPropertiesFuncById(id);
    if (!pf)
        return 0;

    rv = pf(const_cast<QObject *>(object));

    if (rv)
        data->attachedProperties()->insert(id, rv);

    return rv;
}

QObject *qmlAttachedPropertiesObject(int *idCache, const QObject *object,
                                     const QMetaObject *attachedMetaObject, bool create)
{
    if (*idCache == -1)
        *idCache = QDeclarativeMetaType::attachedPropertiesFuncId(attachedMetaObject);

    if (*idCache == -1 || !object)
        return 0;

    return qmlAttachedPropertiesObjectById(*idCache, object, create);
}

QDeclarativeDebuggingEnabler::QDeclarativeDebuggingEnabler()
{
#ifndef QDECLARATIVE_NO_DEBUG_PROTOCOL
    if (!QDeclarativeEnginePrivate::qml_debugging_enabled) {
        qWarning("Qml debugging is enabled. Only use this in a safe environment!");
    }
    QDeclarativeEnginePrivate::qml_debugging_enabled = true;
#endif
}


class QDeclarativeDataExtended {
public:
    QDeclarativeDataExtended();
    ~QDeclarativeDataExtended();

    QHash<int, QObject *> attachedProperties;
    QDeclarativeNotifier objectNameNotifier;
};

QDeclarativeDataExtended::QDeclarativeDataExtended()
{
}

QDeclarativeDataExtended::~QDeclarativeDataExtended()
{
}

QDeclarativeNotifier *QDeclarativeData::objectNameNotifier() const
{
    if (!extendedData) extendedData = new QDeclarativeDataExtended;
    return &extendedData->objectNameNotifier;
}

QHash<int, QObject *> *QDeclarativeData::attachedProperties() const
{
    if (!extendedData) extendedData = new QDeclarativeDataExtended;
    return &extendedData->attachedProperties;
}

void QDeclarativeData::destroyed(QObject *object)
{
    if (deferredComponent)
        deferredComponent->release();

    if (nextContextObject)
        nextContextObject->prevContextObject = prevContextObject;
    if (prevContextObject)
        *prevContextObject = nextContextObject;

    QDeclarativeAbstractBinding *binding = bindings;
    while (binding) {
        QDeclarativeAbstractBinding *next = binding->m_nextBinding;
        binding->m_prevBinding = 0;
        binding->m_nextBinding = 0;
        binding->destroy();
        binding = next;
    }

    if (bindingBits)
        free(bindingBits);

    if (propertyCache)
        propertyCache->release();

    if (ownContext && context)
        context->destroy();

    while (guards) {
        QDeclarativeGuard<QObject> *guard = static_cast<QDeclarativeGuard<QObject> *>(guards);
        *guard = (QObject *)0;
        guard->objectDestroyed(object);
    }

    if (extendedData)
        delete extendedData;

    v8object.Clear(); // The WeakReference handler will clean the actual handle

    if (ownMemory)
        delete this;
}

void QDeclarativeData::parentChanged(QObject *object, QObject *parent)
{
    Q_UNUSED(object);
    Q_UNUSED(parent);
}

void QDeclarativeData::objectNameChanged(QObject *)
{
    if (extendedData) objectNameNotifier()->notify();
}

bool QDeclarativeData::hasBindingBit(int bit) const
{
    if (bindingBitsSize > bit)
        return bindingBits[bit / 32] & (1 << (bit % 32));
    else
        return false;
}

void QDeclarativeData::clearBindingBit(int bit)
{
    if (bindingBitsSize > bit)
        bindingBits[bit / 32] &= ~(1 << (bit % 32));
}

void QDeclarativeData::setBindingBit(QObject *obj, int bit)
{
    if (bindingBitsSize <= bit) {
        int props = obj->metaObject()->propertyCount();
        Q_ASSERT(bit < props);

        int arraySize = (props + 31) / 32;
        int oldArraySize = bindingBitsSize / 32;

        bindingBits = (quint32 *)realloc(bindingBits,
                                         arraySize * sizeof(quint32));

        memset(bindingBits + oldArraySize,
               0x00,
               sizeof(quint32) * (arraySize - oldArraySize));

        bindingBitsSize = arraySize * 32;
    }

    bindingBits[bit / 32] |= (1 << (bit % 32));
}

QString QDeclarativeEnginePrivate::urlToLocalFileOrQrc(const QUrl& url)
{
    if (url.scheme().compare(QLatin1String("qrc"), Qt::CaseInsensitive) == 0) {
        if (url.authority().isEmpty())
            return QLatin1Char(':') + url.path();
        return QString();
    }
    return url.toLocalFile();
}


static QString toLocalFile(const QString &url)
{
    if (!url.startsWith(QLatin1String("file://"), Qt::CaseInsensitive))
        return QString();

    QString file = url.mid(7);

    //XXX TODO: handle windows hostnames: "//servername/path/to/file.txt"

    // magic for drives on windows
    if (file.length() > 2 && file.at(0) == QLatin1Char('/') && file.at(2) == QLatin1Char(':'))
        file.remove(0, 1);

    return file;
}

QString QDeclarativeEnginePrivate::urlToLocalFileOrQrc(const QString& url)
{
    if (url.startsWith(QLatin1String("qrc:"), Qt::CaseInsensitive)) {
        if (url.length() > 4)
            return QLatin1Char(':') + url.mid(4);
        return QString();
    }

    return toLocalFile(url);
}

void QDeclarativeEnginePrivate::sendQuit()
{
    Q_Q(QDeclarativeEngine);
    emit q->quit();
    if (q->receivers(SIGNAL(quit())) == 0) {
        qWarning("Signal QDeclarativeEngine::quit() emitted, but no receivers connected to handle it.");
    }
}

static void dumpwarning(const QDeclarativeError &error)
{
    qWarning().nospace() << qPrintable(error.toString());
}

static void dumpwarning(const QList<QDeclarativeError> &errors)
{
    for (int ii = 0; ii < errors.count(); ++ii)
        dumpwarning(errors.at(ii));
}

void QDeclarativeEnginePrivate::warning(const QDeclarativeError &error)
{
    Q_Q(QDeclarativeEngine);
    q->warnings(QList<QDeclarativeError>() << error);
    if (outputWarningsToStdErr)
        dumpwarning(error);
}

void QDeclarativeEnginePrivate::warning(const QList<QDeclarativeError> &errors)
{
    Q_Q(QDeclarativeEngine);
    q->warnings(errors);
    if (outputWarningsToStdErr)
        dumpwarning(errors);
}

void QDeclarativeEnginePrivate::warning(QDeclarativeEngine *engine, const QDeclarativeError &error)
{
    if (engine)
        QDeclarativeEnginePrivate::get(engine)->warning(error);
    else
        dumpwarning(error);
}

void QDeclarativeEnginePrivate::warning(QDeclarativeEngine *engine, const QList<QDeclarativeError> &error)
{
    if (engine)
        QDeclarativeEnginePrivate::get(engine)->warning(error);
    else
        dumpwarning(error);
}

void QDeclarativeEnginePrivate::warning(QDeclarativeEnginePrivate *engine, const QDeclarativeError &error)
{
    if (engine)
        engine->warning(error);
    else
        dumpwarning(error);
}

void QDeclarativeEnginePrivate::warning(QDeclarativeEnginePrivate *engine, const QList<QDeclarativeError> &error)
{
    if (engine)
        engine->warning(error);
    else
        dumpwarning(error);
}

/*
   This function should be called prior to evaluation of any js expression,
   so that scarce resources are not freed prematurely (eg, if there is a
   nested javascript expression).
 */
void QDeclarativeEnginePrivate::referenceScarceResources()
{
    scarceResourcesRefCount += 1;
}

/*
   This function should be called after evaluation of the js expression is
   complete, and so the scarce resources may be freed safely.
 */
void QDeclarativeEnginePrivate::dereferenceScarceResources()
{
    Q_ASSERT(scarceResourcesRefCount > 0);
    scarceResourcesRefCount -= 1;

    // if the refcount is zero, then evaluation of the "top level"
    // expression must have completed.  We can safely release the
    // scarce resources.
    if (scarceResourcesRefCount == 0) {
        // iterate through the list and release them all.
        // note that the actual SRD is owned by the JS engine,
        // so we cannot delete the SRD; but we can free the
        // memory used by the variant in the SRD.
        while (ScarceResourceData *sr = scarceResources.first()) {
            sr->data = QVariant();
            scarceResources.remove(sr);
        }
    }
}

/*!
  Adds \a path as a directory where the engine searches for
  installed modules in a URL-based directory structure.
  The \a path may be a local filesystem directory or a URL.

  The newly added \a path will be first in the importPathList().

  \sa setImportPathList(), {QML Modules}
*/
void QDeclarativeEngine::addImportPath(const QString& path)
{
    Q_D(QDeclarativeEngine);
    d->importDatabase.addImportPath(path);
}

/*!
  Returns the list of directories where the engine searches for
  installed modules in a URL-based directory structure.

  For example, if \c /opt/MyApp/lib/imports is in the path, then QML that
  imports \c com.mycompany.Feature will cause the QDeclarativeEngine to look
  in \c /opt/MyApp/lib/imports/com/mycompany/Feature/ for the components
  provided by that module. A \c qmldir file is required for defining the
  type version mapping and possibly declarative extensions plugins.

  By default, the list contains the directory of the application executable,
  paths specified in the \c QML_IMPORT_PATH environment variable,
  and the builtin \c ImportsPath from QLibraryInfo.

  \sa addImportPath() setImportPathList()
*/
QStringList QDeclarativeEngine::importPathList() const
{
    Q_D(const QDeclarativeEngine);
    return d->importDatabase.importPathList();
}

/*!
  Sets \a paths as the list of directories where the engine searches for
  installed modules in a URL-based directory structure.

  By default, the list contains the directory of the application executable,
  paths specified in the \c QML_IMPORT_PATH environment variable,
  and the builtin \c ImportsPath from QLibraryInfo.

  \sa importPathList() addImportPath()
  */
void QDeclarativeEngine::setImportPathList(const QStringList &paths)
{
    Q_D(QDeclarativeEngine);
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
void QDeclarativeEngine::addPluginPath(const QString& path)
{
    Q_D(QDeclarativeEngine);
    d->importDatabase.addPluginPath(path);
}


/*!
  Returns the list of directories where the engine searches for
  native plugins for imported modules (referenced in the \c qmldir file).

  By default, the list contains only \c .,  i.e. the engine searches
  in the directory of the \c qmldir file itself.

  \sa addPluginPath() setPluginPathList()
*/
QStringList QDeclarativeEngine::pluginPathList() const
{
    Q_D(const QDeclarativeEngine);
    return d->importDatabase.pluginPathList();
}

/*!
  Sets the list of directories where the engine searches for
  native plugins for imported modules (referenced in the \c qmldir file)
  to \a paths.

  By default, the list contains only \c .,  i.e. the engine searches
  in the directory of the \c qmldir file itself.

  \sa pluginPathList() addPluginPath()
  */
void QDeclarativeEngine::setPluginPathList(const QStringList &paths)
{
    Q_D(QDeclarativeEngine);
    d->importDatabase.setPluginPathList(paths);
}

/*!
  Imports the plugin named \a filePath with the \a uri provided.
  Returns true if the plugin was successfully imported; otherwise returns false.

  On failure and if non-null, the \a errors list will have any errors which occurred prepended to it.

  The plugin has to be a Qt plugin which implements the QDeclarativeExtensionPlugin interface.
*/
bool QDeclarativeEngine::importPlugin(const QString &filePath, const QString &uri, QList<QDeclarativeError> *errors)
{
    Q_D(QDeclarativeEngine);
    return d->importDatabase.importPlugin(filePath, uri, errors);
}

/*!
  Imports the plugin named \a filePath with the \a uri provided.
  Returns true if the plugin was successfully imported; otherwise returns false.

  On failure and if non-null, *\a errorString will be set to a message describing the failure.

  The plugin has to be a Qt plugin which implements the QDeclarativeExtensionPlugin interface.
*/
bool QDeclarativeEngine::importPlugin(const QString &filePath, const QString &uri, QString *errorString)
{
    Q_D(QDeclarativeEngine);
    QList<QDeclarativeError> errors;
    bool retn = d->importDatabase.importPlugin(filePath, uri, &errors);
    if (!errors.isEmpty()) {
        QString builtError;
        for (int i = 0; i < errors.size(); ++i) {
            builtError = QString(QLatin1String("%1\n        %2"))
                    .arg(builtError)
                    .arg(errors.at(i).toString());
        }
        *errorString = builtError;
    }
    return retn;
}

/*!
  \property QDeclarativeEngine::offlineStoragePath
  \brief the directory for storing offline user data

  Returns the directory where SQL and other offline
  storage is placed.

  QDeclarativeWebView and the SQL databases created with openDatabase()
  are stored here.

  The default is QML/OfflineStorage in the platform-standard
  user application data directory.

  Note that the path may not currently exist on the filesystem, so
  callers wanting to \e create new files at this location should create
  it first - see QDir::mkpath().
*/
void QDeclarativeEngine::setOfflineStoragePath(const QString& dir)
{
    Q_D(QDeclarativeEngine);
    qt_qmlsqldatabase_setOfflineStoragePath(d->v8engine(), dir);
}

QString QDeclarativeEngine::offlineStoragePath() const
{
    Q_D(const QDeclarativeEngine);
    return qt_qmlsqldatabase_getOfflineStoragePath(d->v8engine());
}

static void voidptr_destructor(void *v)
{
    void **ptr = (void **)v;
    delete ptr;
}

static void *voidptr_constructor(const void *v)
{
    if (!v) {
        return new void*;
    } else {
        return new void*(*(void **)v);
    }
}

QDeclarativePropertyCache *QDeclarativeEnginePrivate::createCache(const QMetaObject *mo)
{
    Q_Q(QDeclarativeEngine);

    if (!mo->superClass()) {
        QDeclarativePropertyCache *rv = new QDeclarativePropertyCache(q, mo);
        propertyCache.insert(mo, rv);
        return rv;
    } else {
        QDeclarativePropertyCache *super = cache(mo->superClass());
        QDeclarativePropertyCache *rv = super->copy(mo->propertyCount() + mo->methodCount() - 
                                                    mo->superClass()->propertyCount() - 
                                                    mo->superClass()->methodCount());
        rv->append(q, mo);
        propertyCache.insert(mo, rv);
        return rv;
    }
}

QDeclarativePropertyCache *QDeclarativeEnginePrivate::createCache(QDeclarativeType *type, int minorVersion,
                                                                  QDeclarativeError &error)
{
    QList<QDeclarativeType *> types;

    int maxMinorVersion = 0;

    const QMetaObject *metaObject = type->metaObject();
    while (metaObject) {
        QDeclarativeType *t = QDeclarativeMetaType::qmlType(metaObject, type->module(),
                                                            type->majorVersion(), minorVersion);
        if (t) {
            maxMinorVersion = qMax(maxMinorVersion, t->minorVersion());
            types << t;
        } else {
            types << 0;
        }

        metaObject = metaObject->superClass();
    }

    if (QDeclarativePropertyCache *c = typePropertyCache.value(qMakePair(type, maxMinorVersion))) {
        c->addref();
        typePropertyCache.insert(qMakePair(type, minorVersion), c);
        return c;
    }

    QDeclarativePropertyCache *raw = cache(type->metaObject());

    bool hasCopied = false;

    for (int ii = 0; ii < types.count(); ++ii) {
        QDeclarativeType *currentType = types.at(ii);
        if (!currentType)
            continue;

        int rev = currentType->metaObjectRevision();
        int moIndex = types.count() - 1 - ii;

        if (raw->allowedRevisionCache[moIndex] != rev) {
            if (!hasCopied) {
                raw = raw->copy();
                hasCopied = true;
            }
            raw->allowedRevisionCache[moIndex] = rev;
        }
    }

    // Test revision compatibility - the basic rule is:
    //    * Anything that is excluded, cannot overload something that is not excluded *

    // Signals override:
    //    * other signals and methods of the same name.
    //    * properties named on<Signal Name>
    //    * automatic <property name>Changed notify signals

    // Methods override:
    //    * other methods of the same name

    // Properties override:
    //    * other elements of the same name

    bool overloadError = false;
    QString overloadName;

#if 0
    for (QDeclarativePropertyCache::StringCache::ConstIterator iter = raw->stringCache.begin();
         !overloadError && iter != raw->stringCache.end();
         ++iter) {

        QDeclarativePropertyCache::Data *d = *iter;
        if (raw->isAllowedInRevision(d))
            continue; // Not excluded - no problems

        // check that a regular "name" overload isn't happening
        QDeclarativePropertyCache::Data *current = d;
        while (!overloadError && current) {
            current = d->overrideData(current);
            if (current && raw->isAllowedInRevision(current))
                overloadError = true;
        }
    }
#endif

    if (overloadError) {
        if (hasCopied) raw->release();

        error.setDescription(QLatin1String("Type ") + QString::fromUtf8(type->qmlTypeName()) + QLatin1String(" ") + QString::number(type->majorVersion()) + QLatin1String(".") + QString::number(minorVersion) + QLatin1String(" contains an illegal property \"") + overloadName + QLatin1String("\".  This is an error in the type's implementation."));
        return 0;
    }

    if (!hasCopied) raw->addref();
    typePropertyCache.insert(qMakePair(type, minorVersion), raw);

    if (minorVersion != maxMinorVersion) {
        raw->addref();
        typePropertyCache.insert(qMakePair(type, maxMinorVersion), raw);
    }

    return raw;
}

void QDeclarativeEnginePrivate::registerCompositeType(QDeclarativeCompiledData *data)
{
    QByteArray name = data->root->className();

    QByteArray ptr = name + '*';
    QByteArray lst = "QDeclarativeListProperty<" + name + '>';

    int ptr_type = QMetaType::registerType(ptr.constData(), voidptr_destructor,
                                           voidptr_constructor);
    int lst_type = QMetaType::registerType(lst.constData(), voidptr_destructor,
                                           voidptr_constructor);

    m_qmlLists.insert(lst_type, ptr_type);
    m_compositeTypes.insert(ptr_type, data);
    data->addref();
}

bool QDeclarativeEnginePrivate::isList(int t) const
{
    return m_qmlLists.contains(t) || QDeclarativeMetaType::isList(t);
}

int QDeclarativeEnginePrivate::listType(int t) const
{
    QHash<int, int>::ConstIterator iter = m_qmlLists.find(t);
    if (iter != m_qmlLists.end())
        return *iter;
    else
        return QDeclarativeMetaType::listType(t);
}

bool QDeclarativeEnginePrivate::isQObject(int t)
{
    return m_compositeTypes.contains(t) || QDeclarativeMetaType::isQObject(t);
}

QObject *QDeclarativeEnginePrivate::toQObject(const QVariant &v, bool *ok) const
{
    int t = v.userType();
    if (t == QMetaType::QObjectStar || m_compositeTypes.contains(t)) {
        if (ok) *ok = true;
        return *(QObject **)(v.constData());
    } else {
        return QDeclarativeMetaType::toQObject(v, ok);
    }
}

QDeclarativeMetaType::TypeCategory QDeclarativeEnginePrivate::typeCategory(int t) const
{
    if (m_compositeTypes.contains(t))
        return QDeclarativeMetaType::Object;
    else if (m_qmlLists.contains(t))
        return QDeclarativeMetaType::List;
    else
        return QDeclarativeMetaType::typeCategory(t);
}

const QMetaObject *QDeclarativeEnginePrivate::rawMetaObjectForType(int t) const
{
    QHash<int, QDeclarativeCompiledData*>::ConstIterator iter = m_compositeTypes.find(t);
    if (iter != m_compositeTypes.end()) {
        return (*iter)->root;
    } else {
        QDeclarativeType *type = QDeclarativeMetaType::qmlType(t);
        return type?type->baseMetaObject():0;
    }
}

const QMetaObject *QDeclarativeEnginePrivate::metaObjectForType(int t) const
{
    QHash<int, QDeclarativeCompiledData*>::ConstIterator iter = m_compositeTypes.find(t);
    if (iter != m_compositeTypes.end()) {
        return (*iter)->root;
    } else {
        QDeclarativeType *type = QDeclarativeMetaType::qmlType(t);
        return type?type->metaObject():0;
    }
}

bool QDeclarative_isFileCaseCorrect(const QString &fileName)
{
#if defined(Q_OS_MAC) || defined(Q_OS_WIN32)
    QFileInfo info(fileName);

    QString absolute = info.absoluteFilePath();

#if defined(Q_OS_MAC)
    QString canonical = info.canonicalFilePath();
#elif defined(Q_OS_WIN32)
    wchar_t buffer[1024];

    DWORD rv = ::GetShortPathName((wchar_t*)absolute.utf16(), buffer, 1024);
    if (rv == 0 || rv >= 1024) return true;
    rv = ::GetLongPathName(buffer, buffer, 1024);
    if (rv == 0 || rv >= 1024) return true;

    QString canonical((QChar *)buffer);
#endif

    int absoluteLength = absolute.length();
    int canonicalLength = canonical.length();

    int length = qMin(absoluteLength, canonicalLength);
    for (int ii = 0; ii < length; ++ii) {
        const QChar &a = absolute.at(absoluteLength - 1 - ii);
        const QChar &c = canonical.at(canonicalLength - 1 - ii);

        if (a.toLower() != c.toLower())
            return true;
        if (a != c)
            return false;
    }
#else
    Q_UNUSED(fileName)
#endif
    return true;
}

QT_END_NAMESPACE
