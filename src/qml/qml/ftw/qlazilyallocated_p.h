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

#ifndef QLAZILYALLOCATED_P_H
#define QLAZILYALLOCATED_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qtaggedpointer.h>

QT_BEGIN_NAMESPACE

template<typename T, typename Tag = typename QtPrivate::TagInfo<T>::TagType>
class QLazilyAllocated {
public:
    inline QLazilyAllocated();
    inline ~QLazilyAllocated();

    inline bool isAllocated() const;

    inline T *operator->() const;

    inline T &value();
    inline const T &value() const;

    inline Tag tag() const;
    inline void setTag(Tag t);
private:
    mutable QTaggedPointer<T, Tag> d;
};

template<typename T, typename Tag>
QLazilyAllocated<T, Tag>::QLazilyAllocated()
{
}

template<typename T, typename Tag>
QLazilyAllocated<T, Tag>::~QLazilyAllocated()
{
    delete d.data();
}

template<typename T, typename Tag>
bool QLazilyAllocated<T, Tag>::isAllocated() const
{
    return !d.isNull();
}

template<typename T, typename Tag>
T &QLazilyAllocated<T, Tag>::value()
{
    if (d.isNull()) d = new T;
    return *d;
}

template<typename T, typename Tag>
const T &QLazilyAllocated<T, Tag>::value() const
{
    if (d.isNull()) d = new T;
    return *d;
}

template<typename T, typename Tag>
T *QLazilyAllocated<T, Tag>::operator->() const
{
    return d.data();
}

template<typename T, typename Tag>
Tag QLazilyAllocated<T, Tag>::tag() const
{
    return d.tag();
}

template<typename T, typename Tag>
void QLazilyAllocated<T, Tag>::setTag(Tag t)
{
    d.setTag(t);
}

QT_END_NAMESPACE

#endif // QLAZILYALLOCATED_P_H
