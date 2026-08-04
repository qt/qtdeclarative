// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljslinter_p.h"
#include "qqmljslintercodegen_p.h"

#include <private/qqmljsimporter_p.h>
#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljslinterpasses_p.h>
#include <private/qqmljslintervisitor_p.h>
#include <private/qqmljsliteralbindingcheck_p.h>
#include <private/qqmljsloggingutils_p.h>
#include <private/qqmljsutils_p.h>
#include <private/qqmlsa_p.h>

#include <QtCore/qjsonobject.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/qscopedpointer.h>


#if QT_CONFIG(library)
#    include <QtCore/qdiriterator.h>
#    include <QtCore/qlibrary.h>
#endif

#if QT_CONFIG(qmlcontextpropertydump)
#  include <QtCore/qsettings.h>
#endif

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsdiagnosticmessage_p.h>


QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class HasFunctionDefinitionVisitor final : public QQmlJS::AST::Visitor
{
public:
    bool visit(QQmlJS::AST::FunctionDeclaration *functionDeclaration) override
    {
        m_result = !functionDeclaration->name.isEmpty();
        return false;
    }

    void throwRecursionDepthError() override { }
    bool result() const { return m_result; }
    void reset() { m_result = false; }

private:
    bool m_result = false;
};

class UnreachableVisitor final : public QQmlJS::AST::Visitor
{
public:
    UnreachableVisitor(QQmlJSLogger *logger) : m_logger(logger) { }

    bool containsFunctionDeclaration(QQmlJS::AST::Node *node)
    {
        m_hasFunctionDefinition.reset();
        node->accept(&m_hasFunctionDefinition);
        return m_hasFunctionDefinition.result();
    }

    bool visit(QQmlJS::AST::StatementList *unreachable) override
    {
        QQmlJS::SourceLocation location;
        auto report = [this, &location]() {
            if (location.isValid()) {
                m_logger->log(u"Unreachable code"_s, qmlUnreachableCode, location);
            }
            location = QQmlJS::SourceLocation{};
        };

        for (auto it = unreachable; it && it->statement; it = it->next) {
            if (containsFunctionDeclaration(it->statement)) {
                report();
                continue; // don't warn about the location of the function declaration
            }
            location = combine(location,
                               combine(it->statement->firstSourceLocation(),
                                       it->statement->lastSourceLocation()));
        }
        report();
        return false;
    }
    void throwRecursionDepthError() override { }

private:
    QQmlJSLogger *m_logger = nullptr;
    HasFunctionDefinitionVisitor m_hasFunctionDefinition;
};

class CodegenWarningInterface final : public QV4::Compiler::CodegenWarningInterface
{
public:
    CodegenWarningInterface(QQmlJSLogger *logger) : m_logger(logger), m_unreachableVisitor(logger)
    {
    }

    void reportVarUsedBeforeDeclaration(const QString &name, const QString &fileName,
                                        QQmlJS::SourceLocation declarationLocation,
                                        QQmlJS::SourceLocation accessLocation) override
    {
        Q_UNUSED(fileName)

        m_logger->log("Identifier '%1' is used here before its declaration."_L1.arg(name),
                      qmlVarUsedBeforeDeclaration, accessLocation);
        m_logger->log("Note: declaration of '%1' here"_L1.arg(name), qmlVarUsedBeforeDeclaration,
                      declarationLocation, true, true, {}, accessLocation.startLine);
    }

    void reportFunctionUsedBeforeDeclaration(const QString &name, const QString &fileName,
                                             QQmlJS::SourceLocation declarationLocation,
                                             QQmlJS::SourceLocation accessLocation) override
    {
        Q_UNUSED(fileName)

        m_logger->log("Function '%1' is used here before its declaration."_L1.arg(name),
                      qmlFunctionUsedBeforeDeclaration, accessLocation);
        m_logger->log("Note: declaration of '%1' here"_L1.arg(name),
                      qmlFunctionUsedBeforeDeclaration, declarationLocation);
    }

    void reportParserWarnings(const QString &, QQmlJS::SourceLocation, const QString &) override
    {
        // Note: we already reported parser warnings in QQmlJSLinter::typeReader(), so we don't
        // need to report them again here.
    }

    UnreachableVisitor *unreachableVisitor() override { return &m_unreachableVisitor; }

private:
    QQmlJSLogger *m_logger;
    UnreachableVisitor m_unreachableVisitor;
};

QQmlJSLinter::QQmlJSLinter(const QStringList &importPaths, const QStringList &extraPluginPaths,
                           bool useAbsolutePath)
    : m_useAbsolutePath(useAbsolutePath),
      m_enablePlugins(true),
      m_importer(importPaths, nullptr,
                 QQmlJSImporterFlags(UseOptionalImports | TolerateFileSelectors
                                     | PreferQmlFilesFromSourceFolder))
{
    m_plugins = loadPlugins(extraPluginPaths);
}

