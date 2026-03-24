// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljscontextualtypes_p.h"
#include "qqmljsutils_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {

void ContextualTypes::setType(const QString &name, const ContextualType &type)
{
    if (const QString fileSelector = QQmlJSUtils::fileSelectorFor(type.scope);
        !fileSelector.isEmpty()) {
        setFileSelectedType(fileSelector, name, type);
        return;
    }
    auto insertName = [this, &name, &type]() {
        if (!name.startsWith(u'$')) {
            if (!m_names.contains(type.scope, name))
                m_names.insert(type.scope, name);
        }
    };

    const auto it = m_types.find(name);
    if (it == m_types.end()) {
        m_types.insert(name, type);
        insertName();
        return;
    }

    if (it->m_precedence < type.m_precedence)
        return;
    // remove the old name from m_names
    m_names.remove(it->scope, name);
    *it = type;
    insertName();
}

void ContextualTypes::setFileSelectedType(const QString &fileSelector, const QString &name,
                                          const ContextualType &type)
{
    auto insertName = [this, &name, &type]() {
        if (!name.startsWith(u'$')) {
            if (!m_names.contains(type.scope, name))
                m_names.insert(type.scope, name);
        }
    };

    auto it = m_fileSelectedTypes.find(name);
    if (it == m_fileSelectedTypes.end()) {
        insertName();
        m_fileSelectedTypes.insert(name, { fileSelector, type });
        return;
    }

    if (it->type.m_precedence < type.m_precedence)
        return;

    m_names.remove(type.scope, name);
    *it = { fileSelector, type };
    insertName();
}

FileSelectorInfo ContextualTypes::fileSelectorInfoFor(const QQmlJSScope::ConstPtr &scope) const
{
    FileSelectorInfo result;
    for (auto [it, end] = m_names.equal_range(scope); it != end; ++it) {
        if (auto mainTypeIt = m_types.find(*it); mainTypeIt != m_types.end())
            result.mainType = mainTypeIt->scope;

        for (auto [it2, end2] = m_fileSelectedTypes.equal_range(*it); it2 != end2; ++it2) {
            result.fileSelectedTypes.append(*it2);
        }
    }
    return result;
}
} // namespace QQmlJS

QT_END_NAMESPACE
