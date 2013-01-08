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

#include <QtCore/qmap.h>
#include <qmljs_value.h>

#ifdef Q_MAP_DEBUG
#include <QtCore/qdebug.h>
#endif

#include <new>

namespace QQmlJS {
namespace VM {

struct SparseArray;

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


struct SparseArrayNode
{
    quintptr p;
    SparseArrayNode *left;
    SparseArrayNode *right;
    uint size_left;
    int value;

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



struct Q_CORE_EXPORT SparseArray
{
    SparseArray();
    ~SparseArray() {
        if (root())
            freeTree(header.left, Q_ALIGNOF(SparseArrayNode));
    }

private:
    SparseArray(const SparseArray &other);
    SparseArray &operator=(const SparseArray &other);

    int numEntries;
    SparseArrayNode header;
    SparseArrayNode *mostLeftNode;

    void rotateLeft(SparseArrayNode *x);
    void rotateRight(SparseArrayNode *x);
    void rebalance(SparseArrayNode *x);
    void recalcMostLeftNode();

    SparseArrayNode *root() const { return header.left; }

    void deleteNode( SparseArrayNode *z);
    SparseArrayNode *findNode(uint akey) const;

    uint len;

    int allocValue() {
        int idx = freeList;
        if (values.size() <= freeList)
            values.resize(++freeList);
        else
            freeList = values.at(freeList).integerValue();

        return idx;
    }
    void freeValue(int idx) {
        values[idx] = Value::fromInt32(freeList);
        freeList = idx;
    }

    QVector<Value> values;
    int freeList;

public:
    SparseArrayNode *createNode(uint sl, int value, SparseArrayNode *parent, bool left);
    void freeTree(SparseArrayNode *root, int alignment);

    inline uint length() const { return len; }
    void setLength(uint l);

    bool remove(uint key);
    Value take(uint key);

    bool contains(uint key) const;
    Value at(uint key) const;
    Value &operator[](uint key);
    Value operator[](uint key) const;

    Value pop_front();
    void push_front(Value at);
    Value pop_back();
    void push_back(Value at);

    QList<int> keys() const;

    Value valueFromIndex(int idx) const {
        if (idx == -1)
            return Value::undefinedValue();
        return values.at(idx);
    }
    Value &valueRefFromIndex(int idx) {
        return values[idx];
    }

    const SparseArrayNode *end() const { return &header; }
    SparseArrayNode *end() { return &header; }
    const SparseArrayNode *begin() const { if (root()) return mostLeftNode; return end(); }
    SparseArrayNode *begin() { if (root()) return mostLeftNode; return end(); }

    SparseArrayNode *erase(SparseArrayNode *n);

    // more Qt
    SparseArrayNode *find(uint key);
    const SparseArrayNode *find(uint key) const;
    const SparseArrayNode *constFind(uint key) const;
    SparseArrayNode *lowerBound(uint key);
    const SparseArrayNode *lowerBound(uint key) const;
    SparseArrayNode *upperBound(uint key);
    const SparseArrayNode *upperBound(uint key) const;
    SparseArrayNode *insert(uint akey, Value at);

    // STL compatibility
    typedef uint key_type;
    typedef int mapped_type;
    typedef qptrdiff difference_type;
    typedef int size_type;

#ifdef Q_MAP_DEBUG
    void dump() const;
#endif