QQmlJSLinter::Plugin::Plugin(QQmlJSLinter::Plugin &&plugin) noexcept
    : m_name(std::move(plugin.m_name))
    , m_description(std::move(plugin.m_description))
    , m_version(std::move(plugin.m_version))
    , m_author(std::move(plugin.m_author))
    , m_categories(std::move(plugin.m_categories))
    , m_instance(std::move(plugin.m_instance))
    , m_loader(std::move(plugin.m_loader))
    , m_isInternal(std::move(plugin.m_isInternal))
    , m_isValid(std::move(plugin.m_isValid))
{
    // Mark the old Plugin as invalid and make sure it doesn't delete the loader
    Q_ASSERT(!plugin.m_loader);
    plugin.m_instance = nullptr;
    plugin.m_isValid = false;
}

#if QT_CONFIG(library)
QQmlJSLinter::Plugin::Plugin(QString path)
{
    m_loader = std::make_unique<QPluginLoader>(path);
    if (!parseMetaData(m_loader->metaData(), path))
        return;

    QObject *object = m_loader->instance();
    if (!object)
        return;

    m_instance = qobject_cast<QQmlSA::LintPlugin *>(object);
    if (!m_instance)
        return;

    m_isValid = true;
}
#endif

QQmlJSLinter::Plugin::Plugin(const QStaticPlugin &staticPlugin)
{
    if (!parseMetaData(staticPlugin.metaData(), u"built-in"_s))
        return;

    m_instance = qobject_cast<QQmlSA::LintPlugin *>(staticPlugin.instance());
    if (!m_instance)
        return;

    m_isValid = true;
}

QQmlJSLinter::Plugin::~Plugin()
{
#if QT_CONFIG(library)
    if (m_loader != nullptr) {
        m_loader->unload();
        m_loader->deleteLater();
    }
#endif
}

bool QQmlJSLinter::Plugin::parseMetaData(const QJsonObject &metaData, QString pluginName)
{
    const QString pluginIID = QStringLiteral(QmlLintPluginInterface_iid);

    if (metaData[u"IID"].toString() != pluginIID)
        return false;

    QJsonObject pluginMetaData = metaData[u"MetaData"].toObject();

    for (const QString &requiredKey :
         { u"name"_s, u"version"_s, u"author"_s, u"loggingCategories"_s }) {
        if (!pluginMetaData.contains(requiredKey)) {
            qWarning() << pluginName << "is missing the required " << requiredKey
                       << "metadata, skipping";
            return false;
        }
    }

    m_name = pluginMetaData[u"name"].toString();
    m_author = pluginMetaData[u"author"].toString();
    m_version = pluginMetaData[u"version"].toString();
    m_description = pluginMetaData[u"description"].toString(u"-/-"_s);
    m_isInternal = pluginMetaData[u"isInternal"].toBool(false);

    if (!pluginMetaData[u"loggingCategories"].isArray()) {
        qWarning() << pluginName << "has loggingCategories which are not an array, skipping";
        return false;
    }

    const QJsonArray categories = pluginMetaData[u"loggingCategories"].toArray();
    for (const QJsonValue &value : categories) {
        if (!value.isObject()) {
            qWarning() << pluginName << "has invalid loggingCategories entries, skipping";
            return false;
        }

        const QJsonObject object = value.toObject();

        for (const QString &requiredKey : { u"name"_s, u"description"_s }) {
            if (!object.contains(requiredKey)) {
                qWarning() << pluginName << " logging category is missing the required "
                           << requiredKey << "metadata, skipping";
                return false;
            }
        }

        const QString prefix = (m_isInternal ? u""_s : u"Plugin."_s).append(m_name).append(u'.');
        const QString categoryId =
                prefix + object[u"name"].toString();
        const auto settingsNameIt = object.constFind(u"settingsName");
        const QString settingsName = (settingsNameIt == object.constEnd())
                ? categoryId
                : prefix + settingsNameIt->toString(categoryId);
        m_categories << QQmlJS::LoggerCategory{ categoryId, settingsName,
                                                object["description"_L1].toString(),
                                                QQmlJS::WarningSeverity::Warning };
        const auto itSeverity = object.find("defaultSeverity"_L1);
        if (itSeverity == object.end())
            continue;

        const QString severityName = itSeverity->toString();
        const auto severity = QQmlJS::LoggingUtils::severityFromString(severityName);
        if (!severity.has_value()) {
            qWarning() << "Invalid logging severity" << severityName << "provided for"
                       << m_categories.last().id().name().toString()
                       << "(allowed are: disable, info, warning, error) found in plugin metadata.";
            continue;
        }

        m_categories.last().setSeverity(severity.value());
    }

    return true;
}

