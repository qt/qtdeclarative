// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlformatoptions_p.h"
#include "qqmlformatsettings_p.h"

#if QT_CONFIG(commandlineparser)
#  include <QCommandLineParser>
#  include <QCommandLineOption>
#endif

using namespace Qt::StringLiterals;

QQmlFormatOptions::QQmlFormatOptions()
{
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
#if defined(Q_OS_WIN)
    return Windows;
#else
    return Unix;
#endif
}

void QQmlFormatOptions::applySettings(const QQmlFormatSettings &settings)
{
    // If the options is already set by commandline, don't override it with the values in the .ini
    // file.
    if (!isMarked(Settings::IndentWidth)
        && settings.isSet(QQmlFormatSettings::s_indentWidthSetting)) {
        setIndentWidth(settings.value(QQmlFormatSettings::s_indentWidthSetting).toInt());
    }

    if (!isMarked(Settings::UseTabs) && settings.isSet(QQmlFormatSettings::s_useTabsSetting)) {
        setTabsEnabled(settings.value(QQmlFormatSettings::s_useTabsSetting).toBool());
    }

    if (!isMarked(Settings::MaxColumnWidth)
        && settings.isSet(QQmlFormatSettings::s_maxColumnWidthSetting)) {
        setMaxColumnWidth(settings.value(QQmlFormatSettings::s_maxColumnWidthSetting).toInt());
    }

    if (!isMarked(Settings::NormalizeOrder)
        && settings.isSet(QQmlFormatSettings::s_normalizeSetting)) {
        setNormalizeEnabled(settings.value(QQmlFormatSettings::s_normalizeSetting).toBool());
    }

    if (!isMarked(Settings::NewlineType) && settings.isSet(QQmlFormatSettings::s_newlineSetting)) {
        setNewline(QQmlFormatOptions::parseEndings(
                settings.value(QQmlFormatSettings::s_newlineSetting).toString()));
    }

    if (!isMarked(Settings::ObjectsSpacing)
        && settings.isSet(QQmlFormatSettings::s_objectsSpacingSetting)) {
        setObjectsSpacing(settings.value(QQmlFormatSettings::s_objectsSpacingSetting).toBool());
    }

    if (!isMarked(Settings::FunctionsSpacing)
        && settings.isSet(QQmlFormatSettings::s_functionsSpacingSetting)) {
        setFunctionsSpacing(settings.value(QQmlFormatSettings::s_functionsSpacingSetting).toBool());
    }
}

