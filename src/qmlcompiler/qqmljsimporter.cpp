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
        const QString &filename, QHash<QString, QQmlJSExportedScope> *objects,
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

    const QString warningMessage = reader.warningMessage();
    if (!warningMessage.isEmpty())
        m_warnings.append({ warningMessage, QtWarningMsg, QQmlJS::SourceLocation() });

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

static QString internalName(const QQmlJSScope::ConstPtr &scope)
{
    if (const auto *factory = scope.factory())
        return factory->internalName();
    return scope->internalName();
}

static bool isComposite(const QQmlJSScope::ConstPtr &scope)
{
    // The only thing the factory can do is load a composite type.
    return scope.factory() || scope->isComposite();
}

QQmlJSImporter::Import QQmlJSImporter::readQmldir(const QString &path)
{
    Import result;
    auto reader = createQmldirParserForFile(path + SlashQmldir);
    result.imports.append(reader.imports());
    result.dependencies.append(reader.dependencies());

    QHash<QString, QQmlJSExportedScope> qmlComponents;
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

        auto mo = qmlComponents.find(it->fileName);
        if (mo == qmlComponents.end()) {
            QQmlJSScope::Ptr imported = localFile2ScopeTree(filePath);
            if (it->singleton) {
                if (auto *factory = imported.factory())
                    factory->setIsSingleton(true);
                else
                    imported->setIsSingleton(true);
            }
            mo = qmlComponents.insert(it->fileName, {imported, QList<QQmlJSScope::Export>() });
        }

        mo->exports.append(QQmlJSScope::Export(
                               reader.typeNamespace(), it.key(), it->version, QTypeRevision()));
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
        auto mo = result.scripts.find(script.fileName);
        if (mo == result.scripts.end())
            mo = result.scripts.insert(script.fileName, { localFile2ScopeTree(filePath), {} });

        mo->exports.append(QQmlJSScope::Export(
                               reader.typeNamespace(), script.nameSpace,
                               script.version, QTypeRevision()));
    }
    return result;
}

void QQmlJSImporter::importDependencies(const QQmlJSImporter::Import &import,
                                        QQmlJSImporter::AvailableTypes *types,
                                        const QString &prefix, QTypeRevision version,
                                        bool isDependency)
{
    // Import the dependencies with an invalid prefix. The prefix will never be matched by actual
    // QML code but the C++ types will be visible.
    for (auto const &dependency : qAsConst(import.dependencies))
        importHelper(dependency.module, types, QString(), dependency.version, true);

    for (auto const &import : qAsConst(import.imports)) {
        importHelper(import.module, types, isDependency ? QString() : prefix,
                     (import.flags & QQmlDirParser::Import::Auto) ? version : import.version,
                     isDependency);
    }
}

static bool isVersionAllowed(const QQmlJSScope::Export &exportEntry,
                             const QQmlJSScope::Import &importDescription)
{
    return !importDescription.version().isValid()
            || exportEntry.version() <= importDescription.version();
}

