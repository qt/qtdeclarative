// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickWindow>
#include <QtCore/QUrl>
#include <QDebug>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QQmlEngine engine;

    // Add the qrc root as QML import path so that the "shared" module can be found.
    engine.addImportPath(QStringLiteral(":/"));

#ifdef Q_OS_MACOS
    engine.addImportPath(app.applicationDirPath() + QStringLiteral("/../PlugIns"));
#endif

    QQmlComponent component(&engine);
    QQuickWindow::setDefaultAlphaBuffer(true);
    component.loadUrl(QUrl("qrc:///window/window.qml"));
    if ( component.isReady() )
        component.create();
    else
        qWarning() << component.errorString();
    return app.exec();
}
