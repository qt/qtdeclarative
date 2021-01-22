/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
