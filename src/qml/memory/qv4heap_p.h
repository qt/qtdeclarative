/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4HEAP_P_H
#define QV4HEAP_P_H

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

#include <QtCore/QString>
#include <private/qv4global_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct VTable
{
    const VTable * const parent;
    uint isExecutionContext : 1;
    uint isString : 1;
    uint isObject : 1;
    uint isFunctionObject : 1;
    uint isErrorObject : 1;
    uint isArrayData : 1;
    uint unused : 18;
    uint type : 8;
    const char *className;
    void (*destroy)(Heap::Base *);
    void (*markObjects)(Heap::Base *, ExecutionEngine *e);
    bool (*isEqualTo)(Managed *m, Managed *other);
};

namespace Heap {

struct Q_QML_EXPORT Base {
    quintptr mm_data; // vtable and markbit

    inline ReturnedValue asReturnedValue() const;
    inline void mark(QV4::ExecutionEngine *engine);

    enum {
        MarkBit = 0x1,
        NotInUse = 0x2,
        PointerMask = ~0x3
    };

    void setVtable(const VTable *v) {
        Q_ASSERT(!(mm_data & MarkBit));
        mm_data = reinterpret_cast<quintptr>(v);
    }
    VTable *vtable() const {
        return reinterpret_cast<VTable *>(mm_data & PointerMask);
    }
    inline bool isMarked() const {
        return mm_data & MarkBit;
    }
    inline void setMarkBit() {
        mm_data |= MarkBit;
    }
    inline void clearMarkBit() {
        mm_data &= ~MarkBit;
    }

    inline bool inUse() const {
        return !(mm_data & NotInUse);
    }

    Base *nextFree() {
        return reinterpret_cast<Base *>(mm_data & PointerMask);
    }
    void setNextFree(Base *m) {
        mm_data = (reinterpret_cast<quintptr>(m) | NotInUse);
    }

    void *operator new(size_t, Managed *m) { return m; }
    void *operator new(size_t, Heap::Base *m) { return m; }
    void operator delete(void *, Heap::Base *) {}
};

template <typename T>
struct Pointer {
    Pointer() {}
    Pointer(T *t) : ptr(t) {}

    T *operator->() const { return ptr; }
    operator T *() const { return ptr; }

    Pointer &operator =(T *t) { ptr = t; return *this; }

    template <typename Type>
    Type *cast() { return static_cast<Type *>(ptr); }

    T *ptr;
};

}

}

QT_END_NAMESPACE

#endif
