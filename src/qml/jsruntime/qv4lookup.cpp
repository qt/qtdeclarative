// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qv4functionobject_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4lookup_p.h>
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
                getter = getterProto;
            } else {
                getter = getterProtoAccessor;
            }
            return;
        }
        proto = proto->prototype();
    }
    // ### put in a getterNotFound!
    getter = getterFallback;
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
        primitiveLookup.proto = engine->booleanPrototype()->d();
        break;
    case Value::Managed_Type: {
        // ### Should move this over to the Object path, as strings also have an internalClass
        Q_ASSERT(object.isStringOrSymbol());
        primitiveLookup.proto = static_cast<const Managed &>(object).internalClass()->prototype;
        Q_ASSERT(primitiveLookup.proto);
        Scope scope(engine);
        ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
        if (object.isString() && name->equals(engine->id_length())) {
            // special case, as the property is on the object itself
            getter = stringLengthGetter;
            return stringLengthGetter(this, engine, object);
        }
        break;
    }
    case Value::Integer_Type:
    default: // Number
        primitiveLookup.proto = engine->numberPrototype()->d();
    }

    PropertyKey name = engine->identifierTable->asPropertyKey(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    protoLookup.protoId = primitiveLookup.proto->internalClass->protoId;
    resolveProtoGetter(name, primitiveLookup.proto);

    if (getter == getterProto)
        getter = primitiveGetterProto;
    else if (getter == getterProtoAccessor)
        getter = primitiveGetterAccessor;
    return getter(this, engine, object);
}

ReturnedValue Lookup::resolveGlobalGetter(ExecutionEngine *engine)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    Object *o = engine->globalObject;
    PropertyKey name = engine->identifierTable->asPropertyKey(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
    protoLookup.protoId = o->internalClass()->protoId;
    resolveProtoGetter(name, o->d());

    if (getter == getterProto)
        globalGetter = globalGetterProto;
    else if (getter == getterProtoAccessor)
        globalGetter = globalGetterProtoAccessor;
    else {
        globalGetter = globalGetterGeneric;
        Scope scope(engine);
        ScopedString n(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[nameIndex]);
        return engine->throwReferenceError(n);
    }
    return globalGetter(this, engine);
}

ReturnedValue Lookup::getterGeneric(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (const Object *o = object.as<Object>())
        return l->resolveGetter(engine, o);
    return l->resolvePrimitiveGetter(engine, object);
}

static inline void setupObjectLookupTwoClasses(Lookup *l, const Lookup &first, const Lookup &second)
{
    Heap::InternalClass *ic1 = first.objectLookup.ic;
    const uint offset1 = first.objectLookup.offset;
    Heap::InternalClass *ic2 = second.objectLookup.ic;
    const uint offset2 = second.objectLookup.offset;

    l->objectLookupTwoClasses.ic = ic1;
    l->objectLookupTwoClasses.ic2 = ic2;
    l->objectLookupTwoClasses.offset = offset1;
    l->objectLookupTwoClasses.offset2 = offset2;
}

static inline void setupProtoLookupTwoClasses(Lookup *l, const Lookup &first, const Lookup &second)
{
    const quintptr protoId1 = first.protoLookup.protoId;
    const Value *data1 = first.protoLookup.data;
    const quintptr protoId2 = second.protoLookup.protoId;
    const Value *data2 = second.protoLookup.data;

    l->protoLookupTwoClasses.protoId = protoId1;
    l->protoLookupTwoClasses.protoId2 = protoId2;
    l->protoLookupTwoClasses.data = data1;
    l->protoLookupTwoClasses.data2 = data2;
}

