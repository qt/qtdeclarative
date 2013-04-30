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
#include "qjsvalueiterator_p.h"
#include "private/qintrusivelist_p.h"

#include <private/qqmlpropertycache_p.h>

#include "qv8objectresource_p.h"
#include "qv8contextwrapper_p.h"
#include "qv8qobjectwrapper_p.h"
#include "qv8typewrapper_p.h"
#include "qv8listwrapper_p.h"
#include "qv8variantwrapper_p.h"
#include "qv8valuetypewrapper_p.h"
#include "qv8sequencewrapper_p.h"
#include "qv4jsonwrapper_p.h"
#include <private/qv4value_p.h>

namespace QV4 {
struct ArrayObject;
}

namespace v8 {

// Needed for V8ObjectSet
inline uint qHash(const v8::Handle<v8::Object> &object, uint seed = 0)
{
    return (object->GetIdentityHash() ^ seed);
}

}

QT_BEGIN_NAMESPACE

namespace QV4 {
    struct ExecutionEngine;
    struct Value;
}

#define V4FUNCTION(function, engine) new QV4::BuiltinFunctionOld(engine->rootContext, engine->id_undefined, function)

// Uncomment the following line to enable global handle debugging.  When enabled, all the persistent
// handles allocated using qPersistentNew() (or registered with qPersistentRegsiter()) and disposed
// with qPersistentDispose() are tracked.  If you try and do something illegal, like double disposing
// a handle, qFatal() is called.
// #define QML_GLOBAL_HANDLE_DEBUGGING

#define V8ENGINE() ((QV8Engine *)v8::External::Cast(args.Data().get())->Value())
#define V8FUNCTION(function, engine) v8::FunctionTemplate::New(function, v8::External::New((QV8Engine*)engine))->GetFunction()
#define V8THROW_ERROR(string) { \
    v8::ThrowException(v8::Exception::Error(v8::String::New(string))); \
    return v8::Handle<v8::Value>(); \
}
#define V8THROW_TYPE(string) { \
    v8::ThrowException(v8::Exception::TypeError(v8::String::New(string))); \
    return v8::Handle<v8::Value>(); \
}
#define V8ENGINE_ACCESSOR() ((QV8Engine *)v8::External::Cast(info.Data().get())->Value());
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

class Q_QML_PRIVATE_EXPORT QQmlV4Handle
{
public:
    QQmlV4Handle() : d(0) {}
    QQmlV4Handle(const QQmlV4Handle &other) : d(other.d) {}
    QQmlV4Handle &operator=(const QQmlV4Handle &other) { d = other.d; return *this; }

    static QQmlV4Handle fromV8Handle(v8::Handle<v8::Value> h) {
        return QQmlV4Handle(h);
    }
    v8::Handle<v8::Value> toV8Handle() const {
        return v8::Value::NewFromInternalValue(d);
    }

    QV4::Value toValue() const;
    static QQmlV4Handle fromValue(const QV4::Value &v);
private:
    QQmlV4Handle(v8::Handle<v8::Value> h) : d(h.val) {}
    quint64 d;
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
    friend class QJSEngine;
    typedef QSet<QV4::Object *> V8ObjectSet;
public:
    static QV8Engine* get(QJSEngine* q) { Q_ASSERT(q); return q->handle(); }
    static QJSEngine* get(QV8Engine* d) { Q_ASSERT(d); return d->q; }
    static QV4::ExecutionEngine *getV4(QJSEngine *q) { return q->handle()->m_v4Engine; }
    static QV4::ExecutionEngine *getV4(QV8Engine *d) { return d->m_v4Engine; }

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
    QV4::Value global();
    v8::Handle<v8::Context> context() const { return m_context; }

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

    QV4::Value getOwnPropertyNames(const QV4::Value &o);
    void freezeObject(const QV4::Value &value);

    static inline bool startsWithUpper(QV4::String *);

