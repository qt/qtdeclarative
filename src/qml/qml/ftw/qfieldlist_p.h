/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFIELDLIST_P_H
#define QFIELDLIST_P_H

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
#include <QtCore/qtaggedpointer.h>

#include <private/qflagpointer_p.h>

// QForwardFieldList is a super simple linked list that can only prepend
template<class N, N *N::*nextMember, typename Tag = QtPrivate::TagInfo<N>>
class QForwardFieldList
{
public:
    inline QForwardFieldList();
    inline N *first() const;
    inline N *takeFirst();

    inline void prepend(N *);
    template <typename OtherTag>
    inline void copyAndClearPrepend(QForwardFieldList<N, nextMember, OtherTag> &);

    inline bool isEmpty() const;
    inline bool isOne() const;
    inline bool isMany() const;

    static inline N *next(N *v);

    inline Tag tag() const;
    inline void setTag(Tag t);
private:
    QTaggedPointer<N, Tag> _first;
};

// QFieldList is a simple linked list, that can append and prepend and also
// maintains a count
template<class N, N *N::*nextMember>
class QFieldList
{
public:
    inline QFieldList();
    inline N *first() const;
    inline N *takeFirst();

    inline void append(N *);
    inline void prepend(N *);

    inline bool isEmpty() const;
    inline bool isOne() const;
    inline bool isMany() const;
    inline int count() const;

    inline void append(QFieldList<N, nextMember> &);
    inline void prepend(QFieldList<N, nextMember> &);
    inline void insertAfter(N *, QFieldList<N, nextMember> &);

    inline void copyAndClear(QFieldList<N, nextMember> &);
    template <typename Tag>
    inline void copyAndClearAppend(QForwardFieldList<N, nextMember, Tag> &);
    template <typename Tag>
    inline void copyAndClearPrepend(QForwardFieldList<N, nextMember, Tag> &);

    static inline N *next(N *v);

    inline bool flag() const;
    inline void setFlag();
    inline void clearFlag();
    inline void setFlagValue(bool);
private:
    N *_first;
    N *_last;
    quint32 _flag:1;
    quint32 _count:31;
};

template<class N, N *N::*nextMember, typename Tag>
QForwardFieldList<N, nextMember, Tag>::QForwardFieldList()
{
}

template<class N, N *N::*nextMember, typename Tag>
N *QForwardFieldList<N, nextMember, Tag>::first() const
{
    return _first.data();
}

template<class N, N *N::*nextMember, typename Tag>
N *QForwardFieldList<N, nextMember, Tag>::takeFirst()
{
    N *value = _first.data();
    if (value) {
        _first = next(value);
        value->*nextMember = nullptr;
    }
    return value;
}

template<class N, N *N::*nextMember, typename Tag>
void QForwardFieldList<N, nextMember, Tag>::prepend(N *v)
{
    Q_ASSERT(v->*nextMember == nullptr);
    v->*nextMember = _first.data();
    _first = v;
}

template<class N, N *N::*nextMember, typename Tag>
template <typename OtherTag>
void QForwardFieldList<N, nextMember, Tag>::copyAndClearPrepend(QForwardFieldList<N, nextMember, OtherTag> &o)
{
    _first = nullptr;
    while (N *n = o.takeFirst()) prepend(n);
}

template<class N, N *N::*nextMember, typename Tag>
bool QForwardFieldList<N, nextMember, Tag>::isEmpty() const
{
    return _first.isNull();
}

template<class N, N *N::*nextMember, typename Tag>
bool QForwardFieldList<N, nextMember, Tag>::isOne() const
{
    return _first.data() && _first->*nextMember == 0;
}

template<class N, N *N::*nextMember, typename Tag>
bool QForwardFieldList<N, nextMember, Tag>::isMany() const
{
    return _first.data() && _first->*nextMember != 0;
}

template<class N, N *N::*nextMember, typename Tag>
N *QForwardFieldList<N, nextMember, Tag>::next(N *v)
{
    Q_ASSERT(v);
    return v->*nextMember;
}

template<class N, N *N::*nextMember, typename Tag>
Tag QForwardFieldList<N, nextMember, Tag>::tag() const
{
    return _first.tag();
}

