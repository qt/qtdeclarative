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

#ifndef QDECLARATIVEV8ENGINE_P_H
#define QDECLARATIVEV8ENGINE_P_H

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

#include <private/qv8_p.h>
#include <qjsengine.h>
#include <qjsvalue.h>
#include "qjsvalue_p.h"
#include "qjsvalueiterator_p.h"
#include "qscriptoriginalglobalobject_p.h"
#include "qscripttools_p.h"

#include <private/qdeclarativepropertycache_p.h>

#include "qv8contextwrapper_p.h"
#include "qv8qobjectwrapper_p.h"
#include "qv8stringwrapper_p.h"
#include "qv8typewrapper_p.h"
#include "qv8listwrapper_p.h"
#include "qv8variantwrapper_p.h"
#include "qv8valuetypewrapper_p.h"

QT_BEGIN_NAMESPACE


// Uncomment the following line to enable global handle debugging.  When enabled, all the persistent
// handles allocated using qPersistentNew() (or registered with qPersistentRegsiter()) and disposed
// with qPersistentDispose() are tracked.  If you try and do something illegal, like double disposing
// a handle, qFatal() is called.
// #define QML_GLOBAL_HANDLE_DEBUGGING

#define V8_RESOURCE_TYPE(resourcetype) \
public: \
    enum { V8ResourceType = QV8ObjectResource:: resourcetype }; \
    virtual QV8ObjectResource::ResourceType resourceType() const { return QV8ObjectResource:: resourcetype; } \
private: 

#define V8ENGINE() ((QV8Engine *)v8::External::Unwrap(args.Data()))
#define V8FUNCTION(function, engine) v8::FunctionTemplate::New(function, v8::External::Wrap((QV8Engine*)engine))->GetFunction()
#define V8THROW_ERROR(string) { \
    v8::ThrowException(v8::Exception::Error(v8::String::New(string))); \
    return v8::Handle<v8::Value>(); \
}
#define V8THROW_TYPE(string) { \
    v8::ThrowException(v8::Exception::TypeError(v8::String::New(string))); \
    return v8::Handle<v8::Value>(); \
}
#define V8ENGINE_ACCESSOR() ((QV8Engine *)v8::External::Unwrap(info.Data()));
#define V8THROW_ERROR_SETTER(string) { \
    v8::ThrowException(v8::Exception::Error(v8::String::New(string))); \
    return; \
}

#define V8_DEFINE_EXTENSION(dataclass, datafunction) \
    inline dataclass *datafunction(QV8Engine *engine) \
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

class QV8Engine;
class QV8ObjectResource : public v8::Object::ExternalResource
{
public:
    QV8ObjectResource(QV8Engine *engine) : engine(engine) { Q_ASSERT(engine); }
    enum ResourceType { ContextType, QObjectType, TypeType, ListType, VariantType, 
                        ValueTypeType, XMLHttpRequestType, DOMNodeType, SQLDatabaseType,
                        ListModelType, Context2DType, ParticleDataType };
    virtual ResourceType resourceType() const = 0;

    QV8Engine *engine;
};

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
//         Q_INVOKABLE void myMethod(QDeclarativeV8Function*);
//     };
// The QDeclarativeV8Function - and consequently the arguments and return value - only remains 
// valid during the call.  If the return value isn't set within myMethod(), the will return
// undefined.
class QV8Engine;
class QDeclarativeV8Function
{
public:
    int Length() const { return _ac; }
    v8::Local<v8::Value> operator[](int idx) { return (*_a)->Get(idx); }
    QDeclarativeContextData *context() { return _c; }
    v8::Handle<v8::Object> qmlGlobal() { return *_g; }
    void returnValue(v8::Handle<v8::Value> rv) { *_r = rv; }
    QV8Engine *engine() const { return _e; }
private:
    friend class QV8QObjectWrapper;
    QDeclarativeV8Function();
    QDeclarativeV8Function(const QDeclarativeV8Function &);
    QDeclarativeV8Function &operator=(const QDeclarativeV8Function &);

    QDeclarativeV8Function(int length, v8::Handle<v8::Object> &args, 
                           v8::Handle<v8::Value> &rv, v8::Handle<v8::Object> &global,
                           QDeclarativeContextData *c, QV8Engine *e)
    : _ac(length), _a(&args), _r(&rv), _g(&global), _c(c), _e(e) {}

