/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QV4ARRAY_H
#define QV4ARRAY_H

#include "qv4global.h"
#include <QtCore/qmap.h>
#include <qmljs_value.h>
#include <qv4propertydescriptor.h>
#include <assert.h>

#ifdef Q_MAP_DEBUG
#include <QtCore/qdebug.h>
#endif

#include <new>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

struct SparseArray;

class ArrayElementLessThan
{
public:
    inline ArrayElementLessThan(ExecutionContext *context, Object *thisObject, const Value &comparefn)
        : m_context(context), thisObject(thisObject), m_comparefn(comparefn) {}

    bool operator()(const PropertyDescriptor &v1, const PropertyDescriptor &v2) const;

private:
    ExecutionContext *m_context;
    Object *thisObject;
    Value m_comparefn;
};


struct SparseArrayNode
{
    quintptr p;
    SparseArrayNode *left;
    SparseArrayNode *right;
    uint size_left;
    uint value;

    enum Color { Red = 0, Black = 1 };
    enum { Mask = 3 }; // reserve the second bit as well

    const SparseArrayNode *nextNode() const;
    SparseArrayNode *nextNode() { return const_cast<SparseArrayNode *>(const_cast<const SparseArrayNode *>(this)->nextNode()); }
    const SparseArrayNode *previousNode() const;
    SparseArrayNode *previousNode() { return const_cast<SparseArrayNode *>(const_cast<const SparseArrayNode *>(this)->previousNode()); }

    Color color() const { return Color(p & 1); }
    void setColor(Color c) { if (c == Black) p |= Black; else p &= ~Black; }
    SparseArrayNode *parent() const { return reinterpret_cast<SparseArrayNode *>(p & ~Mask); }
    void setParent(SparseArrayNode *pp) { p = (p & Mask) | quintptr(pp); }

    uint key() const {
        uint k = size_left;
        const SparseArrayNode *n = this;
        while (SparseArrayNode *p = n->parent()) {
            if (p && p->right == n)
                k += p->size_left;
            n = p;
        }
        return k;
    }

    SparseArrayNode *copy(SparseArray *d) const;

    SparseArrayNode *lowerBound(uint key);
    SparseArrayNode *upperBound(uint key);
};


inline SparseArrayNode *SparseArrayNode::lowerBound(uint akey)
{
    SparseArrayNode *n = this;
    SparseArrayNode *last = 0;
    while (n) {
        if (akey <= n->size_left) {
            last = n;
            n = n->left;
        } else {
            akey -= n->size_left;
            n = n->right;
        }
    }
    return last;
}


inline SparseArrayNode *SparseArrayNode::upperBound(uint akey)
{
    SparseArrayNode *n = this;
    SparseArrayNode *last = 0;
    while (n) {
        if (akey < n->size_left) {
            last = n;
            n = n->left;
        } else {
            akey -= n->size_left;
            n = n->right;
        }
    }
    return last;
}



struct Q_V4_EXPORT SparseArray
{
    SparseArray();
    ~SparseArray() {
        if (root())
            freeTree(header.left, Q_ALIGNOF(SparseArrayNode));
    }

    SparseArray(const SparseArray &other);
private:
    SparseArray &operator=(const SparseArray &other);

    int numEntries;
    SparseArrayNode header;
    SparseArrayNode *mostLeftNode;

    void rotateLeft(SparseArrayNode *x);
    void rotateRight(SparseArrayNode *x);
    void rebalance(SparseArrayNode *x);
    void recalcMostLeftNode();

    SparseArrayNode *root() const { return header.left; }

    void deleteNode(SparseArrayNode *z);


public:
    SparseArrayNode *createNode(uint sl, SparseArrayNode *parent, bool left);
    void freeTree(SparseArrayNode *root, int alignment);

    SparseArrayNode *findNode(uint akey) const;

    uint pop_front();
    void push_front(uint at);
    uint pop_back(uint len);
    void push_back(uint at, uint len);

    QList<int> keys() const;

    const SparseArrayNode *end() const { return &header; }
    SparseArrayNode *end() { return &header; }
    const SparseArrayNode *begin() const { if (root()) return mostLeftNode; return end(); }
    SparseArrayNode *begin() { if (root()) return mostLeftNode; return end(); }

