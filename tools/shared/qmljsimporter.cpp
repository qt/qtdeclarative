/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qmljsimporter.h"
#include "typedescriptionreader.h"
#include "qmljstypereader.h"

#include <QtQml/private/qqmlimportresolver_p.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qdiriterator.h>

static const QLatin1String SlashQmldir             = QLatin1String("/qmldir");
static const QLatin1String SlashPluginsDotQmltypes = QLatin1String("/plugins.qmltypes");

static const QString prefixedName(const QString &prefix, const QString &name)
{
    Q_ASSERT(!prefix.endsWith('.'));
    return prefix.isEmpty() ? name : (prefix  + QLatin1Char('.') + name);
}

static QQmlDirParser createQmldirParserForFile(const QString &filename)
{
    QFile f(filename);
    f.open(QFile::ReadOnly);
    QQmlDirParser parser;
    parser.parse(f.readAll());
    return parser;
}

void QmlJSImporter::readQmltypes(
        const QString &filename, QHash<QString, ScopeTree::Ptr> *objects)
{
    const QFileInfo fileInfo(filename);
    if (!fileInfo.exists()) {
        m_warnings.append(QLatin1String("QML types file does not exist: ") + filename);
        return;
    }

    if (fileInfo.isDir()) {
        m_warnings.append(QLatin1String("QML types file cannot be a directory: ") + filename);
        return;
    }

    QFile file(filename);
    file.open(QFile::ReadOnly);
    TypeDescriptionReader reader { filename, file.readAll() };
    QStringList dependencies;
    auto succ = reader(objects, &dependencies);
    if (!succ)
        m_warnings.append(reader.errorMessage());
}

QmlJSImporter::Import QmlJSImporter::readQmldir(const QString &path)
{
    Import result;
    auto reader = createQmldirParserForFile(path + SlashQmldir);
    result.imports.append(reader.imports());
    result.dependencies.append(reader.dependencies());

    QHash<QString, ScopeTree::Ptr> qmlComponents;
    const auto components = reader.components();
    for (auto it = components.begin(), end = components.end(); it != end; ++it) {
        const QString filePath = path + QLatin1Char('/') + it->fileName;
        if (!QFile::exists(filePath)) {
            m_warnings.append(it->fileName + QLatin1String(" is listed as component in ")
                              + path + SlashQmldir
                              + QLatin1String(" but does not exist.\n"));
            continue;
        }

        auto mo = qmlComponents.find(it.key());
        if (mo == qmlComponents.end())
            mo = qmlComponents.insert(it.key(), localFile2ScopeTree(filePath));

        (*mo)->addExport(
                    it.key(), reader.typeNamespace(),
                    ComponentVersion(it->version));
    }
    for (auto it = qmlComponents.begin(), end = qmlComponents.end(); it != end; ++it)
        result.objects.insert(it.key(), it.value());

    if (!reader.plugins().isEmpty() && QFile::exists(path + SlashPluginsDotQmltypes))
        readQmltypes(path + SlashPluginsDotQmltypes, &result.objects);

    const auto scripts = reader.scripts();
    for (const auto &script : scripts) {
        const QString filePath = path + QLatin1Char('/') + script.fileName;
        result.scripts.insert(script.nameSpace, localFile2ScopeTree(filePath));
    }
    return result;
}

void QmlJSImporter::importDependencies(
        const QmlJSImporter::Import &import,
        QmlJSImporter::ImportedTypes *types, const QString &prefix, QTypeRevision version)
{
    // Import the dependencies with an invalid prefix. The prefix will never be matched by actual
    // QML code but the C++ types will be visible.
    const QString invalidPrefix = QString::fromLatin1("$dependency$");
    for (auto const &dependency : qAsConst(import.dependencies))
        importHelper(dependency.module, types, invalidPrefix, dependency.version);

    for (auto const &import : qAsConst(import.imports)) {
        importHelper(import.module, types, prefix,
                     import.isAutoImport ? version : import.version);
    }
}

void QmlJSImporter::processImport(
        const QmlJSImporter::Import &import,
        QmlJSImporter::ImportedTypes *types,
        const QString &prefix)
{
    for (auto it = import.scripts.begin(); it != import.scripts.end(); ++it)
        types->qmlNames.insert(prefixedName(prefix, it.key()), it.value());

    // add objects
    for (auto it = import.objects.begin(); it != import.objects.end(); ++it) {
        const auto &val = it.value();
        types->cppNames.insert(val->internalName(), val);

        const auto exports = val->exports();
        for (const auto &valExport : exports)
            types->qmlNames.insert(prefixedName(prefix, valExport.type()), val);
    }

    for (auto it = import.objects.begin(); it != import.objects.end(); ++it) {
        const auto &val = it.value();
        if (!val->isComposite()) // Otherwise we have already done it in localFile2ScopeTree()
            val->resolveTypes(types->cppNames);
    }
}

