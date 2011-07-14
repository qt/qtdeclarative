/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QHASHEDSTRING_P_H
#define QHASHEDSTRING_P_H

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
#include <QtCore/qstring.h>
#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

class QHashedStringRef;
class QHashedString : public QString
{
public:
    inline QHashedString();
    inline QHashedString(const QString &string);
    inline QHashedString(const QString &string, quint32);
    inline QHashedString(const QHashedString &string);

    inline QHashedString &operator=(const QHashedString &string);
    inline bool operator==(const QHashedString &string) const;
    inline bool operator==(const QHashedStringRef &string) const;

    inline quint32 hash() const;
    inline quint32 existingHash() const;

    static inline bool isUpper(const QChar &);
private:
    friend class QHashedStringRef;

    void computeHash() const;
    mutable quint32 m_hash;
};

class QHashedV8String 
{
public:
    inline QHashedV8String();
    explicit inline QHashedV8String(v8::Handle<v8::String>);
    inline QHashedV8String(const QHashedV8String &string);
    inline QHashedV8String &operator=(const QHashedV8String &other);

    inline bool operator==(const QHashedV8String &string);

    inline quint32 hash() const;
    inline int length() const; 
    inline quint32 symbolId() const;

    inline v8::Handle<v8::String> string() const;

private:
    v8::String::CompleteHashData m_hash;
    v8::Handle<v8::String> m_string;
};

class QHashedStringRef 
{
public:
    inline QHashedStringRef();
    inline QHashedStringRef(const QString &);
    inline QHashedStringRef(const QChar *, int);
    inline QHashedStringRef(const QChar *, int, quint32);
    inline QHashedStringRef(const QHashedString &);
    inline QHashedStringRef(const QHashedStringRef &);

    inline bool operator==(const QHashedString &string) const;
    inline bool operator==(const QHashedStringRef &string) const;

    inline quint32 hash() const;

    inline const QChar *constData() const;
    inline int length() const;
    inline bool startsWithUpper() const;

private:
    friend class QHashedString;

    void computeHash() const;

    const QChar *m_data;
    int m_length;
    mutable quint32 m_hash;
};

class QHashedCStringRef
{
public:
    inline QHashedCStringRef();
    inline QHashedCStringRef(const char *, int);
    inline QHashedCStringRef(const char *, int, quint32);
    inline QHashedCStringRef(const QHashedCStringRef &);

    inline quint32 hash() const;

    inline const char *constData() const;
    inline int length() const;
private:
    void computeHash() const;

    const char *m_data;
    int m_length;
    mutable quint32 m_hash;
};

class QStringHashData;
class QStringHashNode
{
public:
    QStringHashNode()
    : nlist(0), next(0), length(0), hash(0), pooled(0), ckey(0), symbolId()
    {
    }

    QStringHashNode(const QHashedString &key)
    : nlist(0), next(0), length(key.length()), hash(key.hash()), pooled(0), ckey(0), key(key), symbolId(0) {
    }

    QStringHashNode(const QHashedCStringRef &key)
    : nlist(0), next(0), length(key.length()), hash(key.hash()), pooled(0), ckey(key.constData()), symbolId(0) {
    }

    QStringHashNode(const QStringHashNode &o)
    : nlist(0), next(0), length(o.length), hash(o.hash), pooled(0), ckey(o.ckey), key(o.key), symbolId(o.symbolId) {
    }

    QStringHashNode *nlist;
    QStringHashNode *next;
    qint32 length;
    quint32 hash;

    quint32 pooled:1;
    const char *ckey;
    QString key;
    
    quint32 symbolId;

    inline bool equals(v8::Handle<v8::String> string) {
        return ckey?string->Equals((char*)ckey, length):
                    string->Equals((uint16_t*)key.constData(), length);
    }

    inline bool symbolEquals(const QHashedV8String &string) {
        Q_ASSERT(string.symbolId() != 0);
        return length == string.length() && hash == string.hash() && 
               (string.symbolId() == symbolId || equals(string.string()));
    }

