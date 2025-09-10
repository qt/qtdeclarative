// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsimporter_p.h"
#include "qqmljstypedescriptionreader_p.h"
#include "qqmljstypereader_p.h"
#include "qqmljsimportvisitor_p.h"
#include "qqmljslogger_p.h"
#include "qqmljsutils_p.h"

#include <QtQml/private/qqmlimportresolver_p.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qdiriterator.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static const QLatin1String SlashQmldir        = QLatin1String("/qmldir");
static const QLatin1String PluginsDotQmltypes = QLatin1String("plugins.qmltypes");
static const QLatin1String JsrootDotQmltypes  = QLatin1String("jsroot.qmltypes");


QQmlJS::Import::Import(QString prefix, QString name, QTypeRevision version, bool isFile,
                       bool isDependency) :
      m_prefix(std::move(prefix)),
      m_name(std::move(name)),
      m_version(version),
      m_isFile(isFile),
      m_isDependency(isDependency)
{
}

bool QQmlJS::Import::isValid() const
{
    return !m_name.isEmpty();
}

static const QString prefixedName(const QString &prefix, const QString &name)
{
    Q_ASSERT(!prefix.endsWith(u'.'));
    return prefix.isEmpty() ? name : (prefix  + QLatin1Char('.') + name);
}

QQmlDirParser QQmlJSImporter::createQmldirParserForFile(const QString &filename, Import *import)
{
    Q_ASSERT(import);
    QFile f(filename);
    QQmlDirParser parser;
    if (f.open(QFile::ReadOnly)) {
        parser.parse(QString::fromUtf8(f.readAll()));
        import->warnings.append(parser.errors(filename));
    } else {
        import->warnings.append({
            QStringLiteral("Could not open qmldir file: ") + filename,
            QtWarningMsg,
            QQmlJS::SourceLocation()
        });
    }

    return parser;
}

