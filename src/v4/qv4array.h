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

    uint len;
    PropertyDescriptor *lengthProperty;
    union {
        uint freeList;
        uint offset;
    };
    QVector<PropertyDescriptor> values;
    SparseArray *sparse;

    void fillDescriptor(PropertyDescriptor *pd, Value v)
    {
        pd->type = PropertyDescriptor::Data;
        pd->writable = PropertyDescriptor::Enabled;
        pd->enumberable = PropertyDescriptor::Enabled;
        pd->configurable = PropertyDescriptor::Enabled;
        pd->value = v;
    }

    uint allocValue() {
        uint idx = freeList;
        if (values.size() <= (int)freeList)
            values.resize(++freeList);
        else
            freeList = values.at(freeList).value.integerValue();
        return idx;
    }

    uint allocValue(Value v) {
        uint idx = allocValue();
        PropertyDescriptor *pd = &values[idx];
        fillDescriptor(pd, v);
        return idx;
    }
    void freeValue(int idx) {
        PropertyDescriptor &pd = values[idx];
        pd.type = PropertyDescriptor::Generic;
        pd.value.tag = Value::_Undefined_Type;
        pd.value.int_32 = freeList;
        freeList = idx;
    }

    PropertyDescriptor *descriptor(uint index) {
        PropertyDescriptor *pd = values.data() + index;
        if (!sparse)
            pd += offset;
        return pd;
    }
    const PropertyDescriptor *descriptor(uint index) const {
        const PropertyDescriptor *pd = values.data() + index;
        if (!sparse)
            pd += offset;
        return pd;
    }

    void getHeadRoom() {
        assert(!sparse && !offset);
        offset = qMax(values.size() >> 2, 16);
        QVector<PropertyDescriptor> newValues(values.size() + offset);
        memcpy(newValues.data() + offset, values.constData(), values.size()*sizeof(PropertyDescriptor));
        values = newValues;
    }

