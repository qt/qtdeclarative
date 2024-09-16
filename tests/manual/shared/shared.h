// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QDir>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlFileSelector>
#include <QQuickView> //Not using QQmlApplicationEngine because many examples don't have a Window{}

#ifdef Q_OS_MACOS
#define ADD_MACOS_BUNDLE_IMPORT_PATH \
    view.engine()->addImportPath(app.applicationDirPath() + QStringLiteral("/../PlugIns"));
#else
#define ADD_MACOS_BUNDLE_IMPORT_PATH
#endif

#define DECLARATIVE_EXAMPLE_MAIN(NAME) int main(int argc, char* argv[]) \
{\
    QGuiApplication app(argc,argv);\
    app.setOrganizationName("QtProject");\
    app.setOrganizationDomain("qt-project.org");\
    app.setApplicationName(QFileInfo(app.applicationFilePath()).baseName());\
    QQuickView view;\
    ADD_MACOS_BUNDLE_IMPORT_PATH\
    if (qEnvironmentVariableIntValue("QT_QUICK_CORE_PROFILE")) {\
        QSurfaceFormat f = view.format();\
        f.setProfile(QSurfaceFormat::CoreProfile);\
        f.setVersion(4, 4);\
        view.setFormat(f);\
    }\
    if (qEnvironmentVariableIntValue("QT_QUICK_MULTISAMPLE")) {\
        QSurfaceFormat f = view.format();\
        f.setSamples(4);\
        view.setFormat(f);\
    }\
    view.connect(view.engine(), &QQmlEngine::quit, &app, &QCoreApplication::quit);\
    new QQmlFileSelector(view.engine(), &view);\
    view.setSource(QUrl("qrc:/qt/qml/" #NAME ".qml")); \
    if (view.status() == QQuickView::Error)\
        return -1;\
    view.setResizeMode(QQuickView::SizeRootObjectToView);\
    view.show();\
    return app.exec();\
}
