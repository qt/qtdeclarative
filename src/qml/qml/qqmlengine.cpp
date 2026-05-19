// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlengine_p.h"
#include "qqmlengine.h"

#include <private/qqmlcontext_p.h>
#include <private/qqmlpluginimporter_p.h>
#include <private/qqmlprofiler_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qqmlsourcecoordinate_p.h>
#include <private/qqmltype_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlcomponent_p.h>

#include <QtQml/qqml.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlincubator.h>
#include <QtQml/qqmlscriptstring.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qstorageinfo.h>
#include <QtCore/qthread.h>
#include <QtCore/qtyperevision.h>

#if QT_CONFIG(qml_network)
#include <QtQml/qqmlnetworkaccessmanagerfactory.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#endif

#ifdef Q_OS_WIN // for %APPDATA%
#  include <qt_windows.h>
#  include <shlobj.h>
#  include <QtCore/qlibrary.h>
#  ifndef CSIDL_APPDATA
#    define CSIDL_APPDATA           0x001a  // <username>\Application Data
#  endif
#endif // Q_OS_WIN

#ifdef Q_OS_DARWIN
#  include <unistd.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

void qml_register_types_QML();

/*!
  \qmltype QtObject
    \nativetype QObject
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

Q_CONSTINIT std::atomic<bool> QQmlEnginePrivate::qml_debugging_enabled{false};
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

QQmlEnginePrivate::~QQmlEnginePrivate()
{
    if (inProgressCreations)
        qWarning() << QQmlEngine::tr("There are still \"%1\" items in the process of being created at engine destruction.").arg(inProgressCreations);

    if (incubationController) incubationController->d = nullptr;
    incubationController = nullptr;

#if QT_CONFIG(qml_debug)
    delete profiler;
#endif
}

void QQmlPrivate::qdeclarativeelement_destructor(QObject *o)
{
    QObjectPrivate *p = QObjectPrivate::get(o);
    if (QQmlData *d = QQmlData::get(p)) {
        const auto invalidate = [](QQmlContextData *c) {c->invalidate();};
        if (d->ownContext) {
            d->ownContext->deepClearContextObject(o, invalidate, invalidate);
            d->ownContext.reset();
            d->context = nullptr;
            Q_ASSERT(!d->outerContext || d->outerContext->contextObject() != o);
        } else if (d->outerContext && d->outerContext->contextObject() == o) {
            d->outerContext->deepClearContextObject(o, invalidate, invalidate);
        }

        if (d->hasVMEMetaObject || d->hasInterceptorMetaObject) {
            // This is somewhat dangerous because another thread might concurrently
            // try to resolve the dynamic metaobject. In practice this will then
            // lead to either the code path that still returns the interceptor
            // metaobject or the code path that returns the string casted one. Both
            // is fine if you cannot actually touch the object itself. Since the
            // other thread is obviously not synchronized to this one, it can't.
            //
            // In particular we do this when delivering the frameSwapped() signal
            // in QQuickWindow. The handler for frameSwapped() is written in a way
            // that is thread safe as long as QQuickWindow's dtor hasn't finished.
            // QQuickWindow's dtor does synchronize with the render thread, but it
            // runs _after_ qdeclarativeelement_destructor.
            static_cast<QQmlInterceptorMetaObject *>(p->metaObject)->invalidate();
            d->hasVMEMetaObject = d->hasInterceptorMetaObject = false;
        }

        // Mark this object as in the process of deletion to
        // prevent it resolving in bindings
        QQmlData::markAsDeleted(o);
    }
}

template<>
int qmlRegisterType<void>(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    QQmlPrivate::RegisterType type = {
        QQmlPrivate::RegisterType::CurrentVersion,
        QMetaType(),
        QMetaType(),
        0, nullptr, nullptr,
        QString(),
        nullptr,
        uri,
        QTypeRevision::fromVersion(versionMajor, versionMinor),
        qmlName,
        nullptr,
        nullptr,
        nullptr,
        -1,
        -1,
        -1,
        nullptr,
        nullptr,
        nullptr,
        QTypeRevision::zero(),
        -1,
        QQmlPrivate::ValueTypeCreationMethod::None,
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

bool QQmlEnginePrivate::baseModulesUninitialized = true;
void QQmlEnginePrivate::init()
{
    Q_Q(QQmlEngine);

    if (baseModulesUninitialized) {
        // Register builtins
        qml_register_types_QML();

        // No need to specifically register those.
        static_assert(std::is_same_v<QStringList, QList<QString>>);
        static_assert(std::is_same_v<QVariantList, QList<QVariant>>);

        qRegisterMetaType<QQmlScriptString>();
        qRegisterMetaType<QQmlComponent::Status>();
        qRegisterMetaType<QList<QObject*> >();
        qRegisterMetaType<QQmlBinding*>();

        // Protect the module: We don't want any URL interceptor to mess with the builtins.
        qmlProtectModule("QML", 1);

        QQmlData::init();
        baseModulesUninitialized = false;
    }

    q->handle()->setQmlEngine(q);

    rootContext = new QQmlContext(q,true);
}

/*!
  \class QQmlEngine
  \since 5.0
  \inmodule QtQml
  \brief The QQmlEngine class provides an environment for instantiating QML components.

  A QQmlEngine is used to manage \l{QQmlComponent}{components} and objects created from
  them and execute their bindings and functions. QQmlEngine also inherits from
  \l{QJSEngine} which allows seamless integration between your QML components and
  JavaScript code.

  Each QML component is instantiated in a QQmlContext. In QML, contexts are arranged
  hierarchically and this hierarchy is managed by the QQmlEngine. By default,
  components are instantiated in the \l {QQmlEngine::rootContext()}{root context}.

  \sa QQmlComponent, QQmlContext, {QML Global Object}, QQmlApplicationEngine
*/

