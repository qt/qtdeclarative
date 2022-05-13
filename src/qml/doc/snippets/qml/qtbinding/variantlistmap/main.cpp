// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QtQuick>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

//![0]
// C++
QQuickView view(QUrl::fromLocalFile("MyItem.qml"));

QVariantList list;
list << 10 << QColor(Qt::green) << "bottles";

QVariantMap map;
map.insert("language", "QML");
map.insert("released", QDate(2010, 9, 21));

QMetaObject::invokeMethod(view.rootObject(), "readValues",
        Q_ARG(QVariant, QVariant::fromValue(list)),
        Q_ARG(QVariant, QVariant::fromValue(map)));
//![0]

    view.setSource(QUrl::fromLocalFile("MyItem.qml"));
    view.show();

    return app.exec();
}

