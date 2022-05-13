// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtCore>
#include <QtQml>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

//![0]
QQmlEngine engine;
QQmlComponent component(&engine, "MyItem.qml");
QObject *object = component.create();

qDebug() << "Property value:" << QQmlProperty::read(object, "someNumber").toInt();
QQmlProperty::write(object, "someNumber", 5000);

qDebug() << "Property value:" << object->property("someNumber").toInt();
object->setProperty("someNumber", 100);
//![0]

    return app.exec();
}