/*!
  Create a new QQmlEngine with the given \a parent.
*/
QQmlEngine::QQmlEngine(QObject *parent)
: QJSEngine(*new QQmlEnginePrivate, parent)
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

  See ~QJSEngine() for details on cleaning up the JS engine.
*/
QQmlEngine::~QQmlEngine()
{
    Q_D(QQmlEngine);

#if QT_CONFIG(qml_worker_script)
    // Delete the workerscript engine early
    // so that it won't be able to use the type loader anymore.
    delete std::exchange(d->workerScriptEngine, nullptr);
#endif

    QV4::ExecutionEngine *v4 = handle();
    v4->inShutdown = true;
    QJSEnginePrivate::removeFromDebugServer(this);

    // Emit onDestruction signals for the root context before
    // we destroy the contexts, engine, Singleton Types etc. that
    // may be required to handle the destruction signal.
    QQmlContextPrivate::get(rootContext())->emitDestruction();

    // clean up all singleton type instances which we own.
    // we do this here and not in the private dtor since otherwise a crash can
    // occur (if we are the QObject parent of the QObject singleton instance)
    // XXX TODO: performance -- store list of singleton types separately?
    d->singletonInstances.clear();

    delete d->rootContext;
    d->rootContext = nullptr;

    v4->typeLoader()->invalidate();

    // QQmlGadgetPtrWrapper can have QQmlData with various references.
    qDeleteAll(d->cachedValueTypeInstances);
    d->cachedValueTypeInstances.clear();

    v4->resetQmlEngine();
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

  This function causes the property metadata of most components previously
  loaded by the engine to be destroyed. It does so by dropping unreferenced
  components from the engine's component cache. It does not drop components that
  are still referenced since that would almost certainly lead to crashes further
  down the line.

  If no components are referenced, this function returns the engine to a state
  where it does not contain any loaded component data. This may be useful in
  order to reload a smaller subset of the previous component set, or to load a
  new version of a previously loaded component.

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

  \sa trimComponentCache(), clearSingletons()
 */
void QQmlEngine::clearComponentCache()
{
    Q_D(QQmlEngine);

    // QQmlGadgetPtrWrapper can have QQmlData with various references.
    qDeleteAll(std::exchange(d->cachedValueTypeInstances, {}));

    QV4::ExecutionEngine *v4 = handle();

    // Reset the values of JavaScript libraries and ECMAScript modules
    // So that they get re-evaluated on next usage.
    {
        const auto cus = v4->compilationUnits();
        for (const auto &cu : cus) {
            cu->setValue(QV4::Value::emptyValue());
            delete[] std::exchange(cu->imports, nullptr);
        }
    }

    // Contexts can hold on to CUs but live on the JS heap.
    // Use a non-incremental GC run to get rid of those.
    QV4::MemoryManager *mm = v4->memoryManager;
    auto oldLimit = mm->gcStateMachine->timeLimit;
    mm->setGCTimeLimit(-1);
    mm->runGC();
    mm->gcStateMachine->timeLimit = std::move(oldLimit);

    v4->trimCompilationUnits();
    v4->typeLoader()->clearCache();
    QQmlMetaType::freeUnusedTypesAndCaches();
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
    QV4::ExecutionEngine *v4 = handle();
    v4->trimCompilationUnits();
    v4->typeLoader()->trimCache();
}

/*!
  Clears all singletons the engine owns.

  This function drops all singleton instances, deleting any QObjects owned by
  the engine among them. This is useful to make sure that no QML-created objects
  are left before calling clearComponentCache().

  QML properties holding QObject-based singleton instances become null if the
  engine owns the singleton or retain their value if the engine doesn't own it.
  The singletons are not automatically re-created by accessing existing
  QML-created objects. Only when new components are instantiated, the singletons
  are re-created.

  \sa clearComponentCache()
 */
void QQmlEngine::clearSingletons()
{
    Q_D(QQmlEngine);
    d->singletonInstances.clear();
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
    return QQmlTypeLoader::get(this)->urlInterceptors().constLast();
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
    QQmlTypeLoader::get(this)->addUrlInterceptor(urlInterceptor);
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
    QQmlTypeLoader::get(this)->removeUrlInterceptor(urlInterceptor);
}

/*!
  Run the current URL interceptors on the given \a url of the given \a type and
  return the result.
 */
QUrl QQmlEngine::interceptUrl(const QUrl &url, QQmlAbstractUrlInterceptor::DataType type) const
{
    return QQmlTypeLoader::get(this)->interceptUrl(url, type);
}

/*!
  Returns the list of currently active URL interceptors.
 */
QList<QQmlAbstractUrlInterceptor *> QQmlEngine::urlInterceptors() const
{
    return QQmlTypeLoader::get(this)->urlInterceptors();
}

QSharedPointer<QQmlImageProviderBase> QQmlEnginePrivate::imageProvider(const QString &providerId) const
{
    const QString providerIdLower = providerId.toLower();
    QMutexLocker locker(&imageProviderMutex);
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
    QQmlTypeLoader::get(this)->setNetworkAccessManagerFactory(factory);
}

class QQmlEnginePublicAPIToken {};

/*!
  Returns the current QQmlNetworkAccessManagerFactory.

  \sa setNetworkAccessManagerFactory()
*/
QQmlNetworkAccessManagerFactory *QQmlEngine::networkAccessManagerFactory() const
{
    return QQmlTypeLoader::get(this)->networkAccessManagerFactory().get(QQmlEnginePublicAPIToken());
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
    return handle()->getNetworkAccessManager();
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
    QMutexLocker locker(&d->imageProviderMutex);
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
    QMutexLocker locker(&d->imageProviderMutex);
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
    QMutexLocker locker(&d->imageProviderMutex);
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
  \since 6.6
  If this method is called inside of a function that is part of
  a binding in QML, the binding will be treated as a translation binding.

  \code
  class I18nAwareClass : public QObject {

    //...

     QString text() const
     {
          if (auto engine = qmlEngine(this))
              engine->markCurrentFunctionAsTranslationBinding();
          return tr("Hello, world!");
     }
  };
  \endcode

  \note This function is mostly useful if you wish to provide your
  own alternative to the qsTr function. To ensure that properties
  exposed from C++ classes are updated on language changes, it is
  instead recommended to react to \c LanguageChange events. That
  is a more general mechanism which also works when the class is
  used in a non-QML context, and has slightly less overhead. However,
  using \c markCurrentFunctionAsTranslationBinding can be acceptable
  when the class is already closely tied to the QML engine.
  For more details, see \l {Prepare for Dynamic Language Changes}

  \sa QQmlEngine::retranslate
*/
void QQmlEngine::markCurrentFunctionAsTranslationBinding()
{
    Q_D(QQmlEngine);
    if (auto propertyCapture = d->propertyCapture)
        propertyCapture->captureTranslation();
}

/*!
  \since 6.12
  Set the singleton instance to use for the given type on the QML engine.

  This function allows you to manually set a QObject-derived instance to use as
  the singleton in QML for this engine. This allows you to control the creation
  of the instance. This can be useful in several scenarios, including for
  cases where your singleton needs to communicate with backend components.

  This function takes the \a moduleName and \a typeName to indicate the
  singleton type you are trying to set, and the \a instance to set. The type
  has to be already registered as a QML singleton type, ideally by using
  \l QML_ELEMENT and \l QML_SINGLETON. If the module has not already been
  loaded, it will be now.

  The function returns true on success or false on failure. If a
  failure occurs, a warning is emitted detailing the failure.

  As an example, the singleton might need a backend service to work,
  and could then be declared as follows:
  \snippet code/src_qml_qqmlengine.cpp 6

  Upon initialization of the application, you can then do:
  \snippet code/src_qml_qqmlengine.cpp 7

  Note instead of providing a default constructor or a static create
  function, the \l QML_UNCREATABLE() macro was used to indicate this
  item cannot be created by the qml engine.

  Singleton instances can only be set once per type and engine, and must
  be set before any use. Once a singleton instance is created or set,
  it is no longer possible to set it using this function, so you should set
  the instances before they are first used from QML.

  The engine will \e{not} take ownership of the instance you pass, unless
  you explicitly instruct the engine to do so by using
  \l QJSEngine::setObjectOwnership().

  \warning Make sure the \a instance outlives the lifetime of the engine.
*/
bool QQmlEngine::setExternalSingletonInstance(QAnyStringView moduleName, QAnyStringView typeName, QObject *instance)
{
    Q_D(QQmlEngine);

    const auto loadHelper = QQml::makeRefPointer<LoadHelper>(
            QQmlTypeLoader::get(this), moduleName, typeName, QQmlTypeLoader::Synchronous);
    const QQmlType type = loadHelper->type();

    if (!type.isValid()) {
        qWarning().noquote() << "Error setting singleton instance: type" << typeName << "in module" << moduleName << "is not valid";
        return false;
    }

    if (!instance) {
        qWarning() << "Error setting singleton instance: the instance cannot be a nullptr";
        return false;
    }
    if (!type.isSingleton()) {
        qWarning() << "Error setting singleton instance: the type" << type.elementName() << "is not declared as a singleton type";
        return false;
    }
    const QQmlType::SingletonInstanceInfo::ConstPtr siinfo = type.singletonInstanceInfo();
    Q_ASSERT(siinfo != nullptr);
    QJSValue value = d->singletonInstances.value(siinfo);
    if (!value.isUndefined()) {
        qWarning() << "Error setting singleton instance: there already is an instance for this singleton";
        return false;
    }

    const auto baseMetaObject = type.baseMetaObject();
    if (!(baseMetaObject && instance->metaObject()->inherits(baseMetaObject))) {
        qWarning() << "Error setting singleton instance: the meta type of the instance" << instance->metaObject()->className()
        << "does not match the type of the registered singleton"
        << (baseMetaObject ? baseMetaObject->className() : "(unknown)"); //be careful to assume baseMetaObject is valid
        return false;
    }

    QQmlData *data = QQmlData::get(instance, true);
    if (!data->explicitIndestructibleSet) {
        // Unless already explicitly set, set it up so that the engine won't delete
        // the object.
        data->explicitIndestructibleSet = true;
        data->indestructible = true;
    }
    // even though the object is defined in C++, qmlContext(obj) and qmlEngine(obj)
    // should behave identically to QML singleton types. You can, however, manually
    // assign a context; and clearSingletons() retains the contexts, in which case
    // we don't want to see warnings about the object already having a context.
    if (!data->context) {
        auto contextData = QQmlContextData::get(new QQmlContext(rootContext(), this));
        data->context = contextData.data();
        contextData->addOwnedObject(data);
    }

    value = newQObject(instance);
    d->singletonInstances.convertAndInsert(d->v4Engine.get(), siinfo, &value);

    return true;
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
  string translations. It is exposed in C++ as \l QJSEngine::uiLanguage property.

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
    QQmlType type = QQmlMetaType::qmlTypeById(qmlTypeId);

    if (!type.isValid()) {
        qWarning().noquote() << "Singleton instance: type with id" << qmlTypeId << "is not valid";
        return {};
    }

    if (!type.isSingleton()) {
        qWarning() << "Singleton instance: type" << type.elementName() << "with id" << qmlTypeId << "is not declared as a singleton type";
        return {};
    }

    return d->singletonInstance<QJSValue>(type);
}


/*!
  \fn template<typename T> T QQmlEngine::singletonInstance(QAnyStringView uri, QAnyStringView typeName)

  \overload
  Returns the instance of a singleton type named \a typeName from the module specified by \a uri.

  This method can be used as an alternative to calling qmlTypeId followed by the id based overload of
  singletonInstance. This is convenient when one only needs to do a one time setup of a
  singleton; if repeated access to the singleton is required, caching its typeId will allow
  faster subsequent access via the
  \l {QQmlEngine::singletonInstance(int qmlTypeId)}{type-id based overload}.

  The template argument \e T may be either QJSValue or a pointer to a QObject-derived
  type and depends on how the singleton was registered. If no instance of \e T has been
  created yet, it is created now. If \a typeName does not represent a valid singleton
  type, either a default constructed QJSValue or a \c nullptr is returned.

  \snippet code/src_qml_qqmlengine.cpp 5

  \sa QML_SINGLETON, qmlRegisterSingletonType(), qmlTypeId()
  \since 6.5
*/
template<>
QJSValue QQmlEngine::singletonInstance<QJSValue>(QAnyStringView uri, QAnyStringView typeName)
{
    Q_D(QQmlEngine);

    auto loadHelper = QQml::makeRefPointer<LoadHelper>(
            QQmlTypeLoader::get(this), uri, typeName, QQmlTypeLoader::Synchronous);
    const QQmlType type = loadHelper->type();

    if (!type.isValid()) {
        qWarning().noquote() << "Singleton instance: type" << typeName << "in module" << uri << "is not valid";
        return {};
    }

    if (!type.isSingleton()) {
        qWarning() << "Singleton instance: type" << type.elementName() << "is not declared as a singleton type";
        return {};
    }

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
  Returns the QQmlContext for the \a object, or nullptr if no
  context has been set.

  When the QQmlEngine instantiates a QObject, an internal context is assigned
  to it automatically. Such internal contexts are read-only. You cannot set
  context properties on them.

  \sa qmlContext(), qmlEngine(), QQmlContext::setContextProperty()
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
    if (e->type() == QEvent::LanguageChange) {
        retranslate();
    }

    return QJSEngine::event(e);
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
    for (int ii = 0; ii < errors.size(); ++ii)
        dumpwarning(errors.at(ii));
}

void QQmlEnginePrivate::warning(const QQmlError &error)
{
    Q_Q(QQmlEngine);
    emit q->warnings(QList<QQmlError>({error}));
    if (outputWarningsToMsgLog)
        dumpwarning(error);
}

void QQmlEnginePrivate::warning(const QList<QQmlError> &errors)
{
    Q_Q(QQmlEngine);
    emit q->warnings(errors);
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
    QV4::ExecutionEngine *engine = v4Engine.get();
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

  \b {See also} \l setImportPathList(), \l {QML Modules},
    and \l [QtQml] {QML Import Path}
*/
void QQmlEngine::addImportPath(const QString& path)
{
    QQmlTypeLoader::get(this)->addImportPath(path);
}

/*!
  Returns the list of directories where the engine searches for
  installed modules in a URL-based directory structure.

  For example, if \c /opt/MyApp/lib/imports is in the path, then QML that
  imports \c com.mycompany.Feature will cause the QQmlEngine to look
  in \c /opt/MyApp/lib/imports/com/mycompany/Feature/ for the components
  provided by that module. A \c qmldir file is required for defining the
  type version mapping and possibly QML extensions plugins.

  By default, this list contains the paths mentioned in
  \l {QML Import Path}.

  \sa addImportPath(), setImportPathList()
*/
QStringList QQmlEngine::importPathList() const
{
    return QQmlTypeLoader::get(this)->importPathList();
}

/*!
  Sets \a paths as the list of directories where the engine searches for
  installed modules in a URL-based directory structure.

  By default, this list contains the paths mentioned in
  \l {QML Import Path}.

  \warning Calling setImportPathList does not preserve the default
  import paths.

  \sa importPathList(), addImportPath()
  */
void QQmlEngine::setImportPathList(const QStringList &paths)
{
    QQmlTypeLoader::get(this)->setImportPathList(paths);
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
    QQmlTypeLoader::get(this)->addPluginPath(path);
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
    return QQmlTypeLoader::get(this)->pluginPathList();
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
    QQmlTypeLoader::get(this)->setPluginPathList(paths);
}

#if QT_CONFIG(library)
#if QT_DEPRECATED_SINCE(6, 4)
/*!
  \deprecated [6.4] Import the module from QML with an "import" statement instead.

  Imports the plugin named \a filePath with the \a uri provided.
  Returns true if the plugin was successfully imported; otherwise returns false.

  On failure and if non-null, the \a errors list will have any errors which occurred prepended to it.

  The plugin has to be a Qt plugin which implements the QQmlEngineExtensionPlugin interface.

  \note Directly loading plugins like this can confuse the module import logic. In order to make
        the import logic load plugins from a specific place, you can use \l addPluginPath(). Each
        plugin should be part of a QML module that you can import using the "import" statement.
*/
bool QQmlEngine::importPlugin(const QString &filePath, const QString &uri, QList<QQmlError> *errors)
{
    QQmlTypeLoaderQmldirContent qmldir;
    QQmlPluginImporter importer(uri, QTypeRevision(),  &qmldir, QQmlTypeLoader::get(this), errors);
    return importer.importDynamicPlugin(filePath, uri, false).isValid();
}
#endif
#endif

/*!
  \property QQmlEngine::offlineStoragePath
  \brief the directory for storing offline user data

  Returns the directory where SQL and other offline
  storage is placed.

  The SQL databases created with \c openDatabaseSync() are stored here.

  The default is QML/OfflineStorage in the platform-standard
  user application data directory.

  Note that the path may not currently exist on the filesystem, so
  callers wanting to \e create new files at this location should create
  it first - see QDir::mkpath().

  \sa {Qt Quick Local Storage QML Types}
*/

/*!
  \fn void QQmlEngine::offlineStoragePathChanged()
  This signal is emitted when \l offlineStoragePath changes.
  \since 6.5
*/

void QQmlEngine::setOfflineStoragePath(const QString& dir)
{
    Q_D(QQmlEngine);
    if (dir == d->offlineStoragePath)
        return;
    d->offlineStoragePath = dir;
    Q_EMIT offlineStoragePathChanged();
}

QString QQmlEngine::offlineStoragePath() const
{
    Q_D(const QQmlEngine);

    if (d->offlineStoragePath.isEmpty()) {
        QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QQmlEnginePrivate *e = const_cast<QQmlEnginePrivate *>(d);
        if (!dataLocation.isEmpty()) {
            e->offlineStoragePath = dataLocation.replace(QLatin1Char('/'), QDir::separator())
                                  + QDir::separator() + QLatin1String("QML")
                                  + QDir::separator() + QLatin1String("OfflineStorage");
            Q_EMIT e->q_func()->offlineStoragePathChanged();
        }
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

static bool hasRequiredProperties(const QQmlPropertyCache::ConstPtr &propertyCache)
{
    bool requiredPropertiesFound = false;
    // we don't expect to find any, so the loop has no early termination check
    if (propertyCache) {
        for (int idx = 0, count = propertyCache->propertyCount(); idx < count; ++idx)
            requiredPropertiesFound |= propertyCache->property(idx)->isRequired();
    }
    return requiredPropertiesFound;
}

template<>
QJSValue QQmlEnginePrivate::singletonInstance<QJSValue>(const QQmlType &type)
{
    Q_Q(QQmlEngine);

    QQmlType::SingletonInstanceInfo::ConstPtr siinfo = type.singletonInstanceInfo();
    Q_ASSERT(siinfo != nullptr);

    QJSValue value = singletonInstances.value(siinfo);
    if (!value.isUndefined())
        return value;

    if (siinfo->scriptCallback) {
        value = siinfo->scriptCallback(q, q);
        if (value.isQObject()) {
            QObject *o = value.toQObject();
            // even though the object is defined in C++, qmlContext(obj) and qmlEngine(obj)
            // should behave identically to QML singleton types.
            q->setContextForObject(o, new QQmlContext(q->rootContext(), q));
        }
        singletonInstances.convertAndInsert(v4Engine.get(), siinfo, &value);

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
            QQmlPropertyCache::ConstPtr propertyCache = QQmlData::ensurePropertyCache(o);
            if (Q_UNLIKELY(hasRequiredProperties(propertyCache))) {
                // there's no way to set required properties on a singleton
                delete o;
                o = nullptr;
                QQmlError error;
                error.setMessageType(QtMsgType::QtCriticalMsg);
                error.setDescription(QString::asprintf("Singleton \"%s\" is not available because the type has unset required properties.",
                                                       qPrintable(QString::fromUtf8(type.typeName()))));
                warning(error);
            } else {
                // even though the object is defined in C++, qmlContext(obj) and qmlEngine(obj)
                // should behave identically to QML singleton types. You can, however, manually
                // assign a context; and clearSingletons() retains the contexts, in which case
                // we don't want to see warnings about the object already having a context.
                QQmlData *data = QQmlData::get(o, true);
                if (!data->context) {
                    auto contextData = QQmlContextData::get(new QQmlContext(q->rootContext(), q));
                    data->context = contextData.data();
                    contextData->addOwnedObject(data);
                }
            }
        }

        value = q->newQObject(o);
        singletonInstances.convertAndInsert(v4Engine.get(), siinfo, &value);
    } else if (!siinfo->url.isEmpty()) {
        QQmlComponent component(q, siinfo->url, QQmlComponent::PreferSynchronous);
        if (component.isError()) {
            warning(component.errors());
            v4Engine->throwError(
                    QLatin1String("Due to the preceding error(s), "
                                  "Singleton \"%1\" could not be loaded.")
                            .arg(QString::fromUtf8(type.typeName())));

            return QJSValue(QJSValue::UndefinedValue);
        }
        QObject *o = component.beginCreate(q->rootContext());
        auto *compPriv = QQmlComponentPrivate::get(&component);
        if (compPriv->hasUnsetRequiredProperties()) {
            /* We would only get the errors from the component after (complete)Create.
                We can't call create, as we need to convertAndInsert before completeCreate (otherwise
                tst_qqmllanguage::compositeSingletonCircular fails).
                On the other hand, we don't want to call cnovertAndInsert if we have an error
                So create the unset required component errors manually.
            */
            delete o;
            const auto requiredProperties = compPriv->requiredProperties();
            QList<QQmlError> errors (requiredProperties->size());
            for (const auto &reqProp: *requiredProperties)
                errors.push_back(QQmlComponentPrivate::unsetRequiredPropertyToQQmlError(reqProp));
            warning(errors);
            v4Engine->throwError(
                    QLatin1String("Due to the preceding error(s), "
                                  "Singleton \"%1\" could not be loaded.")
                            .arg(QString::fromUtf8(type.typeName())));
            return QJSValue(QJSValue::UndefinedValue);
        }

        value = q->newQObject(o);
        singletonInstances.convertAndInsert(v4Engine.get(), siinfo, &value);
        component.completeCreate();
    }

    return value;
}

void QQmlEnginePrivate::executeRuntimeFunction(const QUrl &url, qsizetype functionIndex,
                                               QObject *thisObject, int argc, void **args,
                                               QMetaType *types)
{
    const auto unit = compilationUnitFromUrl(url);
    if (!unit)
        return;
    executeRuntimeFunction(unit, functionIndex, thisObject, argc, args, types);
}

void QQmlEnginePrivate::executeRuntimeFunction(const QV4::ExecutableCompilationUnit *unit,
                                               qsizetype functionIndex, QObject *thisObject,
                                               int argc, void **args, QMetaType *types)
{
    Q_ASSERT(unit);
    Q_ASSERT((functionIndex >= 0) && (functionIndex < unit->runtimeFunctions.size()));
    Q_ASSERT(thisObject);

    QQmlData *ddata = QQmlData::get(thisObject);
    Q_ASSERT(ddata && ddata->context);

    QV4::Function *function = unit->runtimeFunctions[functionIndex];
    Q_ASSERT(function);
    Q_ASSERT(function->compiledFunction);

    QV4::ExecutionEngine *v4 = v4Engine.get();

    // NB: always use scriptContext() by default as this method ignores whether
    // there's already a stack frame (except when dealing with closures). the
    // method is called from C++ (through QQmlEngine::executeRuntimeFunction())
    // and thus the caller must ensure correct setup
    QV4::Scope scope(v4);
    QV4::ExecutionContext *ctx = v4->scriptContext();
    QV4::Scoped<QV4::ExecutionContext> callContext(scope,
        QV4::QmlContext::create(ctx, ddata->context, thisObject));

    if (auto nested = function->nestedFunction()) {
        // if a nested function is already known, call the closure directly
        function = nested;
    } else if (function->isClosureWrapper()) {
        // if there is a nested function, but we don't know it, we need to call
        // an outer function first and then the inner function. we fetch the
        // return value of a function call (that is a closure) by calling a
        // different version of ExecutionEngine::callInContext() that returns a
        // QV4::ReturnedValue with no arguments since they are not needed by the
        // outer function anyhow
        QV4::Scoped<QV4::JavaScriptFunctionObject> result(scope,
            v4->callInContext(function, thisObject, callContext, 0, nullptr));
        Q_ASSERT(result->function());
        Q_ASSERT(result->function()->compilationUnit == function->compilationUnit);

        // overwrite the function and its context
        function = result->function();
        callContext = QV4::Scoped<QV4::ExecutionContext>(scope, result->scope());
    }

    v4->callInContext(function, thisObject, callContext, argc, args, types);
}

QV4::ExecutableCompilationUnit *QQmlEnginePrivate::compilationUnitFromUrl(const QUrl &url)
{
    QV4::ExecutionEngine *v4 = v4Engine.get();
    if (auto unit = v4->compilationUnitForUrl(url)) {
        if (!unit->runtimeStrings)
            unit->populate();
        return unit.data();
    }

    auto unit = v4->typeLoader()->getType(url)->compilationUnit();
    if (!unit)
        return nullptr;

    auto executable = v4->executableCompilationUnit(std::move(unit));
    executable->populate();
    return executable.data();
}

QQmlRefPointer<QQmlContextData> QQmlEnginePrivate::createComponentRootContext(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
        const QQmlRefPointer<QQmlContextData> &parentContext, int subComponentIndex)
{
    Q_ASSERT(unit);

    QQmlRefPointer<QQmlContextData> context =
            createBareContext(unit, parentContext, subComponentIndex);

    const auto *dependentScripts = unit->dependentScriptsPtr();
    const qsizetype dependentScriptsSize = dependentScripts->size();
    if (!dependentScriptsSize)
        return context;

    QV4::ExecutionEngine *v4 = v4Engine.get();
    Q_ASSERT(v4);

    QV4::Scope scope(v4);
    QV4::ScopedObject scripts(scope, v4->newArrayObject(dependentScriptsSize));
    context->setImportedScripts(v4, scripts);
    QV4::ScopedValue v(scope);
    for (qsizetype i = 0; i < dependentScriptsSize; ++i)
        scripts->put(i, (v = dependentScripts->at(i)->scriptValueForContext(context)));

    return context;
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

void hasJsOwnershipIndicator(QQmlGuardImpl *) {};

LoadHelper::LoadHelper(
        QQmlTypeLoader *loader, QAnyStringView uri, QAnyStringView typeName,
        QQmlTypeLoader::Mode mode)
    : QQmlTypeLoader::Blob({}, QQmlDataBlob::QmlFile, loader)
    , m_uri(uri.toString())
    , m_typeName(typeName.toString())
    , m_mode(mode)
{
    m_typeLoader->loadWithStaticData(this, QByteArray(), m_mode);
}

void LoadHelper::registerCallback(QQmlComponentPrivate *callback)
{
    m_callback = callback;
}

void LoadHelper::unregisterCallback(QQmlComponentPrivate *callback)
{
    if (m_callback) {
        Q_ASSERT(callback == m_callback);
        m_callback = nullptr;
    }
}

void LoadHelper::done()
{
    if (!couldFindModule()) {
        m_resolveTypeResult = ResolveTypeResult::NoSuchModule;
        return;
    }

    QQmlTypeModule *module = QQmlMetaType::typeModule(m_uri, QTypeRevision{});
    if (module) {
        m_type = module->type(m_typeName, {});
        if (m_type.isValid()) {
            m_resolveTypeResult = ResolveTypeResult::ModuleFound;
            return;
        }
    }

    // The module exists (see check above), but there is no QQmlTypeModule
    // ==> pure QML module, attempt resolveType
    QTypeRevision versionReturn;
    QList<QQmlError> errors;
    QQmlImportNamespace *ns_return = nullptr;
    m_importCache->resolveType(
            typeLoader(), m_typeName, &m_type, &versionReturn, &ns_return, &errors);
    m_resolveTypeResult = ResolveTypeResult::ModuleFound;
}

void LoadHelper::completed()
{
    QQmlTypeLoader::Blob::completed();

    if (m_callback) {
        m_callback->completeLoadFromModule(m_uri, m_typeName);
        m_callback = nullptr;
    }
}

void LoadHelper::dataReceived(const SourceCodeData &)
{
    auto import = std::make_shared<PendingImport>();
    import->uri = m_uri;
    QList<QQmlError> errorList;
    if (!Blob::addImport(import, &errorList)) {
        qCDebug(lcQmlImport) << "LoadHelper: Errors loading " << m_uri << errorList;
        m_uri.clear(); // reset m_uri to remember the failure
    }
}

bool LoadHelper::couldFindModule() const
{
    if (m_uri.isEmpty())
        return false;
    for (const auto &import: std::as_const(m_unresolvedImports))
        if (import->priority == 0) // compare QQmlTypeData::allDependenciesDone
            return false;
    return true;
}

QT_END_NAMESPACE

#include "moc_qqmlengine.cpp"
