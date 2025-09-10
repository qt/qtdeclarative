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


using namespace QQmlJS::Dom;

struct Options
{
    bool verbose = false;
    bool inplace = false;
    bool force = false;
    bool tabs = false;
    bool valid = false;
    bool normalize = false;
    bool ignoreSettings = false;
    bool writeDefaultSettings = false;
    bool objectsSpacing = false;
    bool functionsSpacing = false;

    int indentWidth = 4;
    bool indentWidthSet = false;
    QString newline = "native";

    QStringList files;
    QStringList arguments;
    QStringList errors;
};

// TODO refactor
// Move out to the LineWriterOptions class / helper
static LineWriterOptions composeLwOptions(const Options &options, QStringView code)
{
    LineWriterOptions lwOptions;
    lwOptions.formatOptions.indentSize = options.indentWidth;
    lwOptions.formatOptions.useTabs = options.tabs;
    lwOptions.updateOptions = LineWriterOptions::Update::None;
    if (options.newline == "native") {
        // find out current line endings...
        int newlineIndex = code.indexOf(QChar(u'\n'));
        int crIndex = code.indexOf(QChar(u'\r'));
        if (newlineIndex >= 0) {
            if (crIndex >= 0) {
                if (crIndex + 1 == newlineIndex)
                    lwOptions.lineEndings = LineWriterOptions::LineEndings::Windows;
                else
                    qWarning().noquote() << "Invalid line ending in file, using default";

            } else {
                lwOptions.lineEndings = LineWriterOptions::LineEndings::Unix;
            }
        } else if (crIndex >= 0) {
            lwOptions.lineEndings = LineWriterOptions::LineEndings::OldMacOs;
        } else {
            qWarning().noquote() << "Unknown line ending in file, using default";
        }
    } else if (options.newline == "macos") {
        lwOptions.lineEndings = LineWriterOptions::LineEndings::OldMacOs;
    } else if (options.newline == "windows") {
        lwOptions.lineEndings = LineWriterOptions::LineEndings::Windows;
    } else if (options.newline == "unix") {
        lwOptions.lineEndings = LineWriterOptions::LineEndings::Unix;
    } else {
        qWarning().noquote() << "Unknown line ending type" << options.newline << ", using default";
    }

    if (options.normalize)
        lwOptions.attributesSequence = LineWriterOptions::AttributesSequence::Normalize;
    else
        lwOptions.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;

    lwOptions.objectsSpacing = options.objectsSpacing;
    lwOptions.functionsSpacing = options.functionsSpacing;
    return lwOptions;
}

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

