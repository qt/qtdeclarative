/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLV8ENGINE_P_H
#define QQMLV8ENGINE_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <QtCore/qset.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstack.h>
#include <QtCore/qstringlist.h>
#include <QtCore/QElapsedTimer>
#include <QtCore/QThreadStorage>

#include <private/qv8_p.h>
#include <qjsengine.h>
#include <qjsvalue.h>
#include "qjsvalue_p.h"
#include "qjsvalueiterator_p.h"
#include "qscriptoriginalglobalobject_p.h"
#include "qscripttools_p.h"

#include <private/qqmlpropertycache_p.h>

#include "qv8objectresource_p.h"
#include "qv8contextwrapper_p.h"
#include "qv8qobjectwrapper_p.h"
#include "qv8stringwrapper_p.h"
#include "qv8typewrapper_p.h"
#include "qv8listwrapper_p.h"
#include "qv8variantwrapper_p.h"
#include "qv8valuetypewrapper_p.h"
#include "qv8sequencewrapper_p.h"
#include "qv8jsonwrapper_p.h"

namespace v8 {

// Needed for V8ObjectSet
inline uint qHash(const v8::Handle<v8::Object> &object, uint seed = 0)
{
    return (object->GetIdentityHash() ^ seed);
}

}

QT_BEGIN_NAMESPACE


// Uncomment the following line to enable global handle debugging.  When enabled, all the persistent
// handles allocated using qPersistentNew() (or registered with qPersistentRegsiter()) and disposed
// with qPersistentDispose() are tracked.  If you try and do something illegal, like double disposing
// a handle, qFatal() is called.
// #define QML_GLOBAL_HANDLE_DEBUGGING

#define V8ENGINE() ((QV8Engine *)v8::External::Cast(*args.Data())->Value())
#define V8FUNCTION(function, engine) v8::FunctionTemplate::New(function, v8::External::New((QV8Engine*)engine))->GetFunction()
#define V8THROW_ERROR(string) { \
    v8::ThrowException(v8::Exception::Error(v8::String::New(string))); \
    return v8::Handle<v8::Value>(); \
}
#define V8THROW_TYPE(string) { \
    v8::ThrowException(v8::Exception::TypeError(v8::String::New(string))); \
    return v8::Handle<v8::Value>(); \
}
#define V8ENGINE_ACCESSOR() ((QV8Engine *)v8::External::Cast(*info.Data())->Value());
#define V8THROW_ERROR_SETTER(string) { \
    v8::ThrowException(v8::Exception::Error(v8::String::New(string))); \
    return; \
}

#define V8ASSERT_TYPE(condition, string) \
    if (!(condition)) { \
        v8::ThrowException(v8::Exception::TypeError(v8::String::New(string))); \
        return v8::Handle<v8::Value>(); \
    }
#define V8ASSERT_TYPE_SETTER(condition, string) \
    if (!(condition)) { \
        v8::ThrowException(v8::Exception::TypeError(v8::String::New(string))); \
        return; \
    }

#define V8_DEFINE_EXTENSION(dataclass, datafunction) \
    static inline dataclass *datafunction(QV8Engine *engine) \
    { \
        static int extensionId = -1; \
        if (extensionId == -1) { \
            QV8Engine::registrationMutex()->lock(); \
            if (extensionId == -1) \
                extensionId = QV8Engine::registerExtension(); \
            QV8Engine::registrationMutex()->unlock(); \
        } \
        dataclass *rv = (dataclass *)engine->extensionData(extensionId); \
        if (!rv) { \
            rv = new dataclass(engine); \
            engine->setExtensionData(extensionId, rv); \
        } \
        return rv; \
    } \

template<class T>
inline T *v8_resource_cast(v8::Handle<v8::Object> object) {
    QV8ObjectResource *resource = static_cast<QV8ObjectResource *>(object->GetExternalResource());
    return (resource && (quint32)resource->resourceType() == (quint32)T::V8ResourceType)?static_cast<T *>(resource):0;
}

