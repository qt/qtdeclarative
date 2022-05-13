// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qdebug.h>
#include <QtCore/qcoreapplication.h>

// Process that just outputs a fake "Waiting" message.
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc > 1)
        qDebug() << argv[1];
    return app.exec();
}
