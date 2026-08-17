// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QLibraryInfo>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QDir>
#include <QFileInfo>

#include <algorithm>

class tst_qml_only_import : public QObject
{
    Q_OBJECT

private slots:
    void qtconf();
    void qtToolPaths_data();
    void qtToolPaths();
    void launch();
};

void tst_qml_only_import::qtconf()
{
    const QList<QString> importPaths = QLibraryInfo::paths(QLibraryInfo::QmlImportsPath);
    qDebug() << "Import paths:" << importPaths;

    // nested.module is imported only from QML, with no DEPENDENCIES/IMPORTS
    // TARGET, so its import root can only have reached qt.conf via the scanner.
    const bool found = std::any_of(importPaths.cbegin(), importPaths.cend(),
        [](const QString &path) {
            return path.endsWith(QLatin1String("external")) && QDir(path).exists("nested/module");
        });
    QVERIFY2(found, "The QML-only import's module root was not found in qt.conf");
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

void tst_qml_only_import::qtToolPaths_data()
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

void tst_qml_only_import::qtToolPaths()
{
    QFETCH(const int, location);
    QFETCH(const QString, expectedPath);

    const auto libraryPath = QLibraryInfo::LibraryPath(location);
    qDebug() << "Paths:" << QLibraryInfo::paths(libraryPath);

    // Same as in tst_non_bundle, but this app is a bundle on Apple platforms,
    // where the generated qt.conf isn't the only thing that makes QLibraryInfo
    // report app-prefixed paths: a bundle prefix does so on its own, and adds
    // the modern bundle suffixes ("MacOS", "Helpers") on top. Neither may come
    // ahead of the Qt paths for a location the qt.conf doesn't list, or code
    // asking for a single path stops finding Qt's binaries and helper tools.
    QCOMPARE(canonical(QLibraryInfo::path(libraryPath)), canonical(expectedPath));
}

void tst_qml_only_import::launch()
{
    QQmlApplicationEngine engine;
    engine.loadFromModule("QmlOnlyImportApp", "Main");
    QCOMPARE(engine.rootObjects().size(), 1);
}

QTEST_MAIN(tst_qml_only_import)

#include "tst_qml_only_import.moc"
