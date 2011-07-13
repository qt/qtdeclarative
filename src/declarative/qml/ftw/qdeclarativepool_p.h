/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEPOOL_P_H
#define QDECLARATIVEPOOL_P_H

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

QT_BEGIN_NAMESPACE

class QDeclarativePool
{
public:
    // The class has a destructor that needs to be called
    class Class { 
    public:
        inline QDeclarativePool *pool() const;

    private:
        void *operator new(size_t);
        void *operator new(size_t, void *m) { return m; }
        friend class QDeclarativePool;

        QDeclarativePool *_pool;
        Class *_next;
        void (*_destroy)(Class *);
    };

    // The class is plain old data and no destructor needs to
    // be called
    class POD {
    public:
        inline QDeclarativePool *pool() const;
        
    private:
        void *operator new(size_t);
        void *operator new(size_t, void *m) { return m; }
        friend class QDeclarativePool;

        QDeclarativePool *_pool;
    };

    inline QDeclarativePool();
    inline ~QDeclarativePool();

    void clear();

    template<typename T>
    inline T *New();

    inline QString *NewString(const QString &);

private:
    struct StringClass : public QString, public Class {
    };

    inline void *allocate(int size);
    void newpage();

    template<typename T>
    inline void initialize(POD *);
    template<typename T>
    inline void initialize(Class *);
    template<typename T>
    static void destroy(Class *c);

    struct Page {
        struct Header {
            Page *next;
            char *free;
        } header;

        static const int pageSize = 4 * 4096 - sizeof(Header);

        char memory[pageSize];
    };

    Page *_page;
    Class *_classList;
};

QDeclarativePool::QDeclarativePool()
: _page(0), _classList(0)
{
}

QDeclarativePool::~QDeclarativePool()
{
    clear();
}

template<typename T>
T *QDeclarativePool::New()
{
    T *rv = new (allocate(sizeof(T))) T;
    initialize<T>(rv);
    rv->_pool = this;
    return rv;
}

QString *QDeclarativePool::NewString(const QString &s)
{
    QString *rv = New<StringClass>();
    *rv = s;
    return rv;
}

void *QDeclarativePool::allocate(int size)
{
    if (!_page || (_page->header.free + size) > (_page->memory + Page::pageSize))
        newpage();

    void *rv = _page->header.free;
    _page->header.free += size + ((8 - size) & 7); // ensure 8 byte alignment;
    return rv;
}

template<typename T>
void QDeclarativePool::initialize(QDeclarativePool::POD *)
{
}

template<typename T>
void QDeclarativePool::initialize(QDeclarativePool::Class *c)
{
    c->_next = _classList;
    c->_destroy = &destroy<T>;
    _classList = c;
}

template<typename T>
void QDeclarativePool::destroy(Class *c)
{
    static_cast<T *>(c)->~T();
}

QDeclarativePool *QDeclarativePool::Class::pool() const
{
    return _pool;
}

QDeclarativePool *QDeclarativePool::POD::pool() const
{
    return _pool;
}

QT_END_NAMESPACE

#endif // QDECLARATIVEPOOL_P_H

