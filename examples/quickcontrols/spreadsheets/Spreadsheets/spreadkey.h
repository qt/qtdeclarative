// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SPREADKEY_H
#define SPREADKEY_H

#include <utility>

namespace spread {
// make a const_iterator from an iterator for the same container
// to avoid mixing iterators with const_iterators warning
template <typename Container>
constexpr typename Container::const_iterator make_const(Container c, typename Container::iterator i)
{
    return typename Container::const_iterator(i);
}
}

// using std::pair<> as SpreadKey for now
// for any further updates it could be anything
using SpreadKey = std::pair<int, int>;

#endif // SPREADKEY_H
