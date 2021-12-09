/****************************************************************************
**
** Copyright (C) 2016 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sergio Martins <sergio.martins@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "../shared/qqmltoolingsettings.h"

#include <QtQmlLint/private/qqmllinter_p.h>

#include <QtQmlCompiler/private/qqmljsresourcefilemapper_p.h>
#include <QtQmlCompiler/private/qqmljscompiler_p.h>

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

constexpr int JSON_LOGGING_FORMAT_REVISION = 3;

int main(int argv, char *argc[])
{
    qSetGlobalQHashSeed(0);
    QMap<QString, QQmlJSLogger::Option> options = QQmlJSLogger::options();

    QCoreApplication app(argv, argc);
    QCoreApplication::setApplicationName("qmllint");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
#if QT_CONFIG(commandlineparser)
    QCommandLineParser parser;
    QQmlToolingSettings settings(QLatin1String("qmllint"));
    parser.setApplicationDescription(QLatin1String(R"(QML syntax verifier and analyzer

All warnings can be set to three levels:
    disable - Fully disables the warning.
    info - Displays the warning but does not influence the return code.
    warning - Displays the warning and leads to a non-zero exit code if encountered.
)"));

    for (auto it = options.cbegin(); it != options.cend(); ++it) {
        QCommandLineOption option(
                it.key(),
                it.value().m_description
                        + QStringLiteral(" (default: %1)").arg(it.value().levelToString()),
                QStringLiteral("level"), it.value().levelToString());
        parser.addOption(option);
        settings.addOption(QStringLiteral("Warnings/") + it.value().m_settingsName,
                           it.value().levelToString());
    }

    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption silentOption(QStringList() << "s" << "silent",
                                    QLatin1String("Don't output syntax errors"));
    parser.addOption(silentOption);

    QCommandLineOption jsonOption(QStringList() << "json",
                                  QLatin1String("Output linting errors as JSON"));
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

    parser.addPositionalArgument(QLatin1String("files"),
                                 QLatin1String("list of qml or js files to verify"));

    parser.process(app);

    if (parser.isSet(writeDefaultsOption)) {
        return settings.writeDefaults() ? 0 : 1;
    }

    auto updateLogLevels = [&]() {
        for (auto it = options.begin(); it != options.end(); ++it) {
            const QString &key = it.key();
            const QString &settingsName = QStringLiteral("Warnings/") + it.value().m_settingsName;
            if (parser.isSet(key) || settings.isSet(settingsName)) {
                const QString value = parser.isSet(key) ? parser.value(key)
                                                        : settings.value(settingsName).toString();
                auto &option = it.value();
                if (!option.setLevel(value)) {
                    qWarning() << "Invalid logging level" << value << "provided for" << it.key()
                               << "(allowed are: disable, info, warning)";
                    parser.showHelp(-1);
                }
            }
        }
    };

    updateLogLevels();

    const auto positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        parser.showHelp(-1);
    }

    bool silent = parser.isSet(silentOption);
    bool useAbsolutePath = parser.isSet(absolutePath);
    bool useJson = parser.isSet(jsonOption);

    // use host qml import path as a sane default if not explicitly disabled
    QStringList defaultImportPaths =
            QStringList { QLibraryInfo::path(QLibraryInfo::QmlImportsPath), QDir::currentPath() };

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

#else
    bool silent = false;
    bool useAbsolutePaths = false;
    bool useJson = false;
    bool warnUnqualified = true;
    bool warnWithStatement = true;
    bool warnInheritanceCycle = true;
    QStringList qmlImportPaths {};
    QStringList qmltypesFiles {};
    QStringList resourceFiles {};
#endif
    bool success = true;
    QQmlLinter linter(qmlImportPaths, useAbsolutePath);

    QJsonArray jsonFiles;

#if QT_CONFIG(commandlineparser)
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
        }
#else
    const auto arguments = app.arguments();
    for (const QString &filename : arguments) {
#endif
        success &= linter.lintFile(filename, nullptr, silent, useJson ? &jsonFiles : nullptr,
                                   qmlImportPaths, qmldirFiles, resourceFiles, options);
    }

    if (useJson) {
        QJsonObject result;

        result[u"revision"_qs] = JSON_LOGGING_FORMAT_REVISION;
        result[u"files"_qs] = jsonFiles;

        QTextStream(stdout) << QString::fromUtf8(
                QJsonDocument(result).toJson(QJsonDocument::Compact));
    }

    return success ? 0 : -1;
}
