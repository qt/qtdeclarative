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

#include "qv8qobjectwrapper_p.h"
#include "qv8contextwrapper_p.h"
#include "qv8engine_p.h"

#include <private/qqmlguard_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qjsvalue_p.h>
#include <private/qscript_impl_p.h>
#include <private/qqmlaccessors_p.h>
#include <private/qqmlexpression_p.h>
#include <private/qqmlglobal_p.h>

#include <QtQml/qjsvalue.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qtimer.h>
#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
# if (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
// The code in this file does not violate strict aliasing, but GCC thinks it does
// so turn off the warnings for us to have a clean build
#  pragma GCC diagnostic ignored "-Wstrict-aliasing"
# endif
#endif

#define QOBJECT_TOSTRING_INDEX -2
#define QOBJECT_DESTROY_INDEX -3

// XXX TODO: Need to review all calls to QQmlEngine *engine() to confirm QObjects work
// correctly in a worker thread

class QV8QObjectInstance : public QQmlGuard<QObject>
{
public:
    QV8QObjectInstance(QObject *o, QV8QObjectWrapper *w)
    : QQmlGuard<QObject>(o), wrapper(w)
    {
    }

    ~QV8QObjectInstance()
    {
        qPersistentDispose(v8object);
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

class QV8SignalHandlerResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(SignalHandlerType)
public:
    QV8SignalHandlerResource(QV8Engine *engine, QObject *object, int index);

    QQmlGuard<QObject> object;
    int index;
};

namespace {

template<typename A, typename B, typename C, typename D, typename E,
         typename F, typename G, typename H>
class MaxSizeOf8 {
    template<typename Z, typename X>
    struct SMax {
        char dummy[sizeof(Z) > sizeof(X) ? sizeof(Z) : sizeof(X)];
    };
public:
    static const size_t Size = sizeof(SMax<A, SMax<B, SMax<C, SMax<D, SMax<E, SMax<F, SMax<G, H> > > > > > >);
};

struct CallArgument {
    inline CallArgument();
    inline ~CallArgument();
    inline void *dataPtr();

    inline void initAsType(int type);
    inline void fromValue(int type, QV8Engine *, v8::Handle<v8::Value>);
    inline v8::Handle<v8::Value> toValue(QV8Engine *);

private:
    CallArgument(const CallArgument &);

    inline void cleanup();

    union {
        float floatValue;
        double doubleValue;
        quint32 intValue;
        bool boolValue;
        QObject *qobjectPtr;

        char allocData[MaxSizeOf8<QVariant,
                                QString,
                                QList<QObject *>,
                                QJSValue,
                                QQmlV8Handle,
                                QJsonArray,
                                QJsonObject,
                                QJsonValue>::Size];
        qint64 q_for_alignment;
    };

    // Pointers to allocData
    union {
        QString *qstringPtr;
        QVariant *qvariantPtr;
        QList<QObject *> *qlistPtr;
        QJSValue *qjsValuePtr;
        QQmlV8Handle *handlePtr;
        QJsonArray *jsonArrayPtr;
        QJsonObject *jsonObjectPtr;
        QJsonValue *jsonValuePtr;
    };

    int type;
};
}

QV8QObjectResource::QV8QObjectResource(QV8Engine *engine, QObject *object) 
: QV8ObjectResource(engine), object(object) 
{
}

QV8SignalHandlerResource::QV8SignalHandlerResource(QV8Engine *engine, QObject *object, int index)
: QV8ObjectResource(engine), object(object), index(index)
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

    qPersistentDispose(m_hiddenObject);
    qPersistentDispose(m_destroySymbol);
    qPersistentDispose(m_toStringSymbol);
    qPersistentDispose(m_signalHandlerConstructor);
    qPersistentDispose(m_methodConstructor);
    qPersistentDispose(m_constructor);

    QIntrusiveList<QV8QObjectResource, &QV8QObjectResource::weakResource>::iterator i = m_javaScriptOwnedWeakQObjects.begin();
    for (; i != m_javaScriptOwnedWeakQObjects.end(); ++i) {
        QV8QObjectResource *resource = *i;
        Q_ASSERT(resource);
        deleteWeakQObject(resource, true);
    }
}

struct ReadAccessor {
    static inline void Indirect(QObject *object, const QQmlPropertyData &property,
                                void *output, QQmlNotifier **n)
    {
        Q_ASSERT(n == 0);
        Q_UNUSED(n);

        void *args[] = { output, 0 };
        QMetaObject::metacall(object, QMetaObject::ReadProperty, property.coreIndex, args);
    }

    static inline void Direct(QObject *object, const QQmlPropertyData &property,
                              void *output, QQmlNotifier **n)
    {
        Q_ASSERT(n == 0);
        Q_UNUSED(n);

        void *args[] = { output, 0 };
        object->qt_metacall(QMetaObject::ReadProperty, property.coreIndex, args);
    }

    static inline void Accessor(QObject *object, const QQmlPropertyData &property,
                                void *output, QQmlNotifier **n)
    {
        Q_ASSERT(property.accessors);

        property.accessors->read(object, property.accessorData, output);
        if (n) property.accessors->notifier(object, property.accessorData, n);
    }
};

static inline v8::Handle<v8::Value> valueToHandle(QV8Engine *, int v)
{ return v8::Integer::New(v); }
static inline v8::Handle<v8::Value> valueToHandle(QV8Engine *, uint v)
{ return v8::Integer::NewFromUnsigned(v); }
static inline v8::Handle<v8::Value> valueToHandle(QV8Engine *, bool v)
{ return v8::Boolean::New(v); }
static inline v8::Handle<v8::Value> valueToHandle(QV8Engine *e, const QString &v)
{ return e->toString(v); }
static inline v8::Handle<v8::Value> valueToHandle(QV8Engine *, float v)
{ return v8::Number::New(v); }
static inline v8::Handle<v8::Value> valueToHandle(QV8Engine *, double v)
{ return v8::Number::New(v); }
static inline v8::Handle<v8::Value> valueToHandle(QV8Engine *e, QObject *v)
{ return e->newQObject(v); }

template<typename T, void (*ReadFunction)(QObject *, const QQmlPropertyData &,
                                          void *, QQmlNotifier **)>
static v8::Handle<v8::Value> GenericValueGetter(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    v8::Handle<v8::Object> This = info.This();
    QV8QObjectResource *resource = v8_resource_check<QV8QObjectResource>(This);

    QObject *object = resource->object;
    if (QQmlData::wasDeleted(object)) return v8::Undefined();

    QQmlPropertyData *property =
        (QQmlPropertyData *)v8::External::Cast(*info.Data())->Value();

    QQmlEngine *engine = resource->engine->engine();
    QQmlEnginePrivate *ep = engine?QQmlEnginePrivate::get(engine):0;

    T value = T();

    if (ep && ep->propertyCapture) {
        if (ReadFunction == ReadAccessor::Accessor && property->accessors->notifier) {
            QQmlNotifier *notifier = 0;
            ReadFunction(object, *property, &value, &notifier);
            if (notifier) ep->captureProperty(notifier);
        } else if (!property->isConstant()) {
            ep->captureProperty(object, property->coreIndex, property->notifyIndex);
            ReadFunction(object, *property, &value, 0);
        } else {
            ReadFunction(object, *property, &value, 0);
        }
    } else {
        ReadFunction(object, *property, &value, 0);
    }

    return valueToHandle(resource->engine, value);
}

#define FAST_GETTER_FUNCTION(property, cpptype) \
    (property->hasAccessors()?((v8::AccessorGetter)GenericValueGetter<cpptype, &ReadAccessor::Accessor>):(property->isDirect()?((v8::AccessorGetter)GenericValueGetter<cpptype, &ReadAccessor::Direct>):((v8::AccessorGetter)GenericValueGetter<cpptype, &ReadAccessor::Indirect>)))

static quint32 toStringHash = quint32(-1);
static quint32 destroyHash = quint32(-1);

