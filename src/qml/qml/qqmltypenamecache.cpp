/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

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

QQmlTypeNameCache::Result QQmlTypeNameCache::query(const QHashedStringRef &name) const
{
    Result result = query(m_namedImports, name);

    if (!result.isValid())
        result = typeSearch(m_anonymousImports, name);

    if (!result.isValid())
        result = query(m_anonymousCompositeSingletons, name);

    if (!result.isValid()) {
        // Look up anonymous types from the imports of this document
        QQmlImportNamespace *typeNamespace = nullptr;
        QList<QQmlError> errors;
        QQmlType t;
        bool typeFound = m_imports.resolveType(name, &t, nullptr, nullptr, &typeNamespace, &errors);
        if (typeFound) {
            return Result(t);
        }

    }

    return result;
}

QQmlTypeNameCache::Result QQmlTypeNameCache::query(const QHashedStringRef &name,
                                                   const QQmlImportRef *importNamespace) const
{
    Q_ASSERT(importNamespace && importNamespace->scriptIndex == -1);

    Result result = typeSearch(importNamespace->modules, name);

    if (!result.isValid())
        result = query(importNamespace->compositeSingletons, name);

    if (!result.isValid()) {
        // Look up types from the imports of this document
        // ### it would be nice if QQmlImports allowed us to resolve a namespace
        // first, and then types on it.
        QString qualifiedTypeName = importNamespace->m_qualifier + QLatin1Char('.') + name.toString();
        QQmlImportNamespace *typeNamespace = nullptr;
        QList<QQmlError> errors;
        QQmlType t;
        bool typeFound = m_imports.resolveType(qualifiedTypeName, &t, nullptr, nullptr, &typeNamespace, &errors);
        if (typeFound) {
            return Result(t);
        }
    }

    return result;
}

QQmlTypeNameCache::Result QQmlTypeNameCache::query(const QV4::String *name, QQmlImport::RecursionRestriction recursionRestriction) const
{
    Result result = query(m_namedImports, name);

    if (!result.isValid())
        result = typeSearch(m_anonymousImports, name);

    if (!result.isValid())
        result = query(m_anonymousCompositeSingletons, name);

    if (!result.isValid()) {
        // Look up anonymous types from the imports of this document
        QString typeName = name->toQStringNoThrow();
        QQmlImportNamespace *typeNamespace = nullptr;
        QList<QQmlError> errors;
        QQmlType t;
        bool typeRecursionDetected = false;
        bool typeFound = m_imports.resolveType(typeName, &t, nullptr, nullptr, &typeNamespace, &errors,
                                               QQmlType::AnyRegistrationType,
                                               recursionRestriction == QQmlImport::AllowRecursion ? &typeRecursionDetected : nullptr);
        if (typeFound) {
            return Result(t);
        }

    }

    return result;
}

QQmlTypeNameCache::Result QQmlTypeNameCache::query(const QV4::String *name, const QQmlImportRef *importNamespace) const
{
    Q_ASSERT(importNamespace && importNamespace->scriptIndex == -1);

    QMap<const QQmlImportRef *, QStringHash<QQmlImportRef> >::const_iterator it = m_namespacedImports.constFind(importNamespace);
    if (it != m_namespacedImports.constEnd()) {
        Result r = query(*it, name);
        if (r.isValid())
            return r;
    }

    Result r = typeSearch(importNamespace->modules, name);

    if (!r.isValid())
        r = query(importNamespace->compositeSingletons, name);

    if (!r.isValid()) {
        // Look up types from the imports of this document
        // ### it would be nice if QQmlImports allowed us to resolve a namespace
        // first, and then types on it.
        QString qualifiedTypeName = importNamespace->m_qualifier + QLatin1Char('.') + name->toQStringNoThrow();
        QQmlImportNamespace *typeNamespace = nullptr;
        QList<QQmlError> errors;
        QQmlType t;
        bool typeFound = m_imports.resolveType(qualifiedTypeName, &t, nullptr, nullptr, &typeNamespace, &errors);
        if (typeFound) {
            return Result(t);
        }
    }

    return r;
}

QT_END_NAMESPACE

