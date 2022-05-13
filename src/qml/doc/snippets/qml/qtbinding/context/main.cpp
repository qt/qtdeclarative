// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QtQuick>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

//![0]
QQuickView view;
view.rootContext()->setContextProperty("currentDateTime", QDateTime::currentDateTime());
view.setSource(QUrl::fromLocalFile("MyItem.qml"));
view.show();
//![0]

    return app.exec();
}

