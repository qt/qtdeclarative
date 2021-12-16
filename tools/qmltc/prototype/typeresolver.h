/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TYPERESOLVER_H
#define TYPERESOLVER_H

#include "prototype/visitor.h"

#include <private/qqmljsscope_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmlirbuilder_p.h>
#include <private/qqmljstyperesolver_p.h>

namespace Qmltc {
class TypeResolver : public QQmlJSTypeResolver
{
public:
    TypeResolver(QQmlJSImporter *importer);

    // helper function for code generator
    QStringList gatherKnownCppClassNames() const
    {
        QStringList cppNames;
        QHash<QString, QQmlJSScope::ConstPtr> builtins = m_importer->builtinInternalNames();
        cppNames.reserve(builtins.size() + m_imports.size());
        const auto getInternalName = [](const QQmlJSScope::ConstPtr &t) {
            if (!t)
                return QString();
            return t->internalName();
        };
        std::transform(builtins.cbegin(), builtins.cend(), std::back_inserter(cppNames),
                       getInternalName);

        // builtins must be valid: all QQmlJSScopes are not nullptr and have
        // non-empty internal names. m_imports may have nullptrs, due to import
        // namespaces
        Q_ASSERT(std::find(cppNames.cbegin(), cppNames.cend(), QString()) == cppNames.cend());

        std::transform(m_imports.cbegin(), m_imports.cend(), std::back_inserter(cppNames),
                       getInternalName);
        return cppNames;
    }

    void init(Visitor &visitor, QQmlJS::AST::Node *program);

    // TODO: this shouldn't be exposed. instead, all the custom passes on
    // QQmlJSScope types must happen inside Visitor
    QQmlJSScope::Ptr root() const { return m_root; }

    QQmlJSScope::Ptr scopeForLocation(const QV4::CompiledData::Location &location) const;

    // returns an import pair {url, modifiable type} for a given \a type
    QPair<QString, QQmlJSScope::Ptr> importedType(const QQmlJSScope::ConstPtr &type) const;

private:
    QQmlJSImporter *m_importer = nullptr;

    QHash<QV4::CompiledData::Location, QQmlJSScope::Ptr> m_objectsByLocationNonConst;
    QQmlJSScope::Ptr m_root;
};
}

#endif // TYPERESOLVER_H
