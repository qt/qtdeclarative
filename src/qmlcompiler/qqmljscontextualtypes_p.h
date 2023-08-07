// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSCONTEXTUALTYPES_P_H
#define QQMLJSCONTEXTUALTYPES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/qstring.h>
#include <QtCore/qhash.h>
#include <private/qqmljsscope_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
/*! \internal
 *  Maps type names to types and the compile context of the types. The context can be
 *  INTERNAL (for c++ and synthetic jsrootgen types) or QML (for qml types).
 */
struct ContextualTypes
{
    enum CompileContext { INTERNAL, QML };

    ContextualTypes(
            CompileContext context,
            const QHash<QString, ImportedScope<QQmlJSScope::ConstPtr>> types,
            const QQmlJSScope::ConstPtr &arrayType)
        : m_types(types)
        , m_context(context)
        , m_arrayType(arrayType)
    {}

    CompileContext context() const { return m_context; }
    QQmlJSScope::ConstPtr arrayType() const { return m_arrayType; }

    bool hasType(const QString &name) const { return m_types.contains(name); }
    ImportedScope<QQmlJSScope::ConstPtr> type(const QString &name) const { return m_types[name]; }
    void setType(const QString &name, const ImportedScope<QQmlJSScope::ConstPtr> &type)
    {
        m_types.insert(name, type);
    }
    void clearType(const QString &name)
    {
        m_types[name].scope = QQmlJSScope::ConstPtr();
    }

    bool isNullType(const QString &name) const
    {
        const auto it = m_types.constFind(name);
        return it != m_types.constEnd() && it->scope.isNull();
    }

    void addTypes(ContextualTypes &&types)
    {
        Q_ASSERT(types.m_context == m_context);
        m_types.insert(std::move(types.m_types));
    }

    void addTypes(const ContextualTypes &types)
    {
        Q_ASSERT(types.m_context == m_context);
        m_types.insert(types.m_types);
    }

    const QHash<QString, ImportedScope<QQmlJSScope::ConstPtr>> &types() const { return m_types; }

    void clearTypes() { m_types.clear(); }

private:
    QHash<QString, ImportedScope<QQmlJSScope::ConstPtr>> m_types;
    CompileContext m_context;

    // For resolving sequence types
    QQmlJSScope::ConstPtr m_arrayType;
};
}

QT_END_NAMESPACE

#endif // QQMLJSCONTEXTUALTYPES_P_H
