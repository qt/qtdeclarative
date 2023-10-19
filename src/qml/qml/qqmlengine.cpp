// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlengine_p.h"
#include "qqmlengine.h"

#include "qqmlcontext_p.h"
#include "qqml.h"
#include "qqmlcontext.h"
#include "qqmlscriptstring.h"
#include "qqmlglobal_p.h"
#include "qqmlnotifier_p.h"
#include "qqmlincubator.h"
#include "qqmlabstracturlinterceptor.h"

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
#include <private/qqmlscriptdata_p.h>
#include <QtQml/private/qqmlcomponentattached_p.h>
#include <QtQml/private/qqmlsourcecoordinate_p.h>
#include <QtQml/private/qqmlcomponent_p.h>

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
#include <private/qv4sequenceobject_p.h>

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

    QQmlMetaType::freeUnusedTypesAndCaches();

#if QT_CONFIG(qml_debug)
    delete profiler;
#endif
    qDeleteAll(cachedValueTypeInstances);
}

void QQmlPrivate::qdeclarativeelement_destructor(QObject *o)
{
    QObjectPrivate *p = QObjectPrivate::get(o);
    if (QQmlData *d = QQmlData::get(p)) {
        if (d->ownContext) {
            for (QQmlRefPointer<QQmlContextData> lc = d->ownContext->linkedContext(); lc;
                 lc = lc->linkedContext()) {
                lc->invalidate();
                if (lc->contextObject() == o)
                    lc->setContextObject(nullptr);
            }
            d->ownContext->invalidate();
            if (d->ownContext->contextObject() == o)
                d->ownContext->setContextObject(nullptr);
            d->ownContext.reset();
            d->context = nullptr;
        }

        if (d->outerContext && d->outerContext->contextObject() == o)
            d->outerContext->setContextObject(nullptr);

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

QQmlData::QQmlData(Ownership ownership)
    : ownMemory(ownership == OwnsMemory), indestructible(true), explicitIndestructibleSet(false),
      hasTaintedV4Object(false), isQueuedForDeletion(false), rootObjectInCreation(false),
      hasInterceptorMetaObject(false), hasVMEMetaObject(false), hasConstWrapper(false),
      bindingBitsArraySize(InlineBindingArraySize), notifyList(nullptr),
      bindings(nullptr), signalHandlers(nullptr), nextContextObject(nullptr), prevContextObject(nullptr),
      lineNumber(0), columnNumber(0), jsEngineId(0),
      guards(nullptr), extendedData(nullptr)
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

    // Relaxed semantics here. If we're on a different thread we might schedule a useless event,
    // but that should be rare.
    if (!ddata->notifyList.loadRelaxed())
        return;

    auto objectThreadData = QObjectPrivate::get(object)->threadData.loadRelaxed();
    if (QThread::currentThreadId() != objectThreadData->threadId.loadRelaxed()) {
        if (!objectThreadData->thread.loadAcquire())
            return;

        QMetaMethod m = QMetaObjectPrivate::signal(object->metaObject(), index);
        QList<QByteArray> parameterTypes = m.parameterTypes();

        auto ev = std::make_unique<QMetaCallEvent>(m.methodIndex(), 0, nullptr,
                                                   object, index,
                                                   parameterTypes.size() + 1);

        void **args = ev->args();
        QMetaType *types = ev->types();

        for (int ii = 0; ii < parameterTypes.size(); ++ii) {
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
        QCoreApplication::postEvent(mpo, ev.release());

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
    QVarLengthArray<QObject *> workStack;
    workStack.push_back(o);
    while (!workStack.isEmpty()) {
        auto currentObject = workStack.last();
        workStack.pop_back();
        QQmlData::setQueuedForDeletion(currentObject);
        auto currentObjectPriv = QObjectPrivate::get(currentObject);
        for (QObject *child: std::as_const(currentObjectPriv->children))
            workStack.push_back(child);
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
                ddata->ownContext.reset();
                ddata->context = nullptr;
            }
            ddata->isQueuedForDeletion = true;

            // Disconnect the notifiers now - during object destruction this would be too late,
            // since the disconnect call wouldn't be able to call disconnectNotify(), as it isn't
            // possible to get the metaobject anymore.
            // Also, there is no point in evaluating bindings in order to set properties on
            // half-deleted objects.
            ddata->disconnectNotifiers(DeleteNotifyList::No);
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

QQmlData::DeferredData::DeferredData() = default;
QQmlData::DeferredData::~DeferredData() = default;

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
        // Named builtins
        qmlRegisterType<void>("QML", 1, 0, "void");

        const int varId = qmlRegisterType<QVariant>("QML", 1, 0, "var");
        QQmlMetaType::registerTypeAlias(varId, QLatin1String("variant"));
        qmlRegisterAnonymousSequentialContainer<QList<QVariant>>("QML", 1);

        qmlRegisterType<QObject>("QML", 1, 0, "QtObject");
        qmlRegisterType<QQmlComponent>("QML", 1, 0, "Component");

        qmlRegisterType<int>("QML", 1, 0, "int");
        qmlRegisterAnonymousSequentialContainer<QList<int>>("QML", 1);

        const int realId = qmlRegisterType<double>("QML", 1, 0, "real");
        QQmlMetaType::registerTypeAlias(realId, QLatin1String("double"));
        qmlRegisterAnonymousSequentialContainer<QList<double>>("QML", 1);

        qmlRegisterType<QString>("QML", 1, 0, "string");
        qmlRegisterAnonymousSequentialContainer<QList<QString>>("QML", 1);

        qmlRegisterType<bool>("QML", 1, 0, "bool");
        qmlRegisterAnonymousSequentialContainer<QList<bool>>("QML", 1);

        qmlRegisterType<QDateTime>("QML", 1, 0, "date");
        qmlRegisterAnonymousSequentialContainer<QList<QDateTime>>("QML", 1);

        qmlRegisterType<QUrl>("QML", 1, 0, "url");
        qmlRegisterAnonymousSequentialContainer<QList<QUrl>>("QML", 1);

#if QT_CONFIG(regularexpression)
        qmlRegisterType<QRegularExpression>("QML", 1, 0, "regexp");
        qmlRegisterAnonymousSequentialContainer<QList<QRegularExpression>>("QML", 1);
#else
        qmlRegisterType<void>("QML", 1, 0, "regexp");
#endif

        // Anonymous builtins
        qmlRegisterAnonymousType<std::nullptr_t>("QML", 1);
        qmlRegisterAnonymousType<QVariantMap>("QML", 1);

        qmlRegisterAnonymousType<QJSValue>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<QJSValue>>("QML", 1);

        qmlRegisterAnonymousType<qint8>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<qint8>>("QML", 1);

        qmlRegisterAnonymousType<quint8>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<quint8>>("QML", 1);

        qmlRegisterAnonymousType<short>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<short>>("QML", 1);

        qmlRegisterAnonymousType<ushort>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<ushort>>("QML", 1);

        qmlRegisterAnonymousType<uint>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<uint>>("QML", 1);

        qmlRegisterAnonymousType<qlonglong>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<qlonglong>>("QML", 1);

        qmlRegisterAnonymousType<qulonglong>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<qulonglong>>("QML", 1);

        qmlRegisterAnonymousType<float>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<float>>("QML", 1);

        qmlRegisterAnonymousType<QChar>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<QChar>>("QML", 1);

        qmlRegisterAnonymousType<QDate>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<QDate>>("QML", 1);

        qmlRegisterAnonymousType<QTime>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<QTime>>("QML", 1);

        qmlRegisterAnonymousType<QByteArray>("QML", 1);
        qmlRegisterAnonymousSequentialContainer<QList<QByteArray>>("QML", 1);

        // No need to specifically register those.
        static_assert(std::is_same_v<QStringList, QList<QString>>);
        static_assert(std::is_same_v<QVariantList, QList<QVariant>>);

        qRegisterMetaType<QQmlScriptString>();
        qRegisterMetaType<QQmlComponent::Status>();
        qRegisterMetaType<QList<QObject*> >();
        qRegisterMetaType<QQmlBinding*>();

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

  A QQmlEngine is used to manage \l{components}{QQmlComponent} and objects created from
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

  See ~QJSEngine() for details on cleaning up the JS engine.
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
    d->singletonInstances.clear();

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

  \sa trimComponentCache(), clearSingletons()
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
    QQmlType type = QQmlMetaType::qmlTypeById(qmlTypeId);

    if (!type.isValid() || !type.isSingleton())
        return QJSValue();

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

    auto loadHelper = QQml::makeRefPointer<LoadHelper>(&d->typeLoader, uri);

    auto [moduleStatus, type] = loadHelper->resolveType(typeName);

    if (moduleStatus == LoadHelper::ResolveTypeResult::NoSuchModule)
        return {};
    if (!type.isValid())
        return {};
    if (!type.isSingleton())
        return {};

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
        if (binding->hasFlag(QV4::CompiledData::Binding::IsDeferredBinding))
            deferData->bindings.insert(property ? property->coreIndex() : -1, binding);
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
    // Can only happen on "home" thread. We apply relaxed semantics when loading the atomics.

    NotifyList *list = notifyList.loadRelaxed();

    if (!list) {
        list = new NotifyList;
        // We don't really care when this change takes effect on other threads. The notifyList can
        // only become non-null once in the life time of a QQmlData. It becomes null again when the
        // underlying QObject is deleted. At that point any interaction with the QQmlData is UB
        // anyway. So, for all intents and purposese, the list becomes non-null once and then stays
        // non-null "forever". We can apply relaxed semantics.
        notifyList.storeRelaxed(list);
    }

    Q_ASSERT(!endpoint->isConnected());

    index = qMin(index, 0xFFFF - 1);

    // Likewise, we don't really care _when_ the change in the connectionMask is propagated to other
    // threads. Cross-thread event ordering is inherently nondeterministic. Therefore, when querying
    // the conenctionMask in the presence of concurrent modification, any result is correct.
    list->connectionMask.storeRelaxed(
            list->connectionMask.loadRelaxed() | (1ULL << quint64(index % 64)));

    if (index < list->notifiesSize) {
        endpoint->next = list->notifies[index];
        if (endpoint->next) endpoint->next->prev = &endpoint->next;
        endpoint->prev = &list->notifies[index];
        list->notifies[index] = endpoint;
    } else {
        list->maximumTodoIndex = qMax(int(list->maximumTodoIndex), index);

        endpoint->next = list->todo;
        if (endpoint->next) endpoint->next->prev = &endpoint->next;
        endpoint->prev = &list->todo;
        list->todo = endpoint;
    }
}

void QQmlData::disconnectNotifiers(QQmlData::DeleteNotifyList doDelete)
{
    // Can only happen on "home" thread. We apply relaxed semantics when loading  the atomics.
    if (NotifyList *list = notifyList.loadRelaxed()) {
        while (QQmlNotifierEndpoint *todo = list->todo)
            todo->disconnect();
        for (int ii = 0; ii < list->notifiesSize; ++ii) {
            while (QQmlNotifierEndpoint *ep = list->notifies[ii])
                ep->disconnect();
        }
        free(list->notifies);

        if (doDelete == DeleteNotifyList::Yes) {
            // We can only get here from QQmlData::destroyed(), and that can only come from the
            // the QObject dtor. If you're still sending signals at that point you have UB already
            // without any threads. Therefore, it's enough to apply relaxed semantics.
            notifyList.storeRelaxed(nullptr);
            delete list;
        } else {
            // We can use relaxed semantics here. The worst thing that can happen is that some
            // signal is falsely reported as connected. Signal connectedness across threads
            // is not quite deterministic anyway.
            list->connectionMask.storeRelaxed(0);
            list->maximumTodoIndex = 0;
            list->notifiesSize = 0;
            list->notifies = nullptr;

        }
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

    compilationUnit.reset();

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
        propertyCache.reset();

    ownContext.reset();

    while (guards) {
        auto *guard = guards;
        guard->setObject(nullptr);
        if (guard->objectDestroyed)
            guard->objectDestroyed(guard);
    }

    disconnectNotifiers(DeleteNotifyList::Yes);

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
    priv->declarativeData = new QQmlData(OwnsMemory);
    return static_cast<QQmlData *>(priv->declarativeData);
}

QQmlPropertyCache::ConstPtr QQmlData::createPropertyCache(QObject *object)
{
    QQmlData *ddata = QQmlData::get(object, /*create*/true);
    ddata->propertyCache = QQmlMetaType::propertyCache(object, QTypeRevision {});
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

  \b {See also} \l setImportPathList(), \l {QML Modules},
    and \l [QtQml] {QML Import Path}
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

  By default, this list contains the paths mentioned in
  \l {QML Import Path}.

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

  By default, this list contains the paths mentioned in
  \l {QML Import Path}.

  \warning Calling setImportPathList does not preserve the default
  import paths.

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
    Q_D(QQmlEngine);
    QQmlTypeLoaderQmldirContent qmldir;
    QQmlPluginImporter importer(
                uri, QTypeRevision(), &d->importDatabase, &qmldir, &d->typeLoader, errors);
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
            QQmlData::ensurePropertyCache(o);

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
    Q_ASSERT(ddata && ddata->outerContext);

    QV4::Function *function = unit->runtimeFunctions[functionIndex];
    Q_ASSERT(function);
    Q_ASSERT(function->compiledFunction);

    QV4::ExecutionEngine *v4 = v4engine();

    // NB: always use scriptContext() by default as this method ignores whether
    // there's already a stack frame (except when dealing with closures). the
    // method is called from C++ (through QQmlEngine::executeRuntimeFunction())
    // and thus the caller must ensure correct setup
    QV4::Scope scope(v4);
    QV4::ExecutionContext *ctx = v4->scriptContext();
    QV4::Scoped<QV4::ExecutionContext> callContext(scope,
        QV4::QmlContext::create(ctx, ddata->outerContext, thisObject));

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
        QV4::ScopedFunctionObject result(scope,
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
    auto unit = typeLoader.getType(url)->compilationUnit();
    if (!unit)
        return nullptr;
    if (!unit->engine)
        unit->linkToEngine(v4engine());
    return unit;
}

QQmlRefPointer<QQmlContextData>
QQmlEnginePrivate::createInternalContext(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
                                         const QQmlRefPointer<QQmlContextData> &parentContext,
                                         int subComponentIndex, bool isComponentRoot)
{
    Q_ASSERT(unit);

    QQmlRefPointer<QQmlContextData> context;
    context = QQmlContextData::createRefCounted(parentContext);
    context->setInternal(true);
    context->setImports(unit->typeNameCache);
    context->initFromTypeCompilationUnit(unit, subComponentIndex);

    if (isComponentRoot && unit->dependentScripts.size()) {
        QV4::ExecutionEngine *v4 = v4engine();
        Q_ASSERT(v4);
        QV4::Scope scope(v4);

        QV4::ScopedObject scripts(scope, v4->newArrayObject(unit->dependentScripts.size()));
        context->setImportedScripts(QV4::PersistentValue(v4, scripts.asReturnedValue()));
        QV4::ScopedValue v(scope);
        for (int i = 0; i < unit->dependentScripts.size(); ++i) {
            QQmlRefPointer<QQmlScriptData> s = unit->dependentScripts.at(i);
            scripts->put(i, (v = s->scriptValueForContext(context)));
        }
    }

    return context;
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

void hasJsOwnershipIndicator(QQmlGuardImpl *) {};

LoadHelper::LoadHelper(QQmlTypeLoader *loader, QAnyStringView uri)
    : QQmlTypeLoader::Blob({}, QQmlDataBlob::QmlFile, loader)
    , m_uri(uri.toString())

{
    auto import = std::make_shared<PendingImport>();
    import->uri = m_uri;
    QList<QQmlError> errorList;
    if (!Blob::addImport(import, &errorList)) {
        qCDebug(lcQmlImport) << "LoadHelper: Errors loading " << m_uri << errorList;
        m_uri.clear(); // reset m_uri to remember the failure
    }
}

LoadHelper::ResolveTypeResult LoadHelper::resolveType(QAnyStringView typeName)
{
    QQmlType type;
    if (!couldFindModule())
        return {ResolveTypeResult::NoSuchModule, type};
    QQmlTypeModule *module = QQmlMetaType::typeModule(m_uri, QTypeRevision{});
    if (module) {
        type = module->type(typeName.toString(), {});
        if (type.isValid())
            return {ResolveTypeResult::ModuleFound, type};
    }
    // The module exists (see check above), but there is no QQmlTypeModule
    // ==> pure QML module, attempt resolveType
    QTypeRevision versionReturn;
    QList<QQmlError> errors;
    QQmlImportNamespace *ns_return = nullptr;
    m_importCache->resolveType(typeName.toString(), &type, &versionReturn,
                               &ns_return,
                               &errors);
    return {ResolveTypeResult::ModuleFound, type};
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

#include "moc_qqmlengine_p.cpp"

#include "moc_qqmlengine.cpp"

