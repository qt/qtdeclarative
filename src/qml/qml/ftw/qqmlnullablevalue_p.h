// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLNULLABLEVALUE_P_H
#define QQMLNULLABLEVALUE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

template<typename T>
struct QQmlNullableValue
{
    QQmlNullableValue()
    : value(T()) {}
    QQmlNullableValue(const QQmlNullableValue<T> &o)
    : isNull(o.isNull), value(o.value) {}
    QQmlNullableValue(const T &t)
    : isNull(false), value(t) {}
    QQmlNullableValue<T> &operator=(const T &t)
    { isNull = false; value = t; return *this; }
    QQmlNullableValue<T> &operator=(const QQmlNullableValue<T> &o)
    { isNull = o.isNull; value = o.value; return *this; }
    operator T() const { return value; }

    void invalidate() { isNull = true; }
    bool isValid() const { return !isNull; }
    bool isNull = true;
    T value;
};

QT_END_NAMESPACE

#endif // QQMLNULLABLEVALUE_P_H
