// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsstorageinitializer_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \internal
 * \class QQmlJSStorageInitializer
 *
 * The QQmlJSStorageInitializer is a compile pass that initializes the storage
 * for all register contents.
 *
 * QQmlJSStorageInitializer does not have to use the byte code at all but
 * operates only on the annotations and the function description.
 */

QQmlJSCompilePass::BlocksAndAnnotations QQmlJSStorageInitializer::run(Function *function)
{
    m_function = function;

    if (QQmlJSRegisterContent &returnType = function->returnType; returnType.isValid()) {
        if (const QQmlJSScope::ConstPtr stored
                = m_typeResolver->storedType(returnType.containedType())) {
            returnType = returnType.storedIn(m_typeResolver->trackedType(stored));
        } else {
            addError(QStringLiteral("Cannot store the return type %1.")
                             .arg(returnType.containedType()->internalName()));
            return {};
        }
    }

    const auto storeRegister = [&](QQmlJSRegisterContent &content) {
        if (!content.isValid())
            return;

        const QQmlJSScope::ConstPtr original
                = m_typeResolver->originalType(content.containedType());
        const QQmlJSScope::ConstPtr originalStored = m_typeResolver->storedType(original);
        const QQmlJSScope::ConstPtr originalTracked = m_typeResolver->trackedType(originalStored);
        content = content.storedIn(originalTracked);

        const QQmlJSScope::ConstPtr adjustedStored
                = m_typeResolver->storedType(content.containedType());

        if (!m_typeResolver->adjustTrackedType(originalTracked, adjustedStored)) {
            addError(QStringLiteral("Cannot adjust stored type for %1.")
                             .arg(content.containedType()->internalName()));
        }
    };

    const auto storeRegisters = [&](VirtualRegisters &registers) {
        for (auto j = registers.begin(), jEnd = registers.end(); j != jEnd; ++j)
            storeRegister(j.value().content);
    };

    storeRegister(function->qmlScope);

    for (QQmlJSRegisterContent &argument : function->argumentTypes) {
        Q_ASSERT(argument.isValid());
        storeRegister(argument);
    }

    for (QQmlJSRegisterContent &argument : function->registerTypes) {
        Q_ASSERT(argument.isValid());
        storeRegister(argument);
    }

    for (auto i = m_annotations.begin(), iEnd = m_annotations.end(); i != iEnd; ++i) {
        storeRegister(i->second.changedRegister);
        storeRegisters(i->second.typeConversions);
        storeRegisters(i->second.readRegisters);
    }

    return { std::move(m_basicBlocks), std::move(m_annotations) };
}

QT_END_NAMESPACE
