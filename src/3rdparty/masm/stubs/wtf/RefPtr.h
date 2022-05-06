/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/
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
