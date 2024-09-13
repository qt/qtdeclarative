// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>

#if QT_CONFIG(commandlineparser)
#    include <QCommandLineParser>
#endif

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtQmlFormat/private/qqmlformatsettings_p.h>
#include <QtQmlFormat/private/qqmlformatoptions_p.h>

using namespace QQmlJS::Dom;

static void logParsingErrors(const DomItem &fileItem, const QString &filename)
{
    fileItem.iterateErrors(
            [](const DomItem &, const ErrorMessage &msg) {
                errorToQDebug(msg);
                return true;
            },
            true);
    qWarning().noquote() << "Failed to parse" << filename;
}

// TODO
// refactor this workaround. ExternalOWningItem is not recognized as an owning type
// in ownerAs.
static std::shared_ptr<ExternalOwningItem> getFileItemOwner(const DomItem &fileItem)
{
    std::shared_ptr<ExternalOwningItem> filePtr = nullptr;
    switch (fileItem.internalKind()) {
    case DomType::JsFile:
        filePtr = fileItem.ownerAs<JsFile>();
        break;
    default:
        filePtr = fileItem.ownerAs<QmlFile>();
        break;
    }
    return filePtr;
}

// TODO refactor
// Introduce better encapsulation and separation of concerns and move to DOM API
// returns a DomItem corresponding to the loaded file and bool indicating the validity of the file
static std::pair<DomItem, bool> parse(const QString &filename)
{
    auto envPtr =
            DomEnvironment::create(QStringList(),
                                   QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                                           | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);
    // placeholder for a node
    // containing metadata (ExternalItemInfo) about the loaded file
    DomItem fMetadataItem;
    envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, filename),
                     // callback called when everything is loaded that receives the
                     // loaded external file pair (path, oldValue, newValue)
                     [&fMetadataItem](Path, const DomItem &, const DomItem &extItemInfo) {
                         fMetadataItem = extItemInfo;
                     });
    auto fItem = fMetadataItem.fileObject();
    auto filePtr = getFileItemOwner(fItem);
    return { fItem, filePtr && filePtr->isValid() };
}

static bool parseFile(const QString &filename, const QQmlFormatOptions &options)
{
    const auto [fileItem, validFile] = parse(filename);
    if (!validFile) {
        logParsingErrors(fileItem, filename);
        return false;
    }

    // Turn AST back into source code
    if (options.isVerbose())
        qWarning().noquote() << "Dumping" << filename;

    const auto &code = getFileItemOwner(fileItem)->code();
    auto lwOptions = options.optionsForCode(code);
    WriteOutChecks checks = WriteOutCheck::Default;
    //Disable writeOutChecks for some usecases
    if (options.forceEnabled() ||
        code.size() > 32000 ||
        fileItem.internalKind() == DomType::JsFile) {
        checks = WriteOutCheck::None;
    }

    bool res = false;
    if (options.isInplace()) {
        if (options.isVerbose())
            qWarning().noquote() << "Writing to file" << filename;
        FileWriter fw;
        const unsigned numberOfBackupFiles = 0;
        res = fileItem.writeOut(filename, numberOfBackupFiles, lwOptions, &fw, checks);
    } else {
        QFile out;
        if (out.open(stdout, QIODevice::WriteOnly)) {
            LineWriter lw([&out](QStringView s) { out.write(s.toUtf8()); }, filename, lwOptions);
            OutWriter ow(lw);
            res = fileItem.writeOutForFile(ow, checks);
            ow.flush();
        } else {
            res = false;
        }
    }
    return res;
}