template<class T>
inline T *v8_resource_check(v8::Handle<v8::Object> object) {
    T *resource = static_cast<T *>(object->GetExternalResource());
    Q_ASSERT(resource && resource->resourceType() == (quint32)T::V8ResourceType);
    return resource;
}

// Used to allow a QObject method take and return raw V8 handles without having to expose
// v8 in the public API.
// Use like this:
//     class MyClass : public QObject {
//         Q_OBJECT
//         ...
//         Q_INVOKABLE void myMethod(QQmlV8Function*);
//     };
// The QQmlV8Function - and consequently the arguments and return value - only remains 
// valid during the call.  If the return value isn't set within myMethod(), the will return
// undefined.
class QV8Engine;
class QQmlV8Function
{
public:
    int Length() const { return _ac; }
    v8::Local<v8::Value> operator[](int idx) { return (*_a)->Get(idx); }
    QQmlContextData *context() { return _c; }
    v8::Handle<v8::Object> qmlGlobal() { return *_g; }
    void returnValue(v8::Handle<v8::Value> rv) { *_r = rv; }
    QV8Engine *engine() const { return _e; }
private:
    friend class QV8QObjectWrapper;
    QQmlV8Function();
    QQmlV8Function(const QQmlV8Function &);
    QQmlV8Function &operator=(const QQmlV8Function &);

    QQmlV8Function(int length, v8::Handle<v8::Object> &args, 
                           v8::Handle<v8::Value> &rv, v8::Handle<v8::Object> &global,
                           QQmlContextData *c, QV8Engine *e)
    : _ac(length), _a(&args), _r(&rv), _g(&global), _c(c), _e(e) {}

    int _ac;
    v8::Handle<v8::Object> *_a;
    v8::Handle<v8::Value> *_r;
    v8::Handle<v8::Object> *_g;
    QQmlContextData *_c;
    QV8Engine *_e;
};

class QQmlV8Handle
{
public:
    QQmlV8Handle() : d(0) {}
    QQmlV8Handle(const QQmlV8Handle &other) : d(other.d) {}
    QQmlV8Handle &operator=(const QQmlV8Handle &other) { d = other.d; return *this; }

    static QQmlV8Handle fromHandle(v8::Handle<v8::Value> h) {
        return QQmlV8Handle(*h);
    }
    v8::Handle<v8::Value> toHandle() const {
        return v8::Handle<v8::Value>((v8::Value *)d);
    }
private:
    QQmlV8Handle(void *d) : d(d) {}
    void *d;
};

class QObject;
class QQmlEngine;
class QQmlValueType;
class QNetworkAccessManager;
class QQmlContextData;

class Q_AUTOTEST_EXPORT QV8GCCallback
{
private:
    class ThreadData;
public:
    static void garbageCollectorPrologueCallback(v8::GCType, v8::GCCallbackFlags);
    static void registerGcPrologueCallback();

    class Q_AUTOTEST_EXPORT Node {
    public:
        typedef void (*PrologueCallback)(Node *node);
        Node(PrologueCallback callback);
        ~Node();

        QIntrusiveListNode node;
        PrologueCallback prologueCallback;
    };

    static void addGcCallbackNode(Node *node);
};

class Q_QML_PRIVATE_EXPORT QV8Engine
{
    typedef QSet<v8::Handle<v8::Object> > V8ObjectSet;
public:
    static QV8Engine* get(QJSEngine* q) { Q_ASSERT(q); return q->handle(); }
    static QJSEngine* get(QV8Engine* d) { Q_ASSERT(d); return d->q; }

    enum ContextOwnership {
        AdoptCurrentContext,
        CreateNewContext
    };
    QV8Engine(QJSEngine* qq, ContextOwnership ownership = CreateNewContext);
    virtual ~QV8Engine();

    // This enum should be in sync with QQmlEngine::ObjectOwnership
    enum ObjectOwnership { CppOwnership, JavaScriptOwnership };

    struct Deletable {
        virtual ~Deletable() {}
    };

