// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsliteralbindingcheck_p.h"

#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qqmljsmetatypes_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// This makes no sense, but we want to warn about things QQmlPropertyResolver complains about.
static bool canConvertForLiteralBinding(
        QQmlJSTypeResolver *resolver, const QQmlJSScope::ConstPtr &from,
        const QQmlJSScope::ConstPtr &to) {
    if (resolver->equals(from, to))
        return true;

    if (!resolver->canConvertFromTo(from, to))
        return false;

    const bool fromIsString = resolver->equals(from, resolver->stringType());

    if (resolver->equals(to, resolver->stringType())
            || resolver->equals(to, resolver->stringListType())
            || resolver->equals(to, resolver->byteArrayType())
            || resolver->equals(to, resolver->urlType())) {
        return fromIsString;
    }

    if (resolver->isNumeric(to))
        return resolver->isNumeric(from);

    if (resolver->equals(to, resolver->boolType()))
        return resolver->equals(from, resolver->boolType());

    return true;
}

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
                logger->log(u"Cannot assign to read-only property %1"_s.arg(propertyName),
                            qmlReadOnlyProperty, binding.sourceLocation());
                continue;
            }

            if (!canConvertForLiteralBinding(
                        resolver, binding.literalType(resolver), property.type())) {
                logger->log(u"Cannot assign literal of type %1 to %2"_s.arg(
                                    QQmlJSScope::prettyName(binding.literalTypeName()),
                                    QQmlJSScope::prettyName(property.typeName())),
                            qmlIncompatibleType, binding.sourceLocation());
                continue;
            }
        }
    }
}

QT_END_NAMESPACE
