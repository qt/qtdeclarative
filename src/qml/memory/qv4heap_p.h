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
#ifndef QV4HEAP_P_H
#define QV4HEAP_P_H

#include <QtCore/QString>
#include <private/qv4global_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

struct Q_QML_EXPORT Base {
    union {
        const ManagedVTable *vtable;
        quintptr mm_data;
    };

    inline ReturnedValue asReturnedValue() const;
    inline void mark(QV4::ExecutionEngine *engine);

    enum {
        MarkBit = 0x1,
        NotInUse = 0x2,
        PointerMask = ~0x3
    };

    ManagedVTable *gcGetVtable() const {
        return reinterpret_cast<ManagedVTable *>(mm_data & PointerMask);
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

    T *operator->() const { return static_cast<T *>(ptr); }
    operator T *() const { return static_cast<T *>(ptr); }

    Pointer &operator =(T *t) { ptr = t; return *this; }

    template <typename Type>
    Type *as() { return static_cast<Type *>(ptr); }

    // Use Base, not T here, to ensure T inherits from ptr
    Base *ptr;
};

}

}

QT_END_NAMESPACE

#endif
