// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsshadowcheck_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
 * \internal
 * \class QQmlJSShadowCheck
 *
 * This pass looks for possible shadowing when accessing members of QML-exposed
 * types. A member can be shadowed if a non-final property is re-declared in a
 * derived class. As the QML engine will always pick up the most derived variant
 * of that property, we cannot rely on any property of a type to be actually
 * accessible, unless one of a few special cases holds:
 *
 * 1. We are dealing with a direct scope lookup, without an intermediate object.
 *    Such lookups are protected from shadowing. For example "property int a: b"
 *    always works.
 * 2. The object we are retrieving the property from is identified by an ID, or
 *    an attached property or a singleton. Such objects cannot be replaced.
 *    Therefore we can be sure to see all the type information at compile time.
 * 3. The property is declared final.
 * 4. The object we are retrieving the property from is a value type. Value
 *    types cannot be used polymorphically.
 *
 * If the property is potentially shadowed, we can still retrieve it, but we
 * don't know its type. We should assume "var" then.
 *
 * All of the above also holds for methods. There we have to transform the
 * arguments and return types into "var".
 */

void QQmlJSShadowCheck::run(
        InstructionAnnotations *annotations, const Function *function,
        QQmlJS::DiagnosticMessage *error)
{
    m_annotations = annotations;
    m_function = function;
    m_error = error;
    m_state = initialState(function);
    decode(m_function->code.constData(), static_cast<uint>(m_function->code.size()));

    for (const auto &store : m_resettableStores)
        checkResettable(store.accumulatorIn, store.instructionOffset);

    // Re-check all base types. We may have made them var after detecting them.
    for (const auto &base : m_baseTypes) {
        if (checkBaseType(base) == Shadowable)
            break;
    }
}

void QQmlJSShadowCheck::generate_LoadProperty(int nameIndex)
{
    if (!m_state.readsRegister(Accumulator))
        return; // enum lookup cannot be shadowed.

    auto accumulatorIn = m_state.registers.find(Accumulator);
    if (accumulatorIn != m_state.registers.end()) {
        checkShadowing(
                accumulatorIn.value().content, m_jsUnitGenerator->stringForIndex(nameIndex),
                Accumulator);
    }
}

void QQmlJSShadowCheck::generate_GetLookup(int index)
{
    if (!m_state.readsRegister(Accumulator))
        return; // enum lookup cannot be shadowed.

    auto accumulatorIn = m_state.registers.find(Accumulator);
    if (accumulatorIn != m_state.registers.end()) {
        checkShadowing(
                accumulatorIn.value().content, m_jsUnitGenerator->lookupName(index), Accumulator);
    }
}

void QQmlJSShadowCheck::generate_GetOptionalLookup(int index, int offset)
{
    Q_UNUSED(offset);
    generate_GetLookup(index);
}

void QQmlJSShadowCheck::handleStore(int base, const QString &memberName)
{
    const int instructionOffset = currentInstructionOffset();
    const QQmlJSRegisterContent &readAccumulator
            = (*m_annotations)[instructionOffset].readRegisters[Accumulator].content;
    const auto baseType = m_state.registers[base].content;

    // If the accumulator is already read as var, we don't have to do anything.
    if (m_typeResolver->registerContains(readAccumulator, m_typeResolver->varType())) {
        if (checkBaseType(baseType) == NotShadowable)
            m_baseTypes.append(baseType);
        return;
    }

    if (checkShadowing(baseType, memberName, base) == Shadowable)
        return;

    // If the property isn't shadowable, we have to turn the read register into
    // var if the accumulator can hold undefined. This has to be done in a second pass
    // because the accumulator may still turn into var due to its own shadowing.
    const QQmlJSRegisterContent member = m_typeResolver->memberType(baseType, memberName);
    if (member.isProperty())
        m_resettableStores.append({m_state.accumulatorIn(), instructionOffset});
}

void QQmlJSShadowCheck::generate_StoreProperty(int nameIndex, int base)
{
    handleStore(base, m_jsUnitGenerator->stringForIndex(nameIndex));
}

void QQmlJSShadowCheck::generate_SetLookup(int index, int base)
{
    handleStore(base, m_jsUnitGenerator->lookupName(index));
}

void QQmlJSShadowCheck::generate_CallProperty(int nameIndex, int base, int argc, int argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    checkShadowing(m_state.registers[base].content, m_jsUnitGenerator->lookupName(nameIndex), base);
}

void QQmlJSShadowCheck::generate_CallPropertyLookup(int nameIndex, int base, int argc, int argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    checkShadowing(m_state.registers[base].content, m_jsUnitGenerator->lookupName(nameIndex), base);
}