    SparseArrayNode *erase(SparseArrayNode *n);

    SparseArrayNode *lowerBound(uint key);
    const SparseArrayNode *lowerBound(uint key) const;
    SparseArrayNode *upperBound(uint key);
    const SparseArrayNode *upperBound(uint key) const;
    SparseArrayNode *insert(uint akey);

    // STL compatibility
    typedef uint key_type;
    typedef int mapped_type;
    typedef qptrdiff difference_type;
    typedef int size_type;

#ifdef Q_MAP_DEBUG
    void dump() const;
#endif
};

inline SparseArrayNode *SparseArray::findNode(uint akey) const
{
    SparseArrayNode *n = root();

    while (n) {
        if (akey == n->size_left) {
            return n;
        } else if (akey < n->size_left) {
            n = n->left;
        } else {
            akey -= n->size_left;
            n = n->right;
        }
    }

    return 0;
}

inline uint SparseArray::pop_front()
{
    uint idx = UINT_MAX ;

    SparseArrayNode *n = findNode(0);
    if (n) {
        idx = n->value;
        deleteNode(n);
        // adjust all size_left indices on the path to leftmost item by 1
        SparseArrayNode *n = root();
        while (n) {
            n->size_left -= 1;
            n = n->left;
        }
    }
    return idx;
}

inline void SparseArray::push_front(uint value)
{
    // adjust all size_left indices on the path to leftmost item by 1
    SparseArrayNode *n = root();
    while (n) {
        n->size_left += 1;
        n = n->left;
    }
    n = insert(0);
    n->value = value;
}

inline uint SparseArray::pop_back(uint len)
{
    uint idx = UINT_MAX;
    if (!len)
        return idx;

    SparseArrayNode *n = findNode(len - 1);
    if (n) {
        idx = n->value;
        deleteNode(n);
    }
    return idx;
}

inline void SparseArray::push_back(uint index, uint len)
{
    SparseArrayNode *n = insert(len);
    n->value = index;
}

#ifdef Q_MAP_DEBUG

void SparseArray::dump() const
{
    const_iterator it = begin();
    qDebug() << "map dump:";
    while (it != end()) {
        const SparseArrayNode *n = it.i;
        int depth = 0;
        while (n && n != root()) {
            ++depth;
            n = n->parent();
        }
        QByteArray space(4*depth, ' ');
        qDebug() << space << (it.i->color() == SparseArrayNode::Red ? "Red  " : "Black") << it.i << it.i->left << it.i->right
                 << it.key() << it.value();
        ++it;
    }
    qDebug() << "---------";
}
#endif


inline SparseArrayNode *SparseArray::erase(SparseArrayNode *n)
{
    if (n == end())
        return n;

    SparseArrayNode *next = n->nextNode();
    deleteNode(n);
    return next;
}

inline QList<int> SparseArray::keys() const
{
    QList<int> res;
    res.reserve(numEntries);
    SparseArrayNode *n = mostLeftNode;
    while (n != end()) {
        res.append(n->key());
        n = n->nextNode();
    }
    return res;
}

inline const SparseArrayNode *SparseArray::lowerBound(uint akey) const
{
    const SparseArrayNode *lb = root()->lowerBound(akey);
    if (!lb)
        lb = end();
    return lb;
}


inline SparseArrayNode *SparseArray::lowerBound(uint akey)
{
    SparseArrayNode *lb = root()->lowerBound(akey);
    if (!lb)
        lb = end();
    return lb;
}


inline const SparseArrayNode *SparseArray::upperBound(uint akey) const
{
    const SparseArrayNode *ub = root()->upperBound(akey);
    if (!ub)
        ub = end();
    return ub;
}


inline SparseArrayNode *SparseArray::upperBound(uint akey)
{
    SparseArrayNode *ub = root()->upperBound(akey);
    if (!ub)
        ub = end();
    return ub;
}


class Array
{
    friend struct ArrayPrototype;

    uint arrayLen;
    ArrayObject *arrayObject;
    union {
        uint arrayFreeList;
        uint arrayOffset;
    };
    QVector<PropertyDescriptor> arrayData;
    SparseArray *sparseArray;

