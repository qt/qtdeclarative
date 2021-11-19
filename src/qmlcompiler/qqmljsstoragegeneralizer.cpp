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

#include "qqmljsstoragegeneralizer_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \internal
 * \class QQmlJSStorageGeneralizer
 *
 * The QQmlJSStorageGeneralizer is a compile pass that changes all type
 * annotations and the function signature to use a generic storage type like
 * QVariant or QObject*. This is necessary if we cannot rely on the original
 * type to be immediately accessible, for example because we cannot include its
 * header.
 *
 * QQmlJSStorageGeneralizer does not have to use the byte code at all but
 * operates only on the annotations and the function description.
 */

QQmlJSCompilePass::InstructionAnnotations QQmlJSStorageGeneralizer::run(
        InstructionAnnotations annotations, Function *function,
        QQmlJS::DiagnosticMessage *error)
{
    m_error = error;

    if (QQmlJSScope::ConstPtr &returnType = function->returnType) {
        if (QQmlJSScope::ConstPtr stored = m_typeResolver->genericType(
                    returnType, QQmlJSTypeResolver::ComponentIsGeneric::Yes)) {
            returnType = stored;
        } else {
            setError(QStringLiteral("Cannot store the return type %1.")
                     .arg(returnType->internalName(), 0));
            return InstructionAnnotations();
        }
    }

    for (QQmlJSScope::ConstPtr &argument : function->argumentTypes) {
        Q_ASSERT(argument);
        if (QQmlJSScope::ConstPtr stored = m_typeResolver->genericType(
                    argument, QQmlJSTypeResolver::ComponentIsGeneric::Yes)) {
            argument = std::move(stored);
        } else {
            setError(QStringLiteral("Cannot store the argument type %1.")
                     .arg(argument->internalName(), 0));
            return InstructionAnnotations();
        }
    }

    const auto transformRegisters = [&](QHash<int, QQmlJSRegisterContent> &registers, int offset) {
        for (auto j = registers.begin(), jEnd = registers.end(); j != jEnd; ++j) {
            const QQmlJSRegisterContent &content = *j;
            if (QQmlJSScope::ConstPtr specific = content.storedType()) {
                if (QQmlJSScope::ConstPtr generic = m_typeResolver->genericType(specific)) {
                    *j = content.storedIn(generic);
                } else {
                    setError(QStringLiteral("Cannot store the register type %1.")
                             .arg(specific->internalName()), offset);
                    return false;
                }
            }
        }
        return true;
    };

    for (auto i = annotations.begin(), iEnd = annotations.end(); i != iEnd; ++i) {
        if (!transformRegisters(i->registers, i.key()))
            return InstructionAnnotations();
        if (!transformRegisters(i->expectedTargetTypesBeforeJump, i.key()))
            return InstructionAnnotations();
    }

    return annotations;
}

QT_END_NAMESPACE
