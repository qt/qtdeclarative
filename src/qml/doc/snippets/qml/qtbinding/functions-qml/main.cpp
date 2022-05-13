// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QtCore>
#include <QtQuick>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

//![0]
// main.cpp
QQmlEngine engine;
QQmlComponent component(&engine, "MyItem.qml");
QObject *object = component.create();

QString returnedValue;
QString msg = "Hello from C++";
QMetaObject::invokeMethod(object, "myQmlFunction",
        Q_RETURN_ARG(QString, returnedValue),
        Q_ARG(QString, msg));

qDebug() << "QML function returned:" << returnedValue;
delete object;
//![0]
}

