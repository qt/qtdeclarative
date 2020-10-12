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

#include "qqmljsimporter_p.h"
#include "qqmljstypedescriptionreader_p.h"
#include "qqmljstypereader_p.h"

#include <QtQml/private/qqmlimportresolver_p.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qdiriterator.h>

QT_BEGIN_NAMESPACE

static const QLatin1String SlashQmldir             = QLatin1String("/qmldir");
static const QLatin1String SlashPluginsDotQmltypes = QLatin1String("/plugins.qmltypes");

static const QString prefixedName(const QString &prefix, const QString &name)
{
    Q_ASSERT(!prefix.endsWith(u'.'));
    return prefix.isEmpty() ? name : (prefix  + QLatin1Char('.') + name);
}

static QQmlDirParser createQmldirParserForFile(const QString &filename)
{
    QFile f(filename);
    f.open(QFile::ReadOnly);
    QQmlDirParser parser;
    parser.parse(QString::fromUtf8(f.readAll()));
    return parser;
}

void QQmlJSImporter::readQmltypes(
        const QString &filename, QHash<QString, QQmlJSScope::Ptr> *objects,
        QList<QQmlDirParser::Import> *dependencies)
{
    const QFileInfo fileInfo(filename);
    if (!fileInfo.exists()) {
        m_warnings.append({
                              QStringLiteral("QML types file does not exist: ") + filename,
                              QtWarningMsg,
                              QQmlJS::SourceLocation()
                          });
        return;
    }

    if (fileInfo.isDir()) {
        m_warnings.append({
                              QStringLiteral("QML types file cannot be a directory: ") + filename,
                              QtWarningMsg,
                              QQmlJS::SourceLocation()
                          });
        return;
    }

    QFile file(filename);
    file.open(QFile::ReadOnly);
    QQmlJSTypeDescriptionReader reader { filename, QString::fromUtf8(file.readAll()) };
    QStringList dependencyStrings;
    auto succ = reader(objects, &dependencyStrings);
    if (!succ)
        m_warnings.append({ reader.errorMessage(), QtCriticalMsg, QQmlJS::SourceLocation() });

    if (dependencyStrings.isEmpty())
        return;

    m_warnings.append({
                          QStringLiteral("Found deprecated dependency specifications in %1."
                                         "Specify dependencies in qmldir and use qmltyperegistrar "
                                         "to generate qmltypes files without dependencies.")
                                .arg(filename),
                          QtWarningMsg,
                          QQmlJS::SourceLocation()
                      });

    for (const QString &dependency : qAsConst(dependencyStrings)) {
        const auto blank = dependency.indexOf(u' ');
        if (blank < 0) {
            dependencies->append(QQmlDirParser::Import(dependency, {},
                                                       QQmlDirParser::Import::Default));
            continue;
        }

        const QString module = dependency.left(blank);
        const QString versionString = dependency.mid(blank + 1).trimmed();
        if (versionString == QStringLiteral("auto")) {
            dependencies->append(QQmlDirParser::Import(module, {}, QQmlDirParser::Import::Auto));
            continue;
        }

        const auto dot = versionString.indexOf(u'.');

        const QTypeRevision version = dot < 0
                ? QTypeRevision::fromMajorVersion(versionString.toUShort())
                : QTypeRevision::fromVersion(versionString.left(dot).toUShort(),
                                             versionString.mid(dot + 1).toUShort());

        dependencies->append(QQmlDirParser::Import(module, version,
                                                   QQmlDirParser::Import::Default));
    }
}

QQmlJSImporter::Import QQmlJSImporter::readQmldir(const QString &path)
{
    Import result;
    auto reader = createQmldirParserForFile(path + SlashQmldir);
    result.imports.append(reader.imports());
    result.dependencies.append(reader.dependencies());

    QHash<QString, QQmlJSScope::Ptr> qmlComponents;
    const auto components = reader.components();
    for (auto it = components.begin(), end = components.end(); it != end; ++it) {
        const QString filePath = path + QLatin1Char('/') + it->fileName;
        if (!QFile::exists(filePath)) {
            m_warnings.append({
                                  it->fileName + QStringLiteral(" is listed as component in ")
                                        + path + SlashQmldir
                                        + QStringLiteral(" but does not exist.\n"),
                                  QtWarningMsg,
                                  QQmlJS::SourceLocation()
                              });
            continue;
        }

        auto mo = qmlComponents.find(it.key());
        if (mo == qmlComponents.end())
            mo = qmlComponents.insert(it.key(), localFile2ScopeTree(filePath));

        (*mo)->addExport(it.key(), reader.typeNamespace(), it->version);
    }
    for (auto it = qmlComponents.begin(), end = qmlComponents.end(); it != end; ++it)
        result.objects.insert(it.key(), it.value());

    const auto typeInfos = reader.typeInfos();
    for (const auto &typeInfo : typeInfos) {
        const QString typeInfoPath = QFileInfo(typeInfo).isRelative()
                ? path + u'/' + typeInfo : typeInfo;
        readQmltypes(typeInfoPath, &result.objects, &result.dependencies);
    }

    if (typeInfos.isEmpty() && !reader.plugins().isEmpty()) {
        const QString defaultTypeInfoPath = path + SlashPluginsDotQmltypes;
        if (QFile::exists(defaultTypeInfoPath)) {
            m_warnings.append({
                                  QStringLiteral("typeinfo not declared in qmldir file: ")
                                    + defaultTypeInfoPath,
                                  QtWarningMsg,
                                  QQmlJS::SourceLocation()
                              });
            readQmltypes(defaultTypeInfoPath, &result.objects, &result.dependencies);
        }
    }

    const auto scripts = reader.scripts();
    for (const auto &script : scripts) {
        const QString filePath = path + QLatin1Char('/') + script.fileName;
        result.scripts.insert(script.nameSpace, localFile2ScopeTree(filePath));
    }
    return result;
}