    void initQmlGlobalObject();
    void setEngine(QQmlEngine *engine);
    QQmlEngine *engine() { return m_engine; }
    v8::Local<v8::Object> global() { return m_context->Global(); }
    v8::Handle<v8::Context> context() const { return m_context; }

    inline void registerValue(QJSValuePrivate *data);
    inline void unregisterValue(QJSValuePrivate *data);
    inline void invalidateAllValues();

    inline void registerValueIterator(QJSValueIteratorPrivate *data);
    inline void unregisterValueIterator(QJSValueIteratorPrivate *data);
    inline void invalidateAllIterators();

    QV8ContextWrapper *contextWrapper() { return &m_contextWrapper; }
    QV8QObjectWrapper *qobjectWrapper() { return &m_qobjectWrapper; }
    QV8TypeWrapper *typeWrapper() { return &m_typeWrapper; }
    QV8ListWrapper *listWrapper() { return &m_listWrapper; }
    QV8VariantWrapper *variantWrapper() { return &m_variantWrapper; }
    QV8ValueTypeWrapper *valueTypeWrapper() { return &m_valueTypeWrapper; }
    QV8SequenceWrapper *sequenceWrapper() { return &m_sequenceWrapper; }

    void *xmlHttpRequestData() { return m_xmlHttpRequestData; }

    Deletable *listModelData() { return m_listModelData; }
    void setListModelData(Deletable *d) { if (m_listModelData) delete m_listModelData; m_listModelData = d; }

    QQmlContextData *callingContext();

    v8::Local<v8::Array> getOwnPropertyNames(v8::Handle<v8::Object>);
    inline QJSValuePrivate::PropertyFlags getPropertyFlags(v8::Handle<v8::Object> object, v8::Handle<v8::Value> property);
    void freezeObject(v8::Handle<v8::Value>);

    inline QString toString(v8::Handle<v8::Value> string);
    inline QString toString(v8::Handle<v8::String> string);
    static QString toStringStatic(v8::Handle<v8::Value>);
    static QString toStringStatic(v8::Handle<v8::String>);
    static inline bool startsWithUpper(v8::Handle<v8::String>);

    QVariant toVariant(v8::Handle<v8::Value>, int typeHint);
    v8::Handle<v8::Value> fromVariant(const QVariant &);
    inline bool isVariant(v8::Handle<v8::Value>);

    // Compile \a source (from \a fileName at \a lineNumber) in QML mode
    v8::Local<v8::Script> qmlModeCompile(const QString &source, 
                                         const QString &fileName = QString(), 
                                         quint16 lineNumber = 1);
    v8::Local<v8::Script> qmlModeCompile(const char *source, int sourceLength = -1,
                                         const QString &fileName = QString(),
                                         quint16 lineNumber = 1);

    // Return the QML global "scope" object for the \a ctxt context and \a scope object.
    inline v8::Local<v8::Object> qmlScope(QQmlContextData *ctxt, QObject *scope);

    // Return a JS wrapper for the given QObject \a object
    inline v8::Handle<v8::Value> newQObject(QObject *object);
    inline v8::Handle<v8::Value> newQObject(QObject *object, const ObjectOwnership ownership);
    inline bool isQObject(v8::Handle<v8::Value>);
    inline QObject *toQObject(v8::Handle<v8::Value>);

    // Return a JS string for the given QString \a string
    inline v8::Local<v8::String> toString(const QString &string);

    // Create a new value type object
    inline v8::Handle<v8::Value> newValueType(QObject *, int coreIndex, QQmlValueType *);
    inline v8::Handle<v8::Value> newValueType(const QVariant &, QQmlValueType *);
    inline bool isValueType(v8::Handle<v8::Value>) const;
    inline QVariant toValueType(v8::Handle<v8::Value> obj);

    // Create a new sequence type object
    inline v8::Handle<v8::Value> newSequence(int sequenceType, QObject *, int coreIndex, bool *succeeded);

    // Create a new QVariant object.  This doesn't examine the type of the variant, but always returns
    // a QVariant wrapper
    inline v8::Handle<v8::Value> newQVariant(const QVariant &);

