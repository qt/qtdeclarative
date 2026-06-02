// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLTCTYPERESOLVER_P_H
#define QQMLTCTYPERESOLVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmltcvisitor_p.h>

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qqmljsimporter_p.h>
#include <private/qqmljslogger_p.h>

QT_BEGIN_NAMESPACE

namespace QQmltc {

class QmltcTypeResolver : public QQmlJSTypeResolver
{
public:
    QmltcTypeResolver(QQmlJSImporter *importer) : QQmlJSTypeResolver(importer), m_importer(importer)
    {
        Q_ASSERT(importer);
    }

    void init(QmltcVisitor *visitor, QQmlJS::AST::Node *program);

    QQmlJSScope::Ptr scopeForLocation(const QV4::CompiledData::Location &location) const;

    // returns an import pair {url, modifiable type} for a given \a type
    std::pair<QString, QQmlJSScope::Ptr> importedType(const QQmlJSScope::ConstPtr &type) const;

private:
    QQmlJSImporter *m_importer = nullptr;

    QHash<QV4::CompiledData::Location, QQmlJSScope::Ptr> m_objectsByLocationNonConst;
    QQmlJSScope::Ptr m_root;
};

} // namespace QQmltc

QT_END_NAMESPACE

#endif // QQMLTCTYPERESOLVER_P_H