    inline bool equals(const QHashedV8String &string) {
        return length == string.length() && hash == string.hash() && 
               equals(string.string());
    }

    inline bool equals(const QHashedStringRef &string) {
        return length == string.length() && 
            hash == string.hash() && 
               ckey?(cstrCompare(string.constData(), ckey, length)):
                       (0 == ::memcmp(string.constData(), key.constData(), length * sizeof(QChar)));
    }

    inline bool equals(const QHashedCStringRef &string) {
        return length == string.length() && 
            hash == string.hash() && 
               ckey?(0 == ::memcmp(string.constData(), ckey, length)):
                    (cstrCompare(key.constData(), string.constData(), length));
    }

private:
    static inline bool cstrCompare(const QChar *lhsChar, const char *rhs, int length) {
        Q_ASSERT(lhsChar && rhs);
        const uint16_t *lhs = (const uint16_t*)lhsChar;
        while (length--) 
            if (*lhs++ != *rhs++) return false;
        return true;
    }
};

struct QStringHashData
{
public:
    QStringHashData() 
    : nodes(0), buckets(0), numBuckets(0), size(0), numBits(0) {}

    QStringHashNode *nodes;
    QStringHashNode **buckets;
    int numBuckets;
    int size;
    short numBits;

    void rehash();
};

template<class T>
class QStringHash
{
private:
    struct Node : public QStringHashNode {
        Node(const QHashedString &key, const T &value) : QStringHashNode(key), value(value) {}
        Node(const QHashedCStringRef &key, const T &value) : QStringHashNode(key), value(value) {}
        Node(const Node &o) : QStringHashNode(o), value(o.value) {}
        Node() {}
        T value;
    };
    struct ReservedNodePool
    {
        ReservedNodePool() : count(0), used(0), nodes(0) {}
        ~ReservedNodePool() { delete [] nodes; }
        int count;
        int used;
        Node *nodes;
    };

    QStringHashData data;
    ReservedNodePool *nodePool;

    inline Node *findNode(const QHashedStringRef &) const;
    inline Node *findNode(const QHashedCStringRef &) const;
    inline Node *findNode(const QHashedV8String &) const;
    inline Node *findSymbolNode(const QHashedV8String &) const;
    inline Node *createNode(const QHashedString &, const T &);
    inline Node *createNode(const QHashedCStringRef &, const T &);

    inline Node *takeNode(const QHashedString &key, const T &value);
    inline Node *takeNode(const QHashedCStringRef &key, const T &value);
    inline Node *takeNode(const Node &o);

    inline void copy(const QStringHash<T> &);
public:
    inline QStringHash();
    inline QStringHash(const QStringHash &);
    inline ~QStringHash();

    QStringHash &operator=(const QStringHash<T> &);

    void copyAndReserve(const QStringHash<T> &other, int additionalReserve);

    inline bool isEmpty() const;
    inline void clear();
    inline int count() const;

    inline void insert(const QString &, const T &);
    inline void insert(const QHashedString &, const T &);
    inline void insert(const QHashedStringRef &, const T &);
    inline void insert(const QHashedCStringRef &, const T &);

    inline T *value(const QString &) const;
    inline T *value(const QHashedString &) const;
    inline T *value(const QHashedStringRef &) const;
    inline T *value(const QHashedV8String &) const;
    inline T *value(const QHashedCStringRef &) const;

    inline bool contains(const QString &) const;
    inline bool contains(const QHashedString &) const;
    inline bool contains(const QHashedStringRef &) const;
    inline bool contains(const QHashedCStringRef &) const;

    T &operator[](const QString &);
    T &operator[](const QHashedString &);
    T &operator[](const QHashedStringRef &);
    T &operator[](const QHashedCStringRef &);

    class ConstIterator {
    public:
        ConstIterator() : n(0) {}
        ConstIterator(Node *n) : n(n) {}

