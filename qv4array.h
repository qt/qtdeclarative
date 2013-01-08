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

struct SparseArrayData;

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

    SparseArrayNode *copy(SparseArrayData *d) const;

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



struct Q_CORE_EXPORT SparseArrayData
{
    SparseArrayData();
    ~SparseArrayData() {
        if (root())
            freeTree(header.left, Q_ALIGNOF(SparseArrayNode));
    }

    int numEntries;
    SparseArrayNode header;
    SparseArrayNode *mostLeftNode;
    uint length;

    void rotateLeft(SparseArrayNode *x);
    void rotateRight(SparseArrayNode *x);
    void rebalance(SparseArrayNode *x);
    void recalcMostLeftNode();

    SparseArrayNode *createNode(uint sl, int value, SparseArrayNode *parent, bool left);
    void freeTree(SparseArrayNode *root, int alignment);

    SparseArrayNode *root() const { return header.left; }

    const SparseArrayNode *end() const { return &header; }
    SparseArrayNode *end() { return &header; }
    const SparseArrayNode *begin() const { if (root()) return mostLeftNode; return end(); }
    SparseArrayNode *begin() { if (root()) return mostLeftNode; return end(); }

    void deleteNode( SparseArrayNode *z);
    SparseArrayNode *findNode(uint akey) const;

};

inline SparseArrayNode *SparseArrayData::findNode(uint akey) const
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


class Array
{
    SparseArrayData *d;
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
    inline Array() : d(new SparseArrayData), freeList(0) {}
    Array(const Array &other);

    inline ~Array() { delete d; }

    Array &operator=(const Array &other);

    bool operator==(const Array &other) const;
    inline bool operator!=(const Array &other) const { return !(*this == other); }

    inline int numEntries() const { return d->numEntries; }
    inline uint length() const { return d->length; }
    void setLength(uint l);

    inline bool isEmpty() const { return d->numEntries == 0; }

    void clear();

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

    class const_iterator;

    class iterator
    {
        friend class const_iterator;
        SparseArrayNode *i;

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef int value_type;
        typedef int *pointer;
        typedef int &reference;

        inline iterator() : i(0) { }
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

#ifndef QT_STRICT_ITERATORS
    public:
        inline bool operator==(const const_iterator &o) const
            { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const
            { return i != o.i; }
#endif
        friend class Array;
    };
    friend class iterator;

    class const_iterator
    {
        friend class iterator;
        const SparseArrayNode *i;

    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef qptrdiff difference_type;
        typedef int value_type;
        typedef const int *pointer;
        typedef int reference;

        inline const_iterator() : i(0) { }
        inline const_iterator(const SparseArrayNode *node) : i(node) { }
#ifdef QT_STRICT_ITERATORS
        explicit inline const_iterator(const iterator &o)
#else
        inline const_iterator(const iterator &o)
#endif
        { i = o.i; }

        inline uint key() const { return i->key(); }
        inline int value() const { return i->value; }
        inline int operator*() const { return i->value; }
        inline const int *operator->() const { return &i->value; }
        inline bool operator==(const const_iterator &o) const { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const { return i != o.i; }

        inline const_iterator &operator++() {
            i = i->nextNode();
            return *this;
        }
        inline const_iterator operator++(int) {
            const_iterator r = *this;
            i = i->nextNode();
            return r;
        }
        inline const_iterator &operator--() {
            i = i->previousNode();
            return *this;
        }
        inline const_iterator operator--(int) {
            const_iterator r = *this;
            i = i->previousNode();
            return r;
        }
        inline const_iterator operator+(int j) const
        { const_iterator r = *this; if (j > 0) while (j--) ++r; else while (j++) --r; return r; }
        inline const_iterator operator-(int j) const { return operator+(-j); }
        inline const_iterator &operator+=(int j) { return *this = *this + j; }
        inline const_iterator &operator-=(int j) { return *this = *this - j; }

#ifdef QT_STRICT_ITERATORS
    private:
        inline bool operator==(const iterator &o) const { return operator==(const_iterator(o)); }
        inline bool operator!=(const iterator &o) const { return operator!=(const_iterator(o)); }
#endif
        friend class Array;
    };
    friend class const_iterator;

    // STL style
    inline iterator begin() { return iterator(d->begin()); }
    inline const_iterator begin() const { return const_iterator(d->begin()); }
    inline const_iterator constBegin() const { return const_iterator(d->begin()); }
    inline const_iterator cbegin() const { return const_iterator(d->begin()); }
    inline iterator end() { return iterator(d->end()); }
    inline const_iterator end() const { return const_iterator(d->end()); }
    inline const_iterator constEnd() const { return const_iterator(d->end()); }
    inline const_iterator cend() const { return const_iterator(d->end()); }
    iterator erase(iterator it);

    // more Qt
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    iterator find(uint key);
    const_iterator find(uint key) const;
    const_iterator constFind(uint key) const;
    iterator lowerBound(uint key);
    const_iterator lowerBound(uint key) const;
    iterator upperBound(uint key);
    const_iterator upperBound(uint key) const;
    iterator insert(uint akey, Value at);

    // STL compatibility
    typedef uint key_type;
    typedef int mapped_type;
    typedef qptrdiff difference_type;
    typedef int size_type;
    inline bool empty() const { return isEmpty(); }

#ifdef Q_MAP_DEBUG
    void dump() const;
#endif

