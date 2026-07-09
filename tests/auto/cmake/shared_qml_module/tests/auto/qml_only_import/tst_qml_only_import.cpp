// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QLibraryInfo>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QDir>

#include <algorithm>

class tst_qml_only_import : public QObject
{
    Q_OBJECT

private slots:
    void qtconf();
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

void tst_qml_only_import::launch()
{
    QQmlApplicationEngine engine;
    engine.loadFromModule("QmlOnlyImportApp", "Main");
    QCOMPARE(engine.rootObjects().size(), 1);
}

QTEST_MAIN(tst_qml_only_import)

#include "tst_qml_only_import.moc"
