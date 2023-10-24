// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljscodegenerator_p.h"
#include "qqmljsmetatypes_p.h"
#include "qqmljsregistercontent_p.h"
#include "qqmljsscope_p.h"
#include "qqmljsutils_p.h"

#include <private/qqmljstypepropagator_p.h>

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljsutils_p.h>
#include <private/qv4compilerscanfunctions_p.h>
#include <private/qduplicatetracker_p.h>

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
 * \internal
 * \class QQmlJSCodeGenerator
 *
 * This is a final compile pass that generates C++ code from a function and the
 * annotations produced by previous passes. Such annotations are produced by
 * QQmlJSTypePropagator, and possibly amended by other passes.
 */

#define BYTECODE_UNIMPLEMENTED() Q_ASSERT_X(false, Q_FUNC_INFO, "not implemented");

#define INJECT_TRACE_INFO(function) \
    static const bool injectTraceInfo = true; \
    if (injectTraceInfo) { \
        m_body += u"// "_s + QStringLiteral(#function) + u'\n'; \
    }


static bool isTypeStorable(const QQmlJSTypeResolver *resolver, const QQmlJSScope::ConstPtr &type)
{
    return !type.isNull()
            && !resolver->equals(type, resolver->nullType())
            && !resolver->equals(type, resolver->voidType());
}

QString QQmlJSCodeGenerator::castTargetName(const QQmlJSScope::ConstPtr &type) const
{
    return type->augmentedInternalName();
}

QQmlJSCodeGenerator::QQmlJSCodeGenerator(const QV4::Compiler::Context *compilerContext,
        const QV4::Compiler::JSUnitGenerator *unitGenerator,
        const QQmlJSTypeResolver *typeResolver,
        QQmlJSLogger *logger)
    : QQmlJSCompilePass(unitGenerator, typeResolver, logger)
    , m_context(compilerContext)
{}

QString QQmlJSCodeGenerator::metaTypeFromType(const QQmlJSScope::ConstPtr &type) const
{
    return u"QMetaType::fromType<"_s + type->augmentedInternalName() + u">()"_s;
}

QString QQmlJSCodeGenerator::metaTypeFromName(const QQmlJSScope::ConstPtr &type) const
{
    return u"[]() { static const auto t = QMetaType::fromName(\""_s
            + QString::fromUtf8(QMetaObject::normalizedType(type->augmentedInternalName().toUtf8()))
            + u"\"); return t; }()"_s;
}

QString QQmlJSCodeGenerator::metaObject(const QQmlJSScope::ConstPtr &objectType)
{
    if (!objectType->isComposite()) {
        if (objectType->internalName() == u"QObject"_s
                || objectType->internalName() == u"QQmlComponent"_s) {
            return u'&' + objectType->internalName() + u"::staticMetaObject"_s;
        }
        return metaTypeFromName(objectType) + u".metaObject()"_s;
    }

    reject(u"retrieving the metaObject of a composite type without using an instance."_s);
    return QString();
}

QString QQmlJSCodeGenerator::metaType(const QQmlJSScope::ConstPtr &type)
{
    return m_typeResolver->equals(m_typeResolver->genericType(type), type)
            ? metaTypeFromType(type)
            : metaTypeFromName(type);
}

QQmlJSAotFunction QQmlJSCodeGenerator::run(
        const Function *function, const InstructionAnnotations *annotations,
        QQmlJS::DiagnosticMessage *error)
{
    m_annotations = annotations;
    m_function = function;
    m_error = error;

    QHash<int, int> numRegisterVariablesPerIndex;

    auto addVariable = [&](int registerIndex, const QQmlJSScope::ConstPtr &seenType) {
        // Don't generate any variables for registers that are initialized with undefined.
        if (registerIndex == InvalidRegister || !isTypeStorable(m_typeResolver, seenType))
            return;

        const RegisterVariablesKey key = { seenType->internalName(), registerIndex };


        const auto oldSize = m_registerVariables.size();
        auto &e = m_registerVariables[key];
        if (m_registerVariables.size() != oldSize) {
            e.variableName = u"r%1_%2"_s
                                     .arg(registerIndex)
                                     .arg(numRegisterVariablesPerIndex[registerIndex]++);
            e.storedType = m_typeResolver->comparableType(seenType);
        }
        ++e.numTracked;
    };

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wrange-loop-analysis")
    for (const auto &annotation : *m_annotations) {
        addVariable(annotation.second.changedRegisterIndex,
                    annotation.second.changedRegister.storedType());
        for (auto it = annotation.second.typeConversions.begin(),
             end = annotation.second.typeConversions.end();
             it != end; ++it) {
            addVariable(it.key(), it.value().content.storedType());
        }
    }
QT_WARNING_POP

    // ensure we have m_labels for loops
    for (const auto loopLabel : m_context->labelInfo)
        m_labels.insert(loopLabel, u"label_%1"_s.arg(m_labels.size()));

    // Initialize the first instruction's state to hold the arguments.
    // After this, the arguments (or whatever becomes of them) are carried
    // over into any further basic blocks automatically.
    m_state.State::operator=(initialState(m_function));

    const QByteArray byteCode = function->code;
    decode(byteCode.constData(), static_cast<uint>(byteCode.size()));

    QQmlJSAotFunction result;
    result.includes.swap(m_includes);

    result.code += u"// %1 at line %2, column %3\n"_s
            .arg(m_context->name).arg(m_context->line).arg(m_context->column);

    for (auto registerIt = m_registerVariables.cbegin(), registerEnd = m_registerVariables.cend();
         registerIt != registerEnd; ++registerIt) {

        const int registerIndex = registerIt.key().registerIndex;
        const bool registerIsArgument = isArgument(registerIndex);

        result.code += registerIt.key().internalName;

        const QQmlJSScope::ConstPtr storedType = registerIt->storedType;
        const bool isPointer
                = (storedType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference);
        if (isPointer)
            result.code += u" *"_s;
        else
            result.code += u' ';

        if (!registerIsArgument
                && registerIndex != Accumulator
                && registerIndex != This
                && !m_typeResolver->registerIsStoredIn(
                    function->registerTypes[registerIndex - firstRegisterIndex()],
                    m_typeResolver->voidType())) {
            result.code += registerIt->variableName + u" = "_s;
            result.code += convertStored(m_typeResolver->voidType(), storedType, QString());
        } else if (registerIsArgument && m_typeResolver->registerIsStoredIn(
                       argumentType(registerIndex), storedType)) {
            const int argumentIndex = registerIndex - FirstArgument;
            const QQmlJSRegisterContent argument
                    = m_function->argumentTypes[argumentIndex];
            const QQmlJSRegisterContent original
                    = m_typeResolver->original(argument);

            const bool needsConversion = argument != original;
            if (!isPointer && registerIt->numTracked == 1 && !needsConversion) {
                // Not a pointer, never written to, and doesn't need any initial conversion.
                // This is a readonly argument.
                //
                // We would like to make the variable a const ref if it's a readonly argument,
                // but due to the various call interfaces accepting non-const values, we can't.
                // We rely on those calls to still not modify their arguments in place.
                result.code += u'&';
            }

            result.code += registerIt->variableName + u" = "_s;

            const QString originalValue = u"*static_cast<"_s + castTargetName(original.storedType())
                    + u"*>(argumentsPtr["_s + QString::number(argumentIndex) + u"])"_s;

            if (needsConversion)
                result.code += conversion(original, argument, originalValue);
            else
                result.code += originalValue;
        } else {
            result.code += registerIt->variableName;
        }
        result.code += u";\n"_s;
    }

    result.code += m_body;

    for (const QQmlJSRegisterContent &argType : std::as_const(function->argumentTypes)) {
        if (argType.isValid()) {
            result.argumentTypes.append(
                        m_typeResolver->originalType(argType.storedType())
                        ->augmentedInternalName());
        } else {
            result.argumentTypes.append(u"void"_s);
        }
    }

    if (function->returnType) {
        result.returnType = function->returnType->internalName();
        if (function->returnType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            result.returnType += u'*';
    } else {
        result.returnType = u"void"_s;
    }

    return result;
}

QString QQmlJSCodeGenerator::errorReturnValue()
{
    if (auto ret = m_function->returnType) {
        return ret->accessSemantics() == QQmlJSScope::AccessSemantics::Reference
                ? convertStored(m_typeResolver->nullType(), ret, QString())
                : ret->internalName() + u"()"_s;
    }
    return QString();
}

void QQmlJSCodeGenerator::generate_Ret()
{
    INJECT_TRACE_INFO(generate_Ret);

    if (m_function->returnType) {
        const QString signalUndefined = u"aotContext->setReturnValueUndefined();\n"_s;
        if (!m_state.accumulatorVariableIn.isEmpty()) {
            const QString in = m_state.accumulatorVariableIn;
            if (m_typeResolver->registerIsStoredIn(
                        m_state.accumulatorIn(), m_typeResolver->varType())) {
                m_body += u"if (!"_s + in + u".isValid())\n"_s;
                m_body += u"    "_s + signalUndefined;
            } else if (m_typeResolver->registerIsStoredIn(
                           m_state.accumulatorIn(), m_typeResolver->jsPrimitiveType())) {
                m_body += u"if ("_s + in
                        + u".type() == QJSPrimitiveValue::Undefined)\n"_s;
                m_body += u"    "_s + signalUndefined;
            } else if (m_typeResolver->registerIsStoredIn(
                           m_state.accumulatorIn(), m_typeResolver->jsValueType())) {
                m_body += u"if ("_s + in + u".isUndefined())\n"_s;
                m_body += u"    "_s + signalUndefined;
            }
            m_body += u"return "_s
                    + convertStored(m_state.accumulatorIn().storedType(), m_function->returnType, in);
        } else {
            if (m_typeResolver->equals(m_state.accumulatorIn().storedType(),
                                       m_typeResolver->voidType())) {
                m_body += signalUndefined;
            }
            m_body += u"return "_s + convertStored(
                        m_state.accumulatorIn().storedType(), m_function->returnType, QString());
        }
    } else {
        m_body += u"return"_s;
    }

    m_body += u";\n"_s;
    m_skipUntilNextLabel = true;
    resetState();
}

void QQmlJSCodeGenerator::generate_Debug()
{
    BYTECODE_UNIMPLEMENTED();
}

static QString toNumericString(double value)
{
    if (value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max()) {
        const int i = value;
        if (i == value)
            return QString::number(i);
    }

    switch (qFpClassify(value)) {
    case FP_INFINITE: {
        const QString inf = u"std::numeric_limits<double>::infinity()"_s;
        return std::signbit(value) ? (u'-' + inf) : inf;
    }
    case FP_NAN:
        return u"std::numeric_limits<double>::quiet_NaN()"_s;
    case FP_ZERO:
        return std::signbit(value) ? u"-0.0"_s : u"0"_s;
    default:
        break;
    }

    return QString::number(value, 'f', std::numeric_limits<double>::max_digits10);
}

void QQmlJSCodeGenerator::generate_LoadConst(int index)
{
    INJECT_TRACE_INFO(generate_LoadConst);

    // You cannot actually get it to generate LoadConst for anything but double. We have
    // a numer of specialized instructions for the other types, after all. However, let's
    // play it safe.

    const QV4::ReturnedValue encodedConst = m_jsUnitGenerator->constant(index);
    const QV4::StaticValue value = QV4::StaticValue::fromReturnedValue(encodedConst);
    const QQmlJSScope::ConstPtr type = m_typeResolver->typeForConst(encodedConst);

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    if (type == m_typeResolver->realType()) {
        m_body += conversion(
                    type, m_state.accumulatorOut(),
                    toNumericString(value.doubleValue()));
    } else if (type == m_typeResolver->int32Type()) {
        m_body += conversion(
                    type, m_state.accumulatorOut(),
                    QString::number(value.integerValue()));
    } else if (type == m_typeResolver->boolType()) {
        m_body += conversion(
                    type, m_state.accumulatorOut(),
                    value.booleanValue() ? u"true"_s : u"false"_s);
    } else if (type == m_typeResolver->voidType()) {
        m_body += conversion(
                    type, m_state.accumulatorOut(),
                    QString());
    } else if (type == m_typeResolver->nullType()) {
        m_body += conversion(
                    type, m_state.accumulatorOut(),
                    u"nullptr"_s);
    } else {
        reject(u"unsupported constant type"_s);
    }

    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadZero()
{
    INJECT_TRACE_INFO(generate_LoadZero);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s + conversion(
                m_typeResolver->int32Type(), m_state.accumulatorOut(), u"0"_s);
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadTrue()
{
    INJECT_TRACE_INFO(generate_LoadTrue);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s + conversion(
                m_typeResolver->boolType(), m_state.accumulatorOut(), u"true"_s);
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadFalse()
{
    INJECT_TRACE_INFO(generate_LoadFalse);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s + conversion(
                m_typeResolver->boolType(), m_state.accumulatorOut(), u"false"_s);
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadNull()
{
    INJECT_TRACE_INFO(generate_LoadNull);

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += conversion(m_typeResolver->nullType(), m_state.accumulatorOut(),
                         u"nullptr"_s);
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadUndefined()
{
    INJECT_TRACE_INFO(generate_LoadUndefined);

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += conversion(m_typeResolver->voidType(), m_state.accumulatorOut(),
                         QString());
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadInt(int value)
{
    INJECT_TRACE_INFO(generate_LoadInt);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s;
    m_body += conversion(m_typeResolver->int32Type(), m_state.accumulatorOut(),
                         QString::number(value));
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_MoveConst(int constIndex, int destTemp)
{
    INJECT_TRACE_INFO(generate_MoveConst);

    Q_ASSERT(destTemp == m_state.changedRegisterIndex());

    auto var = changedRegisterVariable();
    if (var.isEmpty())
        return; // Do not load 'undefined'

    const auto v4Value = QV4::StaticValue::fromReturnedValue(
                m_jsUnitGenerator->constant(constIndex));

    const auto changed = m_state.changedRegister();
    QQmlJSScope::ConstPtr contained;
    QString input;

    m_body += var + u" = "_s;
    if (v4Value.isNull()) {
        contained = m_typeResolver->nullType();
    } else if (v4Value.isUndefined()) {
        contained = m_typeResolver->voidType();
    } else if (v4Value.isBoolean()) {
        contained = m_typeResolver->boolType();
        input = v4Value.booleanValue() ? u"true"_s : u"false"_s;
    } else if (v4Value.isInteger()) {
        contained = m_typeResolver->int32Type();
        input = QString::number(v4Value.int_32());
    } else if (v4Value.isDouble()) {
        contained = m_typeResolver->realType();
        input = toNumericString(v4Value.doubleValue());
    } else {
        reject(u"unknown const type"_s);
    }
    m_body += conversion(contained, changed, input) + u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadReg(int reg)
{
    INJECT_TRACE_INFO(generate_LoadReg);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s;
    m_body += conversion(
                registerType(reg), m_state.accumulatorOut(), consumedRegisterVariable(reg));
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_StoreReg(int reg)
{
    INJECT_TRACE_INFO(generate_StoreReg);

    Q_ASSERT(m_state.changedRegisterIndex() == reg);
    Q_ASSERT(m_state.accumulatorIn().isValid());
    const QString var = changedRegisterVariable();
    if (var.isEmpty())
        return; // don't store "undefined"
    m_body += var;
    m_body += u" = "_s;
    m_body += conversion(m_state.accumulatorIn(), m_state.changedRegister(),
                         consumedAccumulatorVariableIn());
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_MoveReg(int srcReg, int destReg)
{
    INJECT_TRACE_INFO(generate_MoveReg);

    Q_ASSERT(m_state.changedRegisterIndex() == destReg);
    const QString destRegName = changedRegisterVariable();
    if (destRegName.isEmpty())
        return; // don't store things we cannot store.
    m_body += destRegName;
    m_body += u" = "_s;
    m_body += conversion(
                registerType(srcReg), m_state.changedRegister(), consumedRegisterVariable(srcReg));
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadImport(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadLocal(int index)
{
    Q_UNUSED(index);
    reject(u"LoadLocal"_s);
}

void QQmlJSCodeGenerator::generate_StoreLocal(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadScopedLocal(int scope, int index)
{
    Q_UNUSED(scope)
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_StoreScopedLocal(int scope, int index)
{
    Q_UNUSED(scope)
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadRuntimeString(int stringId)
{
    INJECT_TRACE_INFO(generate_LoadRuntimeString);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s;
    m_body += conversion(m_typeResolver->stringType(), m_state.accumulatorOut(),
                         QQmlJSUtils::toLiteral(m_jsUnitGenerator->stringForIndex(stringId)));
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_MoveRegExp(int regExpId, int destReg)
{
    Q_UNUSED(regExpId)
    Q_UNUSED(destReg)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadClosure(int value)
{
    Q_UNUSED(value)
    reject(u"LoadClosure"_s);
}

void QQmlJSCodeGenerator::generate_LoadName(int nameIndex)
{
    Q_UNUSED(nameIndex)
    reject(u"LoadName"_s);
}

void QQmlJSCodeGenerator::generate_LoadGlobalLookup(int index)
{
    INJECT_TRACE_INFO(generate_LoadGlobalLookup);

    AccumulatorConverter registers(this);

    const QString lookup = u"aotContext->loadGlobalLookup("_s + QString::number(index)
            + u", &"_s + m_state.accumulatorVariableOut + u", "_s
            + metaTypeFromType(m_state.accumulatorOut().storedType()) + u')';
    const QString initialization = u"aotContext->initLoadGlobalLookup("_s
            + QString::number(index) + u')';
    generateLookup(lookup, initialization);
}

void QQmlJSCodeGenerator::generate_LoadQmlContextPropertyLookup(int index)
{
    INJECT_TRACE_INFO(generate_LoadQmlContextPropertyLookup);

    AccumulatorConverter registers(this);

    const int nameIndex = m_jsUnitGenerator->lookupNameIndex(index);
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::JavaScriptGlobal) {
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(
                    m_typeResolver->original(m_state.accumulatorOut()), m_state.accumulatorOut(),
                    u"aotContext->javaScriptGlobalProperty("_s + QString::number(nameIndex) + u")")
                + u";\n"_s;
        return;
    }

    const QString indexString = QString::number(index);
    if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::ObjectById) {
        const QString lookup = u"aotContext->loadContextIdLookup("_s
                + indexString + u", "_s
                + contentPointer(m_state.accumulatorOut(), m_state.accumulatorVariableOut) + u')';
        const QString initialization = u"aotContext->initLoadContextIdLookup("_s
                + indexString + u')';
        generateLookup(lookup, initialization);
        return;
    }

    const bool isProperty = m_state.accumulatorOut().isProperty();
    const QQmlJSScope::ConstPtr scope = m_state.accumulatorOut().scopeType();
    const QQmlJSScope::ConstPtr stored = m_state.accumulatorOut().storedType();
    if (isProperty) {
        const auto lookupType = contentType(m_state.accumulatorOut(), m_state.accumulatorVariableOut);

        const QString lookup = u"aotContext->loadScopeObjectPropertyLookup("_s
                + indexString + u", "_s
                + contentPointer(m_state.accumulatorOut(), m_state.accumulatorVariableOut) + u')';
        const QString initialization
                = u"aotContext->initLoadScopeObjectPropertyLookup("_s
                + indexString + u", "_s
                + lookupType + u')';
        const QString preparation = getLookupPreparation(
                    m_state.accumulatorOut(), m_state.accumulatorVariableOut, index);

        generateLookup(lookup, initialization, preparation);
    } else if (m_state.accumulatorOut().isType() || m_state.accumulatorOut().isImportNamespace()) {
        generateTypeLookup(index);
    } else {
        reject(u"lookup of %1"_s.arg(m_state.accumulatorOut().descriptiveName()));
    }
}

void QQmlJSCodeGenerator::generate_StoreNameSloppy(int nameIndex)
{
    INJECT_TRACE_INFO(generate_StoreNameSloppy);

    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    const QQmlJSRegisterContent type = m_typeResolver->scopedType(m_function->qmlScope, name);
    Q_ASSERT(type.isProperty());

    switch (type.variant()) {
    case QQmlJSRegisterContent::ScopeProperty:
    case QQmlJSRegisterContent::ExtensionScopeProperty: {
        // Do not convert here. We may intentionally pass the "wrong" type, for example to trigger
        // a property reset.
        m_body += u"aotContext->storeNameSloppy("_s + QString::number(nameIndex)
                + u", "_s
                + contentPointer(m_state.accumulatorIn(), m_state.accumulatorVariableIn)
                + u", "_s
                + contentType(m_state.accumulatorIn(), m_state.accumulatorVariableIn) + u')';
        m_body += u";\n"_s;
        break;
    }
    case QQmlJSRegisterContent::ScopeMethod:
    case QQmlJSRegisterContent::ExtensionScopeMethod:
        reject(u"assignment to scope method"_s);
        break;
    default:
        Q_UNREACHABLE();
    }
}

void QQmlJSCodeGenerator::generate_StoreNameStrict(int name)
{
    Q_UNUSED(name)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadElement(int base)
{
    INJECT_TRACE_INFO(generate_LoadElement);

    const QQmlJSRegisterContent baseType = registerType(base);

    if (!m_typeResolver->isNumeric(m_state.accumulatorIn())
            || (!baseType.isList()
                && !m_typeResolver->registerIsStoredIn(baseType, m_typeResolver->stringType()))) {
        reject(u"LoadElement with non-list base type or non-numeric arguments"_s);
        return;
    }

    AccumulatorConverter registers(this);

    const QString baseName = registerVariable(base);
    const QString indexName = m_state.accumulatorVariableIn;

    const QString voidAssignment = u"    "_s + m_state.accumulatorVariableOut + u" = "_s +
            conversion(m_typeResolver->globalType(m_typeResolver->voidType()),
                       m_state.accumulatorOut(), QString()) + u";\n"_s;

    if (!m_typeResolver->isIntegral(m_state.accumulatorIn())) {
        m_body += u"if (!QJSNumberCoercion::isArrayIndex("_s + indexName + u"))\n"_s
                + voidAssignment
                + u"else "_s;
    }

    if (!m_typeResolver->isUnsignedInteger(
                m_typeResolver->containedType(m_state.accumulatorIn()))) {
        m_body += u"if ("_s + indexName + u" < 0)\n"_s
                + voidAssignment
                + u"else "_s;
    }

    if (m_typeResolver->registerIsStoredIn(baseType, m_typeResolver->listPropertyType())) {
        // Our QQmlListProperty only keeps plain QObject*.
        const auto elementType = m_typeResolver->globalType(m_typeResolver->qObjectType());

        m_body += u"if ("_s + indexName + u" < "_s + baseName
                + u".count(&"_s + baseName + u"))\n"_s;
        m_body += u"    "_s + m_state.accumulatorVariableOut + u" = "_s +
                conversion(elementType, m_state.accumulatorOut(),
                           baseName + u".at(&"_s + baseName + u", "_s
                           + indexName + u')') + u";\n"_s;
        m_body += u"else\n"_s
                + voidAssignment;
        return;
    }

    const auto elementType = m_typeResolver->valueType(baseType);

    QString access = baseName + u".at("_s + indexName + u')';

    // TODO: Once we get a char type in QML, use it here.
    if (m_typeResolver->registerIsStoredIn(baseType, m_typeResolver->stringType()))
        access = u"QString("_s + access + u")"_s;
    else if (m_state.isRegisterAffectedBySideEffects(base))
        reject(u"LoadElement on a sequence potentially affected by side effects"_s);
    else if (baseType.storedType()->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence)
        reject(u"LoadElement on a sequence wrapped in a non-sequence type"_s);

    m_body += u"if ("_s + indexName + u" < "_s + baseName + u".size())\n"_s;
    m_body += u"    "_s + m_state.accumulatorVariableOut + u" = "_s +
            conversion(elementType, m_state.accumulatorOut(), access) + u";\n"_s;
    m_body += u"else\n"_s
            + voidAssignment;
}

void QQmlJSCodeGenerator::generate_StoreElement(int base, int index)
{
    INJECT_TRACE_INFO(generate_StoreElement);

    const QQmlJSRegisterContent baseType = registerType(base);
    const QQmlJSRegisterContent indexType = registerType(index);

    if (!m_typeResolver->isNumeric(registerType(index)) || !baseType.isList()) {
        reject(u"StoreElement with non-list base type or non-numeric arguments"_s);
        return;
    }

    if (!m_typeResolver->registerIsStoredIn(baseType, m_typeResolver->listPropertyType())) {
        reject(u"indirect StoreElement"_s);
        return;
    }

    const QString baseName = registerVariable(base);
    const QString indexName = registerVariable(index);

    const auto valueType = m_typeResolver->valueType(baseType);
    const auto elementType = m_typeResolver->globalType(m_typeResolver->genericType(
                                          m_typeResolver->containedType(valueType)));

    m_body += u"if ("_s;
    if (!m_typeResolver->isIntegral(indexType))
        m_body += u"QJSNumberCoercion::isArrayIndex("_s + indexName + u") && "_s;
    if (!m_typeResolver->isUnsignedInteger(m_typeResolver->containedType(indexType)))
        m_body += indexName + u" >= 0 && "_s;
    m_body += indexName + u" < "_s + baseName + u".count(&"_s + baseName
            + u"))\n"_s;
    m_body += u"    "_s + baseName + u".replace(&"_s + baseName
            + u", "_s + indexName + u", "_s;
    m_body += conversion(m_state.accumulatorIn(), elementType, m_state.accumulatorVariableIn)
            + u");\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadProperty(int nameIndex)
{
    Q_UNUSED(nameIndex)
    reject(u"LoadProperty"_s);
}

void QQmlJSCodeGenerator::generate_LoadOptionalProperty(int name, int offset)
{
    Q_UNUSED(name)
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generateEnumLookup(int index)
{
    const QString enumMember = m_state.accumulatorOut().enumMember();

    // If we're referring to the type, there's nothing to do.
    if (enumMember.isEmpty())
        return;

    // If the metaenum has the value, just use it and skip all the rest.
    const QQmlJSMetaEnum metaEnum = m_state.accumulatorOut().enumeration();
    if (metaEnum.hasValues()) {
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + QString::number(metaEnum.value(enumMember));
        m_body += u";\n"_s;
        return;
    }

    const QQmlJSScope::ConstPtr scopeType = m_state.accumulatorOut().scopeType();

    // Otherwise we would have found an enum with values.
    Q_ASSERT(!scopeType->isComposite());

    const QString enumName = metaEnum.isFlag() ? metaEnum.alias() : metaEnum.name();
    if (enumName.isEmpty()) {
        if (metaEnum.isFlag() && !metaEnum.name().isEmpty())
            reject(u"qmltypes misses name entry for flag; did you pass the enum type to Q_FLAG instead of the QFlag type?"
                   "\nType is %1, enum name is %2"_s.arg(scopeType->internalName(), metaEnum.name()));
        reject(u"qmltypes misses name entry for enum"_s);
    }
    const QString lookup = u"aotContext->getEnumLookup("_s + QString::number(index)
            + u", &"_s + m_state.accumulatorVariableOut + u')';
    const QString initialization = u"aotContext->initGetEnumLookup("_s
            + QString::number(index) + u", "_s + metaObject(scopeType)
            + u", \""_s + enumName + u"\", \""_s + enumMember
            + u"\")"_s;
    generateLookup(lookup, initialization);
}

void QQmlJSCodeGenerator::generateTypeLookup(int index)
{
    const QString indexString = QString::number(index);
    const QQmlJSRegisterContent accumulatorIn = m_state.registers.value(Accumulator).content;
    const QString namespaceString
            = accumulatorIn.isImportNamespace()
                ? QString::number(accumulatorIn.importNamespace())
                : u"QQmlPrivate::AOTCompiledContext::InvalidStringId"_s;

    switch (m_state.accumulatorOut().variant()) {
    case QQmlJSRegisterContent::Singleton: {
        rejectIfNonQObjectOut(u"non-QObject singleton type"_s);
        const QString lookup = u"aotContext->loadSingletonLookup("_s + indexString
                + u", &"_s + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadSingletonLookup("_s + indexString
                + u", "_s + namespaceString + u')';
        generateLookup(lookup, initialization);
        break;
    }
    case QQmlJSRegisterContent::ScopeModulePrefix:
        break;
    case QQmlJSRegisterContent::ScopeAttached: {
        rejectIfNonQObjectOut(u"non-QObject attached type"_s);
        const QString lookup = u"aotContext->loadAttachedLookup("_s + indexString
                + u", aotContext->qmlScopeObject, &"_s + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadAttachedLookup("_s + indexString
                + u", "_s + namespaceString + u", aotContext->qmlScopeObject)"_s;
        generateLookup(lookup, initialization);
        break;
    }
    case QQmlJSRegisterContent::Script:
        reject(u"script lookup"_s);
        break;
    case QQmlJSRegisterContent::MetaType: {
        if (!m_typeResolver->registerIsStoredIn(
                    m_state.accumulatorOut(), m_typeResolver->metaObjectType())) {
            // TODO: Can we trigger this somehow?
            //       It might be impossible, but we better be safe here.
            reject(u"meta-object stored in different type"_s);
        }
        const QString lookup = u"aotContext->loadTypeLookup("_s + indexString
                + u", &"_s + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadTypeLookup("_s + indexString
                + u", "_s + namespaceString + u")"_s;
        generateLookup(lookup, initialization);
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

void QQmlJSCodeGenerator::generateVariantEqualityComparison(
        const QQmlJSRegisterContent &nonStorableContent, const QString &registerName, bool invert)
{
    const auto nonStorableType = m_typeResolver->containedType(nonStorableContent);
    QQmlJSScope::ConstPtr comparedType =
            m_typeResolver->equals(nonStorableType, m_typeResolver->nullType())
            ? m_typeResolver->nullType()
            : m_typeResolver->voidType();

    // The common operations for both nulltype and voidtype
    m_body += u"if ("_s + registerName
            + u".metaType() == QMetaType::fromType<QJSPrimitiveValue>()) {\n"_s
            + m_state.accumulatorVariableOut + u" = "_s
            + conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                         u"static_cast<const QJSPrimitiveValue *>("_s + registerName
                                 + u".constData())"_s + u"->type() "_s
                                 + (invert ? u"!="_s : u"=="_s)
                                 + (m_typeResolver->equals(comparedType, m_typeResolver->nullType())
                                            ? u"QJSPrimitiveValue::Null"_s
                                            : u"QJSPrimitiveValue::Undefined"_s))
            + u";\n} else if ("_s + registerName
            + u".metaType() == QMetaType::fromType<QJSValue>()) {\n"_s
            + m_state.accumulatorVariableOut + u" = "_s
            + conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                         (invert ? u"!"_s : QString()) + u"static_cast<const QJSValue *>("_s
                                 + registerName + u".constData())"_s + u"->"_s
                                 + (m_typeResolver->equals(comparedType, m_typeResolver->nullType())
                                            ? u"isNull()"_s
                                            : u"isUndefined()"_s))
            + u";\n}"_s;

    // Generate nullType specific operations (the case when variant contains QObject * or
    // std::nullptr_t)
    if (m_typeResolver->equals(nonStorableType, m_typeResolver->nullType())) {
        m_body += u"else if ("_s + registerName
                + u".metaType().flags().testFlag(QMetaType::PointerToQObject)) {\n"_s
                + m_state.accumulatorVariableOut + u" = "_s
                + conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                             u"*static_cast<QObject *const *>("_s + registerName
                                     + u".constData())"_s + (invert ? u"!="_s : u"=="_s)
                                     + u" nullptr"_s)
                + u";\n} else if ("_s + registerName
                + u".metaType() == QMetaType::fromType<std::nullptr_t>()) {\n"_s
                + m_state.accumulatorVariableOut + u" = "_s
                + conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                             (invert ? u"false"_s : u"true"_s))
                + u";\n}\n"_s;
    }

    // fallback case (if variant contains a different type, then it is not null or undefined)
    m_body += u"else {\n"_s + m_state.accumulatorVariableOut + u" = "_s
            + conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                         (invert ? (registerName + u".isValid() ? true : false"_s)
                                 : (registerName + u".isValid() ? false : true"_s)))
            + u";\n}"_s;
}

void QQmlJSCodeGenerator::rejectIfNonQObjectOut(const QString &error)
{
    if (m_state.accumulatorOut().storedType()->accessSemantics()
        != QQmlJSScope::AccessSemantics::Reference) {
        reject(error);
    }
}

/*!
 * \internal
 *
 * generates a check for the content pointer to be valid.
 * Returns true if the content pointer needs to be retrieved from a QVariant, or
 * false if the variable can be used as-is.
 */
bool QQmlJSCodeGenerator::generateContentPointerCheck(
        const QQmlJSScope::ConstPtr &required, const QQmlJSRegisterContent &actual,
        const QString &variable, const QString &errorMessage)
{
    const QQmlJSScope::ConstPtr scope = required;
    const QQmlJSScope::ConstPtr input = m_typeResolver->containedType(actual);
    if (QQmlJSUtils::searchBaseAndExtensionTypes(input,
            [&](const QQmlJSScope::ConstPtr &base) {
                return m_typeResolver->equals(base, scope);
            })) {
        return false;
    }

    if (!m_typeResolver->canHold(input, scope)) {
        reject(u"lookup of members of %1 in %2"_s.arg(
                   scope->internalName(), input->internalName()));
    }

    bool needsVarContentConversion = false;
    QString processedErrorMessage;
    if (actual.storedType()->isReferenceType()) {
        // Since we have verified the type in qqmljstypepropagator.cpp we now know
        // that we can only have either null or the actual type here. Therefore,
        // it's enough to check the pointer for null.
        m_body += u"if ("_s + variable + u" == nullptr) {\n    "_s;
        processedErrorMessage = errorMessage.arg(u"null");
    } else if (m_typeResolver->equals(actual.storedType(), m_typeResolver->varType())) {
        // Since we have verified the type in qqmljstypepropagator.cpp we now know
        // that we can only have either undefined or the actual type here. Therefore,
        // it's enough to check the QVariant for isValid().
        m_body += u"if (!"_s + variable + u".isValid()) {\n    "_s;
        needsVarContentConversion = true;
        processedErrorMessage = errorMessage.arg(u"undefined");
    } else {
        reject(u"retrieving metatype from %1"_s.arg(actual.descriptiveName()));
    }

    generateSetInstructionPointer();
    m_body += u"    aotContext->engine->throwError(QJSValue::TypeError, "_s;
    m_body += u"QLatin1String(\"%1\"));\n"_s.arg(processedErrorMessage);
    m_body += u"    return "_s + errorReturnValue() + u";\n"_s;
    m_body += u"}\n"_s;
    return needsVarContentConversion;
}

QString QQmlJSCodeGenerator::resolveValueTypeContentPointer(
        const QQmlJSScope::ConstPtr &required, const QQmlJSRegisterContent &actual,
        const QString &variable, const QString &errorMessage)
{
    if (generateContentPointerCheck(required, actual, variable, errorMessage))
        return variable + u".data()"_s;
    return contentPointer(actual, variable);
}

QString QQmlJSCodeGenerator::resolveQObjectPointer(
        const QQmlJSScope::ConstPtr &required, const QQmlJSRegisterContent &actual,
        const QString &variable, const QString &errorMessage)
{
    if (generateContentPointerCheck(required, actual, variable, errorMessage))
        return u"*static_cast<QObject *const *>("_s + variable + u".constData())"_s;
    return variable;
}

void QQmlJSCodeGenerator::generate_GetLookup(int index)
{
    INJECT_TRACE_INFO(generate_GetLookup);

    if (m_state.accumulatorOut().isMethod()) {
        reject(u"lookup of function property."_s);
        return;
    }

    if (m_state.accumulatorOut().isImportNamespace()) {
        Q_ASSERT(m_state.accumulatorOut().variant() == QQmlJSRegisterContent::ObjectModulePrefix);
        // If we have an object module prefix, we need to pass through the original object.
        if (m_state.accumulatorVariableIn != m_state.accumulatorVariableOut) {
            m_body += m_state.accumulatorVariableOut + u" = "_s
                    + conversion(m_state.accumulatorIn(), m_state.accumulatorOut(),
                                 m_state.accumulatorVariableIn)
                    + u";\n"_s;
        }
        return;
    }

    AccumulatorConverter registers(this);

    if (m_state.accumulatorOut().isEnumeration()) {
        generateEnumLookup(index);
        return;
    }

    const QString indexString = QString::number(index);
    const QString namespaceString = m_state.accumulatorIn().isImportNamespace()
            ? QString::number(m_state.accumulatorIn().importNamespace())
            : u"QQmlPrivate::AOTCompiledContext::InvalidStringId"_s;
    const auto accumulatorIn = m_state.accumulatorIn();
    const QQmlJSScope::ConstPtr scope = m_state.accumulatorOut().scopeType();
    const bool isReferenceType = scope->isReferenceType();

    switch (m_state.accumulatorOut().variant()) {
    case QQmlJSRegisterContent::ObjectAttached: {
        if (!isReferenceType) {
            // This can happen on incomplete type information. We contextually know that the
            // type must be a QObject, but we cannot construct the inheritance chain. Then we
            // store it in a generic type. Technically we could even convert it to QObject*, but
            // that would be expensive.
            reject(u"attached object for non-QObject type"_s);
        }

        if (!m_state.accumulatorIn().storedType()->isReferenceType()) {
            // This can happen if we retroactively determine that the property might not be
            // what we think it is (ie, it can be shadowed).
            reject(u"attached object of potentially non-QObject base"_s);
        }

        rejectIfNonQObjectOut(u"non-QObject attached type"_s);

        const QString lookup = u"aotContext->loadAttachedLookup("_s + indexString
                + u", "_s + m_state.accumulatorVariableIn
                + u", &"_s + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadAttachedLookup("_s
                + indexString + u", "_s + namespaceString + u", "_s
                + m_state.accumulatorVariableIn + u')';
        generateLookup(lookup, initialization);
        return;
    }
    case QQmlJSRegisterContent::ScopeAttached:
    case QQmlJSRegisterContent::Singleton:
    case QQmlJSRegisterContent::MetaType: {
        generateTypeLookup(index);
        return;
    }
    default:
        break;
    }

    Q_ASSERT(m_state.accumulatorOut().isProperty());

    if (m_typeResolver->registerIsStoredIn(accumulatorIn, m_typeResolver->jsValueType())) {
        reject(u"lookup in QJSValue"_s);
    } else if (isReferenceType) {
        const QString inputPointer = resolveQObjectPointer(
                    scope, accumulatorIn, m_state.accumulatorVariableIn,
                    u"Cannot read property '%1' of %2"_s.arg(
                        m_jsUnitGenerator->lookupName(index)));
        const QString lookup = u"aotContext->getObjectLookup("_s + indexString
                + u", "_s + inputPointer + u", "_s
                + contentPointer(m_state.accumulatorOut(), m_state.accumulatorVariableOut) + u')';
        const QString initialization = u"aotContext->initGetObjectLookup("_s
                + indexString + u", "_s + inputPointer
                + u", "_s + contentType(m_state.accumulatorOut(), m_state.accumulatorVariableOut)
                + u')';
        const QString preparation = getLookupPreparation(
                    m_state.accumulatorOut(), m_state.accumulatorVariableOut, index);
        generateLookup(lookup, initialization, preparation);
    } else if ((scope->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence
                || m_typeResolver->equals(scope, m_typeResolver->stringType()))
               && m_jsUnitGenerator->lookupName(index) == u"length"_s) {
        const QQmlJSScope::ConstPtr stored = accumulatorIn.storedType();
        if (stored->isListProperty()) {
            m_body += m_state.accumulatorVariableOut + u" = "_s;
            m_body += conversion(
                        m_typeResolver->globalType(m_typeResolver->int32Type()),
                        m_state.accumulatorOut(),
                        m_state.accumulatorVariableIn + u".count("_s + u'&'
                            + m_state.accumulatorVariableIn + u')');
            m_body += u";\n"_s;
        } else if (stored->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence
                   || m_typeResolver->equals(stored, m_typeResolver->stringType())) {
            m_body += m_state.accumulatorVariableOut + u" = "_s
                    + conversion(m_typeResolver->globalType(m_typeResolver->int32Type()),
                                 m_state.accumulatorOut(),
                                 m_state.accumulatorVariableIn + u".length()"_s)
                    + u";\n"_s;
        } else {
            reject(u"access to 'length' property of sequence wrapped in non-sequence"_s);
        }
    } else if (m_typeResolver->registerIsStoredIn(accumulatorIn,
                                                  m_typeResolver->variantMapType())) {
        QString mapLookup = m_state.accumulatorVariableIn + u"["_s
                + QQmlJSUtils::toLiteral(m_jsUnitGenerator->lookupName(index)) + u"]"_s;
        m_body += m_state.accumulatorVariableOut + u" = "_s;
        m_body += conversion(m_typeResolver->globalType(m_typeResolver->varType()),
                             m_state.accumulatorOut(), mapLookup);
        m_body += u";\n"_s;
    } else {
        if (m_state.isRegisterAffectedBySideEffects(Accumulator))
            reject(u"reading from a value that's potentially affected by side effects"_s);

        const QString inputContentPointer = resolveValueTypeContentPointer(
                    scope, accumulatorIn, m_state.accumulatorVariableIn,
                    u"Cannot read property '%1' of %2"_s.arg(
                        m_jsUnitGenerator->lookupName(index)));

        const QString lookup = u"aotContext->getValueLookup("_s + indexString
                + u", "_s + inputContentPointer
                + u", "_s + contentPointer(m_state.accumulatorOut(), m_state.accumulatorVariableOut)
                + u')';
        const QString initialization = u"aotContext->initGetValueLookup("_s
                + indexString + u", "_s
                + metaObject(scope) + u", "_s
                + contentType(m_state.accumulatorOut(), m_state.accumulatorVariableOut) + u')';
        const QString preparation = getLookupPreparation(
                    m_state.accumulatorOut(), m_state.accumulatorVariableOut, index);
        generateLookup(lookup, initialization, preparation);
    }
}

void QQmlJSCodeGenerator::generate_GetOptionalLookup(int index, int offset)
{
    Q_UNUSED(index)
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_StoreProperty(int nameIndex, int baseReg)
{
    Q_UNUSED(nameIndex)
    Q_UNUSED(baseReg)
    reject(u"StoreProperty"_s);
}

QString QQmlJSCodeGenerator::setLookupPreparation(
        const QQmlJSRegisterContent &content, const QString &arg, int lookup)
{
    if (m_typeResolver->registerContains(content, content.storedType()))
        return QString();

    if (m_typeResolver->registerIsStoredIn(content, m_typeResolver->varType())) {
        return u"const QMetaType argType = aotContext->lookupResultMetaType("_s
                + QString::number(lookup) + u");\n"_s
                + u"if (argType.isValid())\n    "_s + arg + u".convert(argType)";
    }
    // TODO: We could make sure they're compatible, for example QObject pointers.
    return QString();
}


void QQmlJSCodeGenerator::generate_SetLookup(int index, int baseReg)
{
    INJECT_TRACE_INFO(generate_SetLookup);

    const QString indexString = QString::number(index);
    const QQmlJSScope::ConstPtr valueType = m_state.accumulatorIn().storedType();
    const QQmlJSRegisterContent callBase = m_typeResolver->original(registerType(baseReg));
    const QQmlJSRegisterContent specific = m_state.readAccumulator();
    Q_ASSERT(specific.isConversion());
    const QQmlJSScope::ConstPtr conversionResultScope = specific.conversionResultScope();

    if (specific.storedType().isNull()) {
        reject(u"SetLookup. Could not find property "
               + m_jsUnitGenerator->lookupName(index)
               + u" on type "
               + callBase.storedType()->internalName());
        return;
    }

    // Choose a container that can hold both, the "in" accumulator and what we actually want.
    // If the types are all the same because we can all store them as verbatim C++ types,
    // the container will also be that type.
    const QQmlJSScope::ConstPtr container = m_typeResolver->merge(specific.storedType(), valueType);
    const QQmlJSRegisterContent property = specific.storedIn(container);

    const QString object = registerVariable(baseReg);
    m_body += u"{\n"_s;
    QString variableIn;
    QString variableInType;
    QString preparation;
    QString argType;
    if (!m_typeResolver->registerContains(
                m_state.accumulatorIn(), m_typeResolver->containedType(property))) {
        m_body += u"auto converted = "_s
                + conversion(m_state.accumulatorIn(), property, consumedAccumulatorVariableIn())
                + u";\n"_s;
        variableIn = contentPointer(property, u"converted"_s);
        variableInType = contentType(property, u"converted"_s);
        preparation = setLookupPreparation(property, u"converted"_s, index);
        if (preparation.isEmpty())
            argType = contentType(property, u"converted"_s);
        else
            argType = u"argType"_s;
    } else {
        variableIn = contentPointer(property, m_state.accumulatorVariableIn);
        variableInType = contentType(property, m_state.accumulatorVariableIn);
        argType = variableInType;
    }

    switch (conversionResultScope->accessSemantics()) {
    case QQmlJSScope::AccessSemantics::Reference: {
        const QString basePointer = resolveQObjectPointer(
                    conversionResultScope, registerType(baseReg), object,
                    u"TypeError: Value is %1 and could not be converted to an object"_s);

        const QString lookup = u"aotContext->setObjectLookup("_s + indexString
                + u", "_s + basePointer + u", "_s + variableIn + u')';
        const QString initialization = u"aotContext->initSetObjectLookup("_s
                + indexString + u", "_s + basePointer + u", "_s + argType + u')';
        generateLookup(lookup, initialization, preparation);
        break;
    }
    case QQmlJSScope::AccessSemantics::Sequence: {
        const QString propertyName = m_jsUnitGenerator->lookupName(index);
        if (propertyName != u"length"_s) {
            reject(u"setting non-length property on a sequence type"_s);
            break;
        }

        if (!conversionResultScope->isListProperty()) {
            reject(u"resizing sequence types (because of missing write-back)"_s);
            break;
        }

        // We can resize without write back on a list property because it's actually a reference.
        m_body += u"const int begin = "_s + object + u".count(&" + object + u");\n"_s;
        m_body += u"const int end = "_s
                + (variableIn.startsWith(u'&') ? variableIn.mid(1) : (u'*' + variableIn))
                + u";\n"_s;
        m_body += u"for (int i = begin; i < end; ++i)\n"_s;
        m_body += u"    "_s + object + u".append(&"_s + object + u", nullptr);\n"_s;
        m_body += u"for (int i = begin; i > end; --i)\n"_s;
        m_body += u"    "_s + object + u".removeLast(&"_s + object + u')'
                + u";\n"_s;
        break;
    }
    case QQmlJSScope::AccessSemantics::Value: {
        const QString baseContentPointer = resolveValueTypeContentPointer(
                    conversionResultScope, registerType(baseReg), object,
                    u"TypeError: Value is %1 and could not be converted to an object"_s);

        const QString lookup = u"aotContext->setValueLookup("_s + indexString
                + u", "_s + baseContentPointer
                + u", "_s + variableIn + u')';
        const QString initialization = u"aotContext->initSetValueLookup("_s
                + indexString + u", "_s + metaObject(conversionResultScope)
                + u", "_s + argType + u')';

        generateLookup(lookup, initialization, preparation);
        reject(u"SetLookup on value types (because of missing write-back)"_s);
        break;
    }
    case QQmlJSScope::AccessSemantics::None:
        Q_UNREACHABLE();
        break;
    }

    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadSuperProperty(int property)
{
    Q_UNUSED(property)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_StoreSuperProperty(int property)
{
    Q_UNUSED(property)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Yield()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_YieldStar()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Resume(int)
{
    BYTECODE_UNIMPLEMENTED();
}

QString QQmlJSCodeGenerator::argumentsList(int argc, int argv, QString *outVar)
{
    QString types;
    QString args;

    if (m_state.changedRegisterIndex() == InvalidRegister ||
            m_typeResolver->registerContains(
                m_state.accumulatorOut(), m_typeResolver->voidType())) {
        types = u"QMetaType()"_s;
        args = u"nullptr"_s;
    } else {
        *outVar = u"callResult"_s;
        const QQmlJSScope::ConstPtr outType = m_state.accumulatorOut().storedType();
        m_body += outType->augmentedInternalName() + u' ' + *outVar;
        if (!m_typeResolver->registerContains(m_state.accumulatorOut(), outType)) {
             if (m_typeResolver->equals(outType, m_typeResolver->varType())
                || m_typeResolver->equals(outType, m_typeResolver->jsPrimitiveType())) {
                m_body += u'('
                        + metaType(m_typeResolver->containedType(m_state.accumulatorOut()))
                        + u')';
             }
        }
        m_body += u";\n";

        args = contentPointer(m_state.accumulatorOut(), *outVar);
        types = contentType(m_state.accumulatorOut(), *outVar);
    }

    for (int i = 0; i < argc; ++i) {
        const QQmlJSRegisterContent content = registerType(argv + i);
        const QString var = registerVariable(argv + i);
        args += u", "_s + contentPointer(content, var);
        types += u", "_s + contentType(content, var);
    }

    return u"void *args[] = { "_s + args + u" };\n"_s
            + u"const QMetaType types[] = { "_s + types + u" };\n"_s;
}

void QQmlJSCodeGenerator::generateMoveOutVar(const QString &outVar)
{
    if (m_state.accumulatorVariableOut.isEmpty() || outVar.isEmpty())
        return;

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += u"std::move(" + outVar + u");\n";
}

void QQmlJSCodeGenerator::generate_CallValue(int name, int argc, int argv)
{
    Q_UNUSED(name)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CallWithReceiver(int name, int thisObject, int argc, int argv)
{
    Q_UNUSED(name)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CallProperty(int nameIndex, int baseReg, int argc, int argv)
{
    Q_UNUSED(nameIndex);
    Q_UNUSED(baseReg);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    reject(u"CallProperty"_s);
}

bool QQmlJSCodeGenerator::inlineStringMethod(const QString &name, int base, int argc, int argv)
{
    if (name != u"arg"_s || argc != 1)
        return false;

    const auto arg = [&](const QQmlJSScope::ConstPtr &type) {
        return convertStored(registerType(argv).storedType(), type, consumedRegisterVariable(argv));
    };

    const auto ret = [&](const QString &arg) {
        const QString expression = convertStored(
                    registerType(base).storedType(), m_typeResolver->stringType(),
                    consumedRegisterVariable(base)) + u".arg("_s + arg + u')';
        return conversion(
                m_typeResolver->stringType(), m_state.accumulatorOut(), expression);
    };

    const QQmlJSRegisterContent input = m_state.readRegister(argv);
    m_body += m_state.accumulatorVariableOut + u" = "_s;

    if (m_typeResolver->isNumeric(input))
        m_body += ret(arg(m_typeResolver->containedType(input)));
    else if (m_typeResolver->registerContains(input, m_typeResolver->boolType()))
        m_body += ret(arg(m_typeResolver->boolType()));
    else
        m_body += ret(arg(m_typeResolver->stringType()));
    m_body += u";\n"_s;
    return true;
}

bool QQmlJSCodeGenerator::inlineTranslateMethod(const QString &name, int argc, int argv)
{
    addInclude(u"qcoreapplication.h"_s);

    const auto arg = [&](int i, const QQmlJSScope::ConstPtr &type) {
        Q_ASSERT(i < argc);
        return convertStored(registerType(argv + i).storedType(), type,
                             consumedRegisterVariable(argv + i));
    };

    const auto stringArg = [&](int i) {
        return i < argc
                ? (arg(i, m_typeResolver->stringType()) + u".toUtf8().constData()"_s)
                : u"\"\""_s;
    };

    const auto intArg = [&](int i) {
        return i < argc ? arg(i, m_typeResolver->int32Type()) : u"-1"_s;
    };

    const auto stringRet = [&](const QString &expression) {
        return conversion(
                m_typeResolver->stringType(), m_state.accumulatorOut(), expression);
    };

    const auto capture = [&]() {
        m_body += u"aotContext->captureTranslation();\n"_s;
    };

    if (name == u"QT_TRID_NOOP"_s || name == u"QT_TR_NOOP"_s) {
        Q_ASSERT(argc > 0);
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + stringRet(arg(0, m_typeResolver->stringType())) + u";\n"_s;
        return true;
    }

    if (name == u"QT_TRANSLATE_NOOP"_s) {
        Q_ASSERT(argc > 1);
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + stringRet(arg(1, m_typeResolver->stringType())) + u";\n"_s;
        return true;
    }

    if (name == u"qsTrId"_s) {
        capture();
        // We inline qtTrId() here because in the !QT_CONFIG(translation) case it's unavailable.
        // QCoreApplication::translate() is always available in some primitive form.
        // Also, this saves a function call.
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + stringRet(u"QCoreApplication::translate(nullptr, "_s + stringArg(0) +
                            u", nullptr, "_s + intArg(1) + u")"_s) + u";\n"_s;
        return true;
    }

    if (name == u"qsTr"_s) {
        capture();
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + stringRet(u"QCoreApplication::translate("_s
                            + u"aotContext->translationContext().toUtf8().constData(), "_s
                            + stringArg(0) + u", "_s + stringArg(1) + u", "_s
                            + intArg(2) + u")"_s) + u";\n"_s;
        return true;
    }

    if (name == u"qsTranslate"_s) {
        capture();
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + stringRet(u"QCoreApplication::translate("_s
                            + stringArg(0) + u", "_s + stringArg(1) + u", "_s
                            + stringArg(2) + u", "_s + intArg(3) + u")"_s) + u";\n"_s;
        return true;
    }

    return false;
}

static QString maxExpression(int argc)
{
    Q_ASSERT_X(argc >= 2, Q_FUNC_INFO, "max() expects at least two arguments.");

    QString expression =
            u"[&]() { \nauto  tmpMax = (qIsNull(arg2) && qIsNull(arg1) && std::copysign(1.0, arg2) == 1) ? arg2 : ((arg2 > arg1 || std::isnan(arg2)) ? arg2 : arg1);\n"_s;
    for (int i = 2; i < argc; i++) {
        expression +=
                "\ttmpMax = (qIsNull(%1) && qIsNull(tmpMax) && std::copysign(1.0, %1) == 1) ? arg2 : ((%1 > tmpMax || std::isnan(%1)) ? %1 : tmpMax);\n"_L1
                        .arg("arg"_L1 + QString::number(i + 1));
    }
    expression += "return tmpMax;\n}()"_L1;

    return expression;
}

static QString minExpression(int argc)
{
    Q_ASSERT_X(argc >= 2, Q_FUNC_INFO, "min() expects at least two arguments.");

    QString expression =
            u"[&]() { \nauto  tmpMin = (qIsNull(arg2) && qIsNull(arg1) && std::copysign(1.0, arg2) == -1) ? arg2 : ((arg2 < arg1 || std::isnan(arg2)) ? arg2 : arg1);\n"_s;
    for (int i = 2; i < argc; i++) {
        expression +=
                "tmpMin = (qIsNull(%1) && qIsNull(tmpMin) && std::copysign(1.0, %1) == -1) ? arg2 : ((%1 < tmpMin || std::isnan(%1)) ? %1 : tmpMin);\n"_L1
                        .arg("arg"_L1 + QString::number(i + 1));
    }
    expression += "return tmpMin;\n}()"_L1;

    return expression;
}

bool QQmlJSCodeGenerator::inlineMathMethod(const QString &name, int argc, int argv)
{
    addInclude(u"cmath"_s);
    addInclude(u"limits"_s);
    addInclude(u"qalgorithms.h"_s);
    addInclude(u"qrandom.h"_s);
    addInclude(u"qjsprimitivevalue.h"_s);

    // If the result is not stored, we don't need to generate any code. All the math methods are
    // conceptually pure functions.
    if (m_state.changedRegisterIndex() != Accumulator)
        return true;

    m_body += u"{\n"_s;
    for (int i = 0; i < argc; ++i) {
        m_body += u"const double arg%1 = "_s.arg(i + 1) + convertStored(
                        registerType(argv + i).storedType(),
                        m_typeResolver->realType(), consumedRegisterVariable(argv + i))
                + u";\n"_s;
    }

    const QString qNaN = u"std::numeric_limits<double>::quiet_NaN()"_s;
    const QString inf = u"std::numeric_limits<double>::infinity()"_s;
    m_body += m_state.accumulatorVariableOut + u" = "_s;

    QString expression;

    if (name == u"abs" && argc == 1) {
        expression = u"(qIsNull(arg1) ? 0 : (arg1 < 0.0 ? -arg1 : arg1))"_s;
    } else if (name == u"acos"_s && argc == 1) {
        expression = u"arg1 > 1.0 ? %1 : std::acos(arg1)"_s.arg(qNaN);
    } else if (name == u"acosh"_s && argc == 1) {
        expression = u"arg1 < 1.0 ? %1 : std::acosh(arg1)"_s.arg(qNaN);
    } else if (name == u"asin"_s && argc == 1) {
        expression = u"arg1 > 1.0 ? %1 : std::asin(arg1)"_s.arg(qNaN);
    } else if (name == u"asinh"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::asinh(arg1)"_s;
    } else if (name == u"atan"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::atan(arg1)"_s;
    } else if (name == u"atanh"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::atanh(arg1)"_s;
    } else if (name == u"atan2"_s) {
        // TODO: complicated
        return false;
    } else if (name == u"cbrt"_s && argc == 1) {
        expression = u"std::cbrt(arg1)"_s;
    } else if (name == u"ceil"_s && argc == 1) {
        expression = u"(arg1 < 0.0 && arg1 > -1.0) ? std::copysign(0.0, -1.0) : std::ceil(arg1)"_s;
    } else if (name == u"clz32"_s && argc == 1) {
        expression = u"qint32(qCountLeadingZeroBits(quint32(QJSNumberCoercion::toInteger(arg1))))"_s;
    } else if (name == u"cos"_s && argc == 1) {
        expression = u"std::cos(arg1)"_s;
    } else if (name == u"cosh"_s && argc == 1) {
        expression = u"std::cosh(arg1)"_s;
    } else if (name == u"exp"_s && argc == 1) {
        expression = u"std::isinf(arg1) "
                "? (std::copysign(1.0, arg1) == -1 ? 0.0 : %1) "
                ": std::exp(arg1)"_s.arg(inf);
    } else if (name == u"expm1"_s) {
        // TODO: complicated
        return false;
    } else if (name == u"floor"_s && argc == 1) {
        expression = u"std::floor(arg1)"_s;
    } else if (name == u"fround"_s && argc == 1) {
        expression = u"(std::isnan(arg1) || std::isinf(arg1) || qIsNull(arg1)) "
                "? arg1 "
                ": double(float(arg1))"_s;
    } else if (name == u"hypot"_s) {
        // TODO: complicated
        return false;
    } else if (name == u"imul"_s && argc == 2) {
        expression = u"qint32(quint32(QJSNumberCoercion::toInteger(arg1)) "
                "* quint32(QJSNumberCoercion::toInteger(arg2)))"_s;
    } else if (name == u"log"_s && argc == 1) {
        expression = u"arg1 < 0.0 ? %1 : std::log(arg1)"_s.arg(qNaN);
    } else if (name == u"log10"_s && argc == 1) {
        expression = u"arg1 < 0.0 ? %1 : std::log10(arg1)"_s.arg(qNaN);
    } else if (name == u"log1p"_s && argc == 1) {
        expression = u"arg1 < -1.0 ? %1 : std::log1p(arg1)"_s.arg(qNaN);
    } else if (name == u"log2"_s && argc == 1) {
        expression = u"arg1 < -0.0 ? %1 : std::log2(arg1)"_s.arg(qNaN);
    } else if (name == u"max"_s && argc >= 2) {
        expression = maxExpression(argc);
    } else if (name == u"min"_s && argc >= 2) {
        expression = minExpression(argc);
    } else if (name == u"pow"_s) {
        expression = u"QQmlPrivate::jsExponentiate(arg1, arg2)"_s;
    } else if (name == u"random"_s && argc == 0) {
        expression = u"QRandomGenerator::global()->generateDouble()"_s;
    } else if (name == u"round"_s && argc == 1) {
        expression = u"std::isfinite(arg1) "
                "? ((arg1 < 0.5 && arg1 >= -0.5) "
                    "? std::copysign(0.0, arg1) "
                    ": std::floor(arg1 + 0.5)) "
                ": arg1"_s;
    } else if (name == u"sign"_s && argc == 1) {
        expression = u"std::isnan(arg1) "
                "? %1 "
                ": (qIsNull(arg1) "
                    "? arg1 "
                    ": (std::signbit(arg1) ? -1.0 : 1.0))"_s.arg(qNaN);
    } else if (name == u"sin"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::sin(arg1)"_s;
    } else if (name == u"sinh"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::sinh(arg1)"_s;
    } else if (name == u"sqrt"_s && argc == 1) {
        expression = u"std::sqrt(arg1)"_s;
    } else if (name == u"tan"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::tan(arg1)"_s;
    } else if (name == u"tanh"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::tanh(arg1)"_s;
    } else if (name == u"trunc"_s && argc == 1) {
        expression = u"std::trunc(arg1)"_s;
    } else {
        return false;
    }

    m_body += conversion(m_typeResolver->realType(), m_state.accumulatorOut(), expression);

    m_body += u";\n"_s;
    m_body += u"}\n"_s;
    return true;
}

static QString messageTypeForMethod(const QString &method)
{
    if (method == u"log" || method == u"debug")
        return u"QtDebugMsg"_s;
    if (method == u"info")
        return u"QtInfoMsg"_s;
    if (method == u"warn")
        return u"QtWarningMsg"_s;
    if (method == u"error")
        return u"QtCriticalMsg"_s;
    return QString();
}

bool QQmlJSCodeGenerator::inlineConsoleMethod(const QString &name, int argc, int argv)
{
    const QString type = messageTypeForMethod(name);
    if (type.isEmpty())
        return false;

    addInclude(u"qloggingcategory.h"_s);

    m_body += u"{\n";
    m_body += u"    bool firstArgIsCategory = false;\n";
    const QQmlJSRegisterContent firstArg = argc > 0 ? registerType(argv) : QQmlJSRegisterContent();

    // We could check for internalName == "QQmlLoggingCategory" here, but we don't want to
    // because QQmlLoggingCategory is not a builtin. Tying the specific internal name and
    // intheritance hierarchy in here would be fragile.
    // TODO: We could drop the check for firstArg in some cases if we made some base class
    //       of QQmlLoggingCategory a builtin.
    const bool firstArgIsReference = argc > 0
            && m_typeResolver->containedType(firstArg)->isReferenceType();

    if (firstArgIsReference) {
        m_body += u"    QObject *firstArg = ";
        m_body += convertStored(
                    firstArg.storedType(),
                    m_typeResolver->genericType(firstArg.storedType()),
                    registerVariable(argv));
        m_body += u";\n";
    }

    m_body += u"    const QLoggingCategory *category = aotContext->resolveLoggingCategory(";
    m_body += firstArgIsReference ? u"firstArg" : u"nullptr";
    m_body += u", &firstArgIsCategory);\n";
    m_body += u"    if (category && category->isEnabled(" + type + u")) {\n";

    m_body += u"        const QString message = ";
    if (argc > 0) {
        const QString firstArgStringConversion = convertStored(
                    registerType(argv).storedType(),
                    m_typeResolver->stringType(), registerVariable(argv));
        if (firstArgIsReference) {
            m_body += u"(firstArgIsCategory ? QString() : (" + firstArgStringConversion;
            if (argc > 1)
                m_body += u".append(QLatin1Char(' ')))).append(";
            else
                m_body += u"))";
        } else {
            m_body += firstArgStringConversion;
            if (argc > 1)
                m_body += u".append(QLatin1Char(' ')).append(";
        }

        for (int i = 1; i < argc; ++i) {
            if (i > 1)
                m_body += u".append(QLatin1Char(' ')).append("_s;
            m_body += convertStored(
                        registerType(argv + i).storedType(),
                        m_typeResolver->stringType(), consumedRegisterVariable(argv + i)) + u')';
        }
    } else {
        m_body += u"QString()";
    }
    m_body += u";\n        ";
    generateSetInstructionPointer();
    m_body += u"        aotContext->writeToConsole(" + type + u", message, category);\n";
    m_body += u"    }\n";
    m_body += u"}\n";
    return true;
}

bool QQmlJSCodeGenerator::inlineArrayMethod(const QString &name, int base, int argc, int argv)
{
    const auto intType = m_typeResolver->int32Type();
    const auto valueType = registerType(base).storedType()->valueType();
    const auto boolType = m_typeResolver->boolType();
    const auto stringType = m_typeResolver->stringType();
    const auto baseType = registerType(base);

    const QString baseVar = registerVariable(base);
    const QString qjsListMethod = u"QJSList(&"_s + baseVar + u", aotContext->engine)."
            + name + u"(";

    addInclude(u"qjslist.h"_s);

    if (name == u"includes" && argc > 0 && argc < 3) {
        QString call = qjsListMethod
                + convertStored(registerType(argv).storedType(), valueType,
                                consumedRegisterVariable(argv));
        if (argc == 2) {
            call += u", " + convertStored(registerType(argv + 1).storedType(), intType,
                                              consumedRegisterVariable(argv + 1));
        }
        call += u")";

        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(boolType, m_state.accumulatorOut(), call) + u";\n"_s;
        return true;
    }

    if (name == u"toString" || (name == u"join" && argc < 2)) {
        QString call = qjsListMethod;
        if (argc == 1) {
            call += convertStored(registerType(argv).storedType(), stringType,
                                  consumedRegisterVariable(argv));
        }
        call += u")";

        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(stringType, m_state.accumulatorOut(), call) + u";\n"_s;
        return true;
    }

    if (name == u"slice" && argc < 3) {
        QString call = qjsListMethod;
        for (int i = 0; i < argc; ++i) {
            if (i > 0)
                call += u", ";
            call += convertStored(registerType(argv + i).storedType(), intType,
                                   consumedRegisterVariable(argv + i));
        }
        call += u")";

        const auto outType = baseType.storedType()->isListProperty()
                ? m_typeResolver->globalType(m_typeResolver->qObjectListType())
                : baseType;

        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(outType, m_state.accumulatorOut(), call) + u";\n"_s;
        return true;
    }

    if ((name == u"indexOf" || name == u"lastIndexOf") && argc > 0 && argc < 3) {
        QString call = qjsListMethod
                + convertStored(registerType(argv).storedType(), valueType,
                                consumedRegisterVariable(argv));
        if (argc == 2) {
            call += u", " + convertStored(registerType(argv + 1).storedType(), intType,
                                          consumedRegisterVariable(argv + 1));
        }
        call += u")";

        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(intType, m_state.accumulatorOut(), call) + u";\n"_s;
        return true;
    }

    return false;
}

void QQmlJSCodeGenerator::generate_CallPropertyLookup(int index, int base, int argc, int argv)
{
    INJECT_TRACE_INFO(generate_CallPropertyLookup);

    if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::JavaScriptReturnValue)
        reject(u"call to untyped JavaScript function"_s);

    AccumulatorConverter registers(this);

    const QQmlJSRegisterContent baseType = registerType(base);
    if (baseType.storedType()->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
        const QString name = m_jsUnitGenerator->stringForIndex(
                    m_jsUnitGenerator->lookupNameIndex(index));
        if (m_typeResolver->equals(m_typeResolver->originalContainedType(baseType), mathObject())) {
            if (inlineMathMethod(name, argc, argv))
                return;
        }

        if (m_typeResolver->equals(m_typeResolver->originalContainedType(baseType), consoleObject())) {
            if (inlineConsoleMethod(name, argc, argv))
                return;
        }

        if (m_typeResolver->equals(m_typeResolver->originalContainedType(baseType),
                                   m_typeResolver->stringType())) {
            if (inlineStringMethod(name, base, argc, argv))
                return;
        }

        if (baseType.storedType()->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence) {
            if (inlineArrayMethod(name, base, argc, argv))
                return;
        }

        // This is possible, once we establish the right kind of lookup for it
        reject(u"call to property '%1' of %2"_s.arg(name, baseType.descriptiveName()));
    }

    const QString indexString = QString::number(index);

    m_body += u"{\n"_s;

    QString outVar;
    m_body += argumentsList(argc, argv, &outVar);
    const QString lookup = u"aotContext->callObjectPropertyLookup("_s + indexString
            + u", "_s + registerVariable(base)
            + u", args, types, "_s + QString::number(argc) + u')';
    const QString initialization = u"aotContext->initCallObjectPropertyLookup("_s
            + indexString + u')';
    generateLookup(lookup, initialization);
    generateMoveOutVar(outVar);

    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generate_CallName(int name, int argc, int argv)
{
    Q_UNUSED(name);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    reject(u"CallName"_s);
}

void QQmlJSCodeGenerator::generate_CallPossiblyDirectEval(int argc, int argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CallGlobalLookup(int index, int argc, int argv)
{
    Q_UNUSED(index);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    reject(u"CallGlobalLookup"_s);
}

void QQmlJSCodeGenerator::generate_CallQmlContextPropertyLookup(int index, int argc, int argv)
{
    INJECT_TRACE_INFO(generate_CallQmlContextPropertyLookup);

    if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::JavaScriptReturnValue)
        reject(u"call to untyped JavaScript function"_s);

    if (m_typeResolver->equals(m_state.accumulatorOut().scopeType(),
                               m_typeResolver->jsGlobalObject())) {
        const QString name = m_jsUnitGenerator->stringForIndex(
                m_jsUnitGenerator->lookupNameIndex(index));
        if (inlineTranslateMethod(name, argc, argv))
            return;
    }

    AccumulatorConverter registers(this);

    const QString indexString = QString::number(index);

    m_body += u"{\n"_s;
    QString outVar;
    m_body += argumentsList(argc, argv, &outVar);
    const QString lookup = u"aotContext->callQmlContextPropertyLookup("_s + indexString
            + u", args, types, "_s + QString::number(argc) + u')';
    const QString initialization = u"aotContext->initCallQmlContextPropertyLookup("_s
            + indexString + u')';
    generateLookup(lookup, initialization);
    generateMoveOutVar(outVar);

    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generate_CallWithSpread(int func, int thisObject, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_TailCall(int func, int thisObject, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Construct(int func, int argc, int argv)
{
    Q_UNUSED(func);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    reject(u"Construct"_s);
}

void QQmlJSCodeGenerator::generate_ConstructWithSpread(int func, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_SetUnwindHandler(int offset)
{
    Q_UNUSED(offset)
    reject(u"SetUnwindHandlerh"_s);
}

void QQmlJSCodeGenerator::generate_UnwindDispatch()
{
    reject(u"UnwindDispatch"_s);
}

void QQmlJSCodeGenerator::generate_UnwindToLabel(int level, int offset)
{
    Q_UNUSED(level)
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DeadTemporalZoneCheck(int name)
{
    Q_UNUSED(name)
    INJECT_TRACE_INFO(generate_DeadTemporalZoneCheck);
    // Nothing to do here. If we have statically asserted the dtz check in the type propagator
    // the value cannot be empty. Otherwise we can't get here.
}

void QQmlJSCodeGenerator::generate_ThrowException()
{
    INJECT_TRACE_INFO(generate_ThrowException);

    generateSetInstructionPointer();
    m_body += u"aotContext->engine->throwError("_s
        + conversion(m_state.accumulatorIn(), m_typeResolver->globalType(
                         m_typeResolver->jsValueType()),
                     m_state.accumulatorVariableIn) + u");\n"_s;
    m_body += u"return "_s + errorReturnValue() + u";\n"_s;
    m_skipUntilNextLabel = true;
    resetState();
}

void QQmlJSCodeGenerator::generate_GetException()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_SetException()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateCallContext()
{
    INJECT_TRACE_INFO(generate_CreateCallContext);

    m_body += u"{\n"_s;
}

void QQmlJSCodeGenerator::generate_PushCatchContext(int index, int nameIndex)
{
    Q_UNUSED(index)
    Q_UNUSED(nameIndex)
    reject(u"PushCatchContext"_s);
}

void QQmlJSCodeGenerator::generate_PushWithContext()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PushBlockContext(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CloneBlockContext()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PushScriptContext(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PopScriptContext()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PopContext()
{
    INJECT_TRACE_INFO(generate_PopContext);

    // Add a semicolon before the closing brace, in case there was a bare label before it.
    m_body += u";}\n"_s;
}

void QQmlJSCodeGenerator::generate_GetIterator(int iterator)
{
    Q_UNUSED(iterator)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_IteratorNext(int value, int done)
{
    Q_UNUSED(value)
    Q_UNUSED(done)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_IteratorNextForYieldStar(int iterator, int object)
{
    Q_UNUSED(iterator)
    Q_UNUSED(object)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_IteratorClose(int done)
{
    Q_UNUSED(done)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DestructureRestElement()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DeleteProperty(int base, int index)
{
    Q_UNUSED(base)
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DeleteName(int name)
{
    Q_UNUSED(name)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_TypeofName(int name)
{
    Q_UNUSED(name);
    reject(u"TypeofName"_s);
}

void QQmlJSCodeGenerator::generate_TypeofValue()
{
    reject(u"TypeofValue"_s);
}

void QQmlJSCodeGenerator::generate_DeclareVar(int varName, int isDeletable)
{
    Q_UNUSED(varName)
    Q_UNUSED(isDeletable)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DefineArray(int argc, int args)
{
    INJECT_TRACE_INFO(generate_DefineArray);

    const QQmlJSScope::ConstPtr stored = m_state.accumulatorOut().storedType();
    if (stored->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence) {
        // This rejects any attempt to store the list into a QVariant.
        // Therefore, we don't have to adjust the contained type below.
        reject(u"storing an array in a non-sequence type"_s);
        return;
    }

    const QQmlJSScope::ConstPtr value = stored->valueType();
    Q_ASSERT(value);

    QStringList initializer;
    for (int i = 0; i < argc; ++i) {
        initializer += convertStored(
                    registerType(args + i).storedType(), value,
                    consumedRegisterVariable(args + i));
    }

    if (stored->isListProperty()) {
        reject(u"creating a QQmlListProperty not backed by a property"_s);

        // We can, technically, generate code for this. But it's dangerous:
        //
        // const QString storage = m_state.accumulatorVariableOut + u"_storage"_s;
        // m_body += stored->internalName() + u"::ListType " + storage
        //         + u" = {"_s + initializer.join(u", "_s) + u"};\n"_s;
        // m_body += m_state.accumulatorVariableOut
        //         + u" = " + stored->internalName() + u"(nullptr, &"_s + storage + u");\n"_s;
    } else {
        m_body += m_state.accumulatorVariableOut + u" = "_s + stored->internalName() + u'{';
        m_body += initializer.join(u", "_s);
        m_body += u"};\n";
    }
}

void QQmlJSCodeGenerator::generate_DefineObjectLiteral(int internalClassId, int argc, int args)
{
    Q_UNUSED(internalClassId)
    Q_UNUSED(argc)
    Q_UNUSED(args)
    reject(u"DefineObjectLiteral"_s);
}

void QQmlJSCodeGenerator::generate_CreateClass(int classIndex, int heritage, int computedNames)
{
    Q_UNUSED(classIndex)
    Q_UNUSED(heritage)
    Q_UNUSED(computedNames)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateMappedArgumentsObject()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateUnmappedArgumentsObject()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateRestParameter(int argIndex)
{
    Q_UNUSED(argIndex)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_ConvertThisToObject()
{
    INJECT_TRACE_INFO(generate_ConvertThisToObject);

    m_body += changedRegisterVariable() + u" = "_s
            + conversion(m_typeResolver->qObjectType(), m_state.changedRegister(),
                         u"aotContext->thisObject()"_s)
            + u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadSuperConstructor()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_ToObject()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Jump(int offset)
{
    INJECT_TRACE_INFO(generate_Jump);

    generateJumpCodeWithTypeConversions(offset);
    m_body += u";\n"_s;
    m_skipUntilNextLabel = true;
    resetState();
}

void QQmlJSCodeGenerator::generate_JumpTrue(int offset)
{
    INJECT_TRACE_INFO(generate_JumpTrue);

    m_body += u"if ("_s;
    m_body += convertStored(m_state.accumulatorIn().storedType(), m_typeResolver->boolType(),
                            m_state.accumulatorVariableIn);
    m_body += u") "_s;
    generateJumpCodeWithTypeConversions(offset);
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_JumpFalse(int offset)
{
    INJECT_TRACE_INFO(generate_JumpFalse);

    m_body += u"if (!"_s;
    m_body += convertStored(m_state.accumulatorIn().storedType(), m_typeResolver->boolType(),
                            m_state.accumulatorVariableIn);
    m_body += u") "_s;
    generateJumpCodeWithTypeConversions(offset);
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_JumpNoException(int offset)
{
    INJECT_TRACE_INFO(generate_JumpNoException);

    m_body += u"if (!context->engine->hasException()) "_s;
    generateJumpCodeWithTypeConversions(offset);
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_JumpNotUndefined(int offset)
{
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CheckException()
{
    INJECT_TRACE_INFO(generate_CheckException);

    generateExceptionCheck();
}

void QQmlJSCodeGenerator::generate_CmpEqNull()
{
    INJECT_TRACE_INFO(generate_CmlEqNull);

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += conversion(
                m_typeResolver->boolType(), m_state.accumulatorOut(),
                u"QJSPrimitiveValue(QJSPrimitiveNull()).equals("_s
                + convertStored(
                    m_state.accumulatorIn().storedType(), m_typeResolver->jsPrimitiveType(),
                    consumedAccumulatorVariableIn()) + u')');
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_CmpNeNull()
{
    INJECT_TRACE_INFO(generate_CmlNeNull);

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += conversion(
                m_typeResolver->boolType(), m_state.accumulatorOut(),
                u"!QJSPrimitiveValue(QJSPrimitiveNull()).equals("_s
                + convertStored(
                    m_state.accumulatorIn().storedType(), m_typeResolver->jsPrimitiveType(),
                    consumedAccumulatorVariableIn()) + u')');
    m_body += u";\n"_s;
}

QString QQmlJSCodeGenerator::eqIntExpression(int lhsConst)
{
    if (m_typeResolver->isIntegral(m_state.accumulatorIn()))
        return QString::number(lhsConst) + u" == "_s + m_state.accumulatorVariableIn;

    if (m_typeResolver->registerIsStoredIn(m_state.accumulatorIn(), m_typeResolver->boolType())) {
        return QString::number(lhsConst) + u" == "_s
                + convertStored(m_state.accumulatorIn().storedType(), m_typeResolver->int32Type(),
                                consumedAccumulatorVariableIn());
    }

    if (m_typeResolver->isNumeric(m_state.accumulatorIn())) {
        return convertStored(m_typeResolver->int32Type(), m_typeResolver->realType(),
                             QString::number(lhsConst)) + u" == "_s
                + convertStored(m_state.accumulatorIn().storedType(), m_typeResolver->realType(),
                                consumedAccumulatorVariableIn());
    }

    QString result;
    result += convertStored(m_typeResolver->int32Type(), m_typeResolver->jsPrimitiveType(),
                            QString::number(lhsConst));
    result += u".equals("_s;
    result += convertStored(m_state.accumulatorIn().storedType(), m_typeResolver->jsPrimitiveType(),
                            consumedAccumulatorVariableIn());
    result += u')';
    return result;
}

QString QQmlJSCodeGenerator::getLookupPreparation(
        const QQmlJSRegisterContent &content, const QString &var, int lookup)
{
    if (m_typeResolver->registerContains(content, content.storedType()))
        return QString();

    if (m_typeResolver->registerIsStoredIn(content, m_typeResolver->varType())) {
        return var + u" = QVariant(aotContext->lookupResultMetaType("_s
                + QString::number(lookup) + u"))"_s;
    }
    // TODO: We could make sure they're compatible, for example QObject pointers.
    return QString();
}

QString QQmlJSCodeGenerator::contentPointer(const QQmlJSRegisterContent &content, const QString &var)
{
    const QQmlJSScope::ConstPtr stored = content.storedType();
    if (m_typeResolver->registerContains(content, stored))
        return u'&' + var;

    if (m_typeResolver->registerIsStoredIn(content, m_typeResolver->varType())
            || m_typeResolver->registerIsStoredIn(content, m_typeResolver->jsPrimitiveType())) {
        return var + u".data()"_s;
    }

    if (stored->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
        return u'&' + var;

    if (m_typeResolver->isNumeric(content.storedType())
        && m_typeResolver->containedType(content)->scopeType() == QQmlSA::ScopeType::EnumScope) {
        return u'&' + var;
    }

    if (stored->isListProperty() && m_typeResolver->containedType(content)->isListProperty())
        return u'&' + var;

    reject(u"content pointer of unsupported wrapper type "_s + content.descriptiveName());
    return QString();
}

QString QQmlJSCodeGenerator::contentType(const QQmlJSRegisterContent &content, const QString &var)
{
    const QQmlJSScope::ConstPtr stored = content.storedType();
    const QQmlJSScope::ConstPtr contained = QQmlJSScope::nonCompositeBaseType(
                m_typeResolver->containedType(content));
    if (m_typeResolver->equals(contained, stored))
        return metaTypeFromType(stored);

    if (m_typeResolver->equals(stored, m_typeResolver->varType())
            || m_typeResolver->registerIsStoredIn(content, m_typeResolver->jsPrimitiveType())) {
        return var + u".metaType()"_s; // We expect the container to be initialized
    }

    if (stored->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
        return metaTypeFromName(contained);

    if (m_typeResolver->isNumeric(stored) && contained->scopeType() == QQmlSA::ScopeType::EnumScope)
        return metaTypeFromType(contained->baseType());

    if (stored->isListProperty() && m_typeResolver->containedType(content)->isListProperty())
        return metaTypeFromType(m_typeResolver->listPropertyType());

    reject(u"content type of unsupported wrapper type "_s + content.descriptiveName());
    return QString();
}

void QQmlJSCodeGenerator::generate_CmpEqInt(int lhsConst)
{
    INJECT_TRACE_INFO(generate_CmpEqInt);

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                         eqIntExpression(lhsConst)) + u";\n"_s;
}

void QQmlJSCodeGenerator::generate_CmpNeInt(int lhsConst)
{
    INJECT_TRACE_INFO(generate_CmpNeInt);

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                         u"!("_s + eqIntExpression(lhsConst) + u')') + u";\n"_s;
}

void QQmlJSCodeGenerator::generate_CmpEq(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpEq);
    generateEqualityOperation(lhs, u"equals"_s, false);
}

void QQmlJSCodeGenerator::generate_CmpNe(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpNe);
    generateEqualityOperation(lhs, u"equals"_s, true);
}

void QQmlJSCodeGenerator::generate_CmpGt(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpGt);
    generateCompareOperation(lhs, u">"_s);
}

void QQmlJSCodeGenerator::generate_CmpGe(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpGe);
    generateCompareOperation(lhs, u">="_s);
}

void QQmlJSCodeGenerator::generate_CmpLt(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpLt);
    generateCompareOperation(lhs, u"<"_s);
}

void QQmlJSCodeGenerator::generate_CmpLe(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpLe);
    generateCompareOperation(lhs, u"<="_s);
}

void QQmlJSCodeGenerator::generate_CmpStrictEqual(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpStrictEqual);
    generateEqualityOperation(lhs, u"strictlyEquals"_s, false);
}

void QQmlJSCodeGenerator::generate_CmpStrictNotEqual(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpStrictNotEqual);
    generateEqualityOperation(lhs, u"strictlyEquals"_s, true);
}

void QQmlJSCodeGenerator::generate_CmpIn(int lhs)
{
    Q_UNUSED(lhs)
    reject(u"CmpIn"_s);
}

void QQmlJSCodeGenerator::generate_CmpInstanceOf(int lhs)
{
    Q_UNUSED(lhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_As(int lhs)
{
    INJECT_TRACE_INFO(generate_As);

    const QString input = registerVariable(lhs);
    const QQmlJSRegisterContent inputContent = m_state.readRegister(lhs);
    const QQmlJSRegisterContent outputContent = m_state.accumulatorOut();

    // If the original output is a conversion, we're supposed to check for the contained
    // type and if it doesn't match, set the result to null or undefined.
    const QQmlJSRegisterContent originalContent = m_typeResolver->original(outputContent);

    const QQmlJSScope::ConstPtr contained = m_typeResolver->containedType(originalContent);

    if (contained->isReferenceType()) {
        const QQmlJSScope::ConstPtr genericContained = m_typeResolver->genericType(contained);
        const QString inputConversion = inputContent.storedType()->isReferenceType()
                ? input
                : convertStored(inputContent.storedType(), genericContained, input);

        m_body += m_state.accumulatorVariableOut + u" = "_s;
        if (m_typeResolver->equals(
                    m_state.accumulatorIn().storedType(), m_typeResolver->metaObjectType())
                && contained->isComposite()) {
            m_body += conversion(
                        genericContained, outputContent,
                        m_state.accumulatorVariableIn + u"->cast("_s + inputConversion + u')');
        } else {
            m_body += conversion(
                        genericContained, outputContent,
                        u'(' + metaObject(contained) + u")->cast("_s + inputConversion + u')');
        }
        m_body += u";\n"_s;
        return;
    } else if (m_typeResolver->equals(inputContent.storedType(), m_typeResolver->varType())) {
        if (originalContent.isConversion()) {
            const auto origins = originalContent.conversionOrigins();
            Q_ASSERT(origins.size() == 2);

            const auto target = m_typeResolver->equals(origins[0], m_typeResolver->voidType())
                ? origins[1]
                : origins[0];

            Q_ASSERT(!m_typeResolver->equals(target, m_typeResolver->voidType()));

            m_body += m_state.accumulatorVariableOut + u" = "_s;
            m_body += input + u".metaType() == "_s + metaType(target)
                    + u" ? " + conversion(inputContent, outputContent, input)
                    + u" : " + conversion(m_typeResolver->globalType(m_typeResolver->voidType()),
                                          outputContent, QString());
            m_body += u";\n"_s;
            return;
        }
    }

    reject(u"unsupported type assertion"_s);

}

void QQmlJSCodeGenerator::generate_UNot()
{
    INJECT_TRACE_INFO(generate_UNot);
    generateUnaryOperation(u"!"_s);
}

void QQmlJSCodeGenerator::generate_UPlus()
{
    INJECT_TRACE_INFO(generate_UPlus);
    generateUnaryOperation(u"+"_s);
}

void QQmlJSCodeGenerator::generate_UMinus()
{
    INJECT_TRACE_INFO(generate_UMinus);
    generateUnaryOperation(u"-"_s);
}

void QQmlJSCodeGenerator::generate_UCompl()
{
    INJECT_TRACE_INFO(generate_UCompl);
    generateUnaryOperation(u"~"_s);
}

void QQmlJSCodeGenerator::generate_Increment()
{
    INJECT_TRACE_INFO(generate_Increment);
    generateInPlaceOperation(u"++"_s);
}

void QQmlJSCodeGenerator::generate_Decrement()
{
    INJECT_TRACE_INFO(generate_Decrement);
    generateInPlaceOperation(u"--"_s);
}

void QQmlJSCodeGenerator::generate_Add(int lhs)
{
    INJECT_TRACE_INFO(generate_Add);
    generateArithmeticOperation(lhs, u"+"_s);
}

void QQmlJSCodeGenerator::generate_BitAnd(int lhs)
{
    INJECT_TRACE_INFO(generate_BitAnd);
    generateArithmeticOperation(lhs, u"&"_s);
}

void QQmlJSCodeGenerator::generate_BitOr(int lhs)
{
    INJECT_TRACE_INFO(generate_BitOr);
    generateArithmeticOperation(lhs, u"|"_s);
}

void QQmlJSCodeGenerator::generate_BitXor(int lhs)
{
    INJECT_TRACE_INFO(generate_BitXor);
    generateArithmeticOperation(lhs, u"^"_s);
}

void QQmlJSCodeGenerator::generate_UShr(int lhs)
{
    INJECT_TRACE_INFO(generate_BitUShr);
    generateShiftOperation(lhs, u">>"_s);
}

void QQmlJSCodeGenerator::generate_Shr(int lhs)
{
    INJECT_TRACE_INFO(generate_Shr);
    generateShiftOperation(lhs, u">>"_s);
}

void QQmlJSCodeGenerator::generate_Shl(int lhs)
{
    INJECT_TRACE_INFO(generate_Shl);
    generateShiftOperation(lhs, u"<<"_s);
}

void QQmlJSCodeGenerator::generate_BitAndConst(int rhs)
{
    INJECT_TRACE_INFO(generate_BitAndConst);
    generateArithmeticConstOperation(rhs, u"&"_s);
}

void QQmlJSCodeGenerator::generate_BitOrConst(int rhs)
{
    INJECT_TRACE_INFO(generate_BitOrConst);
    generateArithmeticConstOperation(rhs, u"|"_s);
}

void QQmlJSCodeGenerator::generate_BitXorConst(int rhs)
{
    INJECT_TRACE_INFO(generate_BitXorConst);
    generateArithmeticConstOperation(rhs, u"^"_s);
}

void QQmlJSCodeGenerator::generate_UShrConst(int rhs)
{
    INJECT_TRACE_INFO(generate_UShrConst);
    generateArithmeticConstOperation(rhs & 0x1f, u">>"_s);
}

void QQmlJSCodeGenerator::generate_ShrConst(int rhs)
{
    INJECT_TRACE_INFO(generate_ShrConst);
    generateArithmeticConstOperation(rhs & 0x1f, u">>"_s);
}

void QQmlJSCodeGenerator::generate_ShlConst(int rhs)
{
    INJECT_TRACE_INFO(generate_ShlConst);
    generateArithmeticConstOperation(rhs & 0x1f, u"<<"_s);
}

void QQmlJSCodeGenerator::generate_Exp(int lhs)
{
    INJECT_TRACE_INFO(generate_Exp);

    const QString lhsString = conversion(
                registerType(lhs), m_state.readRegister(lhs), consumedRegisterVariable(lhs));
    const QString rhsString = conversion(
                m_state.accumulatorIn(), m_state.readAccumulator(),
                consumedAccumulatorVariableIn());

    Q_ASSERT(m_error->isValid() || !lhsString.isEmpty());
    Q_ASSERT(m_error->isValid() || !rhsString.isEmpty());

    const QQmlJSRegisterContent originalOut = m_typeResolver->original(m_state.accumulatorOut());
    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += conversion(
                originalOut, m_state.accumulatorOut(),
                u"QQmlPrivate::jsExponentiate("_s + lhsString + u", "_s + rhsString + u')');
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_Mul(int lhs)
{
    INJECT_TRACE_INFO(generate_Mul);
    generateArithmeticOperation(lhs, u"*"_s);
}

void QQmlJSCodeGenerator::generate_Div(int lhs)
{
    INJECT_TRACE_INFO(generate_Div);
    generateArithmeticOperation(lhs, u"/"_s);
}

void QQmlJSCodeGenerator::generate_Mod(int lhs)
{
    INJECT_TRACE_INFO(generate_Mod);

    const auto lhsVar = convertStored(
                registerType(lhs).storedType(), m_typeResolver->jsPrimitiveType(),
                consumedRegisterVariable(lhs));
    const auto rhsVar = convertStored(
                m_state.accumulatorIn().storedType(), m_typeResolver->jsPrimitiveType(),
                consumedAccumulatorVariableIn());
    Q_ASSERT(m_error->isValid() || !lhsVar.isEmpty());
    Q_ASSERT(m_error->isValid() || !rhsVar.isEmpty());

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s;
    m_body += conversion(m_typeResolver->jsPrimitiveType(), m_state.accumulatorOut(),
                       u'(' + lhsVar + u" % "_s + rhsVar + u')');
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_Sub(int lhs)
{
    INJECT_TRACE_INFO(generate_Sub);
    generateArithmeticOperation(lhs, u"-"_s);
}

void QQmlJSCodeGenerator::generate_InitializeBlockDeadTemporalZone(int firstReg, int count)
{
    Q_UNUSED(firstReg)
    Q_UNUSED(count)
    // Ignore. We reject uninitialized values anyway.
}

void QQmlJSCodeGenerator::generate_ThrowOnNullOrUndefined()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_GetTemplateObject(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

QV4::Moth::ByteCodeHandler::Verdict QQmlJSCodeGenerator::startInstruction(
        QV4::Moth::Instr::Type type)
{
    m_state.State::operator=(nextStateFromAnnotations(m_state, *m_annotations));
    const auto accumulatorIn = m_state.registers.find(Accumulator);
    if (accumulatorIn != m_state.registers.end()
            && isTypeStorable(m_typeResolver, accumulatorIn.value().content.storedType())) {
        m_state.accumulatorVariableIn = m_registerVariables.value(RegisterVariablesKey {
            accumulatorIn.value().content.storedType()->internalName(),
            Accumulator
        }).variableName;
        Q_ASSERT(!m_state.accumulatorVariableIn.isEmpty());
    } else {
        m_state.accumulatorVariableIn.clear();
    }

    auto labelIt = m_labels.constFind(currentInstructionOffset());
    if (labelIt != m_labels.constEnd()) {
        m_body += *labelIt + u":;\n"_s;
        m_skipUntilNextLabel = false;
    } else if (m_skipUntilNextLabel && !instructionManipulatesContext(type)) {
        return SkipInstruction;
    }

    if (m_state.changedRegisterIndex() == Accumulator)
        m_state.accumulatorVariableOut = changedRegisterVariable();
    else
        m_state.accumulatorVariableOut.clear();

    // If the accumulator type is valid, we want an accumulator variable.
    // If not, we don't want one.
    Q_ASSERT(m_state.changedRegisterIndex() == Accumulator
             || m_state.accumulatorVariableOut.isEmpty());
    Q_ASSERT(m_state.changedRegisterIndex() != Accumulator
             || !m_state.accumulatorVariableOut.isEmpty()
             || !isTypeStorable(m_typeResolver, m_state.changedRegister().storedType()));

    // If the instruction has no side effects and doesn't write any register, it's dead.
    // We might still need the label, though, and the source code comment.
    if (!m_state.hasSideEffects() && changedRegisterVariable().isEmpty()) {
        generateJumpCodeWithTypeConversions(0);
        return SkipInstruction;
    }

    return ProcessInstruction;
}

void QQmlJSCodeGenerator::endInstruction(QV4::Moth::Instr::Type)
{
    if (!m_skipUntilNextLabel)
        generateJumpCodeWithTypeConversions(0);
}

void QQmlJSCodeGenerator::generateSetInstructionPointer()
{
    m_body += u"aotContext->setInstructionPointer("_s
        + QString::number(nextInstructionOffset()) + u");\n"_s;
}

void QQmlJSCodeGenerator::generateExceptionCheck()
{
    m_body += u"if (aotContext->engine->hasError())\n"_s;
    m_body += u"    return "_s + errorReturnValue() + u";\n"_s;
}

void QQmlJSCodeGenerator::generateEqualityOperation(int lhs, const QString &function, bool invert)
{
    const QQmlJSRegisterContent lhsContent = registerType(lhs);
    const bool strictlyComparableWithVar = function == "strictlyEquals"_L1
            && canStrictlyCompareWithVar(m_typeResolver, lhsContent, m_state.accumulatorIn());
    auto isComparable = [&]() {
        if (m_typeResolver->isPrimitive(lhsContent)
                && m_typeResolver->isPrimitive(m_state.accumulatorIn())) {
            return true;
        }
        if (m_typeResolver->isNumeric(lhsContent) && m_state.accumulatorIn().isEnumeration())
            return true;
        if (m_typeResolver->isNumeric(m_state.accumulatorIn()) && lhsContent.isEnumeration())
            return true;
        if (strictlyComparableWithVar)
            return true;
        if (canCompareWithQObject(m_typeResolver, lhsContent, m_state.accumulatorIn()))
            return true;
        if (canCompareWithQUrl(m_typeResolver, lhsContent, m_state.accumulatorIn()))
            return true;
        return false;
    };

    if (!isComparable()) {
        reject(u"incomparable types %1 and %2"_s.arg(m_state.accumulatorIn().descriptiveName(),
                                                     lhsContent.descriptiveName()));
    }

    const QQmlJSScope::ConstPtr lhsType = lhsContent.storedType();
    const QQmlJSScope::ConstPtr rhsType = m_state.accumulatorIn().storedType();

    const auto primitive = m_typeResolver->jsPrimitiveType();
    if (m_typeResolver->equals(lhsType, rhsType) && !m_typeResolver->equals(lhsType, primitive)) {
        m_body += m_state.accumulatorVariableOut + u" = "_s;
        if (isTypeStorable(m_typeResolver, lhsType)) {
            m_body += conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                                 consumedRegisterVariable(lhs) + (invert ? u" != "_s : u" == "_s)
                                 + consumedAccumulatorVariableIn());
        } else {
            // null === null and  undefined === undefined
            m_body += invert ? u"false"_s : u"true"_s;
        }
    } else if (strictlyComparableWithVar) {
        // Determine which side is holding a storable type
        if (const auto registerVariableName = registerVariable(lhs);
            !registerVariableName.isEmpty()) {
            // lhs register holds var type
            generateVariantEqualityComparison(m_state.accumulatorIn(), registerVariableName,
                                              invert);
        } else {
            // lhs content is not storable and rhs is var type
            generateVariantEqualityComparison(lhsContent, m_state.accumulatorVariableIn, invert);
        }
    } else if (canCompareWithQObject(m_typeResolver, lhsContent, m_state.accumulatorIn())) {
        m_body += m_state.accumulatorVariableOut + u" = "_s;
        m_body += u'('
                + (isTypeStorable(m_typeResolver, lhsContent.storedType())
                           ? registerVariable(lhs)
                           : m_state.accumulatorVariableIn)
                + (invert ? u" != "_s : u" == "_s) + u"nullptr)"_s;
    } else {
        m_body += m_state.accumulatorVariableOut + u" = "_s;
        m_body += conversion(
                    m_typeResolver->boolType(), m_state.accumulatorOut(),
                    (invert ? u"!"_s : QString())
                    + convertStored(registerType(lhs).storedType(), primitive,
                                    consumedRegisterVariable(lhs))
                    + u'.' + function + u'(' + convertStored(
                        m_state.accumulatorIn().storedType(), primitive,
                        consumedAccumulatorVariableIn())
                    + u')');
    }
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generateCompareOperation(int lhs, const QString &cppOperator)
{
    m_body += m_state.accumulatorVariableOut + u" = "_s;

    const auto lhsType = registerType(lhs);
    const QQmlJSScope::ConstPtr compareType =
            m_typeResolver->isNumeric(lhsType) && m_typeResolver->isNumeric(m_state.accumulatorIn())
                ? m_typeResolver->merge(lhsType, m_state.accumulatorIn()).storedType()
                : m_typeResolver->jsPrimitiveType();

    m_body += conversion(
                m_typeResolver->boolType(), m_state.accumulatorOut(),
                convertStored(registerType(lhs).storedType(), compareType,
                              consumedRegisterVariable(lhs))
                    + u' ' + cppOperator + u' '
                    + convertStored(m_state.accumulatorIn().storedType(), compareType,
                                    consumedAccumulatorVariableIn()));
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generateArithmeticOperation(int lhs, const QString &cppOperator)
{
    generateArithmeticOperation(
                conversion(registerType(lhs), m_state.readRegister(lhs),
                           consumedRegisterVariable(lhs)),
                conversion(m_state.accumulatorIn(), m_state.readAccumulator(),
                           consumedAccumulatorVariableIn()),
                cppOperator);
}

void QQmlJSCodeGenerator::generateShiftOperation(int lhs, const QString &cppOperator)
{
    generateArithmeticOperation(
                conversion(registerType(lhs), m_state.readRegister(lhs),
                           consumedRegisterVariable(lhs)),
                u'(' + conversion(m_state.accumulatorIn(), m_state.readAccumulator(),
                           consumedAccumulatorVariableIn()) + u" & 0x1f)"_s,
                cppOperator);
}

void QQmlJSCodeGenerator::generateArithmeticOperation(
        const QString &lhs, const QString &rhs, const QString &cppOperator)
{
    Q_ASSERT(m_error->isValid() || !lhs.isEmpty());
    Q_ASSERT(m_error->isValid() || !rhs.isEmpty());

    const QQmlJSRegisterContent originalOut = m_typeResolver->original(m_state.accumulatorOut());
    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s;
    m_body += conversion(
                originalOut, m_state.accumulatorOut(),
                u'(' + lhs + u' ' + cppOperator + u' ' + rhs + u')');
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generateArithmeticConstOperation(int rhsConst, const QString &cppOperator)
{
    generateArithmeticOperation(
                conversion(m_state.accumulatorIn(), m_state.readAccumulator(),
                           consumedAccumulatorVariableIn()),
                conversion(m_typeResolver->globalType(m_typeResolver->int32Type()),
                           m_state.readAccumulator(), QString::number(rhsConst)),
                cppOperator);
}

void QQmlJSCodeGenerator::generateUnaryOperation(const QString &cppOperator)
{
    const auto var = conversion(m_state.accumulatorIn(),
                                m_typeResolver->original(m_state.readAccumulator()),
                                consumedAccumulatorVariableIn());

    if (var == m_state.accumulatorVariableOut) {
        m_body += m_state.accumulatorVariableOut + u" = "_s + cppOperator + var + u";\n"_s;
        return;
    }

    const auto original = m_typeResolver->original(m_state.accumulatorOut());
    if (m_state.accumulatorOut() == original) {
        m_body += m_state.accumulatorVariableOut + u" = "_s + var + u";\n"_s;
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + cppOperator + m_state.accumulatorVariableOut + u";\n"_s;
        return;
    }

    m_body += m_state.accumulatorVariableOut + u" = "_s + conversion(
                original, m_state.accumulatorOut(), cppOperator + var) + u";\n"_s;
}

void QQmlJSCodeGenerator::generateInPlaceOperation(const QString &cppOperator)
{
    {
        // If actually in place, we cannot consume the variable.
        const QString var = conversion(m_state.accumulatorIn(), m_state.readAccumulator(),
                                       m_state.accumulatorVariableIn);
        if (var == m_state.accumulatorVariableOut) {
            m_body += cppOperator + var + u";\n"_s;
            return;
        }
    }

    const QString var = conversion(m_state.accumulatorIn(), m_state.readAccumulator(),
                                   consumedAccumulatorVariableIn());

    const auto original = m_typeResolver->original(m_state.accumulatorOut());
    if (m_state.accumulatorOut() == original) {
        m_body += m_state.accumulatorVariableOut + u" = "_s + var + u";\n"_s;
        m_body += cppOperator + m_state.accumulatorVariableOut + u";\n"_s;
        return;
    }

    m_body += u"{\n"_s;
    m_body += u"auto converted = "_s + var + u";\n"_s;
    m_body += m_state.accumulatorVariableOut + u" = "_s + conversion(
                original, m_state.accumulatorOut(), u'('
                + cppOperator + u"converted)"_s)  + u";\n"_s;
    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generateLookup(const QString &lookup, const QString &initialization,
                                        const QString &resultPreparation)
{
    if (!resultPreparation.isEmpty())
        m_body += resultPreparation + u";\n"_s;
    m_body += u"while (!"_s + lookup + u") {\n"_s;
    generateSetInstructionPointer();
    m_body += initialization + u";\n"_s;
    generateExceptionCheck();
    if (!resultPreparation.isEmpty())
        m_body += resultPreparation + u";\n"_s;
    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generateJumpCodeWithTypeConversions(int relativeOffset)
{
    QString conversionCode;
    const int absoluteOffset = nextInstructionOffset() + relativeOffset;
    const auto annotation = m_annotations->find(absoluteOffset);
    if (annotation != m_annotations->constEnd()) {
        const auto &conversions = annotation->second.typeConversions;

        for (auto regIt = conversions.constBegin(), regEnd = conversions.constEnd();
             regIt != regEnd; ++regIt) {
            int registerIndex = regIt.key();
            const QQmlJSRegisterContent targetType = regIt.value().content;
            if (!targetType.isValid())
                continue;

            QQmlJSRegisterContent currentType;
            QString currentVariable;
            if (registerIndex == m_state.changedRegisterIndex()) {
                currentType = m_state.changedRegister();
                currentVariable = changedRegisterVariable();
            } else {
                auto it = m_state.registers.find(registerIndex);
                if (it == m_state.registers.end())
                    continue;
                currentType = it.value().content;
                currentVariable = consumedRegisterVariable(registerIndex);
            }

            // Actually == here. We want the jump code also for equal types
            if (currentType == targetType
                    || !isTypeStorable(m_typeResolver, targetType.storedType())) {
                continue;
            }

            const auto variable = m_registerVariables.constFind(RegisterVariablesKey {
                    targetType.storedType()->internalName(),
                    registerIndex
            });
            if (variable == m_registerVariables.end() || variable->variableName == currentVariable)
                continue;

            conversionCode += variable->variableName;
            conversionCode += u" = "_s;
            conversionCode += conversion(currentType, targetType, currentVariable);
            conversionCode += u";\n"_s;
        }
    }

    if (relativeOffset) {
        auto labelIt = m_labels.find(absoluteOffset);
        if (labelIt == m_labels.end())
            labelIt = m_labels.insert(absoluteOffset, u"label_%1"_s.arg(m_labels.size()));
        conversionCode += u"    goto "_s + *labelIt + u";\n"_s;
    }

    if (!conversionCode.isEmpty())
        m_body += u"{\n"_s + conversionCode + u"}\n"_s;
}

QString QQmlJSCodeGenerator::registerVariable(int index) const
{
    const auto it = m_registerVariables.constFind(RegisterVariablesKey {
        registerType(index).storedType()->internalName(),
        index
    });
    if (it != m_registerVariables.constEnd())
        return it->variableName;

    return QString();
}

QString QQmlJSCodeGenerator::consumedRegisterVariable(int index) const
{
    const QString var = registerVariable(index);
    if (var.isEmpty() || !m_state.canMoveReadRegister(index))
        return var;
    return u"std::move(" + var + u")";
}

QString QQmlJSCodeGenerator::consumedAccumulatorVariableIn() const
{
    return m_state.canMoveReadRegister(Accumulator)
            ? u"std::move(" + m_state.accumulatorVariableIn + u")"
            : m_state.accumulatorVariableIn;
}

QString QQmlJSCodeGenerator::changedRegisterVariable() const
{
    const QQmlJSScope::ConstPtr storedType = m_state.changedRegister().storedType();
    if (storedType.isNull())
        return QString();

    return m_registerVariables.value(RegisterVariablesKey {
        storedType->internalName(),
        m_state.changedRegisterIndex()
    }).variableName;
}

QQmlJSRegisterContent QQmlJSCodeGenerator::registerType(int index) const
{
    auto it = m_state.registers.find(index);
    if (it != m_state.registers.end())
        return it.value().content;

    return QQmlJSRegisterContent();
}

QString QQmlJSCodeGenerator::conversion(
        const QQmlJSRegisterContent &from, const QQmlJSRegisterContent &to, const QString &variable)
{
    const QQmlJSScope::ConstPtr contained = m_typeResolver->containedType(to);

    // If both types are stored in QJSPrimitiveValue we coerce using QJSPrimitiveValue
    if (m_typeResolver->registerIsStoredIn(from, m_typeResolver->jsPrimitiveType())
            && m_typeResolver->registerIsStoredIn(to, m_typeResolver->jsPrimitiveType())) {
        if (m_typeResolver->equals(contained, m_typeResolver->jsPrimitiveType()))
            return variable;

        const QString conversion = variable + u".to<QJSPrimitiveValue::%1>()"_s;
        if (m_typeResolver->equals(contained, m_typeResolver->boolType()))
            return conversion.arg(u"Boolean"_s);
        if (m_typeResolver->isIntegral(to))
            return conversion.arg(u"Integer"_s);
        if (m_typeResolver->equals(contained, m_typeResolver->realType()))
            return conversion.arg(u"Double"_s);
        if (m_typeResolver->equals(contained, m_typeResolver->stringType()))
            return conversion.arg(u"String"_s);
        reject(u"Conversion of QJSPrimitiveValue to "_s + contained->internalName());
    }

    if (m_typeResolver->registerIsStoredIn(to, contained)
            || m_typeResolver->isNumeric(to.storedType())
            || to.storedType()->isReferenceType()
            || m_typeResolver->registerContains(from, contained)) {
        // If:
        // * the output is not actually wrapped at all, or
        // * the output is stored in a numeric type (as there are no internals to a number), or
        // * the output is a QObject pointer, or
        // * we merely wrap the value into a new container,
        // we can convert by stored type.
        return convertStored(from.storedType(), to.storedType(), variable);
    } else {
        return convertContained(from, to, variable);
    }
}

QString QQmlJSCodeGenerator::convertStored(
        const QQmlJSScope::ConstPtr &from, const QQmlJSScope::ConstPtr &to, const QString &variable)
{
    // TODO: most values can be moved, which is much more efficient with the common types.
    //       add a move(from, to, variable) function that implements the moves.
    Q_ASSERT(!to->isComposite()); // We cannot directly convert to composites.

    const auto jsValueType = m_typeResolver->jsValueType();
    const auto varType = m_typeResolver->varType();
    const auto jsPrimitiveType = m_typeResolver->jsPrimitiveType();
    const auto boolType = m_typeResolver->boolType();

    auto zeroBoolOrInt = [&](const QQmlJSScope::ConstPtr &to) {
        if (m_typeResolver->equals(to, boolType))
            return u"false"_s;
        if (m_typeResolver->isSignedInteger(to))
            return u"0"_s;
        if (m_typeResolver->isUnsignedInteger(to))
            return u"0u"_s;
        return QString();
    };

    if (m_typeResolver->equals(from, m_typeResolver->voidType())) {
        if (to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            return u"static_cast<"_s + to->internalName() + u" *>(nullptr)"_s;
        const QString zero = zeroBoolOrInt(to);
        if (!zero.isEmpty())
            return zero;
        if (m_typeResolver->equals(to, m_typeResolver->floatType()))
            return u"std::numeric_limits<float>::quiet_NaN()"_s;
        if (m_typeResolver->equals(to, m_typeResolver->realType()))
            return u"std::numeric_limits<double>::quiet_NaN()"_s;
        if (m_typeResolver->equals(to, m_typeResolver->stringType()))
            return QQmlJSUtils::toLiteral(u"undefined"_s);
        if (m_typeResolver->equals(to, m_typeResolver->varType()))
            return u"QVariant()"_s;
        if (m_typeResolver->equals(to, m_typeResolver->jsValueType()))
            return u"QJSValue();"_s;
        if (m_typeResolver->equals(to, m_typeResolver->jsPrimitiveType()))
            return u"QJSPrimitiveValue()"_s;
        if (m_typeResolver->equals(from, to))
            return QString();
    }

    if (m_typeResolver->equals(from, m_typeResolver->nullType())) {
        if (to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            return u"static_cast<"_s + to->internalName() + u" *>(nullptr)"_s;
        if (m_typeResolver->equals(to, jsValueType))
            return u"QJSValue(QJSValue::NullValue)"_s;
        if (m_typeResolver->equals(to, jsPrimitiveType))
            return u"QJSPrimitiveValue(QJSPrimitiveNull())"_s;
        if (m_typeResolver->equals(to, varType))
            return u"QVariant::fromValue<std::nullptr_t>(nullptr)"_s;
        const QString zero = zeroBoolOrInt(to);
        if (!zero.isEmpty())
            return zero;
        if (m_typeResolver->equals(to, m_typeResolver->floatType()))
            return u"0.0f"_s;
        if (m_typeResolver->equals(to, m_typeResolver->realType()))
            return u"0.0"_s;
        if (m_typeResolver->equals(to, m_typeResolver->stringType()))
            return QQmlJSUtils::toLiteral(u"null"_s);
        if (m_typeResolver->equals(from, to))
            return QString();
        reject(u"Conversion from null to %1"_s.arg(to->internalName()));
    }

    if (m_typeResolver->equals(from, to))
        return variable;

    if (from->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
        if (to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
            // Compare internalName here. The same C++ type can be exposed muliple times in
            // different QML types. However, the C++ names have to be unique. We can always
            // static_cast to those.

            for (QQmlJSScope::ConstPtr base = from; base; base = base->baseType()) {
                // We still have to cast as other execution paths may result in different types.
                if (base->internalName() == to->internalName())
                    return u"static_cast<"_s + to->internalName() + u" *>("_s + variable + u')';
            }
            for (QQmlJSScope::ConstPtr base = to; base; base = base->baseType()) {
                if (base->internalName() == from->internalName())
                    return u"static_cast<"_s + to->internalName() + u" *>("_s + variable + u')';
            }
        } else if (m_typeResolver->equals(to, m_typeResolver->boolType())) {
            return u'(' + variable + u" != nullptr)"_s;
        }
    }

    auto isJsValue = [&](const QQmlJSScope::ConstPtr &candidate) {
        return m_typeResolver->equals(candidate, jsValueType) || candidate->isScript();
    };

    if (isJsValue(from) && isJsValue(to))
        return variable;

    const auto isBoolOrNumber = [&](const QQmlJSScope::ConstPtr &type) {
        return m_typeResolver->isNumeric(m_typeResolver->globalType(type))
                || m_typeResolver->equals(type, m_typeResolver->boolType())
                || type->scopeType() == QQmlSA::ScopeType::EnumScope;
    };

    if (m_typeResolver->equals(from, m_typeResolver->realType())
            || m_typeResolver->equals(from, m_typeResolver->floatType())) {
        if (m_typeResolver->isSignedInteger(to))
            return u"QJSNumberCoercion::toInteger("_s + variable + u')';
        if (m_typeResolver->isUnsignedInteger(to))
            return u"uint(QJSNumberCoercion::toInteger("_s + variable + u"))"_s;
        if (m_typeResolver->equals(to, m_typeResolver->boolType()))
            return u'(' + variable + u" && !std::isnan("_s + variable + u"))"_s;
    }

    if (isBoolOrNumber(from) && isBoolOrNumber(to))
        return to->internalName() + u'(' + variable + u')';


    if (m_typeResolver->equals(from, jsPrimitiveType)) {
        if (m_typeResolver->equals(to, m_typeResolver->realType()))
            return variable + u".toDouble()"_s;
        if (m_typeResolver->equals(to, boolType))
            return variable + u".toBoolean()"_s;
        if (m_typeResolver->isSignedInteger(to))
            return variable + u".toInteger()"_s;
        if (m_typeResolver->isUnsignedInteger(to))
            return u"uint("_s + variable + u".toInteger())"_s;
        if (m_typeResolver->equals(to, m_typeResolver->stringType()))
            return variable + u".toString()"_s;
        if (m_typeResolver->equals(to, jsValueType))
            return u"QJSValue(QJSPrimitiveValue("_s + variable + u"))"_s;
        if (m_typeResolver->equals(to, varType))
            return variable + u".toVariant()"_s;
        if (to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            return u"static_cast<"_s + to->internalName() + u" *>(nullptr)"_s;
    }

    if (isJsValue(from)) {
        if (m_typeResolver->equals(to, jsPrimitiveType))
            return variable + u".toPrimitive()"_s;
        if (m_typeResolver->equals(to, varType))
            return variable + u".toVariant(QJSValue::RetainJSObjects)"_s;
        return u"qjsvalue_cast<"_s + castTargetName(to) + u">("_s + variable + u')';
    }

    if (m_typeResolver->equals(to, jsPrimitiveType)) {
        // null and undefined have been handled above already
        Q_ASSERT(!m_typeResolver->equals(from, m_typeResolver->nullType()));
        Q_ASSERT(!m_typeResolver->equals(from, m_typeResolver->voidType()));

        if (m_typeResolver->equals(from, m_typeResolver->boolType())
                || m_typeResolver->equals(from, m_typeResolver->int32Type())
                || m_typeResolver->equals(from, m_typeResolver->realType())
                || m_typeResolver->equals(from, m_typeResolver->stringType())) {
            return u"QJSPrimitiveValue("_s + variable + u')';
        } else if (m_typeResolver->isSignedInteger(from)
                   || m_typeResolver->equals(from, m_typeResolver->uint16Type())
                   || m_typeResolver->equals(from, m_typeResolver->uint8Type())) {
            return u"QJSPrimitiveValue(int("_s + variable + u"))"_s;
        } else if (m_typeResolver->isNumeric(from)) {
            return u"QJSPrimitiveValue(double("_s + variable + u"))"_s;
        }
    }

    if (m_typeResolver->equals(to, jsValueType))
        return u"aotContext->engine->toScriptValue("_s + variable + u')';

    if (m_typeResolver->equals(from, varType)) {
        if (m_typeResolver->equals(to, m_typeResolver->listPropertyType()))
            return u"QQmlListReference("_s + variable + u", aotContext->qmlEngine())"_s;
        return u"aotContext->engine->fromVariant<"_s + castTargetName(to) + u">("_s
                + variable + u')';
    }

    if (m_typeResolver->equals(to, varType))
        return u"QVariant::fromValue("_s + variable + u')';

    if (m_typeResolver->equals(from, m_typeResolver->urlType())
            && m_typeResolver->equals(to, m_typeResolver->stringType())) {
        return variable + u".toString()"_s;
    }

    if (m_typeResolver->equals(from, m_typeResolver->stringType())
            && m_typeResolver->equals(to, m_typeResolver->urlType())) {
        return u"QUrl("_s + variable + u')';
    }

    if (m_typeResolver->equals(from, m_typeResolver->byteArrayType())
            && m_typeResolver->equals(to, m_typeResolver->stringType())) {
        return u"QString::fromUtf8("_s + variable + u')';
    }

    if (m_typeResolver->equals(from, m_typeResolver->stringType())
            && m_typeResolver->equals(to, m_typeResolver->byteArrayType())) {
        return variable + u".toUtf8()"_s;
    }

    for (const auto &originType : {
         m_typeResolver->dateTimeType(),
         m_typeResolver->dateType(),
         m_typeResolver->timeType()}) {
        if (m_typeResolver->equals(from, originType)) {
            for (const auto &targetType : {
                 m_typeResolver->dateTimeType(),
                 m_typeResolver->dateType(),
                 m_typeResolver->timeType(),
                 m_typeResolver->stringType()}) {
                if (m_typeResolver->equals(to, targetType)) {
                    return u"aotContext->engine->coerceValue<%1, %2>(%3)"_s.arg(
                                originType->internalName(), targetType->internalName(), variable);
                }
            }
            break;
        }
    }

    const auto retrieveFromPrimitive = [&](
            const QQmlJSScope::ConstPtr &type, const QString &expression) -> QString
    {
        if (m_typeResolver->equals(type, m_typeResolver->boolType()))
            return expression + u".toBoolean()"_s;
        if (m_typeResolver->isSignedInteger(type))
            return expression + u".toInteger()"_s;
        if (m_typeResolver->isUnsignedInteger(type))
            return u"uint("_s + expression + u".toInteger())"_s;
        if (m_typeResolver->equals(type, m_typeResolver->realType()))
            return expression + u".toDouble()"_s;
        if (m_typeResolver->equals(type, m_typeResolver->floatType()))
            return u"float("_s + expression + u".toDouble())"_s;
        if (m_typeResolver->equals(type, m_typeResolver->stringType()))
            return expression + u".toString()"_s;
        return QString();
    };

    if (!retrieveFromPrimitive(from, u"x"_s).isEmpty()) {
        const QString retrieve = retrieveFromPrimitive(
                    to, convertStored(from, m_typeResolver->jsPrimitiveType(), variable));
        if (!retrieve.isEmpty())
            return retrieve;
    }

    if (from->isReferenceType() && m_typeResolver->equals(to, m_typeResolver->stringType())) {
        return u"aotContext->engine->coerceValue<"_s + castTargetName(from) + u", "
                + castTargetName(to) + u">("_s + variable + u')';
    }

    // Any value type is a non-null JS 'object' and therefore coerces to true.
    if (m_typeResolver->equals(to, m_typeResolver->boolType())) {
        // All the interesting cases are already handled above:
        Q_ASSERT(!m_typeResolver->equals(from, m_typeResolver->nullType()));
        Q_ASSERT(!m_typeResolver->equals(from, m_typeResolver->voidType()));
        Q_ASSERT(retrieveFromPrimitive(from, u"x"_s).isEmpty());
        Q_ASSERT(!isBoolOrNumber(from));

        return u"true"_s;
    }

    if (m_typeResolver->areEquivalentLists(from, to))
        return variable;

    if (from->isListProperty()
            && to->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence
            && to->valueType()->isReferenceType()
            && !to->isListProperty()) {
        return variable + u".toList<"_s + to->internalName() + u">()"_s;
    }

    bool isExtension = false;
    if (const auto ctor = m_typeResolver->selectConstructor(to, from, &isExtension); ctor.isValid()) {
        const auto argumentTypes = ctor.parameters();
        return (isExtension ? to->extensionType().scope->internalName() : to->internalName())
                + u"("_s + convertStored(from, argumentTypes[0].type(), variable) + u")"_s;
    }

    // TODO: add more conversions

    reject(u"conversion from "_s + from->internalName() + u" to "_s + to->internalName());
    return QString();
}

QString QQmlJSCodeGenerator::convertContained(const QQmlJSRegisterContent &from, const QQmlJSRegisterContent &to, const QString &variable)
{
    const QQmlJSScope::ConstPtr containedFrom = m_typeResolver->containedType(from);
    const QQmlJSScope::ConstPtr containedTo = m_typeResolver->containedType(to);

    // Those should be handled before, by convertStored().
    Q_ASSERT(!to.storedType()->isReferenceType());
    Q_ASSERT(!m_typeResolver->registerIsStoredIn(to, containedTo));
    Q_ASSERT(!m_typeResolver->isIntegral(from.storedType()));
    Q_ASSERT(!m_typeResolver->equals(containedFrom, containedTo));

    if (!m_typeResolver->registerIsStoredIn(to, m_typeResolver->varType()) &&
            !m_typeResolver->registerIsStoredIn(to, m_typeResolver->jsPrimitiveType())) {
        reject(u"internal conversion into unsupported wrapper type."_s);
    }

    bool isExtension = false;
    if (const auto ctor = m_typeResolver->selectConstructor(
                containedTo, containedFrom, &isExtension); ctor.isValid()) {
        const auto argumentTypes = ctor.parameters();
        const QQmlJSScope::ConstPtr argumentType = argumentTypes[0].type();

        // We need to store the converted argument in a temporary
        // because it might not be an lvalue.

        QString input;
        QString argPointer;

        if (m_typeResolver->equals(argumentType, containedFrom)) {
            input = variable;
            argPointer = contentPointer(from, u"arg"_s);
        } else {
            const QQmlJSRegisterContent argument
                    = m_typeResolver->globalType(argumentType)
                    .storedIn(m_typeResolver->genericType(argumentType));
            input = conversion(from, argument, variable);
            argPointer = contentPointer(argument, u"arg"_s);
        }

        return u"[&](){ auto arg = " + input
                + u"; return aotContext->constructValueType("_s + metaType(containedTo)
                + u", "_s + metaObject(
                    isExtension ? containedTo->extensionType().scope : containedTo)
                + u", "_s + QString::number(int(ctor.constructorIndex()))
                + u", "_s + argPointer + u"); }()"_s;
    }

    reject(u"internal conversion with incompatible or ambiguous types"_s);
    return QString();
}

void QQmlJSCodeGenerator::reject(const QString &thing)
{
    setError(u"Cannot generate efficient code for %1"_s.arg(thing));
}

QQmlJSCodeGenerator::AccumulatorConverter::AccumulatorConverter(QQmlJSCodeGenerator *generator)
    : accumulatorOut(generator->m_state.accumulatorOut())
    , accumulatorVariableIn(generator->m_state.accumulatorVariableIn)
    , accumulatorVariableOut(generator->m_state.accumulatorVariableOut)
    , generator(generator)
{
    if (accumulatorVariableOut.isEmpty())
        return;

    const QQmlJSTypeResolver *resolver = generator->m_typeResolver;
    const QQmlJSScope::ConstPtr origContained = resolver->originalContainedType(accumulatorOut);
    const QQmlJSScope::ConstPtr stored = accumulatorOut.storedType();
    const QQmlJSScope::ConstPtr origStored = resolver->originalType(stored);

    // If the stored type differs or if we store in QVariant and the contained type differs,
    // then we have to use a temporary ...
    if (!resolver->equals(origStored, stored)
        || (!resolver->equals(origContained, resolver->containedType(accumulatorOut))
            && resolver->equals(stored, resolver->varType()))) {

        const bool storable = isTypeStorable(resolver, origStored);
        generator->m_state.accumulatorVariableOut = storable ? u"retrieved"_s : QString();
        generator->m_state.setRegister(Accumulator, resolver->original(accumulatorOut));
        generator->m_body += u"{\n"_s;
        if (storable) {
            generator->m_body += origStored->augmentedInternalName() + u' '
                    + generator->m_state.accumulatorVariableOut + u";\n";
        }
    } else if (generator->m_state.accumulatorVariableIn == generator->m_state.accumulatorVariableOut
               && generator->m_state.readsRegister(Accumulator)
               && resolver->registerIsStoredIn(
                   generator->m_state.accumulatorOut(), resolver->varType())) {
        // If both m_state.accumulatorIn and m_state.accumulatorOut are QVariant, we will need to
        // prepare the output QVariant, and afterwards use the input variant. Therefore we need to
        // move the input out of the way first.
        generator->m_state.accumulatorVariableIn
                = generator->m_state.accumulatorVariableIn + u"_moved"_s;
        generator->m_body += u"{\n"_s;
        generator->m_body += u"QVariant "_s + generator->m_state.accumulatorVariableIn
                + u" = std::move("_s + generator->m_state.accumulatorVariableOut + u");\n"_s;
    }
}

QQmlJSCodeGenerator::AccumulatorConverter::~AccumulatorConverter()
{
    if (accumulatorVariableOut != generator->m_state.accumulatorVariableOut) {
        generator->m_body += accumulatorVariableOut + u" = "_s + generator->conversion(
                    generator->m_state.accumulatorOut(), accumulatorOut,
                    generator->m_state.accumulatorVariableOut) + u";\n"_s;
        generator->m_body += u"}\n"_s;
        generator->m_state.setRegister(Accumulator, accumulatorOut);
        generator->m_state.accumulatorVariableOut = accumulatorVariableOut;
    } else if (accumulatorVariableIn != generator->m_state.accumulatorVariableIn) {
        generator->m_body += u"}\n"_s;
        generator->m_state.accumulatorVariableIn = accumulatorVariableIn;
    }
}


QT_END_NAMESPACE
