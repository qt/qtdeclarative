// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QQuickView>

#include "graph.h"
#include <QtQml/QQmlContext>

int main(int argc, char *argv[])
{
    qputenv("QSG_RHI", "1"); // ### Qt 6 remove, this will be the default anyway

    QGuiApplication a(argc, argv);

    QQuickView view;
    view.resize(800, 400);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:///scenegraph/graph/main.qml"));
    view.show();

    return QGuiApplication::exec();
}