void QQmlJSImporter::readQmltypes(const QString &filename, Import *result)
{
    const QFileInfo fileInfo(filename);
    if (!fileInfo.exists()) {
        result->warnings.append({
                              QStringLiteral("QML types file does not exist: ") + filename,
                              QtWarningMsg,
                              QQmlJS::SourceLocation()
                          });
        return;
    }

    if (fileInfo.isDir()) {
        result->warnings.append({
                              QStringLiteral("QML types file cannot be a directory: ") + filename,
                              QtWarningMsg,
                              QQmlJS::SourceLocation()
                          });
        return;
    }

    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        result->warnings.append({
                              QStringLiteral("QML types file cannot be opened: ") + filename,
                              QtWarningMsg,
                              QQmlJS::SourceLocation()
                          });
        return;
    }

    QQmlJSTypeDescriptionReader reader { filename, QString::fromUtf8(file.readAll()) };
    QStringList dependencyStrings;
    auto succ = reader(&result->objects, &dependencyStrings);
    if (!succ)
        result->warnings.append({ reader.errorMessage(), QtCriticalMsg, QQmlJS::SourceLocation() });

    const QString warningMessage = reader.warningMessage();
    if (!warningMessage.isEmpty())
        result->warnings.append({ warningMessage, QtWarningMsg, QQmlJS::SourceLocation() });

    if (dependencyStrings.isEmpty())
        return;

    result->warnings.append({
                          QStringLiteral("Found deprecated dependency specifications in %1."
                                         "Specify dependencies in qmldir and use qmltyperegistrar "
                                         "to generate qmltypes files without dependencies.")
                                .arg(filename),
                          QtWarningMsg,
                          QQmlJS::SourceLocation()
                      });

    for (const QString &dependency : std::as_const(dependencyStrings)) {
        const auto blank = dependency.indexOf(u' ');
        if (blank < 0) {
            result->dependencies.append(
                    QQmlDirParser::Import(dependency, {}, QQmlDirParser::Import::Default));
            continue;
        }

        const QString module = dependency.left(blank);
        const QString versionString = dependency.mid(blank + 1).trimmed();
        if (versionString == QStringLiteral("auto")) {
            result->dependencies.append(
                    QQmlDirParser::Import(module, {}, QQmlDirParser::Import::Auto));
            continue;
        }

        const auto dot = versionString.indexOf(u'.');

        const QTypeRevision version = dot < 0
                ? QTypeRevision::fromMajorVersion(versionString.toUShort())
                : QTypeRevision::fromVersion(versionString.left(dot).toUShort(),
                                             versionString.mid(dot + 1).toUShort());

        result->dependencies.append(
                QQmlDirParser::Import(module, version, QQmlDirParser::Import::Default));
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

static QStringList aliases(const QQmlJSScope::ConstPtr &scope)
{
    return isComposite(scope)
        ? QStringList()
        : scope->aliases();
}

QQmlJSImporter::QQmlJSImporter(const QStringList &importPaths, QQmlJSResourceFileMapper *mapper,
                               QQmlJSImporterFlags flags)
    : m_importPaths(importPaths),
      m_mapper(mapper),
      m_flags(flags),
      m_importVisitor([](QQmlJS::AST::Node *rootNode, QQmlJSImporter *self,
                         const ImportVisitorPrerequisites &p) {
          auto visitor = std::unique_ptr<QQmlJS::AST::BaseVisitor>(new QQmlJSImportVisitor(
                  p.m_target, self, p.m_logger, p.m_implicitImportDirectory, p.m_qmldirFiles));
          QQmlJS::AST::Node::accept(rootNode, visitor.get());
      })
{
}

static QString resolvePreferredPath(
        const QString &qmldirPath, const QString &prefer, QQmlJSResourceFileMapper *mapper)
{
    if (prefer.isEmpty())
        return qmldirPath;

    if (!prefer.endsWith(u'/')) {
        qWarning() << "Ignoring invalid prefer path" << prefer << "(has to end with slash)";
        return qmldirPath;
    }

    if (prefer.startsWith(u':')) {
        // Resource path: Resolve via resource file mapper if possible.
        if (!mapper)
            return qmldirPath;

        Q_ASSERT(prefer.endsWith(u'/'));
        const auto entry = mapper->entry(
                QQmlJSResourceFileMapper::resourceFileFilter(prefer.mid(1) + SlashQmldir.mid(1)));

        // This can be empty if the .qrc files does not belong to this module.
        // In that case we trust the given qmldir file.
        return entry.filePath.endsWith(SlashQmldir)
                ? entry.filePath
                : qmldirPath;
    }

    // Host file system path. This should be rare. We don't generate it.
    const QFileInfo f(prefer + SlashQmldir);
    const QString canonical = f.canonicalFilePath();
    if (canonical.isEmpty()) {
        qWarning() << "No qmldir at" << prefer;
        return qmldirPath;
    }
    return canonical;
}

QQmlJSImporter::Import QQmlJSImporter::readQmldir(const QString &modulePath)
{
    Import result;
    const QString moduleQmldirPath = modulePath + SlashQmldir;
    auto reader = createQmldirParserForFile(moduleQmldirPath, &result);

    const QString resolvedQmldirPath
            = resolvePreferredPath(moduleQmldirPath, reader.preferredPath(), m_mapper);
    if (resolvedQmldirPath != moduleQmldirPath)
        reader = createQmldirParserForFile(resolvedQmldirPath, &result);

    // Leave the trailing slash
    Q_ASSERT(resolvedQmldirPath.endsWith(SlashQmldir));
    QStringView resolvedPath = QStringView(resolvedQmldirPath).chopped(SlashQmldir.size() - 1);

    result.name = reader.typeNamespace();

    result.isStaticModule = reader.isStaticModule();
    result.isSystemModule = reader.isSystemModule();
    result.imports.append(reader.imports());
    result.dependencies.append(reader.dependencies());

    if (result.isSystemModule) {
        // Import jsroot first, so that the builtins override it.
        const QString jsrootPath = resolvedPath + JsrootDotQmltypes;
        if (QFile::exists(jsrootPath)) {
            readQmltypes(jsrootPath, &result);
        } else {
            result.warnings.append({
                QStringLiteral("System module at %1 does not contain jsroot.qmltypes")
                                             .arg(resolvedPath),
                QtWarningMsg,
                QQmlJS::SourceLocation()
            });
        }
    }

    const auto typeInfos = reader.typeInfos();
    for (const auto &typeInfo : typeInfos) {
        const QString typeInfoPath = QFileInfo(typeInfo).isRelative()
                ? resolvedPath + typeInfo
                : typeInfo;
        readQmltypes(typeInfoPath, &result);
    }

    if (typeInfos.isEmpty() && !reader.plugins().isEmpty()) {
        const QString defaultTypeInfoPath = resolvedPath + PluginsDotQmltypes;
        if (QFile::exists(defaultTypeInfoPath)) {
            result.warnings.append({
                                  QStringLiteral("typeinfo not declared in qmldir file: ")
                                    + defaultTypeInfoPath,
                                  QtWarningMsg,
                                  QQmlJS::SourceLocation()
                              });
            readQmltypes(defaultTypeInfoPath, &result);
        }
    }

    QHash<QString, QQmlJSExportedScope> qmlComponents;
    const auto components = reader.components();
    for (auto it = components.begin(), end = components.end(); it != end; ++it) {
        const QString filePath = resolvedPath + it->fileName;
        if (!QFile::exists(filePath)) {
            result.warnings.append({
                                  it->fileName + QStringLiteral(" is listed as component in ")
                                        + resolvedQmldirPath
                                        + QStringLiteral(" but does not exist.\n"),
                                  QtWarningMsg,
                                  QQmlJS::SourceLocation()
                              });
            continue;
        }

        auto mo = qmlComponents.find(it->fileName);
        if (mo == qmlComponents.end()) {
            QQmlJSScope::Ptr imported = localFile2ScopeTree(filePath);
            if (auto *factory = imported.factory()) {
                if (it->singleton) {
                    factory->setIsSingleton(true);
                }
            }
            mo = qmlComponents.insert(it->fileName, {imported, QList<QQmlJSScope::Export>() });
        }

        mo->exports.append(QQmlJSScope::Export(
                               reader.typeNamespace(), it.key(), it->version, QTypeRevision()));
    }
    for (auto it = qmlComponents.begin(), end = qmlComponents.end(); it != end; ++it)
        result.objects.append(it.value());

    const auto scripts = reader.scripts();
    for (const auto &script : scripts) {
        const QString filePath = resolvedPath + script.fileName;
        auto mo = result.scripts.find(script.fileName);
        if (mo == result.scripts.end())
            mo = result.scripts.insert(script.fileName, { localFile2ScopeTree(filePath), {} });

        mo->exports.append(QQmlJSScope::Export(
                               reader.typeNamespace(), script.nameSpace,
                               script.version, QTypeRevision()));
    }
    return result;
}

QQmlJSImporter::Import QQmlJSImporter::readDirectory(const QString &directory)
{
    Import import;
    if (directory.startsWith(u':')) {
        if (m_mapper) {
            const auto resources = m_mapper->filter(
                        QQmlJSResourceFileMapper::resourceQmlDirectoryFilter(directory.mid(1)));
            for (const auto &entry : resources) {
                const QString name = QFileInfo(entry.resourcePath).baseName();
                if (name.front().isUpper()) {
                    import.objects.append({
                        localFile2ScopeTree(entry.filePath),
                        { QQmlJSScope::Export(QString(), name, QTypeRevision(), QTypeRevision()) }
                    });
                }
            }
        } else {
            qWarning() << "Cannot read files from resource directory" << directory
                       << "because no resource file mapper was provided";
        }

        return import;
    }

    QDirIterator it {
        directory,
        QStringList() << QLatin1String("*.qml"),
        QDir::NoFilter
    };
    while (it.hasNext()) {
        QString name = it.nextFileInfo().completeBaseName();

        // Non-uppercase names cannot be imported anyway.
        if (!name.front().isUpper())
            continue;

        // .ui.qml is fine
        if (name.endsWith(u".ui"))
            name = name.chopped(3);

        // Names with dots in them cannot be imported either.
        if (name.contains(u'.'))
            continue;

        import.objects.append({
                localFile2ScopeTree(it.filePath()),
                { QQmlJSScope::Export(QString(), name, QTypeRevision(), QTypeRevision()) }
        });
    }
    return import;
}

void QQmlJSImporter::importDependencies(
        const QQmlJSImporter::Import &import, QQmlJSImporter::AvailableTypes *types,
        const QString &prefix, QTypeRevision version, bool isDependency)
{
    // Import the dependencies with an invalid prefix. The prefix will never be matched by actual
    // QML code but the C++ types will be visible.
    for (auto const &dependency : std::as_const(import.dependencies))
        importHelper(dependency.module, types, QString(), dependency.version, true);

    bool hasOptionalImports = false;
    for (auto const &import : std::as_const(import.imports)) {
        if (import.flags & QQmlDirParser::Import::Optional) {
            hasOptionalImports = true;
            if (!useOptionalImports()) {
                continue;
            }

            if (!(import.flags & QQmlDirParser::Import::OptionalDefault))
                continue;
        }

        importHelper(import.module, types, isDependency ? QString() : prefix,
                     (import.flags & QQmlDirParser::Import::Auto) ? version : import.version,
                     isDependency);
    }

    if (hasOptionalImports && !useOptionalImports()) {
        types->warnings.append({
            u"%1 uses optional imports which are not supported. Some types might not be found."_s
                                          .arg(import.name),
            QtCriticalMsg, QQmlJS::SourceLocation()
        });
    }
}

static bool isVersionAllowed(const QQmlJSScope::Export &exportEntry,
                             const QQmlJS::Import &importDescription)
{
    const QTypeRevision importVersion = importDescription.version();
    const QTypeRevision exportVersion = exportEntry.version();
    if (!importVersion.hasMajorVersion())
        return true;
    if (importVersion.majorVersion() != exportVersion.majorVersion())
        return false;
    return !importVersion.hasMinorVersion()
            || exportVersion.minorVersion() <= importVersion.minorVersion();
}

void QQmlJSImporter::processImport(
        const QQmlJS::Import &importDescription, const QQmlJSImporter::Import &import,
        QQmlJSImporter::AvailableTypes *types)
{
    // In the list of QML types we prefix unresolvable QML names with $anonymous$, and C++
    // names with $internal$. This is to avoid clashes between them.
    // In the list of C++ types we insert types that don't have a C++ name as their
    // QML name prefixed with $anonymous$.
    const QString anonPrefix = QStringLiteral("$anonymous$");
    const QString internalPrefix = QStringLiteral("$internal$");
    const QString modulePrefix = QStringLiteral("$module$");
    QHash<QString, QList<QQmlJSScope::Export>> seenExports;

    const auto insertAliases = [&](const QQmlJSScope::ConstPtr &scope, QTypeRevision revision) {
        const QStringList cppAliases = aliases(scope);
        for (const QString &alias : cppAliases)
            types->cppNames.setType(alias, { scope, revision });
    };

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

            const auto it = types->qmlNames.types().find(qmlName);
            if (it != types->qmlNames.types().end()) {

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
                    types->warnings.append({
                        QStringLiteral("Ambiguous type detected. "
                                       "%1 %2.%3 is defined multiple times.")
                            .arg(qmlName)
                            .arg(valExport.version().majorVersion())
                            .arg(valExport.version().minorVersion()),
                        QtCriticalMsg,
                        QQmlJS::SourceLocation()
                    });

                    // Invalidate the type. We don't know which one to use.
                    types->qmlNames.clearType(qmlName);
                    continue;
                }
                case HigherVersion:
                    continue;
                }
            }

            types->qmlNames.setType(qmlName, { val.scope, valExport.version() });
            seenExports[qmlName].append(valExport);
        }

        const QTypeRevision bestRevision = bestExport.isValid()
                ? bestExport.revision()
                : QTypeRevision::zero();
        types->cppNames.setType(cppName, { val.scope, bestRevision });

        insertAliases(val.scope, bestRevision);

        const QTypeRevision bestVersion = bestExport.isValid()
                ? bestExport.version()
                : QTypeRevision::zero();
        types->qmlNames.setType(prefixedName(internalPrefix, cppName), { val.scope, bestVersion });
    };

    // Empty type means "this is the prefix"
    if (!importDescription.prefix().isEmpty())
        types->qmlNames.setType(importDescription.prefix(), {});

    if (!importDescription.isDependency()) {
        // Add a marker to show that this module has been imported
        types->qmlNames.setType(prefixedName(modulePrefix, importDescription.name()), {});

        if (import.isStaticModule)
            types->staticModules << import.name;

        if (import.isSystemModule)
            types->hasSystemModule = true;

        types->warnings.append(import.warnings);
    }

    for (auto it = import.scripts.begin(); it != import.scripts.end(); ++it) {
        // You cannot have a script without an export
        Q_ASSERT(!it->exports.isEmpty());
        insertExports(*it, prefixedName(anonPrefix, internalName(it->scope)));
    }

    // add objects
    for (const auto &val : import.objects) {
        const QString cppName = isComposite(val.scope)
                ? prefixedName(anonPrefix, internalName(val.scope))
                : internalName(val.scope);

        if (val.exports.isEmpty()) {
            // Insert an unresolvable dummy name
            types->qmlNames.setType(
                        prefixedName(internalPrefix, cppName), { val.scope, QTypeRevision() });
            types->cppNames.setType(cppName, { val.scope, QTypeRevision() });
            insertAliases(val.scope, QTypeRevision());
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
    tempTypes.cppNames.addTypes(types->cppNames);

    // At present, there are corner cases that couldn't be resolved in a single
    // pass of resolveTypes() (e.g. QQmlEasingEnums::Type). However, such cases
    // only happen when enumerations are involved, thus the strategy is to
    // resolve enumerations (which can potentially create new child scopes)
    // before resolving the type fully
    const QQmlJSScope::ConstPtr arrayType = tempTypes.cppNames.type(u"Array"_s).scope;
    for (auto it = import.objects.begin(); it != import.objects.end(); ++it) {
        if (!it->scope.factory()) {
            QQmlJSScope::resolveEnums(it->scope, tempTypes.cppNames);
            QQmlJSScope::resolveList(it->scope, arrayType);
        }
    }

    for (const auto &val : std::as_const(import.objects)) {
        // Otherwise we have already done it in localFile2ScopeTree()
        if (!val.scope.factory() && val.scope->baseType().isNull()) {

            // ignore the scope currently analyzed by QQmlJSImportVisitor, as its only populated
            // after importing the implicit directory.
            if (val.scope->baseTypeName() == "$InProcess$"_L1)
                continue;

            // Composite types use QML names, and we should have resolved those already.
            // ... except that old qmltypes files might specify composite types with C++ names.
            // Warn about those.
            if (val.scope->isComposite()) {
                types->warnings.append({
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
 * \internal
 * Imports builtins, but only the subset hardcoded into the parser.
 */
QQmlJSImporter::ImportedTypes QQmlJSImporter::importHardCodedBuiltins()
{
    const auto builtins = builtinImportHelper();

    QQmlJS::ContextualTypes result(
            QQmlJS::ContextualTypes::QML, {}, {}, builtins.cppNames.arrayType());
    for (const QString hardcoded : {
            "void"_L1, "int"_L1, "bool"_L1, "double"_L1, "real"_L1, "string"_L1, "url"_L1,
            "date"_L1, "regexp"_L1, "rect"_L1, "point"_L1, "size"_L1, "variant"_L1, "var"_L1,
         }) {

        const auto type = builtins.qmlNames.type(hardcoded);
        Q_ASSERT(type.scope);
        result.setType(hardcoded, type);
    }

    return ImportedTypes(std::move(result), {});
}


QQmlJSImporter::AvailableTypes QQmlJSImporter::builtinImportHelper()
{
    if (m_builtins)
        return *m_builtins;

    AvailableTypes builtins(QQmlJS::ContextualTypes(QQmlJS::ContextualTypes::INTERNAL, {}, {}, {}));

    importHelper(u"QML"_s, &builtins, QString(), QTypeRevision::fromVersion(1, 0));

    QQmlJSScope::ConstPtr arrayType = builtins.cppNames.type(u"Array"_s).scope;
    Q_ASSERT(arrayType);

    m_builtins = AvailableTypes(QQmlJS::ContextualTypes(
            QQmlJS::ContextualTypes::INTERNAL, builtins.cppNames.types(), builtins.cppNames.names(),
            arrayType));
    m_builtins->qmlNames = QQmlJS::ContextualTypes(
            QQmlJS::ContextualTypes::QML, builtins.qmlNames.types(), builtins.qmlNames.names(),
            arrayType);
    m_builtins->staticModules = std::move(builtins.staticModules);
    m_builtins->warnings = std::move(builtins.warnings);
    m_builtins->hasSystemModule = builtins.hasSystemModule;

    return *m_builtins;
}

/*!
 * Imports types from the specified \a qmltypesFiles.
 */
QList<QQmlJS::DiagnosticMessage> QQmlJSImporter::importQmldirs(const QStringList &qmldirFiles)
{
    QList<QQmlJS::DiagnosticMessage> warnings;
    for (const auto &file : qmldirFiles) {
        Import result;
        QString qmldirName;
        if (file.endsWith(SlashQmldir)) {
            result = readQmldir(file.chopped(SlashQmldir.size()));
            setQualifiedNamesOn(result);
            qmldirName = file;
        } else {
            warnings.append({
                QStringLiteral("Argument %1 to -i option is not a qmldir file. Assuming qmltypes.")
                                  .arg(file),
                QtWarningMsg,
                QQmlJS::SourceLocation()
            });

            readQmltypes(file, &result);

            // Append _FAKE_QMLDIR to our made up qmldir name so that if it ever gets used somewhere
            // else except for cache lookups, it will blow up due to a missing file instead of
            // producing weird results.
            qmldirName = file + QStringLiteral("_FAKE_QMLDIR");
        }

        warnings.append(result.warnings);
        m_seenQmldirFiles.insert(qmldirName, result);

        for (const auto &object : std::as_const(result.objects)) {
            for (const auto &ex : object.exports) {
                m_seenImports.insert({ex.package(), ex.version()}, qmldirName);
                // We also have to handle the case that no version is provided
                m_seenImports.insert({ex.package(), QTypeRevision()}, qmldirName);
            }
        }
    }

    return warnings;
}

QQmlJSImporter::ImportedTypes QQmlJSImporter::importModule(const QString &module,
                                                           const QString &prefix,
                                                           QTypeRevision version,
                                                           QStringList *staticModuleList)
{
    const AvailableTypes builtins = builtinImportHelper();
    AvailableTypes result(builtins.cppNames);
    if (!importHelper(module, &result, prefix, version)) {
        result.warnings.append({
            QStringLiteral("Failed to import %1. Are your import paths set up properly?")
                                         .arg(module),
            QtWarningMsg,
            QQmlJS::SourceLocation()
        });
    }

    if (staticModuleList)
        *staticModuleList << result.staticModules;

    return ImportedTypes(std::move(result.qmlNames), std::move(result.warnings));
}

QQmlJSImporter::ImportedTypes QQmlJSImporter::builtinInternalNames()
{
    auto builtins = builtinImportHelper();
    return ImportedTypes(std::move(builtins.cppNames), std::move(builtins.warnings));
}

bool QQmlJSImporter::importHelper(const QString &module, AvailableTypes *types,
                                  const QString &prefix, QTypeRevision version, bool isDependency,
                                  bool isFile)
{
    // QtQuick/Controls and QtQuick.Controls are the same module
    const QString moduleCacheName = QString(module).replace(u'/', u'.');

    if (isDependency)
        Q_ASSERT(prefix.isEmpty());

    const QQmlJS::Import cacheKey(prefix, moduleCacheName, version, isFile, isDependency);

    auto getTypesFromCache = [&]() -> bool {
        if (!m_cachedImportTypes.contains(cacheKey))
            return false;

        const auto &cacheEntry = m_cachedImportTypes[cacheKey];

        types->cppNames.addTypes(cacheEntry->cppNames);
        types->staticModules << cacheEntry->staticModules;
        types->hasSystemModule |= cacheEntry->hasSystemModule;

        // No need to import qml names for dependencies
        if (!isDependency) {
            types->warnings.append(cacheEntry->warnings);
            types->qmlNames.addTypes(cacheEntry->qmlNames);
        }

        return true;
    };

    if (getTypesFromCache())
        return true;

    auto cacheTypes = QSharedPointer<QQmlJSImporter::AvailableTypes>(
            new QQmlJSImporter::AvailableTypes(QQmlJS::ContextualTypes(
                    QQmlJS::ContextualTypes::INTERNAL, {}, {}, types->cppNames.arrayType())));
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

    QStringList modulePaths;
    if (isFile) {
        const auto import = readDirectory(module);
        m_seenQmldirFiles.insert(module, import);
        m_seenImports.insert(importId, module);
        importDependencies(import, cacheTypes.get(), prefix, version, isDependency);
        processImport(cacheKey, import, cacheTypes.get());

        // Try to load a qmldir below, on top of the directory import.
        modulePaths.append(module);
    } else if (module == "QML"_L1) {
        // Always load builtins from the resource file system. We don't want any nasty surprises
        // that could surface when loading them from an actual import path. We rely on the builtins
        // to contain a number of types later on. It's better to always use our own copy.
        modulePaths = { ":/qt-project.org/imports/QML"_L1 };
    } else {
        modulePaths = qQmlResolveImportPaths(module, m_importPaths, version);
    }

    for (auto const &modulePath : modulePaths) {
        QString qmldirPath = modulePath + SlashQmldir;
        if (modulePath.startsWith(u':')) {
            if (module == "QML"_L1) {
                // Do not try to map the builtins' resource path.
            } else if (m_mapper) {
                const QString resourcePath = modulePath.mid(
                            1, modulePath.endsWith(u'/') ? modulePath.size() - 2 : -1)
                        + SlashQmldir;
                const auto entry = m_mapper->entry(
                            QQmlJSResourceFileMapper::resourceFileFilter(resourcePath));
                qmldirPath = entry.filePath;
            } else {
                // The builtins are loaded from the resource file system. Don't warn about them.
                qWarning() << "Cannot read files from resource directory" << modulePath
                           << "because no resource file mapper was provided";
            }
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
            setQualifiedNamesOn(import);
            m_seenQmldirFiles.insert(qmldirPath, import);
            m_seenImports.insert(importId, qmldirPath);
            importDependencies(import, cacheTypes.get(), prefix, version, isDependency);

            // Potentially merges with the result of readDirectory() above.
            processImport(cacheKey, import, cacheTypes.get());

            const bool typesFromCache = getTypesFromCache();
            Q_ASSERT(typesFromCache);
            return typesFromCache;
        }
    }

    if (isFile) {
        // We've loaded the directory above
        const bool typesFromCache = getTypesFromCache();
        Q_ASSERT(typesFromCache);
        return typesFromCache;
    }

    m_seenImports.insert(importId, QString());
    return false;
}

QQmlJSScope::Ptr QQmlJSImporter::localFile2ScopeTree(const QString &filePath)
{
    const QString sourceFolderFile = preferQmlFilesFromSourceFolder()
            ? QQmlJSUtils::qmlSourcePathFromBuildPath(m_mapper, filePath)
            : filePath;

    const auto seen = m_importedFiles.find(sourceFolderFile);
    if (seen != m_importedFiles.end())
        return *seen;

    return *m_importedFiles.insert(
            sourceFolderFile,
            { QQmlJSScope::create(),
              QSharedPointer<QDeferredFactory<QQmlJSScope>>(new QDeferredFactory<QQmlJSScope>(
                      this, sourceFolderFile)) });
}

/*!
\internal
Add scopes manually created and QQmlJSImportVisited to QQmlJSImporter.
This allows theses scopes to not get loaded twice during linting, for example.

Returns false if the importer contains a scope different than \a scope for the same
QQmlJSScope::filePath.
*/
bool QQmlJSImporter::registerScope(const QQmlJSScope::Ptr &scope)
{
    Q_ASSERT(!scope.factory());

    QQmlJSScope::Ptr &existing = m_importedFiles[scope->filePath()];
    if (existing)
        return existing == scope;
    existing = scope;
    return true;
}

QQmlJSScope::Ptr QQmlJSImporter::importFile(const QString &file)
{
    return localFile2ScopeTree(file);
}

QQmlJSImporter::ImportedTypes QQmlJSImporter::importDirectory(
        const QString &directory, const QString &prefix)
{
    const AvailableTypes builtins = builtinImportHelper();
    QQmlJSImporter::AvailableTypes types(QQmlJS::ContextualTypes(
            QQmlJS::ContextualTypes::INTERNAL, {}, {}, builtins.cppNames.arrayType()));
    importHelper(directory, &types, prefix, QTypeRevision(), false, true);
    return ImportedTypes(std::move(types.qmlNames), std::move(types.warnings));
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

void QQmlJSImporter::clearCache()
{
    m_seenImports.clear();
    m_cachedImportTypes.clear();
    m_seenQmldirFiles.clear();
    m_importedFiles.clear();
    m_builtins.reset();
}

QQmlJSScope::ConstPtr QQmlJSImporter::jsGlobalObject()
{
    return builtinImportHelper().cppNames.type(u"GlobalObject"_s).scope;
}

void QQmlJSImporter::setQualifiedNamesOn(const Import &import)
{
    for (auto &object : import.objects) {
        if (object.exports.isEmpty())
            continue;
        if (auto *factory = object.scope.factory()) {
            factory->setModuleName(import.name);
        } else {
            object.scope->setOwnModuleName(import.name);
        }
    }
}

void QQmlJSImporter::runImportVisitor(QQmlJS::AST::Node *rootNode,
                                      const ImportVisitorPrerequisites &p)
{
    m_importVisitor(rootNode, this, p);
}

QT_END_NAMESPACE
