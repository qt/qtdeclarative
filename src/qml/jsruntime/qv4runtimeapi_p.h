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
#ifndef QV4RUNTIMEAPI_P_H
#define QV4RUNTIMEAPI_P_H

#include <private/qv4global_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct NoThrowEngine;

struct Q_QML_PRIVATE_EXPORT Runtime {
    // call
    static ReturnedValue callGlobalLookup(ExecutionEngine *engine, uint index, CallData *callData);
    static ReturnedValue callActivationProperty(ExecutionEngine *engine, int nameIndex, CallData *callData);
    static ReturnedValue callQmlScopeObjectProperty(ExecutionEngine *engine, int propertyIndex, CallData *callData);
    static ReturnedValue callQmlContextObjectProperty(ExecutionEngine *engine, int propertyIndex, CallData *callData);
    static ReturnedValue callProperty(ExecutionEngine *engine, int nameIndex, CallData *callData);
    static ReturnedValue callPropertyLookup(ExecutionEngine *engine, uint index, CallData *callData);
    static ReturnedValue callElement(ExecutionEngine *engine, const Value &index, CallData *callData);
    static ReturnedValue callValue(ExecutionEngine *engine, const Value &func, CallData *callData);

    // construct
    static ReturnedValue constructGlobalLookup(ExecutionEngine *engine, uint index, CallData *callData);
    static ReturnedValue constructActivationProperty(ExecutionEngine *engine, int nameIndex, CallData *callData);
    static ReturnedValue constructProperty(ExecutionEngine *engine, int nameIndex, CallData *callData);
    static ReturnedValue constructPropertyLookup(ExecutionEngine *engine, uint index, CallData *callData);
    static ReturnedValue constructValue(ExecutionEngine *engine, const Value &func, CallData *callData);

    // set & get
    static void setActivationProperty(ExecutionEngine *engine, int nameIndex, const Value &value);
    static void setProperty(ExecutionEngine *engine, const Value &object, int nameIndex, const Value &value);
    static void setElement(ExecutionEngine *engine, const Value &object, const Value &index, const Value &value);
    static ReturnedValue getProperty(ExecutionEngine *engine, const Value &object, int nameIndex);
    static ReturnedValue getActivationProperty(ExecutionEngine *engine, int nameIndex);
    static ReturnedValue getElement(ExecutionEngine *engine, const Value &object, const Value &index);

