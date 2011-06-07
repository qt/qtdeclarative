/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv8qobjectwrapper_p.h"
#include "qv8contextwrapper_p.h"
#include "qv8engine_p.h"

#include <private/qdeclarativeguard_p.h>
#include <private/qdeclarativepropertycache_p.h>
#include <private/qdeclarativeengine_p.h>
#include <private/qdeclarativevmemetaobject_p.h>
#include <private/qdeclarativebinding_p.h>

#include <QtScript/qscriptvalue.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qtimer.h>
#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_METATYPE(QScriptValue);
Q_DECLARE_METATYPE(QDeclarativeV8Handle);

#if defined(__GNUC__)
# if (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
// The code in this file does not violate strict aliasing, but GCC thinks it does
// so turn off the warnings for us to have a clean build
#  pragma GCC diagnostic ignored "-Wstrict-aliasing"
# endif
#endif

#define QOBJECT_TOSTRING_INDEX -2
#define QOBJECT_DESTROY_INDEX -3

class QV8QObjectResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(QObjectType);

public:
    QV8QObjectResource(QV8Engine *engine, QObject *object);

    QDeclarativeGuard<QObject> object;
};

class QV8QObjectInstance : public QDeclarativeGuard<QObject>
{
public:
    QV8QObjectInstance(QObject *o, QV8QObjectWrapper *w)
    : QDeclarativeGuard<QObject>(o), wrapper(w)
    {
    }

    ~QV8QObjectInstance()
    {
        v8object.Dispose();
        v8object.Clear();
    }

    virtual void objectDestroyed(QObject *o)
    {
        if (wrapper)
            wrapper->m_taintedObjects.remove(o);
        delete this;
    }

    v8::Persistent<v8::Object> v8object;
    QV8QObjectWrapper *wrapper;
};

namespace {
struct MetaCallArgument {
    inline MetaCallArgument();
    inline ~MetaCallArgument();
    inline void *dataPtr();

    inline void initAsType(int type);
    inline void fromValue(int type, QV8Engine *, v8::Handle<v8::Value>);
    inline v8::Handle<v8::Value> toValue(QV8Engine *);

private:
    MetaCallArgument(const MetaCallArgument &);

    inline void cleanup();

    char data[4 * sizeof(void *)];
    int type;
    bool isObjectType;
};
}

QV8QObjectResource::QV8QObjectResource(QV8Engine *engine, QObject *object) 
: QV8ObjectResource(engine), object(object) 
{
}

static QAtomicInt objectIdCounter(1);

QV8QObjectWrapper::QV8QObjectWrapper()
: m_engine(0), m_id(objectIdCounter.fetchAndAddOrdered(1))
{
}

QV8QObjectWrapper::~QV8QObjectWrapper()
{
    for (TaintedHash::Iterator iter = m_taintedObjects.begin(); 
         iter != m_taintedObjects.end();
         ++iter) {
        (*iter)->wrapper = 0;
    }
    m_taintedObjects.clear();
}

void QV8QObjectWrapper::destroy()
{
    qDeleteAll(m_connections);
    m_connections.clear();

    m_hiddenObject.Dispose();
    m_destroySymbol.Dispose();
    m_toStringSymbol.Dispose();
    m_methodConstructor.Dispose();
    m_constructor.Dispose();
}

#define FAST_VALUE_GETTER(name, cpptype, defaultvalue, constructor) \
static v8::Handle<v8::Value> name ## ValueGetter(v8::Local<v8::String>, const v8::AccessorInfo &info) \
{ \
    v8::Handle<v8::Object> This = info.This(); \
    QV8QObjectResource *resource = v8_resource_cast<QV8QObjectResource>(This); \
 \
    if (!resource || resource->object.isNull()) return v8::Undefined(); \
 \
    QObject *object = resource->object; \
 \
    uint32_t data = info.Data()->Uint32Value(); \
    int index = data & 0x7FFF; \
    int notify = (data & 0x7FFF0000) >> 16; \
    if (notify == 0x7FFF) notify = -1; \
 \
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(resource->engine->engine()); \
    if (notify /* 0 means constant */ && ep->captureProperties) { \
        typedef QDeclarativeEnginePrivate::CapturedProperty CapturedProperty; \
        ep->capturedProperties << CapturedProperty(object, index, notify); \
    } \
 \
    cpptype value = defaultvalue; \
    void *args[] = { &value, 0 }; \
    QMetaObject::metacall(object, QMetaObject::ReadProperty, index, args); \
 \
    return constructor(value); \
} 

#define CREATE_FUNCTION \
    "(function(method) { "\
        "return (function(object, data, qmlglobal) { "\
            "return (function() { "\
                "return method(object, data, qmlglobal, arguments.length, arguments); "\
            "});"\
        "});"\
    "})"

void QV8QObjectWrapper::init(QV8Engine *engine)
{
    m_engine = engine;

    m_toStringSymbol = v8::Persistent<v8::String>::New(v8::String::NewSymbol("toString"));
    m_destroySymbol = v8::Persistent<v8::String>::New(v8::String::NewSymbol("destroy"));
    m_hiddenObject = v8::Persistent<v8::Object>::New(v8::Object::New());

    {
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetFallbackPropertyHandler(Getter, Setter, Query, 0, Enumerator);
    ft->InstanceTemplate()->SetHasExternalResource(true);
    m_constructor = v8::Persistent<v8::Function>::New(ft->GetFunction());
    }
    {
    v8::ScriptOrigin origin(m_hiddenObject); // Hack to allow us to identify these functions
    v8::Local<v8::Script> script = v8::Script::New(v8::String::New(CREATE_FUNCTION), &origin);
    v8::Local<v8::Function> fn = v8::Local<v8::Function>::Cast(script->Run());
    v8::Handle<v8::Value> invokeFn = v8::FunctionTemplate::New(Invoke)->GetFunction();
    v8::Handle<v8::Value> args[] = { invokeFn };
    v8::Local<v8::Function> createFn = v8::Local<v8::Function>::Cast(fn->Call(engine->global(), 1, args));
    m_methodConstructor = v8::Persistent<v8::Function>::New(createFn);
    }

    {
    v8::Local<v8::Object> prototype = engine->global()->Get(v8::String::New("Function"))->ToObject()->Get(v8::String::New("prototype"))->ToObject();
    prototype->Set(v8::String::New("connect"), V8FUNCTION(Connect, engine));
    prototype->Set(v8::String::New("disconnect"), V8FUNCTION(Disconnect, engine));
    }
}

bool QV8QObjectWrapper::isQObject(v8::Handle<v8::Object> obj)
{
    return v8_resource_cast<QV8QObjectResource>(obj) != 0;
}

QObject *QV8QObjectWrapper::toQObject(v8::Handle<v8::Object> obj)
{
    QV8QObjectResource *r =  v8_resource_cast<QV8QObjectResource>(obj);
    return r?r->object:0;
}

// r *MUST* be a QV8ObjectResource (r->type() == QV8ObjectResource::QObjectType)
QObject *QV8QObjectWrapper::QV8QObjectWrapper::toQObject(QV8ObjectResource *r)
{
    Q_ASSERT(r->resourceType() == QV8ObjectResource::QObjectType);
    return static_cast<QV8QObjectResource *>(r)->object;
}

// Load value properties
static v8::Handle<v8::Value> LoadProperty(QV8Engine *engine, QObject *object, 
                                          const QDeclarativePropertyCache::Data &property)
{
    Q_ASSERT(!property.isFunction());

#define PROPERTY_LOAD(metatype, cpptype, constructor) \
    if (property.propType == QMetaType:: metatype) { \
        cpptype type = cpptype(); \
        void *args[] = { &type, 0 }; \
        QMetaObject::metacall(object, QMetaObject::ReadProperty, property.coreIndex, args); \
        return constructor(type); \
    }

    if (property.isQObject()) {
        QObject *rv = 0;
        void *args[] = { &rv, 0 };
        QMetaObject::metacall(object, QMetaObject::ReadProperty, property.coreIndex, args);
        return engine->newQObject(rv);
    } else if (property.isQList()) {
        return engine->listWrapper()->newList(object, property.coreIndex, property.propType);
    } else PROPERTY_LOAD(QReal, qreal, v8::Number::New)
    else PROPERTY_LOAD(Int || property.isEnum(), int, v8::Number::New)
    else PROPERTY_LOAD(Bool, bool, v8::Boolean::New)
    else PROPERTY_LOAD(QString, QString, engine->toString)
    else PROPERTY_LOAD(UInt, uint, v8::Integer::NewFromUnsigned)
    else PROPERTY_LOAD(Float, float, v8::Number::New)
    else PROPERTY_LOAD(Double, double, v8::Number::New)
    else if(property.isV8Handle()) {
        QDeclarativeV8Handle handle;
        void *args[] = { &handle, 0 };
        QMetaObject::metacall(object, QMetaObject::ReadProperty, property.coreIndex, args); 
        return reinterpret_cast<v8::Handle<v8::Value> &>(handle);
    } else if (QDeclarativeValueTypeFactory::isValueType((uint)property.propType)) {
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine->engine());
        QDeclarativeValueType *valueType = ep->valueTypes[property.propType];
        if (valueType)
            return engine->newValueType(object, property.coreIndex, valueType);
    } 

    QVariant var = object->metaObject()->property(property.coreIndex).read(object);
    return engine->fromVariant(var);
}

