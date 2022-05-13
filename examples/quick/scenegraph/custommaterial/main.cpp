// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>

#include <QtQuick/QQuickView>

#include "customitem.h"

//! [1]
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:///scenegraph/custommaterial/main.qml"));
    view.show();

    return app.exec();
}
//! [1]