void QV8QObjectWrapper::init(QV8Engine *engine)
{
    m_engine = engine;

    m_toStringSymbol = qPersistentNew<v8::String>(v8::String::NewSymbol("toString"));
    m_destroySymbol = qPersistentNew<v8::String>(v8::String::NewSymbol("destroy"));
    m_hiddenObject = qPersistentNew<v8::Object>(v8::Object::New());

    m_toStringString = QHashedV8String(m_toStringSymbol);
    m_destroyString = QHashedV8String(m_destroySymbol);

    toStringHash = m_toStringString.hash();
    destroyHash = m_destroyString.hash();

    {
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetFallbackPropertyHandler(Getter, Setter, Query, 0, Enumerator);
    ft->InstanceTemplate()->SetHasExternalResource(true);
    m_constructor = qPersistentNew<v8::Function>(ft->GetFunction());
    }
    {
    v8::ScriptOrigin origin(m_hiddenObject); // Hack to allow us to identify these functions
#define CREATE_FUNCTION_SOURCE \
    "(function(method) { "\
        "return (function(object, data, qmlglobal) { "\
            "return (function() { "\
                "return method(object, data, qmlglobal, arguments.length, arguments); "\
            "});"\
        "});"\
    "})"
    v8::Local<v8::Script> script = v8::Script::New(v8::String::New(CREATE_FUNCTION_SOURCE), &origin, 0,
                                                   v8::Handle<v8::String>(), v8::Script::NativeMode);
#undef CREATE_FUNCTION_SOURCE
    v8::Local<v8::Function> fn = v8::Local<v8::Function>::Cast(script->Run());
    v8::Handle<v8::Value> invokeFn = v8::FunctionTemplate::New(Invoke)->GetFunction();
    v8::Handle<v8::Value> args[] = { invokeFn };
    v8::Local<v8::Function> createFn = v8::Local<v8::Function>::Cast(fn->Call(engine->global(), 1, args));
    m_methodConstructor = qPersistentNew<v8::Function>(createFn);
    }

    v8::Local<v8::Function> connect = V8FUNCTION(Connect, engine);
    v8::Local<v8::Function> disconnect = V8FUNCTION(Disconnect, engine);

    {
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->PrototypeTemplate()->Set(v8::String::New("connect"), connect, v8::DontEnum);
    ft->PrototypeTemplate()->Set(v8::String::New("disconnect"), disconnect, v8::DontEnum);
    m_signalHandlerConstructor = qPersistentNew<v8::Function>(ft->GetFunction());
    }

    {
    v8::Local<v8::Object> prototype = engine->global()->Get(v8::String::New("Function"))->ToObject()->Get(v8::String::New("prototype"))->ToObject();
    prototype->Set(v8::String::New("connect"), connect, v8::DontEnum);
    prototype->Set(v8::String::New("disconnect"), disconnect, v8::DontEnum);
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
QObject *QV8QObjectWrapper::toQObject(QV8ObjectResource *r)
{
    Q_ASSERT(r->resourceType() == QV8ObjectResource::QObjectType);
    return static_cast<QV8QObjectResource *>(r)->object;
}

// Load value properties
template<void (*ReadFunction)(QObject *, const QQmlPropertyData &,
                              void *, QQmlNotifier **)>
static v8::Handle<v8::Value> LoadProperty(QV8Engine *engine, QObject *object,
                                          const QQmlPropertyData &property,
                                          QQmlNotifier **notifier)
{
    Q_ASSERT(!property.isFunction());

    if (property.isQObject()) {
        QObject *rv = 0;
        ReadFunction(object, property, &rv, notifier);
        return engine->newQObject(rv);
    } else if (property.isQList()) {
        return engine->listWrapper()->newList(object, property.coreIndex, property.propType);
    } else if (property.propType == QMetaType::QReal) {
        qreal v = 0;
        ReadFunction(object, property, &v, notifier);
        return valueToHandle(engine, v);
    } else if (property.propType == QMetaType::Int || property.isEnum()) {
        int v = 0;
        ReadFunction(object, property, &v, notifier);
        return valueToHandle(engine, v);
    } else if (property.propType == QMetaType::Bool) {
        bool v = false;
        ReadFunction(object, property, &v, notifier);
        return valueToHandle(engine, v);
    } else if (property.propType == QMetaType::QString) {
        QString v;
        ReadFunction(object, property, &v, notifier);
        return valueToHandle(engine, v);
    } else if (property.propType == QMetaType::UInt) {
        uint v = 0;
        ReadFunction(object, property, &v, notifier);
        return valueToHandle(engine, v);
    } else if (property.propType == QMetaType::Float) {
        float v = 0;
        ReadFunction(object, property, &v, notifier);
        return valueToHandle(engine, v);
    } else if (property.propType == QMetaType::Double) {
        double v = 0;
        ReadFunction(object, property, &v, notifier);
        return valueToHandle(engine, v);
    } else if (property.isV8Handle()) {
        QQmlV8Handle handle;
        ReadFunction(object, property, &handle, notifier);
        return handle.toHandle();
    } else if (property.propType == qMetaTypeId<QJSValue>()) {
        QJSValue v;
        ReadFunction(object, property, &v, notifier);
        return QJSValuePrivate::get(v)->asV8Value(engine);
    } else if (property.isQVariant()) {
        QVariant v;
        ReadFunction(object, property, &v, notifier);

        if (QQmlValueTypeFactory::isValueType(v.userType())) {
            if (QQmlValueType *valueType = QQmlValueTypeFactory::valueType(v.userType()))
                return engine->newValueType(object, property.coreIndex, valueType); // VariantReference value-type.
        }

        return engine->fromVariant(v);
    } else if (QQmlValueTypeFactory::isValueType(property.propType)) {
        Q_ASSERT(notifier == 0);

        if (QQmlValueType *valueType = QQmlValueTypeFactory::valueType(property.propType))
            return engine->newValueType(object, property.coreIndex, valueType);
    } else {
        Q_ASSERT(notifier == 0);

        // see if it's a sequence type
        bool succeeded = false;
        v8::Handle<v8::Value> retn = engine->newSequence(property.propType, object, property.coreIndex,
                                                         &succeeded);
        if (succeeded)
            return retn;
    }

    if (property.propType == QMetaType::UnknownType) {
        QMetaProperty p = object->metaObject()->property(property.coreIndex);
        qWarning("QMetaProperty::read: Unable to handle unregistered datatype '%s' for property "
                 "'%s::%s'", p.typeName(), object->metaObject()->className(), p.name());
        return v8::Undefined();
    } else {
        QVariant v(property.propType, (void *)0);
        ReadFunction(object, property, v.data(), notifier);
        return engine->fromVariant(v);
    }
}

v8::Handle<v8::Value> QV8QObjectWrapper::GetProperty(QV8Engine *engine, QObject *object, 
                                                     v8::Handle<v8::Value> *objectHandle, 
                                                     const QHashedV8String &property,
                                                     QQmlContextData *context,
                                                     QV8QObjectWrapper::RevisionMode revisionMode)
{
    // XXX More recent versions of V8 introduced "Callable" objects.  It is possible that these
    // will be a faster way of creating QObject method objects.
    struct MethodClosure {
       static v8::Handle<v8::Value> create(QV8Engine *engine, QObject *object, 
                                           v8::Handle<v8::Value> *objectHandle, 
                                           int index) { 
           v8::Handle<v8::Value> argv[] = { 
               objectHandle?*objectHandle:engine->newQObject(object),
               v8::Integer::New(index)
           };
           Q_ASSERT(argv[0]->IsObject());
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
           Q_ASSERT(argv[0]->IsObject());
           return engine->qobjectWrapper()->m_methodConstructor->Call(engine->global(), 3, argv);
       }
    };

    if (QQmlData::wasDeleted(object))
        return v8::Handle<v8::Value>();

    {
        // Comparing the hash first actually makes a measurable difference here, at least on x86
        quint32 hash = property.hash();
        if (hash == toStringHash && engine->qobjectWrapper()->m_toStringString == property) {
            return MethodClosure::create(engine, object, objectHandle, QOBJECT_TOSTRING_INDEX);
        } else if (hash == destroyHash && engine->qobjectWrapper()->m_destroyString == property) {
            return MethodClosure::create(engine, object, objectHandle, QOBJECT_DESTROY_INDEX);
        }
    }

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    {
        QQmlData *ddata = QQmlData::get(object, false);
        if (ddata && ddata->propertyCache)
            result = ddata->propertyCache->property(property, object, context);
        if (!result)
            result = QQmlPropertyCache::property(engine->engine(), object, property, context, local);
    }

    if (!result)
        return v8::Handle<v8::Value>();

    QQmlData::flushPendingBinding(object, result->coreIndex);

    if (revisionMode == QV8QObjectWrapper::CheckRevision && result->hasRevision()) {
        QQmlData *ddata = QQmlData::get(object);
        if (ddata && ddata->propertyCache && !ddata->propertyCache->isAllowedInRevision(result))
            return v8::Handle<v8::Value>();
    }

    if (result->isFunction() && !result->isVarProperty()) {
        if (result->isVMEFunction()) {
            QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(object);
            Q_ASSERT(vmemo);
            return vmemo->vmeMethod(result->coreIndex);
        } else if (result->isV8Function()) {
            return MethodClosure::createWithGlobal(engine, object, objectHandle, result->coreIndex);
        } else if (result->isSignalHandler()) {
            v8::Local<v8::Object> handler = engine->qobjectWrapper()->m_signalHandlerConstructor->NewInstance();
            QV8SignalHandlerResource *r = new QV8SignalHandlerResource(engine, object, result->coreIndex);
            handler->SetExternalResource(r);
            return handler;
        } else {
            return MethodClosure::create(engine, object, objectHandle, result->coreIndex);
        }
    }

    QQmlEnginePrivate *ep =
        engine->engine()?QQmlEnginePrivate::get(engine->engine()):0;

    if (result->hasAccessors()) {
        QQmlNotifier *n = 0;
        QQmlNotifier **nptr = 0;

        if (ep && ep->propertyCapture && result->accessors->notifier)
            nptr = &n;

        v8::Handle<v8::Value> rv = LoadProperty<ReadAccessor::Accessor>(engine, object, *result, nptr);

        if (result->accessors->notifier) {
            if (n) ep->captureProperty(n);
        } else {
            ep->captureProperty(object, result->coreIndex, result->notifyIndex);
        }

        return rv;
    }

    if (ep && !result->isConstant())
        ep->captureProperty(object, result->coreIndex, result->notifyIndex);

    if (result->isVarProperty()) {
        QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(object);
        Q_ASSERT(vmemo);
        return vmemo->vmeProperty(result->coreIndex);
    } else if (result->isDirect())  {
        return LoadProperty<ReadAccessor::Direct>(engine, object, *result, 0);
    } else {
        return LoadProperty<ReadAccessor::Indirect>(engine, object, *result, 0);
    }
}

// Setter for writable properties.  Shared between the interceptor and fast property accessor
static inline void StoreProperty(QV8Engine *engine, QObject *object, QQmlPropertyData *property,
                                 v8::Handle<v8::Value> value)
{
    QQmlBinding *newBinding = 0;
    if (value->IsFunction()) {
        if (value->ToObject()->GetHiddenValue(engine->bindingFlagKey()).IsEmpty()) {
            if (!property->isVarProperty() && property->propType != qMetaTypeId<QJSValue>()) {
                // assigning a JS function to a non var or QJSValue property or is not allowed.
                QString error = QLatin1String("Cannot assign JavaScript function to ");
                if (!QMetaType::typeName(property->propType))
                    error += QLatin1String("[unknown property type]");
                else
                    error += QLatin1String(QMetaType::typeName(property->propType));
                v8::ThrowException(v8::Exception::Error(engine->toString(error)));
                return;
            }
        } else {
            // binding assignment.
            QQmlContextData *context = engine->callingContext();
            v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(value);

            v8::Local<v8::StackTrace> trace =
                v8::StackTrace::CurrentStackTrace(1, (v8::StackTrace::StackTraceOptions)(v8::StackTrace::kLineNumber |
                                                                                         v8::StackTrace::kScriptName));
            v8::Local<v8::StackFrame> frame = trace->GetFrame(0);
            int lineNumber = frame->GetLineNumber();
            int columnNumber = frame->GetColumn();
            QString url = engine->toString(frame->GetScriptName());

            newBinding = new QQmlBinding(&function, object, context, url, qmlSourceCoordinate(lineNumber), qmlSourceCoordinate(columnNumber));
            newBinding->setTarget(object, *property, context);
            newBinding->setEvaluateFlags(newBinding->evaluateFlags() |
                                         QQmlBinding::RequiresThisObject);
        }
    }

    QQmlAbstractBinding *oldBinding = 
        QQmlPropertyPrivate::setBinding(object, property->coreIndex, -1, newBinding);
    if (oldBinding)
        oldBinding->destroy();

    if (!newBinding && property->isVarProperty()) {
        // allow assignment of "special" values (null, undefined, function) to var properties
        QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(object);
        Q_ASSERT(vmemo);
        vmemo->setVMEProperty(property->coreIndex, value);
        return;
    }

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
    } else if (value->IsUndefined() && property->propType == QMetaType::QJsonValue) {
        PROPERTY_STORE(QJsonValue, QJsonValue(QJsonValue::Undefined));
    } else if (!newBinding && property->propType == qMetaTypeId<QJSValue>()) {
        PROPERTY_STORE(QJSValue, engine->scriptValueFromInternal(value));
    } else if (value->IsUndefined()) {
        QString error = QLatin1String("Cannot assign [undefined] to ");
        if (!QMetaType::typeName(property->propType))
            error += QLatin1String("[unknown property type]");
        else
            error += QLatin1String(QMetaType::typeName(property->propType));
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
    } else if (property->isVarProperty()) {
        QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(object);
        Q_ASSERT(vmemo);
        vmemo->setVMEProperty(property->coreIndex, value);
    } else {
        QVariant v;
        if (property->isQList()) 
            v = engine->toVariant(value, qMetaTypeId<QList<QObject *> >());
        else
            v = engine->toVariant(value, property->propType);

        QQmlContextData *context = engine->callingContext();
        if (!QQmlPropertyPrivate::write(object, *property, v, context)) {
            const char *valueType = 0;
            if (v.userType() == QVariant::Invalid) valueType = "null";
            else valueType = QMetaType::typeName(v.userType());

            const char *targetTypeName = QMetaType::typeName(property->propType);
            if (!targetTypeName)
                targetTypeName = "an unregistered type";

            QString error = QLatin1String("Cannot assign ") +
                            QLatin1String(valueType) +
                            QLatin1String(" to ") +
                            QLatin1String(targetTypeName);
            v8::ThrowException(v8::Exception::Error(engine->toString(error)));
        }
    }
}

bool QV8QObjectWrapper::SetProperty(QV8Engine *engine, QObject *object, const QHashedV8String &property, QQmlContextData *context,
                                    v8::Handle<v8::Value> value, QV8QObjectWrapper::RevisionMode revisionMode)
{
    if (engine->qobjectWrapper()->m_toStringString == property ||
        engine->qobjectWrapper()->m_destroyString == property)
        return true;

    if (QQmlData::wasDeleted(object))
        return false;

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    result = QQmlPropertyCache::property(engine->engine(), object, property, context, local);

    if (!result)
        return false;

    if (revisionMode == QV8QObjectWrapper::CheckRevision && result->hasRevision()) {
        QQmlData *ddata = QQmlData::get(object);
        if (ddata && ddata->propertyCache && !ddata->propertyCache->isAllowedInRevision(result))
            return false;
    }

    if (!result->isWritable() && !result->isQList()) {
        QString error = QLatin1String("Cannot assign to read-only property \"") +
                        engine->toString(property.string()) + QLatin1Char('\"');
        v8::ThrowException(v8::Exception::Error(engine->toString(error)));
        return true;
    }

    StoreProperty(engine, object, result, value);

    return true;
}

v8::Handle<v8::Value> QV8QObjectWrapper::Getter(v8::Local<v8::String> property, 
                                                const v8::AccessorInfo &info)
{
    QV8QObjectResource *resource = v8_resource_check<QV8QObjectResource>(info.This());

    if (QQmlData::wasDeleted(resource->object))
        return v8::Handle<v8::Value>();

    QObject *object = resource->object;

    QHashedV8String propertystring(property);

    QV8Engine *v8engine = resource->engine;
    QQmlContextData *context = v8engine->callingContext();

    v8::Handle<v8::Value> This = info.This();
    v8::Handle<v8::Value> result = GetProperty(v8engine, object, &This, propertystring, 
                                               context, QV8QObjectWrapper::IgnoreRevision);
    if (!result.IsEmpty())
        return result;

    if (QV8Engine::startsWithUpper(property)) {
        // Check for attached properties
        if (context && context->imports) {
            QQmlTypeNameCache::Result r = context->imports->query(propertystring);

            if (r.isValid()) {
                if (r.scriptIndex != -1) {
                    return v8::Undefined();
                } else if (r.type) {
                    return v8engine->typeWrapper()->newObject(object, r.type, QV8TypeWrapper::ExcludeEnums);
                } else if (r.importNamespace) {
                    return v8engine->typeWrapper()->newObject(object, context->imports, r.importNamespace, 
                                                              QV8TypeWrapper::ExcludeEnums);
                }
                Q_ASSERT(!"Unreachable");
            }
        }
    } 

    return v8::Handle<v8::Value>();
}

v8::Handle<v8::Value> QV8QObjectWrapper::Setter(v8::Local<v8::String> property, 
                                                v8::Local<v8::Value> value,
                                                const v8::AccessorInfo &info)
{
    QV8QObjectResource *resource = v8_resource_check<QV8QObjectResource>(info.This());

    if (QQmlData::wasDeleted(resource->object))
        return value;

    QObject *object = resource->object;

    QHashedV8String propertystring(property);

    QV8Engine *v8engine = resource->engine;
    QQmlContextData *context = v8engine->callingContext();
    bool result = SetProperty(v8engine, object, propertystring, context, value, QV8QObjectWrapper::IgnoreRevision);

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
    QV8QObjectResource *resource = v8_resource_check<QV8QObjectResource>(info.This());

    if (resource->object.isNull()) 
        return v8::Handle<v8::Integer>();

    QV8Engine *engine = resource->engine;
    QObject *object = resource->object;
    QQmlContextData *context = engine->callingContext();

    QHashedV8String propertystring(property);

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    result = QQmlPropertyCache::property(engine->engine(), object, propertystring, context, local);

    if (!result)
        return v8::Handle<v8::Integer>();
    else if (!result->isWritable() && !result->isQList())
        return v8::Integer::New(v8::ReadOnly | v8::DontDelete);
    else
        return v8::Integer::New(v8::DontDelete);
}

v8::Handle<v8::Array> QV8QObjectWrapper::Enumerator(const v8::AccessorInfo &info)
{
    QV8QObjectResource *resource = v8_resource_check<QV8QObjectResource>(info.This());

    if (resource->object.isNull()) 
        return v8::Array::New();

    QObject *object = resource->object;

    QStringList result;

    QQmlEnginePrivate *ep = resource->engine->engine()
            ? QQmlEnginePrivate::get(resource->engine->engine())
            : 0;

    QQmlPropertyCache *cache = 0;
    QQmlData *ddata = QQmlData::get(object);
    if (ddata)
        cache = ddata->propertyCache;

    if (!cache) {
        cache = ep ? ep->cache(object) : 0;
        if (cache) {
            if (ddata) { cache->addref(); ddata->propertyCache = cache; }
        } else {
            // Not cachable - fall back to QMetaObject (eg. dynamic meta object)
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

static void FastValueSetter(v8::Local<v8::String>, v8::Local<v8::Value> value,
                            const v8::AccessorInfo& info)
{
    QV8QObjectResource *resource = v8_resource_check<QV8QObjectResource>(info.This());

    if (QQmlData::wasDeleted(resource->object))
        return; 

    QObject *object = resource->object;

    QQmlPropertyData *property =
        (QQmlPropertyData *)v8::External::Cast(*info.Data())->Value();

    int index = property->coreIndex;

    QQmlData *ddata = QQmlData::get(object, false);
    Q_ASSERT(ddata);
    Q_ASSERT(ddata->propertyCache);

    QQmlPropertyData *pdata = ddata->propertyCache->property(index);
    Q_ASSERT(pdata);

    Q_ASSERT(pdata->isWritable() || pdata->isQList());

    StoreProperty(resource->engine, object, pdata, value);
}

static void FastValueSetterReadOnly(v8::Local<v8::String> property, v8::Local<v8::Value>,
                                    const v8::AccessorInfo& info)
{
    QV8QObjectResource *resource = v8_resource_check<QV8QObjectResource>(info.This());

    if (QQmlData::wasDeleted(resource->object))
        return; 

    QV8Engine *v8engine = resource->engine;

    QString error = QLatin1String("Cannot assign to read-only property \"") +
                    v8engine->toString(property) + QLatin1Char('\"');
    v8::ThrowException(v8::Exception::Error(v8engine->toString(error)));
}

void QV8QObjectWrapper::WeakQObjectReferenceCallback(v8::Persistent<v8::Value> handle, void *wrapper)
{
    Q_ASSERT(handle->IsObject());
    v8::Handle<v8::Object> v8object = handle->ToObject();
    QV8QObjectResource *resource = v8_resource_check<QV8QObjectResource>(v8object);
    Q_ASSERT(resource);

    static_cast<QV8QObjectWrapper*>(wrapper)->unregisterWeakQObjectReference(resource);
    if (static_cast<QV8QObjectWrapper*>(wrapper)->deleteWeakQObject(resource, false)) {
        // dispose
        v8object->SetExternalResource(0);
        delete resource;
        qPersistentDispose(handle);
    } else {
        handle.MakeWeak(0, WeakQObjectReferenceCallback); // revive.
    }
}

static void WeakQObjectInstanceCallback(v8::Persistent<v8::Value> handle, void *data)
{
    QV8QObjectInstance *instance = (QV8QObjectInstance *)data;
    instance->v8object.Clear();
    qPersistentDispose(handle);
}

v8::Local<v8::Object> QQmlPropertyCache::newQObject(QObject *object, QV8Engine *engine)
{
    Q_ASSERT(object);
    Q_ASSERT(this->engine);

    Q_ASSERT(QQmlData::get(object, false));
    Q_ASSERT(QQmlData::get(object, false)->propertyCache == this);

    // Setup constructor
    if (constructor.IsEmpty()) {
        v8::Local<v8::FunctionTemplate> ft;

        const QHashedString toString(QStringLiteral("toString"));
        const QHashedString destroy(QStringLiteral("destroy"));

        // As we use hash linking, or with property overrides, it is possible that iterating
        // over the values can yield duplicates.  To combat this, we must unique'ify our properties.
        const bool checkForDuplicates = stringCache.isLinked() || _hasPropertyOverrides;

        StringCache uniqueHash;
        if (checkForDuplicates)
            uniqueHash.reserve(stringCache.count());

        // XXX TODO: Enables fast property accessors.  These more than double the property access 
        // performance, but the  cost of setting up this structure hasn't been measured so 
        // its not guaranteed that this is a win overall.  We need to try and measure the cost.
        for (StringCache::ConstIterator iter = stringCache.begin(); iter != stringCache.end(); ++iter) {
            if (iter.equals(toString) || iter.equals(destroy))
                continue;

            if (checkForDuplicates) {
                if (uniqueHash.contains(iter))
                    continue;
                uniqueHash.insert(iter);
            }

            QQmlPropertyData *property = (*iter).second;
            if (property->notFullyResolved()) resolve(property);

            if (property->isFunction())
                continue;

            v8::AccessorGetter fastgetter = 0;

            if (property->isQObject()) 
                fastgetter = FAST_GETTER_FUNCTION(property, QObject*);
            else if (property->propType == QMetaType::Int || property->isEnum()) 
                fastgetter = FAST_GETTER_FUNCTION(property, int);
            else if (property->propType == QMetaType::Bool)
                fastgetter = FAST_GETTER_FUNCTION(property, bool);
            else if (property->propType == QMetaType::QString)
                fastgetter = FAST_GETTER_FUNCTION(property, QString);
            else if (property->propType == QMetaType::UInt)
                fastgetter = FAST_GETTER_FUNCTION(property, uint);
            else if (property->propType == QMetaType::Float) 
                fastgetter = FAST_GETTER_FUNCTION(property, float);
            else if (property->propType == QMetaType::Double) 
                fastgetter = FAST_GETTER_FUNCTION(property, double);

            if (fastgetter) {
                if (ft.IsEmpty()) {
                    ft = v8::FunctionTemplate::New();
                    ft->InstanceTemplate()->SetFallbackPropertyHandler(QV8QObjectWrapper::Getter, 
                                                                       QV8QObjectWrapper::Setter,
                                                                       QV8QObjectWrapper::Query, 
                                                                       0,
                                                                       QV8QObjectWrapper::Enumerator);
                    ft->InstanceTemplate()->SetHasExternalResource(true);
                }

                v8::AccessorSetter fastsetter = FastValueSetter;
                if (!property->isWritable())
                    fastsetter = FastValueSetterReadOnly;

                // We wrap the raw QQmlPropertyData pointer here.  This is safe as the
                // pointer will remain valid at least as long as the lifetime of any QObject's of
                // this type and the property accessor checks if the object is 0 (deleted) before
                // dereferencing the pointer.
                ft->InstanceTemplate()->SetAccessor(engine->toString(iter.key()), fastgetter, fastsetter,
                                                    v8::External::New(property));
            }
        }

        if (ft.IsEmpty()) {
            constructor = qPersistentNew<v8::Function>(engine->qobjectWrapper()->m_constructor);
        } else {
            ft->InstanceTemplate()->SetFallbackPropertyHandler(QV8QObjectWrapper::Getter, 
                                                               QV8QObjectWrapper::Setter,
                                                               QV8QObjectWrapper::Query, 
                                                               0,
                                                               QV8QObjectWrapper::Enumerator);
            ft->InstanceTemplate()->SetHasExternalResource(true);
            constructor = qPersistentNew<v8::Function>(ft->GetFunction());
        }

        QQmlCleanup::addToEngine(this->engine);
    }

    v8::Local<v8::Object> result = constructor->NewInstance();
    QV8QObjectResource *r = new QV8QObjectResource(engine, object);
    result->SetExternalResource(r);
    return result;
}

v8::Local<v8::Object> QV8QObjectWrapper::newQObject(QObject *object, QQmlData *ddata, QV8Engine *engine)
{
    v8::Local<v8::Object> rv;

    if (!ddata->propertyCache && engine->engine()) {
        ddata->propertyCache = QQmlEnginePrivate::get(engine->engine())->cache(object);
        if (ddata->propertyCache) ddata->propertyCache->addref();
    }

    if (ddata->propertyCache && ddata->propertyCache->qmlEngine() == engine->engine()) {
        rv = ddata->propertyCache->newQObject(object, engine);
    } else {
        // XXX NewInstance() should be optimized
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
      persistent handle in QQmlData::v8object.  We mark the QV8QObjectWrapper that 
      "owns" this handle by setting the QQmlData::v8objectid to the id of this 
      QV8QObjectWrapper.
   2. If another QV8QObjectWrapper has create the handle in QQmlData::v8object we create 
      an entry in the m_taintedObject hash where we store the handle and mark the object as 
      "tainted" in the QQmlData::hasTaintedV8Object flag.
We have to mark the object as tainted to ensure that we search our m_taintedObject hash even
in the case that the original QV8QObjectWrapper owner of QQmlData::v8object has 
released the handle.
*/
v8::Handle<v8::Value> QV8QObjectWrapper::newQObject(QObject *object)
{
    if (QQmlData::wasDeleted(object))
        return v8::Null();

    QQmlData *ddata = QQmlData::get(object, true);
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
        ddata->v8object = qPersistentNew<v8::Object>(rv);
        ddata->v8object.MakeWeak(this, WeakQObjectReferenceCallback);
        ddata->v8objectid = m_id;
        QV8QObjectResource *resource = v8_resource_check<QV8QObjectResource>(rv);
        registerWeakQObjectReference(resource);
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
            ddata->v8object = qPersistentNew<v8::Object>(rv);
            ddata->v8object.MakeWeak(this, WeakQObjectReferenceCallback);
            ddata->v8objectid = m_id;
            QV8QObjectResource *resource = v8_resource_check<QV8QObjectResource>(rv);
            registerWeakQObjectReference(resource);

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
            (*iter)->v8object = qPersistentNew<v8::Object>(rv);
            (*iter)->v8object.MakeWeak((*iter), WeakQObjectInstanceCallback);
        }

        return v8::Local<v8::Object>::New((*iter)->v8object);
    }
}

// returns true if the object's qqmldata v8object handle should
// be disposed by the caller, false if it should not be (due to
// creation status, etc).
bool QV8QObjectWrapper::deleteWeakQObject(QV8QObjectResource *resource, bool calledFromEngineDtor)
{
    QObject *object = resource->object;
    if (object) {
        QQmlData *ddata = QQmlData::get(object, false);
        if (ddata) {
            if (!calledFromEngineDtor && ddata->rootObjectInCreation) {
                // if weak ref callback is triggered (by gc) for a root object
                // prior to completion of creation, we should NOT delete it.
                return false;
            }

            ddata->v8object.Clear();
            if (!object->parent() && !ddata->indestructible) {
                // This object is notionally destroyed now
                if (ddata->ownContext && ddata->context)
                    ddata->context->emitDestruction();
                ddata->isQueuedForDeletion = true;
                if (calledFromEngineDtor)
                    delete object;
                else
                    object->deleteLater();
            }
        }
    }

    return true;
}

QPair<QObject *, int> QV8QObjectWrapper::ExtractQtSignal(QV8Engine *engine, v8::Handle<v8::Object> object)
{
    if (object->IsFunction())
        return ExtractQtMethod(engine, v8::Handle<v8::Function>::Cast(object));

    if (QV8SignalHandlerResource *resource = v8_resource_cast<QV8SignalHandlerResource>(object))
        return qMakePair(resource->object.data(), resource->index);

    return qMakePair((QObject *)0, -1);
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

class QV8QObjectConnectionList : public QObject, public QQmlGuard<QObject>
{
public:
    QV8QObjectConnectionList(QObject *object, QV8Engine *engine);
    ~QV8QObjectConnectionList();

    struct Connection {
        Connection() 
        : needsDestroy(false) {}
        Connection(const Connection &other) 
        : thisObject(other.thisObject), function(other.function), needsDestroy(false) {}
        Connection &operator=(const Connection &other) {
            thisObject = other.thisObject;
            function = other.function;
            needsDestroy = other.needsDestroy;
            return *this;
        }

        v8::Persistent<v8::Object> thisObject;
        v8::Persistent<v8::Function> function;

        void dispose() {
            qPersistentDispose(thisObject);
            qPersistentDispose(function);
        }

        bool needsDestroy;
    };

    struct ConnectionList : public QList<Connection> {
        ConnectionList() : connectionsInUse(0), connectionsNeedClean(false) {}
        int connectionsInUse;
        bool connectionsNeedClean;
    };

    QV8Engine *engine;

    typedef QHash<int, ConnectionList> SlotHash;
    SlotHash slotHash;
    bool needsDestroy;
    int inUse;

    virtual void objectDestroyed(QObject *);
    virtual int qt_metacall(QMetaObject::Call, int, void **);
};

QV8QObjectConnectionList::QV8QObjectConnectionList(QObject *object, QV8Engine *engine)
: QQmlGuard<QObject>(object), engine(engine), needsDestroy(false), inUse(0)
{
}

QV8QObjectConnectionList::~QV8QObjectConnectionList()
{
    for (SlotHash::Iterator iter = slotHash.begin(); iter != slotHash.end(); ++iter) {
        QList<Connection> &connections = *iter;
        for (int ii = 0; ii < connections.count(); ++ii) {
            qPersistentDispose(connections[ii].thisObject);
            qPersistentDispose(connections[ii].function);
        }
    }
    slotHash.clear();
}

void QV8QObjectConnectionList::objectDestroyed(QObject *object)
{
    engine->qobjectWrapper()->m_connections.remove(object);

    if (inUse)
        needsDestroy = true;
    else
        delete this;
}

int QV8QObjectConnectionList::qt_metacall(QMetaObject::Call method, int index, void **metaArgs)
{
    if (method == QMetaObject::InvokeMetaMethod) {
        SlotHash::Iterator iter = slotHash.find(index);
        if (iter == slotHash.end())
            return -1;
        ConnectionList &connectionList = *iter;
        if (connectionList.isEmpty())
            return -1;

        inUse++;

        connectionList.connectionsInUse++;

        QList<Connection> connections = connectionList;

        QVarLengthArray<int, 9> dummy;
        int *argsTypes = QQmlPropertyCache::methodParameterTypes(data(), index, dummy, 0);

        v8::HandleScope handle_scope;
        v8::Context::Scope scope(engine->context());

        int argCount = argsTypes?argsTypes[0]:0;
        QVarLengthArray<v8::Handle<v8::Value>, 9> args(argCount);

        for (int ii = 0; ii < argCount; ++ii) {
            int type = argsTypes[ii + 1];
            if (type == qMetaTypeId<QVariant>()) {
                args[ii] = engine->fromVariant(*((QVariant *)metaArgs[ii + 1]));
            } else {
                args[ii] = engine->fromVariant(QVariant(type, metaArgs[ii + 1]));
            }
        }

        for (int ii = 0; ii < connections.count(); ++ii) {
            Connection &connection = connections[ii];
            if (connection.needsDestroy)
                continue;

            v8::TryCatch try_catch;
            if (connection.thisObject.IsEmpty()) {
                connection.function->Call(engine->global(), argCount, args.data());
            } else {
                connection.function->Call(connection.thisObject, argCount, args.data());
            }

            if (try_catch.HasCaught()) {
                QQmlError error;
                error.setDescription(QString(QLatin1String("Unknown exception occurred during evaluation of connected function: %1")).arg(engine->toString(connection.function->GetName())));
                v8::Local<v8::Message> message = try_catch.Message();
                if (!message.IsEmpty())
                    QQmlExpressionPrivate::exceptionToError(message, error);
                QQmlEnginePrivate::get(engine->engine())->warning(error);
            }
        }

        connectionList.connectionsInUse--;
        if (connectionList.connectionsInUse == 0 && connectionList.connectionsNeedClean) {
            for (QList<Connection>::Iterator iter = connectionList.begin(); 
                 iter != connectionList.end(); ) {
                if (iter->needsDestroy) {
                    iter->dispose();
                    iter = connectionList.erase(iter);
                } else {
                    ++iter;
                }
            }
        }

        inUse--;
        if (inUse == 0 && needsDestroy)
            delete this;
    } 

    return -1;
}

v8::Handle<v8::Value> QV8QObjectWrapper::Connect(const v8::Arguments &args)
{
    if (args.Length() == 0)
        V8THROW_ERROR("Function.prototype.connect: no arguments given");

    QV8Engine *engine = V8ENGINE();

    QPair<QObject *, int> signalInfo = ExtractQtSignal(engine, args.This());
    QObject *signalObject = signalInfo.first;
    int signalIndex = signalInfo.second;

    if (signalIndex < 0)
        V8THROW_ERROR("Function.prototype.connect: this object is not a signal");

    if (!signalObject)
        V8THROW_ERROR("Function.prototype.connect: cannot connect to deleted QObject");

    if (signalObject->metaObject()->method(signalIndex).methodType() != QMetaMethod::Signal)
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
        slotIter = connectionList->slotHash.insert(signalIndex, QV8QObjectConnectionList::ConnectionList());
        QMetaObject::connect(signalObject, signalIndex, connectionList, signalIndex);
    }

    QV8QObjectConnectionList::Connection connection;
    if (!functionThisValue.IsEmpty()) 
        connection.thisObject = qPersistentNew<v8::Object>(functionThisValue->ToObject());
    connection.function = qPersistentNew<v8::Function>(v8::Handle<v8::Function>::Cast(functionValue));

    slotIter->append(connection);

    return v8::Undefined();
}

v8::Handle<v8::Value> QV8QObjectWrapper::Disconnect(const v8::Arguments &args)
{
    if (args.Length() == 0)
        V8THROW_ERROR("Function.prototype.disconnect: no arguments given");

    QV8Engine *engine = V8ENGINE();

    QPair<QObject *, int> signalInfo = ExtractQtSignal(engine, args.This());
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

    QV8QObjectConnectionList::ConnectionList &connections = *slotIter;

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
                    if (connections.connectionsInUse) {
                        connection.needsDestroy = true;
                        connections.connectionsNeedClean = true;
                    } else {
                        connection.dispose();
                        connections.removeAt(ii);
                    }
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
                if (connections.connectionsInUse) {
                    connection.needsDestroy = true;
                    connections.connectionsNeedClean = true;
                } else {
                    connection.dispose();
                    connections.removeAt(ii);
                }
                return v8::Undefined();
            }
        }
    }

    return v8::Undefined();
}

/*!
    \fn v8::Handle<v8::Value> QV8QObjectWrapper::getProperty(QObject *object, const QHashedV8String &property, QV8QObjectWrapper::RevisionMode revisionMode)

    Get the \a property of \a object.  Returns an empty handle if the property doesn't exist.

    Only searches for real properties of \a object (including methods), not attached properties etc.
*/

/*
    \fn bool QV8QObjectWrapper::setProperty(QObject *object, const QHashedV8String &property, v8::Handle<v8::Value> value, RevisionMode revisionMode)

    Set the \a property of \a object to \a value.

    Returns true if the property was "set" - even if this results in an exception being thrown -
    and false if the object has no such property.

    Only searches for real properties of \a object (including methods), not attached properties etc.
*/

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

        // Special handling is required for value types.
        // We need to save the current value in a temporary,
        // and reapply it after converting all arguments.
        // This avoids the "overwriting copy-value-type-value"
        // problem during Q_INVOKABLE function invocation.
        QQmlValueType *valueTypeObject = qobject_cast<QQmlValueType*>(object);
        QVariant valueTypeValue;
        if (valueTypeObject)
            valueTypeValue = valueTypeObject->value();

        // Convert all arguments.
        QVarLengthArray<CallArgument, 9> args(argCount + 1);
        args[0].initAsType(returnType);
        for (int ii = 0; ii < argCount; ++ii)
            args[ii + 1].fromValue(argTypes[ii], engine, callArgs[ii]);
        QVarLengthArray<void *, 9> argData(args.count());
        for (int ii = 0; ii < args.count(); ++ii)
            argData[ii] = args[ii].dataPtr();

        // Reinstate saved value type object value if required.
        if (valueTypeObject)
            valueTypeObject->setValue(valueTypeValue);

        QMetaObject::metacall(object, QMetaObject::InvokeMetaMethod, index, argData.data());

        return args[0].toValue(engine);

    } else if (returnType != QMetaType::Void) {
        
        CallArgument arg;
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

/*!
    Returns the match score for converting \a actual to be of type \a conversionType.  A 
    zero score means "perfect match" whereas a higher score is worse.

    The conversion table is copied out of the \l QScript::callQtMethod()
    function.
*/
static int MatchScore(v8::Handle<v8::Value> actual, int conversionType)
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
        case QMetaType::QJsonValue:
            return 5;
        default:
            return 10;
        }
    } else if (actual->IsString()) {
        switch (conversionType) {
        case QMetaType::QString:
            return 0;
        case QMetaType::QJsonValue:
            return 5;
        default:
            return 10;
        }
    } else if (actual->IsBoolean()) {
        switch (conversionType) {
        case QMetaType::Bool:
            return 0;
        case QMetaType::QJsonValue:
            return 5;
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
        case QMetaType::QJsonArray:
            return 3;
        case QMetaType::QStringList:
        case QMetaType::QVariantList:
            return 5;
        case QMetaType::QVector4D:
        case QMetaType::QMatrix4x4:
            return 6;
        case QMetaType::QVector3D:
            return 7;
        default:
            return 10;
        }
    } else if (actual->IsNull()) {
        switch (conversionType) {
        case QMetaType::VoidStar:
        case QMetaType::QObjectStar:
        case QMetaType::QJsonValue:
            return 0;
        default: {
            const char *typeName = QMetaType::typeName(conversionType);
            if (typeName && typeName[strlen(typeName) - 1] == '*')
                return 0;
            else
                return 10;
        }
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
        } else if (r && r->resourceType() == QV8ObjectResource::ValueTypeType) {
            if (r->engine->toVariant(actual, -1).userType() == conversionType)
                return 0;
            return 10;
        } else if (conversionType == QMetaType::QJsonObject) {
            return 5;
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

/*!
Returns the next related method, if one, or 0.
*/
static const QQmlPropertyData * RelatedMethod(QObject *object,
                                                      const QQmlPropertyData *current,
                                                      QQmlPropertyData &dummy)
{
    QQmlPropertyCache *cache = QQmlData::get(object)->propertyCache;
    if (!current->isOverload())
        return 0;

    Q_ASSERT(!current->overrideIndexIsProperty);

    if (cache) {
        return cache->method(current->overrideIndex);
    } else {
        const QMetaObject *mo = object->metaObject();
        int methodOffset = mo->methodCount() - QMetaObject_methods(mo);

        while (methodOffset > current->overrideIndex) {
            mo = mo->superClass();
            methodOffset -= QMetaObject_methods(mo);
        }

        QMetaMethod method = mo->method(current->overrideIndex);
        dummy.load(method);
        
        // Look for overloaded methods
        QByteArray methodName = method.name();
        for (int ii = current->overrideIndex - 1; ii >= methodOffset; --ii) {
            if (methodName == mo->method(ii).name()) {
                dummy.setFlags(dummy.getFlags() | QQmlPropertyData::IsOverload);
                dummy.overrideIndexIsProperty = 0;
                dummy.overrideIndex = ii;
                return &dummy;
            }
        }

        return &dummy;
    }
}

static v8::Handle<v8::Value> CallPrecise(QObject *object, const QQmlPropertyData &data,
                                         QV8Engine *engine, CallArgs &callArgs)
{
    QByteArray unknownTypeError;

    int returnType = QQmlPropertyCache::methodReturnType(object, data, &unknownTypeError);

    if (returnType == QMetaType::UnknownType) {
        QString typeName = QString::fromLatin1(unknownTypeError);
        QString error = QString::fromLatin1("Unknown method return type: %1").arg(typeName);
        v8::ThrowException(v8::Exception::Error(engine->toString(error)));
        return v8::Handle<v8::Value>();
    }

    if (data.hasArguments()) {

        int *args = 0;
        QVarLengthArray<int, 9> dummy;

        args = QQmlPropertyCache::methodParameterTypes(object, data.coreIndex, dummy, 
                                                               &unknownTypeError);

        if (!args) {
            QString typeName = QString::fromLatin1(unknownTypeError);
            QString error = QString::fromLatin1("Unknown method parameter type: %1").arg(typeName);
            v8::ThrowException(v8::Exception::Error(engine->toString(error)));
            return v8::Handle<v8::Value>();
        }

        if (args[0] > callArgs.Length()) {
            QString error = QLatin1String("Insufficient arguments");
            v8::ThrowException(v8::Exception::Error(engine->toString(error)));
            return v8::Handle<v8::Value>();
        }

        return CallMethod(object, data.coreIndex, returnType, args[0], args + 1, engine, callArgs);

    } else {

        return CallMethod(object, data.coreIndex, returnType, 0, 0, engine, callArgs);

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
static v8::Handle<v8::Value> CallOverloaded(QObject *object, const QQmlPropertyData &data,
                                            QV8Engine *engine, CallArgs &callArgs)
{
    int argumentCount = callArgs.Length();

    const QQmlPropertyData *best = 0;
    int bestParameterScore = INT_MAX;
    int bestMatchScore = INT_MAX;

    // Special handling is required for value types.
    // We need to save the current value in a temporary,
    // and reapply it after converting all arguments.
    // This avoids the "overwriting copy-value-type-value"
    // problem during Q_INVOKABLE function invocation.
    QQmlValueType *valueTypeObject = qobject_cast<QQmlValueType*>(object);
    QVariant valueTypeValue;
    if (valueTypeObject)
        valueTypeValue = valueTypeObject->value();

    QQmlPropertyData dummy;
    const QQmlPropertyData *attempt = &data;

    do {
        QVarLengthArray<int, 9> dummy;
        int methodArgumentCount = 0;
        int *methodArgTypes = 0;
        if (attempt->hasArguments()) {
            typedef QQmlPropertyCache PC;
            int *args = PC::methodParameterTypes(object, attempt->coreIndex, dummy, 0);
            if (!args) // Must be an unknown argument
                continue;

            methodArgumentCount = args[0];
            methodArgTypes = args + 1;
        }

        if (methodArgumentCount > argumentCount)
            continue; // We don't have sufficient arguments to call this method

        int methodParameterScore = argumentCount - methodArgumentCount;
        if (methodParameterScore > bestParameterScore)
            continue; // We already have a better option

        int methodMatchScore = 0;
        for (int ii = 0; ii < methodArgumentCount; ++ii) 
            methodMatchScore += MatchScore(callArgs[ii], methodArgTypes[ii]);

        if (bestParameterScore > methodParameterScore || bestMatchScore > methodMatchScore) {
            best = attempt;
            bestParameterScore = methodParameterScore;
            bestMatchScore = methodMatchScore;
        }

        if (bestParameterScore == 0 && bestMatchScore == 0)
            break; // We can't get better than that

    } while((attempt = RelatedMethod(object, attempt, dummy)) != 0);

    if (best) {
        if (valueTypeObject)
            valueTypeObject->setValue(valueTypeValue);
        return CallPrecise(object, *best, engine, callArgs);
    } else {
        QString error = QLatin1String("Unable to determine callable overload.  Candidates are:");
        const QQmlPropertyData *candidate = &data;
        while (candidate) {
            error += QLatin1String("\n    ") + 
                     QString::fromUtf8(object->metaObject()->method(candidate->coreIndex).methodSignature().constData());
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
    QQmlData *ddata = QQmlData::get(object, false);
    if (!ddata || ddata->indestructible || ddata->rootObjectInCreation) {
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

    QQmlPropertyData method;

    if (QQmlData *ddata = static_cast<QQmlData *>(QObjectPrivate::get(object)->declarativeData)) {
        if (ddata->propertyCache) {
            QQmlPropertyData *d = ddata->propertyCache->method(index);
            if (!d) 
                return v8::Undefined();
            method = *d;
        } 
    }

    if (method.coreIndex == -1) {
        method.load(object->metaObject()->method(index));

        if (method.coreIndex == -1)
            return v8::Undefined();
    }

    if (method.isV8Function()) {
        v8::Handle<v8::Value> rv;
        v8::Handle<v8::Object> qmlglobal = args[2]->ToObject();

        QQmlV8Function func(argCount, arguments, rv, qmlglobal, 
                                    resource->engine->contextWrapper()->context(qmlglobal),
                                    resource->engine);
        QQmlV8Function *funcptr = &func;

        void *args[] = { 0, &funcptr };
        QMetaObject::metacall(object, QMetaObject::InvokeMetaMethod, method.coreIndex, args);

        if (rv.IsEmpty()) return v8::Undefined();
        return rv;
    }

    CallArgs callArgs(argCount, &arguments);
    if (!method.isOverload()) {
        return CallPrecise(object, method, resource->engine, callArgs);
    } else {
        return CallOverloaded(object, method, resource->engine, callArgs);
    }
}

CallArgument::CallArgument()
: type(QVariant::Invalid)
{
}

CallArgument::~CallArgument()
{
    cleanup();
}

void CallArgument::cleanup()
{
    if (type == QMetaType::QString) {
        qstringPtr->~QString();
    } else if (type == -1 || type == QMetaType::QVariant) {
        qvariantPtr->~QVariant();
    } else if (type == qMetaTypeId<QJSValue>()) {
        qjsValuePtr->~QJSValue();
    } else if (type == qMetaTypeId<QList<QObject *> >()) {
        qlistPtr->~QList<QObject *>();
    }  else if (type == QMetaType::QJsonArray) {
        jsonArrayPtr->~QJsonArray();
    }  else if (type == QMetaType::QJsonObject) {
        jsonObjectPtr->~QJsonObject();
    }  else if (type == QMetaType::QJsonValue) {
        jsonValuePtr->~QJsonValue();
    } 
}

void *CallArgument::dataPtr()
{
    if (type == -1)
        return qvariantPtr->data();
    else
        return (void *)&allocData;
}

void CallArgument::initAsType(int callType)
{
    if (type != 0) { cleanup(); type = 0; }
    if (callType == QMetaType::UnknownType) return;

    if (callType == qMetaTypeId<QJSValue>()) {
        qjsValuePtr = new (&allocData) QJSValue();
        type = callType;
    } else if (callType == QMetaType::Int ||
               callType == QMetaType::UInt ||
               callType == QMetaType::Bool ||
               callType == QMetaType::Double ||
               callType == QMetaType::Float) {
        type = callType;
    } else if (callType == QMetaType::QObjectStar) {
        qobjectPtr = 0;
        type = callType;
    } else if (callType == QMetaType::QString) {
        qstringPtr = new (&allocData) QString();
        type = callType;
    } else if (callType == QMetaType::QVariant) {
        type = callType;
        qvariantPtr = new (&allocData) QVariant();
    } else if (callType == qMetaTypeId<QList<QObject *> >()) {
        type = callType;
        qlistPtr = new (&allocData) QList<QObject *>();
    } else if (callType == qMetaTypeId<QQmlV8Handle>()) {
        type = callType;
        handlePtr = new (&allocData) QQmlV8Handle;
    } else if (callType == QMetaType::QJsonArray) {
        type = callType;
        jsonArrayPtr = new (&allocData) QJsonArray();
    } else if (callType == QMetaType::QJsonObject) {
        type = callType;
        jsonObjectPtr = new (&allocData) QJsonObject();
    } else if (callType == QMetaType::QJsonValue) {
        type = callType;
        jsonValuePtr = new (&allocData) QJsonValue();
    } else if (callType == QMetaType::Void) {
        type = -1;
        qvariantPtr = new (&allocData) QVariant();
    } else {
        type = -1;
        qvariantPtr = new (&allocData) QVariant(callType, (void *)0);
    }
}

void CallArgument::fromValue(int callType, QV8Engine *engine, v8::Handle<v8::Value> value)
{
    if (type != 0) { cleanup(); type = 0; }

    if (callType == qMetaTypeId<QJSValue>()) {
        qjsValuePtr = new (&allocData) QJSValue(QJSValuePrivate::get(new QJSValuePrivate(engine, value)));
        type = qMetaTypeId<QJSValue>();
    } else if (callType == QMetaType::Int) {
        intValue = quint32(value->Int32Value());
        type = callType;
    } else if (callType == QMetaType::UInt) {
        intValue = quint32(value->Uint32Value());
        type = callType;
    } else if (callType == QMetaType::Bool) {
        boolValue = value->BooleanValue();
        type = callType;
    } else if (callType == QMetaType::Double) {
        doubleValue = double(value->NumberValue());
        type = callType;
    } else if (callType == QMetaType::Float) {
        floatValue = float(value->NumberValue());
        type = callType;
    } else if (callType == QMetaType::QString) {
        if (value->IsNull() || value->IsUndefined())
            qstringPtr = new (&allocData) QString();
        else
            qstringPtr = new (&allocData) QString(engine->toString(value->ToString()));
        type = callType;
    } else if (callType == QMetaType::QObjectStar) {
        qobjectPtr = engine->toQObject(value);
        type = callType;
    } else if (callType == qMetaTypeId<QVariant>()) {
        qvariantPtr = new (&allocData) QVariant(engine->toVariant(value, -1));
        type = callType;
    } else if (callType == qMetaTypeId<QList<QObject*> >()) {
        qlistPtr = new (&allocData) QList<QObject *>();
        if (value->IsArray()) {
            v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(value);
            uint32_t length = array->Length();
            for (uint32_t ii = 0; ii < length; ++ii) 
                qlistPtr->append(engine->toQObject(array->Get(ii)));
        } else {
            qlistPtr->append(engine->toQObject(value));
        }
        type = callType;
    } else if (callType == qMetaTypeId<QQmlV8Handle>()) {
        handlePtr = new (&allocData) QQmlV8Handle(QQmlV8Handle::fromHandle(value));
        type = callType;
    } else if (callType == QMetaType::QJsonArray) {
        jsonArrayPtr = new (&allocData) QJsonArray(engine->jsonArrayFromJS(value));
        type = callType;
    } else if (callType == QMetaType::QJsonObject) {
        jsonObjectPtr = new (&allocData) QJsonObject(engine->jsonObjectFromJS(value));
        type = callType;
    } else if (callType == QMetaType::QJsonValue) {
        jsonValuePtr = new (&allocData) QJsonValue(engine->jsonValueFromJS(value));
        type = callType;
    } else if (callType == QMetaType::Void) {
        *qvariantPtr = QVariant();
    } else {
        qvariantPtr = new (&allocData) QVariant();
        type = -1;

        QQmlEnginePrivate *ep = engine->engine() ? QQmlEnginePrivate::get(engine->engine()) : 0;
        QVariant v = engine->toVariant(value, -1); // why -1 instead of callType?

        if (v.userType() == callType) {
            *qvariantPtr = v;
        } else if (v.canConvert(callType)) {
            *qvariantPtr = v;
            qvariantPtr->convert(callType);
        } else if (engine->sequenceWrapper()->isSequenceType(callType) && v.userType() == qMetaTypeId<QVariantList>()) {
            // convert the JS array to a sequence of the correct type.
            QVariant seqV = engine->toVariant(value, callType);
            *qvariantPtr = seqV;
        } else {
            QQmlMetaObject mo = ep ? ep->rawMetaObjectForType(callType) : QQmlMetaObject();
            if (!mo.isNull()) {
                QObject *obj = ep->toQObject(v);

                if (obj != 0 && !QQmlMetaObject::canConvert(obj, mo))
                    obj = 0;

                *qvariantPtr = QVariant(callType, &obj);
            } else {
                *qvariantPtr = QVariant(callType, (void *)0);
            }
        }
    }
}

v8::Handle<v8::Value> CallArgument::toValue(QV8Engine *engine)
{
    if (type == qMetaTypeId<QJSValue>()) {
        return QJSValuePrivate::get(*qjsValuePtr)->asV8Value(engine);
    } else if (type == QMetaType::Int) {
        return v8::Integer::New(int(intValue));
    } else if (type == QMetaType::UInt) {
        return v8::Integer::NewFromUnsigned(intValue);
    } else if (type == QMetaType::Bool) {
        return v8::Boolean::New(boolValue);
    } else if (type == QMetaType::Double) {
        return v8::Number::New(doubleValue);
    } else if (type == QMetaType::Float) {
        return v8::Number::New(floatValue);
    } else if (type == QMetaType::QString) {
        return engine->toString(*qstringPtr);
    } else if (type == QMetaType::QObjectStar) {
        QObject *object = qobjectPtr;
        if (object)
            QQmlData::get(object, true)->setImplicitDestructible();
        return engine->newQObject(object);
    } else if (type == qMetaTypeId<QList<QObject *> >()) {
        // XXX Can this be made more by using Array as a prototype and implementing
        // directly against QList<QObject*>?
        QList<QObject *> &list = *qlistPtr;
        v8::Local<v8::Array> array = v8::Array::New(list.count());
        for (int ii = 0; ii < list.count(); ++ii) 
            array->Set(ii, engine->newQObject(list.at(ii)));
        return array;
    } else if (type == qMetaTypeId<QQmlV8Handle>()) {
        return handlePtr->toHandle();
    } else if (type == QMetaType::QJsonArray) {
        return engine->jsonArrayToJS(*jsonArrayPtr);
    } else if (type == QMetaType::QJsonObject) {
        return engine->jsonObjectToJS(*jsonObjectPtr);
    } else if (type == QMetaType::QJsonValue) {
        return engine->jsonValueToJS(*jsonValuePtr);
    } else if (type == -1 || type == qMetaTypeId<QVariant>()) {
        QVariant value = *qvariantPtr;
        v8::Handle<v8::Value> rv = engine->fromVariant(value);
        if (QObject *object = engine->toQObject(rv)) 
            QQmlData::get(object, true)->setImplicitDestructible();
        return rv;
    } else {
        return v8::Undefined();
    }
}

QT_END_NAMESPACE