v8::Handle<v8::Value> QV8QObjectWrapper::GetProperty(QV8Engine *engine, QObject *object, 
                                                     v8::Handle<v8::Value> *objectHandle, 
                                                     v8::Handle<v8::String> property,
                                                     QV8QObjectWrapper::RevisionMode revisionMode)
{
    // XXX aakenned This can't possibly be the best solution!!!
    struct MethodClosure {
       static v8::Handle<v8::Value> create(QV8Engine *engine, QObject *object, 
                                           v8::Handle<v8::Value> *objectHandle, 
                                           int index) { 
           v8::Handle<v8::Value> argv[] = { 
               objectHandle?*objectHandle:engine->newQObject(object),
               v8::Integer::New(index)
           };
           return engine->qobjectWrapper()->m_methodConstructor->Call(engine->global(), 2, argv);
       }
       static v8::Handle<v8::Value> createWithGlobal(QV8Engine *engine, QObject *object, 
                                                     v8::Handle<v8::Value> *objectHandle, 
                                                     int index) { 
           v8::Handle<v8::Value> argv[] = { 
               objectHandle?*objectHandle:engine->newQObject(object),
               v8::Integer::New(index),
               v8::Context::GetCallingQmlGlobal()
           };
           return engine->qobjectWrapper()->m_methodConstructor->Call(engine->global(), 3, argv);
       }
    };

    if (engine->qobjectWrapper()->m_toStringSymbol->StrictEquals(property)) {
        return MethodClosure::create(engine, object, objectHandle, QOBJECT_TOSTRING_INDEX);
    } else if (engine->qobjectWrapper()->m_destroySymbol->StrictEquals(property)) {
        return MethodClosure::create(engine, object, objectHandle, QOBJECT_DESTROY_INDEX);
    }

    QDeclarativePropertyCache::Data local;
    QDeclarativePropertyCache::Data *result = 0;
    result = QDeclarativePropertyCache::property(engine->engine(), object, property, local);

    if (!result)
        return v8::Handle<v8::Value>();

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine->engine());

    if (revisionMode == QV8QObjectWrapper::CheckRevision && result->revision != 0) {
        QDeclarativeData *ddata = QDeclarativeData::get(object);
        if (ddata && ddata->propertyCache && !ddata->propertyCache->isAllowedInRevision(result))
            return v8::Handle<v8::Value>();
    }

    typedef QDeclarativeEnginePrivate::CapturedProperty CapturedProperty;

    if (result->isFunction()) {
        if (result->flags & QDeclarativePropertyCache::Data::IsVMEFunction) {
            return ((QDeclarativeVMEMetaObject *)(object->metaObject()))->vmeMethod(result->coreIndex);
        } else if (result->flags & QDeclarativePropertyCache::Data::IsV8Function) {
            return MethodClosure::createWithGlobal(engine, object, objectHandle, result->coreIndex);
        } else {
            return MethodClosure::create(engine, object, objectHandle, result->coreIndex);
        }
    }

    if (ep->captureProperties && !result->isConstant()) {
        if (result->coreIndex == 0)
            ep->capturedProperties << CapturedProperty(QDeclarativeData::get(object, true)->objectNameNotifier());
        else
            ep->capturedProperties << CapturedProperty(object, result->coreIndex, result->notifyIndex);
    }

    return LoadProperty(engine, object, *result);
}

// Setter for writable properties.  Shared between the interceptor and fast property accessor
static inline void StoreProperty(QV8Engine *engine, QObject *object, QDeclarativePropertyCache::Data *property,
                                 v8::Handle<v8::Value> value)
{
    QDeclarativeBinding *newBinding = 0;

    if (value->IsFunction()) {
        QDeclarativeContextData *context = engine->callingContext();
        v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(value);

        v8::Local<v8::StackTrace> trace = 
            v8::StackTrace::CurrentStackTrace(1, (v8::StackTrace::StackTraceOptions)(v8::StackTrace::kLineNumber | 
                                                                                     v8::StackTrace::kScriptName));
        v8::Local<v8::StackFrame> frame = trace->GetFrame(0);
        int lineNumber = frame->GetLineNumber();
        QString url = engine->toString(frame->GetScriptName());

        QDeclarativePropertyCache::ValueTypeData valueTypeData;
        newBinding = new QDeclarativeBinding(&function, object, context);
        newBinding->setSourceLocation(url, lineNumber);
        newBinding->setTarget(QDeclarativePropertyPrivate::restore(*property, valueTypeData, object, context));
        newBinding->setEvaluateFlags(newBinding->evaluateFlags() | QDeclarativeBinding::RequiresThisObject);
    }

    QDeclarativeAbstractBinding *oldBinding = 
        QDeclarativePropertyPrivate::setBinding(object, property->coreIndex, -1, newBinding);
    if (oldBinding)
        oldBinding->destroy();

#define PROPERTY_STORE(cpptype, value) \
    cpptype o = value; \
    int status = -1; \
    int flags = 0; \
    void *argv[] = { &o, 0, &status, &flags }; \
    QMetaObject::metacall(object, QMetaObject::WriteProperty, property->coreIndex, argv);


    if (value->IsNull() && property->isQObject()) {
        PROPERTY_STORE(QObject*, 0);
    } else if (value->IsUndefined() && property->isResettable()) {
        void *a[] = { 0 };
        QMetaObject::metacall(object, QMetaObject::ResetProperty, property->coreIndex, a);
    } else if (value->IsUndefined() && property->propType == qMetaTypeId<QVariant>()) {
        PROPERTY_STORE(QVariant, QVariant());
    } else if (value->IsUndefined()) {
        QString error = QLatin1String("Cannot assign [undefined] to ") +
                        QLatin1String(QMetaType::typeName(property->propType));
        v8::ThrowException(v8::Exception::Error(engine->toString(error)));
    } else if (value->IsFunction()) {
        // this is handled by the binding creation above
    } else if (property->propType == QMetaType::Int && value->IsNumber()) {
        PROPERTY_STORE(int, qRound(value->ToNumber()->Value()));
    } else if (property->propType == QMetaType::QReal && value->IsNumber()) {
        PROPERTY_STORE(qreal, qreal(value->ToNumber()->Value()));
    } else if (property->propType == QMetaType::Float && value->IsNumber()) {
        PROPERTY_STORE(float, float(value->ToNumber()->Value()));
    } else if (property->propType == QMetaType::Double && value->IsNumber()) {
        PROPERTY_STORE(double, double(value->ToNumber()->Value()));
    } else if (property->propType == QMetaType::QString && value->IsString()) {
        PROPERTY_STORE(QString, engine->toString(value->ToString()));
    } else {
        QVariant v;
        if (property->isQList()) 
            v = engine->toVariant(value, qMetaTypeId<QList<QObject *> >());
        else
            v = engine->toVariant(value, property->propType);

        QDeclarativeContextData *context = engine->callingContext();

        if (!QDeclarativePropertyPrivate::write(object, *property, v, context)) {
            const char *valueType = 0;
            if (v.userType() == QVariant::Invalid) valueType = "null";
            else valueType = QMetaType::typeName(v.userType());

            QString error = QLatin1String("Cannot assign ") +
                            QLatin1String(valueType) +
                            QLatin1String(" to ") +
                            QLatin1String(QMetaType::typeName(property->propType));
            v8::ThrowException(v8::Exception::Error(engine->toString(error)));
        }
    }
}