std::vector<QQmlJSLinter::Plugin> QQmlJSLinter::loadPlugins(QStringList extraPluginPaths)
{
    std::vector<Plugin> plugins;

    QDuplicateTracker<QString> seenPlugins;

    const auto &staticPlugins = QPluginLoader::staticPlugins();
    for (const QStaticPlugin &staticPlugin : staticPlugins) {
        Plugin plugin(staticPlugin);
        if (!plugin.isValid())
            continue;

        if (seenPlugins.hasSeen(plugin.name().toLower())) {
            qWarning() << "Two plugins named" << plugin.name()
                       << "present, make sure no plugins are duplicated. The second plugin will "
                          "not be loaded.";
            continue;
        }

        plugins.push_back(std::move(plugin));
    }

#if QT_CONFIG(library)
    const QStringList paths = [&extraPluginPaths]() {
        QStringList result{ extraPluginPaths };
        const QStringList libraryPaths = QCoreApplication::libraryPaths();
        for (const auto &path : libraryPaths) {
            result.append(path + u"/qmllint"_s);
        }
        return result;
    }();
    for (const QString &pluginDir : paths) {
        QDirIterator it{ pluginDir, QDir::Files };

        while (it.hasNext()) {
            auto potentialPlugin = it.next();

            if (!QLibrary::isLibrary(potentialPlugin))
                continue;

            Plugin plugin(potentialPlugin);

            if (!plugin.isValid())
                continue;

            if (seenPlugins.hasSeen(plugin.name().toLower())) {
                qWarning() << "Two plugins named" << plugin.name()
                           << "present, make sure no plugins are duplicated. The second plugin "
                              "will not be loaded.";
                continue;
            }

            plugins.push_back(std::move(plugin));
        }
    }
#endif
    Q_UNUSED(extraPluginPaths)
    return plugins;
}

void QQmlJSLinter::parseComments(QQmlJSLogger *logger,
                                 const QList<QQmlJS::SourceLocation> &comments)
{
    QHash<int, QSet<QString>> disablesPerLine;
    QHash<int, QSet<QString>> enablesPerLine;
    QHash<int, QSet<QString>> oneLineDisablesPerLine;

    struct PostponedWarning
    {
        QString message;
        QQmlSA::LoggerWarningId category;
        QQmlJS::SourceLocation location;
    };

    std::vector<PostponedWarning> postponedWarnings;
    auto guard = qScopeGuard([&postponedWarnings, &logger]() {
        // only log messages after processing the logger->ignoreWarnings() calls, so that the
        // qmlInvalidLintDirective warnings can be disabled if needed.
        for (const auto &warning : postponedWarnings)
            logger->log(warning.message, warning.category, warning.location);
    });

    const QString code = logger->code();
    const QStringList lines = code.split(u'\n');
    const auto loggerCategories = logger->categories();

    for (const auto &loc : comments) {
        const QString comment = code.mid(loc.offset, loc.length);
        if (!comment.startsWith(u" qmllint ") && !comment.startsWith(u"qmllint "))
            continue;

        QStringList words = comment.split(u' ', Qt::SkipEmptyParts);
        if (words.size() < 2)
            continue;

        QSet<QString> categories;
        for (qsizetype i = 2; i < words.size(); i++) {
            const QString category = words.at(i);
            const auto categoryExists = std::any_of(
                    loggerCategories.cbegin(), loggerCategories.cend(),
                    [&](const QQmlJS::LoggerCategory &cat) { return cat.id().name() == category; });

            if (categoryExists)
                categories << category;
            else {
                postponedWarnings.push_back(
                        { u"qmllint directive on unknown category \"%1\""_s.arg(category),
                          qmlInvalidLintDirective, loc });
            }
        }

        if (words.size() == 2) {
            const auto &loggerCategories = logger->categories();
            for (const auto &option : loggerCategories)
                categories << option.id().name().toString();
        }

        const QString command = words.at(1);
        if (command == u"disable"_s) {
            if (const qsizetype lineIndex = loc.startLine - 1; lineIndex < lines.size()) {
                const QString line = lines[lineIndex];
                const QString preComment = line.left(line.indexOf(comment) - 2);

                bool lineHasContent = false;
                for (qsizetype i = 0; i < preComment.size(); i++) {
                    if (!preComment[i].isSpace()) {
                        lineHasContent = true;
                        break;
                    }
                }

                if (lineHasContent)
                    oneLineDisablesPerLine[loc.startLine] |= categories;
                else
                    disablesPerLine[loc.startLine] |= categories;
            }
        } else if (command == u"enable"_s) {
            enablesPerLine[loc.startLine + 1] |= categories;
        } else {
            postponedWarnings.push_back(
                    { u"Invalid qmllint directive \"%1\" provided"_s.arg(command),
                      qmlInvalidLintDirective, loc });
        }
    }

    if (disablesPerLine.isEmpty() && oneLineDisablesPerLine.isEmpty())
        return;

    QSet<QString> currentlyDisabled;
    for (qsizetype i = 1; i <= lines.size(); i++) {
        currentlyDisabled.unite(disablesPerLine[i]).subtract(enablesPerLine[i]);

        currentlyDisabled.unite(oneLineDisablesPerLine[i]);

        if (!currentlyDisabled.isEmpty())
            logger->ignoreWarnings(i, currentlyDisabled);

        currentlyDisabled.subtract(oneLineDisablesPerLine[i]);
    }
}