        ConstIterator &operator++() { n = (Node *)n->nlist; return *this; }
        bool operator==(const ConstIterator &o) const { return n == o.n; }
        bool operator!=(const ConstIterator &o) const { return n != o.n; }

        QHashedString key() const { 
            if (n->ckey) {
                return QHashedString(QString::fromLatin1(n->ckey, n->length), n->hash);
            } else {
                return QHashedString(n->key, n->hash);
            }
        }
        const T &value() const { return n->value; }
        const T &operator*() const { return n->value; }
    private:
        Node *n;
    };

    ConstIterator begin() const { return ConstIterator((Node *)data.nodes); }
    ConstIterator end() const { return ConstIterator(); }

    inline void reserve(int);
};

template<class T>
QStringHash<T>::QStringHash()
: nodePool(0)
{
}

template<class T>
QStringHash<T>::QStringHash(const QStringHash<T> &other)
: data(other.data), nodePool(0)
{
    reserve(other.count());
    copy(other);
}

template<class T>
QStringHash<T> &QStringHash<T>::operator=(const QStringHash<T> &other)
{
    if (&other == this)
        return *this;

    clear();

    data = other.data;
    reserve(other.count());
    copy(other);

    return *this;
}

template<class T>
void QStringHash<T>::copyAndReserve(const QStringHash<T> &other, int additionalReserve)
{
    clear();

    data = other.data;
    reserve(other.count() + additionalReserve);
    copy(other);
}

template<class T>
QStringHash<T>::~QStringHash()
{
    clear();
}

template<class T>
void QStringHash<T>::clear() 
{
    // If all the nodes were allocated from the node pool, we
    // don't need to clean them individually
    if (!nodePool || data.size != nodePool->used) {
        QStringHashNode *n = data.nodes;
        while (n) {
            Node *o = (Node *)n;
            n = n->nlist;
            if (!o->pooled) delete o;
        }
    }
    if (nodePool) delete nodePool; 
    delete [] data.buckets;

    data = QStringHashData();
    nodePool = 0;
}

template<class T>
bool QStringHash<T>::isEmpty() const
{
    return data.nodes == 0;
}

template<class T>
int QStringHash<T>::count() const
{
    return data.size;
}

template<class T>
typename QStringHash<T>::Node *QStringHash<T>::takeNode(const QHashedString &key, const T &value)
{
    if (nodePool && nodePool->used != nodePool->count) {
        Node *rv = nodePool->nodes + nodePool->used++;
        rv->length = key.length();
        rv->hash = key.hash();
        rv->key = key;
        rv->pooled = 1;
        rv->value = value;
        return rv;
    } else {
        return new Node(key, value);
    }
}

template<class T>
typename QStringHash<T>::Node *QStringHash<T>::takeNode(const QHashedCStringRef &key, const T &value)
{
    if (nodePool && nodePool->used != nodePool->count) {
        Node *rv = nodePool->nodes + nodePool->used++;
        rv->length = key.length();
        rv->hash = key.hash();
        rv->ckey = key.constData();
        rv->pooled = 1;
        rv->value = value;
        return rv;
    } else {
        return new Node(key, value);
    }
}

template<class T>
typename QStringHash<T>::Node *QStringHash<T>::takeNode(const Node &o)
{
    if (nodePool && nodePool->used != nodePool->count) {
        Node *rv = nodePool->nodes + nodePool->used++;
        rv->length = o.length;
        rv->hash = o.hash;
        rv->ckey = o.ckey;
        rv->key = o.key;
        rv->pooled = 1;
        rv->symbolId = o.symbolId;
        rv->value = o.value;
        return rv;
    } else {
        return new Node(o);
    }
}