QV4::Moth::ByteCodeHandler::Verdict QQmlJSShadowCheck::startInstruction(QV4::Moth::Instr::Type)
{
    m_state = nextStateFromAnnotations(m_state, *m_annotations);
    return (m_state.hasSideEffects() || m_state.changedRegisterIndex() != InvalidRegister)
            ? ProcessInstruction
            : SkipInstruction;
}

void QQmlJSShadowCheck::endInstruction(QV4::Moth::Instr::Type)
{
}

QQmlJSShadowCheck::Shadowability QQmlJSShadowCheck::checkShadowing(
        const QQmlJSRegisterContent &baseType, const QString &memberName, int baseRegister)
{
    if (checkBaseType(baseType) == Shadowable)
        return Shadowable;
    else
        m_baseTypes.append(baseType);

    if (baseType.storedType()->accessSemantics() != QQmlJSScope::AccessSemantics::Reference)
        return NotShadowable;

    switch (baseType.variant()) {
    case QQmlJSRegisterContent::ExtensionObjectProperty:
    case QQmlJSRegisterContent::ExtensionScopeProperty:
    case QQmlJSRegisterContent::MethodReturnValue:
    case QQmlJSRegisterContent::ObjectProperty:
    case QQmlJSRegisterContent::ScopeProperty:
    case QQmlJSRegisterContent::Unknown: {
        const QQmlJSRegisterContent member = m_typeResolver->memberType(baseType, memberName);

        // You can have something like parent.QtQuick.Screen.pixelDensity
        // In that case "QtQuick" cannot be resolved as member type and we would later have to look
        // for "QtQuick.Screen" instead. However, you can only do that with attached properties and
        // those are not shadowable.
        if (!member.isValid()) {
            Q_ASSERT(m_typeResolver->isPrefix(memberName));
            return NotShadowable;
        }

        if (member.isProperty()) {
            if (member.property().isFinal())
                return NotShadowable; // final properties can't be shadowed
        } else if (!member.isMethod()) {
            return NotShadowable; // Only properties and methods can be shadowed
        }

        m_logger->log(
                u"Member %1 of %2 can be shadowed"_s.arg(memberName, baseType.descriptiveName()),
                qmlCompiler, currentSourceLocation());

        // Make it "var". We don't know what it is.
        const QQmlJSScope::ConstPtr varType = m_typeResolver->varType();
        const QQmlJSRegisterContent varContent = m_typeResolver->globalType(varType);
        InstructionAnnotation &currentAnnotation = (*m_annotations)[currentInstructionOffset()];

        if (currentAnnotation.changedRegisterIndex != InvalidRegister) {
            m_typeResolver->adjustOriginalType(
                    currentAnnotation.changedRegister.storedType(), varType);
            m_typeResolver->adjustOriginalType(
                    m_typeResolver->containedType(currentAnnotation.changedRegister), varType);
            m_adjustedTypes.insert(currentAnnotation.changedRegister);
        }

        for (auto it = currentAnnotation.readRegisters.begin(),
             end = currentAnnotation.readRegisters.end();
             it != end; ++it) {
            if (it.key() != baseRegister)
                it->second.content = m_typeResolver->convert(it->second.content, varContent);
        }
        return Shadowable;
    }
    default:
        // In particular ObjectById is fine as that cannot change into something else
        // Singleton should also be fine, unless the factory function creates an object
        // with different property types than the declared class.
        return NotShadowable;
    }
}

void QQmlJSShadowCheck::checkResettable(
        const QQmlJSRegisterContent &accumulatorIn, int instructionOffset)
{
    const QQmlJSScope::ConstPtr varType = m_typeResolver->varType();

    // The stored type is not necessarily updated by the shadow check, but it
    // will be in the basic blocks pass. For the purpose of adjusting newly
    // shadowable types we can ignore it. We only want to know if any of the
    // contents can hold undefined.
    if (!m_typeResolver->canHoldUndefined(accumulatorIn.storedIn(varType)))
        return;

    const QQmlJSRegisterContent varContent = m_typeResolver->globalType(varType);

    QQmlJSRegisterContent &readAccumulator
            = (*m_annotations)[instructionOffset].readRegisters[Accumulator].content;
    readAccumulator = m_typeResolver->convert(readAccumulator, varContent);
}

QQmlJSShadowCheck::Shadowability QQmlJSShadowCheck::checkBaseType(
        const QQmlJSRegisterContent &baseType)
{
    if (!m_adjustedTypes.contains(baseType))
        return NotShadowable;
    setError(u"Cannot use shadowable base type for further lookups: %1"_s.arg(baseType.descriptiveName()));
    return Shadowable;
}

QT_END_NAMESPACE