    int _ac;
    v8::Handle<v8::Object> *_a;
    v8::Handle<v8::Value> *_r;
    v8::Handle<v8::Object> *_g;
    QDeclarativeContextData *_c;
    QV8Engine *_e;
};

class QDeclarativeV8Handle
{
public:
    QDeclarativeV8Handle() : d(0) {}
    QDeclarativeV8Handle(const QDeclarativeV8Handle &other) : d(other.d) {}
    QDeclarativeV8Handle &operator=(const QDeclarativeV8Handle &other) { d = other.d; return *this; }

    static QDeclarativeV8Handle fromHandle(v8::Handle<v8::Value> h) {
        return QDeclarativeV8Handle(*h);
    }
    v8::Handle<v8::Value> toHandle() const {
        return v8::Handle<v8::Value>((v8::Value *)d);
    }
private:
    QDeclarativeV8Handle(void *d) : d(d) {}
    void *d;
};

class QObject;
class QDeclarativeEngine;
class QDeclarativeValueType;
class QNetworkAccessManager;
class QDeclarativeContextData;

class Q_DECLARATIVE_EXPORT QV8Engine
{
public:
    static QV8Engine* get(QJSEngine* q) { Q_ASSERT(q); return q->handle(); }
    static QJSEngine* get(QV8Engine* d) { Q_ASSERT(d); return d->q; }

    QV8Engine(QJSEngine* qq,QJSEngine::ContextOwnership ownership = QJSEngine::CreateNewContext);
    ~QV8Engine();

    struct Deletable {
        virtual ~Deletable() {}
    };

    class Exception
    {
        typedef QPair<v8::Persistent<v8::Value>, v8::Persistent<v8::Message> > ValueMessagePair;

        v8::Persistent<v8::Value> m_value;
        v8::Persistent<v8::Message> m_message;
        QStack<ValueMessagePair> m_stack;

        Q_DISABLE_COPY(Exception)
    public:
        inline Exception();
        inline ~Exception();
        inline void set(v8::Handle<v8::Value> value, v8::Handle<v8::Message> message);
        inline void clear();
        inline operator bool() const;
        inline operator v8::Handle<v8::Value>() const;
        inline int lineNumber() const;
        inline QStringList backtrace() const;

        inline void push();
        inline void pop();
    };

    void initDeclarativeGlobalObject();
    void setEngine(QDeclarativeEngine *engine);
    QDeclarativeEngine *engine() { return m_engine; }
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

    void *xmlHttpRequestData() { return m_xmlHttpRequestData; }
    void *sqlDatabaseData() { return m_sqlDatabaseData; }

    Deletable *listModelData() { return m_listModelData; }
    void setListModelData(Deletable *d) { if (m_listModelData) delete m_listModelData; m_listModelData = d; }

    QDeclarativeContextData *callingContext();

