// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QV4JSCALL_H
#define QV4JSCALL_H

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

#include <private/qqmlengine_p.h>
#include <private/qqmllistwrapper_p.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qv4alloca_p.h>
#include <private/qv4context_p.h>
#include <private/qv4dateobject_p.h>
#include <private/qv4function_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4object_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4regexpobject_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4stackframe_p.h>
#include <private/qv4urlobject_p.h>
#include <private/qv4variantobject_p.h>

#if QT_CONFIG(regularexpression)
#include <QtCore/qregularexpression.h>
#endif

QT_BEGIN_NAMESPACE

namespace QV4 {

template<typename Args>
CallData *callDatafromJS(const Scope &scope, const Args *args, const FunctionObject *f = nullptr)
{
    int size = int(offsetof(QV4::CallData, args)/sizeof(QV4::Value)) + args->argc;
    CallData *ptr = reinterpret_cast<CallData *>(scope.alloc<Scope::Uninitialized>(size));
    ptr->function = Encode::undefined();
    ptr->context = Encode::undefined();
    ptr->accumulator = Encode::undefined();
    ptr->thisObject = args->thisObject ? args->thisObject->asReturnedValue() : Encode::undefined();
    ptr->newTarget = Encode::undefined();
    ptr->setArgc(args->argc);
    if (args->argc)
        memcpy(ptr->args, args->args, args->argc*sizeof(Value));
    if (f)
        ptr->function = f->asReturnedValue();
    return ptr;
}

struct JSCallArguments
{
    JSCallArguments(const Scope &scope, int argc = 0)
        : thisObject(scope.alloc()), args(scope.alloc(argc)), argc(argc)
    {
    }

    CallData *callData(const Scope &scope, const FunctionObject *f = nullptr) const
    {
        return callDatafromJS(scope, this, f);
    }

    Value *thisObject;
    Value *args;
    const int argc;
};

struct JSCallData
{
    JSCallData(const Value *thisObject, const Value *argv, int argc)
        : thisObject(thisObject), args(argv), argc(argc)
    {
    }

    Q_IMPLICIT JSCallData(const JSCallArguments &args)
        : thisObject(args.thisObject), args(args.args), argc(args.argc)
    {
    }

    CallData *callData(const Scope &scope, const FunctionObject *f = nullptr) const
    {
        return callDatafromJS(scope, this, f);
    }

