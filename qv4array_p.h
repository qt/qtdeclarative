/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
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
#ifndef QV4ARRAY_P_H
#define QV4ARRAY_P_H

#include "qmljs_runtime.h"
#include <QtCore/QVector>
#include <deque>
#include <algorithm>

namespace QQmlJS {
namespace VM {

class Array
{
public:
    inline Array();
    inline Array(const Array &other);
    inline ~Array();

    inline Array &operator = (const Array &other);

    inline bool isEmpty() const;
    inline uint size() const;
    inline uint count() const;
    inline Value at(uint index) const;
    inline void assign(uint index, const Value &v);
    inline void clear();
    inline void resize(uint size);
    inline void concat(const Array &other);
    inline Value pop();
    inline Value takeFirst();
    inline void sort(ExecutionContext *context, const Value &comparefn);
    inline void splice(double start, double deleteCount,
                       const QVector<Value> &items,
                       Array &other);
    inline void push(const Value &value);

private:
    std::deque<Value> *to_vector;
};

class ArrayElementLessThan
{
public:
    inline ArrayElementLessThan(ExecutionContext *context, const Value &comparefn)
        : m_context(context), m_comparefn(comparefn) {}

    bool operator()(const Value &v1, const Value &v2) const;

private:
    ExecutionContext *m_context;
    Value m_comparefn;
};

inline Array::Array()
{
    to_vector = new std::deque<Value>();
}

inline Array::Array(const Array &other)
{
    to_vector = new std::deque<Value>(*other.to_vector);
}

inline Array::~Array()
{
    delete to_vector;
}

inline Array &Array::operator = (const Array &other)
{
    *to_vector = *other.to_vector;
    return *this;
}

inline bool Array::isEmpty() const
{
    return to_vector->empty();
}

inline uint Array::size() const
{
    return to_vector->size();
}

inline uint Array::count() const
{
    return to_vector->size();
}

inline Value Array::at(uint index) const
{
    return index < to_vector->size() ? to_vector->at(index) : Value::undefinedValue();
}

inline void Array::assign(uint index, const Value &v)
{
    if (index == to_vector->size())
        to_vector->push_back(v);
    else {
        if (index > to_vector->size())
            resize(index + 1);

        (*to_vector)[index] = v;
    }
}

inline void Array::clear()
{
    to_vector->clear();
}

inline void Array::resize(uint s)
{
    to_vector->resize(s, Value::undefinedValue());
}

inline void Array::concat(const Array &other)
{
    for (std::deque<Value>::iterator it = other.to_vector->begin(); it != other.to_vector->end(); ++it) {
        const Value &v = *it;
        if (! v.isUndefined())
            to_vector->push_back(v);
    }
}

inline Value Array::pop()
{
    if (isEmpty())
        return Value::undefinedValue();

    Value v = to_vector->back();
    to_vector->pop_back();
    return v;
}

inline Value Array::takeFirst()
{
    if (isEmpty())
        return Value::undefinedValue();

    Value v = to_vector->front();
    to_vector->pop_front();
    return v;
}

inline void Array::sort(ExecutionContext *context, const Value &comparefn)
{
    ArrayElementLessThan lessThan(context, comparefn);
    std::sort(to_vector->begin(), to_vector->end(), lessThan);
}

inline void Array::push(const Value &value)
{
    to_vector->push_back(value);
}

inline void Array::splice(double start, double deleteCount,
                          const QVector<Value> &items,
                          Array &other)
{
    const double len = size();
    if (start < 0)
        start = qMax(len + start, double(0));
    else if (start > len)
        start = len;
    deleteCount = qMax(qMin(deleteCount, len - start), double(0));

    const uint st = uint(start);
    const uint dc = uint(deleteCount);
    other.resize(dc);

    const uint itemsSize = uint(items.size());

    for (uint i = 0; i < dc; ++i)
        other.assign(i, to_vector->at(st + i));
    if (itemsSize > dc)
        to_vector->insert(to_vector->begin() + st, itemsSize - dc, Value::undefinedValue());
    else if (itemsSize < dc)
        to_vector->erase(to_vector->begin() + st, to_vector->begin() + (dc - itemsSize));
    for (uint i = 0; i < itemsSize; ++i)
        (*to_vector)[st + i] = items.at(i);
}

} // end of namespace VM
} // end of namespace QQmlJS

#endif // QV4ARRAY_P_H
