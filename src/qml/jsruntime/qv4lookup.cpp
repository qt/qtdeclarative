// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4lookup_p.h"

#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4runtime_p.h>
#include <private/qv4stackframe_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;


void Lookup::resolveProtoGetter(PropertyKey name, const Heap::Object *proto)
{
    while (proto) {
        auto index = proto->internalClass->findValueOrGetter(name);
        if (index.isValid()) {
            PropertyAttributes attrs = index.attrs;
            protoLookup.data = proto->propertyData(index.index);
            if (attrs.isData()) {
                call = Call::GetterProto;
            } else {
                call = Call::GetterProtoAccessor;
            }
            return;
        }
        proto = proto->prototype();
    }
    // ### put in a getterNotFound!
    call = Call::GetterQObjectPropertyFallback;
}

ReturnedValue Lookup::resolveGetter(ExecutionEngine *engine, const Object *object)
{
    return object->resolveLookupGetter(engine, this);
}

ReturnedValue Lookup::resolvePrimitiveGetter(ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    primitiveLookup.type = object.type();
    switch (primitiveLookup.type) {
    case Value::Undefined_Type:
    case Value::Null_Type: {
        Scope scope(engine);
        ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
        const QString message = QStringLiteral("Cannot read property '%1' of %2").arg(name->toQString())
            .arg(QLatin1String(primitiveLookup.type == Value::Undefined_Type ? "undefined" : "null"));
        return engine->throwTypeError(message);
    }
    case Value::Boolean_Type:
        primitiveLookup.proto.set(engine, engine->booleanPrototype()->d());
        break;
    case Value::Managed_Type: {
        // ### Should move this over to the Object path, as strings also have an internalClass
        Q_ASSERT(object.isStringOrSymbol());
        primitiveLookup.proto.set(engine, static_cast<const Managed &>(object).internalClass()->prototype);
        Q_ASSERT(primitiveLookup.proto);
        Scope scope(engine);
        ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
        if (object.isString() && name->equals(engine->id_length())) {
            // special case, as the property is on the object itself
            call = Call::GetterStringLength;
            return stringLengthGetter(this, engine, object);
        }
        break;
    }
    case Value::Integer_Type:
    default: // Number
        primitiveLookup.proto.set(engine, engine->numberPrototype()->d());
    }

    PropertyKey name = engine->identifierTable->asPropertyKey(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    protoLookup.protoId = primitiveLookup.proto->internalClass->protoId;
    resolveProtoGetter(name, primitiveLookup.proto);

    switch (call) {
    case Call::GetterProto:
        call = Call::GetterProtoPrimitive;
        break;
    case Call::GetterProtoAccessor:
        call = Call::GetterAccessorPrimitive;
        break;
    default:
        break;
    }

    return getter(engine, object);
}

ReturnedValue Lookup::resolveGlobalGetter(ExecutionEngine *engine)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    Object *o = engine->globalObject;
    PropertyKey name = engine->identifierTable->asPropertyKey(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    protoLookup.protoId = o->internalClass()->protoId;
    resolveProtoGetter(name, o->d());

    switch (call) {
    case Call::GetterProto:
        call = Call::GlobalGetterProto;
        break;
    case Call::GetterProtoAccessor:
        call = Call::GlobalGetterProtoAccessor;
        break;
    default:
        call = Call::GlobalGetterGeneric;
        Scope scope(engine);
        ScopedString n(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
        return engine->throwReferenceError(n);
    }

    return globalGetter(engine);
}

ReturnedValue Lookup::getterGeneric(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    if (const Object *o = object.as<Object>())
        return lookup->resolveGetter(engine, o);
    return lookup->resolvePrimitiveGetter(engine, object);
}

static inline void setupObjectLookupTwoClasses(Lookup *lookup, const Lookup &first, const Lookup &second)
{
    Heap::InternalClass *ic1 = first.objectLookup.ic;
    const uint offset1 = first.objectLookup.offset;
    Heap::InternalClass *ic2 = second.objectLookup.ic;
    const uint offset2 = second.objectLookup.offset;
    auto engine = ic1->engine;

    lookup->objectLookupTwoClasses.ic.set(engine, ic1);
    lookup->objectLookupTwoClasses.ic2.set(engine, ic2);
    lookup->objectLookupTwoClasses.offset = offset1;
    lookup->objectLookupTwoClasses.offset2 = offset2;
}

static inline void setupProtoLookupTwoClasses(Lookup *lookup, const Lookup &first, const Lookup &second)
{
    const quintptr protoId1 = first.protoLookup.protoId;
    const Value *data1 = first.protoLookup.data;
    const quintptr protoId2 = second.protoLookup.protoId;
    const Value *data2 = second.protoLookup.data;

    lookup->protoLookupTwoClasses.protoId = protoId1;
    lookup->protoLookupTwoClasses.protoId2 = protoId2;
    lookup->protoLookupTwoClasses.data = data1;
    lookup->protoLookupTwoClasses.data2 = data2;
}

ReturnedValue Lookup::getterTwoClasses(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    if (const Object *o = object.as<Object>()) {

        // Do the resolution on a second lookup, then merge.
        Lookup second;
        memset(&second, 0, sizeof(Lookup));
        second.nameIndex = lookup->nameIndex;
        second.forCall = lookup->forCall;
        second.call = Call::GetterGeneric;
        const ReturnedValue result = second.resolveGetter(engine, o);

        switch (lookup->call) {
        case Call::Getter0Inline: {
            switch (second.call) {
            case Call::Getter0Inline:
                setupObjectLookupTwoClasses(lookup, *lookup, second);
                lookup->call = Call::Getter0InlineGetter0Inline;
                return result;
            case Call::Getter0MemberData:
                setupObjectLookupTwoClasses(lookup, *lookup, second);
                lookup->call = Call::Getter0InlineGetter0MemberData;
                return result;
            default:
                break;
            }
            break;
        }
        case Call::Getter0MemberData:
            switch (second.call) {
            case Call::Getter0Inline:
                setupObjectLookupTwoClasses(lookup, *lookup, second);
                lookup->call = Call::Getter0InlineGetter0MemberData;
                return result;
            case Call::Getter0MemberData:
                setupObjectLookupTwoClasses(lookup, *lookup, second);
                lookup->call = Call::Getter0MemberDataGetter0MemberData;
                return result;
            default:
                break;
            }
            break;
        case Call::GetterProto:
            switch (second.call) {
            case Call::GetterProto:
                setupProtoLookupTwoClasses(lookup, *lookup, second);
                lookup->call =  Call::GetterProtoTwoClasses;
                return result;
            default:
                break;
            }
            break;
        case Call::GetterProtoAccessor:
            switch (second.call) {
            case Call::GetterProtoAccessor:
                setupProtoLookupTwoClasses(lookup, *lookup, second);
                lookup->call = Call::GetterProtoAccessorTwoClasses;
                return result;
            default:
                break;
            }
            break;
        default:
            break;
        }

        // If any of the above options were true, the propertyCache was inactive.
        second.releasePropertyCache();
    }

    lookup->call = Call::GetterQObjectPropertyFallback;
    return getterFallback(lookup, engine, object);
}

ReturnedValue Lookup::getterFallback(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    QV4::Scope scope(engine);
    QV4::ScopedObject o(scope, object.toObject(scope.engine));
    if (!o)
        return Encode::undefined();
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[lookup->nameIndex]);
    return o->get(name);
}

ReturnedValue Lookup::getter0MemberData(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (lookup->objectLookup.ic == o->internalClass)
            return o->memberData->values.data()[lookup->objectLookup.offset].asReturnedValue();
    }
    return getterTwoClasses(lookup, engine, object);
}

