// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtQml/qqmlengine.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QQmlEngine engine;
    return 0;
}
