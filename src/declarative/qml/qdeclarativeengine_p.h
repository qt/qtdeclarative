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

#include "private/qdeclarativetypeloader_p.h"
#include "private/qdeclarativeimport_p.h"
#include "private/qpodvector_p.h"
#include "qdeclarative.h"
#include "private/qdeclarativevaluetype_p.h"
#include "qdeclarativecontext.h"
#include "private/qdeclarativecontext_p.h"
#include "qdeclarativeexpression.h"
#include "qdeclarativeimageprovider.h"
#include "private/qdeclarativeproperty_p.h"
#include "private/qdeclarativepropertycache_p.h"
#include "private/qdeclarativemetatype_p.h"
#include "private/qdeclarativedirparser_p.h"
#include "private/qintrusivelist_p.h"

#include <QtScript/QScriptValue>
#include <QtScript/QScriptString>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qstack.h>
#include <QtCore/qmutex.h>
#include <QtScript/qscriptengine.h>

#include <private/qobject_p.h>

#include <private/qv8engine_p.h>

QT_BEGIN_NAMESPACE

class QDeclarativeContext;
class QDeclarativeEngine;
class QDeclarativeContextPrivate;
class QDeclarativeExpression;
class QDeclarativeImportDatabase;
class ScarceResourceData;
class QScriptEngineDebugger;
class QNetworkReply;
class QNetworkAccessManager;
class QDeclarativeNetworkAccessManagerFactory;
class QDeclarativeAbstractBinding;
class QScriptDeclarativeClass;
class QDeclarativeTypeNameCache;
class QDeclarativeComponentAttached;
class QDeclarativeCleanup;
class QDeclarativeDelayedError;
class QDeclarativeWorkerScriptEngine;
class QDir;
class QSGTexture;
class QSGContext;

class QDeclarativeScriptEngine : public QScriptEngine
{
public:
    QDeclarativeScriptEngine(QDeclarativeEnginePrivate *priv);
    virtual ~QDeclarativeScriptEngine();

    static QDeclarativeScriptEngine *get(QScriptEngine* e) { return static_cast<QDeclarativeScriptEngine*>(e); }

    QDeclarativeEnginePrivate *p;

    QUrl baseUrl;

    virtual QNetworkAccessManager *networkAccessManager();
};

