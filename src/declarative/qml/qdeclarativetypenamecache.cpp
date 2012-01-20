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

#include "qdeclarativetypenamecache_p.h"

#include "qdeclarativeengine_p.h"

QT_BEGIN_NAMESPACE

QDeclarativeTypeNameCache::QDeclarativeTypeNameCache()
{
}

QDeclarativeTypeNameCache::~QDeclarativeTypeNameCache()
{
}

void QDeclarativeTypeNameCache::add(const QHashedString &name, int importedScriptIndex)
{
    if (m_namedImports.contains(name))
        return;

    Import import;
    import.scriptIndex = importedScriptIndex;
    m_namedImports.insert(name, import);
}

QDeclarativeTypeNameCache::Result QDeclarativeTypeNameCache::query(const QHashedStringRef &name)
{
    Import *i = m_namedImports.value(name);
    if (i) {
        if (i->scriptIndex != -1)
            return Result(i->scriptIndex);
        else
            return Result((const void *)i);
    }

    for (int ii = 0; ii < m_anonymousImports.count(); ++ii) {
        if (QDeclarativeType *type = m_anonymousImports.at(ii).type(name))
            return Result(type);
    }

    return Result();
}

QDeclarativeTypeNameCache::Result QDeclarativeTypeNameCache::query(const QHashedStringRef &name, 
                                                                   const void *importNamespace)
{
    Q_ASSERT(importNamespace);
    Import *i = (Import *)importNamespace;
    Q_ASSERT(i->scriptIndex == -1);

    for (int ii = 0; ii < i->modules.count(); ++ii) {
        if (QDeclarativeType *type = i->modules.at(ii).type(name))
            return Result(type);
    }
    
    return Result();
}

QDeclarativeTypeNameCache::Result QDeclarativeTypeNameCache::query(const QHashedV8String &name)
{
    Import *i = m_namedImports.value(name);
    if (i) {
        if (i->scriptIndex != -1)
            return Result(i->scriptIndex);
        else
            return Result((const void *)i);
    }

    for (int ii = 0; ii < m_anonymousImports.count(); ++ii) {
        if (QDeclarativeType *type = m_anonymousImports.at(ii).type(name))
            return Result(type);
    }

    return Result();
}

QDeclarativeTypeNameCache::Result QDeclarativeTypeNameCache::query(const QHashedV8String &name, const void *importNamespace)
{
    Q_ASSERT(importNamespace);
    Import *i = (Import *)importNamespace;
    Q_ASSERT(i->scriptIndex == -1);

    for (int ii = 0; ii < i->modules.count(); ++ii) {
        if (QDeclarativeType *type = i->modules.at(ii).type(name))
            return Result(type);
    }
    
    return Result();
}

QDeclarativeMetaType::ModuleApiInstance *QDeclarativeTypeNameCache::moduleApi(const void *importNamespace)
{
    Q_ASSERT(importNamespace);
    Import *i = (Import *)importNamespace;
    Q_ASSERT(i->scriptIndex == -1);

    return i->moduleApi;
}

QT_END_NAMESPACE

