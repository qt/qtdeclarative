/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4memberdata_p.h"
#include "qv4mm_p.h"

using namespace QV4;

DEFINE_MANAGED_VTABLE(MemberData);

void MemberData::markObjects(Heap::Base *that, ExecutionEngine *e)
{
    Heap::MemberData *m = static_cast<Heap::MemberData *>(that);
    for (uint i = 0; i < m->size; ++i)
        m->data[i].mark(e);
}

Heap::MemberData *MemberData::reallocate(ExecutionEngine *e, Heap::MemberData *old, uint idx)
{
    uint s = old ? old->size : 0;
    if (idx < s)
        return old;

    int newAlloc = qMax((uint)4, 2*idx);
    uint alloc = sizeof(Heap::MemberData) + (newAlloc)*sizeof(Value);
    Scope scope(e);
    Scoped<MemberData> newMemberData(scope, e->memoryManager->allocManaged<MemberData>(alloc));
    if (old)
        memcpy(newMemberData->d(), old, sizeof(Heap::MemberData) + s*sizeof(Value));
    else
        new (newMemberData->d()) Heap::MemberData;
    newMemberData->d()->size = newAlloc;
    return newMemberData->d();
}