template<class T>
void QStringHash<T>::copy(const QStringHash<T> &other)
{
    data.nodes = 0;
    data.buckets = 0;

    QStringHashNode *n = other.data.nodes;
    while (n) {
        Node *o = (Node *)n;
        Node *mynode = takeNode(*o);
        mynode->nlist = data.nodes;
        data.nodes = mynode;
        n = o->nlist;
    }

    data.rehash();
}

template<class T>
typename QStringHash<T>::Node *QStringHash<T>::createNode(const QHashedString &key, const T &value)
{
    if (data.size == data.numBuckets) 
        data.rehash();

    Node *n = takeNode(key, value);
    n->nlist = data.nodes;
    data.nodes = n;

    int bucket = key.hash() % data.numBuckets;
    n->next = data.buckets[bucket];
    data.buckets[bucket] = n;

    data.size++; 

    return n;
}

template<class T>
typename QStringHash<T>::Node *QStringHash<T>::createNode(const QHashedCStringRef &key, const T &value)
{
    if (data.size == data.numBuckets) 
        data.rehash();

    Node *n = takeNode(key, value);
    n->nlist = data.nodes;
    data.nodes = n;

    int bucket = key.hash() % data.numBuckets;
    n->next = data.buckets[bucket];
    data.buckets[bucket] = n;

    data.size++; 

    return n;
}

template<class T>
void QStringHash<T>::insert(const QString &key, const T &value)
{
    QHashedStringRef ch(key);
    Node *n = findNode(key);
    if (n) n->value = value;
    else createNode(QHashedString(key, ch.hash()), value);
}

template<class T>
void QStringHash<T>::insert(const QHashedString &key, const T &value)
{
    Node *n = findNode(key);
    if (n) n->value = value;
    else createNode(key, value);
}

template<class T>
void QStringHash<T>::insert(const QHashedStringRef &key, const T &value)
{
    Node *n = findNode(key);
    if (n) n->value = value;
    else createNode(key, value);
}

template<class T>
void QStringHash<T>::insert(const QHashedCStringRef &key, const T &value)
{
    Node *n = findNode(key);
    if (n) n->value = value;
    else createNode(key, value);
}

template<class T>
typename QStringHash<T>::Node *QStringHash<T>::findNode(const QHashedStringRef &string) const
{
    QStringHashNode *node = 0;
    if (data.numBuckets) {
        node = data.buckets[string.hash() % data.numBuckets];
        while (node && !node->equals(string))
            node = node->next;
    } 

    return (Node *)node;
}

template<class T>
typename QStringHash<T>::Node *QStringHash<T>::findNode(const QHashedCStringRef &string) const
{
    QStringHashNode *node = 0;
    if (data.numBuckets) {
        node = data.buckets[string.hash() % data.numBuckets];
        while (node && !node->equals(string))
            node = node->next;
    } 

    return (Node *)node;
}

template<class T>
typename QStringHash<T>::Node *QStringHash<T>::findNode(const QHashedV8String &string) const
{
    QStringHashNode *node = 0;
    if (data.numBuckets) {
        quint32 hash = string.hash();
        node = data.buckets[hash % data.numBuckets];
        while (node && !node->equals(string))
            node = node->next;
    } 

    return (Node *)node;
}

template<class T>
typename QStringHash<T>::Node *QStringHash<T>::findSymbolNode(const QHashedV8String &string) const
{
    Q_ASSERT(string.symbolId() != 0);

    QStringHashNode *node = 0;
    if (data.numBuckets) {
        quint32 hash = string.hash();
        node = data.buckets[hash % data.numBuckets];
        while (node && !node->symbolEquals(string))
            node = node->next;

        if (node)
            node->symbolId = string.symbolId();
    } 

    return (Node *)node;
}

template<class T>
T *QStringHash<T>::value(const QString &key) const
{
    Node *n = findNode(key);
    return n?&n->value:0;
}

template<class T>
T *QStringHash<T>::value(const QHashedString &key) const
{
    Node *n = findNode(key);
    return n?&n->value:0;
}

template<class T>
T *QStringHash<T>::value(const QHashedStringRef &key) const
{
    Node *n = findNode(key);
    return n?&n->value:0;
}