/*!
 * Imports builtins.qmltypes found in any of the import paths.
 */
QmlJSImporter::ImportedTypes QmlJSImporter::importBuiltins()
{
    ImportedTypes types;

    for (auto const &dir : m_importPaths) {
        Import result;
        QDirIterator it { dir, QStringList() << QLatin1String("builtins.qmltypes"), QDir::NoFilter,
                          QDirIterator::Subdirectories };
        while (it.hasNext())
            readQmltypes(it.next(), &result.objects);
        importDependencies(result, &types);
        processImport(result, &types);
    }

    return types;
}

/*!
 * Imports types from the specified \a qmltypesFiles.
 */
QmlJSImporter::ImportedTypes QmlJSImporter::importQmltypes(const QStringList &qmltypesFiles)
{
    ImportedTypes types;
    Import result;

    for (const auto &qmltypeFile : qmltypesFiles)
        readQmltypes(qmltypeFile, &result.objects);

    importDependencies(result, &types);
    processImport(result, &types);

    return types;
}

QmlJSImporter::ImportedTypes QmlJSImporter::importModule(
        const QString &module, const QString &prefix, QTypeRevision version)
{
    ImportedTypes result;
    importHelper(module, &result, prefix, version);
    return result;
}

void QmlJSImporter::importHelper(const QString &module, ImportedTypes *types,
                                                const QString &prefix, QTypeRevision version)
{

    const QPair<QString, QTypeRevision> importId { module, version };
    const auto it = m_seenImports.find(importId);
    if (it != m_seenImports.end()) {
        importDependencies(*it, types, prefix, version);
        processImport(*it, types, prefix);
        return;
    }

    const auto qmltypesPaths = qQmlResolveImportPaths(module, m_importPaths, version);
    for (auto const &qmltypesPath : qmltypesPaths) {
        const QFileInfo file(qmltypesPath + SlashQmldir);
        if (file.exists()) {
            const auto import = readQmldir(file.canonicalPath());
            m_seenImports.insert(importId, import);
            importDependencies(import, types, prefix, version);
            processImport(import, types, prefix);
            return;
        }
    }

    m_seenImports.insert(importId, {});
}

ScopeTree::Ptr QmlJSImporter::localFile2ScopeTree(const QString &filePath)
{
    const auto seen = m_importedFiles.find(filePath);
    if (seen != m_importedFiles.end())
        return *seen;

    QmlJSTypeReader typeReader(filePath);
    ScopeTree::Ptr result = typeReader();
    m_importedFiles.insert(filePath, result);

    const QStringList errors = typeReader.errors();
    for (const QString &error : errors)
        m_warnings.append(error);

    ImportedTypes types;

    QDirIterator it {
        QFileInfo(filePath).canonicalPath(),
        QStringList() << QLatin1String("*.qml"),
        QDir::NoFilter
    };
    while (it.hasNext()) {
        ScopeTree::Ptr scope(localFile2ScopeTree(it.next()));
        if (!scope->internalName().isEmpty())
            types.qmlNames.insert(scope->internalName(), scope);
    }

    const auto imports = typeReader.imports();
    for (const auto &import : imports)
        importHelper(import.module, &types, import.prefix, import.version);

    result->resolveTypes(types.qmlNames);
    return result;
}

QmlJSImporter::ImportedTypes QmlJSImporter::importFileOrDirectory(
        const QString &fileOrDirectory, const QString &prefix)
{
    ImportedTypes result;

    QString name = fileOrDirectory;

    QFileInfo fileInfo(name);
    if (fileInfo.isFile()) {
        ScopeTree::Ptr scope(localFile2ScopeTree(fileInfo.canonicalFilePath()));
        result.qmlNames.insert(prefix.isEmpty() ? scope->internalName() : prefix, scope);
        return result;
    }

    QDirIterator it {
        fileInfo.canonicalFilePath(),
        QStringList() << QLatin1String("*.qml"),
        QDir::NoFilter
    };
    while (it.hasNext()) {
        ScopeTree::Ptr scope(localFile2ScopeTree(it.next()));
        if (!scope->internalName().isEmpty())
            result.qmlNames.insert(prefixedName(prefix, scope->internalName()), scope);
    }

    return result;
}
