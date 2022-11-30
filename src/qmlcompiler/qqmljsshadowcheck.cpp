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
 */

void QQmlJSShadowCheck::run(
        const InstructionAnnotations *annotations, const Function *function,
        QQmlJS::DiagnosticMessage *error)
{
    m_annotations = annotations;
    m_function = function;
    m_error = error;
    m_state = initialState(function);
    decode(m_function->code.constData(), static_cast<uint>(m_function->code.size()));
}

void QQmlJSShadowCheck::generate_LoadProperty(int nameIndex)
{
    if (!m_state.readsRegister(Accumulator))
        return; // enum lookup cannot be shadowed.

    auto accumulatorIn = m_state.registers.find(Accumulator);
    if (accumulatorIn != m_state.registers.end())
        checkShadowing(accumulatorIn.value(), m_jsUnitGenerator->stringForIndex(nameIndex));
}

void QQmlJSShadowCheck::generate_GetLookup(int index)
{
    if (!m_state.readsRegister(Accumulator))
        return; // enum lookup cannot be shadowed.

    auto accumulatorIn = m_state.registers.find(Accumulator);
    if (accumulatorIn != m_state.registers.end())
        checkShadowing(accumulatorIn.value(), m_jsUnitGenerator->lookupName(index));
}

void QQmlJSShadowCheck::generate_StoreProperty(int nameIndex, int base)
{
    checkShadowing(m_state.registers[base], m_jsUnitGenerator->stringForIndex(nameIndex));
}

void QQmlJSShadowCheck::generate_SetLookup(int index, int base)
{
    checkShadowing(m_state.registers[base], m_jsUnitGenerator->lookupName(index));
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

void QQmlJSShadowCheck::checkShadowing(
        const QQmlJSRegisterContent &baseType, const QString &memberName)
{
    if (baseType.storedType()->accessSemantics() != QQmlJSScope::AccessSemantics::Reference)
        return;

    switch (baseType.variant()) {
    case QQmlJSRegisterContent::ObjectProperty:
    case QQmlJSRegisterContent::ExtensionObjectProperty:
    case QQmlJSRegisterContent::ScopeProperty:
    case QQmlJSRegisterContent::ExtensionScopeProperty: {
        const QQmlJSRegisterContent member = m_typeResolver->memberType(baseType, memberName);

        // You can have something like parent.QtQuick.Screen.pixelDensity
        // In that case "QtQuick" cannot be resolved as member type and we would later have to look
        // for "QtQuick.Screen" instead. However, you can only do that with attached properties and
        // those are not shadowable.
        if (!member.isValid()) {
            Q_ASSERT(m_typeResolver->isPrefix(memberName));
            return;
        }

        if (member.isProperty()) {
            if (member.property().isFinal())
                return; // final properties can't be shadowed
        } else if (!member.isMethod()) {
            return; // Only properties and methods can be shadowed
        }

        setError(u"Member %1 of %2 can be shadowed"_s
                         .arg(memberName, m_state.accumulatorIn().descriptiveName()));
        return;
    }
    default:
        // In particular ObjectById is fine as that cannot change into something else
        // Singleton should also be fine, unless the factory function creates an object
        // with different property types than the declared class.
        return;
    }
}

QT_END_NAMESPACE
