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

#ifndef QDECLARATIVEENGINE_P_H
#define QDECLARATIVEENGINE_P_H

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

#include "qdeclarativeengine.h"

#include "qdeclarativetypeloader_p.h"
#include "qdeclarativeimport_p.h"
#include <private/qpodvector_p.h>
#include "qdeclarative.h"
#include "qdeclarativevaluetype_p.h"
#include "qdeclarativecontext.h"
#include "qdeclarativecontext_p.h"
#include "qdeclarativeexpression.h"
#include "qdeclarativeimageprovider.h"
#include "qdeclarativeproperty_p.h"
#include "qdeclarativepropertycache_p.h"
#include "qdeclarativemetatype_p.h"
#include "qdeclarativedirparser_p.h"
#include <private/qintrusivelist_p.h>
#include <private/qrecyclepool_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qstack.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstring.h>
#include <QtCore/qthread.h>

#include <private/qobject_p.h>

#include <private/qv8engine_p.h>
#include <private/qjsengine_p.h>

QT_BEGIN_NAMESPACE

class QDeclarativeContext;
class QDeclarativeEngine;
class QDeclarativeContextPrivate;
class QDeclarativeExpression;
class QDeclarativeImportDatabase;
class QNetworkReply;
class QNetworkAccessManager;
class QDeclarativeNetworkAccessManagerFactory;
class QDeclarativeAbstractBinding;
class QDeclarativeTypeNameCache;
class QDeclarativeComponentAttached;
class QDeclarativeCleanup;
class QDeclarativeDelayedError;
class QDeclarativeWorkerScriptEngine;
class QDeclarativeVME;
class QDir;
class QDeclarativeIncubator;

// This needs to be declared here so that the pool for it can live in QDeclarativeEnginePrivate.
// The inline method definitions are in qdeclarativeexpression_p.h
class QDeclarativeJavaScriptExpressionGuard : public QDeclarativeNotifierEndpoint
{
public:
    inline QDeclarativeJavaScriptExpressionGuard(QDeclarativeJavaScriptExpression *);

    static inline void endpointCallback(QDeclarativeNotifierEndpoint *);
    static inline QDeclarativeJavaScriptExpressionGuard *New(QDeclarativeJavaScriptExpression *e);
    inline void Delete();

    QDeclarativeJavaScriptExpression *expression;
    QDeclarativeJavaScriptExpressionGuard *next;
};

