// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTCTYPERESOLVER_H
#define QMLTCTYPERESOLVER_H

#include "qmltcvisitor.h"

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qqmljsimporter_p.h>
#include <private/qqmljslogger_p.h>

QT_BEGIN_NAMESPACE

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
    QPair<QString, QQmlJSScope::Ptr> importedType(const QQmlJSScope::ConstPtr &type) const;

private:
    QQmlJSImporter *m_importer = nullptr;

    QHash<QV4::CompiledData::Location, QQmlJSScope::Ptr> m_objectsByLocationNonConst;
    QQmlJSScope::Ptr m_root;
};

QT_END_NAMESPACE

#endif // QMLTCTYPERESOLVER_H
