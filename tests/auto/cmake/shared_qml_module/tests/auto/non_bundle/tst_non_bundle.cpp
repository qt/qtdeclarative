// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QLibraryInfo>
#include <QDebug>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QDir>
#include <QFileInfo>

#include <algorithm>

class tst_non_bundle : public QObject
{
    Q_OBJECT

private slots:
    void qtConfNextToBinary();
    void qtconf();
    void launch();
};

void tst_non_bundle::qtConfNextToBinary()
{
    // For a non-bundle executable, qt.conf is written next to the binary, not
    // into a bundle Resources directory.
    const QDir binaryDir(QCoreApplication::applicationDirPath());
    const QString qtConfPath = binaryDir.filePath(QLatin1String("qt.conf"));
    QVERIFY2(QFileInfo::exists(qtConfPath),
             qPrintable(QStringLiteral("qt.conf should exist next to the binary at: %1")
                            .arg(qtConfPath)));
}

void tst_non_bundle::qtconf()
{
    const QList<QString> importPaths = QLibraryInfo::paths(QLibraryInfo::QmlImportsPath);
    qDebug() << "Import paths:" << importPaths;

    const bool found = std::any_of(importPaths.cbegin(), importPaths.cend(),
        [](const QString &path) { return QDir(path).exists("Scheduler"); });
    QVERIFY2(found, "The Scheduler import root was not found in qt.conf");
}

void tst_non_bundle::launch()
{
    QQmlApplicationEngine engine;
    engine.loadFromModule("NonBundleApp", "Main");
    QCOMPARE(engine.rootObjects().size(), 1);
}

QTEST_MAIN(tst_non_bundle)

#include "tst_non_bundle.moc"
