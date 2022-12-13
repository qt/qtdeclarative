// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WRAPWITHVARIANT_H
#define WRAPWITHVARIANT_H

#include <QtCore/qvariant.h>
#include <QtCore/qobject.h>
#include <QtQml/qjsprimitivevalue.h>
#include <QtQml/qjsvalue.h>
#include <QtQmlIntegration/qqmlintegration.h>

struct WrapWithVariant
{
    Q_GADGET
    QML_VALUE_TYPE(wrappedWithVariant)
    QML_CONSTRUCTIBLE_VALUE

    Q_PROPERTY(QVariant intJSValue READ intJSValue CONSTANT)
    Q_PROPERTY(QVariant undefinedJsValue READ undefinedJsValue CONSTANT)
    Q_PROPERTY(QVariant nullJsValue READ nullJsValue CONSTANT)
public:
    WrapWithVariant() = default;

    QVariant intJSValue() const { return QVariant::fromValue<QJSValue>(QJSValue(23)); }

    QVariant undefinedJsValue() const
    {
        return QVariant::fromValue<QJSValue>(QJSValue(QJSValue::UndefinedValue));
    }

    QVariant nullJsValue() const
    {
        return QVariant::fromValue<QJSValue>(QJSValue(QJSValue::NullValue));
    }
};

#endif // WRAPWITHVARIANT_H
