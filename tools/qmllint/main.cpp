// Copyright (C) 2016 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sergio Martins <sergio.martins@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <private/qqmljscompiler_p.h>
#include <private/qqmljslinter_p.h>
#include <private/qqmljsloggingutils_p.h>
#include <private/qqmljsresourcefilemapper_p.h>
#include <private/qqmljsutils_p.h>
#include <private/qqmltoolingsettings_p.h>
#include <private/qqmltoolingutils_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qscopeguard.h>

#if QT_CONFIG(commandlineparser)
#include <QtCore/qcommandlineparser.h>
#endif

#include <QtCore/qlibraryinfo.h>

#include <cstdio>

using namespace Qt::StringLiterals;

// Bump this whenever the qmllint JSON format changes
//
// IMPORTANT:
//
// Also change the comment behind the number to describe the latest change. This has the added
// benefit that if another patch changes the version too, it will result in a merge conflict, and
// not get removed silently.
constexpr int JSON_LOGGING_FORMAT_REVISION = 6; // Support multiple DocumentEdits per FixSuggestion

bool argumentsFromCommandLineAndFile(QStringList& allArguments, const QStringList &arguments)
{
    allArguments.reserve(arguments.size());
    for (const QString &argument : arguments) {
        // "@file" doesn't start with a '-' so we can't use QCommandLineParser for it
        if (argument.startsWith(u'@')) {
            QString optionsFile = argument;
            optionsFile.remove(0, 1);
            if (optionsFile.isEmpty()) {
                qWarning().nospace() << "The @ option requires an input file";
                return false;
            }
            QFile f(optionsFile);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qWarning().nospace() << "Cannot open options file specified with @";
                return false;
            }
            while (!f.atEnd()) {
                QString line = QString::fromLocal8Bit(f.readLine().trimmed());
                if (!line.isEmpty())
                    allArguments << line;
            }
        } else {
            allArguments << argument;
        }
    }
    return true;
}

static bool applyFixes(const QQmlJSLinter::Result &linterResult, bool silent, bool dryRun)
{
    if (linterResult.status != QQmlJSLinter::LintSuccess
        && linterResult.status != QQmlJSLinter::HasWarnings) {
        return true;
    }

    QString fixedCode;
    const QQmlJSLinter::FixResult result =
            QQmlJSLinter::applyFixes(linterResult.logger.get(), &fixedCode, silent);
    const QString filename = linterResult.logger->filePath();

    if (result != QQmlJSLinter::NothingToFix && result != QQmlJSLinter::FixSuccess) {
        return false;
    }

    if (dryRun) {
        QTextStream(stdout) << fixedCode;
        return true;
    }
    if (result == QQmlJSLinter::NothingToFix) {
        if (!silent)
            qWarning().nospace() << "Nothing to fix in " << filename;
        return true;
    }

    const QString backupFile = filename + u".bak"_s;
    if (QFile::exists(backupFile) && !QFile::remove(backupFile)) {
        if (!silent) {
            qWarning().nospace() << "Failed to remove old backup file " << backupFile
                                 << ", aborting";
        }
        return false;
    }
    if (!QFile::copy(filename, backupFile)) {
        if (!silent) {
            qWarning().nospace() << "Failed to create backup file " << backupFile << ", aborting";
        }
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        if (!silent) {
            qWarning().nospace() << "Failed to open " << filename
                                 << " for writing:" << file.errorString();
        }
        return false;
    }

    const QByteArray data = fixedCode.toUtf8();
    if (file.write(data) != data.size()) {
        if (!silent) {
            qWarning().nospace() << "Failed to write new contents to " << filename << ": "
                                 << file.errorString();
        }
        return false;
    }
    if (!silent) {
        qDebug().nospace() << "Applied fixes to " << filename << ". Backup created at "
                           << backupFile;
    }
    return true;
}

