// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
