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
    for (const auto &scope : visitor->literalScopesToCheck()) {
        for (const auto &binding : scope->ownPropertyBindings()) {
            if (!binding.hasLiteral())
                continue;

            const QQmlJSMetaProperty property = scope->property(binding.propertyName());
            if (property.isValid()) {
                // If the property is defined in the same scope where it is set,
                // we are in fact allowed to set it, even if it's not writable.
                if (!property.isWritable() && !scope->hasOwnProperty(binding.propertyName())) {
                    logger->logWarning(u"Cannot assign to read-only property %1"_qs
                                                 .arg(binding.propertyName()),
                                         Log_Type, binding.sourceLocation());
                    continue;
                }
                if (!resolver->canConvertFromTo(binding.literalType(), property.type())) {
                    logger->logWarning(u"Cannot assign binding of type %1 to %2"_qs
                                                 .arg(binding.literalTypeName())
                                                 .arg(property.typeName()),
                                         Log_Type, binding.sourceLocation());
                } else if (property.type() == resolver->stringType()
                           && resolver->isNumeric(binding.literalType())) {
                    logger->logWarning(u"Cannot assign a numeric constant to a string property"_qs,
                                         Log_Type, binding.sourceLocation());
                }
            }
        }
    }
}

QT_END_NAMESPACE