QQmlFormatOptions buildCommandLineOptions(const QCoreApplication &app)
{
#if QT_CONFIG(commandlineparser)
    QCommandLineParser parser;
    parser.setApplicationDescription("Formats QML files according to the QML Coding Conventions.");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption(
            QCommandLineOption({ "V", "verbose" },
                               QStringLiteral("Verbose mode. Outputs more detailed information.")));

    QCommandLineOption writeDefaultsOption(
            QStringList() << "write-defaults",
            QLatin1String("Writes defaults settings to .qmlformat.ini and exits (Warning: This "
                          "will overwrite any existing settings and comments!)"));
    parser.addOption(writeDefaultsOption);

    QCommandLineOption ignoreSettings(QStringList() << "ignore-settings",
                                      QLatin1String("Ignores all settings files and only takes "
                                                    "command line options into consideration"));
    parser.addOption(ignoreSettings);

    parser.addOption(QCommandLineOption(
            { "i", "inplace" },
            QStringLiteral("Edit file in-place instead of outputting to stdout.")));

    parser.addOption(QCommandLineOption({ "f", "force" },
                                        QStringLiteral("Continue even if an error has occurred.")));

    parser.addOption(
            QCommandLineOption({ "t", "tabs" }, QStringLiteral("Use tabs instead of spaces.")));

    parser.addOption(QCommandLineOption({ "w", "indent-width" },
                                        QStringLiteral("How many spaces are used when indenting."),
                                        "width", "4"));

    parser.addOption(QCommandLineOption({ "n", "normalize" },
                                        QStringLiteral("Reorders the attributes of the objects "
                                                       "according to the QML Coding Guidelines.")));

    parser.addOption(QCommandLineOption(
            { "F", "files" }, QStringLiteral("Format all files listed in file, in-place"), "file"));

    parser.addOption(QCommandLineOption(
            { "l", "newline" },
            QStringLiteral("Override the new line format to use (native macos unix windows)."),
            "newline", "native"));

    parser.addOption(QCommandLineOption(QStringList() << "objects-spacing", QStringLiteral("Ensure spaces between objects (only works with normalize option).")));

    parser.addOption(QCommandLineOption(QStringList() << "functions-spacing", QStringLiteral("Ensure spaces between functions (only works with normalize option).")));

    parser.addPositionalArgument("filenames", "files to be processed by qmlformat");

    parser.process(app);

    if (parser.isSet(writeDefaultsOption)) {
        QQmlFormatOptions options;
        options.setWriteDefaultSettingsEnabled(true);
        options.setIsValid(true);
        return options;
    }

    bool indentWidthOkay = false;
    const int indentWidth = parser.value("indent-width").toInt(&indentWidthOkay);
    if (!indentWidthOkay) {
        QQmlFormatOptions options;
        options.addError("Error: Invalid value passed to -w");
        return options;
    }

    QStringList files;
    if (!parser.value("files").isEmpty()) {
        QFile file(parser.value("files"));
        if (file.open(QIODevice::Text | QIODevice::ReadOnly)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString file = in.readLine();

                if (file.isEmpty())
                    continue;

                files.push_back(file);
            }
        }
    }

    QQmlFormatOptions options;
    options.setIsVerbose(parser.isSet("verbose"));
    options.setIsInplace(parser.isSet("inplace"));
    options.setForceEnabled(parser.isSet("force"));
    options.setTabsEnabled(parser.isSet("tabs"));
    options.setIgnoreSettingsEnabled(parser.isSet("ignore-settings"));
    options.setNormalizeEnabled(parser.isSet("normalize"));
    options.setObjectsSpacing(parser.isSet("objects-spacing"));
    options.setFunctionsSpacing(parser.isSet("functions-spacing"));
    options.setIsValid(true);

    options.setIndentWidth(indentWidth);
    options.setIndentWidthSet(parser.isSet("indent-width"));
    options.setNewline(QQmlFormatOptions::parseEndings(parser.value("newline"))); // TODO
    options.setFiles(files);
    options.setArguments(parser.positionalArguments());
    return options;
#else
    return Options {};
#endif
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("qmlformat");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QQmlFormatSettings settings(QLatin1String("qmlformat"));

    const auto options = buildCommandLineOptions(app);
    if (!options.isValid()) {
        for (const auto &error : options.errors()) {
            qWarning().noquote() << error;
        }

        return -1;
    }

    if (options.writeDefaultSettingsEnabled())
        return settings.writeDefaults() ? 0 : -1;

    auto getSettings = [&](const QString &file, QQmlFormatOptions options) {
        // Perform formatting inplace if --files option is set.
        if (!options.files().isEmpty())
            options.setIsInplace(true);

        if (options.ignoreSettingsEnabled() || !settings.search(file))
            return options;

        QQmlFormatOptions perFileOptions = options;

        perFileOptions.applySettings(settings);

        return perFileOptions;
    };

    bool success = true;
    if (!options.files().isEmpty()) {
        if (!options.arguments().isEmpty())
            qWarning() << "Warning: Positional arguments are ignored when -F is used";

        for (const QString &file : options.files()) {
            Q_ASSERT(!file.isEmpty());

            if (!parseFile(file, getSettings(file, options)))
                success = false;
        }
    } else {
        for (const QString &file : options.arguments()) {
            if (!parseFile(file, getSettings(file, options)))
                success = false;
        }
    }

    return success ? 0 : 1;
}