template<class T>
T *QStringHash<T>::value(const QHashedCStringRef &key) const
{
    Node *n = findNode(key);
    return n?&n->value:0;
}

template<class T>
T *QStringHash<T>::value(const QHashedV8String &string) const
{
    Node *n = string.symbolId()?findSymbolNode(string):findNode(string);
    return n?&n->value:0;
}

template<class T>
bool QStringHash<T>::contains(const QString &s) const
{
    return 0 != value(s);
}

template<class T>
bool QStringHash<T>::contains(const QHashedString &s) const
{
    return 0 != value(s);
}

template<class T>
bool QStringHash<T>::contains(const QHashedStringRef &s) const
{
    return 0 != value(s);
}

template<class T>
bool QStringHash<T>::contains(const QHashedCStringRef &s) const
{
    return 0 != value(s);
}

template<class T>
T &QStringHash<T>::operator[](const QString &key) 
{
    QHashedStringRef cs(key);
    Node *n = findNode(cs);
    if (n) return n->value;
    else return createNode(QHashedString(key, cs.hash()), T())->value;
}

template<class T>
T &QStringHash<T>::operator[](const QHashedString &key)
{
    Node *n = findNode(key);
    if (n) return n->value;
    else return createNode(key, T())->value;
}

template<class T>
T &QStringHash<T>::operator[](const QHashedStringRef &key) 
{
    Node *n = findNode(key);
    if (n) return n->value;
    else return createNode(key, T())->value;
}

template<class T>
T &QStringHash<T>::operator[](const QHashedCStringRef &key) 
{
    Node *n = findNode(key);
    if (n) return n->value;
    else return createNode(key, T())->value;
}

template<class T>
void QStringHash<T>::reserve(int n)
{
    if (nodePool || 0 == n)
        return;
    nodePool = new ReservedNodePool;
    nodePool->count = n;
    nodePool->used = 0;
    nodePool->nodes = new Node[n];
}

inline uint qHash(const QHashedString &string) 
{ 
    return uint(string.hash()); 
}

inline uint qHash(const QHashedStringRef &string) 
{ 
    return uint(string.hash()); 
}

QHashedString::QHashedString() 
: QString(), m_hash(0) 
{
}

QHashedString::QHashedString(const QString &string) 
: QString(string), m_hash(0) 
{
}

QHashedString::QHashedString(const QString &string, quint32 hash) 
: QString(string), m_hash(hash) 
{
}

QHashedString::QHashedString(const QHashedString &string) 
: QString(string), m_hash(string.m_hash) 
{
}

QHashedString &QHashedString::operator=(const QHashedString &string)
{
    static_cast<QString &>(*this) = string;
    m_hash = string.m_hash;
    return *this;
}

bool QHashedString::operator==(const QHashedString &string) const
{
    return (string.m_hash == m_hash || !string.m_hash || !m_hash) && 
           static_cast<const QString &>(*this) == static_cast<const QString &>(string);
}

bool QHashedString::operator==(const QHashedStringRef &string) const
{
    return length() == string.m_length &&
           (string.m_hash == m_hash || !string.m_hash || !m_hash) && 
           0 == ::memcmp(constData(), string.m_data, string.m_length * sizeof(QChar));
}

quint32 QHashedString::hash() const
{ 
    if (!m_hash) computeHash();
    return m_hash;
}

quint32 QHashedString::existingHash() const
{ 
    return m_hash;
}

bool QHashedString::isUpper(const QChar &qc)
{
    ushort c = qc.unicode();
    // Optimize for _, a-z and A-Z.
    return ((c != '_' ) && (!(c >= 'a' && c <= 'z')) &&
           ((c >= 'A' && c <= 'Z') || QChar::category(c) == QChar::Letter_Uppercase));
}

QHashedV8String::QHashedV8String()
{
}