bool QV8QObjectWrapper::SetProperty(QV8Engine *engine, QObject *object, v8::Handle<v8::String> property,
                                    v8::Handle<v8::Value> value, QV8QObjectWrapper::RevisionMode revisionMode)
{
    if (engine->qobjectWrapper()->m_toStringSymbol->StrictEquals(property) ||
        engine->qobjectWrapper()->m_destroySymbol->StrictEquals(property))
        return true;

    QDeclarativePropertyCache::Data local;
    QDeclarativePropertyCache::Data *result = 0;
    result = QDeclarativePropertyCache::property(engine->engine(), object, property, local);

    if (!result)
        return false;

    if (revisionMode == QV8QObjectWrapper::CheckRevision && result->revision != 0) {
        QDeclarativeData *ddata = QDeclarativeData::get(object);
        if (ddata && ddata->propertyCache && !ddata->propertyCache->isAllowedInRevision(result))
            return false;
    }

    if (!result->isWritable() && !result->isQList()) {
        QString error = QLatin1String("Cannot assign to read-only property \"") +
                        engine->toString(property) + QLatin1Char('\"');
        v8::ThrowException(v8::Exception::Error(engine->toString(error)));
        return true;
    }

    StoreProperty(engine, object, result, value);

    return true;
}

v8::Handle<v8::Value> QV8QObjectWrapper::Getter(v8::Local<v8::String> property, 
                                                const v8::AccessorInfo &info)
{
    QV8QObjectResource *resource = v8_resource_cast<QV8QObjectResource>(info.This());
    v8::Handle<v8::Value> This = info.This();

    if (!resource || resource->object.isNull()) return v8::Undefined();

    QObject *object = resource->object;

    QV8Engine *v8engine = resource->engine;
    v8::Handle<v8::Value> result = GetProperty(v8engine, object, &This, property, QV8QObjectWrapper::IgnoreRevision);
    if (!result.IsEmpty())
        return result;

    if (QV8Engine::startsWithUpper(property)) {
        // Check for attached properties
        QDeclarativeContextData *context = v8engine->callingContext();
        QDeclarativeTypeNameCache::Data *data = context && (context->imports)?context->imports->data(property):0;

        if (data) {
            if (data->type) {
                return v8engine->typeWrapper()->newObject(object, data->type, QV8TypeWrapper::ExcludeEnums);
            } else if (data->typeNamespace) {
                return v8engine->typeWrapper()->newObject(object, data->typeNamespace, QV8TypeWrapper::ExcludeEnums);
            }
        }

        return v8::Undefined();
    } else {
        // XXX throw?
        return v8::Undefined();
    }
}

v8::Handle<v8::Value> QV8QObjectWrapper::Setter(v8::Local<v8::String> property, 
                                                v8::Local<v8::Value> value,
                                                const v8::AccessorInfo &info)
{
    QV8QObjectResource *resource = v8_resource_cast<QV8QObjectResource>(info.This());

    if (!resource || resource->object.isNull()) 
        return value;

    QObject *object = resource->object;

    QV8Engine *v8engine = resource->engine;
    bool result = SetProperty(v8engine, object, property, value, QV8QObjectWrapper::IgnoreRevision);

    if (!result) {
        QString error = QLatin1String("Cannot assign to non-existent property \"") +
                        v8engine->toString(property) + QLatin1Char('\"');
        v8::ThrowException(v8::Exception::Error(v8engine->toString(error)));
        return value;
    }

    return value;
}

v8::Handle<v8::Integer> QV8QObjectWrapper::Query(v8::Local<v8::String> property,
                                                 const v8::AccessorInfo &info)
{
    QV8QObjectResource *resource = v8_resource_cast<QV8QObjectResource>(info.This());

    if (!resource || resource->object.isNull()) 
        return v8::Handle<v8::Integer>();

    QV8Engine *engine = resource->engine;
    QObject *object = resource->object;

    QDeclarativePropertyCache::Data local;
    QDeclarativePropertyCache::Data *result = 0;
    result = QDeclarativePropertyCache::property(engine->engine(), object, property, local);

    if (!result)
        return v8::Handle<v8::Integer>();
    else if (!result->isWritable() && !result->isQList())
        return v8::Integer::New(v8::ReadOnly | v8::DontDelete);
    else
        return v8::Integer::New(v8::DontDelete);
}

v8::Handle<v8::Array> QV8QObjectWrapper::Enumerator(const v8::AccessorInfo &info)
{
    QV8QObjectResource *resource = v8_resource_cast<QV8QObjectResource>(info.This());

    if (!resource || resource->object.isNull()) 
        return v8::Array::New();

    QObject *object = resource->object;

    QStringList result;

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(resource->engine->engine());

    QDeclarativePropertyCache *cache = 0;
    QDeclarativeData *ddata = QDeclarativeData::get(object);
    if (ddata)
        cache = ddata->propertyCache;

    if (!cache) {
        cache = ep->cache(object);
        if (cache) {
            if (ddata) { cache->addref(); ddata->propertyCache = cache; }
        } else {
            // Not cachable - fall back to QMetaObject (eg. dynamic meta object)
            // XXX QDeclarativeOpenMetaObject has a cache, so this is suboptimal.
            // XXX This is a workaround for QTBUG-9420.
            const QMetaObject *mo = object->metaObject();
            int pc = mo->propertyCount();
            int po = mo->propertyOffset();
            for (int i=po; i<pc; ++i)
                result << QString::fromUtf8(mo->property(i).name());
        }
    } else {
        result = cache->propertyNames();
    }

    v8::Local<v8::Array> rv = v8::Array::New(result.count());

    for (int ii = 0; ii < result.count(); ++ii) 
        rv->Set(ii, resource->engine->toString(result.at(ii)));

    return rv;
}

FAST_VALUE_GETTER(QObject, QObject*, 0, resource->engine->newQObject);
FAST_VALUE_GETTER(Int, int, 0, v8::Integer::New);
FAST_VALUE_GETTER(Bool, bool, false, v8::Boolean::New);
FAST_VALUE_GETTER(QString, QString, QString(), resource->engine->toString);
FAST_VALUE_GETTER(UInt, uint, 0, v8::Integer::NewFromUnsigned);
FAST_VALUE_GETTER(Float, float, 0, v8::Number::New);
FAST_VALUE_GETTER(Double, double, 0, v8::Number::New);

static void FastValueSetter(v8::Local<v8::String>, v8::Local<v8::Value> value,
                            const v8::AccessorInfo& info)
{
    QV8QObjectResource *resource = v8_resource_cast<QV8QObjectResource>(info.This());

    if (!resource || resource->object.isNull()) 
        return; 

    QObject *object = resource->object;

    uint32_t data = info.Data()->Uint32Value(); 
    int index = data & 0x7FFF;  // So that we can use the same data for Setter and Getter

    QDeclarativeData *ddata = QDeclarativeData::get(object, false);
    Q_ASSERT(ddata);
    Q_ASSERT(ddata->propertyCache);

    QDeclarativePropertyCache::Data *pdata = ddata->propertyCache->property(index);
    Q_ASSERT(pdata);

    Q_ASSERT(pdata->isWritable() || pdata->isQList());

    StoreProperty(resource->engine, object, pdata, value);
}

static void WeakQObjectReferenceCallback(v8::Persistent<v8::Value> handle, void *)
{
    Q_ASSERT(handle->IsObject());
    
    QV8QObjectResource *resource = v8_resource_cast<QV8QObjectResource>(handle->ToObject());

    Q_ASSERT(resource);

    QObject *object = resource->object;
    if (object) {
        QDeclarativeData *ddata = QDeclarativeData::get(object, false);
        if (ddata) {
            ddata->v8object.Clear();
            if (!object->parent() && !ddata->indestructible)
                object->deleteLater();
        }
    }

    handle.Dispose();
}

static void WeakQObjectInstanceCallback(v8::Persistent<v8::Value> handle, void *data)
{
    QV8QObjectInstance *instance = (QV8QObjectInstance *)data;
    instance->v8object.Clear();
    handle.Dispose();
}

