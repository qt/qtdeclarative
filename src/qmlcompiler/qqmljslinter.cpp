// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljslinter_p.h"

#include "qqmljslintercodegen_p.h"

#include <QtQmlCompiler/private/qqmljsimporter_p.h>
#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>
#include <QtQmlCompiler/private/qqmljsliteralbindingcheck_p.h>

#include <QtCore/qjsonobject.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/qscopedpointer.h>

#include <QtQmlCompiler/private/qqmlsa_p.h>

#if QT_CONFIG(library)
#    include <QtCore/qdiriterator.h>
#    include <QtCore/qlibrary.h>
#endif

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsdiagnosticmessage_p.h>


QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class CodegenWarningInterface final : public QV4::Compiler::CodegenWarningInterface
{
public:
    CodegenWarningInterface(QQmlJSLogger *logger) : m_logger(logger) { }

    void reportVarUsedBeforeDeclaration(const QString &name, const QString &fileName,
                                        QQmlJS::SourceLocation declarationLocation,
                                        QQmlJS::SourceLocation accessLocation) override
    {
        Q_UNUSED(fileName)
        m_logger->log(
                u"Variable \"%1\" is used here before its declaration. The declaration is at %2:%3."_s
                        .arg(name)
                        .arg(declarationLocation.startLine)
                        .arg(declarationLocation.startColumn),
                Log_Type, accessLocation);
    }

private:
    QQmlJSLogger *m_logger;
};

QString QQmlJSLinter::defaultPluginPath()
{
    return QLibraryInfo::path(QLibraryInfo::PluginsPath) + QDir::separator() + u"qmllint";
}

QQmlJSLinter::QQmlJSLinter(const QStringList &importPaths, const QStringList &pluginPaths,
                           bool useAbsolutePath)
    : m_useAbsolutePath(useAbsolutePath),
      m_enablePlugins(true),
      m_importer(importPaths, nullptr, true)
{
    m_plugins = loadPlugins(pluginPaths);
}

QQmlJSLinter::Plugin::Plugin(QQmlJSLinter::Plugin &&plugin) noexcept
{
    m_name = plugin.m_name;
    m_author = plugin.m_author;
    m_description = plugin.m_description;
    m_version = plugin.m_version;
    m_instance = plugin.m_instance;
    m_loader = plugin.m_loader;
    m_isValid = plugin.m_isValid;
    m_isBuiltin = plugin.m_isBuiltin;

    // Mark the old Plugin as invalid and make sure it doesn't delete the loader
    plugin.m_loader = nullptr;
    plugin.m_instance = nullptr;
    plugin.m_isValid = false;
}

