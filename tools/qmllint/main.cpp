// Copyright (C) 2016 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sergio Martins <sergio.martins@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>

#include <QtQmlCompiler/private/qqmljsresourcefilemapper_p.h>
#include <QtQmlCompiler/private/qqmljscompiler_p.h>
#include <QtQmlCompiler/private/qqmljslinter_p.h>
#include <QtQmlCompiler/private/qqmljsloggingutils_p.h>

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

constexpr int JSON_LOGGING_FORMAT_REVISION = 3;

int main(int argv, char *argc[])
{
    QHashSeed::setDeterministicGlobalSeed();
    QList<QQmlJS::LoggerCategory> categories;

    QCoreApplication app(argv, argc);
    QCoreApplication::setApplicationName("qmllint");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    QQmlToolingSettings settings(QLatin1String("qmllint"));
    parser.setApplicationDescription(QLatin1String(R"(QML syntax verifier and analyzer

All warnings can be set to three levels:
    disable - Fully disables the warning.
    info - Displays the warning but does not influence the return code.
    warning - Displays the warning and leads to a non-zero exit code if encountered.
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
    settings.addOption(resourceSetting);

    QCommandLineOption qmlImportPathsOption(
            QStringList() << "I"
                          << "qmldirs",
            QLatin1String("Look for QML modules in specified directory"),
            QLatin1String("directory"));
    parser.addOption(qmlImportPathsOption);
    const QString qmlImportPathsSetting = QLatin1String("AdditionalQmlImportPaths");
    settings.addOption(qmlImportPathsSetting);

    QCommandLineOption qmlImportNoDefault(
                QStringList() << "bare",
                QLatin1String("Do not include default import directories or the current directory. "
                              "This may be used to run qmllint on a project using a different Qt version."));
    parser.addOption(qmlImportNoDefault);
    const QString qmlImportNoDefaultSetting = QLatin1String("DisableDefaultImports");
    settings.addOption(qmlImportNoDefaultSetting, false);

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
    settings.addOption(qmldirFilesSetting);

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
                                            "suggestions without applying them"));
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
    settings.addOption(pluginsDisableSetting);

    QCommandLineOption pluginPathsOption(
            QStringList() << "P"
                          << "plugin-paths",
            QLatin1String("Look for qmllint plugins in specified directory"),
            QLatin1String("directory"));
    parser.addOption(pluginPathsOption);

    auto levelToString = [](const QQmlJS::LoggerCategory &category) -> QString {
        Q_ASSERT(category.isIgnored() || category.level() != QtCriticalMsg);
        if (category.isIgnored())
            return QStringLiteral("disable");

        switch (category.level()) {
        case QtInfoMsg:
            return QStringLiteral("info");
        case QtWarningMsg:
            return QStringLiteral("warning");
        default:
            Q_UNREACHABLE();
            break;
        }
    };

    auto addCategory = [&](const QQmlJS::LoggerCategory &category) {
        categories.push_back(category);
        if (category.isDefault())
            return;
        QCommandLineOption option(
                category.id().name().toString(),
                category.description()
                        + QStringLiteral(" (default: %1)").arg(levelToString(category)),
                QStringLiteral("level"), levelToString(category));
        if (category.isIgnored())
            option.setFlags(QCommandLineOption::HiddenFromHelp);
        parser.addOption(option);
        settings.addOption(QStringLiteral("Warnings/") + category.settingsName(),
                           levelToString(category));
    };

    for (const auto &category : QQmlJSLogger::defaultCategories()) {
        addCategory(category);
    }

    parser.addPositionalArgument(QLatin1String("files"),
                                 QLatin1String("list of qml or js files to verify"));
    if (!parser.parse(app.arguments())) {
        if (parser.unknownOptionNames().isEmpty()) {
            qWarning().noquote() << parser.errorText();
            return 1;
        }
    }

    // Since we can't use QCommandLineParser::process(), we need to handle version and help manually
    if (parser.isSet("version"))
        parser.showVersion();

    if (parser.isSet("help") || parser.isSet("help-all"))
        parser.showHelp(0);

    if (parser.isSet(writeDefaultsOption)) {
        return settings.writeDefaults() ? 0 : 1;
    }

    auto updateLogLevels = [&]() {
        for (auto &category : categories) {
            if (category.isDefault())
                continue;

            const QString &key = category.id().name().toString();
            const QString &settingsName = QStringLiteral("Warnings/") + category.settingsName();
            if (parser.isSet(key) || settings.isSet(settingsName)) {
                const QString value = parser.isSet(key) ? parser.value(key)
                                                        : settings.value(settingsName).toString();

                // Do not try to set the levels if it's due to a default config option.
                // This way we can tell which options have actually been overwritten by the user.
                if (levelToString(category) == value && !parser.isSet(key))
                    continue;

                if (value == "disable"_L1) {
                    category.setLevel(QtCriticalMsg);
                    category.setIgnored(true);
                } else if (value == "info"_L1) {
                    category.setLevel(QtInfoMsg);
                    category.setIgnored(false);
                } else if (value == "warning"_L1) {
                    category.setLevel(QtWarningMsg);
                    category.setIgnored(false);
                } else {
                    qWarning() << "Invalid logging level" << value << "provided for"
                               << category.id().name().toString()
                               << "(allowed are: disable, info, warning)";
                    parser.showHelp(-1);
                }
            }
        }
    };

    bool silent = parser.isSet(silentOption);
    bool useAbsolutePath = parser.isSet(absolutePath);
    bool useJson = parser.isSet(jsonOption);

    // use host qml import path as a sane default if not explicitly disabled
    QStringList defaultImportPaths = { QDir::currentPath() };

    if (parser.isSet(resourceOption)) {
        defaultImportPaths.append(QLatin1String(":/qt-project.org/imports"));
        defaultImportPaths.append(QLatin1String(":/qt/qml"));
    };

    defaultImportPaths.append(QLibraryInfo::path(QLibraryInfo::QmlImportsPath));

    QStringList qmlImportPaths =
            parser.isSet(qmlImportNoDefault) ? QStringList {} : defaultImportPaths;

    QStringList defaultQmldirFiles;
    if (parser.isSet(qmldirFilesOption)) {
        defaultQmldirFiles = parser.values(qmldirFilesOption);
    } else {
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

    QStringList pluginPaths = { QQmlJSLinter::defaultPluginPath() };

    if (parser.isSet(pluginPathsOption))
        pluginPaths << parser.values(pluginPathsOption);

    QQmlJSLinter linter(qmlImportPaths, pluginPaths, useAbsolutePath);

    for (const QQmlJSLinter::Plugin &plugin : linter.plugins()) {
        for (const QQmlJS::LoggerCategory &category : plugin.categories())
            addCategory(category);
    }

    if (!parser.unknownOptionNames().isEmpty())
        parser.process(app);

    updateLogLevels();

    if (parser.isSet(listPluginsOption)) {
        const std::vector<QQmlJSLinter::Plugin> &plugins = linter.plugins();
        if (!plugins.empty()) {
            qInfo().nospace().noquote() << "Plugin\t\t\tBuilt-in?\tVersion\tAuthor\t\tDescription";
            for (const QQmlJSLinter::Plugin &plugin : plugins) {
                qInfo().nospace().noquote()
                        << plugin.name() << "\t\t\t" << (plugin.isBuiltin() ? "Yes" : "No")
                        << "\t\t" << plugin.version() << "\t" << plugin.author() << "\t\t"
                        << plugin.description();
            }
        } else {
            qWarning() << "No plugins installed.";
        }
        return 0;
    }

    const auto positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        parser.showHelp(-1);
    }

    QJsonArray jsonFiles;

    for (const QString &filename : positionalArguments) {
        if (!parser.isSet(ignoreSettings)) {
            settings.search(filename);
            updateLogLevels();

            const QDir fileDir = QFileInfo(filename).absoluteDir();
            auto addAbsolutePaths = [&](QStringList &list, const QStringList &entries) {
                for (const QString &file : entries)
                    list << (QFileInfo(file).isAbsolute() ? file : fileDir.filePath(file));
            };

            resourceFiles = defaultResourceFiles;

            addAbsolutePaths(resourceFiles, settings.value(resourceSetting).toStringList());

            qmldirFiles = defaultQmldirFiles;
            if (settings.isSet(qmldirFilesSetting)
                && !settings.value(qmldirFilesSetting).toStringList().isEmpty()) {
                qmldirFiles = {};
                addAbsolutePaths(qmldirFiles,
                                 settings.value(qmldirFilesSetting).toStringList());
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

            addAbsolutePaths(qmlImportPaths, settings.value(qmlImportPathsSetting).toStringList());

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
        }

        const bool isFixing = parser.isSet(fixFile);

        QQmlJSLinter::LintResult lintResult;

        if (parser.isSet(moduleOption)) {
            lintResult = linter.lintModule(filename, silent, useJson ? &jsonFiles : nullptr,
                                           qmlImportPaths, resourceFiles);
        } else {
            lintResult = linter.lintFile(filename, nullptr, silent || isFixing,
                                         useJson ? &jsonFiles : nullptr, qmlImportPaths,
                                         qmldirFiles, resourceFiles, categories);
        }
        success &= (lintResult == QQmlJSLinter::LintSuccess);

        if (isFixing) {
            if (lintResult != QQmlJSLinter::LintSuccess && lintResult != QQmlJSLinter::HasWarnings)
                continue;

            QString fixedCode;
            const QQmlJSLinter::FixResult result = linter.applyFixes(&fixedCode, silent);

            if (result != QQmlJSLinter::NothingToFix && result != QQmlJSLinter::FixSuccess) {
                success = false;
                continue;
            }

            if (parser.isSet(dryRun)) {
                QTextStream(stdout) << fixedCode;
            } else {
                if (result == QQmlJSLinter::NothingToFix) {
                    if (!silent)
                        qWarning().nospace() << "Nothing to fix in " << filename;
                    continue;
                }

                const QString backupFile = filename + u".bak"_s;
                if (QFile::exists(backupFile) && !QFile::remove(backupFile)) {
                    if (!silent) {
                        qWarning().nospace() << "Failed to remove old backup file " << backupFile
                                             << ", aborting";
                    }
                    success = false;
                    continue;
                }
                if (!QFile::copy(filename, backupFile)) {
                    if (!silent) {
                        qWarning().nospace()
                                << "Failed to create backup file " << backupFile << ", aborting";
                    }
                    success = false;
                    continue;
                }

                QFile file(filename);
                if (!file.open(QIODevice::WriteOnly)) {
                    if (!silent) {
                        qWarning().nospace() << "Failed to open " << filename
                                             << " for writing:" << file.errorString();
                    }
                    success = false;
                    continue;
                }

                const QByteArray data = fixedCode.toUtf8();
                if (file.write(data) != data.size()) {
                    if (!silent) {
                        qWarning().nospace() << "Failed to write new contents to " << filename
                                             << ": " << file.errorString();
                    }
                    success = false;
                    continue;
                }
                if (!silent) {
                    qDebug().nospace() << "Applied fixes to " << filename << ". Backup created at "
                                       << backupFile;
                }
            }
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
            file.open(QFile::WriteOnly);
            file.write(json);
        }
    }

    return success ? 0 : -1;
}