    v8::Local<v8::Array> getOwnPropertyNames(v8::Handle<v8::Object>);
    inline QJSValue::PropertyFlags getPropertyFlags(v8::Handle<v8::Object> object, v8::Handle<v8::Value> property);
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
                                         int lineNumber = 1);

    // Return the QML global "scope" object for the \a ctxt context and \a scope object.
    inline v8::Local<v8::Object> qmlScope(QDeclarativeContextData *ctxt, QObject *scope);

    QScriptPassPointer<QJSValuePrivate> newRegExp(const QRegExp &regexp);
    QScriptPassPointer<QJSValuePrivate> newRegExp(const QString &pattern, const QString &flags);

    // Return a JS wrapper for the given QObject \a object
    inline v8::Handle<v8::Value> newQObject(QObject *object);
    inline bool isQObject(v8::Handle<v8::Value>);
    inline QObject *toQObject(v8::Handle<v8::Value>);

    // Return a JS string for the given QString \a string
    inline v8::Local<v8::String> toString(const QString &string);

    // Create a new value type object
    inline v8::Handle<v8::Value> newValueType(QObject *, int coreIndex, QDeclarativeValueType *);
    inline v8::Handle<v8::Value> newValueType(const QVariant &, QDeclarativeValueType *);

    // Create a new QVariant object.  This doesn't examine the type of the variant, but always returns
    // a QVariant wrapper
    inline v8::Handle<v8::Value> newQVariant(const QVariant &);

    // Return the network access manager for this engine.  By default this returns the network
    // access manager of the QDeclarativeEngine.  It is overridden in the case of a threaded v8
    // instance (like in WorkerScript).
    virtual QNetworkAccessManager *networkAccessManager();

    // Return the list of illegal id names (the names of the properties on the global object)
    const QSet<QString> &illegalNames() const;

    inline void collectGarbage() { gc(); }
    static void gc();

    void clearExceptions();
    void setException(v8::Handle<v8::Value> value, v8::Handle<v8::Message> message = v8::Handle<v8::Message>());
    v8::Handle<v8::Value> throwException(v8::Handle<v8::Value> value);
    bool hasUncaughtException() const;
    int uncaughtExceptionLineNumber() const;
    QStringList uncaughtExceptionBacktrace() const;
    v8::Handle<v8::Value> uncaughtException() const;
    void saveException();
    void restoreException();

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
    inline v8::Handle<v8::Value> makeJSValue(int value);
    inline v8::Handle<v8::Value> makeJSValue(uint value);
    inline v8::Handle<v8::Value> makeJSValue(double value);
    inline v8::Handle<v8::Value> makeJSValue(QJSValue::SpecialValue value);
    inline v8::Handle<v8::Value> makeJSValue(const QString& value);

    inline QScriptPassPointer<QJSValuePrivate> evaluate(const QString &program, const QString &fileName = QString(), int lineNumber = 1);
    QScriptPassPointer<QJSValuePrivate> evaluate(v8::Handle<v8::Script> script, v8::TryCatch& tryCatch);

    QScriptPassPointer<QJSValuePrivate> newArray(uint length);
    v8::Handle<v8::Object> newVariant(const QVariant &variant);
    QScriptPassPointer<QJSValuePrivate> newVariant(QJSValuePrivate* value, const QVariant &variant);

    v8::Handle<v8::Array> variantListToJS(const QVariantList &lst);
    QVariantList variantListFromJS(v8::Handle<v8::Array> jsArray);

    v8::Handle<v8::Object> variantMapToJS(const QVariantMap &vmap);
    QVariantMap variantMapFromJS(v8::Handle<v8::Object> jsObject);

    v8::Handle<v8::Value> variantToJS(const QVariant &value);
    QVariant variantFromJS(v8::Handle<v8::Value> value);

    v8::Handle<v8::Value> metaTypeToJS(int type, const void *data);
    bool metaTypeFromJS(v8::Handle<v8::Value> value, int type, void *data);

    bool convertToNativeQObject(v8::Handle<v8::Value> value,
                                const QByteArray &targetType,
                                void **result);

    QVariant variantValue(v8::Handle<v8::Value> value);

    QJSValue scriptValueFromInternal(v8::Handle<v8::Value>) const;

    void emitSignalHandlerException();

    QObject *qtObjectFromJS(v8::Handle<v8::Value> value);
    QSet<int> visitedConversionObjects;