ReturnedValue Lookup::getter0Inline(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (lookup->objectLookup.ic == o->internalClass)
            return o->inlinePropertyDataWithOffset(lookup->objectLookup.offset)->asReturnedValue();
    }
    return getterTwoClasses(lookup, engine, object);
}

ReturnedValue Lookup::getterProto(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (lookup->protoLookup.protoId == o->internalClass->protoId)
            return lookup->protoLookup.data->asReturnedValue();
    }
    return getterTwoClasses(lookup, engine, object);
}

ReturnedValue Lookup::getter0Inlinegetter0Inline(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (lookup->objectLookupTwoClasses.ic == o->internalClass)
            return o->inlinePropertyDataWithOffset(lookup->objectLookupTwoClasses.offset)->asReturnedValue();
        if (lookup->objectLookupTwoClasses.ic2 == o->internalClass)
            return o->inlinePropertyDataWithOffset(lookup->objectLookupTwoClasses.offset2)->asReturnedValue();
    }
    lookup->call = Call::GetterQObjectPropertyFallback;
    return getterFallback(lookup, engine, object);
}

ReturnedValue Lookup::getter0Inlinegetter0MemberData(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (lookup->objectLookupTwoClasses.ic == o->internalClass)
            return o->inlinePropertyDataWithOffset(lookup->objectLookupTwoClasses.offset)->asReturnedValue();
        if (lookup->objectLookupTwoClasses.ic2 == o->internalClass)
            return o->memberData->values.data()[lookup->objectLookupTwoClasses.offset2].asReturnedValue();
    }
    lookup->call = Call::GetterQObjectPropertyFallback;
    return getterFallback(lookup, engine, object);
}