void QQmlJSImporter::importDependencies(
        const QQmlJSImporter::Import &import,
        QQmlJSImporter::AvailableTypes *types, const QString &prefix, QTypeRevision version)
{
    // Import the dependencies with an invalid prefix. The prefix will never be matched by actual
    // QML code but the C++ types will be visible.
    const QString invalidPrefix = QString::fromLatin1("$dependency$");
    for (auto const &dependency : qAsConst(import.dependencies))
        importHelper(dependency.module, types, invalidPrefix, dependency.version);

    for (auto const &import : qAsConst(import.imports)) {
        importHelper(import.module, types, prefix,
                     (import.flags & QQmlDirParser::Import::Auto) ? version : import.version);
    }
}

void QQmlJSImporter::processImport(
        const QQmlJSImporter::Import &import,
        QQmlJSImporter::AvailableTypes *types,
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
        if (val->baseType().isNull()) // Otherwise we have already done it in localFile2ScopeTree()
            val->resolveTypes(types->cppNames);
    }
}

/*!
 * Imports builtins.qmltypes found in any of the import paths.
 */
QQmlJSImporter::ImportedTypes QQmlJSImporter::importBuiltins()
{
    AvailableTypes types;

    for (auto const &dir : m_importPaths) {
        Import result;
        QDirIterator it { dir, QStringList() << QLatin1String("builtins.qmltypes"), QDir::NoFilter,
                          QDirIterator::Subdirectories };
        while (it.hasNext())
            readQmltypes(it.next(), &result.objects, &result.dependencies);
        importDependencies(result, &types);
        processImport(result, &types);
    }

    return types.qmlNames;
}

/*!
 * Imports types from the specified \a qmltypesFiles.
 */
QQmlJSImporter::ImportedTypes QQmlJSImporter::importQmltypes(const QStringList &qmltypesFiles)
{
    AvailableTypes types;
    Import result;

    for (const auto &qmltypeFile : qmltypesFiles)
        readQmltypes(qmltypeFile, &result.objects, &result.dependencies);

    importDependencies(result, &types);
    processImport(result, &types);

    return types.qmlNames;
}

QQmlJSImporter::ImportedTypes QQmlJSImporter::importModule(
        const QString &module, const QString &prefix, QTypeRevision version)
{
    AvailableTypes result;
    importHelper(module, &result, prefix, version);
    return result.qmlNames;
}

void QQmlJSImporter::importHelper(const QString &module, AvailableTypes *types,
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

QQmlJSScope::Ptr QQmlJSImporter::localFile2ScopeTree(const QString &filePath)
{
    const auto seen = m_importedFiles.find(filePath);
    if (seen != m_importedFiles.end())
        return *seen;

    QQmlJSTypeReader typeReader(filePath);
    QQmlJSScope::Ptr result = typeReader();
    m_importedFiles.insert(filePath, result);

    m_warnings.append(typeReader.errors());

    AvailableTypes types;
    types.qmlNames.insert(importDirectory(QFileInfo(filePath).canonicalPath()));

    const auto imports = typeReader.imports();
    for (const auto &import : imports)
        importHelper(import.module, &types, import.prefix, import.version);

    result->resolveTypes(types.qmlNames);
    return result;
}

QQmlJSScope::Ptr QQmlJSImporter::importFile(const QString &file)
{
    return localFile2ScopeTree(file);
}

QQmlJSImporter::ImportedTypes QQmlJSImporter::importDirectory(
        const QString &directory, const QString &prefix)
{
    QHash<QString, QQmlJSScope::ConstPtr> qmlNames;

    QDirIterator it {
        directory,
        QStringList() << QLatin1String("*.qml"),
        QDir::NoFilter
    };
    while (it.hasNext()) {
        it.next();
        if (!it.fileName().front().isUpper())
            continue; // Non-uppercase names cannot be imported anyway.
        QQmlJSScope::Ptr scope(localFile2ScopeTree(it.filePath()));
        if (!scope->internalName().isEmpty())
            qmlNames.insert(prefixedName(prefix, scope->internalName()), scope);
    }

    return qmlNames;
}

QT_END_NAMESPACE