class Q_DECLARATIVE_EXPORT QDeclarativeEnginePrivate : public QJSEnginePrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeEngine)
public:
    QDeclarativeEnginePrivate(QDeclarativeEngine *);
    ~QDeclarativeEnginePrivate();

    void init();

    class PropertyCapture {
    public:
        inline virtual ~PropertyCapture() {}
        virtual void captureProperty(QDeclarativeNotifier *) = 0;
        virtual void captureProperty(QObject *, int, int) = 0;
    };

    PropertyCapture *propertyCapture;
    inline void captureProperty(QDeclarativeNotifier *);
    inline void captureProperty(QObject *, int, int);

    QRecyclePool<QDeclarativeJavaScriptExpressionGuard> jsExpressionGuardPool;

    QDeclarativeContext *rootContext;
    bool isDebugging;

    bool outputWarningsToStdErr;

    QDeclarativeContextData *sharedContext;
    QObject *sharedScope;

    // Registered cleanup handlers
    QDeclarativeCleanup *cleanup;

    // Bindings that have had errors during startup
    QDeclarativeDelayedError *erroredBindings;
    int inProgressCreations;

    QV8Engine *v8engine() const { return q_func()->handle(); }

    QDeclarativeWorkerScriptEngine *getWorkerScriptEngine();
    QDeclarativeWorkerScriptEngine *workerScriptEngine;

    QUrl baseUrl;

    typedef QPair<QDeclarativeGuard<QObject>,int> FinalizeCallback;
    void registerFinalizeCallback(QObject *obj, int index);

    QDeclarativeVME *activeVME;

    QNetworkAccessManager *createNetworkAccessManager(QObject *parent) const;
    QNetworkAccessManager *getNetworkAccessManager() const;
    mutable QNetworkAccessManager *networkAccessManager;
    mutable QDeclarativeNetworkAccessManagerFactory *networkAccessManagerFactory;

    QHash<QString,QSharedPointer<QDeclarativeImageProvider> > imageProviders;
    QDeclarativeImageProvider::ImageType getImageProviderType(const QUrl &url);
    QDeclarativeTextureFactory *getTextureFromProvider(const QUrl &url, QSize *size, const QSize& req_size);
    QImage getImageFromProvider(const QUrl &url, QSize *size, const QSize& req_size);
    QPixmap getPixmapFromProvider(const QUrl &url, QSize *size, const QSize& req_size);

    // Scarce resources are "exceptionally high cost" QVariant types where allowing the
    // normal JavaScript GC to clean them up is likely to lead to out-of-memory or other
    // out-of-resource situations.  When such a resource is passed into JavaScript we
    // add it to the scarceResources list and it is destroyed when we return from the
    // JavaScript execution that created it.  The user can prevent this behavior by
    // calling preserve() on the object which removes it from this scarceResource list.
    class ScarceResourceData {
    public:
        ScarceResourceData(const QVariant &data) : data(data) {}
        QVariant data;
        QIntrusiveListNode node;
    };
    QIntrusiveList<ScarceResourceData, &ScarceResourceData::node> scarceResources;
    int scarceResourcesRefCount;
    void referenceScarceResources();
    void dereferenceScarceResources();

    QDeclarativeTypeLoader typeLoader;
    QDeclarativeImportDatabase importDatabase;

    QString offlineStoragePath;

    mutable quint32 uniqueId;
    inline quint32 getUniqueId() const {
        return uniqueId++;
    }

    QDeclarativeValueTypeFactory valueTypes;

    // Unfortunate workaround to avoid a circular dependency between 
    // qdeclarativeengine_p.h and qdeclarativeincubator_p.h
    struct Incubator {
        QIntrusiveListNode next;
        // Unfortunate workaround for MSVC
        QIntrusiveListNode nextWaitingFor;
    };
    QIntrusiveList<Incubator, &Incubator::next> incubatorList;
    unsigned int incubatorCount;
    QDeclarativeIncubationController *incubationController;
    void incubate(QDeclarativeIncubator &, QDeclarativeContextData *);

    // These methods may be called from any thread
    inline bool isEngineThread() const;
    inline static bool isEngineThread(const QDeclarativeEngine *);
    template<typename T>
    inline void deleteInEngineThread(T *);
    template<typename T>
    inline static void deleteInEngineThread(QDeclarativeEngine *, T *);

    // These methods may be called from the loader thread
    QDeclarativeMetaType::ModuleApiInstance *moduleApiInstance(const QDeclarativeMetaType::ModuleApi &module);

    // These methods may be called from the loader thread
    inline QDeclarativePropertyCache *cache(QObject *obj);
    inline QDeclarativePropertyCache *cache(const QMetaObject *);
    inline QDeclarativePropertyCache *cache(QDeclarativeType *, int, QDeclarativeError &error);

    // These methods may be called from the loader thread
    bool isQObject(int);
    QObject *toQObject(const QVariant &, bool *ok = 0) const;
    QDeclarativeMetaType::TypeCategory typeCategory(int) const;
    bool isList(int) const;
    int listType(int) const;
    const QMetaObject *rawMetaObjectForType(int) const;
    const QMetaObject *metaObjectForType(int) const;
    void registerCompositeType(QDeclarativeCompiledData *);

    void sendQuit();
    void warning(const QDeclarativeError &);
    void warning(const QList<QDeclarativeError> &);
    static void warning(QDeclarativeEngine *, const QDeclarativeError &);
    static void warning(QDeclarativeEngine *, const QList<QDeclarativeError> &);
    static void warning(QDeclarativeEnginePrivate *, const QDeclarativeError &);
    static void warning(QDeclarativeEnginePrivate *, const QList<QDeclarativeError> &);

    inline static QV8Engine *getV8Engine(QDeclarativeEngine *e);
    inline static QDeclarativeEnginePrivate *get(QDeclarativeEngine *e);
    inline static const QDeclarativeEnginePrivate *get(const QDeclarativeEngine *e);
    inline static QDeclarativeEnginePrivate *get(QDeclarativeContext *c);
    inline static QDeclarativeEnginePrivate *get(QDeclarativeContextData *c);
    inline static QDeclarativeEngine *get(QDeclarativeEnginePrivate *p);

    static QString urlToLocalFileOrQrc(const QUrl& url);
    static QString urlToLocalFileOrQrc(const QString& url);

    static void registerBaseTypes(const char *uri, int versionMajor, int versionMinor);
    static void defineModule();

    static bool qml_debugging_enabled;

    mutable QMutex mutex;