static bool parseFile(const QString &filename, const Options &options)
{
    const auto [fileItem, validFile] = parse(filename);
    if (!validFile) {
        logParsingErrors(fileItem, filename);
        return false;
    }

    // Turn AST back into source code
    if (options.verbose)
        qWarning().noquote() << "Dumping" << filename;

    const auto &code = getFileItemOwner(fileItem)->code();
    auto lwOptions = composeLwOptions(options, code);
    WriteOutChecks checks = WriteOutCheck::Default;
    //Disable writeOutChecks for some usecases
    if (options.force ||
        code.size() > 32000 ||
        fileItem.internalKind() == DomType::JsFile) {
        checks = WriteOutCheck::None;
    }

    bool res = false;
    if (options.inplace) {
        if (options.verbose)
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

Options buildCommandLineOptions(const QCoreApplication &app)
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

    QCommandLineOption filesOption(
            { "F"_L1, "files"_L1 }, "Format all files listed in file, in-place"_L1, "file"_L1);
    parser.addOption(filesOption);

    parser.addOption(QCommandLineOption(
            { "l", "newline" },
            QStringLiteral("Override the new line format to use (native macos unix windows)."),
            "newline", "native"));

    parser.addOption(QCommandLineOption(QStringList() << "objects-spacing", QStringLiteral("Ensure spaces between objects (only works with normalize option).")));

    parser.addOption(QCommandLineOption(QStringList() << "functions-spacing", QStringLiteral("Ensure spaces between functions (only works with normalize option).")));

    parser.addPositionalArgument("filenames", "files to be processed by qmlformat");

    parser.process(app);

    if (parser.isSet(writeDefaultsOption)) {
        Options options;
        options.writeDefaultSettings = true;
        options.valid = true;
        return options;
    }

    if (parser.positionalArguments().empty() && !parser.isSet(filesOption)) {
        Options options;
        options.errors.push_back("Error: Expected at least one input file."_L1);
        return options;
    }

    bool indentWidthOkay = false;
    const int indentWidth = parser.value("indent-width").toInt(&indentWidthOkay);
    if (!indentWidthOkay) {
        Options options;
        options.errors.push_back("Error: Invalid value passed to -w");
        return options;
    }

    QStringList files;
    if (!parser.value("files"_L1).isEmpty()) {
        const QString path = parser.value("files"_L1);
        QFile file(path);
        if (!file.open(QIODevice::Text | QIODevice::ReadOnly)) {
            Options options;
            options.errors.push_back(
                    "Error: Could not open file \"" + path + "\" for option -F."_L1);
            return options;
        }

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString file = in.readLine();

            if (file.isEmpty())
                continue;

            files.push_back(file);
        }

        if (files.isEmpty()) {
            Options options;
            options.errors.push_back("Error: File \""_L1 + path + "\" for option -F is empty."_L1);
            return options;
        }

        for (const auto &file : std::as_const(files)) {
            if (!QFile::exists(file)) {
                Options options;
                options.errors.push_back("Error: Entry \"" + file + "\" of file \"" + path
                                         + "\" passed to option -F could not be found.");
                return options;
            }
        }
    } else {
        const auto &args = parser.positionalArguments();
        for (const auto &file : args) {
            if (!QFile::exists(file)) {
                Options options;
                options.errors.push_back("Error: Could not find file \"" + file + "\".");
                return options;
            }
        }
    }

    Options options;
    options.verbose = parser.isSet("verbose");
    options.inplace = parser.isSet("inplace");
    options.force = parser.isSet("force");
    options.tabs = parser.isSet("tabs");
    options.normalize = parser.isSet("normalize");
    options.ignoreSettings = parser.isSet("ignore-settings");
    options.objectsSpacing = parser.isSet("objects-spacing");
    options.functionsSpacing = parser.isSet("functions-spacing");
    options.valid = true;

    options.indentWidth = indentWidth;
    options.indentWidthSet = parser.isSet("indent-width");
    options.newline = parser.value("newline");
    options.files = files;
    options.arguments = parser.positionalArguments();
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

    QQmlToolingSettings settings(QLatin1String("qmlformat"));

    const QString &useTabsSetting = QStringLiteral("UseTabs");
    settings.addOption(useTabsSetting);

    const QString &indentWidthSetting = QStringLiteral("IndentWidth");
    settings.addOption(indentWidthSetting, 4);

    const QString &normalizeSetting = QStringLiteral("NormalizeOrder");
    settings.addOption(normalizeSetting);

    const QString &newlineSetting = QStringLiteral("NewlineType");
    settings.addOption(newlineSetting, QStringLiteral("native"));

    const QString &objectsSpacingSetting = QStringLiteral("ObjectsSpacing");
    settings.addOption(objectsSpacingSetting);

    const QString &functionsSpacingSetting = QStringLiteral("FunctionsSpacing");
    settings.addOption(functionsSpacingSetting);

    const auto options = buildCommandLineOptions(app);
    if (!options.valid) {
        for (const auto &error : options.errors) {
            qWarning().noquote() << error;
        }

        return -1;
    }

    if (options.writeDefaultSettings)
        return settings.writeDefaults() ? 0 : -1;

    auto getSettings = [&](const QString &file, Options options) {
        // Perform formatting inplace if --files option is set.
        if (!options.files.isEmpty())
            options.inplace = true;

        if (options.ignoreSettings || !settings.search(file))
            return options;

        Options perFileOptions = options;

        // Allow for tab settings to be overwritten by the command line
        if (!options.indentWidthSet) {
            if (settings.isSet(indentWidthSetting))
                perFileOptions.indentWidth = settings.value(indentWidthSetting).toInt();
            if (settings.isSet(useTabsSetting))
                perFileOptions.tabs = settings.value(useTabsSetting).toBool();
        }

        if (settings.isSet(normalizeSetting))
            perFileOptions.normalize = settings.value(normalizeSetting).toBool();

        if (settings.isSet(newlineSetting))
            perFileOptions.newline = settings.value(newlineSetting).toString();

        if (settings.isSet(objectsSpacingSetting))
            perFileOptions.objectsSpacing = settings.value(objectsSpacingSetting).toBool();

        if (settings.isSet(functionsSpacingSetting))
            perFileOptions.functionsSpacing = settings.value(functionsSpacingSetting).toBool();

        return perFileOptions;
    };

    bool success = true;
    if (!options.files.isEmpty()) {
        if (!options.arguments.isEmpty())
            qWarning() << "Warning: Positional arguments are ignored when -F is used";

        for (const QString &file : options.files) {
            Q_ASSERT(!file.isEmpty());

            if (!parseFile(file, getSettings(file, options)))
                success = false;
        }
    } else {
        for (const QString &file : options.arguments) {
            if (!parseFile(file, getSettings(file, options)))
                success = false;
        }
    }

    return success ? 0 : 1;
}