    // typeof
    static ReturnedValue typeofValue(ExecutionEngine *engine, const Value &val);
    static ReturnedValue typeofName(ExecutionEngine *engine, int nameIndex);
    static ReturnedValue typeofScopeObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex);
    static ReturnedValue typeofContextObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex);
    static ReturnedValue typeofMember(ExecutionEngine *engine, const Value &base, int nameIndex);
    static ReturnedValue typeofElement(ExecutionEngine *engine, const Value &base, const Value &index);

    // delete
    static ReturnedValue deleteElement(ExecutionEngine *engine, const Value &base, const Value &index);
    static ReturnedValue deleteMember(ExecutionEngine *engine, const Value &base, int nameIndex);
    static ReturnedValue deleteMemberString(ExecutionEngine *engine, const Value &base, String *name);
    static ReturnedValue deleteName(ExecutionEngine *engine, int nameIndex);

    // exceptions & scopes
    static void throwException(ExecutionEngine *engine, const Value &value);
    static ReturnedValue unwindException(ExecutionEngine *engine);
    static void pushWithScope(const Value &o, ExecutionEngine *engine);
    static void pushCatchScope(NoThrowEngine *engine, int exceptionVarNameIndex);
    static void popScope(ExecutionEngine *engine);

    // closures
    static ReturnedValue closure(ExecutionEngine *engine, int functionId);

    // function header
    static void declareVar(ExecutionEngine *engine, bool deletable, int nameIndex);
    static ReturnedValue setupArgumentsObject(ExecutionEngine *engine);
    static void convertThisToObject(ExecutionEngine *engine);

    // literals
    static ReturnedValue arrayLiteral(ExecutionEngine *engine, Value *values, uint length);
    static ReturnedValue objectLiteral(ExecutionEngine *engine, const Value *args, int classId, int arrayValueCount, int arrayGetterSetterCountAndFlags);
    static ReturnedValue regexpLiteral(ExecutionEngine *engine, int id);

    // foreach
    static ReturnedValue foreachIterator(ExecutionEngine *engine, const Value &in);
    static ReturnedValue foreachNextPropertyName(const Value &foreach_iterator);

    // unary operators
    typedef ReturnedValue (*UnaryOperation)(const Value &value);
    static ReturnedValue uPlus(const Value &value);
    static ReturnedValue uMinus(const Value &value);
    static ReturnedValue uNot(const Value &value);
    static ReturnedValue complement(const Value &value);
    static ReturnedValue increment(const Value &value);
    static ReturnedValue decrement(const Value &value);

    // binary operators
    typedef ReturnedValue (*BinaryOperation)(const Value &left, const Value &right);
    typedef ReturnedValue (*BinaryOperationContext)(ExecutionEngine *engine, const Value &left, const Value &right);

    static ReturnedValue instanceof(ExecutionEngine *engine, const Value &left, const Value &right);
    static ReturnedValue in(ExecutionEngine *engine, const Value &left, const Value &right);
    static ReturnedValue add(ExecutionEngine *engine, const Value &left, const Value &right);
    static ReturnedValue addString(ExecutionEngine *engine, const Value &left, const Value &right);
    static ReturnedValue bitOr(const Value &left, const Value &right);
    static ReturnedValue bitXor(const Value &left, const Value &right);
    static ReturnedValue bitAnd(const Value &left, const Value &right);
    static ReturnedValue sub(const Value &left, const Value &right);
    static ReturnedValue mul(const Value &left, const Value &right);
    static ReturnedValue div(const Value &left, const Value &right);
    static ReturnedValue mod(const Value &left, const Value &right);
    static ReturnedValue shl(const Value &left, const Value &right);
    static ReturnedValue shr(const Value &left, const Value &right);
    static ReturnedValue ushr(const Value &left, const Value &right);
    static ReturnedValue greaterThan(const Value &left, const Value &right);
    static ReturnedValue lessThan(const Value &left, const Value &right);
    static ReturnedValue greaterEqual(const Value &left, const Value &right);
    static ReturnedValue lessEqual(const Value &left, const Value &right);
    static ReturnedValue equal(const Value &left, const Value &right);
    static ReturnedValue notEqual(const Value &left, const Value &right);
    static ReturnedValue strictEqual(const Value &left, const Value &right);
    static ReturnedValue strictNotEqual(const Value &left, const Value &right);

    // comparisons
    typedef Bool (*CompareOperation)(const Value &left, const Value &right);
    static Bool compareGreaterThan(const Value &l, const Value &r);
    static Bool compareLessThan(const Value &l, const Value &r);
    static Bool compareGreaterEqual(const Value &l, const Value &r);
    static Bool compareLessEqual(const Value &l, const Value &r);
    static Bool compareEqual(const Value &left, const Value &right);
    static Bool compareNotEqual(const Value &left, const Value &right);
    static Bool compareStrictEqual(const Value &left, const Value &right);
    static Bool compareStrictNotEqual(const Value &left, const Value &right);

    typedef Bool (*CompareOperationContext)(ExecutionEngine *engine, const Value &left, const Value &right);
    static Bool compareInstanceof(ExecutionEngine *engine, const Value &left, const Value &right);
    static Bool compareIn(ExecutionEngine *engine, const Value &left, const Value &right);

    // conversions
    static Bool toBoolean(const Value &value);
    static ReturnedValue toDouble(const Value &value);
    static int toInt(const Value &value);
    static int doubleToInt(const double &d);
    static unsigned toUInt(const Value &value);
    static unsigned doubleToUInt(const double &d);

    // qml
    static ReturnedValue getQmlContext(NoThrowEngine *engine);
    static ReturnedValue getQmlImportedScripts(NoThrowEngine *engine);
    static ReturnedValue getQmlSingleton(NoThrowEngine *engine, int nameIndex);
    static ReturnedValue getQmlAttachedProperty(ExecutionEngine *engine, int attachedPropertiesId, int propertyIndex);
    static ReturnedValue getQmlScopeObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex);
    static ReturnedValue getQmlContextObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex);
    static ReturnedValue getQmlQObjectProperty(ExecutionEngine *engine, const Value &object, int propertyIndex, bool captureRequired);
    static ReturnedValue getQmlSingletonQObjectProperty(ExecutionEngine *engine, const Value &object, int propertyIndex, bool captureRequired);
    static ReturnedValue getQmlIdObject(ExecutionEngine *engine, const Value &context, uint index);

    static void setQmlScopeObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex, const Value &value);
    static void setQmlContextObjectProperty(ExecutionEngine *engine, const Value &context, int propertyIndex, const Value &value);
    static void setQmlQObjectProperty(ExecutionEngine *engine, const Value &object, int propertyIndex, const Value &value);
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4RUNTIMEAPI_P_H