private:
    // Locker locks the QDeclarativeEnginePrivate data structures for read and write, if necessary.  
    // Currently, locking is only necessary if the threaded loader is running concurrently.  If it is 
    // either idle, or is running with the main thread blocked, no locking is necessary.  This way
    // we only pay for locking when we have to.
    // Consequently, this class should only be used to protect simple accesses or modifications of the 
    // QDeclarativeEnginePrivate structures or operations that can be guarenteed not to start activity
    // on the loader thread.
    // The Locker API is identical to QMutexLocker.  Locker reuses the QDeclarativeEnginePrivate::mutex 
    // QMutex instance and multiple Lockers are recursive in the same thread.
    class Locker 
    {
    public:
        inline Locker(const QDeclarativeEngine *);
        inline Locker(const QDeclarativeEnginePrivate *);
        inline ~Locker();

        inline void unlock();
        inline void relock();

    private:
        const QDeclarativeEnginePrivate *m_ep;
        quint32 m_locked:1;
    };

    // Must be called locked
    QDeclarativePropertyCache *createCache(const QMetaObject *);
    QDeclarativePropertyCache *createCache(QDeclarativeType *, int, QDeclarativeError &error);

    // These members must be protected by a QDeclarativeEnginePrivate::Locker as they are required by
    // the threaded loader.  Only access them through their respective accessor methods.
    QHash<QDeclarativeMetaType::ModuleApi, QDeclarativeMetaType::ModuleApiInstance *> moduleApiInstances;
    QHash<const QMetaObject *, QDeclarativePropertyCache *> propertyCache;
    QHash<QPair<QDeclarativeType *, int>, QDeclarativePropertyCache *> typePropertyCache;
    QHash<int, int> m_qmlLists;
    QHash<int, QDeclarativeCompiledData *> m_compositeTypes;

    // These members is protected by the full QDeclarativeEnginePrivate::mutex mutex
    struct Deletable { Deletable():next(0) {} virtual ~Deletable() {} Deletable *next; };
    QFieldList<Deletable, &Deletable::next> toDeleteInEngineThread;
    void doDeleteInEngineThread();
};

QDeclarativeEnginePrivate::Locker::Locker(const QDeclarativeEngine *e)
: m_ep(QDeclarativeEnginePrivate::get(e))
{
    relock();
}

QDeclarativeEnginePrivate::Locker::Locker(const QDeclarativeEnginePrivate *e)
: m_ep(e), m_locked(false)
{
    relock();
}

QDeclarativeEnginePrivate::Locker::~Locker()
{
    unlock();
}

void QDeclarativeEnginePrivate::Locker::unlock()
{
    if (m_locked) { 
        m_ep->mutex.unlock();
        m_locked = false;
    }
}

void QDeclarativeEnginePrivate::Locker::relock()
{
    Q_ASSERT(!m_locked);
    if (m_ep->typeLoader.isConcurrent()) {
        m_ep->mutex.lock();
        m_locked = true;
    }
}

/*!
Returns true if the calling thread is the QDeclarativeEngine thread.
*/
bool QDeclarativeEnginePrivate::isEngineThread() const
{
    Q_Q(const QDeclarativeEngine);
    return QThread::currentThread() == q->thread();
}

/*!
Returns true if the calling thread is the QDeclarativeEngine \a engine thread.
*/
bool QDeclarativeEnginePrivate::isEngineThread(const QDeclarativeEngine *engine)
{
    Q_ASSERT(engine);
    return QDeclarativeEnginePrivate::get(engine)->isEngineThread();
}

/*!
Delete \a value in the engine thread.  If the calling thread is the engine
thread, \a value will be deleted immediately.

This method should be used for *any* type that has resources that need to
be freed in the engine thread.  This is generally types that use V8 handles.
As there is some small overhead in checking the current thread, it is best
practice to check if any V8 handles actually need to be freed and delete 
the instance directly if not.
*/
template<typename T>
void QDeclarativeEnginePrivate::deleteInEngineThread(T *value)
{
    Q_Q(QDeclarativeEngine);

    Q_ASSERT(value);
    if (isEngineThread()) {
        delete value;
    } else { 
        struct I : public Deletable {
            I(T *value) : value(value) {}
            ~I() { delete value; }
            T *value;
        };
        I *i = new I(value);
        mutex.lock();
        bool wasEmpty = toDeleteInEngineThread.isEmpty();
        toDeleteInEngineThread.append(i);
        mutex.unlock();
        if (wasEmpty)
            QCoreApplication::postEvent(q, new QEvent(QEvent::User));
    }
}

