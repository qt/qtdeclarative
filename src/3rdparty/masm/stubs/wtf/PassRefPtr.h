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
