/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qqmltypenamecache_p.h"

#include "qqmlengine_p.h"

QT_BEGIN_NAMESPACE

QQmlTypeNameCache::QQmlTypeNameCache(const QQmlImports &importCache)
    : m_imports(importCache)
{
}

QQmlTypeNameCache::~QQmlTypeNameCache()
{
}

void QQmlTypeNameCache::add(const QHashedString &name, const QUrl &url, const QHashedString &nameSpace)
{
    if (nameSpace.length() != 0) {
        QQmlImportRef *i = m_namedImports.value(nameSpace);
        Q_ASSERT(i != nullptr);
        i->compositeSingletons.insert(name, url);
        return;
    }

    if (m_anonymousCompositeSingletons.contains(name))
        return;

    m_anonymousCompositeSingletons.insert(name, url);
}

void QQmlTypeNameCache::add(const QHashedString &name, int importedScriptIndex, const QHashedString &nameSpace)
{
    QQmlImportRef import;
    import.scriptIndex = importedScriptIndex;
    import.m_qualifier = name;

    if (nameSpace.length() != 0) {
        QQmlImportRef *i = m_namedImports.value(nameSpace);
        Q_ASSERT(i != nullptr);
        m_namespacedImports[i].insert(name, import);
        return;
    }

    if (m_namedImports.contains(name))
        return;

    m_namedImports.insert(name, import);
}

QT_END_NAMESPACE

