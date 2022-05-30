// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsliteralbindingcheck_p.h"

#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qqmljsmetatypes_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

void QQmlJSLiteralBindingCheck::run(QQmlJSImportVisitor *visitor, QQmlJSTypeResolver *resolver)
{
    QQmlJSLogger *logger = visitor->logger();
    const auto literalScopes = visitor->literalScopesToCheck();
    for (const auto &scope : literalScopes) {
        const auto bindings = scope->ownPropertyBindings();
        for (const auto &binding : bindings) {
            if (!binding.hasLiteral())
                continue;

            const QString propertyName = binding.propertyName();
            const QQmlJSMetaProperty property = scope->property(propertyName);
            if (!property.isValid())
                continue;

            // If the property is defined in the same scope where it is set,
            // we are in fact allowed to set it, even if it's not writable.
            if (!property.isWritable() && !scope->hasOwnProperty(propertyName)) {
                logger->log(u"Cannot assign to read-only property %1"_s.arg(propertyName), qmlType,
                            binding.sourceLocation());
                continue;
            }

            if (!resolver->canConvertFromTo(binding.literalType(resolver), property.type())) {
                logger->log(u"Cannot assign binding of type %1 to %2"_s.arg(
                                    QQmlJSScope::prettyName(binding.literalTypeName()),
                                    QQmlJSScope::prettyName(property.typeName())),
                            qmlType, binding.sourceLocation());
                continue;
            }

            if (resolver->equals(property.type(), resolver->stringType())
                       && resolver->isNumeric(binding.literalType(resolver))) {
                logger->log(u"Cannot assign a numeric constant to a string property"_s, qmlType,
                            binding.sourceLocation());
            }
        }
    }
}

QT_END_NAMESPACE
