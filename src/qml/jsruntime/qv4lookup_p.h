// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QV4LOOKUP_H
#define QV4LOOKUP_H

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

#include "qv4engine_p.h"
#include "qv4object_p.h"
#include "qv4internalclass_p.h"
#include "qv4qmlcontext_p.h"
#include <private/qqmltypewrapper_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {
    struct QObjectMethod;
}

// Note: We cannot hide the copy ctor and assignment operator of this class because it needs to
//       be trivially copyable. But you should never ever copy it. There are refcounted members
//       in there.
struct Q_QML_PRIVATE_EXPORT Lookup {
    union {
        ReturnedValue (*getter)(Lookup *l, ExecutionEngine *engine, const Value &object);
        ReturnedValue (*globalGetter)(Lookup *l, ExecutionEngine *engine);
        ReturnedValue (*qmlContextPropertyGetter)(Lookup *l, ExecutionEngine *engine, Value *thisObject);
        bool (*setter)(Lookup *l, ExecutionEngine *engine, Value &object, const Value &v);
    };
    // NOTE: gc assumes the first two entries in the struct are pointers to heap objects or null
    //       or that the least significant bit is 1 (see the Lookup::markObjects function)
    union {
        struct {
            Heap::Base *h1;
            Heap::Base *h2;
            quintptr unused;
            quintptr unused2;
        } markDef;
        struct {
            Heap::InternalClass *ic;
            quintptr unused;
            uint index;
            uint offset;
        } objectLookup;
        struct {
            quintptr protoId;
            quintptr _unused;
            const Value *data;
        } protoLookup;
        struct {
            Heap::InternalClass *ic;
            Heap::InternalClass *ic2;
            uint offset;
            uint offset2;
        } objectLookupTwoClasses;
        struct {
            quintptr protoId;
            quintptr protoId2;
            const Value *data;
            const Value *data2;
        } protoLookupTwoClasses;
        struct {
            // Make sure the next two values are in sync with protoLookup
            quintptr protoId;
            Heap::Object *proto;
            const Value *data;
            quintptr type;
        } primitiveLookup;
        struct {
            Heap::InternalClass *newClass;
            quintptr protoId;
            uint offset;
            uint unused;
        } insertionLookup;
        struct {
            quintptr _unused;
            quintptr _unused2;
            uint index;
            uint unused;
        } indexedLookup;
        struct {
            Heap::InternalClass *ic;
            Heap::InternalClass *qmlTypeIc; // only used when lookup goes through QQmlTypeWrapper
            const QQmlPropertyCache *propertyCache;
            const QQmlPropertyData *propertyData;
        } qobjectLookup;
        struct {
            Heap::InternalClass *ic;
            Heap::QObjectMethod *method;
            const QQmlPropertyCache *propertyCache;
            const QQmlPropertyData *propertyData;
        } qobjectMethodLookup;
        struct {
            quintptr isConstant; // This is a bool, encoded as 0 or 1. Both values are ignored by gc
            quintptr metaObject; // a (const QMetaObject* & 1) or nullptr
            int coreIndex;
            int notifyIndex;
        } qobjectFallbackLookup;
        struct {
            Heap::InternalClass *ic;
            quintptr metaObject; // a (const QMetaObject* & 1) or nullptr
            const QtPrivate::QMetaTypeInterface *metaType; // cannot use QMetaType; class must be trivial
            quint16 coreIndex;
            bool isFunction;
            bool isEnum;
        } qgadgetLookup;
        struct {
            quintptr unused1;
            quintptr unused2;
            int scriptIndex;
        } qmlContextScriptLookup;
        struct {
            Heap::Base *singletonObject;
            quintptr unused2;
            QV4::ReturnedValue singletonValue;
        } qmlContextSingletonLookup;
        struct {
            quintptr unused1;
            quintptr unused2;
            int objectId;
        } qmlContextIdObjectLookup;
        struct {
            // Same as protoLookup, as used for global lookups
            quintptr reserved1;
            quintptr reserved2;
            quintptr reserved3;
            ReturnedValue (*getterTrampoline)(Lookup *l, ExecutionEngine *engine);
        } qmlContextGlobalLookup;
        struct {
            Heap::Base *qmlTypeWrapper;
            quintptr unused2;
        } qmlTypeLookup;
        struct {
            Heap::InternalClass *ic;
            quintptr unused;
            ReturnedValue encodedEnumValue;
            const QtPrivate::QMetaTypeInterface *metaType;
        } qmlEnumValueLookup;
        struct {
            Heap::InternalClass *ic;
            Heap::Object *qmlScopedEnumWrapper;
        } qmlScopedEnumWrapperLookup;
    };

    uint nameIndex: 28; // Same number of bits we store in the compilation unit for name indices
    uint forCall: 1;    // Whether we are looking up a value in order to call it right away
    uint reserved: 3;

    ReturnedValue resolveGetter(ExecutionEngine *engine, const Object *object);
    ReturnedValue resolvePrimitiveGetter(ExecutionEngine *engine, const Value &object);
    ReturnedValue resolveGlobalGetter(ExecutionEngine *engine);
    void resolveProtoGetter(PropertyKey name, const Heap::Object *proto);

