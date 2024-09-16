// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef HASHMAP_H
#define HASHMAP_H

#include <QtCore/qhash.h>

namespace WTF {

template<typename Key, typename Value>
class HashMap final : public QHash<Key, Value>
{
public:
    void add(const Key &k, const Value &v) { QHash<Key, Value>::insert(k, v); }
    Value get(const Key &k) { return QHash<Key, Value>::value(k); }
};

}

using WTF::HashMap;

#endif
