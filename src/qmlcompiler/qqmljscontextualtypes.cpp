// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljscontextualtypes_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {

void ContextualTypes::setType(const QString &name, const ContextualType type)
{
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

} // namespace QQmlJS

QT_END_NAMESPACE