    QVariant toVariant(const QV4::Value &value, int typeHint);
    QV4::Value fromVariant(const QVariant &);
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
    inline QV4::Value newQObject(QObject *object);
    inline QV4::Value newQObject(QObject *object, const ObjectOwnership ownership);
    inline bool isQObject(const QV4::Value &value);
    inline QObject *toQObject(const QV4::Value &value);

    // Return a JS string for the given QString \a string
    v8::Local<v8::String> toString(const QString &string);

    // Create a new value type object
    inline QV4::Value newValueType(QObject *, int coreIndex, QQmlValueType *);
    inline QV4::Value newValueType(const QVariant &, QQmlValueType *);
    inline bool isValueType(const QV4::Value &value) const;
    inline QVariant toValueType(const QV4::Value &obj);

    // Create a new sequence type object
    inline QV4::Value newSequence(int sequenceType, QObject *, int coreIndex, bool *succeeded);

    // Return the JS string key for the "function is a binding" flag
    inline QV4::Value bindingFlagKey() const;

    // Return the network access manager for this engine.  By default this returns the network
    // access manager of the QQmlEngine.  It is overridden in the case of a threaded v8
    // instance (like in WorkerScript).
    virtual QNetworkAccessManager *networkAccessManager();

    // Return the list of illegal id names (the names of the properties on the global object)
    const QStringHash<bool> &illegalNames() const;

    inline void collectGarbage() { gc(); }
    void gc();

#ifdef QML_GLOBAL_HANDLE_DEBUGGING
    // Used for handle debugging
    static void registerHandle(void *);
    static void releaseHandle(void *);
#endif

    static QMutex *registrationMutex();
    static int registerExtension();

    inline Deletable *extensionData(int) const;
    void setExtensionData(int, Deletable *);

    QV4::Value evaluateScript(const QString &script, QV4::Object *scopeObject = 0);

    QJSValue newArray(uint length);

    QV4::Value variantListToJS(const QVariantList &lst);
    inline QVariantList variantListFromJS(QV4::ArrayObject *array)
    { V8ObjectSet visitedObjects; return variantListFromJS(array, visitedObjects); }

    QV4::Value variantMapToJS(const QVariantMap &vmap);
    inline QVariantMap variantMapFromJS(QV4::Object *object)
    { V8ObjectSet visitedObjects; return variantMapFromJS(object, visitedObjects); }

    QV4::Value variantToJS(const QVariant &value);
    inline QVariant variantFromJS(const QV4::Value &value)
    { V8ObjectSet visitedObjects; return variantFromJS(value, visitedObjects); }

    QV4::Value jsonValueToJS(const QJsonValue &value);
    QJsonValue jsonValueFromJS(const QV4::Value &value);
    QV4::Value jsonObjectToJS(const QJsonObject &object);
    QJsonObject jsonObjectFromJS(const QV4::Value &value);
    QV4::Value jsonArrayToJS(const QJsonArray &array);
    QJsonArray jsonArrayFromJS(const QV4::Value &value);

    QV4::Value metaTypeToJS(int type, const void *data);
    bool metaTypeFromJS(const QV4::Value &value, int type, void *data);

    bool convertToNativeQObject(const QV4::Value &value,
                                const QByteArray &targetType,
                                void **result);

    QJSValue scriptValueFromInternal(const QV4::Value &) const;

    // used for console.time(), console.timeEnd()
    void startTimer(const QString &timerName);
    qint64 stopTimer(const QString &timerName, bool *wasRunning);

    // used for console.count()
    int consoleCountHelper(const QString &file, quint16 line, quint16 column);

    QObject *qtObjectFromJS(const QV4::Value &value);

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

    QV4::ExecutionEngine *m_v4Engine;

    bool m_ownsV8Context;
    v8::Persistent<v8::Context> m_context;

    QV4::PersistentValue m_bindingFlagKey;