ReturnedValue Lookup::getter0MemberDatagetter0MemberData(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (lookup->objectLookupTwoClasses.ic == o->internalClass)
            return o->memberData->values.data()[lookup->objectLookupTwoClasses.offset].asReturnedValue();
        if (lookup->objectLookupTwoClasses.ic2 == o->internalClass)
            return o->memberData->values.data()[lookup->objectLookupTwoClasses.offset2].asReturnedValue();
    }
    lookup->call = Call::GetterQObjectPropertyFallback;
    return getterFallback(lookup, engine, object);
}

ReturnedValue Lookup::getterProtoTwoClasses(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (lookup->protoLookupTwoClasses.protoId == o->internalClass->protoId)
            return lookup->protoLookupTwoClasses.data->asReturnedValue();
        if (lookup->protoLookupTwoClasses.protoId2 == o->internalClass->protoId)
            return lookup->protoLookupTwoClasses.data2->asReturnedValue();
        return getterFallback(lookup, engine, object);
    }
    lookup->call = Call::GetterQObjectPropertyFallback;
    return getterFallback(lookup, engine, object);
}

ReturnedValue Lookup::getterAccessor(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (lookup->objectLookup.ic == o->internalClass) {
            const Value *getter = o->propertyData(lookup->objectLookup.offset);
            if (!getter->isFunctionObject()) // ### catch at resolve time
                return Encode::undefined();

            return checkedResult(engine, static_cast<const FunctionObject *>(getter)->call(
                                     &object, nullptr, 0));
        }
    }
    lookup->call = Call::GetterQObjectPropertyFallback;
    return getterFallback(lookup, engine, object);
}

ReturnedValue Lookup::getterProtoAccessor(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o && lookup->protoLookup.protoId == o->internalClass->protoId) {
        const Value *getter = lookup->protoLookup.data;
        if (!getter->isFunctionObject()) // ### catch at resolve time
            return Encode::undefined();

        return checkedResult(engine, static_cast<const FunctionObject *>(getter)->call(
                                 &object, nullptr, 0));
    }
    return getterTwoClasses(lookup, engine, object);
}

ReturnedValue Lookup::getterProtoAccessorTwoClasses(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        const Value *getter = nullptr;
        if (lookup->protoLookupTwoClasses.protoId == o->internalClass->protoId)
            getter = lookup->protoLookupTwoClasses.data;
        else if (lookup->protoLookupTwoClasses.protoId2 == o->internalClass->protoId)
            getter = lookup->protoLookupTwoClasses.data2;
        if (getter) {
            if (!getter->isFunctionObject()) // ### catch at resolve time
                return Encode::undefined();

            return checkedResult(engine, static_cast<const FunctionObject *>(getter)->call(
                                     &object, nullptr, 0));
        }
    }
    lookup->call = Call::GetterQObjectPropertyFallback;
    return getterFallback(lookup, engine, object);
}