ReturnedValue Lookup::getterTwoClasses(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (const Object *o = object.as<Object>()) {

        // Do the resolution on a second lookup, then merge.
        Lookup second;
        memset(&second, 0, sizeof(Lookup));
        second.nameIndex = l->nameIndex;
        second.forCall = l->forCall;
        second.getter = getterGeneric;
        const ReturnedValue result = second.resolveGetter(engine, o);

        if (l->getter == getter0Inline
                && (second.getter == getter0Inline || second.getter == getter0MemberData)) {
            setupObjectLookupTwoClasses(l, *l, second);
            l->getter = (second.getter == getter0Inline)
                    ? getter0Inlinegetter0Inline
                    : getter0Inlinegetter0MemberData;
            return result;
        }

        if (l->getter == getter0MemberData
                && (second.getter == getter0Inline || second.getter == getter0MemberData)) {
            setupObjectLookupTwoClasses(l, second, *l);
            l->getter = (second.getter == getter0Inline)
                    ? getter0Inlinegetter0MemberData
                    : getter0MemberDatagetter0MemberData;
            return result;
        }


        if (l->getter == getterProto && second.getter == getterProto) {
            setupProtoLookupTwoClasses(l, *l, second);
            l->getter = getterProtoTwoClasses;
            return result;
        }

        if (l->getter == getterProtoAccessor && second.getter == getterProtoAccessor) {
            setupProtoLookupTwoClasses(l, *l, second);
            l->getter = getterProtoAccessorTwoClasses;
            return result;
        }

        // If any of the above options were true, the propertyCache was inactive.
        second.releasePropertyCache();
    }

    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterFallback(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    QV4::Scope scope(engine);
    QV4::ScopedObject o(scope, object.toObject(scope.engine));
    if (!o)
        return Encode::undefined();
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[l->nameIndex]);
    return o->get(name);
}

ReturnedValue Lookup::getterFallbackAsVariant(
        Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (&Lookup::getterFallback == &Lookup::getterFallbackAsVariant) {
        // Certain compilers, e.g. MSVC, will "helpfully" deduplicate methods that are completely
        // equal. As a result, the pointers are the same, which wreaks havoc on the logic that
        // decides how to retrieve the property.
        qFatal("Your C++ compiler is broken.");
    }

    // This getter just marks the presence of a fallback lookup with variant conversion.
    // It only does anything with it when running AOT-compiled code.
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getter0MemberData(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookup.ic == o->internalClass)
            return o->memberData->values.data()[l->objectLookup.offset].asReturnedValue();
    }
    return getterTwoClasses(l, engine, object);
}

ReturnedValue Lookup::getter0Inline(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookup.ic == o->internalClass)
            return o->inlinePropertyDataWithOffset(l->objectLookup.offset)->asReturnedValue();
    }
    return getterTwoClasses(l, engine, object);
}

ReturnedValue Lookup::getterProto(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->protoLookup.protoId == o->internalClass->protoId)
            return l->protoLookup.data->asReturnedValue();
    }
    return getterTwoClasses(l, engine, object);
}

ReturnedValue Lookup::getter0Inlinegetter0Inline(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookupTwoClasses.ic == o->internalClass)
            return o->inlinePropertyDataWithOffset(l->objectLookupTwoClasses.offset)->asReturnedValue();
        if (l->objectLookupTwoClasses.ic2 == o->internalClass)
            return o->inlinePropertyDataWithOffset(l->objectLookupTwoClasses.offset2)->asReturnedValue();
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getter0Inlinegetter0MemberData(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookupTwoClasses.ic == o->internalClass)
            return o->inlinePropertyDataWithOffset(l->objectLookupTwoClasses.offset)->asReturnedValue();
        if (l->objectLookupTwoClasses.ic2 == o->internalClass)
            return o->memberData->values.data()[l->objectLookupTwoClasses.offset2].asReturnedValue();
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getter0MemberDatagetter0MemberData(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookupTwoClasses.ic == o->internalClass)
            return o->memberData->values.data()[l->objectLookupTwoClasses.offset].asReturnedValue();
        if (l->objectLookupTwoClasses.ic2 == o->internalClass)
            return o->memberData->values.data()[l->objectLookupTwoClasses.offset2].asReturnedValue();
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterProtoTwoClasses(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->protoLookupTwoClasses.protoId == o->internalClass->protoId)
            return l->protoLookupTwoClasses.data->asReturnedValue();
        if (l->protoLookupTwoClasses.protoId2 == o->internalClass->protoId)
            return l->protoLookupTwoClasses.data2->asReturnedValue();
        return getterFallback(l, engine, object);
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterAccessor(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (l->objectLookup.ic == o->internalClass) {
            const Value *getter = o->propertyData(l->objectLookup.offset);
            if (!getter->isFunctionObject()) // ### catch at resolve time
                return Encode::undefined();

            return checkedResult(engine, static_cast<const FunctionObject *>(getter)->call(
                                     &object, nullptr, 0));
        }
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterProtoAccessor(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o && l->protoLookup.protoId == o->internalClass->protoId) {
        const Value *getter = l->protoLookup.data;
        if (!getter->isFunctionObject()) // ### catch at resolve time
            return Encode::undefined();

        return checkedResult(engine, static_cast<const FunctionObject *>(getter)->call(
                                 &object, nullptr, 0));
    }
    return getterTwoClasses(l, engine, object);
}

