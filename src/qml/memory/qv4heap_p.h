// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:low-level-memory-management
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

#include <private/qv4global_p.h>
#include <private/qv4mmdefs_p.h>
#include <private/qv4writebarrier_p.h>
#include <private/qv4vtable_p.h>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

template <typename T, size_t o>
struct Pointer {
    static constexpr size_t offset = o;
    T operator->() const { return get(); }
    operator T () const { return get(); }

    Base *base();

    void set(EngineBase *e, T newVal) {
        WriteBarrier::write(e, base(), &ptr, reinterpret_cast<Base *>(newVal));
    }

    T get() const { return reinterpret_cast<T>(ptr); }

    template <typename Type>
    Type *cast() { return static_cast<Type *>(ptr); }

    Base *heapObject() const { return ptr; }

private:
    Base *ptr;
};
typedef Pointer<char *, 0> V4PointerCheck;
static_assert(std::is_trivially_copyable_v<V4PointerCheck>);
static_assert(std::is_trivially_default_constructible_v<V4PointerCheck>);

struct Q_QML_EXPORT Base {
    void *operator new(size_t) = delete;

    static void markObjects(Base *, MarkStack *);

    Pointer<InternalClass *, 0> internalClass;

    inline ReturnedValue asReturnedValue() const;
    inline void mark(QV4::MarkStack *markStack);

    inline bool isMarked() const {
        const HeapItem *h = reinterpret_cast<const HeapItem *>(this);
        Chunk *c = h->chunk();
        Q_ASSERT(!Chunk::testBit(c->extendsBitmap, h - c->realBase()));
        return Chunk::testBit(c->blackBitmap, h - c->realBase());
    }
    inline void setMarkBit() {
        const HeapItem *h = reinterpret_cast<const HeapItem *>(this);
        Chunk *c = h->chunk();
        Q_ASSERT(!Chunk::testBit(c->extendsBitmap, h - c->realBase()));
        return Chunk::setBit(c->blackBitmap, h - c->realBase());
    }

    inline bool inUse() const {
        const HeapItem *h = reinterpret_cast<const HeapItem *>(this);
        Chunk *c = h->chunk();
        Q_ASSERT(!Chunk::testBit(c->extendsBitmap, h - c->realBase()));
        return Chunk::testBit(c->objectBitmap, h - c->realBase());
    }

    void *operator new(size_t, Managed *m) { return m; }
    void *operator new(size_t, Base *m) { return m; }
    void operator delete(void *, Base *) {}

    void init() {}
    void destroy() {}
};
static_assert(std::is_trivially_copyable_v<Base>);
static_assert(std::is_trivially_default_constructible_v<Base>);
// This class needs to consist only of pointer sized members to allow
// for a size/offset translation when cross-compiling between 32- and
// 64-bit.
static_assert(std::is_standard_layout<Base>::value);
static_assert(offsetof(Base, internalClass) == 0);
static_assert(sizeof(Base) == QT_POINTER_SIZE);

inline
void Base::mark(QV4::MarkStack *markStack)
{
    Q_ASSERT(inUse());
    const HeapItem *h = reinterpret_cast<const HeapItem *>(this);
    Chunk *c = h->chunk();
    size_t index = h - c->realBase();
    Q_ASSERT(!Chunk::testBit(c->extendsBitmap, index));
    quintptr *bitmap = c->blackBitmap + Chunk::bitmapIndex(index);
    quintptr bit = Chunk::bitForIndex(index);
    if (!(*bitmap & bit)) {
        *bitmap |= bit;
        markStack->push(this);
    }
}

template<typename T, size_t o>
Base *Pointer<T, o>::base() {
    Base *base = reinterpret_cast<Base *>(this) - (offset/sizeof(Base *));
    Q_ASSERT(base->inUse());
    return base;
}

}

#ifdef QT_NO_QOBJECT
template <class T>
struct QV4QPointer {
};
#else
template <class T>
struct QV4QPointer {
    void init()
    {
        d = nullptr;
        qObject = nullptr;
    }

    void init(T *o)
    {
        Q_ASSERT(d == nullptr);
        Q_ASSERT(qObject == nullptr);
        if (o) {
            d = QtSharedPointer::ExternalRefCountData::getAndRef(o);
            qObject = o;
        }
    }

    void destroy()
    {
        if (d && !d->weakref.deref())
            delete d;
        d = nullptr;
        qObject = nullptr;
    }

    T *data() const {
        return d == nullptr || d->strongref.loadRelaxed() == 0 ? nullptr : qObject;
    }
    operator T*() const { return data(); }
    inline T* operator->() const { return data(); }
    QV4QPointer &operator=(T *o)
    {
        if (d)
            destroy();
        init(o);
        return *this;
    }

    bool isNull() const noexcept
    {
        return !isValid() || d->strongref.loadRelaxed() == 0;
    }

    bool isValid() const noexcept
    {
        return d != nullptr && qObject != nullptr;
    }

private:
    QtSharedPointer::ExternalRefCountData *d;
    T *qObject;
};
static_assert(std::is_trivially_copyable_v<QV4QPointer<QObject>>);
static_assert(std::is_trivially_default_constructible_v<QV4QPointer<QObject>>);
#endif

}

QT_END_NAMESPACE

#endif