ReturnedValue Lookup::getterIndexed(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    Object *o = object.objectValue();
    if (o) {
        Heap::Object *ho = o->d();
        if (ho->arrayData && ho->arrayData->type == Heap::ArrayData::Simple) {
            Heap::SimpleArrayData *s = ho->arrayData.cast<Heap::SimpleArrayData>();
            if (lookup->indexedLookup.index < s->values.size)
                if (!s->data(lookup->indexedLookup.index).isEmpty())
                    return s->data(lookup->indexedLookup.index).asReturnedValue();
        }
        return o->get(lookup->indexedLookup.index);
    }
    lookup->call = Call::GetterQObjectPropertyFallback;
    return getterFallback(lookup, engine, object);
}

ReturnedValue Lookup::getterQObject(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    const auto revertLookup = [lookup, engine, &object]() {
        lookup->qobjectLookup.propertyCache->release();
        lookup->qobjectLookup.propertyCache = nullptr;
        lookup->call = Call::GetterGeneric;
        return Lookup::getterGeneric(lookup, engine, object);
    };

    const QObjectWrapper::Flags flags = lookup->forCall
            ? QObjectWrapper::AllowOverride
            : (QObjectWrapper::AllowOverride | QObjectWrapper::AttachMethods);

    return QObjectWrapper::lookupPropertyGetterImpl(lookup, engine, object, flags, revertLookup);
}

ReturnedValue Lookup::getterQObjectMethod(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    const auto revertLookup = [lookup, engine, &object]() {
        lookup->qobjectMethodLookup.propertyCache->release();
        lookup->qobjectMethodLookup.propertyCache = nullptr;
        lookup->call = Call::GetterGeneric;
        return Lookup::getterGeneric(lookup, engine, object);
    };

    const QObjectWrapper::Flags flags = lookup->forCall
            ? QObjectWrapper::AllowOverride
            : (QObjectWrapper::AllowOverride | QObjectWrapper::AttachMethods);

    return QObjectWrapper::lookupMethodGetterImpl(lookup, engine, object, flags, revertLookup);
}

ReturnedValue Lookup::getterFallbackMethod(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    const auto revertLookup = [lookup, engine, &object]() {
        lookup->call = Call::GetterGeneric;
        return Lookup::getterGeneric(lookup, engine, object);
    };

    const Object *o = object.as<Object>();
    if (!o || o->internalClass() != lookup->qobjectMethodLookup.ic)
        return revertLookup();

    const QObjectWrapper *This = static_cast<const QObjectWrapper *>(o);
    QObject *qobj = This->d()->object();
    if (QQmlData::wasDeleted(qobj))
        return QV4::Encode::undefined();

    Scope scope(engine);
    ScopedString name(
            scope,
            engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[lookup->nameIndex]);

    QV4::ScopedValue result(
            scope, QObjectWrapper::getMethodFallback(
                           engine, This->d(), qobj, name,
                           lookup->forCall ? QObjectWrapper::NoFlag : QObjectWrapper::AttachMethods));

    // In the general case we cannot rely on the method to exist or stay the same across calls.
    // However, the AOT compiler can prove it in certain cases. For these, we store the method.
    if (QObjectMethod *method = result->as<QV4::QObjectMethod>())
        lookup->qobjectMethodLookup.method.set(engine, method->d());

    return result->asReturnedValue();
}