template<class N, N *N::*nextMember, typename Tag>
void QForwardFieldList<N, nextMember, Tag>::setTag(Tag t)
{
    _first.setTag(t);
}

template<class N, N *N::*nextMember>
QFieldList<N, nextMember>::QFieldList()
: _first(nullptr), _last(nullptr), _flag(0), _count(0)
{
}

template<class N, N *N::*nextMember>
N *QFieldList<N, nextMember>::first() const
{
    return _first;
}

template<class N, N *N::*nextMember>
N *QFieldList<N, nextMember>::takeFirst()
{
    N *value = _first;
    if (value) {
        _first = next(value);
        if (_last == value) {
            Q_ASSERT(_first == nullptr);
            _last = nullptr;
        }
        value->*nextMember = nullptr;
        --_count;
    }
    return value;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::append(N *v)
{
    Q_ASSERT(v->*nextMember == nullptr);
    if (isEmpty()) {
        _first = v;
        _last = v;
    } else {
        _last->*nextMember = v;
        _last = v;
    }
    ++_count;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::prepend(N *v)
{
    Q_ASSERT(v->*nextMember == nullptr);
    if (isEmpty()) {
        _first = v;
        _last = v;
    } else {
        v->*nextMember = _first;
        _first = v;
    }
    ++_count;
}

template<class N, N *N::*nextMember>
bool QFieldList<N, nextMember>::isEmpty() const
{
    return _count == 0;
}

template<class N, N *N::*nextMember>
bool QFieldList<N, nextMember>::isOne() const
{
    return _count == 1;
}

template<class N, N *N::*nextMember>
bool QFieldList<N, nextMember>::isMany() const
{
    return _count > 1;
}

template<class N, N *N::*nextMember>
int QFieldList<N, nextMember>::count() const
{
    return _count;
}

template<class N, N *N::*nextMember>
N *QFieldList<N, nextMember>::next(N *v)
{
    Q_ASSERT(v);
    return v->*nextMember;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::append(QFieldList<N, nextMember> &o)
{
    if (!o.isEmpty()) {
        if (isEmpty()) {
            _first = o._first;
            _last = o._last;
            _count = o._count;
        } else {
            _last->*nextMember = o._first;
            _last = o._last;
            _count += o._count;
        }
        o._first = o._last = 0; o._count = 0;
    }
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::prepend(QFieldList<N, nextMember> &o)
{
    if (!o.isEmpty()) {
        if (isEmpty()) {
            _first = o._first;
            _last = o._last;
            _count = o._count;
        } else {
            o._last->*nextMember = _first;
            _first = o._first;
            _count += o._count;
        }
        o._first = o._last = 0; o._count = 0;
    }
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::insertAfter(N *after, QFieldList<N, nextMember> &o)
{
    if (after == 0) {
        prepend(o);
    } else if (after == _last) {
        append(o);
    } else if (!o.isEmpty()) {
        if (isEmpty()) {
            _first = o._first;
            _last = o._last;
            _count = o._count;
        } else {
            o._last->*nextMember = after->*nextMember;
            after->*nextMember = o._first;
            _count += o._count;
        }
        o._first = o._last = 0; o._count = 0;
    }
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::copyAndClear(QFieldList<N, nextMember> &o)
{
    _first = o._first;
    _last = o._last;
    _count = o._count;
    o._first = o._last = nullptr;
    o._count = 0;
}

template<class N, N *N::*nextMember>
template <typename Tag>
void QFieldList<N, nextMember>::copyAndClearAppend(QForwardFieldList<N, nextMember, Tag> &o)
{
    _first = 0;
    _last = 0;
    _count = 0;
    while (N *n = o.takeFirst()) append(n);
}

template<class N, N *N::*nextMember>
template <typename Tag>
void QFieldList<N, nextMember>::copyAndClearPrepend(QForwardFieldList<N, nextMember, Tag> &o)
{
    _first = nullptr;
    _last = nullptr;
    _count = 0;
    while (N *n = o.takeFirst()) prepend(n);
}

template<class N, N *N::*nextMember>
bool QFieldList<N, nextMember>::flag() const
{
    return _flag;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::setFlag()
{
    _flag = true;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::clearFlag()
{
    _flag = false;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::setFlagValue(bool v)
{
    _flag = v;
}

#endif // QFIELDLIST_P_H