    void fillDescriptor(PropertyDescriptor *pd, Value v)
    {
        pd->type = PropertyDescriptor::Data;
        pd->writable = PropertyDescriptor::Enabled;
        pd->enumberable = PropertyDescriptor::Enabled;
        pd->configurable = PropertyDescriptor::Enabled;
        pd->value = v;
    }

    uint allocArrayValue() {
        uint idx = arrayFreeList;
        if (arrayData.size() <= (int)arrayFreeList)
            arrayData.resize(++arrayFreeList);
        else
            arrayFreeList = arrayData.at(arrayFreeList).value.integerValue();
        return idx;
    }

    uint allocArrayValue(Value v) {
        uint idx = allocArrayValue();
        PropertyDescriptor *pd = &arrayData[idx];
        fillDescriptor(pd, v);
        return idx;
    }
    void freeArrayValue(int idx) {
        PropertyDescriptor &pd = arrayData[idx];
        pd.type = PropertyDescriptor::Generic;
        pd.value.tag = Value::_Undefined_Type;
        pd.value.int_32 = arrayFreeList;
        arrayFreeList = idx;
    }

    PropertyDescriptor *arrayDecriptor(uint index) {
        PropertyDescriptor *pd = arrayData.data() + index;
        if (!sparseArray)
            pd += arrayOffset;
        return pd;
    }
    const PropertyDescriptor *arrayDecriptor(uint index) const {
        const PropertyDescriptor *pd = arrayData.data() + index;
        if (!sparseArray)
            pd += arrayOffset;
        return pd;
    }

    void getArrayHeadRoom() {
        assert(!sparseArray && !arrayOffset);
        arrayOffset = qMax(arrayData.size() >> 2, 16);
        QVector<PropertyDescriptor> newValues(arrayData.size() + arrayOffset);
        memcpy(newValues.data() + arrayOffset, arrayData.constData(), arrayData.size()*sizeof(PropertyDescriptor));
        arrayData = newValues;
    }

public:
    Array() : arrayLen(0), arrayObject(0), arrayOffset(0), sparseArray(0) {}
    Array(const Array &other);
    ~Array() { delete sparseArray; }
    void initSparse();

    uint arrayLength() const { return arrayLen; }
    bool setArrayLength(uint newLen);

    void setArrayObject(ArrayObject *a) { arrayObject = a; }

    void setArrayLengthUnchecked(uint l);

    PropertyDescriptor *arrayInsert(uint index) {
        PropertyDescriptor *pd;
        if (!sparseArray && (index < 0x1000 || index < arrayLen + (arrayLen >> 2))) {
            if (index + arrayOffset >= (uint)arrayData.size()) {
                arrayData.resize(arrayOffset + index + 1);
                for (uint i = arrayLen + 1; i < index; ++i) {
                    arrayData[i].type = PropertyDescriptor::Generic;
                    arrayData[i].value.tag = Value::_Undefined_Type;
                }
            }
            pd = arrayDecriptor(index);
        } else {
            initSparse();
            SparseArrayNode *n = sparseArray->insert(index);
            if (n->value == UINT_MAX)
                n->value = allocArrayValue();
            pd = arrayDecriptor(n->value);
        }
        if (index >= arrayLen)
            setArrayLengthUnchecked(index + 1);
        return pd;
    }

    void arraySet(uint index, const PropertyDescriptor *pd) {
        *arrayInsert(index) = *pd;
    }

    void arraySet(uint index, Value value) {
        PropertyDescriptor *pd = arrayInsert(index);
        fillDescriptor(pd, value);
    }

    bool deleteArrayIndex(uint index) {
        if (index >= arrayLen)
            return true;
        PropertyDescriptor *pd = 0;
        if (!sparseArray) {
            pd = arrayAt(index);
        } else {
            SparseArrayNode *n = sparseArray->findNode(index);
            if (n)
                pd = arrayDecriptor(n->value);
        }
        if (!pd || pd->type == PropertyDescriptor::Generic)
            return true;
        if (!pd->isConfigurable())
            return false;
        pd->type = PropertyDescriptor::Generic;
        pd->value.tag = Value::_Undefined_Type;
        if (sparseArray) {
            pd->value.int_32 = arrayFreeList;
            arrayFreeList = pd - arrayData.constData();
        }
        return true;
    }