class Q_AUTOTEST_EXPORT QDeclarativeEnginePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeEngine)
public:
    QDeclarativeEnginePrivate(QDeclarativeEngine *);
    ~QDeclarativeEnginePrivate();

    void init();

    struct CapturedProperty {
        CapturedProperty(QObject *o, int c, int n)
            : object(o), coreIndex(c), notifier(0), notifyIndex(n) {}
        CapturedProperty(QDeclarativeNotifier *n)
            : object(0), coreIndex(-1), notifier(n), notifyIndex(-1) {}

        QObject *object;
        int coreIndex;
        QDeclarativeNotifier *notifier;
        int notifyIndex;
    };
    bool captureProperties;
    QPODVector<CapturedProperty> capturedProperties;

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

    // V8 Engine
    QV8Engine v8engine;

    QDeclarativeScriptEngine scriptEngine;

    QDeclarativeWorkerScriptEngine *getWorkerScriptEngine();
    QDeclarativeWorkerScriptEngine *workerScriptEngine;

    QUrl baseUrl;

    template<class T>
    struct SimpleList {
        SimpleList()
            : count(0), values(0) {}
        SimpleList(int r)
            : count(0), values(new T*[r]) {}

        int count;
        T **values;

        void append(T *v) {
            values[count++] = v;
        }

        T *at(int idx) const {
            return values[idx];
        }

        void clear() {
            delete [] values;
        }
    };

    static void clear(SimpleList<QDeclarativeAbstractBinding> &);
    static void clear(SimpleList<QDeclarativeParserStatus> &);

    QList<SimpleList<QDeclarativeAbstractBinding> > bindValues;
    QList<SimpleList<QDeclarativeParserStatus> > parserStatus;
    QList<QPair<QDeclarativeGuard<QObject>,int> > finalizedParserStatus;
    QDeclarativeComponentAttached *componentAttached;

    void registerFinalizedParserStatusObject(QObject *obj, int index) {
        finalizedParserStatus.append(qMakePair(QDeclarativeGuard<QObject>(obj), index));
    }

    bool inBeginCreate;

    QNetworkAccessManager *createNetworkAccessManager(QObject *parent) const;
    QNetworkAccessManager *getNetworkAccessManager() const;
    mutable QNetworkAccessManager *networkAccessManager;
    mutable QDeclarativeNetworkAccessManagerFactory *networkAccessManagerFactory;

    QHash<QString,QSharedPointer<QDeclarativeImageProvider> > imageProviders;
    QDeclarativeImageProvider::ImageType getImageProviderType(const QUrl &url);
    QSGTexture *getTextureFromProvider(const QUrl &url, QSize *size, const QSize& req_size);
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

    mutable QMutex mutex;

    QDeclarativeTypeLoader typeLoader;
    QDeclarativeImportDatabase importDatabase;

    QString offlineStoragePath;

    mutable quint32 uniqueId;
    quint32 getUniqueId() const {
        return uniqueId++;
    }

    QDeclarativeValueTypeFactory valueTypes;

    QHash<QDeclarativeMetaType::ModuleApi, QDeclarativeMetaType::ModuleApiInstance *> moduleApiInstances;

    QHash<const QMetaObject *, QDeclarativePropertyCache *> propertyCache;
    QHash<QPair<QDeclarativeType *, int>, QDeclarativePropertyCache *> typePropertyCache;
    inline QDeclarativePropertyCache *cache(QObject *obj);
    inline QDeclarativePropertyCache *cache(const QMetaObject *);
    inline QDeclarativePropertyCache *cache(QDeclarativeType *, int, QDeclarativeError &error);
    QDeclarativePropertyCache *createCache(const QMetaObject *);
    QDeclarativePropertyCache *createCache(QDeclarativeType *, int, QDeclarativeError &error);

    void registerCompositeType(QDeclarativeCompiledData *);

    bool isQObject(int);
    QObject *toQObject(const QVariant &, bool *ok = 0) const;
    QDeclarativeMetaType::TypeCategory typeCategory(int) const;
    bool isList(int) const;
    int listType(int) const;
    const QMetaObject *rawMetaObjectForType(int) const;
    const QMetaObject *metaObjectForType(int) const;
    QHash<int, int> m_qmlLists;
    QHash<int, QDeclarativeCompiledData *> m_compositeTypes;

    void sendQuit();
    void warning(const QDeclarativeError &);
    void warning(const QList<QDeclarativeError> &);
    static void warning(QDeclarativeEngine *, const QDeclarativeError &);
    static void warning(QDeclarativeEngine *, const QList<QDeclarativeError> &);
    static void warning(QDeclarativeEnginePrivate *, const QDeclarativeError &);
    static void warning(QDeclarativeEnginePrivate *, const QList<QDeclarativeError> &);

    static QV8Engine *getV8Engine(QDeclarativeEngine *e) { return &e->d_func()->v8engine; }
    static QScriptEngine *getScriptEngine(QDeclarativeEngine *e) { return &e->d_func()->scriptEngine; }
    static QDeclarativeEngine *getEngine(QScriptEngine *e) { return static_cast<QDeclarativeScriptEngine*>(e)->p->q_func(); }
    static QDeclarativeEnginePrivate *get(QDeclarativeEngine *e) { return e->d_func(); }
    static QDeclarativeEnginePrivate *get(QDeclarativeContext *c) { return (c && c->engine()) ? QDeclarativeEnginePrivate::get(c->engine()) : 0; }
    static QDeclarativeEnginePrivate *get(QDeclarativeContextData *c) { return (c && c->engine) ? QDeclarativeEnginePrivate::get(c->engine) : 0; }
    static QDeclarativeEnginePrivate *get(QScriptEngine *e) { return static_cast<QDeclarativeScriptEngine*>(e)->p; }
    static QDeclarativeEngine *get(QDeclarativeEnginePrivate *p) { return p->q_func(); }

    static QString urlToLocalFileOrQrc(const QUrl& url);

    static void defineModule();

    static bool qml_debugging_enabled;

    QSGContext *sgContext;
};

/*!
Returns a QDeclarativePropertyCache for \a obj if one is available.

If \a obj is null, being deleted or contains a dynamic meta object 0
is returned.

The returned cache is not referenced, so if it is to be stored, call addref().
*/
QDeclarativePropertyCache *QDeclarativeEnginePrivate::cache(QObject *obj)
{
    if (!obj || QObjectPrivate::get(obj)->metaObject || QObjectPrivate::get(obj)->wasDeleted)
        return 0;

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

    QDeclarativePropertyCache *rv = typePropertyCache.value(qMakePair(type, minorVersion));
    if (!rv) rv = createCache(type, minorVersion, error);
    return rv;
}

QT_END_NAMESPACE

#endif // QDECLARATIVEENGINE_P_H