public:
    Array() : len(0), lengthProperty(0), offset(0), sparse(0) {}
    Array(const Array &other);
    ~Array() { delete sparse; }
    void initSparse();

    uint length() const { return len; }
    bool setLength(uint newLen);

    void setLengthProperty(PropertyDescriptor *pd) { lengthProperty = pd; }
    PropertyDescriptor *getLengthProperty() { return lengthProperty; }

    void setLengthUnchecked(uint l) {
        len = l;
        if (lengthProperty)
            lengthProperty->value = Value::fromUInt32(l);
    }

    PropertyDescriptor *insert(uint index) {
        PropertyDescriptor *pd;
        if (!sparse && (index < 0x1000 || index < len + (len >> 2))) {
            if (index + offset >= (uint)values.size()) {
                values.resize(offset + index + 1);
                for (uint i = len + 1; i < index; ++i) {
                    values[i].type = PropertyDescriptor::Generic;
                    values[i].value.tag = Value::_Undefined_Type;
                }
            }
            pd = descriptor(index);
        } else {
            initSparse();
            SparseArrayNode *n = sparse->insert(index);
            if (n->value == UINT_MAX)
                n->value = allocValue();
            pd = descriptor(n->value);
        }
        if (index >= len)
            setLengthUnchecked(index + 1);
        return pd;
    }

    void set(uint index, const PropertyDescriptor *pd) {
        *insert(index) = *pd;
    }

    void set(uint index, Value value) {
        PropertyDescriptor *pd = insert(index);
        fillDescriptor(pd, value);
    }

    bool deleteIndex(uint index) {
        if (index >= len)
            return true;
        PropertyDescriptor *pd = 0;
        if (!sparse) {
            pd = at(index);
        } else {
            SparseArrayNode *n = sparse->findNode(index);
            if (n)
                pd = descriptor(n->value);
        }
        if (!pd || pd->type == PropertyDescriptor::Generic)
            return true;
        if (!pd->isConfigurable())
            return false;
        pd->type = PropertyDescriptor::Generic;
        pd->value.tag = Value::_Undefined_Type;
        if (sparse) {
            pd->value.int_32 = freeList;
            freeList = pd - values.constData();
        }
        return true;
    }

    PropertyDescriptor *at(uint index) {
        if (!sparse) {
            if (index >= values.size() - offset)
                return 0;
            return values.data() + index + offset;
        } else {
            SparseArrayNode *n = sparse->findNode(index);
            if (!n)
                return 0;
            return values.data() + n->value;
        }
    }

    const PropertyDescriptor *nonSparseAt(uint index) const {
        if (sparse)
            return 0;
        index += offset;
        if (index >= (uint)values.size())
            return 0;
        return values.constData() + index;
    }

    PropertyDescriptor *nonSparseAtRef(uint index) {
        if (sparse)
            return 0;
        index += offset;
        if (index >= (uint)values.size())
            return 0;
        return values.data() + index;
    }

    const PropertyDescriptor *at(uint index) const {
        if (!sparse) {
            if (index >= values.size() - offset)
                return 0;
            return values.constData() + index + offset;
        } else {
            SparseArrayNode *n = sparse->findNode(index);
            if (!n)
                return 0;
            return values.constData() + n->value;
        }
    }

    void markObjects() const;

    void push_front(Value v) {
        if (!sparse) {
            if (!offset)
                getHeadRoom();

            PropertyDescriptor pd;
            fillDescriptor(&pd, v);
            --offset;
            values[offset] = pd;
        } else {
            uint idx = allocValue(v);
            sparse->push_front(idx);
        }
        setLengthUnchecked(len + 1);
    }
    PropertyDescriptor *front() {
        PropertyDescriptor *pd = 0;
        if (!sparse) {
            if (len)
                pd = values.data() + offset;
        } else {
            SparseArrayNode *n = sparse->findNode(0);
            if (n)
                pd = descriptor(n->value);
        }
        if (pd && pd->type == PropertyDescriptor::Generic)
            return 0;
        return pd;
    }
    void pop_front() {
        if (!len)
            return;
        if (!sparse) {
            ++offset;
        } else {
            uint idx = sparse->pop_front();
            freeValue(idx);
        }
        setLengthUnchecked(len - 1);
    }
    void push_back(Value v) {
        if (!sparse) {
            PropertyDescriptor pd;
            fillDescriptor(&pd, v);
            values.append(pd);
        } else {
            uint idx = allocValue(v);
            sparse->push_back(idx, len);
        }
        setLengthUnchecked(len + 1);
    }
    PropertyDescriptor *back() {
        PropertyDescriptor *pd = 0;
        if (!sparse) {
            if (len)
                pd = values.data() + offset + len;
        } else {
            SparseArrayNode *n = sparse->findNode(len - 1);
            if (n)
                pd = descriptor(n->value);
        }
        if (pd && pd->type == PropertyDescriptor::Generic)
            return 0;
        return pd;
    }
    void pop_back() {
        if (!len)
            return;
        if (!sparse) {
            values.resize(values.size() - 1);
        } else {
            uint idx = sparse->pop_back(len);
            if (idx != UINT_MAX)
                freeValue(idx);
        }
        setLengthUnchecked(len - 1);
    }

    SparseArrayNode *sparseLowerBound(uint idx) { return sparse ? sparse->lowerBound(idx) : 0; }
    SparseArrayNode *sparseBegin() { return sparse ? sparse->begin() : 0; }
    SparseArrayNode *sparseEnd() { return sparse ? sparse->end() : 0; }

    void concat(const Array &other);
    void sort(ExecutionContext *context, Object *thisObject, const Value &comparefn, uint len);
    Value indexOf(Value v, uint fromIndex, uint len, ExecutionContext *ctx, Object *o);
};

}
}

QT_END_NAMESPACE

#endif // QMAP_H
