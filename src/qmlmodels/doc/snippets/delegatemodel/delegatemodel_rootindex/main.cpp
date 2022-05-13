// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QQuickView>
#include <QQmlContext>

#include <QApplication>
#include <QFileSystemModel>

//![0]
int main(int argc, char ** argv)
{
    QApplication app(argc, argv);

    QQuickView view;

    QFileSystemModel model;
    view.rootContext()->setContextProperty("fileSystemModel", &model);

    view.setSource(QUrl::fromLocalFile("view.qml"));
    view.show();

    return app.exec();
}
//![0]

