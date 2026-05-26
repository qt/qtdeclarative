// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljscontextualtypes_p.h"
#include "qqmljsutils_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {

void ContextualTypes::setType(const QString &name, const ContextualType &type)
{
    const auto it = m_types.find(name);
    auto insertName = [this, &name, &type]() {
        if (!name.startsWith(u'$')) {
            if (!m_names.contains(type.scope, name))
                m_names.insert(type.scope, name);
        }
    };
    if (const QString fileSelector = QQmlJSUtils::fileSelectorFor(type.scope);
        !fileSelector.isEmpty()) {
        setFileSelectedType(fileSelector, name, type);
        // If a non-selected variant was already added, we're done;
        // file-selected variants don't replace a main entry. Otherwise we still
        // need to add the type, in case _all_ variants use file selectors
        // this assumes that at least one file selector will actually be active
        if (it == m_types.end()) {
            m_types.insert(name, type);
            insertName();
        }
        return;
    }

    if (it == m_types.end()) {
        m_types.insert(name, type);
        insertName();
        return;
    }

    if (it->m_precedence < type.m_precedence)
        return;
    // The old scope keeps its (scope, name) link in m_names only if it remains
    // referenced via m_fileSelectedTypes for the same name.
    const QQmlJSScope::ConstPtr oldScope = it->scope;
    *it = type;
    const bool isOldScopeStillRefencedAsFileSelected = [&]() {
        for (auto [it, end] = m_fileSelectedTypes.equal_range(name); it != end; ++it) {
            if (it->type.scope == oldScope)
                return true;
        }
        return false;
    }();
    if (!isOldScopeStillRefencedAsFileSelected)
        m_names.remove(oldScope, name);
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

    auto [it, end] = m_fileSelectedTypes.equal_range(name);
    auto match = std::find_if(it, end, [&fileSelector](const FileSelectedType &entry) {
        return entry.fileSelector == fileSelector;
    });
    if (match == end) {
        insertName();
        m_fileSelectedTypes.insert(name, { fileSelector, type });
        return;
    }

    if (match->type.m_precedence < type.m_precedence)
        return;

    m_names.remove(type.scope, name);
    *it = { fileSelector, type };

    insertName();
}

std::optional<ContextualType>
ContextualTypes::fileSelectedTypeFor(const QString &name, const QString &selector) const
{
    Q_ASSERT(!selector.isEmpty());
    for (auto [it, end] = m_fileSelectedTypes.equal_range(name); it != end; ++it) {
        if (it->fileSelector == selector)
            return it->type;
    }
    return std::nullopt;
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
