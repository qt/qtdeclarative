/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

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