    PropertyDescriptor *arrayAt(uint index) {
        if (!sparseArray) {
            if (index >= arrayData.size() - arrayOffset)
                return 0;
            return arrayData.data() + index + arrayOffset;
        } else {
            SparseArrayNode *n = sparseArray->findNode(index);
            if (!n)
                return 0;
            return arrayData.data() + n->value;
        }
    }

    const PropertyDescriptor *nonSparseArrayAt(uint index) const {
        if (sparseArray)
            return 0;
        index += arrayOffset;
        if (index >= (uint)arrayData.size())
            return 0;
        return arrayData.constData() + index;
    }

    PropertyDescriptor *nonSparseArrayAtRef(uint index) {
        if (sparseArray)
            return 0;
        index += arrayOffset;
        if (index >= (uint)arrayData.size())
            return 0;
        return arrayData.data() + index;
    }

    const PropertyDescriptor *arrayAt(uint index) const {
        if (!sparseArray) {
            if (index >= arrayData.size() - arrayOffset)
                return 0;
            return arrayData.constData() + index + arrayOffset;
        } else {
            SparseArrayNode *n = sparseArray->findNode(index);
            if (!n)
                return 0;
            return arrayData.constData() + n->value;
        }
    }

    void markArrayObjects() const;

    void push_front(Value v) {
        if (!sparseArray) {
            if (!arrayOffset)
                getArrayHeadRoom();

            --arrayOffset;
            PropertyDescriptor &pd = values[offset];
            fillDescriptor(&pd, v);
        } else {
            uint idx = allocArrayValue(v);
            sparseArray->push_front(idx);
        }
        setArrayLengthUnchecked(arrayLen + 1);
    }
    PropertyDescriptor *front() {
        PropertyDescriptor *pd = 0;
        if (!sparseArray) {
            if (arrayLen)
                pd = arrayData.data() + arrayOffset;
        } else {
            SparseArrayNode *n = sparseArray->findNode(0);
            if (n)
                pd = arrayDecriptor(n->value);
        }
        if (pd && pd->type == PropertyDescriptor::Generic)
            return 0;
        return pd;
    }
    void pop_front() {
        if (!arrayLen)
            return;
        if (!sparseArray) {
            ++arrayOffset;
        } else {
            uint idx = sparseArray->pop_front();
            freeArrayValue(idx);
        }
        setArrayLengthUnchecked(arrayLen - 1);
    }
    void push_back(Value v) {
        if (!sparseArray) {
            PropertyDescriptor pd;
            fillDescriptor(&pd, v);
            arrayData.append(pd);
        } else {
            uint idx = allocArrayValue(v);
            sparseArray->push_back(idx, arrayLen);
        }
        setArrayLengthUnchecked(arrayLen + 1);
    }
    PropertyDescriptor *back() {
        PropertyDescriptor *pd = 0;
        if (!sparseArray) {
            if (arrayLen)
                pd = arrayData.data() + arrayOffset + arrayLen;
        } else {
            SparseArrayNode *n = sparseArray->findNode(arrayLen - 1);
            if (n)
                pd = arrayDecriptor(n->value);
        }
        if (pd && pd->type == PropertyDescriptor::Generic)
            return 0;
        return pd;
    }
    void pop_back() {
        if (!arrayLen)
            return;
        if (!sparseArray) {
            arrayData.resize(arrayData.size() - 1);
        } else {
            uint idx = sparseArray->pop_back(arrayLen);
            if (idx != UINT_MAX)
                freeArrayValue(idx);
        }
        setArrayLengthUnchecked(arrayLen - 1);
    }

    SparseArrayNode *sparseArrayLowerBound(uint idx) { return sparseArray ? sparseArray->lowerBound(idx) : 0; }
    SparseArrayNode *sparseArrayBegin() { return sparseArray ? sparseArray->begin() : 0; }
    SparseArrayNode *sparseArrayEnd() { return sparseArray ? sparseArray->end() : 0; }

    void arrayConcat(const Array &other);
    void arraySort(ExecutionContext *context, Object *thisObject, const Value &comparefn, uint arrayLen);
    Value arrayIndexOf(Value v, uint fromIndex, uint arrayLen, ExecutionContext *ctx, Object *o);
};

}
}

QT_END_NAMESPACE

#endif // QMAP_H
