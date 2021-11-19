/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmljsshadowcheck_p.h"

QT_BEGIN_NAMESPACE

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
    m_state = initialState(function, m_typeResolver);
    decode(m_function->code.constData(), static_cast<uint>(m_function->code.length()));
}

void QQmlJSShadowCheck::generate_LoadProperty(int nameIndex)
{
    checkShadowing(m_state.accumulatorIn, m_jsUnitGenerator->stringForIndex(nameIndex));
}

void QQmlJSShadowCheck::generate_GetLookup(int index)
{
    checkShadowing(m_state.accumulatorIn, m_jsUnitGenerator->lookupName(index));
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
    return ProcessInstruction;
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

        setError(u"Member %1 of %2 can be shadowed"_qs
                         .arg(memberName, m_state.accumulatorIn.descriptiveName()));
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
