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

#ifndef QFINITESTACK_P_H
#define QFINITESTACK_P_H

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

QT_BEGIN_NAMESPACE

template<typename T>
struct QFiniteStack {
    inline QFiniteStack();
    inline ~QFiniteStack();

    inline void deallocate();
    inline void allocate(int size);

    inline bool isEmpty() const;
    inline const T &top() const;
    inline T &top();
    inline void push(const T &o);
    inline T pop();
    inline int count() const;
    inline const T &at(int index) const;
    inline T &operator[](int index);
private:
    T *_array;
    int _alloc;
    int _size;
};

template<typename T>
QFiniteStack<T>::QFiniteStack()
: _array(0), _alloc(0), _size(0) 
{
}

template<typename T>
QFiniteStack<T>::~QFiniteStack()
{
    deallocate();
}

template<typename T>
bool QFiniteStack<T>::isEmpty() const
{
    return _size == 0;
}

template<typename T>
const T &QFiniteStack<T>::top() const
{
    return _array[_size - 1];
}

template<typename T>
T &QFiniteStack<T>::top()
{
    return _array[_size - 1];
}

template<typename T>
void QFiniteStack<T>::push(const T &o)
{
    if (QTypeInfo<T>::isComplex) {
        new (_array + _size++) T(o);
    } else {
        _array[_size++] = o;
    }
}

template<typename T>
T QFiniteStack<T>::pop()
{
    --_size;

    if (QTypeInfo<T>::isComplex) {
        T rv = _array[_size];
        (_array + _size)->~T();
        return rv;
    } else {
        return _array[_size];
    }
}
    
template<typename T>
int QFiniteStack<T>::count() const
{
    return _size;
}

template<typename T>
const T &QFiniteStack<T>::at(int index) const
{
    return _array[index];
}

template<typename T>
T &QFiniteStack<T>::operator[](int index)
{
    return _array[index];
}

template<typename T>
void QFiniteStack<T>::allocate(int size) 
{
    Q_ASSERT(_array == 0);
    Q_ASSERT(_alloc == 0);
    Q_ASSERT(_size == 0);

    if (!size) return;

    _array = (T *)qMalloc(size * sizeof(T));
    _alloc = size;
}

template<typename T>
void QFiniteStack<T>::deallocate()
{
    if (QTypeInfo<T>::isComplex) {
        T *i = _array + _size;
        while (i != _array) 
            (--i)->~T();
    }

    qFree(_array);

    _array = 0;
    _alloc = 0;
    _size = 0;
}

QT_END_NAMESPACE

#endif // QFINITESTACK_P_H

