// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qml/qqmlprivate.h"
#include "qv4function_p.h"
#include "qv4managed_p.h"
#include "qv4string_p.h"
#include "qv4value_p.h"
#include "qv4engine_p.h"
#include <private/qv4mm_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4functiontable_p.h>
#include <assembler/MacroAssemblerCodeRef.h>
#include <private/qv4vme_moth_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qv4jscall_p.h>
#include <private/qqmlpropertycachecreator_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

bool Function::call(QObject *thisObject, void **a, const QMetaType *types, int argc,
                    ExecutionContext *context)
{
    if (kind != AotCompiled) {
        return QV4::convertAndCall(
                    context->engine(), thisObject, a, types, argc,
                    [this, context](const Value *thisObject, const Value *argv, int argc) {
            return call(thisObject, argv, argc, context);
        });
    }

    ExecutionEngine *engine = context->engine();
    MetaTypesStackFrame frame;
    frame.init(this, thisObject, context, a, types, argc);
    frame.push(engine);
    Moth::VME::exec(&frame, engine);
    frame.pop(engine);
    return !frame.isReturnValueUndefined();
}

static ReturnedValue doCall(
        QV4::Function *self, const QV4::Value *thisObject, const QV4::Value *argv, int argc,
        QV4::ExecutionContext *context)
{
    ExecutionEngine *engine = context->engine();
    JSTypesStackFrame frame;
    frame.init(self, argv, argc);
    frame.setupJSFrame(engine->jsStackTop, Value::undefinedValue(), context->d(),
                       thisObject ? *thisObject : Value::undefinedValue());
    engine->jsStackTop += frame.requiredJSStackFrameSize();
    frame.push(engine);
    ReturnedValue result = Moth::VME::exec(&frame, engine);
    frame.pop(engine);
    return result;
}

ReturnedValue Function::call(
        const Value *thisObject, const Value *argv, int argc, ExecutionContext *context) {
    switch (kind) {
    case AotCompiled:
        return QV4::convertAndCall(
                    context->engine(), aotCompiledFunction, thisObject, argv, argc,
                    [this, context](
                        QObject *thisObject, void **a, const QMetaType *types, int argc) {
            call(thisObject, a, types, argc, context);
        });
    case JsTyped:
        return QV4::coerceAndCall(
                    context->engine(), aotCompiledFunction, thisObject, argv, argc,
                    [this, context](const Value *thisObject, const Value *argv, int argc) {
            return doCall(this, thisObject, argv, argc, context);
        });
    default:
        break;
    }

    return doCall(this, thisObject, argv, argc, context);
}

Function *Function::create(ExecutionEngine *engine, ExecutableCompilationUnit *unit,
                           const CompiledData::Function *function,
                           const QQmlPrivate::AOTCompiledFunction *aotFunction)
{
    return new Function(engine, unit, function, aotFunction);
}

void Function::destroy()
{
    delete this;
}