static void addJsonWarning(QJsonArray &warnings, const QQmlJS::DiagnosticMessage &message,
                           QAnyStringView id, const std::optional<QQmlJSFixSuggestion> &suggestion = {})
{
    QJsonObject jsonMessage;

    QString type;
    switch (message.type) {
    case QtDebugMsg:
        type = u"debug"_s;
        break;
    case QtWarningMsg:
        type = u"warning"_s;
        break;
    case QtCriticalMsg:
        type = u"critical"_s;
        break;
    case QtFatalMsg:
        type = u"fatal"_s;
        break;
    case QtInfoMsg:
        type = u"info"_s;
        break;
    default:
        type = u"unknown"_s;
        break;
    }

    jsonMessage[u"type"_s] = type;
    jsonMessage[u"id"_s] = id.toString();

    const auto convertLocation = [](const QQmlJS::SourceLocation &source, QJsonObject *target) {
        target->insert("line"_L1, int(source.startLine));
        target->insert("column"_L1, int(source.startColumn));
        target->insert("charOffset"_L1, int(source.offset));
        target->insert("length"_L1, int(source.length));
    };

    if (message.loc.isValid())
        convertLocation(message.loc, &jsonMessage);

    jsonMessage[u"message"_s] = message.message;

    QJsonArray suggestions;
    if (suggestion.has_value()) {
        QJsonArray documentEdits;
        for (const auto &documentEdit : suggestion->documentEdits()) {
            QJsonObject location;
            convertLocation(documentEdit.m_location, &location);
            QJsonObject edit {
                { "filename"_L1, documentEdit.m_filename },
                { "location"_L1, location },
                { "replacement"_L1, documentEdit.m_replacement }
            };
            documentEdits.append(edit);
        }

        QJsonObject jsonFix {
            { "message"_L1, suggestion->description() },
            { "documentEdits"_L1, documentEdits },
            { "isAutoApplicable"_L1, suggestion->isAutoApplicable() },
        };
        convertLocation(suggestion->location(), &jsonFix);
        const QString filename = suggestion->filename();
        if (!filename.isEmpty())
            jsonFix.insert("fileName"_L1, filename);
        suggestions << jsonFix;
    }
    jsonMessage[u"suggestions"] = suggestions;

    warnings << jsonMessage;
}

static void processMessages(const QQmlJSLogger &logger, QJsonArray &warnings)
{
    logger.iterateAllMessages([&](const Message &message) {
        addJsonWarning(warnings, message, message.id, message.fixSuggestion);
    });
}

/*!
\internal
Returns false on already-populated files.

Set up the scope of a file to lazy-load via LinterVisitor.
Returns true on success and false if the scope already was populated.
Retrieve lint results via QQmlJSLinter::lintFileInBatch().
*/
bool QQmlJSLinter::prepareFileForBatchLinting(const QString &dirtyFilename,
                                              const QString *fileContents, LintOptions options,
                                              const QStringList &qmlImportPaths,
                                              const QStringList &qmldirFiles,
                                              const QStringList &resourceFiles,
                                              const QList<QQmlJS::LoggerCategory> &categories)
{
    QFileInfo info(dirtyFilename);
    const QString filenameFromUser =
            QDir::cleanPath(m_useAbsolutePath ? info.absoluteFilePath() : dirtyFilename);

    LintInfo &lintInfo = m_lintInfo[filenameFromUser];
    lintInfo.fileContents = fileContents;
    lintInfo.options = options;
    lintInfo.qmlImportPaths = qmlImportPaths;
    lintInfo.qmldirFiles = qmldirFiles;
    lintInfo.categories = categories;

    const QString lowerSuffix = info.suffix().toLower();
    lintInfo.isESModule = lowerSuffix == QLatin1String("mjs");
    lintInfo.isJavaScript = lintInfo.isESModule || lowerSuffix == QLatin1String("js");

    lintInfo.resourceMapper = { resourceFiles };
    m_importer.setResourceFileMapper(lintInfo.resourceMapper ? &*lintInfo.resourceMapper : nullptr);
    lintInfo.handle = m_importer.importFile(filenameFromUser);
    auto guard = qScopeGuard([this]() { m_importer.setResourceFileMapper(nullptr); });

    if (!lintInfo.handle.factory()) {
        // File was already linted or populated once: resetting its factory might break things, like
        // weakpointers in QQmlJSMetaProperty pointing to children QQmlJSScope of lintInfo.handle.
        m_lintInfo.erase(filenameFromUser);
        return false;
    }

    resetFactory(lintInfo.handle, &m_importer,
                 [this, filenameFromUser](QQmlJSImporter *, const QString &,
                                          const QSharedPointer<QQmlJSScope> &) {
                     return typeReader(filenameFromUser);
                 });
    return true;
}

void QQmlJSLinter::Result::generateJson()
{
    json[u"filename"_s] = logger->filePath();

    QJsonArray warnings;
    processMessages(*logger.get(), warnings);
    json[u"warnings"] = warnings;
    json[u"success"] = status == LintSuccess;
}

void QQmlJSLinter::Result::setStatusFromLogger()
{
    if (logger->hasErrors()) {
        status = HasErrors;
        return;
    }
    if (logger->hasWarnings()) {
        status = HasWarnings;
        return;
    }

    status = LintSuccess;
}

