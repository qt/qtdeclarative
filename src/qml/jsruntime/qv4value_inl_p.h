/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4VALUE_INL_H
#define QV4VALUE_INL_H

#include <cmath> // this HAS to come

#include "qv4value_p.h"
#include <private/qv4heap_p.h>
#include "qv4string_p.h"
#include "qv4managed_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

inline double Value::toNumber() const
{
    if (isInteger())
        return int_32;
    if (isDouble())
        return doubleValue();
    return toNumberImpl();
}

inline int Value::toInt32() const
{
    if (isInteger())
        return int_32;
    double d = isNumber() ? doubleValue() : toNumberImpl();

    const double D32 = 4294967296.0;
    const double D31 = D32 / 2.0;

    if ((d >= -D31 && d < D31))
        return static_cast<int>(d);

    return Primitive::toInt32(d);
}

inline unsigned int Value::toUInt32() const
{
    return (unsigned int)toInt32();
}




inline
ReturnedValue Heap::Base::asReturnedValue() const
{
    return Value::fromHeapObject(const_cast<Heap::Base *>(this)).asReturnedValue();
}


#ifndef V4_BOOTSTRAP
inline uint Value::asArrayIndex() const
{
#if QT_POINTER_SIZE == 8
    if (!isNumber())
        return UINT_MAX;
    if (isInteger())
        return int_32 >= 0 ? (uint)int_32 : UINT_MAX;
#else
    if (isInteger() && int_32 >= 0)
        return (uint)int_32;
    if (!isDouble())
        return UINT_MAX;
#endif
    double d = doubleValue();
    uint idx = (uint)d;
    if (idx != d)
        return UINT_MAX;
    return idx;
}



template<>
inline ReturnedValue value_convert<String>(ExecutionEngine *e, const Value &v)
{
    return v.toString(e)->asReturnedValue();
}

#endif

template <typename T>
struct TypedValue : public Value
{
    template<typename X>
    TypedValue &operator =(X *x) {
        m = x;
#if QT_POINTER_SIZE == 4
        tag = Managed_Type;
#endif
        return *this;
    }
    TypedValue &operator =(T *t);
    TypedValue &operator =(const Scoped<T> &v);
//    TypedValue &operator =(const ManagedRef<T> &v);

    TypedValue &operator =(const TypedValue<T> &t);

    bool operator!() const { return !managed(); }

    operator T *() { return static_cast<T *>(managed()); }
    T *operator->() { return static_cast<T *>(managed()); }
    const T *operator->() const { return static_cast<T *>(managed()); }
    T *getPointer() const { return static_cast<T *>(managed()); }

    void mark(ExecutionEngine *e) { if (managed()) managed()->mark(e); }
};
typedef TypedValue<String> StringValue;


} // namespace QV4

QT_END_NAMESPACE

#endif
