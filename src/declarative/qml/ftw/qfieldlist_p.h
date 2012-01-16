/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

    static inline N *next(N *v);

private:
    N *_first;
    N *_last;
    int _count;
};

template<class N, N *N::*nextMember>
QFieldList<N, nextMember>::QFieldList()
: _first(0), _last(0), _count(0)
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
            Q_ASSERT(_first == 0);
            _last = 0;
        }
        value->*nextMember = 0;
        --_count;
    } 
    return value;
}

template<class N, N *N::*nextMember>
void QFieldList<N, nextMember>::append(N *v)
{
    Q_ASSERT(v->*nextMember == 0);
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
    Q_ASSERT(v->*nextMember == 0);
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
    o._first = o._last = 0;
    o._count = 0;
}

#endif // QFIELDLIST_P_H
