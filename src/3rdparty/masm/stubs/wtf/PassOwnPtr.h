// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef PASSOWNPTR_H
#define PASSOWNPTR_H

#include <qscopedpointer.h>

template <typename T> class PassOwnPtr;
template <typename PtrType> PassOwnPtr<PtrType> adoptPtr(PtrType*);

template <typename T>
struct OwnPtr : public QScopedPointer<T>
{
    OwnPtr() {}
    OwnPtr(const PassOwnPtr<T> &ptr)
        : QScopedPointer<T>(ptr.leakRef())
    {}

    OwnPtr(const OwnPtr<T>& other)
        : QScopedPointer<T>(const_cast<OwnPtr<T> &>(other).take())
    {}

    OwnPtr& operator=(const OwnPtr<T>& other)
    {
        this->reset(const_cast<OwnPtr<T> &>(other).take());
        return *this;
    }

    T* get() const { return this->data(); }

    PassOwnPtr<T> release()
    {
        return adoptPtr(this->take());
    }
};

template <typename T>
class PassOwnPtr {
public:
    PassOwnPtr() {}

    PassOwnPtr(T* ptr)
        : m_ptr(ptr)
    {
    }

    PassOwnPtr(const PassOwnPtr<T>& other)
        : m_ptr(other.leakRef())
    {
    }

    PassOwnPtr(const OwnPtr<T>& other)
        : m_ptr(other.take())
    {
    }

    ~PassOwnPtr()
    {
    }

    T* operator->() const { return m_ptr.data(); }

    T* leakRef() const { return m_ptr.take(); }

private:
    template <typename PtrType> friend PassOwnPtr<PtrType> adoptPtr(PtrType*);

    PassOwnPtr<T>& operator=(const PassOwnPtr<T>& t);

    mutable QScopedPointer<T> m_ptr;
};

template <typename T>
PassOwnPtr<T> adoptPtr(T* ptr)
{
    PassOwnPtr<T> result;
    result.m_ptr.reset(ptr);
    return result;
}


#endif // PASSOWNPTR_H
