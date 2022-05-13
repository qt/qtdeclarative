// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtQml>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    //! [0]
    QJSEngine engine;
    qDebug() << "the magic number is:" << engine.evaluate("1 + 2").toNumber();
    //! [0]
    return 0;
}
