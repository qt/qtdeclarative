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
    void qtToolPaths_data();
    void qtToolPaths();
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

/*
    Resolves symlinks so that two spellings of the same directory compare equal,
    while leaving a path that doesn't exist untouched, so it still shows up in
    the failure message.
*/
static QString canonical(const QString &path)
{
    const QString resolved = QFileInfo(path).canonicalFilePath();
    return resolved.isEmpty() ? path : resolved;
}

void tst_non_bundle::qtToolPaths_data()
{
    QTest::addColumn<int>("location");
    QTest::addColumn<QString>("expectedPath");

    QTest::newRow("Binaries")
        << int(QLibraryInfo::BinariesPath)
        << QStringLiteral(EXPECTED_QT_BINARIES_PATH);
    QTest::newRow("LibraryExecutables")
        << int(QLibraryInfo::LibraryExecutablesPath)
        << QStringLiteral(EXPECTED_QT_LIBRARY_EXECUTABLES_PATH);
}

void tst_non_bundle::qtToolPaths()
{
    QFETCH(const int, location);
    QFETCH(const QString, expectedPath);

    const auto libraryPath = QLibraryInfo::LibraryPath(location);
    qDebug() << "Paths:" << QLibraryInfo::paths(libraryPath);

    // The generated qt.conf only lists QmlImports, but its mere presence makes
    // QLibraryInfo report app-prefixed paths for every other location as well,
    // ahead of the Qt-prefixed ones. Code that asks for a single path, to find
    // Qt's own binaries or helper tools, then no longer finds them.
    QCOMPARE(canonical(QLibraryInfo::path(libraryPath)), canonical(expectedPath));
}

void tst_non_bundle::launch()
{
    QQmlApplicationEngine engine;
    engine.loadFromModule("NonBundleApp", "Main");
    QCOMPARE(engine.rootObjects().size(), 1);
}

QTEST_MAIN(tst_non_bundle)

#include "tst_non_bundle.moc"
