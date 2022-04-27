/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qqmljsliteralbindingcheck_p.h"

#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qqmljsmetatypes_p.h>

QT_BEGIN_NAMESPACE

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
                logger->log(u"Cannot assign to read-only property %1"_qs.arg(propertyName),
                            Log_Type, binding.sourceLocation());
                continue;
            }

            if (!resolver->canConvertFromTo(binding.literalType(resolver), property.type())) {
                logger->log(u"Cannot assign binding of type %1 to %2"_qs.arg(
                                    QQmlJSScope::prettyName(binding.literalTypeName()),
                                    QQmlJSScope::prettyName(property.typeName())),
                            Log_Type, binding.sourceLocation());
                continue;
            }

            if (resolver->equals(property.type(), resolver->stringType())
                       && resolver->isNumeric(binding.literalType(resolver))) {
                logger->log(u"Cannot assign a numeric constant to a string property"_qs,
                            Log_Type, binding.sourceLocation());
            }
        }
    }
}

QT_END_NAMESPACE
