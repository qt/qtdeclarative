// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef HASHSET_H
#define HASHSET_H

#include <QtCore/qset.h>

namespace WTF {

template<typename Key>
class HashSet final : public QSet<Key>
{
public:
    struct SetAddResult {
        bool isNewEntry;
    };
    SetAddResult add(const Key &k)
    {
        if (QSet<Key>::find(k) == QSet<Key>::constEnd()) {
            QSet<Key>::insert(k);
            return { true };
        }
        return { false };
    }
};

}

using WTF::HashSet;

#endif
