// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QtQuick/QQuickView>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    // This example needs Vulkan. It will not run otherwise.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:///scenegraph/vulkanunderqml/main.qml"));
    view.show();

    return app.exec();
}