void QQmlJSImporter::processImport(const QQmlJSScope::Import &importDescription,
                                   const QQmlJSImporter::Import &import,
                                   QQmlJSImporter::AvailableTypes *types)
{
    const QString anonPrefix = QStringLiteral("$anonymous$");
    QHash<QString, QList<QQmlJSScope::Export>> seenExports;

    const auto insertExports = [&](const QQmlJSExportedScope &val, const QString &cppName) {
        QQmlJSScope::Export bestExport;

        // Resolve conflicting qmlNames within an import
        for (const auto &valExport : val.exports) {
            const QString qmlName = prefixedName(importDescription.prefix(), valExport.type());
            if (!isVersionAllowed(valExport, importDescription))
                continue;

            // Even if the QML name is overridden by some other type, we still want
            // to insert the C++ type, with the highest revision available.
            if (!bestExport.isValid() || valExport.version() > bestExport.version())
                bestExport = valExport;

            const auto it = types->qmlNames.find(qmlName);
            if (it != types->qmlNames.end()) {

                // The same set of exports can declare the same name multiple times for different
                // versions. That's the common thing and we would just continue here when we hit
                // it again after having inserted successfully once.
                // However, it can also declare *different* names. Then we need to do the whole
                // thing again.
                if (it->scope == val.scope && it->revision == valExport.version())
                    continue;

                const auto existingExports = seenExports.value(qmlName);
                enum { LowerVersion, SameVersion, HigherVersion } seenVersion = LowerVersion;
                for (const QQmlJSScope::Export &entry : existingExports) {
                    if (!isVersionAllowed(entry, importDescription))
                        continue;

                    if (valExport.version() < entry.version()) {
                        seenVersion = HigherVersion;
                        break;
                    }

                    if (seenVersion == LowerVersion && valExport.version() == entry.version())
                        seenVersion = SameVersion;
                }

                switch (seenVersion) {
                case LowerVersion:
                    break;
                case SameVersion: {
                    m_warnings.append({
                        QStringLiteral("Ambiguous type detected. "
                                       "%1 %2.%3 is defined multiple times.")
                            .arg(qmlName)
                            .arg(valExport.version().majorVersion())
                            .arg(valExport.version().minorVersion()),
                        QtCriticalMsg,
                        QQmlJS::SourceLocation()
                    });

                    // Invalidate the type. We don't know which one to use.
                    it->scope = QQmlJSScope::ConstPtr();
                    continue;
                }
                case HigherVersion:
                    continue;
                }
            }

            types->qmlNames.insert(qmlName, { val.scope, valExport.version() });
            seenExports[qmlName].append(valExport);
        }

        types->cppNames.insert(
            cppName, {
                val.scope,
                bestExport.isValid() ? bestExport.revision() : QTypeRevision::zero()
            }
        );
    };

    if (!importDescription.prefix().isEmpty())
        types->qmlNames.insert(importDescription.prefix(), {}); // Empty type means "this is the prefix"

    for (auto it = import.scripts.begin(); it != import.scripts.end(); ++it) {
        // You cannot have a script without an export
        Q_ASSERT(!it->exports.isEmpty());
        insertExports(*it, prefixedName(anonPrefix, internalName(it->scope)));
    }

    // add objects
    for (auto it = import.objects.begin(); it != import.objects.end(); ++it) {
        const auto &val = it.value();

        const QString cppName = isComposite(val.scope)
                ? prefixedName(anonPrefix, internalName(val.scope))
                : internalName(val.scope);

        if (val.exports.isEmpty()) {
            // Insert an unresolvable dummy name
            types->qmlNames.insert(
                        prefixedName(importDescription.prefix(), prefixedName(
                                         anonPrefix, internalName(val.scope))),
                        { val.scope, QTypeRevision() });
            types->cppNames.insert(cppName, { val.scope, QTypeRevision() });
        } else {
            insertExports(val, cppName);
        }
    }

    /* We need to create a temporary AvailableTypes instance here to make builtins available as
       QQmlJSScope::resolveTypes relies on them being available. They cannot be part of the regular
       types as they would keep overwriting existing types when loaded from cache.
       This is only a problem with builtin types as only builtin types can be overridden by any
       sibling import. Consider the following qmldir:

       module Things
       import QtQml 2.0
       import QtQuick.LocalStorage auto

       The module "Things" sees QtQml's definition of Qt, not the builtins', even though
       QtQuick.LocalStorage does not depend on QtQml and is imported afterwards. Conversely:

       module Stuff
       import ModuleOverridingQObject
       import QtQuick

       The module "Stuff" sees QtQml's definition of QObject (via QtQuick), even if
       ModuleOverridingQObject has overridden it.
    */

    QQmlJSImporter::AvailableTypes tempTypes(builtinImportHelper().cppNames);
    tempTypes.cppNames.insert(types->cppNames);

    // At present, there are corner cases that couldn't be resolved in a single
    // pass of resolveTypes() (e.g. QQmlEasingEnums::Type). However, such cases
    // only happen when enumerations are involved, thus the strategy is to
    // resolve enumerations (which can potentially create new child scopes)
    // before resolving the type fully
    const QQmlJSScope::ConstPtr intType = tempTypes.cppNames.value(u"int"_qs).scope;
    for (auto it = import.objects.begin(); it != import.objects.end(); ++it) {
        if (!it->scope.factory())
            QQmlJSScope::resolveEnums(it->scope, intType);
    }

    for (auto it = import.objects.begin(); it != import.objects.end(); ++it) {
        const auto &val = it.value();

        // Otherwise we have already done it in localFile2ScopeTree()
        if (!val.scope.factory() && val.scope->baseType().isNull()) {

            // Composite types use QML names, and we should have resolved those already.
            // ... except that old qmltypes files might specify composite types with C++ names.
            // Warn about those.
            if (val.scope->isComposite()) {
                m_warnings.append({
                    QStringLiteral("Found incomplete composite type %1. Do not use qmlplugindump.")
                                      .arg(val.scope->internalName()),
                    QtWarningMsg,
                    QQmlJS::SourceLocation()
                });
            }

            QQmlJSScope::resolveNonEnumTypes(val.scope, tempTypes.cppNames);
        }
    }
}

