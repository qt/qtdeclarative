// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QFile>
#include <QQuickWindow>
#include <QQuickItem>
#include <QtQuickVectorImageGenerator/private/qquickitemgenerator_p.h>
#include <QtQuickVectorImageGenerator/private/qquickqmlgenerator_p.h>
#include <QtQuickVectorImageGenerator/private/qquickvectorimageglobal_p.h>

#define ENABLE_GUI

int main(int argc, char *argv[])
{
#ifdef ENABLE_GUI
    QGuiApplication app(argc, argv);
#else
    QCoreApplication app(argc, argv);
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription("SVG to QML converter");
    parser.addHelpOption();
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "SVG file to read."));
    parser.addPositionalArgument("output", QCoreApplication::translate("main", "QML file to write."), "[output]");

    QCommandLineOption curveRendererOption({ "c", "curve-renderer" },
                                           QCoreApplication::translate("main", "Use the curve renderer in generated QML."));
    parser.addOption(curveRendererOption);

    QCommandLineOption optimizeOption({ "p", "optimize-paths" },
                                      QCoreApplication::translate("main", "Optimize paths for the curve renderer."));
    parser.addOption(optimizeOption);

    QCommandLineOption typeNameOption({ "t", "type-name" },
                                      QCoreApplication::translate("main", "Use <typename> for Shape."),
                                      QCoreApplication::translate("main", "typename"));
    parser.addOption(typeNameOption);

    QCommandLineOption copyrightOption("copyright-statement",
                                       QCoreApplication::translate("main", "Add <string> as a comment at the start of the generated file."),
                                       QCoreApplication::translate("main", "string"));
    parser.addOption(copyrightOption);

    QCommandLineOption outlineModeOption("outline-stroke-mode",
                                         QCoreApplication::translate("main", "Stroke the outline (contour) of the filled shape instead of "
                                                                             "the original path. Also sets optimize-paths."));
    parser.addOption(outlineModeOption);

    QCommandLineOption assetOutputDirectoryOption("asset-output-directory",
                                                  QCoreApplication::translate("main", "If the SVG refers to external or embedded files, such as images, these "
                                                                                      "will be copied into the same directory as the output QML file by default. "
                                                                                      "Set the asset output directory to override the location."),
                                                  QCoreApplication::translate("main", "directory"));
    parser.addOption(assetOutputDirectoryOption);

    QCommandLineOption assetOutputPrefixOption("asset-output-prefix",
                                               QCoreApplication::translate("main", "If the SVG refers to external or embedded files, such as images, these "
                                                                                   "will be copied to files with unique identifiers. By default, the files will be prefixed "
                                                                                   "with \"svg_asset_\". Set the asset output prefix to override the prefix."),
                                               QCoreApplication::translate("main", "prefix"));
    parser.addOption(assetOutputPrefixOption);

    QCommandLineOption keepPathsOption("keep-external-paths",
                                       QCoreApplication::translate("main", "Any paths to external files will be retained in the QML output. "
                                                                           "The paths will be reformatted as relative to the output file. If "
                                                                           "this is not enabled, copies of the file will be saved to the asset output "
                                                                           "directory. Embedded data will still be saved to files, even if "
                                                                           "this option is set."));
    parser.addOption(keepPathsOption);

#ifdef ENABLE_GUI
    QCommandLineOption guiOption({ "v", "view" },
                                 QCoreApplication::translate("main", "Display the generated QML in a window. This is the default behavior if no "
                                                                     "output file is specified."));
    parser.addOption(guiOption);
#endif
    parser.process(app);
    const QStringList args = parser.positionalArguments();
    if (args.size() < 1) {
        parser.showHelp(1);
    }

    const QString inFileName = args.at(0);

    QString commentString = QLatin1String("Generated from SVG file %1").arg(inFileName);

    const auto outFileName = args.size() > 1 ? args.at(1) : QString{};
    const auto typeName = parser.value(typeNameOption);
    const auto assetOutputDirectory = parser.value(assetOutputDirectoryOption);
    const auto assetOutputPrefix = parser.value(assetOutputPrefixOption);
    const bool keepPaths = parser.isSet(keepPathsOption);
    auto copyrightString = parser.value(copyrightOption);

    if (!copyrightString.isEmpty()) {
        copyrightString = copyrightString.replace("\\n", "\n");
        commentString = copyrightString + u"\n" + commentString;
    }

    QQuickVectorImageGenerator::GeneratorFlags flags;
    if (parser.isSet(curveRendererOption))
        flags |= QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer;
    if (parser.isSet(optimizeOption))
        flags |= QQuickVectorImageGenerator::GeneratorFlag::OptimizePaths;
    if (parser.isSet(outlineModeOption))
        flags |= (QQuickVectorImageGenerator::GeneratorFlag::OutlineStrokeMode
                  | QQuickVectorImageGenerator::GeneratorFlag::OptimizePaths);

    QQuickQmlGenerator generator(inFileName, flags, outFileName);
    generator.setShapeTypeName(typeName);
    generator.setCommentString(commentString);
    generator.setAssetFileDirectory(assetOutputDirectory);
    generator.setAssetFilePrefix(assetOutputPrefix);
    generator.setRetainFilePaths(keepPaths);
    bool ok = generator.generate();

#ifdef ENABLE_GUI
    if (ok && (parser.isSet(guiOption) || outFileName.isEmpty())) {
        app.setOrganizationName("QtProject");
        const QUrl url(QStringLiteral("qrc:/main.qml"));
        QQmlApplicationEngine engine;
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                         &app, [&](QObject *obj, const QUrl &objUrl){
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
            if (obj) {
                auto *containerItem = obj->findChild<QQuickItem*>(QStringLiteral("svg_item"));
                QQuickItemGenerator generator(inFileName, flags, containerItem);
                generator.generate();
            }
        });
        engine.load(url);
        return app.exec();
    }
#endif

    return ok ? 0 : 1;
}