QQmlJSLinter::Result QQmlJSLinter::lintFileInBatch(const QString &dirtyFilename)
{
    QFileInfo info(dirtyFilename);
    const QString filename =
            QDir::cleanPath(m_useAbsolutePath ? info.absoluteFilePath() : dirtyFilename);
    auto it = m_lintInfo.find(filename);
    if (it == m_lintInfo.end() || !it->second.handle.data())
        return { LintResult::FailedToOpen, { }, { } };

    auto &lintInfo = it->second;
    if (lintInfo.result.status != FailedToOpen && lintInfo.result.status != FailedToParse && !lintInfo.isJavaScript)
        lintFileImpl(filename);

    // emit all (possibly pre-recorded) warnings now
    if (const auto &logger = lintInfo.result.logger) {
        logger->manualFlush();
        if (lintInfo.options.testAnyFlag(QQmlJSLinter::GenerateJson))
            lintInfo.result.generateJson();
    }

    Result result = std::move(it->second.result);
    m_lintInfo.erase(it);
    return result;
}

void QQmlJSLinter::setupLoggingCategoriesInLogger(QQmlJSLogger *logger,
                                                  const QList<QQmlJS::LoggerCategory> &categories)
{
    if (m_enablePlugins) {
        for (const Plugin &plugin : m_plugins) {
            for (const QQmlJS::LoggerCategory &category : plugin.categories())
                logger->registerCategory(category);
        }
    }

    for (auto it = categories.cbegin(); it != categories.cend(); ++it) {
        if (auto logger = *it; !QQmlJS::LoggerCategoryPrivate::get(&logger)->hasChanged())
            continue;

        logger->setCategorySeverity(it->id(), it->severity());
    }
}

void QQmlJSLinter::updateUserContextProperties(const QString &fileName)
{
    const QString cachedSettingsPath = m_userContextPropertySettings.currentSettingsPath();
    auto searchResult = m_userContextPropertySettings.search(fileName);
    if (searchResult.iniFilePath == cachedSettingsPath)
        return;
    if (!searchResult.isValid()) {
        m_cachedUserContextProperties = { };
        return;
    }
    m_cachedUserContextProperties = QQmlJS::UserContextProperties{ m_userContextPropertySettings };
}

void QQmlJSLinter::updateHeuristicContextProperties(const QString &fileName)
{
#if QT_CONFIG(qmlcontextpropertydump)
    const QString buildPath =
            QQmlJSUtils::qmlBuildPathFromSourcePath(m_importer.resourceFileMapper(), fileName);

    const QString cachedSettingsPath = m_userContextPropertySettings.currentSettingsPath();
    const auto searchResult = m_heuristicContextPropertySearcher.search(buildPath);
    if (searchResult.iniFilePath == cachedSettingsPath)
        return;
    if (!searchResult.isValid()) {
        m_cachedHeuristicContextProperties = { };
        return;
    }
    QSettings settings(searchResult.iniFilePath, QSettings::IniFormat);
    m_cachedHeuristicContextProperties = QQmlJS::HeuristicContextProperties::collectFrom(&settings);
#endif
}

void QQmlJSLinter::typeReader(const QString &filename)
{
    QString code;

    auto &lintInfo = m_lintInfo[filename];
    auto &result = lintInfo.result;

    result.logger = std::make_unique<QQmlJSLogger>();
    result.logger->setManualFlush(true);
    QFileInfo info(filename);
    result.logger->setFilePath(useAbsolutePath() ? info.absoluteFilePath() : filename);
    result.logger->setSilent(lintInfo.options.testFlag(QQmlJSLinter::Silent)
                             || lintInfo.options.testFlag(QQmlJSLinter::GenerateJson));
    setupLoggingCategoriesInLogger(result.logger.get(), lintInfo.categories);

    if (lintInfo.fileContents == nullptr) {
        QFile file(filename);
        if (!file.open(QFile::ReadOnly)) {
            result.logger->log("Failed to open file %1: %2"_L1.arg(filename, file.errorString()),
                               qmlImport, QQmlJS::SourceLocation());
            result.status = FailedToOpen;
            return;
        }

        code = QString::fromUtf8(file.readAll());
        file.close();
    } else {
        code = *lintInfo.fileContents;
    }

    result.logger->setCode(code);

    QQmlJS::Lexer lexer(&lintInfo.engine);

    lexer.setCode(code, /*lineno = */ 1, /*qmlMode=*/!lintInfo.isJavaScript);
    QQmlJS::Parser parser(&lintInfo.engine);

    const bool parseSuccess = lintInfo.isJavaScript
            ? (lintInfo.isESModule ? parser.parseModule() : parser.parseProgram())
            : parser.parse();
    const auto diagnosticMessages = parser.diagnosticMessages();
    for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages)
        result.logger->log(m.message, qmlSyntax, m.loc);

    if (!parseSuccess) {
        result.status = FailedToParse;
        return;
    }

    m_importer.setImportPaths(lintInfo.qmlImportPaths);

    const QQmlJSResourceFileMapper *mapperPtr =
            lintInfo.resourceMapper ? &*lintInfo.resourceMapper : nullptr;
    m_importer.setResourceFileMapper(mapperPtr);
    // make sure the temporary mapper iscleared from m_importer when it goes out of scope
    auto guard = qScopeGuard([this]() { m_importer.setResourceFileMapper(nullptr); });

    const QString implicitImportDirectory =
            QQmlJSImportVisitor::implicitImportDirectory(result.logger->filePath(), mapperPtr);
    if (lintInfo.isJavaScript) {
        m_importer.runImportVisitor(parser.rootNode(),
                                    {
                                            lintInfo.handle,
                                            lintInfo.result.logger.get(),
                                            implicitImportDirectory,
                                    });
        result.status = LintSuccess;
        return;
    }

    lintInfo.visitor.emplace(&m_importer, result.logger.get(), implicitImportDirectory,
                             lintInfo.qmldirFiles, &lintInfo.engine);

    parseComments(result.logger.get(), lintInfo.engine.comments());
    parser.rootNode()->accept(&*lintInfo.visitor);
}

