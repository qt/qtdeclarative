// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QtQuick>

#include "applicationdata.h"

//![0]
int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQuickView view;

    ApplicationData data;
    view.rootContext()->setContextProperty("applicationData", &data);

    view.setSource(QUrl::fromLocalFile("MyItem.qml"));
    view.show();

    return app.exec();
}
//![0]