ReturnedValue Lookup::getterProtoAccessorTwoClasses(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    // we can safely cast to a QV4::Object here. If object is actually a string,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        const Value *getter = nullptr;
        if (l->protoLookupTwoClasses.protoId == o->internalClass->protoId)
            getter = l->protoLookupTwoClasses.data;
        else if (l->protoLookupTwoClasses.protoId2 == o->internalClass->protoId)
            getter = l->protoLookupTwoClasses.data2;
        if (getter) {
            if (!getter->isFunctionObject()) // ### catch at resolve time
                return Encode::undefined();

            return checkedResult(engine, static_cast<const FunctionObject *>(getter)->call(
                                     &object, nullptr, 0));
        }
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterIndexed(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    Object *o = object.objectValue();
    if (o) {
        Heap::Object *ho = o->d();
        if (ho->arrayData && ho->arrayData->type == Heap::ArrayData::Simple) {
            Heap::SimpleArrayData *s = ho->arrayData.cast<Heap::SimpleArrayData>();
            if (l->indexedLookup.index < s->values.size)
                if (!s->data(l->indexedLookup.index).isEmpty())
                    return s->data(l->indexedLookup.index).asReturnedValue();
        }
        return o->get(l->indexedLookup.index);
    }
    l->getter = getterFallback;
    return getterFallback(l, engine, object);
}

ReturnedValue Lookup::getterQObject(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    const auto revertLookup = [lookup, engine, &object]() {
        lookup->qobjectLookup.propertyCache->release();
        lookup->qobjectLookup.propertyCache = nullptr;
        lookup->getter = Lookup::getterGeneric;
        return Lookup::getterGeneric(lookup, engine, object);
    };

    const QObjectWrapper::Flags flags = lookup->forCall
            ? QObjectWrapper::AllowOverride
            : (QObjectWrapper::AllowOverride | QObjectWrapper::AttachMethods);

    return QObjectWrapper::lookupPropertyGetterImpl(lookup, engine, object, flags, revertLookup);
}

ReturnedValue Lookup::getterQObjectAsVariant(
        Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    if (&Lookup::getterQObject == &Lookup::getterQObjectAsVariant) {
        // Certain compilers, e.g. MSVC, will "helpfully" deduplicate methods that are completely
        // equal. As a result, the pointers are the same, which wreaks havoc on the logic that
        // decides how to retrieve the property.
        qFatal("Your C++ compiler is broken.");
    }

    // This getter marks the presence of a qobjectlookup with variant conversion.
    // It only does anything with it when running AOT-compiled code.
    return getterQObject(lookup, engine, object);
}

ReturnedValue Lookup::getterQObjectMethod(Lookup *lookup, ExecutionEngine *engine, const Value &object)
{
    const auto revertLookup = [lookup, engine, &object]() {
        lookup->qobjectMethodLookup.propertyCache->release();
        lookup->qobjectMethodLookup.propertyCache = nullptr;
        lookup->getter = Lookup::getterGeneric;
        return Lookup::getterGeneric(lookup, engine, object);
    };

    const QObjectWrapper::Flags flags = lookup->forCall
            ? QObjectWrapper::AllowOverride
            : (QObjectWrapper::AllowOverride | QObjectWrapper::AttachMethods);

    return QObjectWrapper::lookupMethodGetterImpl(lookup, engine, object, flags, revertLookup);
}

ReturnedValue Lookup::primitiveGetterProto(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    if (object.type() == l->primitiveLookup.type && !object.isObject()) {
        Heap::Object *o = l->primitiveLookup.proto;
        if (l->primitiveLookup.protoId == o->internalClass->protoId)
            return l->primitiveLookup.data->asReturnedValue();
    }
    l->getter = getterGeneric;
    return getterGeneric(l, engine, object);
}

ReturnedValue Lookup::primitiveGetterAccessor(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    if (object.type() == l->primitiveLookup.type && !object.isObject()) {
        Heap::Object *o = l->primitiveLookup.proto;
        if (l->primitiveLookup.protoId == o->internalClass->protoId) {
            const Value *getter = l->primitiveLookup.data;
            if (!getter->isFunctionObject()) // ### catch at resolve time
                return Encode::undefined();

            return checkedResult(engine, static_cast<const FunctionObject *>(getter)->call(
                                     &object, nullptr, 0));
        }
    }
    l->getter = getterGeneric;
    return getterGeneric(l, engine, object);
}

ReturnedValue Lookup::stringLengthGetter(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    if (const String *s = object.as<String>())
        return Encode(s->d()->length());

    l->getter = getterGeneric;
    return getterGeneric(l, engine, object);
}

ReturnedValue Lookup::globalGetterGeneric(Lookup *l, ExecutionEngine *engine)
{
    return l->resolveGlobalGetter(engine);
}

ReturnedValue Lookup::globalGetterProto(Lookup *l, ExecutionEngine *engine)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    Heap::Object *o = engine->globalObject->d();
    if (l->protoLookup.protoId == o->internalClass->protoId)
        return l->protoLookup.data->asReturnedValue();
    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, engine);
}