    QV8ContextWrapper m_contextWrapper;
    QV8QObjectWrapper m_qobjectWrapper;
    QV8TypeWrapper m_typeWrapper;
    QV8ListWrapper m_listWrapper;
    QV8VariantWrapper m_variantWrapper;
    QV8ValueTypeWrapper m_valueTypeWrapper;
    QV8SequenceWrapper m_sequenceWrapper;
    QV4JsonWrapper m_jsonWrapper;

    QV4::PersistentValue m_freezeObject;

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
    QVariantList variantListFromJS(QV4::ArrayObject *array, V8ObjectSet &visitedObjects);
    QVariantMap variantMapFromJS(QV4::Object *object, V8ObjectSet &visitedObjects);
    QVariant variantFromJS(const QV4::Value &value, V8ObjectSet &visitedObjects);

    static v8::Persistent<v8::Object> *findOwnerAndStrength(QObject *object, bool *shouldBeStrong);

//    typedef QIntrusiveList<QJSValuePrivate, &QJSValuePrivate::m_node> ValueList;
//    ValueList m_values;
//    typedef QIntrusiveList<QJSValueIteratorPrivate, &QJSValueIteratorPrivate::m_node> ValueIteratorList;
//    ValueIteratorList m_valueIterators;

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

bool QV8Engine::isVariant(v8::Handle<v8::Value> value)
{
    return m_variantWrapper.isVariant(value);
}

v8::Local<v8::Object> QV8Engine::qmlScope(QQmlContextData *ctxt, QObject *scope)
{
    return m_contextWrapper.qmlScope(ctxt, scope);
}

bool QV8Engine::isQObject(const QV4::Value &value)
{
    return value.isObject() ? m_qobjectWrapper.isQObject(value) : false;
}

QObject *QV8Engine::toQObject(const QV4::Value &value)
{
    return value.isObject() ? m_qobjectWrapper.toQObject(value) : 0;
}

QV4::Value QV8Engine::newQObject(QObject *object)
{
    return m_qobjectWrapper.newQObject(object)->v4Value();
}

QV4::Value QV8Engine::newQObject(QObject *object, const ObjectOwnership ownership)
{
    if (!object)
        return QV4::Value::nullValue();

    QV4::Value result = newQObject(object);
    QQmlData *ddata = QQmlData::get(object, true);
    if (ownership == JavaScriptOwnership && ddata) {
        ddata->indestructible = false;
        ddata->explicitIndestructibleSet = true;
    }
    return result;
}

QV4::Value QV8Engine::newValueType(QObject *object, int property, QQmlValueType *type)
{
    return m_valueTypeWrapper.newValueType(object, property, type)->v4Value();
}

QV4::Value QV8Engine::newValueType(const QVariant &value, QQmlValueType *type)
{
    return m_valueTypeWrapper.newValueType(value, type)->v4Value();
}

bool QV8Engine::isValueType(const QV4::Value &value) const
{
    return value.isObject() ? m_valueTypeWrapper.isValueType(v8::Handle<v8::Object>::Cast(v8::Value::fromV4Value(value))) : false;
}

QVariant QV8Engine::toValueType(const QV4::Value &obj)
{
    return obj.isObject() ? m_valueTypeWrapper.toVariant(v8::Handle<v8::Object>::Cast(v8::Value::fromV4Value(obj))) : QVariant();
}

QV4::Value QV8Engine::newSequence(int sequenceType, QObject *object, int property, bool *succeeded)
{
    return m_sequenceWrapper.newSequence(sequenceType, object, property, succeeded)->v4Value();
}

QV4::Value QV8Engine::bindingFlagKey() const
{
    return m_bindingFlagKey;
}

// XXX Can this be made more optimal?  It is called prior to resolving each and every 
// unqualified name in QV8ContextWrapper.
bool QV8Engine::startsWithUpper(QV4::String *string)
{
    uint16_t c = string->toQString().at(0).unicode();
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

Q_DECLARE_METATYPE(QQmlV4Handle)

#endif // QQMLV8ENGINE_P_H