#if QT_CONFIG(library)
QQmlJSLinter::Plugin::Plugin(QString path)
{
    m_loader = new QPluginLoader(path);
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

    for (const QString &requiredKey : { u"name"_s, u"version"_s, u"author"_s }) {
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

    return true;
}

std::vector<QQmlJSLinter::Plugin> QQmlJSLinter::loadPlugins(QStringList paths)
{

    std::vector<Plugin> plugins;

    QDuplicateTracker<QString> seenPlugins;

    for (const QStaticPlugin &staticPlugin : QPluginLoader::staticPlugins()) {
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
    for (const QString &pluginDir : paths) {
        QDirIterator it { pluginDir };

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

    return plugins;
}

void QQmlJSLinter::parseComments(QQmlJSLogger *logger,
                                 const QList<QQmlJS::SourceLocation> &comments)
{
    QHash<int, QSet<QQmlJSLoggerCategory>> disablesPerLine;
    QHash<int, QSet<QQmlJSLoggerCategory>> enablesPerLine;
    QHash<int, QSet<QQmlJSLoggerCategory>> oneLineDisablesPerLine;

    const QString code = logger->code();
    const QStringList lines = code.split(u'\n');

    for (const auto &loc : comments) {
        const QString comment = code.mid(loc.offset, loc.length);
        if (!comment.startsWith(u" qmllint ") && !comment.startsWith(u"qmllint "))
            continue;

        QStringList words = comment.split(u' ', Qt::SkipEmptyParts);
        if (words.size() < 2)
            continue;

        const QString command = words.at(1);

        QSet<QQmlJSLoggerCategory> categories;
        for (qsizetype i = 2; i < words.size(); i++) {
            const QString category = words.at(i);
            const auto option = logger->options().constFind(category);
            if (option != logger->options().constEnd())
                categories << option->m_category;
            else
                logger->log(u"qmllint directive on unknown category \"%1\""_s.arg(category),
                            Log_Syntax, loc);
        }

        if (categories.isEmpty()) {
            for (const auto &option : logger->options())
                categories << option.m_category;
        }

        if (command == u"disable"_s) {
            if (const qsizetype lineIndex = loc.startLine - 1; lineIndex < lines.size()) {
                const QString line = lines[loc.startLine - 1];
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
            logger->log(u"Invalid qmllint directive \"%1\" provided"_s.arg(command), Log_Syntax,
                        loc);
        }
    }

    if (disablesPerLine.isEmpty() && oneLineDisablesPerLine.isEmpty())
        return;

    QSet<QQmlJSLoggerCategory> currentlyDisabled;
    for (qsizetype i = 1; i <= lines.length(); i++) {
        currentlyDisabled.unite(disablesPerLine[i]).subtract(enablesPerLine[i]);

        currentlyDisabled.unite(oneLineDisablesPerLine[i]);

        if (!currentlyDisabled.isEmpty())
            logger->ignoreWarnings(i, currentlyDisabled);

        currentlyDisabled.subtract(oneLineDisablesPerLine[i]);
    }
}

QQmlJSLinter::LintResult QQmlJSLinter::lintFile(const QString &filename,
                                                const QString *fileContents, const bool silent,
                                                QJsonArray *json, const QStringList &qmlImportPaths,
                                                const QStringList &qmldirFiles,
                                                const QStringList &resourceFiles,
                                                const QMap<QString, QQmlJSLogger::Option> &options)
{
    // Make sure that we don't expose an old logger if we return before a new one is created.
    m_logger.reset();

    QJsonArray warnings;
    QJsonObject result;

    bool success = true;

    QScopeGuard jsonOutput([&] {
        if (!json)
            return;

        result[u"filename"_s] = QFileInfo(filename).absoluteFilePath();
        result[u"warnings"] = warnings;
        result[u"success"] = success;

        json->append(result);
    });

    auto addJsonWarning = [&](const QQmlJS::DiagnosticMessage &message,
                              const std::optional<FixSuggestion> &suggestion = {}) {
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

        if (message.loc.isValid()) {
            jsonMessage[u"line"_s] = static_cast<int>(message.loc.startLine);
            jsonMessage[u"column"_s] = static_cast<int>(message.loc.startColumn);
            jsonMessage[u"charOffset"_s] = static_cast<int>(message.loc.offset);
            jsonMessage[u"length"_s] = static_cast<int>(message.loc.length);
        }

        jsonMessage[u"message"_s] = message.message;

        QJsonArray suggestions;
        if (suggestion.has_value()) {
            for (const auto &fix : suggestion->fixes) {
                QJsonObject jsonFix;
                jsonFix[u"message"] = fix.message;
                jsonFix[u"line"_s] = static_cast<int>(fix.cutLocation.startLine);
                jsonFix[u"column"_s] = static_cast<int>(fix.cutLocation.startColumn);
                jsonFix[u"charOffset"_s] = static_cast<int>(fix.cutLocation.offset);
                jsonFix[u"length"_s] = static_cast<int>(fix.cutLocation.length);
                jsonFix[u"replacement"_s] = fix.replacementString;
                jsonFix[u"isHint"] = fix.isHint;
                if (!fix.fileName.isEmpty())
                    jsonFix[u"fileName"] = fix.fileName;
                suggestions << jsonFix;
            }
        }
        jsonMessage[u"suggestions"] = suggestions;

        warnings << jsonMessage;
    };

    QString code;

    if (fileContents == nullptr) {
        QFile file(filename);
        if (!file.open(QFile::ReadOnly)) {
            if (json) {
                addJsonWarning(
                        QQmlJS::DiagnosticMessage { QStringLiteral("Failed to open file %1: %2")
                                                            .arg(filename, file.errorString()),
                                                    QtCriticalMsg, QQmlJS::SourceLocation() });
                success = false;
            } else if (!silent) {
                qWarning() << "Failed to open file" << filename << file.error();
            }
            return FailedToOpen;
        }

        code = QString::fromUtf8(file.readAll());
        file.close();
    } else {
        code = *fileContents;
    }

    m_fileContents = code;

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    QFileInfo info(filename);
    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");

    lexer.setCode(code, /*lineno = */ 1, /*qmlMode=*/!isJavaScript);
    QQmlJS::Parser parser(&engine);

    success = isJavaScript ? (isESModule ? parser.parseModule() : parser.parseProgram())
                           : parser.parse();

    if (!success) {
        const auto diagnosticMessages = parser.diagnosticMessages();
        for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
            if (json) {
                addJsonWarning(m);
            } else if (!silent) {
                qWarning().noquote() << QString::fromLatin1("%1:%2:%3: %4")
                                                .arg(filename)
                                                .arg(m.loc.startLine)
                                                .arg(m.loc.startColumn)
                                                .arg(m.message);
            }
        }
        return FailedToParse;
    }

    if (success && !isJavaScript) {
        const auto processMessages = [&]() {
            if (json) {
                for (const auto &error : m_logger->errors())
                    addJsonWarning(error, error.fixSuggestion);
                for (const auto &warning : m_logger->warnings())
                    addJsonWarning(warning, warning.fixSuggestion);
                for (const auto &info : m_logger->infos())
                    addJsonWarning(info, info.fixSuggestion);
            }
        };

        const auto check = [&](QQmlJSResourceFileMapper *mapper) {
            if (m_importer.importPaths() != qmlImportPaths)
                m_importer.setImportPaths(qmlImportPaths);

            m_importer.setResourceFileMapper(mapper);

            m_logger.reset(new QQmlJSLogger);
            m_logger->setFileName(m_useAbsolutePath ? info.absoluteFilePath() : filename);
            m_logger->setCode(code);
            m_logger->setSilent(silent || json);
            QQmlJSScope::Ptr target = QQmlJSScope::create();
            QQmlJSImportVisitor v { target, &m_importer, m_logger.get(),
                                    QQmlJSImportVisitor::implicitImportDirectory(
                                            m_logger->fileName(), m_importer.resourceFileMapper()),
                                    qmldirFiles };

            parseComments(m_logger.get(), engine.comments());

            for (auto it = options.cbegin(); it != options.cend(); ++it) {
                if (!it.value().m_changed)
                    continue;

                m_logger->setCategoryIgnored(it.value().m_category, it.value().m_ignored);
                m_logger->setCategoryLevel(it.value().m_category, it.value().m_level);
            }

            QQmlJSTypeResolver typeResolver(&m_importer);

            // Type resolving is using document parent mode here so that it produces fewer false
            // positives on the "parent" property of QQuickItem. It does produce a few false
            // negatives this way because items can be reparented. Furthermore, even if items are
            // not reparented, the document parent may indeed not be their visual parent. See
            // QTBUG-95530. Eventually, we'll need cleverer logic to deal with this.
            typeResolver.setParentMode(QQmlJSTypeResolver::UseDocumentParent);
            // We don't need to create tracked types and such as we are just linting the code here
            // and not actually compiling it. The duplicated scopes would cause issues during
            // linting.
            typeResolver.setCloneMode(QQmlJSTypeResolver::DoNotCloneTypes);

            typeResolver.init(&v, parser.rootNode());

            QQmlJSLiteralBindingCheck literalCheck;
            literalCheck.run(&v, &typeResolver);

            QScopedPointer<QQmlSA::PassManager> passMan;

            if (m_enablePlugins) {
                passMan.reset(new QQmlSA::PassManager(&v, &typeResolver));

                for (const Plugin &plugin : m_plugins) {
                    if (!plugin.isValid() || !plugin.isEnabled())
                        continue;

                    QQmlSA::LintPlugin *instance = plugin.m_instance;
                    Q_ASSERT(instance);
                    instance->registerPasses(passMan.get(), v.result());
                }

                passMan->analyze(v.result());
            }

            success = !m_logger->hasWarnings() && !m_logger->hasErrors();

            if (m_logger->hasErrors()) {
                processMessages();
                return;
            }

            QQmlJSTypeInfo typeInfo;

            const QStringList resourcePaths = mapper
                    ? mapper->resourcePaths(QQmlJSResourceFileMapper::localFileFilter(filename))
                    : QStringList();
            const QString resolvedPath =
                    (resourcePaths.size() == 1) ? u':' + resourcePaths.first() : filename;

            QQmlJSLinterCodegen codegen { &m_importer, resolvedPath, qmldirFiles, m_logger.get(),
                                          &typeInfo };
            codegen.setTypeResolver(std::move(typeResolver));
            if (passMan)
                codegen.setPassManager(passMan.get());
            QQmlJSSaveFunction saveFunction = [](const QV4::CompiledData::SaveableUnitPointer &,
                                                 const QQmlJSAotFunctionMap &,
                                                 QString *) { return true; };

            QQmlJSCompileError error;

            QLoggingCategory::setFilterRules(u"qt.qml.compiler=false"_s);

            CodegenWarningInterface interface(m_logger.get());
            qCompileQmlFile(filename, saveFunction, &codegen, &error, true, &interface,
                            fileContents);

            QList<QQmlJS::DiagnosticMessage> warnings = m_importer.takeGlobalWarnings();

            if (!warnings.isEmpty()) {
                m_logger->log(QStringLiteral("Type warnings occurred while evaluating file:"),
                              Log_Import, QQmlJS::SourceLocation());
                m_logger->processMessages(warnings, Log_Import);
            }

            success &= !m_logger->hasWarnings() && !m_logger->hasErrors();

            processMessages();
        };

        if (resourceFiles.isEmpty()) {
            check(nullptr);
        } else {
            QQmlJSResourceFileMapper mapper(resourceFiles);
            check(&mapper);
        }
    }

    return success ? LintSuccess : HasWarnings;
}

QQmlJSLinter::FixResult QQmlJSLinter::applyFixes(QString *fixedCode, bool silent)
{
    Q_ASSERT(fixedCode != nullptr);

    // This means that the necessary analysis for applying fixes hasn't run for some reason
    // (because it was JS file, a syntax error etc.). We can't procede without it and if an error
    // has occurred that has to be handled by the caller of lintFile(). Just say that there is
    // nothing to fix.
    if (m_logger == nullptr)
        return NothingToFix;

    QString code = m_fileContents;

    QList<FixSuggestion::Fix> fixesToApply;

    QFileInfo info(m_logger->fileName());
    const QString currentFileAbsolutePath = info.absoluteFilePath();

    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");

    if (isESModule || isJavaScript)
        return NothingToFix;

    for (const auto &messages : { m_logger->infos(), m_logger->warnings(), m_logger->errors() })
        for (const Message &msg : messages) {
            if (!msg.fixSuggestion.has_value())
                continue;

            for (const auto &fix : msg.fixSuggestion->fixes) {
                if (fix.isHint)
                    continue;

                // Ignore fix suggestions for other files
                if (!fix.fileName.isEmpty()
                    && QFileInfo(fix.fileName).absoluteFilePath() != currentFileAbsolutePath) {
                    continue;
                }

                fixesToApply << fix;
            }
        }

    if (fixesToApply.isEmpty())
        return NothingToFix;

    std::sort(fixesToApply.begin(), fixesToApply.end(),
              [](FixSuggestion::Fix &a, FixSuggestion::Fix &b) {
                  return a.cutLocation.offset < b.cutLocation.offset;
              });

    for (auto it = fixesToApply.begin(); it + 1 != fixesToApply.end(); it++) {
        QQmlJS::SourceLocation srcLocA = it->cutLocation;
        QQmlJS::SourceLocation srcLocB = (it + 1)->cutLocation;
        if (srcLocA.offset + srcLocA.length > srcLocB.offset) {
            if (!silent)
                qWarning() << "Fixes for two warnings are overlapping, aborting. Please file a bug "
                              "report.";
            return FixError;
        }
    }

    int offsetChange = 0;

    for (const auto &fix : fixesToApply) {
        qsizetype cutLocation = fix.cutLocation.offset + offsetChange;
        QString before = code.left(cutLocation);
        QString after = code.mid(cutLocation + fix.cutLocation.length);

        code = before + fix.replacementString + after;
        offsetChange += fix.replacementString.length() - fix.cutLocation.length;
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
                                            .arg(m_logger->fileName())
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
