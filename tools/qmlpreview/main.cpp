// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlpreviewapplication.h"

int main(int argc, char *argv[])
{
    QmlPreviewApplication app(argc, argv);
    app.parseArguments();
    return app.exec();
}
