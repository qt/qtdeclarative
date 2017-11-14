/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
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

#include "qv4global_p.h"
#include "qv4runtime_p.h"
#include "qv4engine_p.h"
#include "qv4context_p.h"

#if !defined(V4_BOOTSTRAP)
#include "qv4object_p.h"
#include "qv4internalclass_p.h"
#endif

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Lookup {
    enum { Size = 4 };
    union {
        ReturnedValue (*getter)(Lookup *l, ExecutionEngine *engine, const Value &object);
        ReturnedValue (*globalGetter)(Lookup *l, ExecutionEngine *engine);
        bool (*setter)(Lookup *l, ExecutionEngine *engine, Value &object, const Value &v);
    };
    union {
        struct {
            InternalClass *ic;
            int offset;
        } objectLookup;
        struct {
            const Value *data;
            int icIdentifier;
        } protoLookup;
        struct {
            InternalClass *ic;
            InternalClass *ic2;
            int offset;
            int offset2;
        } objectLookupTwoClasses;
        struct {
            const Value *data;
            const Value *data2;
            int icIdentifier;
            int icIdentifier2;
        } protoLookupTwoClasses;
        struct {
            // Make sure the next two values are in sync with protoLookup
            const Value *data;
            int icIdentifier;
            unsigned type;
            Heap::Object *proto;
        } primitiveLookup;
        struct {
            InternalClass *newClass;
            int icIdentifier;
            int offset;
        } insertionLookup;
    };
    uint nameIndex;

    ReturnedValue resolveGetter(ExecutionEngine *engine, const Object *object);
    ReturnedValue resolvePrimitiveGetter(ExecutionEngine *engine, const Value &object);
    ReturnedValue resolveGlobalGetter(ExecutionEngine *engine);
    void resolveProtoGetter(Identifier *name, const Heap::Object *proto);

    static ReturnedValue getterGeneric(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterTwoClasses(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue getterFallback(Lookup *l, ExecutionEngine *engine, const Value &object);

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

    static ReturnedValue primitiveGetterProto(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue primitiveGetterAccessor(Lookup *l, ExecutionEngine *engine, const Value &object);
    static ReturnedValue stringLengthGetter(Lookup *l, ExecutionEngine *engine, const Value &object);

    static ReturnedValue globalGetterGeneric(Lookup *l, ExecutionEngine *engine);
    static ReturnedValue globalGetterProto(Lookup *l, ExecutionEngine *engine);
    static ReturnedValue globalGetterProtoAccessor(Lookup *l, ExecutionEngine *engine);

    bool resolveSetter(ExecutionEngine *engine, Object *object, const Value &value);
    static bool setterGeneric(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setterTwoClasses(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setterFallback(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setter0(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setter0Inline(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setter0setter0(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool setterInsert(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
    static bool arrayLengthSetter(Lookup *l, ExecutionEngine *engine, Value &object, const Value &value);
};

Q_STATIC_ASSERT(std::is_standard_layout<Lookup>::value);
// Ensure that these offsets are always at this point to keep generated code compatible
// across 32-bit and 64-bit (matters when cross-compiling).
Q_STATIC_ASSERT(offsetof(Lookup, getter) == 0);

}

QT_END_NAMESPACE

#endif