    void getCollectables(QVector<Object *> &objects) const;
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

inline Value SparseArray::at(uint akey) const
{
    SparseArrayNode *n = findNode(akey);
    int idx = n ? n->value : -1;
    return valueFromIndex(idx);
}


inline Value SparseArray::operator[](uint akey) const
{
    return at(akey);
}


inline Value &SparseArray::operator[](uint akey)
{
    SparseArrayNode *n = findNode(akey);
    if (!n)
        n = insert(akey, Value::undefinedValue());
    return valueRefFromIndex(n->value);
}

inline Value SparseArray::pop_front()
{
    int idx = -1 ;
    if (!len)
        return Value::undefinedValue();

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
    --len;
    Value v = valueFromIndex(idx);
    freeValue(idx);
    return v;
}

inline void SparseArray::push_front(Value value)
{
    // adjust all size_left indices on the path to leftmost item by 1
    SparseArrayNode *n = root();
    while (n) {
        n->size_left += 1;
        n = n->left;
    }
    ++len;
    insert(0, value);
}

inline Value SparseArray::pop_back()
{
    int idx = -1;
    if (!len)
        return Value::undefinedValue();

    --len;
    SparseArrayNode *n = findNode(len);
    if (n) {
        idx = n->value;
        deleteNode(n);
    }
    Value v = valueFromIndex(idx);
    freeValue(idx);
    return v;
}

inline void SparseArray::push_back(Value value)
{
    insert(len, value);
}


inline bool SparseArray::contains(uint akey) const
{
    return findNode(akey) != 0;
}


inline const SparseArrayNode *SparseArray::constFind(uint akey) const
{
    SparseArrayNode *n = findNode(akey);
    return n ? n : end();
}


inline const SparseArrayNode *SparseArray::find(uint akey) const
{
    return constFind(akey);
}


inline SparseArrayNode *SparseArray::find(uint akey)
{
    SparseArrayNode *n = findNode(akey);
    return n ? n : end();
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


inline bool SparseArray::remove(uint akey)
{
    SparseArrayNode *node = findNode(akey);
    if (node) {
        deleteNode(node);
        return true;
    }
    return false;
}


inline Value SparseArray::take(uint akey)
{
    SparseArrayNode *node = findNode(akey);
    int t;
    if (node) {
        t = node->value;
        deleteNode(node);
    } else {
        t = -1;
    }
    return valueFromIndex(t);
}


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

inline void SparseArray::getCollectables(QVector<Object *> &objects) const
{
    for (const SparseArrayNode *it = begin(), *eit = end(); it != eit; it = it->nextNode()) {
        if (Object *o = valueFromIndex(it->value).asObject())
            objects.append(o);
    }
}

class Array
{
    SparseArray *d;

public:
    Array() : d(0) {}
    ~Array() { delete d; }
    void init() { if (!d) d = new SparseArray; }

    uint length() const { return d ? d->length() : 0; }
    void setLength(uint l) {
        init();
        d->setLength(l);
    }

    struct iterator
    {
        SparseArrayNode *i;

        inline iterator( SparseArrayNode *node) : i(node) { }

        inline uint key() const { return i->key(); }
        inline int &value() const { return i->value; }
        inline int &operator*() const { return i->value; }
        inline int *operator->() const { return &i->value; }
        inline bool operator==(const iterator &o) const { return i == o.i; }
        inline bool operator!=(const iterator &o) const { return i != o.i; }

        inline iterator &operator++() {
            i = i->nextNode();
            return *this;
        }
        inline iterator operator++(int) {
            iterator r = *this;
            i = i->nextNode();
            return r;
        }
        inline iterator &operator--() {
            i = i->previousNode();
            return *this;
        }
        inline iterator operator--(int) {
            iterator r = *this;
            i = i->previousNode();
            return r;
        }
        inline iterator operator+(int j) const
        { iterator r = *this; if (j > 0) while (j--) ++r; else while (j++) --r; return r; }
        inline iterator operator-(int j) const { return operator+(-j); }
        inline iterator &operator+=(int j) { return *this = *this + j; }
        inline iterator &operator-=(int j) { return *this = *this - j; }

        friend class SparseArray;
    };

    iterator begin() const {
        return iterator(d ? d->begin() : 0);
    }
    iterator end() const {
        return iterator(d ? d->end() : 0);
    }

    iterator find(uint index) const {
        return iterator(d ? d->find(index) : 0);
    }

    void set(uint index, Value value) {
        init();
        (*d)[index] = value;
    }
    Value at(uint index) const {
        return d ? d->at(index) : Value::undefinedValue();
    }
    Value at(iterator it) const {
        return it.i ? d->valueFromIndex(*it) : Value::undefinedValue();
    }

    void getCollectables(QVector<Object *> &objects) const {
        if (d)
            d->getCollectables(objects);
    }

    void push_front(Value v) {
        init();
        d->push_front(v);
    }
    Value pop_front() {
        init();
        return d->pop_front();
    }
    void push_back(Value v) {
        init();
        d->push_back(v);
    }
    Value pop_back() {
        init();
        return d->pop_back();
    }

    void concat(const Array &other);
    void sort(ExecutionContext *context, const Value &comparefn);
    void splice(double start, double deleteCount, const QVector<Value> &, Array &);
};

inline void Array::concat(const Array &other)
{
    init();
    int len = length();
    int newLen = len + other.length();
    for (const SparseArrayNode *it = other.d->begin(); it != other.d->end(); it = it->nextNode()) {
        set(len + it->key(), other.d->valueFromIndex(it->value));
    }
    setLength(newLen);
}

inline void Array::sort(ExecutionContext *context, const Value &comparefn)
{
    if (!d)
        return;

    ArrayElementLessThan lessThan(context, comparefn);
    // ###
    //std::sort(to_vector.begin(), to_vector.end(), lessThan);
}


}
}

#endif // QMAP_H
