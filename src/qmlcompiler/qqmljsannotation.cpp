// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljsannotation_p.h"

QT_BEGIN_NAMESPACE

bool QQmlJSAnnotation::isDeprecation() const { return name == QStringLiteral("Deprecated"); }

QQQmlJSDeprecation QQmlJSAnnotation::deprecation() const {
    Q_ASSERT(isDeprecation());
    QQQmlJSDeprecation deprecation;
    if (bindings.contains(QStringLiteral("reason"))) {

        auto reason = bindings[QStringLiteral("reason")];

        if (std::holds_alternative<QString>(reason)) {
            deprecation.reason = std::get<QString>(reason);
        }
    }

    return deprecation;
}

// hidden friend
bool operator==(const QQmlJSAnnotation &a, const QQmlJSAnnotation &b) noexcept
{
    return a.name == b.name &&
           a.bindings == b.bindings;
}

// hidden friend
size_t qHash(const QQmlJSAnnotation &annotation, size_t seed) noexcept
{
    QtPrivate::QHashCombine combine(seed);
    seed = combine(seed, annotation.name);

    for (auto it = annotation.bindings.constBegin(); it != annotation.bindings.constEnd(); ++it) {
        size_t h = combine(seed, it.key());
        // use + to keep the result independent of the ordering of the keys

        const auto &var = it.value();

        if (var.valueless_by_exception())
            continue;

        if (auto v = get_if<double>(&var))
            seed += combine(h, *v);
        else if (auto v = get_if<QString>(&var))
            seed += combine(h, *v);
        else
            Q_UNREACHABLE();
    }

    return seed;
}

QT_END_NAMESPACE
