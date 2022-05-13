// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QObject>
#include <QtQml>
#include "myobject.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    //! [0]
    QJSEngine engine;
    QObject *someObject = new MyObject;
    QJSValue objectValue = engine.newQObject(someObject);
    engine.globalObject().setProperty("myObject", objectValue);
    //! [0]
    qDebug() << "myObject's calculate() function returns"
             << engine.evaluate("myObject.calculate(10)").toNumber();
    return 0;
}