    static ReturnedValue getterGeneric(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterTwoClasses(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterFallback(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterFallbackAsVariant(Lookup *l, ExecutionEngine *engine, const Value &object);

    static ReturnedValue getter0MemberData(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getter0Inline(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterProto(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getter0Inlinegetter0Inline(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getter0Inlinegetter0MemberData(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getter0MemberDatagetter0MemberData(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterProtoTwoClasses(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterAccessor(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterProtoAccessor(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterProtoAccessorTwoClasses(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterIndexed(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterQObject(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterQObjectAsVariant(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterQObjectMethod(Lookup *l, ExecutionEngine *engine, const Value &object);

    static ReturnedValue primitiveGetterProto(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue primitiveGetterAccessor(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue stringLengthGetter(Lookup *l, ExecutionEngine *engine, const Value &object);

    static ReturnedValue globalGetterGeneric(Lookup *l, ExecutionEngine *engine);
    static ReturnedValue globalGetterProto(Lookup *l, ExecutionEngine *engine);
    static ReturnedValue globalGetterProtoAccessor(Lookup *l, ExecutionEngine *engine);

    bool resolveSetter(ExecutionEngine *engine, Object *object, const Value &value);
    static bool setterGeneric(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    Q_NEVER_INLINE static bool setterTwoClasses(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setterFallback(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setterFallbackAsVariant(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setter0MemberData(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setter0Inline(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setter0setter0(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setterInsert(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setterQObject(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setterQObjectAsVariant(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool arrayLengthSetter(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);

    void markObjects(MarkStack *stack) {
        if (markDef.h1 && !(reinterpret_cast<quintptr>(markDef.h1) & 1))
            markDef.h1->mark(stack);
        if (markDef.h2 && !(reinterpret_cast<quintptr>(markDef.h2) & 1))
            markDef.h2->mark(stack);
    }

    void releasePropertyCache()
    {
        if (getter == getterQObject
                || getter == QQmlTypeWrapper::lookupSingletonProperty
                || setter == setterQObject
                || qmlContextPropertyGetter == QQmlContextWrapper::lookupScopeObjectProperty
                || qmlContextPropertyGetter == QQmlContextWrapper::lookupContextObjectProperty
                || getter == getterQObjectAsVariant
                || setter == setterQObjectAsVariant) {
            if (const QQmlPropertyCache *pc = qobjectLookup.propertyCache)
                pc->release();
        } else if (getter == getterQObjectMethod
                   || getter == QQmlTypeWrapper::lookupSingletonMethod
                   || qmlContextPropertyGetter == QQmlContextWrapper::lookupScopeObjectMethod
                   || qmlContextPropertyGetter == QQmlContextWrapper::lookupContextObjectMethod) {
            if (const QQmlPropertyCache *pc = qobjectMethodLookup.propertyCache)
                pc->release();
        }
    }
};

Q_STATIC_ASSERT(std::is_standard_layout<Lookup>::value);
// Ensure that these offsets are always at this point to keep generated code compatible
// across 32-bit and 64-bit (matters when cross-compiling).
Q_STATIC_ASSERT(offsetof(Lookup, getter) == 0);

inline void setupQObjectLookup(
        Lookup *lookup, const QQmlData *ddata, const QQmlPropertyData *propertyData)
{
    lookup->releasePropertyCache();
    Q_ASSERT(!ddata->propertyCache.isNull());
    lookup->qobjectLookup.propertyCache = ddata->propertyCache.data();
    lookup->qobjectLookup.propertyCache->addref();
    lookup->qobjectLookup.propertyData = propertyData;
}

inline void setupQObjectLookup(
        Lookup *lookup, const QQmlData *ddata, const QQmlPropertyData *propertyData,
        const Object *self)
{
    setupQObjectLookup(lookup, ddata, propertyData);
    lookup->qobjectLookup.ic = self->internalClass();
}


inline void setupQObjectLookup(
        Lookup *lookup, const QQmlData *ddata, const QQmlPropertyData *propertyData,
        const Object *self, const Object *qmlType)
{
    setupQObjectLookup(lookup, ddata, propertyData, self);
    lookup->qobjectLookup.qmlTypeIc = qmlType->internalClass();
}

inline void setupQObjectMethodLookup(
        Lookup *lookup, const QQmlData *ddata, const QQmlPropertyData *propertyData,
        const Object *self, Heap::QObjectMethod *method)
{
    lookup->releasePropertyCache();
    Q_ASSERT(!ddata->propertyCache.isNull());
    lookup->qobjectMethodLookup.method = method;
    lookup->qobjectMethodLookup.ic = self->internalClass();
    lookup->qobjectMethodLookup.propertyCache = ddata->propertyCache.data();
    lookup->qobjectMethodLookup.propertyCache->addref();
    lookup->qobjectMethodLookup.propertyData = propertyData;
}

inline bool qualifiesForMethodLookup(const QQmlPropertyData *propertyData)
{
    return propertyData->isFunction()
            && !propertyData->isSignalHandler() // TODO: Optimize SignalHandler, too
            && !propertyData->isVMEFunction() // Handled by QObjectLookup
            && !propertyData->isVarProperty();
}

}

QT_END_NAMESPACE

#endif
