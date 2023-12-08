// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "bridge.h"

int main(int argc, char **argv){
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("QtProject");
    QGuiApplication::setOrganizationDomain("www.qt-project.org");
    QGuiApplication::setApplicationName("StyleGenerator");

    QCommandLineParser parser;
    parser.setApplicationDescription("Creates a Qt Quick Controls style from a Figma file.");
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
            QCoreApplication::translate("main", "Print essential progress information")},
        {"veryverbose",
            QCoreApplication::translate("main", "Print detailed progress information")},
        {"sanity",
            QCoreApplication::translate("main", "Run extra sanity checks on the Figma file")},
        {{"g", "generate"},
            QCoreApplication::translate("main", "Generate only a subset of the controls, e.g -g RadioButton -g CheckBox"),
            QCoreApplication::translate("main", "control")},
        {{"f", "format"},
            QCoreApplication::translate("main", "The image format(s) to use, e.g -f png@1x -f png@2x -f svg"),
            QCoreApplication::translate("main", "format")},
        {{"s", "fallbackstyle"},
            QCoreApplication::translate("main", "The Qt style to use as fallback for ungenerated controls, e.g -s Fusion"),
            QCoreApplication::translate("main", "style")}
    });
    parser.addPositionalArgument("figma_file_id",
        QCoreApplication::translate("main", "The figma file ID to create a style from."));

    parser.process(app);

    const bool guiMode = argc == 1;
    Bridge bridge(guiMode);

    if (guiMode) {
        // GUI mode
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("bridge", &bridge);
        const QUrl url(QStringLiteral("qrc:/main.qml"));
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                         &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
        engine.load(url);
        return app.exec();
    } else {
        // CLI mode
        if (parser.positionalArguments().isEmpty()
                || !parser.isSet("directory")
                || !parser.isSet("token")) {
            parser.showHelp();
            return 0;
        }

        bridge.m_figmaUrlOrId = parser.positionalArguments().first();
        bridge.m_targetDirectory = QDir(parser.value("directory")).absolutePath();
        bridge.m_figmaToken = parser.value("token");
        bridge.m_sanity = parser.isSet("sanity");

        if (parser.isSet("generate"))
            bridge.m_selectedControls = parser.values("generate");
        else
            bridge.m_selectedControls = bridge.availableControls();
        if (parser.isSet("format"))
            bridge.m_selectedImageFormats = parser.values("format");
        else
            bridge.m_selectedImageFormats = {"png@1x"};
        if (parser.isSet("fallbackstyle"))
            bridge.m_selectedFallbackStyle = parser.value("fallbackstyle");
        else
            bridge.m_selectedFallbackStyle = "Basic";

        QObject::connect(&bridge, &Bridge::finished, &app, &QCoreApplication::quit);
        QObject::connect(&bridge, &Bridge::error, &bridge, [](const QString &msg){ qDebug().noquote() << "Failed:" << msg; });
        if (parser.isSet("verbose") || parser.isSet("veryverbose")) {
            QObject::connect(&bridge, &Bridge::warning, &bridge,
                             [](const QString &msg){ qDebug().noquote() << "Warning:" << msg; });
            QObject::connect(&bridge, &Bridge::progressLabelChanged, &bridge,
                             [](const QString &label){ qDebug().noquote() << "*" << label; });
            QObject::connect(&bridge, &Bridge::finished, &app,
                             []{ qDebug().noquote() << "* Finished!"; });
            QObject::connect(&bridge, &Bridge::figmaFileNameChanged, &app,
                             [](const QString &name){ qDebug().noquote() << "* Figma name: " + name; });
            if (parser.isSet("veryverbose"))
                QObject::connect(&bridge, &Bridge::debug, &bridge,
                                 [](const QString &msg){ qDebug().noquote() << msg; });
        }
        bridge.generate();
        return app.exec();
    }
}
