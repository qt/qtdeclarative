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
            QCoreApplication::translate("main", "Don't show progress")},
        {"sanity",
            QCoreApplication::translate("main", "Run extra sanity checks on the Figma file")},
        {{"g", "generate"},
            QCoreApplication::translate("main", "Generate one control (for debugging)"),
            QCoreApplication::translate("main", "The control to generate")}
    });
    parser.addPositionalArgument("figma_file_id",
        QCoreApplication::translate("main", "The figma file ID to create a style from."));

    if (!parser.parse(QCoreApplication::arguments())) {
        qWarning() << parser.errorText();
        return -1;
    }

    Bridge bridge;
    // Use the same options (as stored with QSettings) from the
    // last session, unless overridden from the command line
    if (parser.isSet("generate"))
        bridge.m_controlToGenerate = parser.value("generate");
    if (parser.isSet("verbose"))
        bridge.m_verbose = parser.isSet("verbose");
    if (parser.isSet("sanity"))
        bridge.m_sanity = parser.isSet("sanity");
    if (parser.isSet("token"))
        bridge.m_figmaToken = parser.value("token");
    if (parser.isSet("directory"))
        bridge.m_targetDirectory = parser.value("directory");
    if (!parser.positionalArguments().isEmpty())
        bridge.m_figmaUrlOrId = parser.positionalArguments().first();

    if (parser.positionalArguments().isEmpty()) {
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
        // Command line tool mode
        QObject::connect(&bridge, &Bridge::finished, &app, &QCoreApplication::quit);
        QObject::connect(&bridge, &Bridge::error, &bridge, [](const QString &msg){ qDebug().noquote() << "Failed:" << msg; });
        if (!parser.isSet("silent")) {
            QObject::connect(&bridge, &Bridge::warning, &bridge,
                             [](const QString &msg){ qDebug().noquote() << "Warning:" << msg; });
            QObject::connect(&bridge, &Bridge::progressLabelChanged, &bridge,
                             [](const QString &label){ qDebug().noquote() << "*" << label; });
            QObject::connect(&bridge, &Bridge::finished, &app,
                             []{ qDebug().noquote() << "* Finished!"; });
            QObject::connect(&bridge, &Bridge::figmaFileNameChanged, &app,
                             [](const QString &name){ qDebug().noquote() << "* Figma name: " + name; });
            if (parser.isSet("verbose"))
                QObject::connect(&bridge, &Bridge::debug, &bridge,
                                 [](const QString &msg){ qDebug().noquote() << msg; });
        }
        bridge.generate();
        return app.exec();
    }
}
