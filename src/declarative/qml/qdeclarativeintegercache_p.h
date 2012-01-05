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

#ifndef QDECLARATIVEINTEGERCACHE_P_H
#define QDECLARATIVEINTEGERCACHE_P_H

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

#include <private/qdeclarativerefcount_p.h>
#include <private/qhashedstring_p.h>

QT_BEGIN_NAMESPACE

class QDeclarativeType;
class QDeclarativeEngine;
class QDeclarativeIntegerCache : public QDeclarativeRefCount
{
public:
    QDeclarativeIntegerCache();
    virtual ~QDeclarativeIntegerCache();

    inline int count() const;
    void add(const QString &, int);
    void reserve(int);

    int value(const QString &);
    inline int value(const QHashedV8String &);

    QString findId(int value) const;

private:
    typedef QStringHash<int> StringCache;
    StringCache stringCache;
};

int QDeclarativeIntegerCache::value(const QHashedV8String &name)
{
    int *result = stringCache.value(name);
    return result?*result:-1;
}

int QDeclarativeIntegerCache::count() const 
{
    return stringCache.count();
}

QT_END_NAMESPACE

#endif // QDECLARATIVEINTEGERCACHE_P_H