    void concat(const Array &other);
    void sort(ExecutionContext *context, const Value &comparefn);
    void getCollectables(QVector<Object *> &objects) const;
    void splice(double start, double deleteCount, const QVector<Value> &, Array &);
};


inline Array::Array(const Array &other)
{
    d = 0;
    *this = other;
}


inline Array &Array::operator=(const Array &other)
{
    if (this != &other) {
        d = new SparseArrayData;
        if (other.d->header.left) {
            d->header.left = other.d->header.left->copy(d);
            d->header.left->setParent(&d->header);
            d->recalcMostLeftNode();
            d->length = other.d->length;
        }
    }
    return *this;
}


inline void Array::clear()
{
    *this = Array();
}



inline Value Array::at(uint akey) const
{
    SparseArrayNode *n = d->findNode(akey);
    int idx = n ? n->value : -1;
    return valueFromIndex(idx);
}


inline Value Array::operator[](uint akey) const
{
    return at(akey);
}


inline Value &Array::operator[](uint akey)
{
    SparseArrayNode *n = d->findNode(akey);
    if (!n)
        n = insert(akey, Value::undefinedValue()).i;
    return valueRefFromIndex(n->value);
}

inline Value Array::pop_front()
{
    int idx = -1 ;
    if (!d->length)
        return Value::undefinedValue();

    SparseArrayNode *n = d->findNode(0);
    if (n) {
        idx = n->value;
        d->deleteNode(n);
        // adjust all size_left indices on the path to leftmost item by 1
        SparseArrayNode *n = d->root();
        while (n) {
            n->size_left -= 1;
            n = n->left;
        }
    }
    --d->length;
    Value v = valueFromIndex(idx);
    freeValue(idx);
    return v;
}

inline void Array::push_front(Value value)
{
    // adjust all size_left indices on the path to leftmost item by 1
    SparseArrayNode *n = d->root();
    while (n) {
        n->size_left += 1;
        n = n->left;
    }
    ++d->length;
    insert(0, value);
}

inline Value Array::pop_back()
{
    int idx = -1;
    if (!d->length)
        return Value::undefinedValue();

    --d->length;
    SparseArrayNode *n = d->findNode(d->length);
    if (n) {
        idx = n->value;
        d->deleteNode(n);
    }
    Value v = valueFromIndex(idx);
    freeValue(idx);
    return v;
}

inline void Array::push_back(Value value)
{
    insert(d->length, value);
}


inline bool Array::contains(uint akey) const
{
    return d->findNode(akey) != 0;
}


inline Array::const_iterator Array::constFind(uint akey) const
{
    SparseArrayNode *n = d->findNode(akey);
    return const_iterator(n ? n : d->end());
}


inline Array::const_iterator Array::find(uint akey) const
{
    return constFind(akey);
}


inline Array::iterator Array::find(uint akey)
{
    SparseArrayNode *n = d->findNode(akey);
    return iterator(n ? n : d->end());
}

#ifdef Q_MAP_DEBUG

void SparseArray::dump() const
{
    const_iterator it = begin();
    qDebug() << "map dump:";
    while (it != end()) {
        const SparseArrayNode *n = it.i;
        int depth = 0;
        while (n && n != d->root()) {
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


inline bool Array::remove(uint akey)
{
    SparseArrayNode *node = d->findNode(akey);
    if (node) {
        d->deleteNode(node);
        return true;
    }
    return false;
}


inline Value Array::take(uint akey)
{
    SparseArrayNode *node = d->findNode(akey);
    int t;
    if (node) {
        t = node->value;
        d->deleteNode(node);
    } else {
        t = -1;
    }
    return valueFromIndex(t);
}


inline Array::iterator Array::erase(iterator it)
{
    if (it == iterator(d->end()))
        return it;

    SparseArrayNode *n = it.i;
    ++it;
    d->deleteNode(n);
    return it;
}

inline QList<int> Array::keys() const
{
    QList<int> res;
    res.reserve(numEntries());
    const_iterator i = begin();
    while (i != end()) {
        res.append(i.key());
        ++i;
    }
    return res;
}

inline Array::const_iterator Array::lowerBound(uint akey) const
{
    SparseArrayNode *lb = d->root()->lowerBound(akey);
    if (!lb)
        lb = d->end();
    return const_iterator(lb);
}


inline Array::iterator Array::lowerBound(uint akey)
{
    SparseArrayNode *lb = d->root()->lowerBound(akey);
    if (!lb)
        lb = d->end();
    return iterator(lb);
}


inline Array::const_iterator Array::upperBound(uint akey) const
{
    SparseArrayNode *ub = d->root()->upperBound(akey);
    if (!ub)
        ub = d->end();
    return const_iterator(ub);
}


inline Array::iterator Array::upperBound(uint akey)
{
    SparseArrayNode *ub = d->root()->upperBound(akey);
    if (!ub)
        ub = d->end();
    return iterator(ub);
}

inline void Array::concat(const Array &other)
{
    int newLen = d->length + other.d->length;
    for (const_iterator it = other.constBegin(); it != other.constEnd(); ++it) {
        insert(d->length + it.key(), other.valueFromIndex(it.value()));
    }
    d->length = newLen;
}

inline void Array::sort(ExecutionContext *context, const Value &comparefn)
{
    ArrayElementLessThan lessThan(context, comparefn);
    // ###
    //std::sort(to_vector.begin(), to_vector.end(), lessThan);
}

inline void Array::getCollectables(QVector<Object *> &objects) const
{
    for (const_iterator it = constBegin(), eit = constEnd(); it != eit; ++it) {
        if (Object *o = valueFromIndex(*it).asObject())
            objects.append(o);
    }
}


}
}

#endif // QMAP_H