    const Value *thisObject;
    const Value *args;
    const int argc;
};

inline
ReturnedValue FunctionObject::callAsConstructor(const JSCallData &data) const
{
    return callAsConstructor(data.args, data.argc, this);
}

inline
ReturnedValue FunctionObject::call(const JSCallData &data) const
{
    return call(data.thisObject, data.args, data.argc);
}

void populateJSCallArguments(ExecutionEngine *v4, JSCallArguments &jsCall, int argc,
                             void **args, const QMetaType *types);

template<typename Callable>
ReturnedValue convertAndCall(
        ExecutionEngine *engine, const QQmlPrivate::AOTCompiledFunction *aotFunction,
        const Value *thisObject, const Value *argv, int argc, Callable call)
{
    const qsizetype numFunctionArguments = aotFunction->argumentTypes.size();
    Q_ALLOCA_VAR(void *, values, (numFunctionArguments + 1) * sizeof(void *));
    Q_ALLOCA_VAR(QMetaType, types, (numFunctionArguments + 1) * sizeof(QMetaType));

    for (qsizetype i = 0; i < numFunctionArguments; ++i) {
        const QMetaType argumentType = aotFunction->argumentTypes[i];
        types[i + 1] = argumentType;
        if (const qsizetype argumentSize = argumentType.sizeOf()) {
            Q_ALLOCA_VAR(void, argument, argumentSize);
            argumentType.construct(argument);
            if (i < argc)
                ExecutionEngine::metaTypeFromJS(argv[i], argumentType, argument);
            values[i + 1] = argument;
        } else {
            values[i + 1] = nullptr;
        }
    }

    Q_ALLOCA_DECLARE(void, returnValue);
    types[0] = aotFunction->returnType;
    if (const qsizetype returnSize = types[0].sizeOf()) {
        Q_ALLOCA_ASSIGN(void, returnValue, returnSize);
        values[0] = returnValue;
    } else {
        values[0] = nullptr;
    }

    if (const QV4::QObjectWrapper *cppThisObject = thisObject->as<QV4::QObjectWrapper>())
        call(cppThisObject->object(), values, types, argc);
    else
        call(nullptr, values, types, argc);

    ReturnedValue result;
    if (values[0]) {
        result = engine->metaTypeToJS(types[0], values[0]);
        types[0].destruct(values[0]);
    } else {
        result = Encode::undefined();
    }

    for (qsizetype i = 1, end = numFunctionArguments + 1; i < end; ++i)
        types[i].destruct(values[i]);

    return result;
}

template<typename Callable>
bool convertAndCall(ExecutionEngine *engine, QObject *thisObject,
                    void **a, const QMetaType *types, int argc, Callable call)
{
    Scope scope(engine);
    QV4::JSCallArguments jsCallData(scope, argc);

    for (int ii = 0; ii < argc; ++ii)
        jsCallData.args[ii] = engine->metaTypeToJS(types[ii + 1], a[ii + 1]);

    ScopedObject jsThisObject(scope);
    if (thisObject) {
        // The result of wrap() can only be null, undefined, or an object.
        jsThisObject = QV4::QObjectWrapper::wrap(engine, thisObject);
        if (!jsThisObject)
            jsThisObject = engine->globalObject;
    } else {
        jsThisObject = engine->globalObject;
    }

    ScopedValue jsResult(scope, call(jsThisObject, jsCallData.args, argc));
    void *result = a[0];
    if (!result)
        return !jsResult->isUndefined();

    const QMetaType resultType = types[0];
    if (scope.hasException()) {
        // Clear the return value
        resultType.construct(result);
    } else {
        // When the return type is QVariant, JS objects are to be returned as
        // QJSValue wrapped in QVariant. metaTypeFromJS unwraps them, unfortunately.
        if (resultType == QMetaType::fromType<QVariant>()) {
            new (result) QVariant(ExecutionEngine::toVariant(jsResult, QMetaType {}));
        } else {
            resultType.construct(result);
            ExecutionEngine::metaTypeFromJS(jsResult, resultType, result);
        }
    }
    return !jsResult->isUndefined();
}

inline ReturnedValue coerce(
    ExecutionEngine *engine, const Value &value, const QQmlType &qmlType, bool isList);

inline QObject *coerceQObject(const Value &value, const QQmlType &qmlType)
{
    QObject *o;
    if (const QV4::QObjectWrapper *wrapper = value.as<QV4::QObjectWrapper>())
        o = wrapper->object();
    else if (const QV4::QQmlTypeWrapper *wrapper = value.as<QQmlTypeWrapper>())
        o = wrapper->object();
    else
        return nullptr;

    return (o && qmlobject_can_qml_cast(o, qmlType)) ? o : nullptr;
}

enum CoercionProblem
{
    InsufficientAnnotation,
    InvalidListType
};

Q_QML_PRIVATE_EXPORT void warnAboutCoercionToVoid(
        ExecutionEngine *engine, const Value &value, CoercionProblem problem);

inline ReturnedValue coerceListType(
    ExecutionEngine *engine, const Value &value, const QQmlType &qmlType)
{
    QMetaType type = qmlType.qListTypeId();
    const auto metaSequence = [&]() {
        // TODO: We should really add the metasequence to the same QQmlType that holds
        //       all the other type information. Then we can get rid of the extra
        //       QQmlMetaType::qmlListType() here.
        return qmlType.isSequentialContainer()
                ? qmlType.listMetaSequence()
                : QQmlMetaType::qmlListType(type).listMetaSequence();
    };

    if (const QV4::Sequence *sequence = value.as<QV4::Sequence>()) {
        if (sequence->d()->listType() == type)
            return value.asReturnedValue();
    }

    if (const QmlListWrapper *list = value.as<QmlListWrapper>()) {
        if (list->d()->propertyType() == type)
            return value.asReturnedValue();
    }

    QMetaType listValueType = qmlType.typeId();
    if (!listValueType.isValid()) {
        warnAboutCoercionToVoid(engine, value, InvalidListType);
        return value.asReturnedValue();
    }

    QV4::Scope scope(engine);

    const ArrayObject *array = value.as<ArrayObject>();
    if (!array) {
        return (listValueType.flags() & QMetaType::PointerToQObject)
                   ? QmlListWrapper::create(engine, listValueType)
                   : SequencePrototype::fromData(engine, type, metaSequence(), nullptr);
    }

    if (listValueType.flags() & QMetaType::PointerToQObject) {
        QV4::Scoped<QmlListWrapper> newList(scope, QmlListWrapper::create(engine, listValueType));
        QQmlListProperty<QObject> *listProperty = newList->d()->property();

        const qsizetype length = array->getLength();
        qsizetype i = 0;
        for (; i < length; ++i) {
            ScopedValue v(scope, array->get(i));
            listProperty->append(listProperty, coerceQObject(v, qmlType));
        }

        return newList->asReturnedValue();
    }

    QV4::Scoped<Sequence> sequence(
            scope, SequencePrototype::fromData(engine, type, metaSequence(), nullptr));
    const qsizetype length = array->getLength();
    for (qsizetype i = 0; i < length; ++i)
        sequence->containerPutIndexed(i, array->get(i));
    return sequence->asReturnedValue();
}

inline ReturnedValue coerce(
    ExecutionEngine *engine, const Value &value, const QQmlType &qmlType, bool isList)
{
    // These are all the named non-list, non-QObject builtins. Only those need special handling.
    // Some of them may be wrapped in VariantObject because that is how they are stored in VME
    // properties.
    if (isList)
        return coerceListType(engine, value, qmlType);

    const QMetaType metaType = qmlType.typeId();
    if (!metaType.isValid()) {
        if (!value.isUndefined())
            warnAboutCoercionToVoid(engine, value, InsufficientAnnotation);
        return value.asReturnedValue();
    }

    switch (metaType.id()) {
    case QMetaType::Void:
        return Encode::undefined();
    case QMetaType::QVariant:
        return value.asReturnedValue();
    case QMetaType::Int:
        return Encode(value.toInt32());
    case QMetaType::Double:
        return value.convertedToNumber();
    case QMetaType::QString:
        return value.toString(engine)->asReturnedValue();
    case QMetaType::Bool:
        return Encode(value.toBoolean());
    case QMetaType::QDateTime:
        if (value.as<DateObject>())
            return value.asReturnedValue();
        if (const VariantObject *varObject = value.as<VariantObject>()) {
            const QVariant &var = varObject->d()->data();
            switch (var.metaType().id()) {
            case QMetaType::QDateTime:
                return engine->newDateObject(var.value<QDateTime>())->asReturnedValue();
            case QMetaType::QTime:
                return engine->newDateObject(var.value<QTime>(), nullptr, -1, 0)->asReturnedValue();
            case QMetaType::QDate:
                return engine->newDateObject(var.value<QDate>(), nullptr, -1, 0)->asReturnedValue();
            default:
                break;
            }
        }
        return engine->newDateObject(QDateTime())->asReturnedValue();
    case QMetaType::QUrl:
        if (value.as<UrlObject>())
            return value.asReturnedValue();
        if (const VariantObject *varObject = value.as<VariantObject>()) {
            const QVariant &var = varObject->d()->data();
            return var.metaType() == QMetaType::fromType<QUrl>()
                       ? engine->newUrlObject(var.value<QUrl>())->asReturnedValue()
                       : engine->newUrlObject()->asReturnedValue();
        }
        // Since URL properties are stored as string, we need to support the string conversion here.
        return value.isString()
            ? engine->newUrlObject(QUrl(value.stringValue()->toQString()))->asReturnedValue()
            : engine->newUrlObject()->asReturnedValue();
#if QT_CONFIG(regularexpression)
    case QMetaType::QRegularExpression:
        if (value.as<RegExpObject>())
            return value.asReturnedValue();
        if (const VariantObject *varObject = value.as<VariantObject>()) {
            const QVariant &var = varObject->d()->data();
            if (var.metaType() == QMetaType::fromType<QRegularExpression>())
                return engine->newRegExpObject(var.value<QRegularExpression>())->asReturnedValue();
        }
        return engine->newRegExpObject(QString(), 0)->asReturnedValue();
#endif
    default:
        break;
    }

    if (metaType.flags() & QMetaType::PointerToQObject) {
        return coerceQObject(value, qmlType)
                ? value.asReturnedValue()
                : Encode::null();
    }

    if (const QQmlValueTypeWrapper *wrapper = value.as<QQmlValueTypeWrapper>()) {
        if (wrapper->type() == metaType)
            return value.asReturnedValue();
    }

    if (void *target = QQmlValueTypeProvider::heapCreateValueType(qmlType, value)) {
        Heap::QQmlValueTypeWrapper *wrapper = engine->memoryManager->allocate<QQmlValueTypeWrapper>(
                nullptr, metaType, QQmlMetaType::metaObjectForValueType(qmlType),
                nullptr, -1, Heap::ReferenceObject::NoFlag);
        Q_ASSERT(!wrapper->gadgetPtr());
        wrapper->setGadgetPtr(target);
        return wrapper->asReturnedValue();
    }

    return Encode::undefined();
}

template<typename Callable>
ReturnedValue coerceAndCall(
    ExecutionEngine *engine,
    const Function::JSTypedFunction *typedFunction, const CompiledData::Function *compiledFunction,
    const Value *thisObject, const Value *argv, int argc, Callable call)
{
    Scope scope(engine);

    QV4::JSCallArguments jsCallData(scope, typedFunction->argumentTypes.size());
    const CompiledData::Parameter *formals = compiledFunction->formalsTable();
    for (qsizetype i = 0; i < jsCallData.argc; ++i) {
        jsCallData.args[i] = coerce(
            engine, i < argc ? argv[i] : Encode::undefined(),
            typedFunction->argumentTypes[i], formals[i].type.isList());
    }

    ScopedValue result(scope, call(thisObject, jsCallData.args, jsCallData.argc));
    return coerce(engine, result, typedFunction->returnType, compiledFunction->returnType.isList());
}

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4JSCALL_H
