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
#include <private/qv4value_p.h>

#include <QtCore/qqueue.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdiriterator.h>

Q_LOGGING_CATEGORY(lcTypeResolver2, "qml.compiler.typeresolver", QtInfoMsg);

static QString prefixedName(const QString &prefix, const QString &name)
{
    Q_ASSERT(!prefix.endsWith(u'.'));
    return prefix.isEmpty() ? name : (prefix + QLatin1Char('.') + name);
}

// TODO: this method is a (almost identical) copy-paste of QQmlJSImporter::importDirectory().
static void customImportDirectory(QHash<QString, std::pair<QString, QQmlJSScope::Ptr>> &qmlTypes,
                                  QQmlJSImporter *importer, const QString &directory,
                                  const QString &prefix = QString())
{
    if (directory.startsWith(u':')) {
        if (QQmlJSResourceFileMapper *mapper = importer->resourceFileMapper()) {
            const auto resources = mapper->filter(
                    QQmlJSResourceFileMapper::resourceQmlDirectoryFilter(directory.mid(1)));
            for (const auto &entry : resources) {
                const QString name = QFileInfo(entry.resourcePath).baseName();
                if (name.front().isUpper()) {
                    qmlTypes.insert(
                            prefixedName(prefix, name),
                            std::make_pair(entry.filePath, importer->importFile(entry.filePath)));
                }
            }
        }
        return;
    }

    QDirIterator it { directory, QStringList() << QLatin1String("*.qml"), QDir::NoFilter };
    while (it.hasNext()) {
        it.next();
        if (!it.fileName().front().isUpper())
            continue; // Non-uppercase names cannot be imported anyway.

        qmlTypes.insert(prefixedName(prefix, QFileInfo(it.filePath()).baseName()),
                        std::make_pair(it.filePath(), importer->importFile(it.filePath())));
    }
}

namespace Qmltc {

TypeResolver::TypeResolver(QQmlJSImporter *importer)
    : QQmlJSTypeResolver(importer), m_importer(importer)
{
    Q_ASSERT(m_importer);
}

void TypeResolver::init(Visitor &visitor, QQmlJS::AST::Node *program)
{
    QQmlJSTypeResolver::init(&visitor, program);
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

    m_implicitImportDir = visitor.getImplicitImportDirectory();
    m_importedDirs = visitor.getImportedDirectories();
    m_importedFiles = visitor.getImportedQmlFiles();
}

QQmlJSScope::Ptr TypeResolver::scopeForLocation(const QV4::CompiledData::Location &location) const
{
    qCDebug(lcTypeResolver2()).nospace()
            << "looking for object at " << location.line << ':' << location.column;
    return m_objectsByLocationNonConst.value(location);
}

QHash<QString, std::pair<QString, QQmlJSScope::Ptr>> TypeResolver::gatherCompiledQmlTypes() const
{
    QHash<QString, std::pair<QString, QQmlJSScope::Ptr>> qmlTypes;
    // implicit directory first
    customImportDirectory(qmlTypes, m_importer, m_implicitImportDir);
    // followed by subdirectories and absolute directories
    for (const QString &dir : m_importedDirs) {
        QString dirPath = dir;
        const QFileInfo dirInfo(dir);
        if (dirInfo.isRelative()) {
            dirPath = QDir(m_implicitImportDir).filePath(dirPath);
        }
        customImportDirectory(qmlTypes, m_importer, dirPath);
    }
    // followed by individual files
    for (const QString &file : m_importedFiles) {
        QString filePath = file;
        const QFileInfo fileInfo(file);
        if (fileInfo.isRelative()) {
            filePath = QDir(m_implicitImportDir).filePath(filePath);
        }
        // TODO: file importing is untested
        const QString baseName = QFileInfo(filePath).baseName();
        if (baseName.front().isUpper())
            qmlTypes.insert(baseName, std::make_pair(filePath, m_importer->importFile(filePath)));
    }
    return qmlTypes;
}

}
