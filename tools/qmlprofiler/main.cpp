// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerapplication.h"

int main(int argc, char *argv[])
{
    QmlProfilerApplication app(argc, argv);

    app.parseArguments();

    if (app.isInteractive()) {
        app.startConsole();
        return app.exec();
    } else {
        int exitValue = app.exec();
        app.outputData();
        return exitValue;
    }
}
