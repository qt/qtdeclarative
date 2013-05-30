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
#include "qv8engine_p.h"

#include <private/qqmlguard_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qjsvalue_p.h>
#include <private/qqmlaccessors_p.h>
#include <private/qqmlexpression_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qqmllistwrapper_p.h>

#include <private/qv4functionobject_p.h>
#include <private/qv4runtime_p.h>
#include <private/qv4variantobject_p.h>

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


using namespace QV4;

QObjectWrapper::QObjectWrapper(ExecutionEngine *engine, QObject *object)
    : Object(engine)
    , object(object)
{
    this->v8Engine = QV8Engine::get(engine->publicEngine);
    type = Type_QObject;
    vtbl = &static_vtbl;

    m_destroy = engine->newIdentifier(QStringLiteral("destroy"));
    m_toString = engine->newIdentifier(QStringLiteral("toString"));
}

QObjectWrapper::~QObjectWrapper()
{
    deleteQObject();
}

void QObjectWrapper::deleteQObject(bool deleteInstantly)
{
    if (!object)
        return;
    QQmlData *ddata = QQmlData::get(object, false);
    if (!ddata)
        return;
    if (!object->parent() && !ddata->indestructible) {
        // This object is notionally destroyed now
        if (ddata->ownContext && ddata->context)
            ddata->context->emitDestruction();
        ddata->isQueuedForDeletion = true;
        if (deleteInstantly)
            delete object;
        else
            object->deleteLater();
    }
}

QV4::Value QObjectWrapper::get(Managed *m, ExecutionContext *ctx, String *name, bool *hasProperty)
{
    QObjectWrapper *that = static_cast<QObjectWrapper*>(m);

    if (QQmlData::wasDeleted(that->object)) {
        if (hasProperty)
            *hasProperty = false;
        return QV4::Value::undefinedValue();
    }

    if (name->isEqualTo(that->m_destroy) || name->isEqualTo(that->m_toString)) {
        bool hasProp = false;
        QV4::Value method = QV4::Object::get(m, ctx, name, &hasProp);

        if (!hasProp) {
            int index = name->isEqualTo(that->m_destroy) ? QV4::QObjectMethod::DestroyMethod : QV4::QObjectMethod::ToStringMethod;
            method = QV4::Value::fromObject(new (ctx->engine->memoryManager) QV4::QObjectMethod(ctx->engine->rootContext, that->object, index, QV4::Value::undefinedValue()));
            QV4::Object::put(m, ctx, name, method);
        }

        if (hasProperty)
            *hasProperty = true;

        return method;
    }

    QHashedV4String propertystring(QV4::Value::fromString(name));

    QV8Engine *v8engine = that->v8Engine;
    QQmlContextData *context = v8engine->callingContext();

    v8::Handle<v8::Value> result = QV8QObjectWrapper::GetProperty(v8engine, that->object, propertystring,
                                                                  context, QV8QObjectWrapper::IgnoreRevision);
    if (!result.IsEmpty()) {
        if (hasProperty)
            *hasProperty = true;
        return result->v4Value();
    }

    if (name->startsWithUpper()) {
        // Check for attached properties
        if (context && context->imports) {
            QQmlTypeNameCache::Result r = context->imports->query(propertystring);

            if (r.isValid()) {
                if (r.scriptIndex != -1) {
                    return QV4::Value::undefinedValue();
                } else if (r.type) {
                    return QmlTypeWrapper::create(v8engine, that->object, r.type, QmlTypeWrapper::ExcludeEnums);
                } else if (r.importNamespace) {
                    return QmlTypeWrapper::create(v8engine, that->object, context->imports, r.importNamespace, QmlTypeWrapper::ExcludeEnums);
                }
                Q_ASSERT(!"Unreachable");
            }
        }
    }

    return QV4::Object::get(m, ctx, name, hasProperty);
}

void QObjectWrapper::put(Managed *m, ExecutionContext *ctx, String *name, const Value &value)
{
    QObjectWrapper *that = static_cast<QObjectWrapper*>(m);

    if (QQmlData::wasDeleted(that->object))
        return;

    QObject *object = that->object;

    QHashedV4String propertystring(QV4::Value::fromString(name));

    QV8Engine *v8engine = that->v8Engine;
    QQmlContextData *context = v8engine->callingContext();
    bool result = QV8QObjectWrapper::SetProperty(v8engine, object, propertystring, context, value, QV8QObjectWrapper::IgnoreRevision);

    if (!result) {
        QString error = QLatin1String("Cannot assign to non-existent property \"") +
                        name->toQString() + QLatin1Char('\"');
        ctx->throwError(error);
    }
}

