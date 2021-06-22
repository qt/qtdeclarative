/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

using namespace QQmlJS::Dom;

struct Options
{
    bool verbose = false;
    bool inplace = false;
    bool force = false;
    bool tabs = false;
    bool valid = false;
    bool normalize = false;

    int indentWidth = 4;
    bool indentWidthSet = false;
    QString newline = "native";

    QStringList files;
    QStringList arguments;
    QStringList errors;
};

bool parseFile(const QString &filename, const Options &options)
{
    DomItem env =
            DomEnvironment::create(QStringList(),
                                   QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                                           | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);
    DomItem tFile; // place where to store the loaded file
    env.loadFile(
            filename, QString(),
            [&tFile](Path, const DomItem &, const DomItem &newIt) {
                tFile = newIt; // callback called when everything is loaded that receives the loaded
                               // external file pair (path, oldValue, newValue)
            },
            LoadOption::DefaultLoad);
    env.loadPendingDependencies();
    DomItem qmlFile = tFile.fileObject();
    std::shared_ptr<QmlFile> qmlFilePtr = qmlFile.ownerAs<QmlFile>();
    if (!qmlFilePtr || !qmlFilePtr->isValid()) {
        qmlFile.iterateErrors(
                [](DomItem, ErrorMessage msg) {
                    errorToQDebug(msg);
                    return true;
                },
                true);
        qWarning().noquote() << "Failed to parse" << filename;
        return false;
    }

    // Turn AST back into source code
    if (options.verbose)
        qWarning().noquote() << "Dumping" << filename;

    LineWriterOptions lwOptions;
    lwOptions.formatOptions.indentSize = options.indentWidth;
    lwOptions.formatOptions.useTabs = options.tabs;
    lwOptions.updateOptions = LineWriterOptions::Update::None;
    if (options.newline == "native") {
        // find out current line endings...
        QStringView code = qmlFilePtr->code();
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
        qWarning().noquote() << "Unknown line ending type" << options.newline;
        return false;
    }

    if (options.normalize)
        lwOptions.attributesSequence = LineWriterOptions::AttributesSequence::Normalize;
    else
        lwOptions.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;
    WriteOutChecks checks = WriteOutCheck::Default;
    if (options.force || qmlFilePtr->code().size() > 32000)
        checks = WriteOutCheck::None;

    MutableDomItem res;
    if (options.inplace) {
        if (options.verbose)
            qWarning().noquote() << "Writing to file" << filename;
        FileWriter fw;
        res = qmlFile.writeOut(filename, 2, lwOptions, &fw, checks);
    } else {
        QFile out;
        out.open(stdout, QIODevice::WriteOnly);
        LineWriter lw([&out](QStringView s) { out.write(s.toUtf8()); }, filename, lwOptions);
        OutWriter ow(lw);
        res = qmlFile.writeOutForFile(ow, checks);
        ow.flush();
    }
    return bool(res);
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

    parser.addPositionalArgument("filenames", "files to be processed by qmlformat");

    parser.process(app);

    bool indentWidthOkay = false;
    const int indentWidth = parser.value("indent-width").toInt(&indentWidthOkay);
    if (!indentWidthOkay) {
        Options options;
        options.errors.push_back("Error: Invalid value passed to -w");
        return options;
    }

    QStringList files;
    if (!parser.value("files").isEmpty()) {
        QFile file(parser.value("files"));
        file.open(QIODevice::Text | QIODevice::ReadOnly);
        if (file.isOpen()) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString file = in.readLine();

                if (file.isEmpty())
                    continue;

                files.push_back(file);
            }
        }
    }

    Options options;
    options.verbose = parser.isSet("verbose");
    options.inplace = parser.isSet("inplace");
    options.force = parser.isSet("force");
    options.tabs = parser.isSet("tabs");
    options.normalize = parser.isSet("normalize");
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

    const auto options = buildCommandLineOptions(app);
    if (!options.valid) {
        for (const auto &error : options.errors) {
            qWarning().noquote() << error;
        }

        return -1;
    }

    bool success = true;
    if (!options.files.isEmpty()) {
        if (!options.arguments.isEmpty())
            qWarning() << "Warning: Positional arguments are ignored when -F is used";

        for (const QString &file : options.files) {
            Q_ASSERT(!file.isEmpty());

            if (!parseFile(file, options))
                success = false;
        }
    } else {
        for (const QString &file : options.arguments) {
            if (!parseFile(file, options))
                success = false;
        }
    }

    return success ? 0 : 1;
}
