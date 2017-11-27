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
#ifndef QV4WRITEBARRIER_P_H
#define QV4WRITEBARRIER_P_H

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

#include <private/qv4global_p.h>
#include <private/qv4value_p.h>

QT_BEGIN_NAMESPACE

#define WRITEBARRIER_none 1

#define WRITEBARRIER(x) (1/WRITEBARRIER_##x == 1)

namespace QV4 {

namespace WriteBarrier {

enum Type {
    NoBarrier,
    Barrier
};

enum NewValueType {
    Primitive,
    Object,
    Unknown
};

// ### this needs to be filled with a real memory fence once marking is concurrent
Q_ALWAYS_INLINE void fence() {}

#if WRITEBARRIER(none)

template <NewValueType type>
static Q_CONSTEXPR inline bool isRequired() {
    return false;
}

inline void write(EngineBase *engine, Heap::Base *base, Value *slot, Value value)
{
    Q_UNUSED(engine);
    Q_UNUSED(base);
    *slot = value;
}

inline void write(EngineBase *engine, Heap::Base *base, Value *slot, Heap::Base *value)
{
    Q_UNUSED(engine);
    Q_UNUSED(base);
    *slot = value;
}

inline void write(EngineBase *engine, Heap::Base *base, Heap::Base **slot, Heap::Base *value)
{
    Q_UNUSED(engine);
    Q_UNUSED(base);
    *slot = value;
}

#endif

}

namespace Heap {

template <typename T, size_t o>
struct Pointer {
    static Q_CONSTEXPR size_t offset = o;
    T operator->() const { return get(); }
    operator T () const { return get(); }

    Heap::Base *base() {
        Heap::Base *base = reinterpret_cast<Heap::Base *>(this) - (offset/sizeof(Heap::Base));
        Q_ASSERT(base->inUse());
        return base;
    }

    void set(EngineBase *e, T newVal) {
        WriteBarrier::write(e, base(), &ptr, reinterpret_cast<Heap::Base *>(newVal));
    }

    T get() const { return reinterpret_cast<T>(ptr); }

    template <typename Type>
    Type *cast() { return static_cast<Type *>(ptr); }

    Heap::Base *heapObject() const { return ptr; }

private:
    Heap::Base *ptr;
};
typedef Pointer<char *, 0> V4PointerCheck;
V4_ASSERT_IS_TRIVIAL(V4PointerCheck)

}

template <size_t o>
struct HeapValue : Value {
    static Q_CONSTEXPR size_t offset = o;
    Heap::Base *base() {
        Heap::Base *base = reinterpret_cast<Heap::Base *>(this) - (offset/sizeof(Heap::Base));
        Q_ASSERT(base->inUse());
        return base;
    }

    void set(EngineBase *e, const Value &newVal) {
        WriteBarrier::write(e, base(), this, newVal);
    }
    void set(EngineBase *e, Heap::Base *b) {
        WriteBarrier::write(e, base(), this, b);
    }
};

template <size_t o>
struct ValueArray {
    static Q_CONSTEXPR size_t offset = o;
    uint size;
    uint alloc;
    Value values[1];

    Heap::Base *base() {
        Heap::Base *base = reinterpret_cast<Heap::Base *>(this) - (offset/sizeof(Heap::Base));
        Q_ASSERT(base->inUse());
        return base;
    }

    void set(EngineBase *e, uint index, Value v) {
        WriteBarrier::write(e, base(), values + index, v);
    }
    void set(EngineBase *e, uint index, Heap::Base *b) {
        WriteBarrier::write(e, base(), values + index, b);
    }
    inline const Value &operator[] (uint index) const {
        Q_ASSERT(index < alloc);
        return values[index];
    }
    inline const Value *data() const {
        return values;
    }

    void insertData(EngineBase *e, uint index, Value v) {
        for (uint i = size - 1; i > index; --i) {
            values[i] = values[i - 1];
        }
        set(e, index, v);
    }
    void removeData(EngineBase *e, uint index, int n = 1) {
        Q_UNUSED(e);
        for (uint i = index; i < size - n; ++i) {
            values[i] = values[i + n];
        }
    }

    void mark(MarkStack *markStack) {
        Value *v = values;
        const Value *end = v + alloc;
        if (alloc > 32*1024) {
            // drain from time to time to avoid overflows in the js stack
            Heap::Base **currentBase = markStack->top;
            while (v < end) {
                v->mark(markStack);
                ++v;
                if (markStack->top >= currentBase + 32*1024) {
                    Heap::Base **oldBase = markStack->base;
                    markStack->base = currentBase;
                    markStack->drain();
                    markStack->base = oldBase;
                }
            }
        } else {
            while (v < end) {
                v->mark(markStack);
                ++v;
            }
        }
    }
};

// It's really important that the offset of values in this structure is
// constant across all architecture,  otherwise JIT cross-compiled code will
// have wrong offsets between host and target.
Q_STATIC_ASSERT(offsetof(ValueArray<0>, values) == 8);

}

QT_END_NAMESPACE

#endif
