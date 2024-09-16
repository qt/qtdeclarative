// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef VECTOR_H
#define VECTOR_H

#include <vector>
#include <wtf/Assertions.h>
#include <wtf/NotFound.h>
#include <qalgorithms.h>

enum WTF_UnusedOverflowMode {
    UnsafeVectorOverflow
};

namespace WTF {

template <typename T, int capacity = 1, int overflowMode = UnsafeVectorOverflow>
class Vector : public std::vector<T> {
public:
    Vector() {}
    Vector(int initialSize) : std::vector<T>(initialSize) {}
    Vector(std::initializer_list<T> list) : std::vector<T>(list) {}

    inline void append(const T& value)
    { this->push_back(value); }

    template <typename OtherType>
    inline void append(const OtherType& other)
    { this->push_back(T(other)); }

    inline void append(T&& other)
    { this->push_back(std::move(other)); }

    inline void append(const Vector<T>& vector)
    {
        this->insert(this->end(), vector.begin(), vector.end());
    }

    inline void append(const T* ptr, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
            this->push_back(T(ptr[i]));
    }

    inline void append(typename std::vector<T>::const_iterator it, size_t count)
    {
        for (size_t i = 0; i < count; ++i, ++it)
            this->push_back(*it);
    }

    unsigned size() const { return static_cast<unsigned>(std::vector<T>::size()); }

    using std::vector<T>::insert;

    inline void reserveInitialCapacity(size_t size) { this->reserve(size); }

    inline void insert(size_t position, T value)
    { this->insert(this->begin() + position, value); }

    inline void grow(size_t size)
    { this->resize(size); }

    inline void shrink(size_t size)
    { this->erase(this->begin() + size, this->end()); }

    inline void shrinkToFit()
    { this->shrink(this->size()); }

    inline void remove(size_t position)
    { this->erase(this->begin() + position); }

    inline bool isEmpty() const { return this->empty(); }

    inline T &last() { return *(this->begin() + this->size() - 1); }

    bool contains(const T &value) const
    {
        for (const T &inVector : *this) {
            if (inVector == value)
                return true;
        }
        return false;
    }
};

template <typename T, int capacity>
void deleteAllValues(const Vector<T, capacity> &vector)
{
    qDeleteAll(vector);
}

}

using WTF::Vector;
using WTF::deleteAllValues;

#endif // VECTOR_H
