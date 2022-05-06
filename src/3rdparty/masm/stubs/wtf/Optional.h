/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#pragma once

#include <QtCore/qglobal.h>
#include <QtQml/private/qtqmlglobal_p.h>

#include <memory>
#if __cplusplus > 201402L && __has_include(<optional>)
#include <optional>
#else

namespace std {

struct nullopt_t {};

constexpr nullopt_t nullopt {};

template<typename T>
class optional {
public:
    optional() = default;
    optional(nullopt_t) {}
    optional(const T &v) : _value(v), _hasValue(true) {}
    ~optional() = default;

    optional &operator =(nullopt_t) {
        _value = T();
        _hasValue = false;
        return *this;
    }

    T operator->() { return _value; }
    T operator*() { return _value; }

    operator bool() const { return _hasValue; }
    bool has_value() const { return _hasValue; }

    T value() const { return _value; }

private:
    T _value = T();
    bool _hasValue = false;
};

}

#endif