QV4::Value QObjectWrapper::enumerateProperties(Object *object)
{
    QObjectWrapper *that = static_cast<QObjectWrapper*>(object);

    if (that->object.isNull())
        return QV4::Value::undefinedValue();

    QStringList result;

    QQmlEnginePrivate *ep = that->v8Engine->engine()
            ? QQmlEnginePrivate::get(that->v8Engine->engine())
            : 0;

    QQmlPropertyCache *cache = 0;
    QQmlData *ddata = QQmlData::get(that->object);
    if (ddata)
        cache = ddata->propertyCache;

    if (!cache) {
        cache = ep ? ep->cache(that->object) : 0;
        if (cache) {
            if (ddata) { cache->addref(); ddata->propertyCache = cache; }
        } else {
            // Not cachable - fall back to QMetaObject (eg. dynamic meta object)
            const QMetaObject *mo = that->object->metaObject();
            int pc = mo->propertyCount();
            int po = mo->propertyOffset();
            for (int i=po; i<pc; ++i)
                result << QString::fromUtf8(mo->property(i).name());
        }
    } else {
        result = cache->propertyNames();
    }

    return QV4::Value::fromObject(that->engine()->newArrayObject(result));
}

void QObjectWrapper::markObjects(Managed *that)
{
    QObjectWrapper *This = static_cast<QObjectWrapper*>(that);

    QQmlVMEMetaObject *vme = QQmlVMEMetaObject::get(This->object);
    if (vme)
        vme->mark();

    QV4::Object::markObjects(that);
}

DEFINE_MANAGED_VTABLE(QObjectWrapper);

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
    }

    virtual void objectDestroyed(QObject *o)
    {
        if (wrapper)
            wrapper->m_taintedObjects.remove(o);
        delete this;
    }

    QV4::WeakValue v8object;
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
    inline void fromValue(int type, QV8Engine *, const QV4::Value&);
    inline QV4::Value toValue(QV8Engine *);

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
                                QQmlV4Handle,
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
        QQmlV4Handle *handlePtr;
        QJsonArray *jsonArrayPtr;
        QJsonObject *jsonObjectPtr;
        QJsonValue *jsonValuePtr;
    };

    int type;
};
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

static inline QV4::Value valueToHandle(QV8Engine *, int v)
{ return QV4::Value::fromInt32(v); }
static inline QV4::Value valueToHandle(QV8Engine *, uint v)
{ return QV4::Value::fromUInt32(v); }
static inline QV4::Value valueToHandle(QV8Engine *, bool v)
{ return QV4::Value::fromBoolean(v); }
static inline QV4::Value valueToHandle(QV8Engine *e, const QString &v)
{ return e->toString(v); }
static inline QV4::Value valueToHandle(QV8Engine *, float v)
{ return QV4::Value::fromDouble(v); }
static inline QV4::Value valueToHandle(QV8Engine *, double v)
{ return QV4::Value::fromDouble(v); }
static inline QV4::Value valueToHandle(QV8Engine *e, QObject *v)
{ return e->newQObject(v); }

void QV8QObjectWrapper::init(QV8Engine *engine)
{
    m_engine = engine;

    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);

    v8::Handle<v8::Function> connect = V8FUNCTION(Connect, engine);
    v8::Handle<v8::Function> disconnect = V8FUNCTION(Disconnect, engine);

    {
    v8::Handle<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->PrototypeTemplate()->Set(v8::String::New("connect"), connect, v8::DontEnum);
    ft->PrototypeTemplate()->Set(v8::String::New("disconnect"), disconnect, v8::DontEnum);
    m_signalHandlerConstructor = ft->GetFunction()->v4Value();
    }

    {
    v8::Handle<v8::Object> prototype = v8::Handle<v8::Object>(engine->global())
            ->Get(v8::String::New("Function"))->ToObject()->Get(v8::String::New("prototype"))->ToObject();
    prototype->Set(v8::String::New("connect"), connect, v8::DontEnum);
    prototype->Set(v8::String::New("disconnect"), disconnect, v8::DontEnum);
    }
}

bool QV8QObjectWrapper::isQObject(v8::Handle<v8::Object> obj)
{
    return obj->v4Value().asQObjectWrapper() != 0;
}

QObject *QV8QObjectWrapper::toQObject(v8::Handle<v8::Object> obj)
{
    QV4::QObjectWrapper *wrapper =  obj->v4Value().asQObjectWrapper();
    return wrapper?wrapper->object:0;
}

// Load value properties
template<void (*ReadFunction)(QObject *, const QQmlPropertyData &,
                              void *, QQmlNotifier **)>
