// Copyright (C) 2018 Crimson AS <info@crimson.no>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4estable_p.h"
#include "qv4object_p.h"

using namespace QV4;

// The ES spec requires that Map/Set be implemented using a data structure that
// is a little different from most; it requires nonlinear access, and must also
// preserve the order of insertion of items in a deterministic way.
//
// This class implements those requirements, except for fast access: that
// will be addressed in a followup patch.

ESTable::ESTable()
    : m_capacity(8)
{
    m_keys = (Value*)malloc(m_capacity * sizeof(Value));
    m_values = (Value*)malloc(m_capacity * sizeof(Value));
    memset(m_keys, 0, m_capacity);
    memset(m_values, 0, m_capacity);
}

ESTable::~ESTable()
{
    free(m_keys);
    free(m_values);
    m_size = 0;
    m_capacity = 0;
    m_keys = nullptr;
    m_values = nullptr;
}

void ESTable::markObjects(MarkStack *s, bool isWeakMap)
{
    for (uint i = 0; i < m_size; ++i) {
        if (!isWeakMap)
            m_keys[i].mark(s);
        m_values[i].mark(s);
    }
}

// Pretends that there's nothing in the table. Doesn't actually free memory, as
// it will almost certainly be reused again anyway.
void ESTable::clear()
{
    m_size = 0;

    std::for_each(m_observers.begin(), m_observers.end(), [](ShiftObserver* ob){
        Q_ASSERT(ob);
        ob->pivot = ShiftObserver::OUT_OF_TABLE;
    });
}

// Update the table to contain \a value for a given \a key. The key is
// normalized, as required by the ES spec.
void ESTable::set(const Value &key, const Value &value)
{
    for (uint i = 0; i < m_size; ++i) {
        if (m_keys[i].sameValueZero(key)) {
            m_values[i] = value;
            return;
        }
    }

    if (m_capacity == m_size) {
        uint oldCap = m_capacity;
        m_capacity *= 2;
        m_keys = (Value*)realloc(m_keys, m_capacity * sizeof(Value));
        m_values = (Value*)realloc(m_values, m_capacity * sizeof(Value));
        memset(m_keys + oldCap, 0, m_capacity - oldCap);
        memset(m_values + oldCap, 0, m_capacity - oldCap);
    }

    Value nk = key;
    if (nk.isDouble()) {
        if (nk.doubleValue() == 0 && std::signbit(nk.doubleValue()))
            nk = Value::fromDouble(+0);
    }

    m_keys[m_size] = nk;
    m_values[m_size] = value;

    m_size++;
}

// Returns true if the table contains \a key, false otherwise.
bool ESTable::has(const Value &key) const
{
    for (uint i = 0; i < m_size; ++i) {
        if (m_keys[i].sameValueZero(key))
            return true;
    }

    return false;
}

// Fetches the value for the given \a key, and if \a hasValue is passed in,
// it is set depending on whether or not the given key was found.
ReturnedValue ESTable::get(const Value &key, bool *hasValue) const
{
    for (uint i = 0; i < m_size; ++i) {
        if (m_keys[i].sameValueZero(key)) {
            if (hasValue)
                *hasValue = true;
            return m_values[i].asReturnedValue();
        }
    }

    if (hasValue)
        *hasValue = false;
    return Encode::undefined();
}

// Removes the given \a key from the table
bool ESTable::remove(const Value &key)
{
    for (uint index = 0; index < m_size; ++index) {
        if (m_keys[index].sameValueZero(key)) {
            // Remove the element at |index| by moving all elements to the right
            // of |index| one place to the left.
            size_t count = (m_size - (index + 1)) * sizeof(Value);
            memmove(m_keys + index, m_keys + index + 1, count);
            memmove(m_values + index, m_values + index + 1, count);
            m_size--;

            std::for_each(m_observers.begin(), m_observers.end(), [index](ShiftObserver* ob) {
                Q_ASSERT(ob);
                if (index <= ob->pivot && ob->pivot != ShiftObserver::OUT_OF_TABLE)
                    ob->pivot = ob->pivot == 0 ? ShiftObserver::OUT_OF_TABLE : ob->pivot - 1;
            });

            return true;
        }
    }
    return false;
}

// Returns the size of the table. Note that the size may not match the underlying allocation.
uint ESTable::size() const
{
    return m_size;
}

// Retrieves a key and value for a given \a idx, and places them in \a key and
// \a value. They must be valid pointers.
void ESTable::iterate(uint idx, Value *key, Value *value)
{
    Q_ASSERT(idx < m_size);
    Q_ASSERT(key);
    Q_ASSERT(value);
    *key = m_keys[idx];
    *value = m_values[idx];
}

void ESTable::removeUnmarkedKeys()
{
    uint idx = 0;
    uint toIdx = 0;
    for (; idx < m_size; ++idx) {
        Q_ASSERT(m_keys[idx].isObject());
        Object &o = static_cast<Object &>(m_keys[idx]);
        if (o.d()->isMarked()) {
            m_keys[toIdx] = m_keys[idx];
            m_values[toIdx] = m_values[idx];
            ++toIdx;
        }
    }
    m_size = toIdx;
}
