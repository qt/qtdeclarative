// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qmlpreviewapplication.h"

int main(int argc, char *argv[])
{
    QmlPreviewApplication app(argc, argv);
    app.parseArguments();
    return app.exec();
}