ReturnedValue Lookup::getterValueType(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    const auto revertLookup = [lookup, engine, &object]() {
        lookup->qgadgetLookup.metaObject = quintptr(0);
        lookup->call = Call::GetterGeneric;
        return Lookup::getterGeneric(lookup, engine, object);
    };

    // we can safely cast to a QV4::Object here. If object is something else,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (!o || o->internalClass != lookup->qgadgetLookup.ic)
        return revertLookup();

    Heap::QQmlValueTypeWrapper *valueTypeWrapper =
            const_cast<Heap::QQmlValueTypeWrapper*>(static_cast<const Heap::QQmlValueTypeWrapper *>(o));
    if (valueTypeWrapper->metaObject() != reinterpret_cast<const QMetaObject *>(lookup->qgadgetLookup.metaObject - 1))
        return revertLookup();

    if (valueTypeWrapper->isReference() && !valueTypeWrapper->readReference())
        return Encode::undefined();

    return QQmlValueTypeWrapper::getGadgetProperty(
            engine, valueTypeWrapper, QMetaType(lookup->qgadgetLookup.metaType),
            lookup->qgadgetLookup.coreIndex, lookup->qgadgetLookup.isFunction,
            lookup->qgadgetLookup.isEnum);
}

ReturnedValue Lookup::primitiveGetterProto(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    if (object.type() == lookup->primitiveLookup.type && !object.isObject()) {
        Heap::Object *o = lookup->primitiveLookup.proto;
        if (lookup->primitiveLookup.protoId == o->internalClass->protoId)
            return lookup->primitiveLookup.data->asReturnedValue();
    }
    lookup->call = Call::GetterGeneric;
    return getterGeneric(lookup, engine, object);
}

ReturnedValue Lookup::primitiveGetterAccessor(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    if (object.type() == lookup->primitiveLookup.type && !object.isObject()) {
        Heap::Object *o = lookup->primitiveLookup.proto;
        if (lookup->primitiveLookup.protoId == o->internalClass->protoId) {
            const Value *getter = lookup->primitiveLookup.data;
            if (!getter->isFunctionObject()) // ### catch at resolve time
                return Encode::undefined();

            return checkedResult(engine, static_cast<const FunctionObject *>(getter)->call(
                                     &object, nullptr, 0));
        }
    }
    lookup->call = Call::GetterGeneric;
    return getterGeneric(lookup, engine, object);
}

ReturnedValue Lookup::stringLengthGetter(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    if (const String *s = object.as<String>())
        return Encode(s->d()->length());

    lookup->call = Call::GetterGeneric;
    return getterGeneric(lookup, engine, object);
}

ReturnedValue Lookup::globalGetterGeneric(Lookup *lookup, ExecutionEngine *engine)
{
    return lookup->resolveGlobalGetter(engine);
}

ReturnedValue Lookup::globalGetterProto(Lookup *lookup, ExecutionEngine *engine)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    Heap::Object *o = engine->globalObject->d();
    if (lookup->protoLookup.protoId == o->internalClass->protoId)
        return lookup->protoLookup.data->asReturnedValue();
    lookup->call = Call::GlobalGetterGeneric;
    return globalGetterGeneric(lookup, engine);
}

ReturnedValue Lookup::globalGetterProtoAccessor(Lookup *lookup, ExecutionEngine *engine)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    Heap::Object *o = engine->globalObject->d();
    if (lookup->protoLookup.protoId == o->internalClass->protoId) {
        const Value *getter = lookup->protoLookup.data;
        if (!getter->isFunctionObject()) // ### catch at resolve time
            return Encode::undefined();

        return checkedResult(engine, static_cast<const FunctionObject *>(getter)->call(
                                  engine->globalObject, nullptr, 0));
    }

    lookup->call = Call::GlobalGetterGeneric;
    return globalGetterGeneric(lookup, engine);
}

bool Lookup::resolveSetter(ExecutionEngine *engine, Object *object, const Value &value)
{
    return object->resolveLookupSetter(engine, this, value);
}

bool Lookup::setterGeneric(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value)
{
    if (object.isObject())
        return lookup->resolveSetter(engine, static_cast<Object *>(&object), value);

    if (engine->currentStackFrame->v4Function->isStrict())
        return false;

    Scope scope(engine);
    ScopedObject o(scope, RuntimeHelpers::convertToObject(scope.engine, object));
    if (!o) // type error
        return false;
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[lookup->nameIndex]);
    return o->put(name, value);
}