/*!
 * Imports builtins.qmltypes and jsroot.qmltypes found in any of the import paths.
 */
QQmlJSImporter::ImportedTypes QQmlJSImporter::importBuiltins()
{
    return builtinImportHelper().qmlNames;
}


QQmlJSImporter::AvailableTypes QQmlJSImporter::builtinImportHelper()
{
    if (!m_builtins.qmlNames.isEmpty() || !m_builtins.cppNames.isEmpty())
        return m_builtins;

    Import result;

    QStringList qmltypesFiles = { QStringLiteral("builtins.qmltypes"),
                                  QStringLiteral("jsroot.qmltypes") };
    const auto importBuiltins = [&](const QStringList &imports) {
        for (auto const &dir : imports) {
            QDirIterator it { dir, qmltypesFiles, QDir::NoFilter, QDirIterator::Subdirectories };
            while (it.hasNext() && !qmltypesFiles.isEmpty()) {
                readQmltypes(it.next(), &result.objects, &result.dependencies);
                qmltypesFiles.removeOne(it.fileName());
            }

            importDependencies(result, &m_builtins);

            if (qmltypesFiles.isEmpty())
                return;
        }
    };

    importBuiltins(m_importPaths);
    if (!qmltypesFiles.isEmpty()) {
        const QString pathsString =
                m_importPaths.isEmpty() ? u"<empty>"_qs : m_importPaths.join(u"\n\t");
        m_warnings.append({ QStringLiteral("Failed to find the following builtins: %1 (so will use "
                                           "qrc). Import paths used:\n\t%2")
                                    .arg(qmltypesFiles.join(u", "), pathsString),
                            QtWarningMsg, QQmlJS::SourceLocation() });
        importBuiltins({ u":/qt-project.org/qml/builtins"_qs }); // use qrc as a "last resort"
    }
    Q_ASSERT(qmltypesFiles.isEmpty()); // since qrc must cover it in all the bad cases

    // Process them together since there they have interdependencies that wouldn't get resolved
    // otherwise
    const QQmlJSScope::Import builtinImport(
                QString(), QStringLiteral("QML"), QTypeRevision::fromVersion(1, 0), false, true);
    processImport(builtinImport, result, &m_builtins);

    return m_builtins;
}

/*!
 * Imports types from the specified \a qmltypesFiles.
 */