void QQmlJSLinter::lintFileImpl(const QString &filename)
{
    Q_ASSERT(m_lintInfo.count(filename) == 1);
    LintInfo &lintInfo = m_lintInfo[filename];

    Q_ASSERT(!lintInfo.isJavaScript);

    QQmlJSTypeResolver typeResolver(&m_importer);

    // Type resolving is using document parent mode here so that it produces fewer false
    // positives on the "parent" property of QQuickItem. It does produce a few false
    // negatives this way because items can be reparented. Furthermore, even if items
    // are not reparented, the document parent may indeed not be their visual parent.
    // See QTBUG-95530. Eventually, we'll need cleverer logic to deal with this.
    typeResolver.setParentMode(QQmlJSTypeResolver::UseDocumentParent);
    // We don't need to create tracked types and such as we are just linting the code
    // here and not actually compiling it. The duplicated scopes would cause issues
    // during linting.
    typeResolver.setCloneMode(QQmlJSTypeResolver::DoNotCloneTypes);

    Q_ASSERT(lintInfo.visitor);
    auto &v = *lintInfo.visitor;
    typeResolver.init(&v, nullptr);

    QStringList resourcePaths;
    if (auto &mapper = lintInfo.resourceMapper)
        resourcePaths = mapper->resourcePaths(QQmlJSResourceFileMapper::localFileFilter(filename));

    m_importer.setResourceFileMapper(lintInfo.resourceMapper ? &*lintInfo.resourceMapper : nullptr);
    auto guard = qScopeGuard([this]() { m_importer.setResourceFileMapper(nullptr); });

    const QString resolvedPath =
            (resourcePaths.size() == 1) ? u':' + resourcePaths.first() : filename;

    updateHeuristicContextProperties(filename);
    updateUserContextProperties(filename);

    QQmlJS::LinterContext context{
        v.addressableScopes(),         *v.knownUnresolvedTypes(),
        v.renamedComponents(),         m_importer,
        m_cachedUserContextProperties, m_cachedHeuristicContextProperties
    };

    QQmlJSLinterCodegen codegen{
        &m_importer, resolvedPath, lintInfo.qmldirFiles, lintInfo.result.logger.get(), context,
    };
    codegen.setTypeResolver(std::move(typeResolver));

    using PassManagerPtr =
            std::unique_ptr<QQmlSA::PassManager,
                            decltype(&QQmlSA::PassManagerPrivate::deletePassManager)>;
    PassManagerPtr passMan(
            QQmlSA::PassManagerPrivate::createPassManager(&v, codegen.typeResolver()),
            &QQmlSA::PassManagerPrivate::deletePassManager);
    QQmlJSLinterPasses::registerDefaultPasses(passMan.get());

    if (m_enablePlugins) {
        for (const Plugin &plugin : m_plugins) {
            if (!plugin.isValid() || !plugin.isEnabled())
                continue;

            QQmlSA::LintPlugin *instance = plugin.m_instance;
            Q_ASSERT(instance);
            instance->registerPasses(passMan.get(), QQmlJSScope::createQQmlSAElement(v.result()));
        }
    }
    passMan->analyze(QQmlJSScope::createQQmlSAElement(v.result()));

    if (lintInfo.result.logger->hasErrors()) {
        lintInfo.result.status = HasErrors;
        return;
    }

    // passMan now has a pointer to the moved from type resolver
    // we fix this in setPassManager
    codegen.setPassManager(passMan.get());

    QQmlJSSaveFunction saveFunction = [](const QV4::CompiledData::SaveableUnitPointer &,
                                         const QQmlJSAotFunctionMap &,
                                         const LookupSignatures &,
                                         const QString *) { return true; };

    QQmlJSCompileError error;

    QLoggingCategory::setFilterRules(u"qt.qml.compiler=false"_s);

    CodegenWarningInterface warningInterface(lintInfo.result.logger.get());
    qCompileQmlFile(filename, saveFunction, &codegen, &error, true, &warningInterface,
                    lintInfo.fileContents);

    QList<QQmlJS::DiagnosticMessage> globalWarnings = m_importer.takeGlobalWarnings();

    if (!globalWarnings.isEmpty()) {
        lintInfo.result.logger->log(QStringLiteral("Type warnings occurred while evaluating file:"),
                                    qmlImport, QQmlJS::SourceLocation());
        lintInfo.result.logger->processMessages(globalWarnings, qmlImport);
    }

    lintInfo.result.setStatusFromLogger();
}

