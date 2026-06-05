// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtQuick/QQuickView>
#include <QGuiApplication>
#include <QQmlEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
//![0]
    QQuickView view;
//![0]
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.loadFromModule("ChartsApp", "App");
    view.show();
    return app.exec();
}
