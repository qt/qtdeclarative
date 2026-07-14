// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewapplication.h"

#include <QtCore/qfile.h>
#include <QtCore/qthread.h>

#include <cstdlib>

int main(int argc, char *argv[])
{
    // Hack to terminate the event loop when stdin is closed (non-interactive mode
    // only; in interactive mode the console owns standard input). The thread is
    // kept alive until _Exit() so we never have to unblock its blocking read.
    std::unique_ptr<QThread> stdinCloseWatcher;

    int exitCode = -1;
    {
        QmlPreviewApplication app(argc, argv);
        app.parseArguments();

        if (app.isInteractive()) {
            app.startConsole();
        } else {
            stdinCloseWatcher.reset(QThread::create([]() {
                QFile input;
                if (input.open(stdin, QIODevice::ReadOnly))
                    input.readAll();
            }));
            QObject::connect(stdinCloseWatcher.get(), &QThread::finished, &app,
                             &QCoreApplication::quit);
            stdinCloseWatcher->start();
        }

        exitCode = app.exec();
    }

    // _Exit() instead of return so that we don't have to unblock the input reader thread.
    std::_Exit(exitCode);
}
