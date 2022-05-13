// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtQml>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QJSEngine engine;
    //! [0]
    engine.globalObject().setProperty("foo", 123);
    qDebug() << "foo times two is:" << engine.evaluate("foo * 2").toNumber();
    //! [0]
    return 0;
}

