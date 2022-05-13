// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QDir>
#include <QGuiApplication>
#include <QQmlFileSelector>
#include <QQuickView>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("QtProject");
    app.setOrganizationDomain("qt-project.org");
    app.setApplicationName(QFileInfo(app.applicationFilePath()).baseName());
    QQuickView view;

    qputenv("QML_XHR_ALLOW_FILE_READ", QByteArray("1"));

    view.connect(view.engine(), &QQmlEngine::quit, &app, &QCoreApplication::quit);
    view.setSource(QUrl("qrc:///xmlhttprequest/xmlhttprequest.qml"));
    if (view.status() == QQuickView::Error)
        return -1;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.show();
    return app.exec();
}
