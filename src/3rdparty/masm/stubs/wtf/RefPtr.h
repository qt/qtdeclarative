/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
