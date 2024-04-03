// Copyright (C) 2018 Crimson AS <info@crimson.no>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QV4ESTABLE_P_H
#define QV4ESTABLE_P_H

#include "qv4value_p.h"

class tst_qv4estable;

QT_BEGIN_NAMESPACE

namespace QV4 {

class Q_AUTOTEST_EXPORT ESTable
{
public:
    ESTable();
    ~ESTable();

    void markObjects(MarkStack *s, bool isWeakMap);
    void clear();
    void set(const Value &k, const Value &v);
    bool has(const Value &k) const;
    ReturnedValue get(const Value &k, bool *hasValue = nullptr) const;
    bool remove(const Value &k);
    uint size() const;
    void iterate(uint idx, Value *k, Value *v);

    void removeUnmarkedKeys();

private:
    friend class ::tst_qv4estable;

    Value *m_keys = nullptr;
    Value *m_values = nullptr;
    uint m_size = 0;
    uint m_capacity = 0;
};

} // namespace QV4

QT_END_NAMESPACE

#endif
