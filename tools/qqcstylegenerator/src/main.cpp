// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui>

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
        {{"v", "verbose"},
            QCoreApplication::translate("main", "Debug out what gets generated.")}
    });
    parser.addPositionalArgument("qtbridge",
        QCoreApplication::translate("main", "The .qtbridge file to create a style from."));

    if (!parser.parse(QCoreApplication::arguments())) {
        qWarning() << parser.errorText();
        return -1;
    }

    if (parser.positionalArguments().length() != 1) {
        parser.showHelp();
        return -1;
    }

    return 0;
}