QQmlJSLinter::Result QQmlJSLinter::lintModule(const QString &module, LintOptions options,
                                              const QStringList &qmlImportPaths,
                                              const QStringList &resourceFiles)
{
    Result lintResult = lintModuleImpl(module, options, qmlImportPaths, resourceFiles);
    if (!options.testFlag(GenerateJson))
        return lintResult;

    QJsonArray warnings;
    processMessages(*lintResult.logger, warnings);

    lintResult.json[u"module"_s] = module;
    lintResult.json[u"warnings"] = warnings;
    lintResult.json[u"success"] = lintResult.status == LintSuccess;

    return lintResult;
}

QQmlJSLinter::Result QQmlJSLinter::lintModuleImpl(const QString &module, LintOptions options,
                                                  const QStringList &qmlImportPaths,
                                                  const QStringList &resourceFiles)
{
    Result result;
    result.logger = std::make_unique<QQmlJSLogger>();

    // We can't lint properly if a module has already been pre-cached
    m_importer.clearCache();

    // We don't support file selectors during module linting currently
    const QQmlJSImporterFlags oldFlags = m_importer.flags();
    QQmlJSImporterFlags newFlags = oldFlags;
    newFlags.setFlag(TolerateFileSelectors, false);
    m_importer.setFlags(newFlags);
    auto flagGuard = qScopeGuard([this, oldFlags]() { m_importer.setFlags(oldFlags); });
    m_importer.setImportPaths(qmlImportPaths);

    QQmlJSResourceFileMapper mapper(resourceFiles);
    if (!resourceFiles.isEmpty())
        m_importer.setResourceFileMapper(&mapper);
    else
        m_importer.setResourceFileMapper(nullptr);
    auto guard = qScopeGuard([this]() { m_importer.setResourceFileMapper(nullptr); });

    result.logger->setFilePath(module);
    result.logger->setCode(u""_s);
    result.logger->setSilent(options.testFlag(Silent) || options.testFlag(GenerateJson));

    const QQmlJSImporter::ImportedTypes types =
            m_importer.importModule(module, quint8(QQmlJS::PrecedenceValues::Default));

    QList<QQmlJS::DiagnosticMessage> importWarnings =
            m_importer.takeGlobalWarnings() + types.warnings();

    if (!importWarnings.isEmpty()) {
        result.logger->log(QStringLiteral("Warnings occurred while importing module:"), qmlImport,
                           QQmlJS::SourceLocation());
        result.logger->processMessages(importWarnings, qmlImport);
    }

    QMap<QString, QSet<QString>> missingTypes;
    QMap<QString, QSet<QString>> partiallyResolvedTypes;

    const QString modulePrefix = u"$module$."_s;
    const QString internalPrefix = u"$internal$."_s;

    for (auto &&[typeName, importedScope] : types.types().asKeyValueRange()) {
        QString name = typeName;
        const QQmlJSScope::ConstPtr scope = importedScope.scope;

        if (name.startsWith(modulePrefix))
            continue;

        if (name.startsWith(internalPrefix)) {
            name = name.mid(internalPrefix.size());
        }

        if (scope.isNull()) {
            if (!missingTypes.contains(name))
                missingTypes[name] = {};
            continue;
        }

        if (!scope->isFullyResolved()) {
            if (!partiallyResolvedTypes.contains(name))
                partiallyResolvedTypes[name] = {};
        }
        const auto &ownProperties = scope->ownProperties();
        for (const auto &property : ownProperties) {
            if (property.typeName().isEmpty()) {
                // If the type name is empty, then it's an intentional vaguery i.e. for some
                // builtins
                continue;
            }
            if (property.type().isNull()) {
                missingTypes[property.typeName()]
                        << scope->internalName() + u'.' + property.propertyName();
                continue;
            }
            if (!property.type()->isFullyResolved()) {
                partiallyResolvedTypes[property.typeName()]
                        << scope->internalName() + u'.' + property.propertyName();
            }
        }
        if (scope->attachedType() && !scope->attachedType()->isFullyResolved()) {
            result.logger->log(u"Attached type of \"%1\" not fully resolved"_s.arg(name),
                               qmlUnresolvedType, scope->sourceLocation());
        }

        const auto &ownMethods = scope->ownMethods();
        for (const auto &method : ownMethods) {
            if (method.returnTypeName().isEmpty())
                continue;
            if (method.returnType().isNull()) {
                missingTypes[method.returnTypeName()] << u"return type of "_s
                                + scope->internalName() + u'.' + method.methodName() + u"()"_s;
            } else if (!method.returnType()->isFullyResolved()) {
                partiallyResolvedTypes[method.returnTypeName()] << u"return type of "_s
                                + scope->internalName() + u'.' + method.methodName() + u"()"_s;
            }

            const auto parameters = method.parameters();
            for (qsizetype i = 0; i < parameters.size(); i++) {
                auto &parameter = parameters[i];
                const QString typeName = parameter.typeName();
                const QSharedPointer<const QQmlJSScope> type = parameter.type();
                if (typeName.isEmpty())
                    continue;
                if (type.isNull()) {
                    missingTypes[typeName] << u"parameter %1 of "_s.arg(i + 1)
                                    + scope->internalName() + u'.' + method.methodName() + u"()"_s;
                    continue;
                }
                if (!type->isFullyResolved()) {
                    partiallyResolvedTypes[typeName] << u"parameter %1 of "_s.arg(i + 1)
                                    + scope->internalName() + u'.' + method.methodName() + u"()"_s;
                    continue;
                }
            }
        }
    }

    for (auto &&[name, uses] :  missingTypes.asKeyValueRange()) {
        QString message = u"Type \"%1\" not found"_s.arg(name);

        if (!uses.isEmpty()) {
            const QStringList usesList = QStringList(uses.begin(), uses.end());
            message += u". Used in %1"_s.arg(usesList.join(u", "_s));
        }

        result.logger->log(message, qmlUnresolvedType, QQmlJS::SourceLocation());
    }

    for (auto &&[name, uses] : partiallyResolvedTypes.asKeyValueRange()) {
        QString message = u"Type \"%1\" is not fully resolved"_s.arg(name);

        if (!uses.isEmpty()) {
            const QStringList usesList = QStringList(uses.begin(), uses.end());
            message += u". Used in %1"_s.arg(usesList.join(u", "_s));
        }

        result.logger->log(message, qmlUnresolvedType, QQmlJS::SourceLocation());
    }

    result.status = (result.logger->hasWarnings() || result.logger->hasErrors()) ? HasWarnings
                                                                                 : LintSuccess;
    return result;
}

