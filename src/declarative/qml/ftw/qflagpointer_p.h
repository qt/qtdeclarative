/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFLAGPOINTER_P_H
#define QFLAGPOINTER_P_H

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

QT_BEGIN_NAMESPACE

template<typename T>
class QFlagPointer {
public:
    inline QFlagPointer();
    inline QFlagPointer(T *);
    inline QFlagPointer(const QFlagPointer<T> &o);

    inline bool isNull() const;

    inline bool flag() const;
    inline void setFlag();
    inline void clearFlag();

    inline bool flag2() const;
    inline void setFlag2();
    inline void clearFlag2();

    inline QFlagPointer<T> &operator=(const QFlagPointer &o);
    inline QFlagPointer<T> &operator=(T *);

    inline T *operator->() const;
    inline T *operator*() const;

private:
    intptr_t ptr_value;

    static const intptr_t FlagBit = 0x1;
    static const intptr_t Flag2Bit = 0x2;
    static const intptr_t FlagsMask = FlagBit | Flag2Bit;
};

template<typename T>
QFlagPointer<T>::QFlagPointer()
: ptr_value(0)
{
}

template<typename T>
QFlagPointer<T>::QFlagPointer(T *v)
: ptr_value(intptr_t(v))
{
    Q_ASSERT((ptr_value & FlagsMask) == 0);
}

template<typename T>
QFlagPointer<T>::QFlagPointer(const QFlagPointer<T> &o)
: ptr_value(o.ptr_value)
{
}

template<typename T>
bool QFlagPointer<T>::isNull() const
{
    return 0 == (ptr_value & (~FlagsMask));
}

template<typename T>
bool QFlagPointer<T>::flag() const
{
    return ptr_value & FlagBit;
}

template<typename T>
void QFlagPointer<T>::setFlag()
{
    ptr_value |= FlagBit;
}

template<typename T>
void QFlagPointer<T>::clearFlag()
{
    ptr_value &= ~FlagBit;
}

template<typename T>
bool QFlagPointer<T>::flag2() const
{
    return ptr_value & Flag2Bit;
}

template<typename T>
void QFlagPointer<T>::setFlag2()
{
    ptr_value|= Flag2Bit;
}

template<typename T>
void QFlagPointer<T>::clearFlag2()
{
    ptr_value &= ~Flag2Bit;
}

template<typename T>
QFlagPointer<T> &QFlagPointer<T>::operator=(const QFlagPointer &o)
{
    ptr_value = o.ptr_value;
    return *this;
}

template<typename T>
QFlagPointer<T> &QFlagPointer<T>::operator=(T *o)
{
    Q_ASSERT((intptr_t(o) & FlagsMask) == 0);

    ptr_value = intptr_t(o) | (ptr_value & FlagsMask);
    return *this;
}

template<typename T>
T *QFlagPointer<T>::operator->() const
{
    return (T *)(ptr_value & ~FlagsMask);
}

template<typename T>
T *QFlagPointer<T>::operator*() const
{
    return (T *)(ptr_value & ~FlagsMask);
}

QT_END_NAMESPACE

#endif // QFLAGPOINTER_P_H
