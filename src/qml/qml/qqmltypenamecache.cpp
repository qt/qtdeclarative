// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltypenamecache_p.h"

QT_BEGIN_NAMESPACE

void QQmlTypeNameCache::add(const QHashedString &name, const QUrl &url, const QHashedString &nameSpace)
{
    if (nameSpace.size() != 0) {
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

    if (nameSpace.size() != 0) {
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

