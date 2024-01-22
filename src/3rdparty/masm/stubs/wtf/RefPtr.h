// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef REFPTR_H
#define REFPTR_H

#include "PassRefPtr.h"

template <typename T>
class RefPtr {
public:
    RefPtr() : m_ptr(0) {}
    RefPtr(const RefPtr<T> &other)
        : m_ptr(other.m_ptr)
    {
        if (m_ptr)
            m_ptr->ref();
    }

    RefPtr<T>& operator=(const RefPtr<T>& other)
    {
        if (other.m_ptr)
            other.m_ptr->ref();
        if (m_ptr)
            m_ptr->deref();
        m_ptr = other.m_ptr;
        return *this;
    }

    RefPtr(const PassRefPtr<T>& other)
        : m_ptr(other.leakRef())
    {
    }

    ~RefPtr()
    {
        if (m_ptr)
            m_ptr->deref();
    }

    T* operator->() const { return m_ptr; }
    T* get() const { return m_ptr; }
    bool operator!() const { return !m_ptr; }

    PassRefPtr<T> release()
    {
        T* ptr = m_ptr;
        m_ptr = 0;
        return adoptRef(ptr);
    }

private:
    T* m_ptr;
};

#endif // REFPTR_H
