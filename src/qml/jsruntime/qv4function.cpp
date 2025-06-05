// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4function_p.h"

#include <private/qqmlpropertycachecreator_p.h>
#include <private/qqmltype_p_p.h>

#include <private/qv4engine_p.h>
#include <private/qv4functiontable_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4jscall_p.h>
#include <private/qv4vme_moth_p.h>

#include <assembler/MacroAssemblerCodeRef.h>

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
                    context->engine(), &aotCompiledFunction, thisObject, argv, argc,
                    [this, context](
                        QObject *thisObject, void **a, const QMetaType *types, int argc) {
            call(thisObject, a, types, argc, context);
        });
    case JsTyped:
        return QV4::coerceAndCall(
                    context->engine(), &jsTypedFunction, compiledFunction, argv, argc,
                    [this, context, thisObject](const Value *argv, int argc) {
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

void Function::mark(MarkStack *ms)
{
    if (internalClass)
        internalClass->mark(ms);
}

static bool isSpecificType(const CompiledData::ParameterType &type)
{
    return type.typeNameIndexOrCommonType()
           != (type.indexIsCommonType() ? quint32(CompiledData::CommonType::Invalid) : 0);
}

Function::Function(ExecutionEngine *engine, ExecutableCompilationUnit *unit,
                   const CompiledData::Function *function,
                   const QQmlPrivate::AOTCompiledFunction *aotFunction)
    : FunctionData(engine, unit)
    , compiledFunction(function)
    , codeData(function->code())
{
    Scope scope(engine);
    Scoped<InternalClass> ic(scope, engine->internalClasses(EngineBase::Class_CallContext));

    // first locals
    const quint32_le *localsIndices = compiledFunction->localsTable();
    for (quint32 i = 0; i < compiledFunction->nLocals; ++i)
        ic = ic->addMember(engine->identifierTable->asPropertyKey(compilationUnit->runtimeStrings[localsIndices[i]]), Attr_NotConfigurable);

    const CompiledData::Parameter *formalsIndices = compiledFunction->formalsTable();
    bool enforceJsTypes = !unit->ignoresFunctionSignature();

    for (quint32 i = 0; i < compiledFunction->nFormals; ++i) {
        ic = ic->addMember(engine->identifierTable->asPropertyKey(compilationUnit->runtimeStrings[formalsIndices[i].nameIndex]), Attr_NotConfigurable);
        if (enforceJsTypes && !isSpecificType(formalsIndices[i].type))
            enforceJsTypes = false;
    }
    internalClass.set(engine, ic->d());

    nFormals = compiledFunction->nFormals;

    if (!enforceJsTypes)
        return;

    if (aotFunction) {
        aotCompiledCode = aotFunction->functionPtr;
        new (&aotCompiledFunction) AOTCompiledFunction;
        kind = AotCompiled;
        aotCompiledFunction.types.resize(aotFunction->numArguments + 1);
        aotFunction->signature(unit, aotCompiledFunction.types.data());
        return;
    }

    // If a function has any typed arguments, but an untyped return value, the return value is void.
    // If it doesn't have any arguments at all and the return value is untyped, the function is
    // untyped. Users can specifically set the return type to "void" to have it enforced.
    if (nFormals == 0 && !isSpecificType(compiledFunction->returnType))
        return;

    QQmlTypeLoader *typeLoader = engine->typeLoader();

    const auto isEnumUsedAsType = [&](const QV4::ExecutableCompilationUnit *unit,
                                      int elementNameId, QQmlTypeLoader *typeLoader,
                                      const quint16 *parameter = nullptr) {
        const QStringView name = unit->baseCompilationUnit()->stringAt(elementNameId);
        const auto split = name.tokenize(u'.').toContainer<QVarLengthArray<QStringView, 4>>();
        if (split.size() != 2)
            return false;

        const QStringView scopeName = split[0];
        const QStringView enumName = split[1];

        auto *pengine = QQmlEnginePrivate::get(engine);
        const auto warn = [&] {
            QQmlError error;
            auto where = parameter ? QStringLiteral("parameter ") + QString::number(*parameter)
                                   : QStringLiteral("return type");
            auto msg = QStringLiteral("Type annotation for %1 of function %2: Enumerations are "
                                      "not types. Use underlying type (int or double) instead.")
                               .arg(where, this->name()->toQString());
            error.setDescription(msg);
            error.setUrl(QUrl(sourceFile()));
            error.setLine(this->sourceLocation().line);
            error.setColumn(this->sourceLocation().column);
            error.setMessageType(QtWarningMsg);
            pengine->warning(error);
        };

        bool ok;
        if (scopeName == QStringLiteral("Qt")) {
            const QMetaObject *mo = &Qt::staticMetaObject;
            for (int i = 0; i < mo->enumeratorCount(); ++i) {
                if (mo->enumerator(i).name() == enumName.toLatin1()) {
                    warn();
                    return true;
                }
            }
            return false;
        }

        const QQmlType scope = unit->typeNameCache()->query<QQmlImport::AllowRecursion>(
                                                            scopeName, typeLoader).type;
        if (!scope.isValid())
            return false;

        scope.scopedEnumIndex(typeLoader, enumName.toString(), &ok);
        if (ok) {
            warn();
            return true;
        }
        scope.unscopedEnumIndex(typeLoader, enumName.toString(), &ok);
        if (ok) {
            warn();
            return true;
        }

        return false;
    };

    auto findQmlType = [&](const CompiledData::ParameterType &param,
                           const quint16 *parameter = nullptr) {
        const quint32 type = param.typeNameIndexOrCommonType();
        if (param.indexIsCommonType()) {
            return QQmlMetaType::qmlType(QQmlPropertyCacheCreatorBase::metaTypeForPropertyType(
                QV4::CompiledData::CommonType(type)));
        }

        if (type == 0 || !typeLoader)
            return QQmlType();

        if (isEnumUsedAsType(unit, type, typeLoader, parameter))
            return QQmlType();

        const QQmlType qmltype = QQmlTypePrivate::visibleQmlTypeByName(unit, type, typeLoader);
        return qmltype.typeId().isValid() ? qmltype : QQmlType();
    };

    new (&jsTypedFunction) JSTypedFunction;
    kind = JsTyped;
    jsTypedFunction.types.reserve(nFormals + 1);
    jsTypedFunction.types.append(findQmlType(compiledFunction->returnType));
    for (quint16 i = 0; i < nFormals; ++i)
        jsTypedFunction.types.append(findQmlType(formalsIndices[i].type, &i));
}

Function::~Function()
{
    if (codeRef) {
        destroyFunctionTable(this, codeRef);
        delete codeRef;
    }

    switch (kind) {
    case JsTyped:
        jsTypedFunction.~JSTypedFunction();
        break;
    case AotCompiled:
        aotCompiledFunction.~AOTCompiledFunction();
        break;
    case JsUntyped:
    case Eval:
        break;
    }
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

    Scope scope(engine);
    Scoped<InternalClass> ic(scope, engine->internalClasses(EngineBase::Class_CallContext));

    // first locals
    const quint32_le *localsIndices = compiledFunction->localsTable();
    for (quint32 i = 0; i < compiledFunction->nLocals; ++i) {
        ic = ic->addMember(
                engine->identifierTable->asPropertyKey(compilationUnit->runtimeStrings[localsIndices[i]]),
                Attr_NotConfigurable);
    }

    ScopedString arg(scope);
    for (const QString &parameterName : parameterNames) {
        arg = engine->newIdentifier(parameterName);
        ic = ic->addMember(arg->propertyKey(), Attr_NotConfigurable);
    }
    internalClass.set(engine, ic->d());
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

FunctionData::FunctionData(EngineBase *engine, ExecutableCompilationUnit *compilationUnit_)
{
    compilationUnit.set(engine, compilationUnit_);
}

} // namespace QV4

QT_END_NAMESPACE