static QV4::Value LoadProperty(QV8Engine *engine, QObject *object,
                                          const QQmlPropertyData &property,
                                          QQmlNotifier **notifier)
{
    Q_ASSERT(!property.isFunction());

    if (property.isQObject()) {
        QObject *rv = 0;
        ReadFunction(object, property, &rv, notifier);
        return engine->newQObject(rv);
    } else if (property.isQList()) {
        return QmlListWrapper::create(engine, object, property.coreIndex, property.propType);
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
    } else if (property.isV4Handle()) {
        QQmlV4Handle handle;
        ReadFunction(object, property, &handle, notifier);
        return handle.toValue();
    } else if (property.propType == qMetaTypeId<QJSValue>()) {
        QJSValue v;
        ReadFunction(object, property, &v, notifier);
        return QJSValuePrivate::get(v)->getValue(QV8Engine::getV4(engine));
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
        QV4::Value retn = engine->newSequence(property.propType, object, property.coreIndex, &succeeded);
        if (succeeded)
            return retn;
    }

    if (property.propType == QMetaType::UnknownType) {
        QMetaProperty p = object->metaObject()->property(property.coreIndex);
        qWarning("QMetaProperty::read: Unable to handle unregistered datatype '%s' for property "
                 "'%s::%s'", p.typeName(), object->metaObject()->className(), p.name());
        return QV4::Value::undefinedValue();
    } else {
        QVariant v(property.propType, (void *)0);
        ReadFunction(object, property, v.data(), notifier);
        return engine->fromVariant(v);
    }
}

