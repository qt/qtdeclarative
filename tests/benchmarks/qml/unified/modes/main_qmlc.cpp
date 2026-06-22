// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "benchmarkharness.h"

#include <QtGui/qguiapplication.h>

#include <QtCore/qtemporarydir.h>

#include <QtTest/qtest.h>

using namespace Qt::StringLiterals;

// "Load run-time-created .qmlc" mode: the documents are the qrc-only (source)
// bundles. The disk cache is enabled with a private temporary directory as its
// path, so the harness's untimed pre-heat writes fresh .qmlc files there and the
// timed loads read them back. A dedicated temp dir guarantees the cache holds
// exactly these documents' units and nothing stale.
int main(int argc, char *argv[])
{
    static QTemporaryDir cacheDir; // kept for the whole process; removed at exit
    qputenv("QML_DISK_CACHE", "qmlc");
    if (!qEnvironmentVariableIsSet("QML_DISK_CACHE_PATH")) // allow override for inspection
        qputenv("QML_DISK_CACHE_PATH", cacheDir.path().toLocal8Bit());
    qputenv("QT_QPA_PLATFORM", "offscreen");
    pinJitThreshold();

    QGuiApplication app(argc, argv);
    BenchmarkRunner runner(u":/qt/qml/Bundle/src"_s, u"Bundle.src"_s);
    return QTest::qExec(&runner, argc, argv);
}
