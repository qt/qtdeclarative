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

enum QQmlFormatOptionLineEndings {
    Native,
    Windows,
    Unix,
    OldMacOs,
};

class QQmlFormatSettings : public QQmlToolingSettings
{
public:
    QQmlFormatSettings(const QString &toolName = u"qmlformat"_s);
    static const inline QLatin1StringView s_useTabsSetting = QLatin1String("UseTabs");
    static const inline QLatin1StringView s_indentWidthSetting = QLatin1String("IndentWidth");
    static const inline QLatin1StringView s_normalizeSetting = QLatin1String("NormalizeOrder");
    static const inline QLatin1StringView s_newlineSetting = QLatin1String("NewlineType");
    static const inline QLatin1StringView s_objectsSpacingSetting = QLatin1String("ObjectsSpacing");
    static const inline QLatin1StringView s_functionsSpacingSetting = QLatin1String("FunctionsSpacing");
};

QQmlFormatSettings::QQmlFormatSettings(const QString &toolName) : QQmlToolingSettings(toolName)
{
    addOption(s_useTabsSetting);
    addOption(s_indentWidthSetting, 4);
    addOption(s_normalizeSetting);
    addOption(s_newlineSetting, QStringLiteral("native"));
    addOption(s_objectsSpacingSetting);
    addOption(s_functionsSpacingSetting);
}

class QQmlFormatOptions
{
public:
    QQmlFormatOptions();
    using LineEndings = QQmlJS::Dom::LineWriterOptions::LineEndings;
    using AttributesSequence = QQmlJS::Dom::LineWriterOptions::AttributesSequence;
    static LineEndings detectLineEndings(const QString &code);
    static LineEndings lineEndings(QQmlFormatOptionLineEndings endings, const QString &code)
    {
        switch (endings) {
        case Native:
            return detectLineEndings(code);
        case OldMacOs:
            return LineEndings::OldMacOs;
        case Windows:
            return LineEndings::Windows;
        case Unix:
            return LineEndings::Unix;
        }
        Q_UNREACHABLE_RETURN(LineEndings::Unix);
    }
    bool tabsEnabled() const { return m_options.formatOptions.useTabs; }
    void setTabsEnabled(bool tabs) { m_options.formatOptions.useTabs = tabs; }
    bool normalizeEnabled() const
    {
        return m_options.attributesSequence == AttributesSequence::Normalize;
    }
    void setNormalizeEnabled(bool normalize)
    {
        m_options.attributesSequence =
                (normalize ? AttributesSequence::Normalize : AttributesSequence::Preserve);
    }
    bool objectsSpacing() const { return m_options.objectsSpacing; }
    void setObjectsSpacing(bool spacing) { m_options.objectsSpacing = spacing; }
    bool functionsSpacing() const { return m_options.functionsSpacing; }
    void setFunctionsSpacing(bool spacing) { m_options.functionsSpacing = spacing; }
    int indentWidth() const { return m_options.formatOptions.indentSize; }
    void setIndentWidth(int width) { m_options.formatOptions.indentSize = width; }
    QQmlJS::Dom::LineWriterOptions optionsForCode(const QString &code) const
    {
        QQmlJS::Dom::LineWriterOptions result = m_options;
        result.lineEndings = lineEndings(m_newline, code);
        return result;
    }
    static QQmlFormatOptionLineEndings parseEndings(const QString &endings);
    QQmlFormatOptionLineEndings newline() const { return m_newline; }
    void setNewline(const QQmlFormatOptionLineEndings &endings) { m_newline = endings; }
    QStringList files() const { return m_files; }
    void setFiles(const QStringList &newFiles) { m_files = newFiles; }
    QStringList arguments() const { return m_arguments; }
    void setArguments(const QStringList &newArguments) { m_arguments = newArguments; }
    bool isVerbose() const { return m_verbose; }
    void setIsVerbose(bool newVerbose) { m_verbose = newVerbose; }
    bool isValid() const { return m_valid; }
    void setIsValid(bool newValid) { m_valid = newValid; }
    bool isInplace() const { return m_inplace; }
    void setIsInplace(bool newInplace) { m_inplace = newInplace; }
    bool forceEnabled() const { return m_force; }
    void setForceEnabled(bool newForce) { m_force = newForce; }
    bool ignoreSettingsEnabled() const { return m_ignoreSettings; }
    void setIgnoreSettingsEnabled(bool newIgnoreSettings) { m_ignoreSettings = newIgnoreSettings; }
    bool writeDefaultSettingsEnabled() const { return m_writeDefaultSettings; }
    void setWriteDefaultSettingsEnabled(bool newWriteDefaultSettings)
    {
        m_writeDefaultSettings = newWriteDefaultSettings;
    }
    bool indentWidthSet() const { return m_indentWidthSet; }
    void setIndentWidthSet(bool newIndentWidthSet) { m_indentWidthSet = newIndentWidthSet; }
    QStringList errors() const { return m_errors; }
    void addError(const QString &newError) { m_errors.append(newError); };
    void applySettings(const QQmlFormatSettings &settings);

private:
    QQmlJS::Dom::LineWriterOptions m_options;
    QQmlFormatOptionLineEndings m_newline = Native;
    QStringList m_files;
    QStringList m_arguments;
    QStringList m_errors;
    bool m_verbose = false;
    bool m_valid = false;
    bool m_inplace = false;
    bool m_force = false;
    bool m_ignoreSettings = false;
    bool m_writeDefaultSettings = false;
    bool m_indentWidthSet = false;
};