Function::Function(ExecutionEngine *engine, ExecutableCompilationUnit *unit,
                   const CompiledData::Function *function,
                   const QQmlPrivate::AOTCompiledFunction *aotFunction)
    : FunctionData(unit)
    , compiledFunction(function)
    , codeData(function->code())
    , jittedCode(nullptr)
    , codeRef(nullptr)
    , aotCompiledFunction(aotFunction)
    , kind(aotFunction ? AotCompiled : JsUntyped)
{
    Scope scope(engine);
    Scoped<InternalClass> ic(scope, engine->internalClasses(EngineBase::Class_CallContext));

    // first locals
    const quint32_le *localsIndices = compiledFunction->localsTable();
    for (quint32 i = 0; i < compiledFunction->nLocals; ++i)
        ic = ic->addMember(engine->identifierTable->asPropertyKey(compilationUnit->runtimeStrings[localsIndices[i]]), Attr_NotConfigurable);

    const CompiledData::Parameter *formalsIndices = compiledFunction->formalsTable();
    const bool enforcesSignature = !aotFunction && unit->enforcesFunctionSignature();
    bool hasTypes = false;
    for (quint32 i = 0; i < compiledFunction->nFormals; ++i) {
        ic = ic->addMember(engine->identifierTable->asPropertyKey(compilationUnit->runtimeStrings[formalsIndices[i].nameIndex]), Attr_NotConfigurable);
        if (enforcesSignature
                && !hasTypes
                && formalsIndices[i].type.typeNameIndexOrCommonType()
                   != quint32(QV4::CompiledData::CommonType::Invalid)) {
            hasTypes = true;
        }
    }
    internalClass = ic->d();

    nFormals = compiledFunction->nFormals;

    if (!enforcesSignature)
        return;

    if (!hasTypes
            && compiledFunction->returnType.typeNameIndexOrCommonType()
               == quint32(QV4::CompiledData::CommonType::Invalid)) {
        return;
    }

    QQmlPrivate::AOTCompiledFunction *synthesized = new QQmlPrivate::AOTCompiledFunction;
    QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine->qmlEngine());

    auto findMetaType = [&](const CompiledData::ParameterType &param) {
        const quint32 type = param.typeNameIndexOrCommonType();
        if (param.indexIsCommonType()) {
            if (param.isList()) {
                return QQmlPropertyCacheCreatorBase::listTypeForPropertyType(
                    QV4::CompiledData::CommonType(type));
            }
            return QQmlPropertyCacheCreatorBase::metaTypeForPropertyType(
                QV4::CompiledData::CommonType(type));
        }

        if (type == 0)
            return QMetaType();

        const QQmlType qmltype = unit->typeNameCache->query(unit->stringAt(type)).type;
        if (!qmltype.isValid())
            return QMetaType();

        const QMetaType metaType = param.isList() ? qmltype.qListTypeId() : qmltype.typeId();
        if (metaType.isValid())
            return metaType;

        if (!qmltype.isComposite()) {
            if (!qmltype.isInlineComponentType())
                return QMetaType();
            const CompositeMetaTypeIds typeIds = unit->typeIdsForComponent(qmltype.elementName());
            return param.isList() ? typeIds.listId : typeIds.id;
        }

        const CompositeMetaTypeIds typeIds = enginePrivate->typeLoader.getType(
                    qmltype.sourceUrl())->compilationUnit()->typeIds;
        return param.isList() ? typeIds.listId : typeIds.id;
    };

    for (quint16 i = 0; i < nFormals; ++i)
        synthesized->argumentTypes.append(findMetaType(formalsIndices[i].type));

    synthesized->returnType = findMetaType(compiledFunction->returnType);
    aotCompiledFunction = synthesized;
    kind = JsTyped;
}

Function::~Function()
{
    if (codeRef) {
        destroyFunctionTable(this, codeRef);
        delete codeRef;
    }
    if (kind == JsTyped)
        delete aotCompiledFunction;
}

void Function::updateInternalClass(ExecutionEngine *engine, const QList<QByteArray> &parameters)
{
    QStringList parameterNames;

    // Resolve duplicate parameter names:
    for (int i = 0, ei = parameters.size(); i != ei; ++i) {
        const QByteArray &param = parameters.at(i);
        int duplicate = -1;

        for (int j = i - 1; j >= 0; --j) {
            const QByteArray &prevParam = parameters.at(j);
            if (param == prevParam) {
                duplicate = j;
                break;
            }
        }

        if (duplicate == -1) {
            parameterNames.append(QString::fromUtf8(param));
        } else {
            const QString dup = parameterNames[duplicate];
            parameterNames.append(dup);
            parameterNames[duplicate] =
                    QString(QChar(0xfffe)) + QString::number(duplicate) + dup;
        }

    }

    internalClass = engine->internalClasses(EngineBase::Class_CallContext);

    // first locals
    const quint32_le *localsIndices = compiledFunction->localsTable();
    for (quint32 i = 0; i < compiledFunction->nLocals; ++i) {
        internalClass = internalClass->addMember(
                engine->identifierTable->asPropertyKey(compilationUnit->runtimeStrings[localsIndices[i]]),
                Attr_NotConfigurable);
    }

    Scope scope(engine);
    ScopedString arg(scope);
    for (const QString &parameterName : parameterNames) {
        arg = engine->newIdentifier(parameterName);
        internalClass = internalClass->addMember(arg->propertyKey(), Attr_NotConfigurable);
    }
    nFormals = parameters.size();
}

QString Function::prettyName(const Function *function, const void *code)
{
    QString prettyName = function ? function->name()->toQString() : QString();
    if (prettyName.isEmpty()) {
        prettyName = QString::number(reinterpret_cast<quintptr>(code), 16);
        prettyName.prepend(QLatin1String("QV4::Function(0x"));
        prettyName.append(QLatin1Char(')'));
    }
    return prettyName;
}

QQmlSourceLocation Function::sourceLocation() const
{
    return QQmlSourceLocation(
            sourceFile(), compiledFunction->location.line(), compiledFunction->location.column());
}

} // namespace QV4

QT_END_NAMESPACE
