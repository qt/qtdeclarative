// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "benchmarkharness.h"

#include <QtGui/qguiapplication.h>

#include <QtTest/qtest.h>

using namespace Qt::StringLiterals;

// "Compiled into the binary" mode: the documents are the cachegen (AOT) bundles,
// whose compilation units are baked into the executable. Forcing the disk cache
// makes the engine prefer those units over recompiling.
int main(int argc, char *argv[])
{
    qputenv("QML_FORCE_DISK_CACHE", "1");
    qputenv("QT_QPA_PLATFORM", "offscreen");
    pinJitThreshold();

    QGuiApplication app(argc, argv);
    BenchmarkRunner runner(u":/qt/qml/Bundle/aot"_s, u"Bundle.aot"_s);
    return QTest::qExec(&runner, argc, argv);
}