bool Lookup::setterTwoClasses(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value)
{
    // A precondition of this method is that lookup->objectLookup is the active variant of the union.
    Q_ASSERT(lookup->call == Call::Setter0MemberData || lookup->call == Call::Setter0Inline);

    if (object.isObject()) {

        // As lookup->objectLookup is active, we can stash some members here, before resolving.
        Heap::InternalClass *ic = lookup->objectLookup.ic;
        const uint index = lookup->objectLookup.index;

        if (!lookup->resolveSetter(engine, static_cast<Object *>(&object), value)) {
            lookup->call = Call::SetterQObjectPropertyFallback;
            return false;
        }

        if (lookup->call == Call::Setter0MemberData || lookup->call == Call::Setter0Inline) {
            auto engine = ic->engine;
            lookup->objectLookupTwoClasses.ic.set(engine, ic);
            lookup->objectLookupTwoClasses.ic2.set(engine, ic);
            lookup->objectLookupTwoClasses.offset = index;
            lookup->objectLookupTwoClasses.offset2 = index;
            lookup->call = Call::Setter0Setter0;
            return true;
        }

        lookup->releasePropertyCache();
    }

    lookup->call = Call::SetterQObjectPropertyFallback;
    return setterFallback(lookup, engine, object, value);
}

bool Lookup::setterFallback(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value)
{
    QV4::Scope scope(engine);
    QV4::ScopedObject o(scope, object.toObject(scope.engine));
    if (!o)
        return false;

    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[lookup->nameIndex]);
    return o->put(name, value);
}

bool Lookup::setter0MemberData(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value)
{
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o && o->internalClass == lookup->objectLookup.ic) {
        o->memberData->values.set(engine, lookup->objectLookup.offset, value);
        return true;
    }

    return setterTwoClasses(lookup, engine, object, value);
}

bool Lookup::setter0Inline(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value)
{
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o && o->internalClass == lookup->objectLookup.ic) {
        o->setInlinePropertyWithOffset(engine, lookup->objectLookup.offset, value);
        return true;
    }

    return setterTwoClasses(lookup, engine, object, value);
}

bool Lookup::setter0setter0(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value)
{
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (o->internalClass == lookup->objectLookupTwoClasses.ic) {
            o->setProperty(engine, lookup->objectLookupTwoClasses.offset, value);
            return true;
        }
        if (o->internalClass == lookup->objectLookupTwoClasses.ic2) {
            o->setProperty(engine, lookup->objectLookupTwoClasses.offset2, value);
            return true;
        }
    }

    lookup->call = Call::SetterQObjectPropertyFallback;
    return setterFallback(lookup, engine, object, value);
}

bool Lookup::setterInsert(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &value)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    Object *o = static_cast<Object *>(object.managed());
    if (o && o->internalClass()->protoId == lookup->insertionLookup.protoId) {
        o->setInternalClass(lookup->insertionLookup.newClass);
        o->d()->setProperty(engine, lookup->insertionLookup.offset, value);
        return true;
    }

    lookup->call = Call::SetterQObjectPropertyFallback;
    return setterFallback(lookup, engine, object, value);
}

bool Lookup::setterQObject(Lookup *lookup, ExecutionEngine *engine, Value &object, const Value &v)
{
    // This setter just marks the presence of a qobjectlookup. It only does anything with it when
    // running AOT-compiled code, though.
    return setterFallback(lookup, engine, object, v);
}

bool Lookup::arrayLengthSetter(Lookup *, ExecutionEngine *engine, Value &object, const Value &value)
{
    Q_ASSERT(object.isObject() && static_cast<Object &>(object).isArrayObject());
    bool ok;
    uint len = value.asArrayLength(&ok);
    if (!ok) {
        engine->throwRangeError(value);
        return false;
    }
    ok = static_cast<Object &>(object).setArrayLength(len);
    if (!ok)
        return false;
    return true;
}

QT_END_NAMESPACE