QV4::Value QV8QObjectWrapper::GetProperty(QV8Engine *engine, QObject *object,
                                                     const QHashedV4String &property,
                                                     QQmlContextData *context,
                                                     QV8QObjectWrapper::RevisionMode revisionMode)
{
    // XXX More recent versions of V8 introduced "Callable" objects.  It is possible that these
    // will be a faster way of creating QObject method objects.
    struct MethodClosure {
       static QV4::Value create(QV8Engine *engine, QObject *object,
                                           int index) {
           QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
           return QV4::Value::fromObject(new (v4->memoryManager) QV4::QObjectMethod(v4->rootContext, object, index, QV4::Value::undefinedValue()));
       }
       static QV4::Value createWithGlobal(QV8Engine *engine, QObject *object,
                                                     int index) {
           QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
           return QV4::Value::fromObject(new (v4->memoryManager) QV4::QObjectMethod(v4->rootContext, object, index, QV4::Value::fromObject(QV8Engine::getV4(engine)->qmlContextObject())));
       }
    };

    if (QQmlData::wasDeleted(object))
        return QV4::Value::emptyValue();

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    {
        QQmlData *ddata = QQmlData::get(object, false);
        if (ddata && ddata->propertyCache)
            result = ddata->propertyCache->property(property, object, context);
        else
            result = QQmlPropertyCache::property(engine->engine(), object, property, context, local);
    }

    if (!result)
        return QV4::Value::emptyValue();

    QQmlData::flushPendingBinding(object, result->coreIndex);

    if (revisionMode == QV8QObjectWrapper::CheckRevision && result->hasRevision()) {
        QQmlData *ddata = QQmlData::get(object);
        if (ddata && ddata->propertyCache && !ddata->propertyCache->isAllowedInRevision(result))
            return QV4::Value::emptyValue();
    }

    if (result->isFunction() && !result->isVarProperty()) {
        if (result->isVMEFunction()) {
            QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(object);
            Q_ASSERT(vmemo);
            return vmemo->vmeMethod(result->coreIndex);
        } else if (result->isV4Function()) {
            return MethodClosure::createWithGlobal(engine, object, result->coreIndex);
        } else if (result->isSignalHandler()) {
            v8::Handle<v8::Object> handler = engine->qobjectWrapper()->m_signalHandlerConstructor.value().asFunctionObject()->newInstance();
            QV8SignalHandlerResource *r = new QV8SignalHandlerResource(engine, object, result->coreIndex);
            handler->SetExternalResource(r);
            return handler->v4Value();
        } else {
            return MethodClosure::create(engine, object, result->coreIndex);
        }
    }

    QQmlEnginePrivate *ep =
        engine->engine()?QQmlEnginePrivate::get(engine->engine()):0;

    if (result->hasAccessors()) {
        QQmlNotifier *n = 0;
        QQmlNotifier **nptr = 0;

        if (ep && ep->propertyCapture && result->accessors->notifier)
            nptr = &n;

        QV4::Value rv = LoadProperty<ReadAccessor::Accessor>(engine, object, *result, nptr);

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
        QV4::FunctionObject *f = value->v4Value().asFunctionObject();
        if (!f->bindingKeyFlag) {
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

            QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
            QV4::ExecutionEngine::StackFrame frame = v4->currentStackFrame();

            newBinding = new QQmlBinding(function->v4Value(), object, context, frame.source,
                                         qmlSourceCoordinate(frame.line), qmlSourceCoordinate(frame.column));
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
        vmemo->setVMEProperty(property->coreIndex, value->v4Value());
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
        PROPERTY_STORE(QJSValue, new QJSValuePrivate(QV8Engine::getV4(engine), value->v4Value()));
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
        PROPERTY_STORE(int, qRound(value->v4Value().asDouble()));
    } else if (property->propType == QMetaType::QReal && value->IsNumber()) {
        PROPERTY_STORE(qreal, qreal(value->v4Value().asDouble()));
    } else if (property->propType == QMetaType::Float && value->IsNumber()) {
        PROPERTY_STORE(float, float(value->v4Value().asDouble()));
    } else if (property->propType == QMetaType::Double && value->IsNumber()) {
        PROPERTY_STORE(double, double(value->v4Value().asDouble()));
    } else if (property->propType == QMetaType::QString && value->IsString()) {
        PROPERTY_STORE(QString, value->v4Value().toQString());
    } else if (property->isVarProperty()) {
        QQmlVMEMetaObject *vmemo = QQmlVMEMetaObject::get(object);
        Q_ASSERT(vmemo);
        vmemo->setVMEProperty(property->coreIndex, value->v4Value());
    } else {
        QVariant v;
        if (property->isQList()) 
            v = engine->toVariant(value->v4Value(), qMetaTypeId<QList<QObject *> >());
        else
            v = engine->toVariant(value->v4Value(), property->propType);

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

bool QV8QObjectWrapper::SetProperty(QV8Engine *engine, QObject *object, const QHashedV4String &property, QQmlContextData *context,
                                    v8::Handle<v8::Value> value, QV8QObjectWrapper::RevisionMode revisionMode)
{
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
                        property.toString() + QLatin1Char('\"');
        v8::ThrowException(v8::Exception::Error(engine->toString(error)));
        return true;
    }

    StoreProperty(engine, object, result, value);

    return true;
}

static void FastValueSetter(v8::Handle<v8::String>, v8::Handle<v8::Value> value,
                            const v8::AccessorInfo& info)
{
    QV4::QObjectWrapper *wrapper = info.This()->v4Value().asQObjectWrapper();

    if (QQmlData::wasDeleted(wrapper->object))
        return; 

    QObject *object = wrapper->object;

    QQmlPropertyData *property =
            (QQmlPropertyData *)v8::External::Cast(info.Data().get())->Value();

    int index = property->coreIndex;

    QQmlData *ddata = QQmlData::get(object, false);
    Q_ASSERT(ddata);
    Q_ASSERT(ddata->propertyCache);

    QQmlPropertyData *pdata = ddata->propertyCache->property(index);
    Q_ASSERT(pdata);

    Q_ASSERT(pdata->isWritable() || pdata->isQList());

    StoreProperty(wrapper->v8Engine, object, pdata, value);
}

static void FastValueSetterReadOnly(v8::Handle<v8::String> property, v8::Handle<v8::Value>,
                                    const v8::AccessorInfo& info)
{
    QV4::QObjectWrapper *wrapper = info.This()->v4Value().asQObjectWrapper();

    if (QQmlData::wasDeleted(wrapper->object))
        return; 

    QV8Engine *v8engine = wrapper->v8Engine;

    QString error = QLatin1String("Cannot assign to read-only property \"") +
                    property->v4Value().toQString() + QLatin1Char('\"');
    v8::ThrowException(v8::Exception::Error(v8engine->toString(error)));
}

v8::Handle<v8::Object> QV8QObjectWrapper::newQObject(QObject *object, QQmlData *ddata, QV8Engine *engine)
{
    if (!ddata->propertyCache && engine->engine()) {
        ddata->propertyCache = QQmlEnginePrivate::get(engine->engine())->cache(object);
        if (ddata->propertyCache) ddata->propertyCache->addref();
    }

    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
    QV4::QObjectWrapper *wrapper = new (v4->memoryManager) QV4::QObjectWrapper(v4, object);
    return QV4::Value::fromObject(wrapper);
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
        return QV4::Value::nullValue();

    QQmlData *ddata = QQmlData::get(object, true);
    if (!ddata) 
        return QV4::Value::undefinedValue();

    if (ddata->v8objectid == m_id && !ddata->v8object.isEmpty()) {
        // We own the v8object 
        return ddata->v8object.value();
    } else if (ddata->v8object.isEmpty() &&
               (ddata->v8objectid == m_id || // We own the QObject
                ddata->v8objectid == 0 ||    // No one owns the QObject
                !ddata->hasTaintedV8Object)) { // Someone else has used the QObject, but it isn't tainted

        v8::Handle<v8::Object> rv = newQObject(object, ddata, m_engine);
        ddata->v8object = rv->v4Value();
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
        if ((!found || (*iter)->v8object.isEmpty()) && ddata->v8object.isEmpty()) {
            v8::Handle<v8::Object> rv = newQObject(object, ddata, m_engine);
            ddata->v8object = rv->v4Value();
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

        if ((*iter)->v8object.isEmpty()) {
            v8::Handle<v8::Object> rv = newQObject(object, ddata, m_engine);
            (*iter)->v8object = rv->v4Value();
        }

        return (*iter)->v8object.value();
    }
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
    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
    QV4::FunctionObject *v4Function = function->v4Value().asFunctionObject();
    if (v4Function->subtype == QV4::FunctionObject::WrappedQtMethod) {
        QObjectMethod *method = static_cast<QObjectMethod*>(v4Function);
        return qMakePair(method->object(), method->methodIndex());
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

        QV4::PersistentValue thisObject;
        QV4::PersistentValue function;

        void dispose() {
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

        int argCount = argsTypes?argsTypes[0]:0;
        QVarLengthArray<QV4::Value, 9> args(argCount);

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

            QV4::FunctionObject *f = connection.function.value().asFunctionObject();
            QV4::ExecutionEngine *v4 = f->internalClass->engine;
            QV4::ExecutionContext *ctx = v4->current;
            try {
                f->call(v4->current, connection.thisObject.isEmpty() ? engine->global() : connection.thisObject.value(), args.data(), argCount);
            } catch (QV4::Exception &e) {
                e.accept(ctx);
                QQmlError error;
                QQmlExpressionPrivate::exceptionToError(e, error);
                if (error.description().isEmpty())
                    error.setDescription(QString(QLatin1String("Unknown exception occurred during evaluation of connected function: %1")).arg(f->name->toQString()));
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

QV4::Value QV8QObjectWrapper::Connect(const v8::Arguments &args)
{
    if (args.Length() == 0)
        V4THROW_ERROR("Function.prototype.connect: no arguments given");

    QV8Engine *engine = V8ENGINE();

    QPair<QObject *, int> signalInfo = ExtractQtSignal(engine, args.This());
    QObject *signalObject = signalInfo.first;
    int signalIndex = signalInfo.second;

    if (signalIndex < 0)
        V4THROW_ERROR("Function.prototype.connect: this object is not a signal");

    if (!signalObject)
        V4THROW_ERROR("Function.prototype.connect: cannot connect to deleted QObject");

    if (signalObject->metaObject()->method(signalIndex).methodType() != QMetaMethod::Signal)
        V4THROW_ERROR("Function.prototype.connect: this object is not a signal");

    v8::Handle<v8::Value> functionValue;
    v8::Handle<v8::Value> functionThisValue;

    if (args.Length() == 1) {
        functionValue = args[0];
    } else {
        functionThisValue = args[0];
        functionValue = args[1];
    }

    if (!functionValue->IsFunction())
        V4THROW_ERROR("Function.prototype.connect: target is not a function");

    if (!functionThisValue.IsEmpty() && !functionThisValue->IsObject())
        V4THROW_ERROR("Function.prototype.connect: target this is not an object");

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
        connection.thisObject = functionThisValue->ToObject()->v4Value();
    connection.function = functionValue->v4Value();

    slotIter->append(connection);

    return QV4::Value::undefinedValue();
}

QV4::Value QV8QObjectWrapper::Disconnect(const v8::Arguments &args)
{
    if (args.Length() == 0)
        V4THROW_ERROR("Function.prototype.disconnect: no arguments given");

    QV8Engine *engine = V8ENGINE();

    QPair<QObject *, int> signalInfo = ExtractQtSignal(engine, args.This());
    QObject *signalObject = signalInfo.first;
    int signalIndex = signalInfo.second;

    if (signalIndex == -1)
        V4THROW_ERROR("Function.prototype.disconnect: this object is not a signal");

    if (!signalObject)
        V4THROW_ERROR("Function.prototype.disconnect: cannot disconnect from deleted QObject");

    if (signalIndex < 0 || signalObject->metaObject()->method(signalIndex).methodType() != QMetaMethod::Signal)
        V4THROW_ERROR("Function.prototype.disconnect: this object is not a signal");

    v8::Handle<v8::Value> functionValue;
    v8::Handle<v8::Value> functionThisValue;

    if (args.Length() == 1) {
        functionValue = args[0];
    } else {
        functionThisValue = args[0];
        functionValue = args[1];
    }

    if (!functionValue->IsFunction())
        V4THROW_ERROR("Function.prototype.disconnect: target is not a function");

    if (!functionThisValue.IsEmpty() && !functionThisValue->IsObject())
        V4THROW_ERROR("Function.prototype.disconnect: target this is not an object");

    QV8QObjectWrapper *qobjectWrapper = engine->qobjectWrapper();
    QHash<QObject *, QV8QObjectConnectionList *> &connectionsList = qobjectWrapper->m_connections;
    QHash<QObject *, QV8QObjectConnectionList *>::Iterator iter = connectionsList.find(signalObject);
    if (iter == connectionsList.end()) 
        return QV4::Value::undefinedValue(); // Nothing to disconnect from

    QV8QObjectConnectionList *connectionList = *iter;
    QV8QObjectConnectionList::SlotHash::Iterator slotIter = connectionList->slotHash.find(signalIndex);
    if (slotIter == connectionList->slotHash.end()) 
        return QV4::Value::undefinedValue(); // Nothing to disconnect from

    QV8QObjectConnectionList::ConnectionList &connections = *slotIter;

    v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(functionValue);
    QPair<QObject *, int> functionData = ExtractQtMethod(engine, function);

    if (functionData.second != -1) {
        // This is a QObject function wrapper
        for (int ii = 0; ii < connections.count(); ++ii) {
            QV8QObjectConnectionList::Connection &connection = connections[ii];

            if (connection.thisObject.isEmpty() == functionThisValue.IsEmpty() &&
                (connection.thisObject.isEmpty() || __qmljs_strict_equal(connection.thisObject, functionThisValue->v4Value()))) {

                QPair<QObject *, int> connectedFunctionData = ExtractQtMethod(engine, connection.function.value());
                if (connectedFunctionData == functionData) {
                    // Match!
                    if (connections.connectionsInUse) {
                        connection.needsDestroy = true;
                        connections.connectionsNeedClean = true;
                    } else {
                        connection.dispose();
                        connections.removeAt(ii);
                    }
                    return QV4::Value::undefinedValue();
                }
            }
        }

    } else {
        // This is a normal JS function
        for (int ii = 0; ii < connections.count(); ++ii) {
            QV8QObjectConnectionList::Connection &connection = connections[ii];
            if (__qmljs_strict_equal(connection.function, function->v4Value()) &&
                connection.thisObject.isEmpty() == functionThisValue.IsEmpty() &&
                (connection.thisObject.isEmpty() || __qmljs_strict_equal(connection.thisObject, functionThisValue->v4Value()))) {
                // Match!
                if (connections.connectionsInUse) {
                    connection.needsDestroy = true;
                    connections.connectionsNeedClean = true;
                } else {
                    connection.dispose();
                    connections.removeAt(ii);
                }
                return QV4::Value::undefinedValue();
            }
        }
    }

    return QV4::Value::undefinedValue();
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
    CallArgs(int length, QV4::Value *args) : _length(length), _args(args) {}
    int Length() const { return _length; }
    QV4::Value operator[](int idx) { return _args[idx]; }

private:
    int _length;
    QV4::Value *_args;
};
}

static QV4::Value CallMethod(QObject *object, int index, int returnType, int argCount,
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
        return QV4::Value::undefinedValue();

    }
}

/*!
    Returns the match score for converting \a actual to be of type \a conversionType.  A 
    zero score means "perfect match" whereas a higher score is worse.

    The conversion table is copied out of the QtScript callQtMethod() function.
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
        QV4::Object *obj = actual->v4Value().asObject();
        QV8Engine *engine = obj->engine()->publicEngine->handle();

        if (QV4::VariantObject *v = obj->asVariantObject()) {
            if (conversionType == qMetaTypeId<QVariant>())
                return 0;
            if (engine->toVariant(actual->v4Value(), -1).userType() == conversionType)
                return 0;
            else
                return 10;
        }

        if (obj->asQObjectWrapper()) {
            switch (conversionType) {
            case QMetaType::QObjectStar:
                return 0;
            default:
                return 10;
            }
        }

        if (QV4::QmlValueTypeWrapper *w = obj->asQmlValueTypeWrapper()) {
            if (engine->toVariant(actual->v4Value(), -1).userType() == conversionType)
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

static QV4::Value CallPrecise(QObject *object, const QQmlPropertyData &data,
                                         QV8Engine *engine, CallArgs &callArgs)
{
    QByteArray unknownTypeError;

    int returnType = QQmlPropertyCache::methodReturnType(object, data, &unknownTypeError);

    if (returnType == QMetaType::UnknownType) {
        QString typeName = QString::fromLatin1(unknownTypeError);
        QString error = QString::fromLatin1("Unknown method return type: %1").arg(typeName);
        QV8Engine::getV4(engine)->current->throwError(error);
    }

    if (data.hasArguments()) {

        int *args = 0;
        QVarLengthArray<int, 9> dummy;

        args = QQmlPropertyCache::methodParameterTypes(object, data.coreIndex, dummy, 
                                                               &unknownTypeError);

        if (!args) {
            QString typeName = QString::fromLatin1(unknownTypeError);
            QString error = QString::fromLatin1("Unknown method parameter type: %1").arg(typeName);
            QV8Engine::getV4(engine)->current->throwError(error);
        }

        if (args[0] > callArgs.Length()) {
            QString error = QLatin1String("Insufficient arguments");
            QV8Engine::getV4(engine)->current->throwError(error);
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
static QV4::Value CallOverloaded(QObject *object, const QQmlPropertyData &data,
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

        QV8Engine::getV4(engine)->current->throwError(error);
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
    } else if (callType == qMetaTypeId<QQmlV4Handle>()) {
        type = callType;
        handlePtr = new (&allocData) QQmlV4Handle;
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

void CallArgument::fromValue(int callType, QV8Engine *engine, const QV4::Value &value)
{
    if (type != 0) { cleanup(); type = 0; }

    if (callType == qMetaTypeId<QJSValue>()) {
        qjsValuePtr = new (&allocData) QJSValue(new QJSValuePrivate(QV8Engine::getV4(engine), value));
        type = qMetaTypeId<QJSValue>();
    } else if (callType == QMetaType::Int) {
        intValue = quint32(value.toInt32());
        type = callType;
    } else if (callType == QMetaType::UInt) {
        intValue = quint32(value.toUInt32());
        type = callType;
    } else if (callType == QMetaType::Bool) {
        boolValue = value.toBoolean();
        type = callType;
    } else if (callType == QMetaType::Double) {
        doubleValue = double(value.toNumber());
        type = callType;
    } else if (callType == QMetaType::Float) {
        floatValue = float(value.toNumber());
        type = callType;
    } else if (callType == QMetaType::QString) {
        if (value.isNull() || value.isUndefined())
            qstringPtr = new (&allocData) QString();
        else
            qstringPtr = new (&allocData) QString(value.toQString());
        type = callType;
    } else if (callType == QMetaType::QObjectStar) {
        qobjectPtr = engine->toQObject(value);
        type = callType;
    } else if (callType == qMetaTypeId<QVariant>()) {
        qvariantPtr = new (&allocData) QVariant(engine->toVariant(value, -1));
        type = callType;
    } else if (callType == qMetaTypeId<QList<QObject*> >()) {
        qlistPtr = new (&allocData) QList<QObject *>();
        if (QV4::ArrayObject *array = value.asArrayObject()) {
            uint32_t length = array->arrayLength();
            for (uint32_t ii = 0; ii < length; ++ii) 
                qlistPtr->append(engine->toQObject(array->getIndexed(ii)));
        } else {
            qlistPtr->append(engine->toQObject(value));
        }
        type = callType;
    } else if (callType == qMetaTypeId<QQmlV4Handle>()) {
        handlePtr = new (&allocData) QQmlV4Handle(QQmlV4Handle(value));
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
        } else if (QV4::SequencePrototype::isSequenceType(callType) && v.userType() == qMetaTypeId<QVariantList>()) {
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

QV4::Value CallArgument::toValue(QV8Engine *engine)
{
    if (type == qMetaTypeId<QJSValue>()) {
        return QJSValuePrivate::get(*qjsValuePtr)->getValue(QV8Engine::getV4(engine));
    } else if (type == QMetaType::Int) {
        return QV4::Value::fromInt32(int(intValue));
    } else if (type == QMetaType::UInt) {
        return QV4::Value::fromUInt32(intValue);
    } else if (type == QMetaType::Bool) {
        return QV4::Value::fromBoolean(boolValue);
    } else if (type == QMetaType::Double) {
        return QV4::Value::fromDouble(doubleValue);
    } else if (type == QMetaType::Float) {
        return QV4::Value::fromDouble(floatValue);
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
        QV4::ArrayObject *array = QV8Engine::getV4(engine)->newArrayObject();
        array->arrayReserve(list.count());
        for (int ii = 0; ii < list.count(); ++ii) 
            array->arrayData[ii].value = engine->newQObject(list.at(ii));
        array->arrayDataLen = list.count();
        array->setArrayLengthUnchecked(list.count());
        return QV4::Value::fromObject(array);
    } else if (type == qMetaTypeId<QQmlV4Handle>()) {
        return handlePtr->toValue();
    } else if (type == QMetaType::QJsonArray) {
        return engine->jsonArrayToJS(*jsonArrayPtr);
    } else if (type == QMetaType::QJsonObject) {
        return engine->jsonObjectToJS(*jsonObjectPtr);
    } else if (type == QMetaType::QJsonValue) {
        return engine->jsonValueToJS(*jsonValuePtr);
    } else if (type == -1 || type == qMetaTypeId<QVariant>()) {
        QVariant value = *qvariantPtr;
        QV4::Value rv = engine->fromVariant(value);
        if (QObject *object = engine->toQObject(rv))
            QQmlData::get(object, true)->setImplicitDestructible();
        return rv;
    } else {
        return QV4::Value::undefinedValue();
    }
}

QObjectMethod::QObjectMethod(ExecutionContext *scope, QObject *object, int index, const Value &qmlGlobal)
    : FunctionObject(scope)
    , m_object(object)
    , m_index(index)
    , m_qmlGlobal(qmlGlobal)
{
    vtbl = &static_vtbl;
    subtype = WrappedQtMethod;
}

QV4::Value QObjectMethod::method_toString(QV4::ExecutionContext *ctx)
{
    QString result;
    if (m_object) {
        QString objectName = m_object->objectName();

        result += QString::fromUtf8(m_object->metaObject()->className());
        result += QLatin1String("(0x");
        result += QString::number((quintptr)m_object.object(),16);

        if (!objectName.isEmpty()) {
            result += QLatin1String(", \"");
            result += objectName;
            result += QLatin1Char('\"');
        }

        result += QLatin1Char(')');
    } else {
        result = QLatin1String("null");
    }

    return QV4::Value::fromString(ctx, result);
}

QV4::Value QObjectMethod::method_destroy(QV4::ExecutionContext *ctx, Value *args, int argc)
{
    if (!m_object)
        return QV4::Value::undefinedValue();
    if (QQmlData::keepAliveDuringGarbageCollection(m_object))
        ctx->throwError(QStringLiteral("Invalid attempt to destroy() an indestructible object"));

    int delay = 0;
    if (argc > 0)
        delay = args[0].toUInt32();

    if (delay > 0)
        QTimer::singleShot(delay, m_object, SLOT(deleteLater()));
    else
        m_object->deleteLater();

    return QV4::Value::undefinedValue();
}

Value QObjectMethod::call(Managed *m, ExecutionContext *context, const Value &thisObject, Value *args, int argc)
{
    QObjectMethod *This = static_cast<QObjectMethod*>(m);
    return This->callInternal(context, thisObject, args, argc);
}

Value QObjectMethod::callInternal(ExecutionContext *context, const Value &thisObject, Value *args, int argc)
{
    if (m_index == DestroyMethod)
        return method_destroy(context, args, argc);
    else if (m_index == ToStringMethod)
        return method_toString(context);

    QObject *object = m_object.data();
    if (!object)
        return QV4::Value::undefinedValue();

    QQmlData *ddata = QQmlData::get(object);
    if (!ddata)
        return QV4::Value::undefinedValue();

    QV8Engine *v8Engine = QV8Engine::get(context->engine->publicEngine);

    QQmlPropertyData method;

    if (QQmlData *ddata = static_cast<QQmlData *>(QObjectPrivate::get(object)->declarativeData)) {
        if (ddata->propertyCache) {
            QQmlPropertyData *d = ddata->propertyCache->method(m_index);
            if (!d)
                return QV4::Value::undefinedValue();
            method = *d;
        }
    }

    if (method.coreIndex == -1) {
        method.load(object->metaObject()->method(m_index));

        if (method.coreIndex == -1)
            return QV4::Value::undefinedValue();
    }

    if (method.isV4Function()) {
        QV4::Value rv = QV4::Value::undefinedValue();

        QQmlV4Function func(argc, args, &rv, m_qmlGlobal.value(),
                            QmlContextWrapper::getContext(m_qmlGlobal.value()),
                            v8Engine);
        QQmlV4Function *funcptr = &func;

        void *args[] = { 0, &funcptr };
        QMetaObject::metacall(object, QMetaObject::InvokeMetaMethod, method.coreIndex, args);

        return rv;
    }

    CallArgs callArgs(argc, args);
    if (!method.isOverload()) {
        return CallPrecise(object, method, v8Engine, callArgs);
    } else {
        return CallOverloaded(object, method, v8Engine, callArgs);
    }
}

DEFINE_MANAGED_VTABLE(QObjectMethod);

QT_END_NAMESPACE

