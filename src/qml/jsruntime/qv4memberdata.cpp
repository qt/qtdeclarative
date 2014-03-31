/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4memberdata_p.h"
#include "qv4mm_p.h"

using namespace QV4;

const ManagedVTable MemberData::static_vtbl =
{
    MemberData::IsExecutionContext,
    MemberData::IsString,
    MemberData::IsObject,
    MemberData::IsFunctionObject,
    MemberData::IsErrorObject,
    MemberData::IsArrayData,
    0,
    MemberData::MyType,
    "MemberData",
    destroy,
    markObjects,
    isEqualTo
};



void MemberData::markObjects(Managed *that, ExecutionEngine *e)
{
    MemberData *m = static_cast<MemberData *>(that);
    for (uint i = 0; i < m->size; ++i)
        m->data[i].mark(e);
}

void Members::ensureIndex(QV4::ExecutionEngine *e, uint idx)
{
    uint s = size();
    if (idx >= s) {
        int newAlloc = qMax((uint)4, 2*idx);
        uint alloc = sizeof(MemberData) + (newAlloc)*sizeof(Value);
        MemberData *newMemberData = reinterpret_cast<MemberData *>(e->memoryManager->allocManaged(alloc));
        if (d())
            memcpy(newMemberData, d(), sizeof(MemberData) + s*sizeof(Value));
        else
            new (newMemberData) MemberData(e->memberDataClass);
        newMemberData->size = newAlloc;
        m = newMemberData;
    }
}