void QQmlJSImporter::importQmldirs(const QStringList &qmldirFiles)
{
    AvailableTypes types(builtinImportHelper().cppNames);

    for (const auto &file : qmldirFiles) {
        Import result;
        QString qmldirName;
        if (file.endsWith(SlashQmldir)) {
            result = readQmldir(file.chopped(SlashQmldir.size()));
            qmldirName = file;
        } else {
            m_warnings.append({
                QStringLiteral("Argument %1 to -i option is not a qmldir file. Assuming qmltypes.")
                                  .arg(file),
                QtWarningMsg,
                QQmlJS::SourceLocation()
            });

            readQmltypes(file, &result.objects, &result.dependencies);

            // Append _FAKE_QMLDIR to our made up qmldir name so that if it ever gets used somewhere
            // else except for cache lookups, it will blow up due to a missing file instead of
            // producing weird results.
            qmldirName = file + QStringLiteral("_FAKE_QMLDIR");
        }

        m_seenQmldirFiles.insert(qmldirName, result);

        for (const auto &object : qAsConst(result.objects)) {
            for (const auto &ex : object.exports) {
                m_seenImports.insert({ex.package(), ex.version()}, qmldirName);
                // We also have to handle the case that no version is provided
                m_seenImports.insert({ex.package(), QTypeRevision()}, qmldirName);
            }
        }
    }
}

QQmlJSImporter::ImportedTypes QQmlJSImporter::importModule(
        const QString &module, const QString &prefix, QTypeRevision version)
{
    AvailableTypes result(builtinImportHelper().cppNames);
    if (!importHelper(module, &result, prefix, version)) {
        m_warnings.append({
                              QStringLiteral("Failed to import %1. Are your include paths set up properly?").arg(module),
                              QtWarningMsg,
                              QQmlJS::SourceLocation()
                          });
    }
    return result.qmlNames;
}

QQmlJSImporter::ImportedTypes QQmlJSImporter::builtinInternalNames()
{
    return builtinImportHelper().cppNames;
}

bool QQmlJSImporter::importHelper(const QString &module, AvailableTypes *types,
                                  const QString &prefix, QTypeRevision version, bool isDependency,
                                  bool isFile)
{
    // QtQuick/Controls and QtQuick.Controls are the same module
    const QString moduleCacheName = QString(module).replace(u'/', u'.');

    if (isDependency)
        Q_ASSERT(prefix.isEmpty());

    const QQmlJSScope::Import cacheKey(prefix, moduleCacheName, version, isFile, isDependency);

    auto getTypesFromCache = [&]() -> bool {
        if (!m_cachedImportTypes.contains(cacheKey))
            return false;

        const auto &cacheEntry = m_cachedImportTypes[cacheKey];

        types->cppNames.insert(cacheEntry->cppNames);

        // No need to import qml names for dependencies
        if (!isDependency)
            types->qmlNames.insert(cacheEntry->qmlNames);

        return true;
    };

    // The QML module only contains builtins and is not registered declaratively, so ignore requests
    // for importing it
    if (module == u"QML"_qs)
        return true;

    if (getTypesFromCache())
        return true;

    auto cacheTypes =
            QSharedPointer<QQmlJSImporter::AvailableTypes>(new QQmlJSImporter::AvailableTypes({}));
    m_cachedImportTypes[cacheKey] = cacheTypes;

    const QPair<QString, QTypeRevision> importId { module, version };
    const auto it = m_seenImports.constFind(importId);

    if (it != m_seenImports.constEnd()) {
        if (it->isEmpty())
            return false;

        Q_ASSERT(m_seenQmldirFiles.contains(*it));
        const QQmlJSImporter::Import import = m_seenQmldirFiles.value(*it);

        importDependencies(import, cacheTypes.get(), prefix, version, isDependency);
        processImport(cacheKey, import, cacheTypes.get());

        const bool typesFromCache = getTypesFromCache();
        Q_ASSERT(typesFromCache);
        return typesFromCache;
    }

    const auto modulePaths = isFile ? QStringList { module }
                                    : qQmlResolveImportPaths(module, m_importPaths, version);

    for (auto const &modulePath : modulePaths) {
        QString qmldirPath;
        if (modulePath.startsWith(u':')) {
            if (m_mapper) {
                const QString resourcePath = modulePath.mid(
                            1, modulePath.endsWith(u'/') ? modulePath.length() - 2 : -1)
                        + SlashQmldir;
                const auto entry = m_mapper->entry(
                            QQmlJSResourceFileMapper::resourceFileFilter(resourcePath));
                qmldirPath = entry.filePath;
            } else {
                qWarning() << "Cannot read files from resource directory" << modulePath
                           << "because no resource file mapper was provided";
            }
        } else {
            qmldirPath = modulePath + SlashQmldir;
        }

        const auto it = m_seenQmldirFiles.constFind(qmldirPath);
        if (it != m_seenQmldirFiles.constEnd()) {
            const QQmlJSImporter::Import import = *it;
            m_seenImports.insert(importId, qmldirPath);
            importDependencies(import, cacheTypes.get(), prefix, version, isDependency);
            processImport(cacheKey, import, cacheTypes.get());

            const bool typesFromCache = getTypesFromCache();
            Q_ASSERT(typesFromCache);
            return typesFromCache;
        }

        const QFileInfo file(qmldirPath);
        if (file.exists()) {
            const auto import = readQmldir(file.canonicalPath());
            m_seenQmldirFiles.insert(qmldirPath, import);
            m_seenImports.insert(importId, qmldirPath);
            importDependencies(import, cacheTypes.get(), prefix, version, isDependency);
            processImport(cacheKey, import, cacheTypes.get());

            const bool typesFromCache = getTypesFromCache();
            Q_ASSERT(typesFromCache);
            return typesFromCache;
        }
    }

    m_seenImports.insert(importId, QString());

    return false;
}

