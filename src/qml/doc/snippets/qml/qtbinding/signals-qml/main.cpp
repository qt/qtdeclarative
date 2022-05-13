// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QtQuick>

#include "myclass.h"

//![0]
int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQuickView view(QUrl::fromLocalFile("MyItem.qml"));
    QObject *item = view.rootObject();

    MyClass myClass;
    QObject::connect(item, SIGNAL(qmlSignal(QString)),
                     &myClass, SLOT(cppSlot(QString)));

    view.show();
    return app.exec();
}
//![0]