v8::Local<v8::Object> QDeclarativePropertyCache::newQObject(QObject *object, QV8Engine *engine)
{
    Q_ASSERT(object);

    Q_ASSERT(QDeclarativeData::get(object, false));
    Q_ASSERT(QDeclarativeData::get(object, false)->propertyCache == this);

    // Setup constructor
    if (constructor.IsEmpty()) {
        v8::Local<v8::FunctionTemplate> ft;

        QString toString = QLatin1String("toString");
        QString destroy = QLatin1String("destroy");

        // XXX Enables fast property accessors.  These more than double the property access 
        // performance, but the  cost of setting up this structure hasn't been measured so 
        // its not guarenteed that this is a win overall
        for (StringCache::ConstIterator iter = stringCache.begin(); iter != stringCache.end(); ++iter) {
            Data *property = *iter;
            if (property->isFunction() || !property->isWritable() ||
                property->coreIndex >= 0x7FFF || property->notifyIndex >= 0x7FFF || 
                property->coreIndex == 0)
                continue;

            v8::AccessorGetter fastgetter = 0;

            
            if (property->isQObject()) 
                fastgetter = QObjectValueGetter;
            else if (property->propType == QMetaType::Int || property->isEnum()) 
                fastgetter = IntValueGetter;
            else if (property->propType == QMetaType::Bool)
                fastgetter = BoolValueGetter;
            else if (property->propType == QMetaType::QString)
                fastgetter = QStringValueGetter;
            else if (property->propType == QMetaType::UInt)
                fastgetter = UIntValueGetter;
            else if (property->propType == QMetaType::Float) 
                fastgetter = FloatValueGetter;
            else if (property->propType == QMetaType::Double) 
                fastgetter = DoubleValueGetter;

            if (fastgetter) {
                int notifyIndex = property->notifyIndex;
                if (property->isConstant()) notifyIndex = 0;
                else if (notifyIndex == -1) notifyIndex = 0x7FFF;
                uint32_t data = (property->notifyIndex & 0x7FFF) << 16 | property->coreIndex;

                QString name = iter.key();
                if (name == toString || name == destroy)
                    continue;

                if (ft.IsEmpty()) {
                    ft = v8::FunctionTemplate::New();
                    ft->InstanceTemplate()->SetFallbackPropertyHandler(QV8QObjectWrapper::Getter, 
                                                                       QV8QObjectWrapper::Setter,
                                                                       QV8QObjectWrapper::Query, 
                                                                       0,
                                                                       QV8QObjectWrapper::Enumerator);
                    ft->InstanceTemplate()->SetHasExternalResource(true);
                }

                ft->InstanceTemplate()->SetAccessor(engine->toString(name), fastgetter, FastValueSetter,
                                                    v8::Integer::NewFromUnsigned(data));
            }
        }

        if (ft.IsEmpty()) {
            constructor = v8::Persistent<v8::Function>::New(engine->qobjectWrapper()->m_constructor);
        } else {
            ft->InstanceTemplate()->SetFallbackPropertyHandler(QV8QObjectWrapper::Getter, 
                                                               QV8QObjectWrapper::Setter,
                                                               QV8QObjectWrapper::Query, 
                                                               0,
                                                               QV8QObjectWrapper::Enumerator);
            ft->InstanceTemplate()->SetHasExternalResource(true);
            constructor = v8::Persistent<v8::Function>::New(ft->GetFunction());
        }
    }

    v8::Local<v8::Object> result = constructor->NewInstance();
    QV8QObjectResource *r = new QV8QObjectResource(engine, object);
    result->SetExternalResource(r);
    return result;
}

v8::Local<v8::Object> QV8QObjectWrapper::newQObject(QObject *object, QDeclarativeData *ddata, QV8Engine *engine)
{
    v8::Local<v8::Object> rv;

    if (ddata->propertyCache) {
        rv = ddata->propertyCache->newQObject(object, engine);
    } else {
        // XXX aakenned - NewInstance() is slow for our case
        rv = m_constructor->NewInstance(); 
        QV8QObjectResource *r = new QV8QObjectResource(engine, object);
        rv->SetExternalResource(r);
    }

    return rv;
}

/*
As V8 doesn't support an equality callback, for QObject's we have to return exactly the same
V8 handle for subsequent calls to newQObject for the same QObject.  To do this we have a two
pronged strategy:
   1. If there is no current outstanding V8 handle to the QObject, we create one and store a 
      persistent handle in QDeclarativeData::v8object.  We mark the QV8QObjectWrapper that 
      "owns" this handle by setting the QDeclarativeData::v8objectid to the id of this 
      QV8QObjectWrapper.
   2. If another QV8QObjectWrapper has create the handle in QDeclarativeData::v8object we create 
      an entry in the m_taintedObject hash where we store the handle and mark the object as 
      "tainted" in the QDeclarativeData::hasTaintedV8Object flag.
We have to mark the object as tainted to ensure that we search our m_taintedObject hash even
in the case that the original QV8QObjectWrapper owner of QDeclarativeData::v8object has 
released the handle.
*/
v8::Handle<v8::Value> QV8QObjectWrapper::newQObject(QObject *object)
{
    if (!object)
        return v8::Null();

    if (QObjectPrivate::get(object)->wasDeleted)
       return v8::Undefined();
    
    QDeclarativeData *ddata = QDeclarativeData::get(object, true);

    if (!ddata) 
        return v8::Undefined();

    if (ddata->v8objectid == m_id && !ddata->v8object.IsEmpty()) {
        // We own the v8object 
        return v8::Local<v8::Object>::New(ddata->v8object);
    } else if (ddata->v8object.IsEmpty() && 
               (ddata->v8objectid == m_id || // We own the QObject
                ddata->v8objectid == 0 ||    // No one owns the QObject
                !ddata->hasTaintedV8Object)) { // Someone else has used the QObject, but it isn't tainted

        v8::Local<v8::Object> rv = newQObject(object, ddata, m_engine);
        ddata->v8object = v8::Persistent<v8::Object>::New(rv);
        ddata->v8object.MakeWeak(0, WeakQObjectReferenceCallback);
        ddata->v8objectid = m_id;
        return rv;

    } else {

        // If this object is tainted, we have to check to see if it is in our
        // tainted object list
        TaintedHash::Iterator iter =
            ddata->hasTaintedV8Object?m_taintedObjects.find(object):m_taintedObjects.end();
        bool found = iter != m_taintedObjects.end();

        // If our tainted handle doesn't exist or has been collected, and there isn't
        // a handle in the ddata, we can assume ownership of the ddata->v8object
        if ((!found || (*iter)->v8object.IsEmpty()) && ddata->v8object.IsEmpty()) {
            v8::Local<v8::Object> rv = newQObject(object, ddata, m_engine);
            ddata->v8object = v8::Persistent<v8::Object>::New(rv);
            ddata->v8object.MakeWeak(0, WeakQObjectReferenceCallback);
            ddata->v8objectid = m_id;

            if (found) {
                delete (*iter);
                m_taintedObjects.erase(iter);
            }

            return rv;
        } else if (!found) {
            QV8QObjectInstance *instance = new QV8QObjectInstance(object, this);
            iter = m_taintedObjects.insert(object, instance);
            ddata->hasTaintedV8Object = true;
        }

        if ((*iter)->v8object.IsEmpty()) {
            v8::Local<v8::Object> rv = newQObject(object, ddata, m_engine);
            (*iter)->v8object = v8::Persistent<v8::Object>::New(rv);
            (*iter)->v8object.MakeWeak((*iter), WeakQObjectInstanceCallback);
        }

        return v8::Local<v8::Object>::New((*iter)->v8object);
    }
}

QPair<QObject *, int> QV8QObjectWrapper::ExtractQtMethod(QV8Engine *engine, v8::Handle<v8::Function> function)
{
    v8::ScriptOrigin origin = function->GetScriptOrigin();
    if (origin.ResourceName()->StrictEquals(engine->qobjectWrapper()->m_hiddenObject)) {

        // This is one of our special QObject method wrappers
        v8::Handle<v8::Value> args[] = { engine->qobjectWrapper()->m_hiddenObject };
        v8::Local<v8::Value> data = function->Call(engine->global(), 1, args);

        if (data->IsArray()) {
            v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(data);
            return qMakePair(engine->toQObject(array->Get(0)), array->Get(1)->Int32Value());
        } 

        // In theory this can't fall through, but I suppose V8 might run out of memory or something
    }

    return qMakePair((QObject *)0, -1);
}

