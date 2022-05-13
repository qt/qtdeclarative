// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltctyperesolver.h"

#include <private/qqmljsimporter_p.h>
#include <private/qqmljsliteralbindingcheck_p.h>
#include <private/qv4value_p.h>

#include <QtCore/qqueue.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdiriterator.h>

Q_LOGGING_CATEGORY(lcTypeResolver2, "qml.qmltc.typeresolver", QtInfoMsg);

void QmltcTypeResolver::init(QmltcVisitor *visitor, QQmlJS::AST::Node *program)
{
    QQmlJSTypeResolver::init(visitor, program);

    QQmlJSLiteralBindingCheck literalCheck;
    literalCheck.run(visitor, this);

    m_root = visitor->result();

    QQueue<QQmlJSScope::Ptr> objects;
    objects.enqueue(m_root);
    while (!objects.isEmpty()) {
        const QQmlJSScope::Ptr object = objects.dequeue();
        const QQmlJS::SourceLocation location = object->sourceLocation();
        qCDebug(lcTypeResolver2()).nospace() << "inserting " << object.data() << " at "
                                             << location.startLine << ':' << location.startColumn;
        m_objectsByLocationNonConst.insert({ location.startLine, location.startColumn }, object);

        const auto childScopes = object->childScopes();
        for (const auto &childScope : childScopes)
            objects.enqueue(childScope);
    }
}

QQmlJSScope::Ptr
QmltcTypeResolver::scopeForLocation(const QV4::CompiledData::Location &location) const
{
    qCDebug(lcTypeResolver2()).nospace()
            << "looking for object at " << location.line() << ':' << location.column();
    return m_objectsByLocationNonConst.value(location);
}

QPair<QString, QQmlJSScope::Ptr>
QmltcTypeResolver::importedType(const QQmlJSScope::ConstPtr &type) const
{
    const auto files = m_importer->importedFiles();
    auto it = std::find_if(files.cbegin(), files.cend(), [&](const QQmlJSScope::Ptr &importedType) {
        return importedType.data() == type.data();
    });
    if (it == files.cend())
        return {};
    return { it.key(), it.value() };
}
