// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QINTRUSIVELIST_P_H
#define QINTRUSIVELIST_P_H

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

#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QIntrusiveListNode;
template<class N, QIntrusiveListNode N::*member>
class QIntrusiveList
{
public:
    inline QIntrusiveList();
    inline ~QIntrusiveList();

    inline bool isEmpty() const;
    inline void insert(N *n);
    inline void remove(N *n);
    inline bool contains(N *) const;

    class iterator {
    public:
        inline iterator();
        inline iterator(N *value);

        inline N *operator*() const;
        inline N *operator->() const;
        inline bool operator==(const iterator &other) const;
        inline bool operator!=(const iterator &other) const;
        inline iterator &operator++();

        inline iterator &erase();

    private:
        N *_value;
    };
    typedef iterator Iterator;

    inline N *first() const;
    static inline N *next(N *current);

    inline iterator begin();
    inline iterator end();

private:
    static inline N *nodeToN(QIntrusiveListNode *node);

    QIntrusiveListNode *__first = nullptr;
};

class QIntrusiveListNode
{
public:
    inline QIntrusiveListNode();
    inline ~QIntrusiveListNode();

    inline void remove();
    inline bool isInList() const;

    QIntrusiveListNode *_next = nullptr;
    QIntrusiveListNode**_prev = nullptr;
};

template<class N, QIntrusiveListNode N::*member>
QIntrusiveList<N, member>::iterator::iterator()
: _value(nullptr)
{
}

template<class N, QIntrusiveListNode N::*member>
QIntrusiveList<N, member>::iterator::iterator(N *value)
: _value(value)
{
}

template<class N, QIntrusiveListNode N::*member>
N *QIntrusiveList<N, member>::iterator::operator*() const
{
    return _value;
}

template<class N, QIntrusiveListNode N::*member>
N *QIntrusiveList<N, member>::iterator::operator->() const
{
    return _value;
}

template<class N, QIntrusiveListNode N::*member>
bool QIntrusiveList<N, member>::iterator::operator==(const iterator &other) const
{
    return other._value == _value;
}

template<class N, QIntrusiveListNode N::*member>
bool QIntrusiveList<N, member>::iterator::operator!=(const iterator &other) const
{
    return other._value != _value;
}

template<class N, QIntrusiveListNode N::*member>
typename QIntrusiveList<N, member>::iterator &QIntrusiveList<N, member>::iterator::operator++()
{
    _value = QIntrusiveList<N, member>::next(_value);
    return *this;
}

template<class N, QIntrusiveListNode N::*member>
typename QIntrusiveList<N, member>::iterator &QIntrusiveList<N, member>::iterator::erase()
{
    N *old = _value;
    _value = QIntrusiveList<N, member>::next(_value);
    (old->*member).remove();
    return *this;
}

template<class N, QIntrusiveListNode N::*member>
QIntrusiveList<N, member>::QIntrusiveList()

{
}

template<class N, QIntrusiveListNode N::*member>
QIntrusiveList<N, member>::~QIntrusiveList()
{
    while (__first) __first->remove();
}

template<class N, QIntrusiveListNode N::*member>
bool QIntrusiveList<N, member>::isEmpty() const
{
    return __first == nullptr;
}

template<class N, QIntrusiveListNode N::*member>
void QIntrusiveList<N, member>::insert(N *n)
{
    QIntrusiveListNode *nnode = &(n->*member);
    nnode->remove();

    nnode->_next = __first;
    if (nnode->_next) nnode->_next->_prev = &nnode->_next;
    __first = nnode;
    nnode->_prev = &__first;
}

template<class N, QIntrusiveListNode N::*member>
void QIntrusiveList<N, member>::remove(N *n)
{
    QIntrusiveListNode *nnode = &(n->*member);
    nnode->remove();
}

template<class N, QIntrusiveListNode N::*member>
bool QIntrusiveList<N, member>::contains(N *n) const
{
    QIntrusiveListNode *nnode = __first;
    while (nnode) {
        if (nodeToN(nnode) == n)
            return true;
        nnode = nnode->_next;
    }
    return false;
}

template<class N, QIntrusiveListNode N::*member>
N *QIntrusiveList<N, member>::first() const
{
    return __first?nodeToN(__first):nullptr;
}

template<class N, QIntrusiveListNode N::*member>
N *QIntrusiveList<N, member>::next(N *current)
{
    QIntrusiveListNode *nextnode = (current->*member)._next;
    N *nextstruct = nextnode?nodeToN(nextnode):nullptr;
    return nextstruct;
}

template<class N, QIntrusiveListNode N::*member>
typename QIntrusiveList<N, member>::iterator QIntrusiveList<N, member>::begin()
{
    return __first?iterator(nodeToN(__first)):iterator();
}

template<class N, QIntrusiveListNode N::*member>
typename QIntrusiveList<N, member>::iterator QIntrusiveList<N, member>::end()
{
    return iterator();
}

template<class N, QIntrusiveListNode N::*member>
N *QIntrusiveList<N, member>::nodeToN(QIntrusiveListNode *node)
{
    QT_WARNING_PUSH
#if defined(Q_CC_CLANG) && Q_CC_CLANG >= 1300
    QT_WARNING_DISABLE_CLANG("-Wnull-pointer-subtraction")
#endif
    return (N *)((char *)node - ((char *)&(((N *)nullptr)->*member) - (char *)nullptr));
    QT_WARNING_POP
}

QIntrusiveListNode::QIntrusiveListNode()
{
}

QIntrusiveListNode::~QIntrusiveListNode()
{
    remove();
}

void QIntrusiveListNode::remove()
{
    if (_prev) *_prev = _next;
    if (_next) _next->_prev = _prev;
    _prev = nullptr;
    _next = nullptr;
}

bool QIntrusiveListNode::isInList() const
{
    return _prev != nullptr;
}

QT_END_NAMESPACE

#endif // QINTRUSIVELIST_P_H
