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

#include "prototype/typeresolver.h"
#include "prototype/visitor.h"

#include <private/qqmljsimporter_p.h>
#include <private/qqmljsliteralbindingcheck_p.h>
#include <private/qv4value_p.h>

#include <QtCore/qqueue.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdiriterator.h>

Q_LOGGING_CATEGORY(lcTypeResolver2, "qml.compiler.typeresolver", QtInfoMsg);

namespace Qmltc {

TypeResolver::TypeResolver(QQmlJSImporter *importer)
    : QQmlJSTypeResolver(importer), m_importer(importer)
{
    Q_ASSERT(m_importer);
}

void TypeResolver::init(Visitor &visitor, QQmlJS::AST::Node *program)
{
    QQmlJSTypeResolver::init(&visitor, program);

    QQmlJSLiteralBindingCheck literalCheck;
    literalCheck.run(&visitor, this);

    m_root = visitor.result();

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

QQmlJSScope::Ptr TypeResolver::scopeForLocation(const QV4::CompiledData::Location &location) const
{
    qCDebug(lcTypeResolver2()).nospace()
            << "looking for object at " << location.line() << ':' << location.column();
    return m_objectsByLocationNonConst.value(location);
}

QPair<QString, QQmlJSScope::Ptr> TypeResolver::importedType(const QQmlJSScope::ConstPtr &type) const
{
    const auto files = m_importer->importedFiles();
    auto it = std::find_if(files.cbegin(), files.cend(), [&](const QQmlJSScope::Ptr &importedType) {
        return importedType.data() == type.data();
    });
    if (it == files.cend())
        return {};
    return { it.key(), it.value() };
}
}
