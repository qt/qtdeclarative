// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qquickliteralbindingcheck_p.h"
#include "qquickvaluetypefromstringcheck_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlJSStructuredTypeError QQuickLiteralBindingCheck::check(const QString &typeName,
                                                           const QString &value) const
{
    return QQuickValueTypeFromStringCheck::check(typeName, value);
}

QT_END_NAMESPACE
