// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QFile>
#include <private/qquickrectangle_p.h>
#include <private/qsvgtinydocument_p.h>
#include <QQuickWindow>

#include "qsvgloader_p.h"

#define ENABLE_GUI

int main(int argc, char *argv[])
{
#ifdef ENABLE_GUI
    qputenv("QT_QUICKSHAPES_BACKEND", "curverenderer");
    QGuiApplication app(argc, argv);
#else
    QCoreApplication app(argc, argv);
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription("SVG to QML converter [tech preview]");
    parser.addHelpOption();
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "SVG file to read."));
    parser.addPositionalArgument("output", QCoreApplication::translate("main", "QML file to write."));

    QCommandLineOption optimizeOption("optimize-paths",
                                      QCoreApplication::translate("main", "Optimize paths for the curve renderer."));
    parser.addOption(optimizeOption);

    QCommandLineOption curveRendererOption("curve-renderer",
                                           QCoreApplication::translate("main", "Use the curve renderer in generated QML."));
    parser.addOption(curveRendererOption);

    QCommandLineOption typeNameOption(QStringList() << "t" << "type-name",
                                      QCoreApplication::translate("main", "Use <typename> for Shape."),
                                      QCoreApplication::translate("main", "typename"));
    parser.addOption(typeNameOption);

#ifdef ENABLE_GUI
    QCommandLineOption guiOption(QStringList() << "v" << "view",
                                 QCoreApplication::translate("main", "Display the SVG in a window."));
    parser.addOption(guiOption);
#endif
    parser.process(app);
    const QStringList args = parser.positionalArguments();
    if (args.size() < 1) {
        parser.showHelp(1);
    }

    const QString inFileName = args.at(0);

    auto *doc = QSvgTinyDocument::load(inFileName);
    if (!doc) {
        fprintf(stderr, "%s is not a valid SVG\n", qPrintable(inFileName));
        return 2;
    }

    const QString commentString = QLatin1String("Generated from SVG file %1").arg(inFileName);

    const auto outFileName = args.size() > 1 ? args.at(1) : QString{};
    const auto typeName = parser.value(typeNameOption);

    QSvgQmlWriter::GeneratorFlags flags;
    if (parser.isSet(curveRendererOption))
        flags |= QSvgQmlWriter::CurveRenderer;
    if (parser.isSet(optimizeOption))
        flags |= QSvgQmlWriter::OptimizePaths;

#ifdef ENABLE_GUI
    if (parser.isSet(guiOption)) {
        app.setOrganizationName("QtProject");
        const QUrl url(QStringLiteral("qrc:/main.qml"));
        QQmlApplicationEngine engine;
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                         &app, [&](QObject *obj, const QUrl &objUrl){
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
            if (obj) {
                auto *containerItem = obj->findChild<QQuickItem*>(QStringLiteral("svg_item"));
                auto *contents = QSvgQmlWriter::loadSVG(doc, outFileName, flags, typeName, containerItem, commentString);
                contents->setWidth(containerItem->implicitWidth()); // Workaround for runtime loader viewbox size logic. TODO: fix
                contents->setHeight(containerItem->implicitHeight());
            }
        });
        engine.load(url);
        return app.exec();
    }
#endif

    QSvgQmlWriter::loadSVG(doc, outFileName, flags, typeName, nullptr, commentString);
    return 0;
}
