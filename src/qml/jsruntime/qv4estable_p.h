/****************************************************************************
**
** Copyright (C) 2018 Crimson AS <info@crimson.no>
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

QT_BEGIN_NAMESPACE

namespace QV4
{

class ESTable
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
    Value *m_keys = nullptr;
    Value *m_values = nullptr;
    uint m_size = 0;
    uint m_capacity = 0;
};

}

QT_END_NAMESPACE

#endif