    // Return the JS string key for the "function is a binding" flag
    inline v8::Handle<v8::String> bindingFlagKey() const;

    // Return the network access manager for this engine.  By default this returns the network
    // access manager of the QQmlEngine.  It is overridden in the case of a threaded v8
    // instance (like in WorkerScript).
    virtual QNetworkAccessManager *networkAccessManager();

    // Return the list of illegal id names (the names of the properties on the global object)
    const QStringHash<bool> &illegalNames() const;

    inline void collectGarbage() { gc(); }
    static void gc();

    v8::Handle<v8::Value> throwException(v8::Handle<v8::Value> value);

#ifdef QML_GLOBAL_HANDLE_DEBUGGING
    // Used for handle debugging
    static void registerHandle(void *);
    static void releaseHandle(void *);
#endif

    static QMutex *registrationMutex();
    static int registerExtension();

    inline Deletable *extensionData(int) const;
    void setExtensionData(int, Deletable *);

    inline v8::Handle<v8::Value> makeJSValue(bool value);
    inline v8::Local<v8::Value> makeJSValue(int value);
    inline v8::Local<v8::Value> makeJSValue(uint value);
    inline v8::Local<v8::Value> makeJSValue(double value);
    inline v8::Handle<v8::Value> makeJSValue(QJSValue::SpecialValue value);
    inline v8::Local<v8::Value> makeJSValue(const QString &value);

    inline QScriptPassPointer<QJSValuePrivate> evaluate(const QString &program, const QString &fileName = QString(), quint16 lineNumber = 1);
    QScriptPassPointer<QJSValuePrivate> evaluate(v8::Handle<v8::Script> script, v8::TryCatch& tryCatch);

    QScriptPassPointer<QJSValuePrivate> newArray(uint length);
    v8::Local<v8::Object> newVariant(const QVariant &variant);

    v8::Local<v8::Array> variantListToJS(const QVariantList &lst);
    inline QVariantList variantListFromJS(v8::Handle<v8::Array> jsArray)
    { V8ObjectSet visitedObjects; return variantListFromJS(jsArray, visitedObjects); }

    v8::Local<v8::Object> variantMapToJS(const QVariantMap &vmap);
    inline QVariantMap variantMapFromJS(v8::Handle<v8::Object> jsObject)
    { V8ObjectSet visitedObjects; return variantMapFromJS(jsObject, visitedObjects); }

    v8::Handle<v8::Value> variantToJS(const QVariant &value);
    inline QVariant variantFromJS(v8::Handle<v8::Value> value)
    { V8ObjectSet visitedObjects; return variantFromJS(value, visitedObjects); }

    v8::Handle<v8::Value> jsonValueToJS(const QJsonValue &value);
    QJsonValue jsonValueFromJS(v8::Handle<v8::Value> value);
    v8::Local<v8::Object> jsonObjectToJS(const QJsonObject &object);
    QJsonObject jsonObjectFromJS(v8::Handle<v8::Value> value);
    v8::Local<v8::Array> jsonArrayToJS(const QJsonArray &array);
    QJsonArray jsonArrayFromJS(v8::Handle<v8::Value> value);

    v8::Handle<v8::Value> metaTypeToJS(int type, const void *data);
    bool metaTypeFromJS(v8::Handle<v8::Value> value, int type, void *data);

    bool convertToNativeQObject(v8::Handle<v8::Value> value,
                                const QByteArray &targetType,
                                void **result);

    QVariant &variantValue(v8::Handle<v8::Value> value);

    QJSValue scriptValueFromInternal(v8::Handle<v8::Value>) const;

    // used for console.time(), console.timeEnd()
    void startTimer(const QString &timerName);
    qint64 stopTimer(const QString &timerName, bool *wasRunning);

    // used for console.count()
    int consoleCountHelper(const QString &file, quint16 line, quint16 column);

    QObject *qtObjectFromJS(v8::Handle<v8::Value> value);

    static QDateTime qtDateTimeFromJsDate(double jsDate);

    void addRelationshipForGC(QObject *object, v8::Persistent<v8::Value> handle);
    void addRelationshipForGC(QObject *object, QObject *other);