ReturnedValue Lookup::globalGetterProtoAccessor(Lookup *l, ExecutionEngine *engine)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    Heap::Object *o = engine->globalObject->d();
    if (l->protoLookup.protoId == o->internalClass->protoId) {
        const Value *getter = l->protoLookup.data;
        if (!getter->isFunctionObject()) // ### catch at resolve time
            return Encode::undefined();

        return checkedResult(engine, static_cast<const FunctionObject *>(getter)->call(
                                  engine->globalObject, nullptr, 0));
    }
    l->globalGetter = globalGetterGeneric;
    return globalGetterGeneric(l, engine);
}

bool Lookup::resolveSetter(ExecutionEngine *engine, Object *object, const Value &value)
{
    return object->resolveLookupSetter(engine, this, value);
}

bool Lookup::setterGeneric(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    if (object.isObject())
        return l->resolveSetter(engine, static_cast<Object *>(&object), value);

    if (engine->currentStackFrame->v4Function->isStrict())
        return false;

    Scope scope(engine);
    ScopedObject o(scope, RuntimeHelpers::convertToObject(scope.engine, object));
    if (!o) // type error
        return false;
    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[l->nameIndex]);
    return o->put(name, value);
}

bool Lookup::setterTwoClasses(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    // A precondition of this method is that l->objectLookup is the active variant of the union.
    Q_ASSERT(l->setter == setter0MemberData || l->setter == setter0Inline);

    if (object.isObject()) {

        // As l->objectLookup is active, we can stash some members here, before resolving.
        Heap::InternalClass *ic = l->objectLookup.ic;
        const uint index = l->objectLookup.index;

        if (!l->resolveSetter(engine, static_cast<Object *>(&object), value)) {
            l->setter = setterFallback;
            return false;
        }

        if (l->setter == Lookup::setter0MemberData || l->setter == Lookup::setter0Inline) {
            l->objectLookupTwoClasses.ic = ic;
            l->objectLookupTwoClasses.ic2 = ic;
            l->objectLookupTwoClasses.offset = index;
            l->objectLookupTwoClasses.offset2 = index;
            l->setter = setter0setter0;
            return true;
        }

        l->releasePropertyCache();
    }

    l->setter = setterFallback;
    return setterFallback(l, engine, object, value);
}