struct QV8QObjectConnectionList : public QObject, public QDeclarativeGuard<QObject>
{
    QV8QObjectConnectionList(QObject *object, QV8Engine *engine);
    ~QV8QObjectConnectionList();

    struct Connection {
        v8::Persistent<v8::Object> thisObject;
        v8::Persistent<v8::Function> function;
    };

    QV8Engine *engine;

    typedef QHash<int, QList<Connection> > SlotHash;
    SlotHash slotHash;

    virtual void objectDestroyed(QObject *);
    virtual int qt_metacall(QMetaObject::Call, int, void **);
};

QV8QObjectConnectionList::QV8QObjectConnectionList(QObject *object, QV8Engine *engine)
: QDeclarativeGuard<QObject>(object), engine(engine)
{
}

QV8QObjectConnectionList::~QV8QObjectConnectionList()
{
    for (SlotHash::Iterator iter = slotHash.begin(); iter != slotHash.end(); ++iter) {
        QList<Connection> &connections = *iter;
        for (int ii = 0; ii < connections.count(); ++ii) {
            connections[ii].thisObject.Dispose();
            connections[ii].function.Dispose();
        }
    }
    slotHash.clear();
}

void QV8QObjectConnectionList::objectDestroyed(QObject *object)
{
    engine->qobjectWrapper()->m_connections.remove(object);
    delete this;
}

int QV8QObjectConnectionList::qt_metacall(QMetaObject::Call method, int index, void **metaArgs)
{
    if (method == QMetaObject::InvokeMetaMethod) {
        SlotHash::Iterator iter = slotHash.find(index);
        if (iter == slotHash.end())
            return -1;
        QList<Connection> &connections = *iter;
        if (connections.isEmpty())
            return -1;

        // XXX optimize
        QMetaMethod method = data()->metaObject()->method(index);
        Q_ASSERT(method.methodType() == QMetaMethod::Signal);
        QList<QByteArray> params = method.parameterTypes();

        v8::HandleScope handle_scope;
        v8::Context::Scope scope(engine->context());

        QVarLengthArray<v8::Handle<v8::Value> > args(params.count());
        int argCount = params.count();

        for (int ii = 0; ii < argCount; ++ii) {
            int type = QMetaType::type(params.at(ii).constData());
            if (type == qMetaTypeId<QVariant>()) {
                args[ii] = engine->fromVariant(*((QVariant *)metaArgs[ii + 1]));
            } else {
                args[ii] = engine->fromVariant(QVariant(type, metaArgs[ii + 1]));
            }
        }

        // XXX what if this list changes or this object is deleted during the calls?
        for (int ii = 0; ii < connections.count(); ++ii) {
            Connection &connection = connections[ii];
            if (connection.thisObject.IsEmpty()) {
                connection.function->Call(engine->global(), argCount, args.data());
            } else {
                connection.function->Call(connection.thisObject, argCount, args.data());
            }
        }
    } 

    return -1;
}

v8::Handle<v8::Value> QV8QObjectWrapper::Connect(const v8::Arguments &args)
{
    if (args.Length() == 0)
        V8THROW_ERROR("Function.prototype.connect: no arguments given");

    QV8Engine *engine = V8ENGINE();

    if (!args.This()->IsFunction())
        V8THROW_ERROR("Function.prototype.connect: this object is not a signal");

    QPair<QObject *, int> signalInfo = ExtractQtMethod(engine, v8::Handle<v8::Function>::Cast(args.This()));
    QObject *signalObject = signalInfo.first;
    int signalIndex = signalInfo.second;

    if (signalIndex == -1)
        V8THROW_ERROR("Function.prototype.connect: this object is not a signal");

    if (!signalObject)
        V8THROW_ERROR("Function.prototype.connect: cannot connect to deleted QObject");

    if (signalIndex < 0 || signalObject->metaObject()->method(signalIndex).methodType() != QMetaMethod::Signal)
        V8THROW_ERROR("Function.prototype.connect: this object is not a signal");

    v8::Local<v8::Value> functionValue;
    v8::Local<v8::Value> functionThisValue;

    if (args.Length() == 1) {
        functionValue = args[0];
    } else {
        functionThisValue = args[0];
        functionValue = args[1];
    }

    if (!functionValue->IsFunction())
        V8THROW_ERROR("Function.prototype.connect: target is not a function");

    if (!functionThisValue.IsEmpty() && !functionThisValue->IsObject())
        V8THROW_ERROR("Function.prototype.connect: target this is not an object");

    QV8QObjectWrapper *qobjectWrapper = engine->qobjectWrapper();
    QHash<QObject *, QV8QObjectConnectionList *> &connections = qobjectWrapper->m_connections;
    QHash<QObject *, QV8QObjectConnectionList *>::Iterator iter = connections.find(signalObject);
    if (iter == connections.end()) 
        iter = connections.insert(signalObject, new QV8QObjectConnectionList(signalObject, engine));

    QV8QObjectConnectionList *connectionList = *iter;
    QV8QObjectConnectionList::SlotHash::Iterator slotIter = connectionList->slotHash.find(signalIndex);
    if (slotIter == connectionList->slotHash.end()) {
        slotIter = connectionList->slotHash.insert(signalIndex, QList<QV8QObjectConnectionList::Connection>());
        QMetaObject::connect(signalObject, signalIndex, connectionList, signalIndex);
    }

    QV8QObjectConnectionList::Connection connection;
    if (!functionThisValue.IsEmpty()) 
        connection.thisObject = v8::Persistent<v8::Object>::New(functionThisValue->ToObject());
    connection.function = v8::Persistent<v8::Function>::New(v8::Handle<v8::Function>::Cast(functionValue));

    slotIter->append(connection);

    return v8::Undefined();
}

v8::Handle<v8::Value> QV8QObjectWrapper::Disconnect(const v8::Arguments &args)
{
    if (args.Length() == 0)
        V8THROW_ERROR("Function.prototype.disconnect: no arguments given");

    QV8Engine *engine = V8ENGINE();

    if (!args.This()->IsFunction())
        V8THROW_ERROR("Function.prototype.disconnect: this object is not a signal");

    QPair<QObject *, int> signalInfo = ExtractQtMethod(engine, v8::Handle<v8::Function>::Cast(args.This()));
    QObject *signalObject = signalInfo.first;
    int signalIndex = signalInfo.second;

    if (signalIndex == -1)
        V8THROW_ERROR("Function.prototype.disconnect: this object is not a signal");

    if (!signalObject)
        V8THROW_ERROR("Function.prototype.disconnect: cannot disconnect from deleted QObject");

    if (signalIndex < 0 || signalObject->metaObject()->method(signalIndex).methodType() != QMetaMethod::Signal)
        V8THROW_ERROR("Function.prototype.disconnect: this object is not a signal");

    v8::Local<v8::Value> functionValue;
    v8::Local<v8::Value> functionThisValue;

    if (args.Length() == 1) {
        functionValue = args[0];
    } else {
        functionThisValue = args[0];
        functionValue = args[1];
    }

    if (!functionValue->IsFunction())
        V8THROW_ERROR("Function.prototype.disconnect: target is not a function");

    if (!functionThisValue.IsEmpty() && !functionThisValue->IsObject())
        V8THROW_ERROR("Function.prototype.disconnect: target this is not an object");

    QV8QObjectWrapper *qobjectWrapper = engine->qobjectWrapper();
    QHash<QObject *, QV8QObjectConnectionList *> &connectionsList = qobjectWrapper->m_connections;
    QHash<QObject *, QV8QObjectConnectionList *>::Iterator iter = connectionsList.find(signalObject);
    if (iter == connectionsList.end()) 
        return v8::Undefined(); // Nothing to disconnect from

    QV8QObjectConnectionList *connectionList = *iter;
    QV8QObjectConnectionList::SlotHash::Iterator slotIter = connectionList->slotHash.find(signalIndex);
    if (slotIter == connectionList->slotHash.end()) 
        return v8::Undefined(); // Nothing to disconnect from

    QList<QV8QObjectConnectionList::Connection> &connections = *slotIter;

    v8::Local<v8::Function> function = v8::Local<v8::Function>::Cast(functionValue);
    QPair<QObject *, int> functionData = ExtractQtMethod(engine, function);

    if (functionData.second != -1) {
        // This is a QObject function wrapper
        for (int ii = 0; ii < connections.count(); ++ii) {
            QV8QObjectConnectionList::Connection &connection = connections[ii];

            if (connection.thisObject.IsEmpty() == functionThisValue.IsEmpty() &&
                (connection.thisObject.IsEmpty() || connection.thisObject->StrictEquals(functionThisValue))) {

                QPair<QObject *, int> connectedFunctionData = ExtractQtMethod(engine, connection.function);
                if (connectedFunctionData == functionData) {
                    // Match!
                    connection.thisObject.Dispose();
                    connection.function.Dispose();
                    connections.removeAt(ii);
                    return v8::Undefined();
                }
            }
        }

    } else {
        // This is a normal JS function
        for (int ii = 0; ii < connections.count(); ++ii) {
            QV8QObjectConnectionList::Connection &connection = connections[ii];
            if (connection.function->StrictEquals(function) &&
                connection.thisObject.IsEmpty() == functionThisValue.IsEmpty() &&
                (connection.thisObject.IsEmpty() || connection.thisObject->StrictEquals(functionThisValue))) {
                // Match!
                connection.thisObject.Dispose();
                connection.function.Dispose();
                connections.removeAt(ii);
                return v8::Undefined();
            }
        }
    }

    return v8::Undefined();
}

