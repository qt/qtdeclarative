// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qquickvaluetypefromstringcheck_p.h"

#include <private/qqmlstringconverters_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlJSStructuredTypeError QQuickValueTypeFromStringCheck::check(const QString &typeName,
                                                                const QString &value)
{
    if (typeName == u"QVector2D") {
        std::array<double, 2> parameters;
        if (!QQmlStringConverters::isValidNumberString<2, u','>(value, &parameters))
            return QQmlJSStructuredTypeError::withInvalidString();

        const auto result = QQmlJSStructuredTypeError::fromSuggestedString(
                u"({ x: %1, y: %2 })"_s.arg(parameters[0]).arg(parameters[1]));
        return result;
    } else if (typeName == u"QVector3D") {
        std::array<double, 3> parameters;
        if (!QQmlStringConverters::isValidNumberString<3, u',', u','>(value, &parameters))
            return QQmlJSStructuredTypeError::withInvalidString();

        const auto result = QQmlJSStructuredTypeError::fromSuggestedString(
                u"({ x: %1, y: %2, z: %3 })"_s.arg(parameters[0])
                        .arg(parameters[1])
                        .arg(parameters[2]));
        return result;
    } else if (typeName == u"QVector4D") {
        std::array<double, 4> parameters;
        if (!QQmlStringConverters::isValidNumberString<4, u',', u',', u','>(value, &parameters))
            return QQmlJSStructuredTypeError::withInvalidString();

        const auto result = QQmlJSStructuredTypeError::fromSuggestedString(
                u"({ x: %1, y: %2, z: %3, w: %4 })"_s.arg(parameters[0])
                        .arg(parameters[1])
                        .arg(parameters[2])
                        .arg(parameters[3]));
        return result;
    } else if (typeName == u"QQuaternion") {
        std::array<double, 4> parameters;
        if (!QQmlStringConverters::isValidNumberString<4, u',', u',', u','>(value, &parameters))
            return QQmlJSStructuredTypeError::withInvalidString();

        const auto result = QQmlJSStructuredTypeError::fromSuggestedString(
                u"({ scalar: %1, x: %2, y: %3, z: %4 })"_s.arg(parameters[0])
                        .arg(parameters[1])
                        .arg(parameters[2])
                        .arg(parameters[3]));
        return result;
    } else if (typeName == u"QMatrix4x4") {
        std::array<double, 16> parameters;
        if (!QQmlStringConverters::isValidNumberString<16, u',', u',', u',', u',', u',', u',', u',',
                                                       u',', u',', u',', u',', u',', u',', u',',
                                                       u','>(value, &parameters)) {
            return QQmlJSStructuredTypeError::withInvalidString();
        }

        QString construction = u"({ "_s;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                construction.append(
                        u"m%1%2: %3"_s.arg(i + 1).arg(j + 1).arg(parameters[i * 4 + j]));
                if (!(i == 3 && j == 3))
                    construction.append(u", "_s);
            }
        }
        construction.append(u" })"_s);

        const auto result = QQmlJSStructuredTypeError::fromSuggestedString(construction);
        return result;
    }

    return QQmlJSStructuredTypeError::withValidString();
}

QT_END_NAMESPACE
