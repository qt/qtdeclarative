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
#ifndef REFCOUNTED_H
#define REFCOUNTED_H

#include "PassRefPtr.h"

template <typename Base>
class RefCounted {
public:
    RefCounted() : m_refCount(1) {}
    ~RefCounted()
    {
        deref();
    }

    void ref()
    {
        ++m_refCount;
    }

    void deref()
    {
        if (!--m_refCount)
            delete static_cast<Base*>(this);
    }

protected:
    int m_refCount;
};

#endif // REFCOUNTED_H