/*!
    Get the \a property of \a object.  Returns an empty handle if the property doesn't exist.

    Only searches for real properties of \a object (including methods), not attached properties etc.
*/
v8::Handle<v8::Value> QV8QObjectWrapper::getProperty(QObject *object, v8::Handle<v8::String> property,
                                                     QV8QObjectWrapper::RevisionMode revisionMode)
{
    return GetProperty(m_engine, object, 0, property, revisionMode);
}

/*
    Set the \a property of \a object to \a value.

    Returns true if the property was "set" - even if this results in an exception being thrown -
    and false if the object has no such property.

    Only searches for real properties of \a object (including methods), not attached properties etc.
*/
bool QV8QObjectWrapper::setProperty(QObject *object, v8::Handle<v8::String> property, 
                                    v8::Handle<v8::Value> value, RevisionMode revisionMode)
{
    return SetProperty(m_engine, object, property, value, revisionMode);
}

namespace {
struct CallArgs
{
    CallArgs(int length, v8::Handle<v8::Object> *args) : _length(length), _args(args) {}
    int Length() const { return _length; }
    v8::Local<v8::Value> operator[](int idx) { return (*_args)->Get(idx); }

private:
    int _length;
    v8::Handle<v8::Object> *_args;
};
}

static v8::Handle<v8::Value> CallMethod(QObject *object, int index, int returnType, int argCount, 
                                        int *argTypes, QV8Engine *engine, CallArgs &callArgs)
{
    if (argCount > 0) {

        QVarLengthArray<MetaCallArgument, 9> args(argCount + 1);
        args[0].initAsType(returnType);

        for (int ii = 0; ii < argCount; ++ii)
            args[ii + 1].fromValue(argTypes[ii], engine, callArgs[ii]);

        QVarLengthArray<void *, 9> argData(args.count());
        for (int ii = 0; ii < args.count(); ++ii)
            argData[ii] = args[ii].dataPtr();

        QMetaObject::metacall(object, QMetaObject::InvokeMetaMethod, index, argData.data());

        return args[0].toValue(engine);

    } else if (returnType != 0) {
        
        MetaCallArgument arg;
        arg.initAsType(returnType);

        void *args[] = { arg.dataPtr() };

        QMetaObject::metacall(object, QMetaObject::InvokeMetaMethod, index, args);

        return arg.toValue(engine);

    } else {

        void *args[] = { 0 };
        QMetaObject::metacall(object, QMetaObject::InvokeMetaMethod, index, args);
        return v8::Undefined();

    }
}

static int EnumType(const QMetaObject *meta, const QString &strname)
{
    QByteArray str = strname.toUtf8();
    QByteArray scope;
    QByteArray name;
    int scopeIdx = str.lastIndexOf("::");
    if (scopeIdx != -1) {
        scope = str.left(scopeIdx);
        name = str.mid(scopeIdx + 2);
    } else { 
        name = str;
    }
    for (int i = meta->enumeratorCount() - 1; i >= 0; --i) {
        QMetaEnum m = meta->enumerator(i);
        if ((m.name() == name) && (scope.isEmpty() || (m.scope() == scope)))
            return QVariant::Int;
    }
    return QVariant::Invalid;
}

/*!
    Returns the match score for converting \a actual to be of type \a conversionType.  A 
    zero score means "perfect match" whereas a higher score is worse.

    The conversion table is copied out of the QtScript callQtMethod() function.
*/
static int MatchScore(v8::Handle<v8::Value> actual, int conversionType, 
                      const QByteArray &conversionTypeName)
{
    if (actual->IsNumber()) {
        switch (conversionType) {
        case QMetaType::Double:
            return 0;
        case QMetaType::Float:
            return 1;
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
            return 2;
        case QMetaType::Long:
        case QMetaType::ULong:
            return 3;
        case QMetaType::Int:
        case QMetaType::UInt:
            return 4;
        case QMetaType::Short:
        case QMetaType::UShort:
            return 5;
            break;
        case QMetaType::Char:
        case QMetaType::UChar:
            return 6;
        default:
            return 10;
        }
    } else if (actual->IsString()) {
        switch (conversionType) {
        case QMetaType::QString:
            return 0;
        default:
            return 10;
        }
    } else if (actual->IsBoolean()) {
        switch (conversionType) {
        case QMetaType::Bool:
            return 0;
        default:
            return 10;
        }
    } else if (actual->IsDate()) {
        switch (conversionType) {
        case QMetaType::QDateTime:
            return 0;
        case QMetaType::QDate:
            return 1;
        case QMetaType::QTime:
            return 2;
        default:
            return 10;
        }
    } else if (actual->IsRegExp()) {
        switch (conversionType) {
        case QMetaType::QRegExp:
            return 0;
        default:
            return 10;
        }
    } else if (actual->IsArray()) {
        switch (conversionType) {
        case QMetaType::QStringList:
        case QMetaType::QVariantList:
            return 5;
        default:
            return 10;
        }
    } else if (actual->IsNull()) {
        switch (conversionType) {
        case QMetaType::VoidStar:
        case QMetaType::QObjectStar:
            return 0;
        default:
            if (!conversionTypeName.endsWith('*'))
                return 10;
            else
                return 0;
        }
    } else if (actual->IsObject()) {
        v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(actual);

        QV8ObjectResource *r = static_cast<QV8ObjectResource *>(obj->GetExternalResource());
        if (r && r->resourceType() == QV8ObjectResource::QObjectType) {
            switch (conversionType) {
            case QMetaType::QObjectStar:
                return 0;
            default:
                return 10;
            }
        } else if (r && r->resourceType() == QV8ObjectResource::VariantType) {
            if (conversionType == qMetaTypeId<QVariant>())
                return 0;
            else if (r->engine->toVariant(actual, -1).userType() == conversionType)
                return 0;
            else
                return 10;
        } else {
            return 10;
        }

    } else {
        return 10;
    }
}

static inline int QMetaObject_methods(const QMetaObject *metaObject)
{
    struct Private
    {
        int revision;
        int className;
        int classInfoCount, classInfoData;
        int methodCount, methodData;
    };

    return reinterpret_cast<const Private *>(metaObject->d.data)->methodCount;
}

static QByteArray QMetaMethod_name(const QMetaMethod &m)
{
    QByteArray sig = m.signature();
    int paren = sig.indexOf('(');
    if (paren == -1)
        return sig;
    else
        return sig.left(paren);
}

/*!
Returns the next related method, if one, or 0.
*/
static const QDeclarativePropertyCache::Data * RelatedMethod(QObject *object, 
                                                             const QDeclarativePropertyCache::Data *current, 
                                                             QDeclarativePropertyCache::Data &dummy)
{
    QDeclarativePropertyCache *cache = QDeclarativeData::get(object)->propertyCache;
    if (current->relatedIndex == -1)
        return 0;

    if (cache) {
        return cache->method(current->relatedIndex);
    } else {
        const QMetaObject *mo = object->metaObject();
        int methodOffset = mo->methodCount() - QMetaObject_methods(mo);

        while (methodOffset > current->relatedIndex) {
            mo = mo->superClass();
            methodOffset -= QMetaObject_methods(mo);
        }

        QMetaMethod method = mo->method(current->relatedIndex);
        dummy.load(method);
        
        // Look for overloaded methods
        QByteArray methodName = QMetaMethod_name(method);
        for (int ii = current->relatedIndex - 1; ii >= methodOffset; --ii) {
            if (methodName == QMetaMethod_name(mo->method(ii))) {
                dummy.relatedIndex = ii;
                return &dummy;
            }
        }

        return &dummy;
    }
}