    static v8::Handle<v8::Value> getPlatform(v8::Local<v8::String> property, const v8::AccessorInfo &info);
    static v8::Handle<v8::Value> getApplication(v8::Local<v8::String> property, const v8::AccessorInfo &info);
#ifndef QT_NO_IM
    static v8::Handle<v8::Value> getInputMethod(v8::Local<v8::String> property, const v8::AccessorInfo &info);
#endif

    struct ThreadData {
        ThreadData();
        ~ThreadData();
        v8::Isolate* isolate;
        bool gcPrologueCallbackRegistered;
        QIntrusiveList<QV8GCCallback::Node, &QV8GCCallback::Node::node> gcCallbackNodes;
    };

    static bool hasThreadData();
    static ThreadData* threadData();
    static void ensurePerThreadIsolate();

    v8::Persistent<v8::Object> m_strongReferencer;

protected:
    QJSEngine* q;
    QQmlEngine *m_engine;
    bool m_ownsV8Context;
    v8::Persistent<v8::Context> m_context;
    QScriptOriginalGlobalObject m_originalGlobalObject;

    v8::Persistent<v8::String> m_bindingFlagKey;

    QV8StringWrapper m_stringWrapper;
    QV8ContextWrapper m_contextWrapper;
    QV8QObjectWrapper m_qobjectWrapper;
    QV8TypeWrapper m_typeWrapper;
    QV8ListWrapper m_listWrapper;
    QV8VariantWrapper m_variantWrapper;
    QV8ValueTypeWrapper m_valueTypeWrapper;
    QV8SequenceWrapper m_sequenceWrapper;
    QV8JsonWrapper m_jsonWrapper;

    v8::Persistent<v8::Function> m_getOwnPropertyNames;
    v8::Persistent<v8::Function> m_freezeObject;

    void *m_xmlHttpRequestData;

    QVector<Deletable *> m_extensionData;
    Deletable *m_listModelData;

    QStringHash<bool> m_illegalNames;

    QElapsedTimer m_time;
    QHash<QString, qint64> m_startedTimers;

    QHash<QString, quint32> m_consoleCount;

    QObject *m_platform;
    QObject *m_application;

    QVariant toBasicVariant(v8::Handle<v8::Value>);

    void initializeGlobal(v8::Handle<v8::Object>);

    double qtDateTimeToJsDate(const QDateTime &dt);

private:
    QVariantList variantListFromJS(v8::Handle<v8::Array> jsArray, V8ObjectSet &visitedObjects);
    QVariantMap variantMapFromJS(v8::Handle<v8::Object> jsObject, V8ObjectSet &visitedObjects);
    QVariant variantFromJS(v8::Handle<v8::Value> value, V8ObjectSet &visitedObjects);

    static v8::Persistent<v8::Object> *findOwnerAndStrength(QObject *object, bool *shouldBeStrong);

    typedef QScriptIntrusiveList<QJSValuePrivate, &QJSValuePrivate::m_node> ValueList;
    ValueList m_values;
    typedef QScriptIntrusiveList<QJSValueIteratorPrivate, &QJSValueIteratorPrivate::m_node> ValueIteratorList;
    ValueIteratorList m_valueIterators;

    Q_DISABLE_COPY(QV8Engine)
};

// Allocate a new Persistent handle.  *ALL* persistent handles in QML must be allocated
// using this method.
template<class T>
v8::Persistent<T> qPersistentNew(v8::Handle<T> that)
{
    v8::Persistent<T> rv = v8::Persistent<T>::New(that);
#ifdef QML_GLOBAL_HANDLE_DEBUGGING
    QV8Engine::registerHandle(*rv);
#endif
    return rv;
}

// Register a Persistent handle that was returned to you by V8 (such as by
// v8::Context::New). This allows us to do handle tracking on these handles too.
template<class T>
void qPersistentRegister(v8::Persistent<T> handle)
{
#ifdef QML_GLOBAL_HANDLE_DEBUGGING
    QV8Engine::registerHandle(*handle);
#else
    Q_UNUSED(handle);
#endif
}

