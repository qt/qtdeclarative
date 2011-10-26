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

#ifndef QDECLARATIVEREFCOUNT_P_H
#define QDECLARATIVEREFCOUNT_P_H

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
#include <QtCore/qatomic.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeRefCount
{
public:
    inline QDeclarativeRefCount();
    inline virtual ~QDeclarativeRefCount();
    inline void addref();
    inline void release();
    inline bool isLastRef() const {return refCount == 1;}
protected:
    inline virtual void destroy();

private:
    QAtomicInt refCount;
};

template<class T>
class QDeclarativeRefPointer
{
public:
    inline QDeclarativeRefPointer();
    inline QDeclarativeRefPointer(T *);
    inline QDeclarativeRefPointer(const QDeclarativeRefPointer<T> &);
    inline ~QDeclarativeRefPointer();

    inline QDeclarativeRefPointer<T> &operator=(const QDeclarativeRefPointer<T> &o);
    inline QDeclarativeRefPointer<T> &operator=(T *);
    
    inline bool isNull() const { return !o; }

    inline bool operator()() const { return !isNull();}
    inline T* operator->() const { return o; }
    inline T& operator*() const { return *o; }
    inline operator T*() const { return o; }
    inline T* data() const { return o; }

    inline QDeclarativeRefPointer<T> &take(T *);

private:
    T *o;
};

QDeclarativeRefCount::QDeclarativeRefCount() 
: refCount(1) 
{
}

QDeclarativeRefCount::~QDeclarativeRefCount() 
{
    Q_ASSERT(refCount.load() == 0);
}

void QDeclarativeRefCount::addref() 
{ 
    Q_ASSERT(refCount.load() > 0);
    refCount.ref(); 
}

void QDeclarativeRefCount::release() 
{ 
    Q_ASSERT(refCount.load() > 0);
    if (!refCount.deref()) 
        destroy(); 
}

void QDeclarativeRefCount::destroy() 
{ 
    delete this; 
}

template<class T>
QDeclarativeRefPointer<T>::QDeclarativeRefPointer()
: o(0) 
{
}

template<class T>
QDeclarativeRefPointer<T>::QDeclarativeRefPointer(T *o)
: o(o) 
{
    if (o) o->addref();
}

template<class T>
QDeclarativeRefPointer<T>::QDeclarativeRefPointer(const QDeclarativeRefPointer<T> &other)
: o(other.o)
{
    if (o) o->addref();
}

template<class T>
QDeclarativeRefPointer<T>::~QDeclarativeRefPointer()
{
    if (o) {
        bool deleted = o->isLastRef();
        o->release();
        if (deleted)
            o = 0;
    }
}

template<class T>
QDeclarativeRefPointer<T> &QDeclarativeRefPointer<T>::operator=(const QDeclarativeRefPointer<T> &other)
{
    if (other.o) other.o->addref();
    if (o) {
        bool deleted = o->isLastRef();
        o->release();
        if (deleted)
            o = 0;
    }
    o = other.o;
    return *this;
}

template<class T>
QDeclarativeRefPointer<T> &QDeclarativeRefPointer<T>::operator=(T *other)
{
    if (other) other->addref();
    if (o) {
        bool deleted = o->isLastRef();
        o->release();
        if (deleted)
            o = 0;
    }
    o = other;
    return *this;
}

/*!
Takes ownership of \a other.  take() does *not* add a reference, as it assumes ownership
of the callers reference of other.
*/
template<class T>
QDeclarativeRefPointer<T> &QDeclarativeRefPointer<T>::take(T *other)
{
    if (o) {
        bool deleted = o->isLastRef();
        o->release();
        if (deleted)
            o = 0;
    }
    o = other;
    return *this;
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVEREFCOUNT_P_H