static v8::Handle<v8::Value> CallPrecise(QObject *object, const QDeclarativePropertyCache::Data &data, 
                                         QV8Engine *engine, CallArgs &callArgs)
{
    if (data.flags & QDeclarativePropertyCache::Data::HasArguments) {

        QMetaMethod m = object->metaObject()->method(data.coreIndex);
        QList<QByteArray> argTypeNames = m.parameterTypes();
        QVarLengthArray<int, 9> argTypes(argTypeNames.count());

        // ### Cache
        for (int ii = 0; ii < argTypeNames.count(); ++ii) {
            argTypes[ii] = QMetaType::type(argTypeNames.at(ii));
            if (argTypes[ii] == QVariant::Invalid) 
                argTypes[ii] = EnumType(object->metaObject(), QString::fromLatin1(argTypeNames.at(ii)));
            if (argTypes[ii] == QVariant::Invalid) {
                QString error = QString::fromLatin1("Unknown method parameter type: %1").arg(QLatin1String(argTypeNames.at(ii)));
                v8::ThrowException(v8::Exception::Error(engine->toString(error)));
                return v8::Handle<v8::Value>();
            }
        }

        if (argTypes.count() > callArgs.Length()) {
            QString error = QLatin1String("Insufficient arguments");
            v8::ThrowException(v8::Exception::Error(engine->toString(error)));
            return v8::Handle<v8::Value>();
        }

        return CallMethod(object, data.coreIndex, data.propType, argTypes.count(), 
                          argTypes.data(), engine, callArgs);

    } else {

        return CallMethod(object, data.coreIndex, data.propType, 0, 0, engine, callArgs);

    }
}

/*!
Resolve the overloaded method to call.  The algorithm works conceptually like this:
    1.  Resolve the set of overloads it is *possible* to call.
        Impossible overloads include those that have too many parameters or have parameters 
        of unknown type.  
    2.  Filter the set of overloads to only contain those with the closest number of 
        parameters.
        For example, if we are called with 3 parameters and there are 2 overloads that
        take 2 parameters and one that takes 3, eliminate the 2 parameter overloads.
    3.  Find the best remaining overload based on its match score.  
        If two or more overloads have the same match score, call the last one.  The match
        score is constructed by adding the matchScore() result for each of the parameters.
*/
static v8::Handle<v8::Value> CallOverloaded(QObject *object, const QDeclarativePropertyCache::Data &data, 
                                            QV8Engine *engine, CallArgs &callArgs)
{
    int argumentCount = callArgs.Length();

    const QDeclarativePropertyCache::Data *best = 0;
    int bestParameterScore = INT_MAX;
    int bestMatchScore = INT_MAX;

    QDeclarativePropertyCache::Data dummy;
    const QDeclarativePropertyCache::Data *attempt = &data;

    do {
        QList<QByteArray> methodArgTypeNames;

        if (attempt->flags & QDeclarativePropertyCache::Data::HasArguments)
            methodArgTypeNames = object->metaObject()->method(attempt->coreIndex).parameterTypes();

        int methodArgumentCount = methodArgTypeNames.count();

        if (methodArgumentCount > argumentCount)
            continue; // We don't have sufficient arguments to call this method

        int methodParameterScore = argumentCount - methodArgumentCount;
        if (methodParameterScore > bestParameterScore)
            continue; // We already have a better option

        int methodMatchScore = 0;
        QVarLengthArray<int, 9> methodArgTypes(methodArgumentCount);

        bool unknownArgument = false;
        for (int ii = 0; ii < methodArgumentCount; ++ii) {
            methodArgTypes[ii] = QMetaType::type(methodArgTypeNames.at(ii));
            if (methodArgTypes[ii] == QVariant::Invalid) 
                methodArgTypes[ii] = EnumType(object->metaObject(), 
                                              QString::fromLatin1(methodArgTypeNames.at(ii)));
            if (methodArgTypes[ii] == QVariant::Invalid) {
                unknownArgument = true;
                break;
            }
            methodMatchScore += MatchScore(callArgs[ii], methodArgTypes[ii], methodArgTypeNames.at(ii));
        }
        if (unknownArgument)
            continue; // We don't understand all the parameters

        if (bestParameterScore > methodParameterScore || bestMatchScore > methodMatchScore) {
            best = attempt;
            bestParameterScore = methodParameterScore;
            bestMatchScore = methodMatchScore;
        }

        if (bestParameterScore == 0 && bestMatchScore == 0)
            break; // We can't get better than that

    } while((attempt = RelatedMethod(object, attempt, dummy)) != 0);

    if (best) {
        return CallPrecise(object, *best, engine, callArgs);
    } else {
        QString error = QLatin1String("Unable to determine callable overload.  Candidates are:");
        const QDeclarativePropertyCache::Data *candidate = &data;
        while (candidate) {
            error += QLatin1String("\n    ") + 
                     QString::fromUtf8(object->metaObject()->method(candidate->coreIndex).signature());
            candidate = RelatedMethod(object, candidate, dummy);
        }

        v8::ThrowException(v8::Exception::Error(engine->toString(error)));
        return v8::Handle<v8::Value>();
    }
}

static v8::Handle<v8::Value> ToString(QV8Engine *engine, QObject *object, int, v8::Handle<v8::Object>)
{
    QString result;
    if (object) {
        QString objectName = object->objectName();

        result += QString::fromUtf8(object->metaObject()->className());
        result += QLatin1String("(0x");
        result += QString::number((quintptr)object,16);

        if (!objectName.isEmpty()) {
            result += QLatin1String(", \"");
            result += objectName;
            result += QLatin1Char('\"');
        }

        result += QLatin1Char(')');
    } else {
        result = QLatin1String("null");
    }

    return engine->toString(result);
}

static v8::Handle<v8::Value> Destroy(QV8Engine *, QObject *object, int argCount, v8::Handle<v8::Object> args)
{
    QDeclarativeData *ddata = QDeclarativeData::get(object, false);
    if (!ddata || ddata->indestructible) {
        const char *error = "Invalid attempt to destroy() an indestructible object";
        v8::ThrowException(v8::Exception::Error(v8::String::New(error)));
        return v8::Undefined();
    }

    int delay = 0;
    if (argCount > 0)
        delay = args->Get(0)->Uint32Value();

    if (delay > 0)
        QTimer::singleShot(delay, object, SLOT(deleteLater()));
    else
        object->deleteLater();

    return v8::Undefined();
}

v8::Handle<v8::Value> QV8QObjectWrapper::Invoke(const v8::Arguments &args)
{
    // object, index, qmlglobal, argCount, args
    Q_ASSERT(args.Length() == 5);
    Q_ASSERT(args[0]->IsObject());

    QV8QObjectResource *resource = v8_resource_cast<QV8QObjectResource>(args[0]->ToObject());

    if (!resource)
        return v8::Undefined();

    int argCount = args[3]->Int32Value();
    v8::Handle<v8::Object> arguments = v8::Handle<v8::Object>::Cast(args[4]);

    // Special hack to return info about this closure.
    if (argCount == 1 && arguments->Get(0)->StrictEquals(resource->engine->qobjectWrapper()->m_hiddenObject)) {
        v8::Local<v8::Array> data = v8::Array::New(2);
        data->Set(0, args[0]);
        data->Set(1, args[1]);
        return data;
    }

    QObject *object = resource->object;
    int index = args[1]->Int32Value();

    if (!object)
        return v8::Undefined();

    if (index < 0) {
        // Builtin functions
        if (index == QOBJECT_TOSTRING_INDEX) {
            return ToString(resource->engine, object, argCount, arguments);
        } else if (index == QOBJECT_DESTROY_INDEX) {
            return Destroy(resource->engine, object, argCount, arguments);
        } else {
            return v8::Undefined();
        }
    }

    QDeclarativePropertyCache::Data method;

    if (QDeclarativeData *ddata = static_cast<QDeclarativeData *>(QObjectPrivate::get(object)->declarativeData)) {
        if (ddata->propertyCache) {
            QDeclarativePropertyCache::Data *d = ddata->propertyCache->method(index);
            if (!d) 
                return v8::Undefined();
            method = *d;
        } 
    }

    if (method.coreIndex == -1) {
        QMetaMethod mm = object->metaObject()->method(index);
        method.load(object->metaObject()->method(index));

        if (method.coreIndex == -1)
            return v8::Undefined();
    }

    if (method.flags & QDeclarativePropertyCache::Data::IsV8Function) {
        v8::Handle<v8::Value> rv;
        v8::Handle<v8::Object> qmlglobal = args[2]->ToObject();

        QDeclarativeV8Function func(argCount, arguments, rv, qmlglobal, 
                                    resource->engine->contextWrapper()->context(qmlglobal),
                                    resource->engine);
        QDeclarativeV8Function *funcptr = &func;

        void *args[] = { 0, &funcptr };
        QMetaObject::metacall(object, QMetaObject::InvokeMetaMethod, method.coreIndex, args);

        if (rv.IsEmpty()) return v8::Undefined();
        return rv;
    }

    CallArgs callArgs(argCount, &arguments);
    if (method.relatedIndex == -1) {
        return CallPrecise(object, method, resource->engine, callArgs);
    } else {
        return CallOverloaded(object, method, resource->engine, callArgs);
    }
}