bool Lookup::setterFallback(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    QV4::Scope scope(engine);
    QV4::ScopedObject o(scope, object.toObject(scope.engine));
    if (!o)
        return false;

    ScopedString name(scope, engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[l->nameIndex]);
    return o->put(name, value);
}

bool Lookup::setterFallbackAsVariant(
        Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    if (&Lookup::setterFallback == &Lookup::setterFallbackAsVariant) {
        // Certain compilers, e.g. MSVC, will "helpfully" deduplicate methods that are completely
        // equal. As a result, the pointers are the same, which wreaks havoc on the logic that
        // decides how to retrieve the property.
        qFatal("Your C++ compiler is broken.");
    }

    // This setter just marks the presence of a fallback lookup with QVariant conversion.
    // It only does anything with it when running AOT-compiled code.
    return setterFallback(l, engine, object, value);
}

bool Lookup::setter0MemberData(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o && o->internalClass == l->objectLookup.ic) {
        o->memberData->values.set(engine, l->objectLookup.offset, value);
        return true;
    }

    return setterTwoClasses(l, engine, object, value);
}

bool Lookup::setter0Inline(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o && o->internalClass == l->objectLookup.ic) {
        o->setInlinePropertyWithOffset(engine, l->objectLookup.offset, value);
        return true;
    }

    return setterTwoClasses(l, engine, object, value);
}

bool Lookup::setter0setter0(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (o) {
        if (o->internalClass == l->objectLookupTwoClasses.ic) {
            o->setProperty(engine, l->objectLookupTwoClasses.offset, value);
            return true;
        }
        if (o->internalClass == l->objectLookupTwoClasses.ic2) {
            o->setProperty(engine, l->objectLookupTwoClasses.offset2, value);
            return true;
        }
    }

    l->setter = setterFallback;
    return setterFallback(l, engine, object, value);
}

bool Lookup::setterInsert(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value)
{
    // Otherwise we cannot trust the protoIds
    Q_ASSERT(engine->isInitialized);

    Object *o = static_cast<Object *>(object.managed());
    if (o && o->internalClass()->protoId == l->insertionLookup.protoId) {
        o->setInternalClass(l->insertionLookup.newClass);
        o->d()->setProperty(engine, l->insertionLookup.offset, value);
        return true;
    }

    l->setter = setterFallback;
    return setterFallback(l, engine, object, value);
}

bool Lookup::setterQObject(Lookup *l, ExecutionEngine *engine, Value &object, const Value &v)
{
    // This setter just marks the presence of a qobjectlookup. It only does anything with it when
    // running AOT-compiled code, though.
    return setterFallback(l, engine, object, v);
}

bool Lookup::setterQObjectAsVariant(
        Lookup *l, ExecutionEngine *engine, Value &object, const Value &v)
{
    if (&Lookup::setterQObject == &Lookup::setterQObjectAsVariant) {
        // Certain compilers, e.g. MSVC, will "helpfully" deduplicate methods that are completely
        // equal. As a result, the pointers are the same, which wreaks havoc on the logic that
        // decides how to retrieve the property.
        qFatal("Your C++ compiler is broken.");
    }

    // This setter marks the presence of a qobjectlookup with QVariant conversion.
    // It only does anything with it when running AOT-compiled code.
    return setterQObject(l, engine, object, v);
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
