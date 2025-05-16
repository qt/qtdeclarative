// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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

QT_END_NAMESPACE