QQmlFormatOptions::QQmlFormatOptions()
{
    m_options.updateOptions = QQmlJS::Dom::LineWriterOptions::Update::None;
    setTabsEnabled(false);
    setNormalizeEnabled(false);
    setObjectsSpacing(false);
    setFunctionsSpacing(false);
    setIndentWidth(4);
}

QQmlFormatOptions::LineEndings QQmlFormatOptions::detectLineEndings(const QString &code)
{
    const QQmlJS::Dom::LineWriterOptions::LineEndings defaultEndings =
#if defined(Q_OS_WIN)
            LineEndings::Windows;
#else
            LineEndings::Unix;
#endif
    // find out current line endings...
    int newlineIndex = code.indexOf(QChar(u'\n'));
    int crIndex = code.indexOf(QChar(u'\r'));
    if (newlineIndex >= 0) {
        if (crIndex >= 0) {
            if (crIndex + 1 == newlineIndex)
                return LineEndings::Windows;
            qWarning().noquote() << "Invalid line ending in file, using default";
            return defaultEndings;
        }
        return LineEndings::Unix;
    }
    if (crIndex >= 0) {
        return LineEndings::OldMacOs;
    }
    qWarning().noquote() << "Unknown line ending in file, using default";
    return defaultEndings;
}

QQmlFormatOptionLineEndings QQmlFormatOptions::parseEndings(const QString &endings)
{
    if (endings == u"unix")
        return Unix;
    if (endings == u"windows")
        return Windows;
    if (endings == u"macos")
        return OldMacOs;
    if (endings == u"native")
        return Native;
    qWarning().noquote() << "Unknown line ending type" << endings << ", using default";
#if defined (Q_OS_WIN)
    return Windows;
#else
    return Unix;
#endif
}

void QQmlFormatOptions::applySettings(const QQmlFormatSettings &settings)
{
    // Don't overwrite tab settings that were already set.
    if (!indentWidthSet()) {
        if (settings.isSet(QQmlFormatSettings::s_indentWidthSetting))
            setIndentWidth(settings.value(QQmlFormatSettings::s_indentWidthSetting).toInt());
        if (settings.isSet(QQmlFormatSettings::s_useTabsSetting))
            setTabsEnabled(settings.value(QQmlFormatSettings::s_useTabsSetting).toBool());
    }
    if (settings.isSet(QQmlFormatSettings::s_normalizeSetting))
        setNormalizeEnabled(settings.value(QQmlFormatSettings::s_normalizeSetting).toBool());
    if (settings.isSet(QQmlFormatSettings::s_newlineSetting))
        setNewline(QQmlFormatOptions::parseEndings(settings.value(QQmlFormatSettings::s_newlineSetting).toString()));
    if (settings.isSet(QQmlFormatSettings::s_objectsSpacingSetting))
        setObjectsSpacing(settings.value(QQmlFormatSettings::s_objectsSpacingSetting).toBool());
    if (settings.isSet(QQmlFormatSettings::s_functionsSpacingSetting))
        setFunctionsSpacing(settings.value(QQmlFormatSettings::s_functionsSpacingSetting).toBool());
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
