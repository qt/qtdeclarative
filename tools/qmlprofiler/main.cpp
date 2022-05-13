// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commandlistener.h"
#include "qmlprofilerapplication.h"

int main(int argc, char *argv[])
{
    QmlProfilerApplication app(argc, argv);

    app.parseArguments();

    if (app.isInteractive()) {
        QThread listenerThread;
        CommandListener listener;
        listener.moveToThread(&listenerThread);
        QObject::connect(&listener, &CommandListener::command,
                         &app, &QmlProfilerApplication::userCommand);
        QObject::connect(&app, &QmlProfilerApplication::readyForCommand,
                         &listener, &CommandListener::readCommand);
        listenerThread.start();
        int exitValue = app.exec();
        listenerThread.quit();
        // wait for listener to exit
        listenerThread.wait();
        return exitValue;
    } else {
        int exitValue = app.exec();
        app.outputData();
        return exitValue;
    }
}