QQmlJSLinter::FixResult QQmlJSLinter::applyFixes(const QQmlJSLogger *logger, QString *fixedCode,
                                                 bool silent)
{
    Q_ASSERT(fixedCode != nullptr);

    // This means that the necessary analysis for applying fixes hasn't run for some reason
    // (because it was JS file, a syntax error etc.). We can't procede without it and if an error
    // has occurred that has to be handled by the caller. Just say that there is
    // nothing to fix.
    if (logger == nullptr)
        return NothingToFix;

    QString code = logger->code();

    QList<QQmlJSFixSuggestion> fixesToApply;

    QFileInfo info(logger->filePath());
    const QString currentFileAbsolutePath = info.absoluteFilePath();

    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");

    if (isESModule || isJavaScript)
        return NothingToFix;

    logger->iterateAllMessages([&](const Message &msg) {
        if (!msg.fixSuggestion.has_value() || !msg.fixSuggestion->isAutoApplicable())
            return;

        // Ignore fix suggestions for other files
        const QString filename = msg.fixSuggestion->filename();
        if (!filename.isEmpty()
                && QFileInfo(filename).absoluteFilePath() != currentFileAbsolutePath) {
            return;
        }

        fixesToApply << msg.fixSuggestion.value();
    });

    if (fixesToApply.isEmpty())
        return NothingToFix;

    QList<QQmlJSDocumentEdit> documentEdits;
    for (const auto &fixToApply : std::as_const(fixesToApply)) {
        const auto &fixDocumentEdits = fixToApply.documentEdits();
        for (const auto &documentEdit : fixDocumentEdits) {
            // TODO also apply documentEdits in other files
            if (documentEdit.m_filename == logger->filePath())
                documentEdits << documentEdit;
        }
    }

    std::sort(documentEdits.begin(), documentEdits.end(),
              [](const QQmlJSDocumentEdit &a, const QQmlJSDocumentEdit &b) {
                  return a.m_location.offset < b.m_location.offset;
              });

    const auto dupes = std::unique(documentEdits.begin(), documentEdits.end());
    documentEdits.erase(dupes, documentEdits.end());

    for (auto it = documentEdits.begin(); it + 1 != documentEdits.end(); it++) {
        const QQmlJS::SourceLocation srcLocA = it->m_location;
        const QQmlJS::SourceLocation srcLocB = (it + 1)->m_location;
        if (srcLocA.offset + srcLocA.length > srcLocB.offset) {
            if (!silent)
                qWarning() << "Document edits for warning fixes are overlapping, aborting. "
                              "Please file a bug report if this is a Qt warning";
            return FixError;
        }
    }

    int offsetEdit = 0;

    for (const auto &edit : std::as_const(documentEdits)) {
        const QQmlJS::SourceLocation fixLocation = edit.m_location;
        qsizetype cutLocation = fixLocation.offset + offsetEdit;
        const QString before = code.left(cutLocation);
        const QString after = code.mid(cutLocation + fixLocation.length);

        const QString replacement = edit.m_replacement;
        code = before + replacement + after;
        offsetEdit += replacement.size() - fixLocation.length;
    }

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    lexer.setCode(code, /*lineno = */ 1, /*qmlMode=*/!isJavaScript);
    QQmlJS::Parser parser(&engine);

    bool success = parser.parse();

    if (!success) {
        const auto diagnosticMessages = parser.diagnosticMessages();

        if (!silent) {
            qDebug() << "File became unparseable after suggestions were applied. Please file a bug "
                        "report.";
        } else {
            return FixError;
        }

        for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
            qWarning().noquote() << QString::fromLatin1("%1:%2:%3: %4")
                                            .arg(logger->filePath())
                                            .arg(m.loc.startLine)
                                            .arg(m.loc.startColumn)
                                            .arg(m.message);
        }
        return FixError;
    }

    *fixedCode = code;
    return FixSuccess;
}

QT_END_NAMESPACE
