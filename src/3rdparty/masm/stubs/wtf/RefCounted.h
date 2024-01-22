// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
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
