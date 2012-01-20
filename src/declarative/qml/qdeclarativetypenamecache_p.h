/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#ifndef QDECLARATIVETYPENAMECACHE_P_H
#define QDECLARATIVETYPENAMECACHE_P_H

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
#include "qdeclarativecleanup_p.h"
#include "qdeclarativemetatype_p.h"

#include <private/qhashedstring_p.h>

#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QDeclarativeType;
class QDeclarativeEngine;
class QDeclarativeTypeNameCache : public QDeclarativeRefCount
{
public:
    QDeclarativeTypeNameCache();
    virtual ~QDeclarativeTypeNameCache();

    inline bool isEmpty() const;

    void add(const QHashedString &, int);

    struct Result {
        inline Result();
        inline Result(const void *importNamespace);
        inline Result(QDeclarativeType *type);
        inline Result(int scriptIndex);
        inline Result(const Result &);

        inline bool isValid() const;

        QDeclarativeType *type;
        const void *importNamespace;
        int scriptIndex;
    };
    Result query(const QHashedStringRef &);
    Result query(const QHashedStringRef &, const void *importNamespace);
    Result query(const QHashedV8String &);
    Result query(const QHashedV8String &, const void *importNamespace);
    QDeclarativeMetaType::ModuleApiInstance *moduleApi(const void *importNamespace);

private:
    friend class QDeclarativeImports;

    struct Import {
        inline Import();
        // Imported module
        QDeclarativeMetaType::ModuleApiInstance *moduleApi;
        QVector<QDeclarativeTypeModuleVersion> modules;

        // Or, imported script
        int scriptIndex;
    };

    QStringHash<Import> m_namedImports;
    QVector<QDeclarativeTypeModuleVersion> m_anonymousImports;

    QDeclarativeEngine *engine;
};

QDeclarativeTypeNameCache::Result::Result()
: type(0), importNamespace(0), scriptIndex(-1)
{
}

QDeclarativeTypeNameCache::Result::Result(const void *importNamespace)
: type(0), importNamespace(importNamespace), scriptIndex(-1)
{
}

QDeclarativeTypeNameCache::Result::Result(QDeclarativeType *type)
: type(type), importNamespace(0), scriptIndex(-1)
{
}

QDeclarativeTypeNameCache::Result::Result(int scriptIndex)
: type(0), importNamespace(0), scriptIndex(scriptIndex)
{
}

QDeclarativeTypeNameCache::Result::Result(const Result &o)
: type(o.type), importNamespace(o.importNamespace), scriptIndex(o.scriptIndex)
{
}

bool QDeclarativeTypeNameCache::Result::isValid() const
{
    return type || importNamespace || scriptIndex != -1;
}

QDeclarativeTypeNameCache::Import::Import()
: moduleApi(0), scriptIndex(-1)
{
}

bool QDeclarativeTypeNameCache::isEmpty() const
{
    return m_namedImports.isEmpty() && m_anonymousImports.isEmpty();
}

QT_END_NAMESPACE

#endif // QDECLARATIVETYPENAMECACHE_P_H