MetaCallArgument::MetaCallArgument()
: type(QVariant::Invalid), isObjectType(false)
{
}

MetaCallArgument::~MetaCallArgument()
{
    cleanup();
}

void MetaCallArgument::cleanup()
{
    if (type == QMetaType::QString) {
        ((QString *)&data)->~QString();
    } else if (type == -1 || type == qMetaTypeId<QVariant>()) {
        ((QVariant *)&data)->~QVariant();
    } else if (type == qMetaTypeId<QScriptValue>()) {
        ((QScriptValue *)&data)->~QScriptValue();
    } else if (type == qMetaTypeId<QList<QObject *> >()) {
        ((QList<QObject *> *)&data)->~QList<QObject *>();
    }
}

void *MetaCallArgument::dataPtr()
{
    if (type == -1)
        return ((QVariant *)data)->data();
    else
        return (void *)&data;
}

void MetaCallArgument::initAsType(int callType)
{
    if (type != 0) { cleanup(); type = 0; }
    if (callType == 0) return;

    if (callType == qMetaTypeId<QScriptValue>()) {
        new (&data) QScriptValue();
        type = callType;
    } else if (callType == QMetaType::Int ||
               callType == QMetaType::UInt ||
               callType == QMetaType::Bool ||
               callType == QMetaType::Double ||
               callType == QMetaType::Float) {
        type = callType;
    } else if (callType == QMetaType::QObjectStar) {
        *((QObject **)&data) = 0;
        type = callType;
    } else if (callType == QMetaType::QString) {
        new (&data) QString();
        type = callType;
    } else if (callType == qMetaTypeId<QVariant>()) {
        type = callType;
        new (&data) QVariant();
    } else if (callType == qMetaTypeId<QList<QObject *> >()) {
        type = callType;
        new (&data) QList<QObject *>();
    } else if (callType == qMetaTypeId<QDeclarativeV8Handle>()) {
        type = callType;
        new (&data) v8::Handle<v8::Value>();
    } else {
        type = -1;
        new (&data) QVariant(callType, (void *)0);
    }
}

void MetaCallArgument::fromValue(int callType, QV8Engine *engine, v8::Handle<v8::Value> value)
{
    if (type != 0) { cleanup(); type = 0; }

    if (callType == qMetaTypeId<QScriptValue>()) {
        new (&data) QScriptValue();
        type = qMetaTypeId<QScriptValue>();
    } else if (callType == QMetaType::Int) {
        *((int *)&data) = int(value->Int32Value());
        type = callType;
    } else if (callType == QMetaType::UInt) {
        *((uint *)&data) = uint(value->Uint32Value());
        type = callType;
    } else if (callType == QMetaType::Bool) {
        *((bool *)&data) = value->BooleanValue();
        type = callType;
    } else if (callType == QMetaType::Double) {
        *((double *)&data) = double(value->NumberValue());
        type = callType;
    } else if (callType == QMetaType::Float) {
        *((float *)&data) = float(value->NumberValue());
        type = callType;
    } else if (callType == QMetaType::QString) {
        if (value->IsNull() || value->IsUndefined())
            new (&data) QString();
        else
            new (&data) QString(engine->toString(value->ToString()));
        type = callType;
    } else if (callType == QMetaType::QObjectStar) {
        *((QObject **)&data) = engine->toQObject(value);
        type = callType;
    } else if (callType == qMetaTypeId<QVariant>()) {
        new (&data) QVariant(engine->toVariant(value, -1));
        type = callType;
    } else if (callType == qMetaTypeId<QList<QObject*> >()) {
        QList<QObject *> *list = new (&data) QList<QObject *>(); 
        if (value->IsArray()) {
            v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(value);
            uint32_t length = array->Length();
            for (uint32_t ii = 0; ii < length; ++ii) 
                list->append(engine->toQObject(array->Get(ii)));
        } else {
            list->append(engine->toQObject(value));
        }
        type = callType;
    } else if (callType == qMetaTypeId<QDeclarativeV8Handle>()) {
        new (&data) v8::Handle<v8::Value>(value);
        type = callType;
    } else {
        new (&data) QVariant();
        type = -1;

        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine->engine());
        QVariant v = engine->toVariant(value, -1);

        if (v.userType() == callType) {
            *((QVariant *)&data) = v;
        } else if (v.canConvert((QVariant::Type)callType)) {
            *((QVariant *)&data) = v;
            ((QVariant *)&data)->convert((QVariant::Type)callType);
        } else if (const QMetaObject *mo = ep->rawMetaObjectForType(callType)) {
            QObject *obj = ep->toQObject(v);
            
            if (obj) {
                const QMetaObject *objMo = obj->metaObject();
                while (objMo && objMo != mo) objMo = objMo->superClass();
                if (!objMo) obj = 0;
            }

            *((QVariant *)&data) = QVariant(callType, &obj);
        } else {
            *((QVariant *)&data) = QVariant(callType, (void *)0);
        }
    }
}

v8::Handle<v8::Value> MetaCallArgument::toValue(QV8Engine *engine)
{
    if (type == qMetaTypeId<QScriptValue>()) {
        return v8::Undefined();
    } else if (type == QMetaType::Int) {
        return v8::Integer::New(*((int *)&data));
    } else if (type == QMetaType::UInt) {
        return v8::Integer::NewFromUnsigned(*((uint *)&data));
    } else if (type == QMetaType::Bool) {
        return v8::Boolean::New(*((bool *)&data));
    } else if (type == QMetaType::Double) {
        return v8::Number::New(*((double *)&data));
    } else if (type == QMetaType::Float) {
        return v8::Number::New(*((float *)&data));
    } else if (type == QMetaType::QString) {
        return engine->toString(*((QString *)&data));
    } else if (type == QMetaType::QObjectStar) {
        QObject *object = *((QObject **)&data);
        if (object)
            QDeclarativeData::get(object, true)->setImplicitDestructible();
        return engine->newQObject(object);
    } else if (type == qMetaTypeId<QList<QObject *> >()) {
        // XXX aakenned Can this be more optimal?  Just use Array as a prototype and 
        // implement directly against QList<QObject*>?
        QList<QObject *> &list = *(QList<QObject *>*)&data;
        v8::Local<v8::Array> array = v8::Array::New(list.count());
        for (int ii = 0; ii < list.count(); ++ii) 
            array->Set(ii, engine->newQObject(list.at(ii)));
        return array;
    } else if (type == qMetaTypeId<QDeclarativeV8Handle>()) {
        return *(v8::Handle<v8::Value>*)&data;
    } else if (type == -1 || type == qMetaTypeId<QVariant>()) {
        QVariant value = *((QVariant *)&data);
        v8::Handle<v8::Value> rv = engine->fromVariant(value);
        if (QObject *object = engine->toQObject(rv)) 
            QDeclarativeData::get(object, true)->setImplicitDestructible();
        return rv;
    } else {
        return v8::Undefined();
    }
}