int main(int argc, char *argv[])
{
    QHashSeed::setDeterministicGlobalSeed();
    QList<QQmlJS::LoggerCategory> defaultCategories;

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("qmllint");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    QQmlToolingSettings defaultSettings(QLatin1String("qmllint"),
                                        { QLatin1String("General"), QLatin1String("Warnings") });
    parser.setApplicationDescription(QLatin1String(R"(QML syntax verifier and analyzer

All warnings can be set to four levels of severity:
    disable - Fully disables the warning.
    info - Displays the warning but does not influence the return code.
    warning - Displays the warning and leads to a non-zero exit code if more warnings than max-warnings occur.
    error - Displays the warning as error and leads to a non-zero exit code if encountered.
)"));

    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption silentOption(QStringList() << "s" << "silent",
                                    QLatin1String("Don't output syntax errors"));
    parser.addOption(silentOption);

    QCommandLineOption jsonOption(QStringList() << "json",
                                  QLatin1String("Write output as JSON to file (or use the special "
                                                "filename '-'  to write to stdout)"),
                                  QLatin1String("file"), QString());
    parser.addOption(jsonOption);

    QCommandLineOption writeDefaultsOption(
            QStringList() << "write-defaults",
            QLatin1String("Writes defaults settings to .qmllint.ini and exits (Warning: This "
                          "will overwrite any existing settings and comments!)"));
    parser.addOption(writeDefaultsOption);

    QCommandLineOption ignoreSettings(QStringList() << "ignore-settings",
                                      QLatin1String("Ignores all settings files and only takes "
                                                    "command line options into consideration"));
    parser.addOption(ignoreSettings);

    QCommandLineOption moduleOption({ QStringLiteral("M"), QStringLiteral("module") },
                                    QStringLiteral("Lint modules instead of files"));
    parser.addOption(moduleOption);

    QCommandLineOption resourceOption(
                { QStringLiteral("resource") },
                QStringLiteral("Look for related files in the given resource file"),
                QStringLiteral("resource"));
    parser.addOption(resourceOption);
    const QString &resourceSetting = QLatin1String("ResourcePath");
    defaultSettings.addOption(resourceSetting);

    QCommandLineOption qmlImportPathsOption(
            QStringList() << "I"
                          << "qmldirs",
            QLatin1String("Look for QML modules in specified directory"),
            QLatin1String("directory"));
    parser.addOption(qmlImportPathsOption);
    const QString qmlImportPathsSetting = QLatin1String("AdditionalQmlImportPaths");
    defaultSettings.addOption(qmlImportPathsSetting);

    QCommandLineOption environmentOption(
            QStringList() << "E",
            QLatin1String("Use the QML_IMPORT_PATH environment variable to look for QML Modules"));
    parser.addOption(environmentOption);

    QCommandLineOption qmlImportNoDefault(
                QStringList() << "bare",
                QLatin1String("Do not include default import directories or the current directory. "
                              "This may be used to run qmllint on a project using a different Qt version."));
    parser.addOption(qmlImportNoDefault);
    const QString qmlImportNoDefaultSetting = QLatin1String("DisableDefaultImports");
    defaultSettings.addOption(qmlImportNoDefaultSetting, false);

    QCommandLineOption qmldirFilesOption(
            QStringList() << "i"
                          << "qmltypes",
            QLatin1String("Import the specified qmldir files. By default, the qmldir file found "
                          "in the current directory is used if present. If no qmldir file is found,"
                          "but qmltypes files are, those are imported instead. When this option is "
                          "set, you have to explicitly add the qmldir or any qmltypes files in the "
                          "current directory if you want it to be used. Importing qmltypes files "
                          "without their corresponding qmldir file is inadvisable."),
            QLatin1String("qmldirs"));
    parser.addOption(qmldirFilesOption);
    const QString qmldirFilesSetting = QLatin1String("OverwriteImportTypes");
    defaultSettings.addOption(qmldirFilesSetting);

    QCommandLineOption absolutePath(
            QStringList() << "absolute-path",
            QLatin1String("Use absolute paths for logging instead of relative ones."));
    absolutePath.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(absolutePath);

    QCommandLineOption fixFile(QStringList() << "f"
                                             << "fix",
                               QLatin1String("Automatically apply fix suggestions"));
    parser.addOption(fixFile);

    QCommandLineOption dryRun(QStringList() << "dry-run",
                              QLatin1String("Only print out the contents of the file after fix "
                                            "suggestions without applying them. Also prints the "
                                            "settings file that would be used for this instance."));
    parser.addOption(dryRun);

    QCommandLineOption listPluginsOption(QStringList() << "list-plugins",
                                         QLatin1String("List all available plugins"));
    parser.addOption(listPluginsOption);

    QCommandLineOption pluginsDisable(
            QStringList() << "D"
                          << "disable-plugins",
            QLatin1String("List of qmllint plugins to disable (all to disable all plugins)"),
            QLatin1String("plugins"));
    parser.addOption(pluginsDisable);
    const QString pluginsDisableSetting = QLatin1String("DisablePlugins");
    defaultSettings.addOption(pluginsDisableSetting);

    QCommandLineOption pluginPathsOption(
            QStringList() << "P"
                          << "plugin-paths",
            QLatin1String("Load qmllint plugins from the specified trusted directory"),
            QLatin1String("directory"));
    parser.addOption(pluginPathsOption);

    QCommandLineOption maxWarnings(
            QStringList() << "W"
                          << "max-warnings",
            QLatin1String("Exit with an error code if more than \"count\" many"
                          "warnings are found by qmllint. By default or if \"count\" "
                          "is -1, warnings do not cause qmllint "
                          "to return with an error exit code."),
            "count"
            );
    parser.addOption(maxWarnings);
    const QString maxWarningsSetting = QLatin1String("MaxWarnings");
    defaultSettings.addOption(maxWarningsSetting, -1);

    // QTBUG-135020: don't break existing user configs and still accept PropertyAliasCycles
    defaultSettings.addOption("PropertyAliasCycles"_L1);

    const auto onlyExplicitCategoriesOptionName = "only-explicit-categories"_L1;
    const auto onlyExplicitCategoriesSettingName = "OnlyExplicitCategories"_L1;
    QCommandLineOption onlyExplicitCategoriesOption(
            QStringList() << onlyExplicitCategoriesOptionName,
            "Enable only categories explicitly set on the command line or in the settings file."_L1);
    parser.addOption(onlyExplicitCategoriesOption);
    defaultSettings.addOption(onlyExplicitCategoriesSettingName, false);

    auto addCategory = [&](const QQmlJS::LoggerCategory &category) {
        defaultCategories.push_back(category);

        const QString severity = QQmlJS::LoggingUtils::severityToString(category.severity());
        QCommandLineOption option(category.id().name().toString(),
                                  category.description() + " (default: %1)"_L1.arg(severity),
                                  "severity"_L1, severity);
        parser.addOption(option);
        defaultSettings.addOption("Warnings/"_L1 + category.settingsName(), severity);
    };

    for (const auto &category : QQmlJSLogger::builtinCategories()) {
        addCategory(category);
    }

    parser.addPositionalArgument(QLatin1String("files"),
                                 QLatin1String("list of qml or js files to verify"));

    QStringList arguments;
    if (!argumentsFromCommandLineAndFile(arguments, app.arguments())) {
        // argumentsFromCommandLine already printed any necessary warnings.
        return 1;
    }

    parser.parse(arguments); // parse but ignore unknown options temporarily: plugins might add some
                             // later

    // Since we can't use QCommandLineParser::process(), we need to handle version and help manually
    if (parser.isSet("version"))
        parser.showVersion();

    bool silent = parser.isSet(silentOption);
    bool useAbsolutePath = parser.isSet(absolutePath);
    bool useJson = parser.isSet(jsonOption);

    // use host qml import path as a sane default if not explicitly disabled
    QStringList defaultImportPaths = { QDir::currentPath() };

    if (parser.isSet(resourceOption)) {
        defaultImportPaths.append(QLatin1String(":/qt-project.org/imports"));
        defaultImportPaths.append(QLatin1String(":/qt/qml"));
    };

    defaultImportPaths.append(QLibraryInfo::paths(QLibraryInfo::QmlImportsPath));

    QStringList qmlImportPaths =
            parser.isSet(qmlImportNoDefault) ? QStringList {} : defaultImportPaths;

    QStringList defaultQmldirFiles;
    if (parser.isSet(qmldirFilesOption)) {
        defaultQmldirFiles = QQmlJSUtils::cleanPaths(parser.values(qmldirFilesOption));
    } else if (!parser.isSet(qmlImportNoDefault)){
        // If nothing given explicitly, use the qmldir file from the current directory.
        QFileInfo qmldirFile(QStringLiteral("qmldir"));
        if (qmldirFile.isFile()) {
            defaultQmldirFiles.append(qmldirFile.absoluteFilePath());
        } else {
            // If no qmldir file is found, use the qmltypes files
            // from the current directory for backwards compatibility.
            QDirIterator it(".", {"*.qmltypes"}, QDir::Files);
            while (it.hasNext()) {
                it.next();
                defaultQmldirFiles.append(it.fileInfo().absoluteFilePath());
            }
        }
    }
    QStringList qmldirFiles = defaultQmldirFiles;

    const QStringList defaultResourceFiles =
            parser.isSet(resourceOption) ? parser.values(resourceOption) : QStringList {};
    QStringList resourceFiles = defaultResourceFiles;

    bool success = true;

    QStringList pluginPaths;

    if (parser.isSet(pluginPathsOption))
        pluginPaths << parser.values(pluginPathsOption);

    QQmlJSLinter linter(qmlImportPaths, pluginPaths, useAbsolutePath);

    for (const QQmlJSLinter::Plugin &plugin : linter.plugins()) {
        for (const QQmlJS::LoggerCategory &category : plugin.categories())
            addCategory(category);
    }

    if (parser.isSet(writeDefaultsOption)) {
        return defaultSettings.writeDefaults() ? 0 : 1;
    }

    if (parser.isSet("help") || parser.isSet("help-all"))
        parser.showHelp(0);

    if (!parser.unknownOptionNames().isEmpty())
        parser.process(app);

    if (parser.isSet(listPluginsOption)) {
        const std::vector<QQmlJSLinter::Plugin> &plugins = linter.plugins();
        if (!plugins.empty()) {
            int nameWidth = "Plugin"_L1.size();
            int versionWidth = "Version"_L1.size();
            int authorWidth = "Author"_L1.size();
            for (const auto &p : plugins) {
                nameWidth = qMax(nameWidth, p.name().size());
                versionWidth = qMax(versionWidth, p.version().size());
                authorWidth = qMax(authorWidth, p.author().size());
            }

            // At least 4 spaces between columns
            nameWidth += 4;
            versionWidth += 4;
            authorWidth += 4;

            qInfo().nospace().noquote() << u"Plugin"_s.leftJustified(nameWidth, u' ')
                                        << u"Version"_s.leftJustified(versionWidth, u' ')
                                        << u"Author"_s.leftJustified(authorWidth, u' ')
                                        << u"Description"_s;
            for (const QQmlJSLinter::Plugin &plugin : plugins) {
                qInfo().nospace().noquote()
                        << plugin.name().leftJustified(nameWidth, u' ')
                        << plugin.version().leftJustified(versionWidth, u' ')
                        << plugin.author().leftJustified(authorWidth, u' ')
                        << plugin.description();
            }
        } else {
            qWarning() << "No plugins installed.";
        }
        return 0;
    }

    const auto positionalArguments = [&parser]() {
        auto positionalArguments = parser.positionalArguments();
        auto begin = positionalArguments.begin(), end = positionalArguments.end();
        std::sort(begin, end);
        positionalArguments.erase(std::unique(begin, end), end);
        return positionalArguments;
    }();

    if (positionalArguments.isEmpty()) {
        parser.showHelp(-1);
    }

    if (parser.isSet(dryRun))
        defaultSettings.reportConfigForFiles(positionalArguments);

    const bool isFixing = parser.isSet(fixFile);
    QJsonArray jsonFiles;

    for (const QString &filename : positionalArguments) {
        QQmlToolingSettings settings(QLatin1String("qmllint"),
                                     { QLatin1String("General"), QLatin1String("Warnings") });

        QList<QQmlJS::LoggerCategory> categories = defaultCategories;

        if (!parser.isSet(ignoreSettings)) {
            QQmlToolingSettings::SearchOptions options;
            options.isQmllintSilent = silent;
            settings.search(filename, options);
        }

        const bool onlyExplicitCategories = parser.isSet(onlyExplicitCategoriesOption)
                || (settings.isSet(onlyExplicitCategoriesSettingName)
                    && settings.value(onlyExplicitCategoriesSettingName).toBool());

        const auto categorySelection = onlyExplicitCategories
                ? QQmlJS::LoggingUtils::CategorySelection::Explicit
                : QQmlJS::LoggingUtils::CategorySelection::All;
        QQmlJS::LoggingUtils::updateLogSeverities(categories, settings, &parser, categorySelection);

        resourceFiles = defaultResourceFiles;
        resourceFiles.append(settings.valueAsAbsolutePathList(resourceSetting, filename));

        qmldirFiles = defaultQmldirFiles;
        if (settings.isSet(qmldirFilesSetting)
            && !settings.value(qmldirFilesSetting).toStringList().isEmpty()) {
            qmldirFiles = settings.valueAsAbsolutePathList(qmldirFilesSetting, filename);
        }

        if (parser.isSet(qmlImportNoDefault)
            || (settings.isSet(qmlImportNoDefaultSetting)
                && settings.value(qmlImportNoDefaultSetting).toBool())) {
            qmlImportPaths = {};
        } else {
            qmlImportPaths = defaultImportPaths;
        }

        if (parser.isSet(qmlImportPathsOption))
            qmlImportPaths << parser.values(qmlImportPathsOption);
        if (parser.isSet(environmentOption)) {
            if (silent) {
                qmlImportPaths << qEnvironmentVariable("QML_IMPORT_PATH")
                                          .split(QDir::separator(), Qt::SkipEmptyParts)
                               << qEnvironmentVariable("QML2_IMPORT_PATH")
                                          .split(QDir::separator(), Qt::SkipEmptyParts);
            } else {
                if (const QStringList dirsFromEnv =
                            QQmlToolingUtils::getAndWarnForInvalidDirsFromEnv(u"QML_IMPORT_PATH"_s);
                    !dirsFromEnv.isEmpty()) {
                    qInfo().nospace().noquote()
                            << "Using import directories passed from environment variable "
                               "\"QML_IMPORT_PATH\": \""
                            << dirsFromEnv.join(u"\", \""_s) << "\".";
                    qmlImportPaths << dirsFromEnv;
                }
                if (const QStringList dirsFromEnv =
                            QQmlToolingUtils::getAndWarnForInvalidDirsFromEnv(
                                    u"QML2_IMPORT_PATH"_s);
                    !dirsFromEnv.isEmpty()) {
                    qInfo().nospace().noquote() << "Using import directories passed from the "
                                                   "deprecated environment variable "
                                                   "\"QML2_IMPORT_PATH\": \""
                                                << dirsFromEnv.join(u"\", \""_s) << "\".";
                    qmlImportPaths << dirsFromEnv;
                }
            }
        }

        qmlImportPaths.append(settings.valueAsAbsolutePathList(qmlImportPathsSetting, filename));

        QSet<QString> disabledPlugins;

        if (parser.isSet(pluginsDisable)) {
            for (const QString &plugin : parser.values(pluginsDisable))
                disabledPlugins << plugin.toLower();
        }

        if (settings.isSet(pluginsDisableSetting)) {
            for (const QString &plugin : settings.value(pluginsDisableSetting).toStringList())
                disabledPlugins << plugin.toLower();
        }

        linter.setPluginsEnabled(!disabledPlugins.contains("all"));

        if (!linter.pluginsEnabled())
            continue;

        auto &plugins = linter.plugins();

        for (auto &plugin : plugins)
            plugin.setEnabled(!disabledPlugins.contains(plugin.name().toLower()));

        QQmlJSLinter::Result lintResult;

        if (parser.isSet(moduleOption)) {
            QQmlJSLinter::LintOptions options;
            options.setFlag(QQmlJSLinter::Silent, silent);
            options.setFlag(QQmlJSLinter::GenerateJson, useJson);
            lintResult = linter.lintModule(filename, options, qmlImportPaths, resourceFiles);
            jsonFiles.append(lintResult.json);
            success &= (lintResult.status == QQmlJSLinter::LintSuccess
                        || lintResult.status == QQmlJSLinter::HasWarnings);
            if (success) {
                const qsizetype value = parser.isSet(maxWarnings)
                        ? parser.value(maxWarnings).toInt()
                        : (settings.isSet(maxWarningsSetting)
                                   ? settings.value(maxWarningsSetting).toInt()
                                   : defaultSettings.value(maxWarningsSetting).toInt());
                if (value != -1 && value < lintResult.logger->numWarnings())
                    success = false;
            }

            if (isFixing)
                success &= applyFixes(lintResult, silent, parser.isSet(dryRun));
        } else {
            // collect all filenames and parameters before actually linting the files
            QQmlJSLinter::LintOptions options;
            options.setFlag(QQmlJSLinter::Silent, silent || isFixing);
            options.setFlag(QQmlJSLinter::GenerateJson, useJson);
            linter.prepareFileForBatchLinting(filename, nullptr, options, qmlImportPaths,
                                              qmldirFiles, resourceFiles, categories);
        }
    }
    if (!parser.isSet(moduleOption)) {
        for (const QString &filename : positionalArguments) {
            QQmlJSLinter::Result lintResult = linter.lintFileInBatch(filename);
            jsonFiles.append(lintResult.json);
            success &= (lintResult.status == QQmlJSLinter::LintSuccess
                        || lintResult.status == QQmlJSLinter::HasWarnings);

            if (success) {
                QQmlToolingSettings settings(
                        QLatin1String("qmllint"),
                        { QLatin1String("General"), QLatin1String("Warnings") });
                if (!parser.isSet(ignoreSettings)) {
                    QQmlToolingSettings::SearchOptions options;
                    options.isQmllintSilent = silent;
                    settings.search(filename, options);
                }

                const qsizetype value = parser.isSet(maxWarnings)
                        ? parser.value(maxWarnings).toInt()
                        : (settings.isSet(maxWarningsSetting)
                                   ? settings.value(maxWarningsSetting).toInt()
                                   : defaultSettings.value(maxWarningsSetting).toInt());
                if (value != -1 && value < lintResult.logger->numWarnings())
                    success = false;
            }

            if (isFixing)
                success &= applyFixes(lintResult, silent, parser.isSet(dryRun));
        }
    }

    if (useJson) {
        QJsonObject result;

        result[u"revision"_s] = JSON_LOGGING_FORMAT_REVISION;
        result[u"files"_s] = jsonFiles;

        QString fileName = parser.value(jsonOption);

        const QByteArray json = QJsonDocument(result).toJson(QJsonDocument::Compact);

        if (fileName == u"-") {
            QTextStream(stdout) << QString::fromUtf8(json);
        } else {
            QFile file(fileName);
            if (file.open(QFile::WriteOnly))
                file.write(json);
            else
                success = false;
        }
    }

    return success ? 0 : -1;
}
