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
#ifndef QV4MEMBERDATA_H
#define QV4MEMBERDATA_H

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

#include "qv4global_p.h"
#include "qv4managed_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

#define MemberDataMembers(class, Member) \
    Member(class, ValueArray, ValueArray, values)

DECLARE_HEAP_OBJECT(MemberData, Base) {
    DECLARE_MARKOBJECTS(MemberData);
};
Q_STATIC_ASSERT(std::is_trivial< MemberData >::value);

}

struct MemberData : Managed
{
    V4_MANAGED(MemberData, Managed)
    V4_INTERNALCLASS(MemberData)

    const Value &operator[] (uint idx) const { return d()->values[idx]; }
    const Value *data() const { return d()->values.data(); }
    void set(EngineBase *e, uint index, Value v) { d()->values.set(e, index, v); }
    void set(EngineBase *e, uint index, Heap::Base *b) { d()->values.set(e, index, b); }

    inline uint size() const { return d()->values.size; }

    static Heap::MemberData *allocate(QV4::ExecutionEngine *e, uint n, Heap::MemberData *old = nullptr);
};

}

QT_END_NAMESPACE

#endif
