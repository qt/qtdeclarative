// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef PASSREFPTR_H
#define PASSREFPTR_H

template <typename T> class RefPtr;

template <typename T>
class PassRefPtr {
public:
    PassRefPtr() : m_ptr(0) {}

    PassRefPtr(T* ptr)
        : m_ptr(ptr)
    {
        if (m_ptr)
            m_ptr->ref();
    }

    PassRefPtr(const PassRefPtr<T>& other)
        : m_ptr(other.leakRef())
    {
    }

    PassRefPtr(const RefPtr<T>& other)
        : m_ptr(other.get())
    {
        if (m_ptr)
            m_ptr->ref();
    }

    ~PassRefPtr()
    {
        if (m_ptr)
            m_ptr->deref();
    }

    T* operator->() const { return m_ptr; }

    T* leakRef() const
    {
        T* result = m_ptr;
        m_ptr = 0;
        return result;
    }

private:
    PassRefPtr<T>& operator=(const PassRefPtr<T>& t);

protected:
    mutable T* m_ptr;
};

template <typename T>
class Ref : public PassRefPtr<T>
{
    using PassRefPtr<T>::PassRefPtr;

    template <typename PtrType> friend Ref<PtrType> adoptRef(PtrType*);
};

template <typename T>
Ref<T> adoptRef(T* ptr)
{
    Ref<T> result;
    result.m_ptr = ptr;
    return result;
}

#endif // PASSREFPTR_H
