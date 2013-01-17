/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmltypenamecache_p.h"

#include "qqmlengine_p.h"

QT_BEGIN_NAMESPACE

QQmlTypeNameCache::QQmlTypeNameCache()
{
}

QQmlTypeNameCache::~QQmlTypeNameCache()
{
}

void QQmlTypeNameCache::add(const QHashedString &name, int importedScriptIndex, const QHashedString &nameSpace)
{
    Import import;
    import.scriptIndex = importedScriptIndex;

    if (nameSpace.length() != 0) {
        Import *i = m_namedImports.value(nameSpace);
        Q_ASSERT(i != 0);
        m_namespacedImports[i].insert(name, import);
        return;
    }

    if (m_namedImports.contains(name))
        return;

    m_namedImports.insert(name, import);
}

QQmlTypeNameCache::Result QQmlTypeNameCache::query(const QHashedStringRef &name)
{
    Result result = query(m_namedImports, name);

    if (!result.isValid())
        result = typeSearch(m_anonymousImports, name);

    return result;
}

QQmlTypeNameCache::Result QQmlTypeNameCache::query(const QHashedStringRef &name, 
                                                                   const void *importNamespace)
{
    Q_ASSERT(importNamespace);
    const Import *i = static_cast<const Import *>(importNamespace);
    Q_ASSERT(i->scriptIndex == -1);

    return typeSearch(i->modules, name);
}

QQmlTypeNameCache::Result QQmlTypeNameCache::query(const QHashedV8String &name)
{
    Result result = query(m_namedImports, name);

    if (!result.isValid())
        result = typeSearch(m_anonymousImports, name);

    return result;
}

QQmlTypeNameCache::Result QQmlTypeNameCache::query(const QHashedV8String &name, const void *importNamespace)
{
    Q_ASSERT(importNamespace);
    const Import *i = static_cast<const Import *>(importNamespace);
    Q_ASSERT(i->scriptIndex == -1);

    QMap<const Import *, QStringHash<Import> >::const_iterator it = m_namespacedImports.find(i);
    if (it != m_namespacedImports.constEnd()) {
        Result r = query(*it, name);
        if (r.isValid())
            return r;
    }

    return typeSearch(i->modules, name);
}

QT_END_NAMESPACE