protected:
    QJSEngine* q;
    QDeclarativeEngine *m_engine;
    bool m_ownsV8Context;
    v8::Persistent<v8::Context> m_context;
    QScriptOriginalGlobalObject m_originalGlobalObject;

    QV8StringWrapper m_stringWrapper;
    QV8ContextWrapper m_contextWrapper;
    QV8QObjectWrapper m_qobjectWrapper;
    QV8TypeWrapper m_typeWrapper;
    QV8ListWrapper m_listWrapper;
    QV8VariantWrapper m_variantWrapper;
    QV8ValueTypeWrapper m_valueTypeWrapper;

    v8::Persistent<v8::Function> m_getOwnPropertyNames;
    v8::Persistent<v8::Function> m_freezeObject;

    void *m_xmlHttpRequestData;
    void *m_sqlDatabaseData;

    QVector<Deletable *> m_extensionData;
    Deletable *m_listModelData;

    QSet<QString> m_illegalNames;

    Exception m_exception;

    QVariant toBasicVariant(v8::Handle<v8::Value>);

    void initializeGlobal(v8::Handle<v8::Object>);

    static v8::Handle<v8::Value> gc(const v8::Arguments &args);
    static v8::Handle<v8::Value> print(const v8::Arguments &args);
    static v8::Handle<v8::Value> isQtObject(const v8::Arguments &args);
    static v8::Handle<v8::Value> rgba(const v8::Arguments &args);
    static v8::Handle<v8::Value> hsla(const v8::Arguments &args);
    static v8::Handle<v8::Value> rect(const v8::Arguments &args);
    static v8::Handle<v8::Value> point(const v8::Arguments &args);
    static v8::Handle<v8::Value> size(const v8::Arguments &args);
    static v8::Handle<v8::Value> vector3d(const v8::Arguments &args);
    static v8::Handle<v8::Value> vector4d(const v8::Arguments &args);
    static v8::Handle<v8::Value> lighter(const v8::Arguments &args);
    static v8::Handle<v8::Value> darker(const v8::Arguments &args);
    static v8::Handle<v8::Value> tint(const v8::Arguments &args);
    static v8::Handle<v8::Value> formatDate(const v8::Arguments &args);
    static v8::Handle<v8::Value> formatTime(const v8::Arguments &args);
    static v8::Handle<v8::Value> formatDateTime(const v8::Arguments &args);
    static v8::Handle<v8::Value> openUrlExternally(const v8::Arguments &args);
    static v8::Handle<v8::Value> fontFamilies(const v8::Arguments &args);
    static v8::Handle<v8::Value> md5(const v8::Arguments &args);
    static v8::Handle<v8::Value> btoa(const v8::Arguments &args);
    static v8::Handle<v8::Value> atob(const v8::Arguments &args);
    static v8::Handle<v8::Value> quit(const v8::Arguments &args);
    static v8::Handle<v8::Value> resolvedUrl(const v8::Arguments &args);
    static v8::Handle<v8::Value> createQmlObject(const v8::Arguments &args);
    static v8::Handle<v8::Value> createComponent(const v8::Arguments &args);
    static v8::Handle<v8::Value> qsTranslate(const v8::Arguments &args);
    static v8::Handle<v8::Value> qsTranslateNoOp(const v8::Arguments &args);
    static v8::Handle<v8::Value> qsTr(const v8::Arguments &args);
    static v8::Handle<v8::Value> qsTrNoOp(const v8::Arguments &args);
    static v8::Handle<v8::Value> qsTrId(const v8::Arguments &args);
    static v8::Handle<v8::Value> qsTrIdNoOp(const v8::Arguments &args);
    static v8::Handle<v8::Value> stringArg(const v8::Arguments &args);

    double qtDateTimeToJsDate(const QDateTime &dt);
    QDateTime qtDateTimeFromJsDate(double jsDate);

private:
    typedef QScriptIntrusiveList<QJSValuePrivate, &QJSValuePrivate::m_node> ValueList;
    ValueList m_values;
    typedef QScriptIntrusiveList<QJSValueIteratorPrivate, &QJSValueIteratorPrivate::m_node> ValueIteratorList;
    ValueIteratorList m_valueIterators;

    Q_DISABLE_COPY(QV8Engine)
    friend class QV8DebugService;
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

v8::Local<v8::Object> QV8Engine::qmlScope(QDeclarativeContextData *ctxt, QObject *scope)
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

v8::Local<v8::String> QV8Engine::toString(const QString &string)
{
    return m_stringWrapper.toString(string);
}

v8::Handle<v8::Value> QV8Engine::newValueType(QObject *object, int property, QDeclarativeValueType *type)
{
    return m_valueTypeWrapper.newValueType(object, property, type);
}

v8::Handle<v8::Value> QV8Engine::newValueType(const QVariant &value, QDeclarativeValueType *type)
{
    return m_valueTypeWrapper.newValueType(value, type);
}

// XXX Can this be made more optimal?  It is called prior to resolving each and every 
// unqualified name in QV8ContextWrapper.
bool QV8Engine::startsWithUpper(v8::Handle<v8::String> string)
{
    uint16_t c = string->GetCharacter(0);
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

#endif // QDECLARATIVEV8ENGINE_P_H