/*!
Delete \a value in the \a engine thread.  If the calling thread is the engine
thread, \a value will be deleted immediately.
*/
template<typename T>
void QDeclarativeEnginePrivate::deleteInEngineThread(QDeclarativeEngine *engine, T *value)
{
    Q_ASSERT(engine);
    QDeclarativeEnginePrivate::get(engine)->deleteInEngineThread<T>(value);
}

/*!
Returns a QDeclarativePropertyCache for \a obj if one is available.

If \a obj is null, being deleted or contains a dynamic meta object 0
is returned.

The returned cache is not referenced, so if it is to be stored, call addref().

XXX thread There is a potential future race condition in this and all the cache()
functions.  As the QDeclarativePropertyCache is returned unreferenced, when called 
from the loader thread, it is possible that the cache will have been dereferenced 
and deleted before the loader thread has a chance to use or reference it.  This
can't currently happen as the cache holds a reference to the 
QDeclarativePropertyCache until the QDeclarativeEngine is destroyed.
*/
QDeclarativePropertyCache *QDeclarativeEnginePrivate::cache(QObject *obj)
{
    if (!obj || QObjectPrivate::get(obj)->metaObject || QObjectPrivate::get(obj)->wasDeleted)
        return 0;

    Locker locker(this);
    const QMetaObject *mo = obj->metaObject();
    QDeclarativePropertyCache *rv = propertyCache.value(mo);
    if (!rv) rv = createCache(mo);
    return rv;
}

/*!
Returns a QDeclarativePropertyCache for \a metaObject.

As the cache is persisted for the life of the engine, \a metaObject must be
a static "compile time" meta-object, or a meta-object that is otherwise known to
exist for the lifetime of the QDeclarativeEngine.

The returned cache is not referenced, so if it is to be stored, call addref().
*/
QDeclarativePropertyCache *QDeclarativeEnginePrivate::cache(const QMetaObject *metaObject)
{
    Q_ASSERT(metaObject);

    Locker locker(this);
    QDeclarativePropertyCache *rv = propertyCache.value(metaObject);
    if (!rv) rv = createCache(metaObject);
    return rv;
}

/*!
Returns a QDeclarativePropertyCache for \a type with \a minorVersion.

The returned cache is not referenced, so if it is to be stored, call addref().
*/
QDeclarativePropertyCache *QDeclarativeEnginePrivate::cache(QDeclarativeType *type, int minorVersion, QDeclarativeError &error)
{
    Q_ASSERT(type);

    if (minorVersion == -1 || !type->containsRevisionedAttributes())
        return cache(type->metaObject());

    Locker locker(this);
    QDeclarativePropertyCache *rv = typePropertyCache.value(qMakePair(type, minorVersion));
    if (!rv) rv = createCache(type, minorVersion, error);
    return rv;
}

QV8Engine *QDeclarativeEnginePrivate::getV8Engine(QDeclarativeEngine *e) 
{ 
    return e->d_func()->v8engine(); 
}

QDeclarativeEnginePrivate *QDeclarativeEnginePrivate::get(QDeclarativeEngine *e) 
{ 
    return e->d_func(); 
}

const QDeclarativeEnginePrivate *QDeclarativeEnginePrivate::get(const QDeclarativeEngine *e) 
{ 
    return e->d_func(); 
}

QDeclarativeEnginePrivate *QDeclarativeEnginePrivate::get(QDeclarativeContext *c) 
{ 
    return (c && c->engine()) ? QDeclarativeEnginePrivate::get(c->engine()) : 0; 
}

QDeclarativeEnginePrivate *QDeclarativeEnginePrivate::get(QDeclarativeContextData *c) 
{ 
    return (c && c->engine) ? QDeclarativeEnginePrivate::get(c->engine) : 0; 
}

QDeclarativeEngine *QDeclarativeEnginePrivate::get(QDeclarativeEnginePrivate *p) 
{ 
    return p->q_func(); 
}

void QDeclarativeEnginePrivate::captureProperty(QDeclarativeNotifier *n)
{
    if (propertyCapture)
        propertyCapture->captureProperty(n);
}

void QDeclarativeEnginePrivate::captureProperty(QObject *o, int c, int n)
{
    if (propertyCapture)
        propertyCapture->captureProperty(o, c, n);
}

QT_END_NAMESPACE

#endif // QDECLARATIVEENGINE_P_H
