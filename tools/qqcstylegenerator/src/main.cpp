// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui>

#include "stylegenerator.h"

int main(int argc, char **argv){
    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Creates a Qt Quick Controls style from a .qtbridge file.");
    parser.addHelpOption();
    parser.addOptions({
        {{"d", "directory"},
            QCoreApplication::translate("main", "The target directory where the style will be created."),
            QCoreApplication::translate("main", "directory"),
            "."},
        {{"t", "token"},
            QCoreApplication::translate("main", "A Figma-generated token that lets the tool access the Figma file."),
            QCoreApplication::translate("main", "token")},
        {{"v", "verbose"},
            QCoreApplication::translate("main", "Print everything that gets generated.")},
        {{"s", "silent"},
            QCoreApplication::translate("main", "Don't show progress")}
    });
    parser.addPositionalArgument("figma_file_id",
        QCoreApplication::translate("main", "The figma file ID to create a style from."));

    if (!parser.parse(QCoreApplication::arguments())) {
        qWarning() << parser.errorText();
        return -1;
    }

    if (parser.positionalArguments().length() != 1) {
        parser.showHelp();
        return -1;
    }

    const QString fileId = parser.positionalArguments().first();
    const QString token = parser.value("token");
    const QString destinationPath = parser.value("directory");
    const bool verbose = parser.isSet("verbose");
    const bool silent = parser.isSet("silent");

    if (token.isEmpty()) {
        qWarning() << "You need to specify a Figma generated token using '--token'";
        return -1;
    }

    try {
        StyleGenerator generator(fileId, token, destinationPath, verbose, silent);
        generator.generateStyle();
    } catch (std::exception &e) {
        qWarning() << "Error:" << e.what();
        return -1;
    }

    return 0;
}
