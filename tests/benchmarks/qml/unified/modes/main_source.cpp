// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "benchmarkharness.h"

#include <QtGui/qguiapplication.h>

#include <QtTest/qtest.h>

using namespace Qt::StringLiterals;

// "Compile from source" mode: the disk cache is disabled, so every load reparses
// and recompiles the document — no .qmlc reads or writes. The documents are the
// NO_CACHEGEN (qrc-only) bundles, which contain no ahead-of-time units.
int main(int argc, char *argv[])
{
    qputenv("QML_DISABLE_DISK_CACHE", "1");
    qputenv("QT_QPA_PLATFORM", "offscreen");
    pinJitThreshold();

    QGuiApplication app(argc, argv);
    BenchmarkRunner runner(u":/qt/qml/Bundle/src"_s, u"Bundle.src"_s);
    return QTest::qExec(&runner, argc, argv);
}
