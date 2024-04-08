// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsvaluetypefromstringcheck_p.h"
#include <private/qqmlstringconverters_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
\internal
\class ValueTypeFromStringError

Helps to differentiate between three states:
\list
\li a value type was constructed by a string in a deprecated way, e.g. "50x70" for a QSizeF
\li a value type was constructed by an invalid string in a deprecated way, e.g. "50,70" for a QSizeF
\li a value type was constructed by a string in a non-deprecated way.
\endlist
*/

/*!
\internal
Checks if known value types are being constructed in a deprecated way, and constructs the code for
the fix-it.
*/
QQmlJSStructuredTypeError QQmlJSValueTypeFromStringCheck::hasError(const QString &typeName,
                                                                  const QString &value)
{
    if (typeName == u"QPointF" || typeName == u"QPoint") {
        std::array<double, 2> numbers;
        if (!QQmlStringConverters::isValidNumberString<2, u','>(value, &numbers))
            return QQmlJSStructuredTypeError::withInvalidString();

        const auto result = QQmlJSStructuredTypeError::fromSuggestedString(
                u"({ x: %1, y: %2 })"_s.arg(numbers[0]).arg(numbers[1]));
        return result;
    } else if (typeName == u"QSizeF" || typeName == u"QSize") {
        std::array<double, 2> numbers;
        if (!QQmlStringConverters::isValidNumberString<2, u'x'>(value, &numbers))
            return QQmlJSStructuredTypeError::withInvalidString();

        const auto result = QQmlJSStructuredTypeError::fromSuggestedString(
                u"({ width: %1, height: %2 })"_s.arg(numbers[0]).arg(numbers[1]));
        return result;
    } else if (typeName == u"QRectF" || typeName == u"QRect") {
        std::array<double, 4> numbers;
        if (!QQmlStringConverters::isValidNumberString<4, u',', u',', u'x'>(value, &numbers))
            return QQmlJSStructuredTypeError::withInvalidString();

        const auto result = QQmlJSStructuredTypeError::fromSuggestedString(
                u"({ x: %1, y: %2, width: %3, height: %4 })"_s.arg(numbers[0])
                        .arg(numbers[1])
                        .arg(numbers[2])
                        .arg(numbers[3]));
        return result;
    }

    return QQmlJSStructuredTypeError::withValidString();
}

QT_END_NAMESPACE
