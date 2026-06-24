// Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>

#include <private/qqmltypeloader_p.h>
#include <private/qmlutils_p.h>

class tst_QQMLTypeLoader : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQMLTypeLoader() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void benchmarkFileExistsCold_data();
    void benchmarkFileExistsCold();
    void benchmarkFileExistsWarmHit_data();
    void benchmarkFileExistsWarmHit();
    void benchmarkFileExistsWarmMiss_data();
    void benchmarkFileExistsWarmMiss();
    void benchmarkCleanFilePath_data();
    void benchmarkCleanFilePath();
    void benchmarkImportPathProbeCold();
    void benchmarkImportPathProbeWarm();

private:
    void addFileSystemAndResourceRows();
};

void tst_QQMLTypeLoader::addFileSystemAndResourceRows()
{
    QTest::addColumn<QString>("dir");

    QTest::newRow("filesystem") << dataDirectory();
    QTest::newRow("resources") << QStringLiteral(":/data");
}

void tst_QQMLTypeLoader::benchmarkFileExistsCold_data()
{
    addFileSystemAndResourceRows();
}

void tst_QQMLTypeLoader::benchmarkFileExistsCold()
{
    QFETCH(QString, dir);

    QQmlEngine engine;
    QQmlTypeLoader *loader = QQmlTypeLoader::get(&engine);

    // The expensive part of a cold lookup is the QDirListing that scans the
    // whole directory to populate the import-dir cache. clearCache() drops that
    // cache (the type loader thread is never started here, so its teardown is a
    // no-op) so that every iteration pays for a fresh scan.
    QBENCHMARK {
        loader->clearCache();
        QVERIFY(loader->fileExists(dir, QLatin1String("doesExist.qml")));
    }
}

void tst_QQMLTypeLoader::benchmarkFileExistsWarmHit_data()
{
    addFileSystemAndResourceRows();
}

void tst_QQMLTypeLoader::benchmarkFileExistsWarmHit()
{
    QFETCH(QString, dir);

    QQmlEngine engine;
    QQmlTypeLoader *loader = QQmlTypeLoader::get(&engine);

    // Prime the cache, then measure repeated cache hits for an existing file.
    QVERIFY(loader->fileExists(dir, QLatin1String("doesExist.qml")));
    QBENCHMARK {
        QVERIFY(loader->fileExists(dir, QLatin1String("doesExist.qml")));
    }
}

void tst_QQMLTypeLoader::benchmarkFileExistsWarmMiss_data()
{
    addFileSystemAndResourceRows();
}

void tst_QQMLTypeLoader::benchmarkFileExistsWarmMiss()
{
    QFETCH(QString, dir);

    QQmlEngine engine;
    QQmlTypeLoader *loader = QQmlTypeLoader::get(&engine);

    // Prime the cache, then measure repeated cache misses (the common case while
    // probing candidate component names that don't exist in a given directory).
    QVERIFY(!loader->fileExists(dir, QLatin1String("doesNotExist.qml")));
    QBENCHMARK {
        QVERIFY(!loader->fileExists(dir, QLatin1String("doesNotExist.qml")));
    }
}

void tst_QQMLTypeLoader::benchmarkCleanFilePath_data()
{
    addFileSystemAndResourceRows();
}

void tst_QQMLTypeLoader::benchmarkCleanFilePath()
{
    QFETCH(QString, dir);

    QQmlEngine engine;
    QQmlTypeLoader *loader = QQmlTypeLoader::get(&engine);
    const QString path = dir + QLatin1String("/doesExist.qml");

    // cleanFilePath() = fileExists() + QDir::cleanPath() for a
    // local path; for a qrc path it bypasses the import-dir cache and goes
    // straight to QFileInfo. Prime once so the local case measures warm lookups.
    QVERIFY(!loader->cleanFilePath(path).isEmpty());
    QBENCHMARK {
        QVERIFY(!loader->cleanFilePath(path).isEmpty());
    }
}

void tst_QQMLTypeLoader::benchmarkImportPathProbeCold()
{
    QQmlEngine engine;
    QQmlTypeLoader *loader = QQmlTypeLoader::get(&engine);

    // Closest microbenchmark to the real "file location" hotspot: while
    // resolving imports, the loader probes a "qmldir" in every import path, most
    // of which don't contain one. Each iteration starts cold so the directory
    // scans dominate, like a first compile.
    const QStringList paths = engine.importPathList();
    QVERIFY(!paths.isEmpty());
    QBENCHMARK {
        loader->clearCache();
        for (const QString &path : paths)
            loader->cleanFilePath(path + QLatin1String("/qmldir"));
    }
}

void tst_QQMLTypeLoader::benchmarkImportPathProbeWarm()
{
    QQmlEngine engine;
    QQmlTypeLoader *loader = QQmlTypeLoader::get(&engine);

    const QStringList paths = engine.importPathList();
    QVERIFY(!paths.isEmpty());

    // Warm: every directory has already been scanned, so this measures the pure
    // cache-lookup cost of probing the same set of qmldir candidates again.
    for (const QString &path : paths)
        loader->cleanFilePath(path + QLatin1String("/qmldir"));
    QBENCHMARK {
        for (const QString &path : paths)
            loader->cleanFilePath(path + QLatin1String("/qmldir"));
    }
}

QTEST_MAIN(tst_QQMLTypeLoader)

#include "tst_qqmltypeloader.moc"