QHashedV8String::QHashedV8String(v8::Handle<v8::String> string)
: m_hash(string->CompleteHash()), m_string(string)
{
    Q_ASSERT(!m_string.IsEmpty());
}

QHashedV8String::QHashedV8String(const QHashedV8String &string)
: m_hash(string.m_hash), m_string(string.m_string)
{
}

QHashedV8String &QHashedV8String::operator=(const QHashedV8String &other)
{
    m_hash = other.m_hash;
    m_string = other.m_string;
    return *this;
}

bool QHashedV8String::operator==(const QHashedV8String &string)
{
    return m_hash.hash == string.m_hash.hash && m_hash.length == string.m_hash.length &&
           m_string.IsEmpty() == m_string.IsEmpty() && 
           (m_string.IsEmpty() || m_string->StrictEquals(string.m_string));
}

quint32 QHashedV8String::hash() const
{
    return m_hash.hash;
}

int QHashedV8String::length() const
{
    return m_hash.length;
}

quint32 QHashedV8String::symbolId() const
{
    return m_hash.symbol_id;
}

v8::Handle<v8::String> QHashedV8String::string() const
{
    return m_string;
}

QHashedStringRef::QHashedStringRef() 
: m_data(0), m_length(0), m_hash(0) 
{
}

QHashedStringRef::QHashedStringRef(const QString &str)
: m_data(str.constData()), m_length(str.length()), m_hash(0)
{
}

QHashedStringRef::QHashedStringRef(const QChar *data, int length)
: m_data(data), m_length(length), m_hash(0)
{
}

QHashedStringRef::QHashedStringRef(const QChar *data, int length, quint32 hash)
: m_data(data), m_length(length), m_hash(hash)
{
}

QHashedStringRef::QHashedStringRef(const QHashedString &string)
: m_data(string.constData()), m_length(string.length()), m_hash(string.m_hash)
{
}

QHashedStringRef::QHashedStringRef(const QHashedStringRef &string)
: m_data(string.m_data), m_length(string.m_length), m_hash(string.m_hash)
{
}

bool QHashedStringRef::operator==(const QHashedString &string) const
{
    return m_length == string.length() && 
           (m_hash == string.m_hash || !m_hash || !string.m_hash) &&
           0 == ::memcmp(string.constData(), m_data, m_length * sizeof(QChar));
}

bool QHashedStringRef::operator==(const QHashedStringRef &string) const
{
    return m_length == string.m_length && 
           (m_hash == string.m_hash || !m_hash || !string.m_hash) &&
           0 == ::memcmp(string.m_data, m_data, m_length * sizeof(QChar));
}

const QChar *QHashedStringRef::constData() const
{
    return m_data;
}

int QHashedStringRef::length() const
{
    return m_length;
}

bool QHashedStringRef::startsWithUpper() const
{
    if (m_length < 1) return false;
    return QHashedString::isUpper(m_data[0]);
}

quint32 QHashedStringRef::hash() const
{ 
    if (!m_hash) computeHash();
    return m_hash;
}

QHashedCStringRef::QHashedCStringRef()
: m_data(0), m_length(0), m_hash(0)
{
}

QHashedCStringRef::QHashedCStringRef(const char *data, int length)
: m_data(data), m_length(length), m_hash(0)
{
}

QHashedCStringRef::QHashedCStringRef(const char *data, int length, quint32 hash)
: m_data(data), m_length(length), m_hash(hash)
{
}

QHashedCStringRef::QHashedCStringRef(const QHashedCStringRef &o)
: m_data(o.m_data), m_length(o.m_length), m_hash(o.m_hash)
{
}

quint32 QHashedCStringRef::hash() const
{
    if (!m_hash) computeHash();
    return m_hash;
}

const char *QHashedCStringRef::constData() const
{
    return m_data;
}

int QHashedCStringRef::length() const
{
    return m_length;
}


QT_END_NAMESPACE

#endif // QHASHEDSTRING_P_H