// Dispose and clear a persistent handle.  *ALL* persistent handles in QML must be
// disposed using this method.
template<class T>
void qPersistentDispose(v8::Persistent<T> &that)
{
#ifdef QML_GLOBAL_HANDLE_DEBUGGING
    QV8Engine::releaseHandle(*that);
#endif
    that.Dispose();
    that.Clear();
}

QString QV8Engine::toString(v8::Handle<v8::Value> string)
{
    return m_stringWrapper.toString(string->ToString());
}

QString QV8Engine::toString(v8::Handle<v8::String> string)
{
    return m_stringWrapper.toString(string);
}

bool QV8Engine::isVariant(v8::Handle<v8::Value> value)
{
    return m_variantWrapper.isVariant(value);
}

v8::Local<v8::Object> QV8Engine::qmlScope(QQmlContextData *ctxt, QObject *scope)
{
    return m_contextWrapper.qmlScope(ctxt, scope);
}

bool QV8Engine::isQObject(v8::Handle<v8::Value> obj)
{
    return obj->IsObject()?m_qobjectWrapper.isQObject(v8::Handle<v8::Object>::Cast(obj)):false;
}

QObject *QV8Engine::toQObject(v8::Handle<v8::Value> obj)
{
    return obj->IsObject()?m_qobjectWrapper.toQObject(v8::Handle<v8::Object>::Cast(obj)):0;
}

v8::Handle<v8::Value> QV8Engine::newQObject(QObject *object)
{
    return m_qobjectWrapper.newQObject(object);
}

v8::Handle<v8::Value> QV8Engine::newQObject(QObject *object, const ObjectOwnership ownership)
{
    if (!object)
        return v8::Null();

    v8::Handle<v8::Value> result = newQObject(object);
    QQmlData *ddata = QQmlData::get(object, true);
    if (ownership == JavaScriptOwnership && ddata) {
        ddata->indestructible = false;
        ddata->explicitIndestructibleSet = true;
    }
    return result;
}

v8::Local<v8::String> QV8Engine::toString(const QString &string)
{
    return m_stringWrapper.toString(string);
}

v8::Handle<v8::Value> QV8Engine::newValueType(QObject *object, int property, QQmlValueType *type)
{
    return m_valueTypeWrapper.newValueType(object, property, type);
}

v8::Handle<v8::Value> QV8Engine::newValueType(const QVariant &value, QQmlValueType *type)
{
    return m_valueTypeWrapper.newValueType(value, type);
}

bool QV8Engine::isValueType(v8::Handle<v8::Value> obj) const
{
    return obj->IsObject()?m_valueTypeWrapper.isValueType(v8::Handle<v8::Object>::Cast(obj)):false;
}

QVariant QV8Engine::toValueType(v8::Handle<v8::Value> obj)
{
    return obj->IsObject()?m_valueTypeWrapper.toVariant(v8::Handle<v8::Object>::Cast(obj)):QVariant();
}

v8::Handle<v8::Value> QV8Engine::newSequence(int sequenceType, QObject *object, int property, bool *succeeded)
{
    return m_sequenceWrapper.newSequence(sequenceType, object, property, succeeded);
}

v8::Handle<v8::String> QV8Engine::bindingFlagKey() const
{
    return m_bindingFlagKey;
}

// XXX Can this be made more optimal?  It is called prior to resolving each and every 
// unqualified name in QV8ContextWrapper.
bool QV8Engine::startsWithUpper(v8::Handle<v8::String> string)
{
    v8::String::Value value(string);
    Q_ASSERT(*value != NULL);
    uint16_t c = **value;
    return (c >= 'A' && c <= 'Z') ||
           (c > 127 && QChar::category(c) == QChar::Letter_Uppercase);
}

QV8Engine::Deletable *QV8Engine::extensionData(int index) const
{
    if (index < m_extensionData.count())
        return m_extensionData[index];
    else
        return 0;
}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlV8Handle)

#endif // QQMLV8ENGINE_P_H
