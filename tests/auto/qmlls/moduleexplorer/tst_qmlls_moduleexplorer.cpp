// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQmlLS/private/qqmllsmoduleexplorer_p.h>

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qtemporarydir.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

using namespace Qt::StringLiterals;

class tst_qmlls_moduleexplorer : public QObject
{
    Q_OBJECT

private slots:
    void findsModulesInImportPath();
    void survivesSymbolicLinkCycles();
    void doesNothingWhenDisabled();
    void cancelStopsExploration();

private:
    static void writeQmldir(const QString &directory, const QString &moduleName);
};

void tst_qmlls_moduleexplorer::writeQmldir(const QString &directory, const QString &moduleName)
{
    QVERIFY(QDir().mkpath(directory));
    QFile qmldir(directory + u"/qmldir"_s);
    QVERIFY(qmldir.open(QIODevice::WriteOnly | QIODevice::Text));
    qmldir.write("module " + moduleName.toUtf8() + "\n");
}

void tst_qmlls_moduleexplorer::findsModulesInImportPath()
{
    QTemporaryDir importPath;
    QVERIFY(importPath.isValid());
    writeQmldir(importPath.filePath(u"First"_s), u"First"_s);
    writeQmldir(importPath.filePath(u"Nested/Second"_s), u"Nested.Second"_s);

    QQmlLSModuleExplorer explorer;
    QSignalSpy finished(&explorer, &QQmlLSModuleExplorer::explorationFinished);
    explorer.explore({ importPath.path() });
    QTRY_VERIFY_WITH_TIMEOUT(!finished.isEmpty(), 10000);

    QStringList modules = explorer.modulesIn({ importPath.path() });
    modules.sort();
    QCOMPARE(modules, (QStringList{ u"First"_s, u"Nested.Second"_s }));
}

void tst_qmlls_moduleexplorer::survivesSymbolicLinkCycles()
{
    QTemporaryDir importPath;
    QVERIFY(importPath.isValid());
    writeQmldir(importPath.filePath(u"Looping/Module"_s), u"Looping.Module"_s);

    const QString parentLink = importPath.filePath(u"Looping/Module/parent"_s);
    if (!QFile::link(u".."_s, parentLink))
        QSKIP("Symbolic links are not supported here");
    QVERIFY(QFile::link(importPath.path(), importPath.filePath(u"Looping/root"_s)));

    QQmlLSModuleExplorer explorer;
    QSignalSpy finished(&explorer, &QQmlLSModuleExplorer::explorationFinished);
    explorer.explore({ importPath.path() });
    QTRY_VERIFY_WITH_TIMEOUT(!finished.isEmpty(), 10000);

    QVERIFY(explorer.modulesIn({ importPath.path() }).contains(u"Looping.Module"_s));
    QVERIFY(!explorer.isExploring());
}

void tst_qmlls_moduleexplorer::doesNothingWhenDisabled()
{
    QTemporaryDir importPath;
    QVERIFY(importPath.isValid());
    writeQmldir(importPath.filePath(u"First"_s), u"First"_s);

    QQmlLSModuleExplorer explorer;
    QSignalSpy started(&explorer, &QQmlLSModuleExplorer::explorationStarted);
    explorer.setEnabled(false);
    explorer.explore({ importPath.path() });

    QVERIFY(!explorer.isExploring());
    QVERIFY(started.isEmpty());
    QVERIFY(explorer.modulesIn({ importPath.path() }).isEmpty());
}

void tst_qmlls_moduleexplorer::cancelStopsExploration()
{
    QTemporaryDir importPath;
    QVERIFY(importPath.isValid());
    for (int i = 0; i < 200; ++i) {
        const QString name = u"Module%1"_s.arg(i);
        writeQmldir(importPath.filePath(name), name);
    }
    QTemporaryDir otherImportPath;
    QVERIFY(otherImportPath.isValid());
    writeQmldir(otherImportPath.filePath(u"Other"_s), u"Other"_s);

    QQmlLSModuleExplorer explorer;
    QSignalSpy finished(&explorer, &QQmlLSModuleExplorer::explorationFinished);
    explorer.explore({ importPath.path(), otherImportPath.path() });
    explorer.cancel();
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 10000);

    QVERIFY(!explorer.isExploring());
    QVERIFY(explorer.modulesIn({ otherImportPath.path() }).isEmpty());
    QVERIFY(explorer.modulesIn({ importPath.path() }).size() < 200);
}

QTEST_MAIN(tst_qmlls_moduleexplorer)
#include "tst_qmlls_moduleexplorer.moc"