QQmlJSScope::Ptr QQmlJSImporter::localFile2ScopeTree(const QString &filePath)
{
    const auto seen = m_importedFiles.find(filePath);
    if (seen != m_importedFiles.end())
        return *seen;

    return *m_importedFiles.insert(filePath, {
                                       QQmlJSScope::create(),
                                       QSharedPointer<QDeferredFactory<QQmlJSScope>>(
                                            new QDeferredFactory<QQmlJSScope>(this, filePath))
                                   });
}

QQmlJSScope::Ptr QQmlJSImporter::importFile(const QString &file)
{
    return localFile2ScopeTree(file);
}

QQmlJSImporter::ImportedTypes QQmlJSImporter::importDirectory(
        const QString &directory, const QString &prefix)
{
    QQmlJSImporter::AvailableTypes types({});

    if (directory.startsWith(u':')) {
        if (m_mapper) {
            const auto resources = m_mapper->filter(
                        QQmlJSResourceFileMapper::resourceQmlDirectoryFilter(directory.mid(1)));
            for (const auto &entry : resources) {
                const QString name = QFileInfo(entry.resourcePath).baseName();
                if (name.front().isUpper()) {
                    types.qmlNames.insert(
                                prefixedName(prefix, name),
                                { localFile2ScopeTree(entry.filePath), QTypeRevision() });
                }
            }
        } else {
            qWarning() << "Cannot read files from resource directory" << directory
                       << "because no resource file mapper was provided";
        }

        importHelper(directory, &types, QString(), QTypeRevision(), false, true);

        return types.qmlNames;
    }

    QDirIterator it {
        directory,
        QStringList() << QLatin1String("*.qml"),
        QDir::NoFilter
    };
    while (it.hasNext()) {
        it.next();
        if (!it.fileName().front().isUpper())
            continue; // Non-uppercase names cannot be imported anyway.

        types.qmlNames.insert(prefixedName(prefix, QFileInfo(it.filePath()).baseName()),
                              { localFile2ScopeTree(it.filePath()), QTypeRevision() });
    }

    importHelper(directory, &types, QString(), QTypeRevision(), false, true);

    return types.qmlNames;
}

void QQmlJSImporter::setImportPaths(const QStringList &importPaths)
{
    m_importPaths = importPaths;

    // We have to get rid off all cache elements directly referencing modules, since changing
    // importPaths might change which module is found first
    m_seenImports.clear();
    m_cachedImportTypes.clear();
    // Luckily this doesn't apply to m_seenQmldirFiles
}

QQmlJSScope::ConstPtr QQmlJSImporter::jsGlobalObject() const
{
    return m_builtins.cppNames[u"GlobalObject"_qs].scope;
}

QT_END_NAMESPACE