QQmlFormatOptions QQmlFormatOptions::buildCommandLineOptions(const QStringList &args)
{
    QQmlFormatOptions options;
#if QT_CONFIG(commandlineparser)
    QCommandLineParser parser;
    parser.setApplicationDescription(
            "Formats QML files according to the QML Coding Conventions."_L1);
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption(
            QCommandLineOption({ "V"_L1, "verbose"_L1 },
                               QStringLiteral("Verbose mode. Outputs more detailed information.")));

    QCommandLineOption writeDefaultsOption(
            QStringList() << "write-defaults"_L1,
            QLatin1String("Writes defaults settings to .qmlformat.ini and exits (Warning: This "
                          "will overwrite any existing settings and comments!)"_L1));
    parser.addOption(writeDefaultsOption);

    QCommandLineOption ignoreSettings(QStringList() << "ignore-settings"_L1,
                                      QLatin1String("Ignores all settings files and only takes "
                                                    "command line options into consideration"_L1));
    parser.addOption(ignoreSettings);

    parser.addOption(QCommandLineOption(
            { "i"_L1, "inplace"_L1 },
            QStringLiteral("Edit file in-place instead of outputting to stdout.")));

    parser.addOption(QCommandLineOption({ "f"_L1, "force"_L1 },
                                        QStringLiteral("Continue even if an error has occurred.")));

    parser.addOption(QCommandLineOption({ "t"_L1, "tabs"_L1 },
                                        QStringLiteral("Use tabs instead of spaces.")));

    parser.addOption(QCommandLineOption({ "w"_L1, "indent-width"_L1 },
                                        QStringLiteral("How many spaces are used when indenting."),
                                        "width"_L1, "4"_L1));

    QCommandLineOption columnWidthOption(
            { "W"_L1, "column-width"_L1 },
            QStringLiteral("Breaks the line into multiple lines if exceedes the specified width."
                           "Use -1 to disable line wrapping. (default)"),
            "width"_L1, "-1"_L1);
    parser.addOption(columnWidthOption);
    parser.addOption(QCommandLineOption({ "n"_L1, "normalize"_L1 },
                                        QStringLiteral("Reorders the attributes of the objects "
                                                       "according to the QML Coding Guidelines.")));

    QCommandLineOption filesOption(
            { "F"_L1, "files"_L1 }, "Format all files listed in file, in-place"_L1, "file"_L1);
    parser.addOption(filesOption);

    parser.addOption(QCommandLineOption(
            { "l"_L1, "newline"_L1 },
            QStringLiteral("Override the new line format to use (native macos unix windows)."),
            "newline"_L1, "native"_L1));

    parser.addOption(QCommandLineOption(
            QStringList() << "objects-spacing"_L1,
            QStringLiteral("Ensure spaces between objects (only works with normalize option).")));

    parser.addOption(QCommandLineOption(
            QStringList() << "functions-spacing"_L1,
            QStringLiteral("Ensure spaces between functions (only works with normalize option).")));

    parser.addPositionalArgument("filenames"_L1, "files to be processed by qmlformat"_L1);

    parser.process(args);

    if (parser.isSet(writeDefaultsOption)) {
        options.setWriteDefaultSettingsEnabled(true);
        return options;
    }

    if (parser.positionalArguments().empty() && !parser.isSet(filesOption)) {
        options.addError("Error: Expected at least one input file."_L1);
        return options;
    }

    bool indentWidthOkay = false;
    const int indentWidth = parser.value("indent-width"_L1).toInt(&indentWidthOkay);
    if (!indentWidthOkay) {
        options.addError("Error: Invalid value passed to -w"_L1);
        return options;
    }

    QStringList files;
    if (!parser.value("files"_L1).isEmpty()) {
        const QString path = parser.value("files"_L1);
        QFile file(path);
        if (!file.open(QIODevice::Text | QIODevice::ReadOnly)) {
            options.addError("Error: Could not open file \""_L1 + path + "\" for option -F."_L1);
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
            options.addError("Error: File \""_L1 + path + "\" for option -F is empty."_L1);
            return options;
        }

        for (const auto &file : std::as_const(files)) {
            if (!QFile::exists(file)) {
                options.addError("Error: Entry \""_L1 + file + "\" of file \""_L1 + path
                                 + "\" passed to option -F could not be found."_L1);
                return options;
            }
        }
    } else {
        const auto &args = parser.positionalArguments();
        for (const auto &file : args) {
            if (!QFile::exists(file)) {
                options.addError("Error: Could not find file \""_L1 + file + "\"."_L1);
                return options;
            }
        }
    }

    options.setIsVerbose(parser.isSet("verbose"_L1));
    options.setIsInplace(parser.isSet("inplace"_L1));
    options.setForceEnabled(parser.isSet("force"_L1));
    options.setIgnoreSettingsEnabled(parser.isSet("ignore-settings"_L1));

    if (parser.isSet("tabs"_L1)) {
        options.mark(Settings::UseTabs);
        options.setTabsEnabled(true);
    }
    if (parser.isSet("normalize"_L1)) {
        options.mark(Settings::NormalizeOrder);
        options.setNormalizeEnabled(true);
    }
    if (parser.isSet("objects-spacing"_L1)) {
        options.mark(Settings::ObjectsSpacing);
        options.setObjectsSpacing(true);
    }
    if (parser.isSet("functions-spacing"_L1)) {
        options.mark(Settings::FunctionsSpacing);
        options.setFunctionsSpacing(true);
    }
    if (parser.isSet("indent-width"_L1)) {
        options.mark(Settings::IndentWidth);
        options.setIndentWidth(indentWidth);
    }

    if (parser.isSet("newline"_L1)) {
        options.mark(Settings::NewlineType);
        options.setNewline(QQmlFormatOptions::parseEndings(parser.value("newline"_L1)));
    }
    options.setFiles(files);
    options.setArguments(parser.positionalArguments());

    if (parser.isSet(columnWidthOption)) {
        bool isValidValue = false;
        const int maxColumnWidth = parser.value(columnWidthOption).toInt(&isValidValue);
        if (!isValidValue || maxColumnWidth < -1) {
            options.addError("Error: Invalid value passed to -W. Must be an integer >= -1"_L1);
            return options;
        }
        options.mark(Settings::MaxColumnWidth);
        options.setMaxColumnWidth(maxColumnWidth);
    }
#endif
    return options;
}

QQmlFormatOptions QQmlFormatOptions::optionsForFile(const QString &fileName,
                                                    QQmlFormatSettings *settings) const
{
    // Perform formatting inplace if --files option is set.
    const bool hasFiles = !files().isEmpty();
    QQmlFormatOptions perFileOptions = *this;
    if (hasFiles)
        perFileOptions.setIsInplace(true);

    if (!ignoreSettingsEnabled() && settings->search(fileName))
        perFileOptions.applySettings(*settings);

    return perFileOptions;
}
