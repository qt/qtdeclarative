// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QIcon>

int main(int argc, char *argv[])
{
    // Allow navigation.js to "GET" walk_route.json with XMLHttpRequest.
    qputenv("QML_XHR_ALLOW_FILE_READ", "1");

    QCoreApplication::setApplicationName("Wearable");
    QCoreApplication::setOrganizationName("QtProject");

    QGuiApplication app(argc, argv);

    //! [style]
    QQuickStyle::setStyle(QStringLiteral("WearableStyle"));
    //! [style]

    //! [icons]
    QIcon::setThemeSearchPaths(QStringList() << ":/qt/qml/Wearable/icons");
    QIcon::setThemeName(QStringLiteral("wearable"));
    //! [icons]

    QQmlApplicationEngine engine;
#ifdef Q_OS_MACOS
    engine.addImportPath(app.applicationDirPath() + "/../PlugIns");
#endif
    engine.loadFromModule("Wearable", "Main");

    return app.exec();
}
