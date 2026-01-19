// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QLibraryInfo>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

class tst_scheduler_app : public QObject
{
    Q_OBJECT

public:
    tst_scheduler_app();
    ~tst_scheduler_app();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void launch();
    void qtconf();
    void correctAppleQtConfLocation();
};

tst_scheduler_app::tst_scheduler_app() {}

tst_scheduler_app::~tst_scheduler_app() {}

void tst_scheduler_app::initTestCase() {}

void tst_scheduler_app::cleanupTestCase() {}

void tst_scheduler_app::launch()
{
    QQmlApplicationEngine engine;
    engine.loadFromModule("p1.p2.p3.SchedulerApp", "Main");
    QCOMPARE(engine.rootObjects().size(), 1);
}

void tst_scheduler_app::qtconf()
{
    const QList<QString> importPaths = QLibraryInfo::paths(QLibraryInfo::QmlImportsPath);

    qDebug() << "Import paths:" << importPaths;

    struct PathCheck {
        QString caseDescription;
        std::function<bool(const QString &)> predicate;
    };

    const QList<PathCheck> requiredCases = {
        {"import path containing 'nested.module' module",
         [](const QString &path) {
           return path.endsWith("external") &&
                  QDir(path).exists("nested/module");
         }},
        {"import path containing containing 'Scheduler' module",
         [](const QString &path) { return QDir(path).exists("Scheduler"); }},
        {"import path containing containing 'p1.p2.p3.SchedulerApp' module",
         [](const QString &path) { return QDir(path).exists("p1/p2/p3/SchedulerApp"); }}
    };

    for (const PathCheck &currentCase : requiredCases) {
        bool found = std::any_of(importPaths.cbegin(), importPaths.cend(), currentCase.predicate);
        if (!found)
            QFAIL(qPrintable(QString("Expected %1 not found").arg(currentCase.caseDescription)));
    }
}

void tst_scheduler_app::correctAppleQtConfLocation()
{
#if !defined(Q_OS_DARWIN)
    QSKIP("This test is only relevant for Apple platforms");
#else
    // Check that the qt.conf file is not located next to the executable.
    QFileInfo executableInfo(QCoreApplication::applicationFilePath());
    QDir executableDir = executableInfo.dir();
    QString qtConfPath = executableDir.filePath(QLatin1String("qt.conf"));
    QVERIFY2(!QFileInfo::exists(qtConfPath),
             qPrintable(QString("qt.conf should not exist at: %1").arg(qtConfPath)));

    // Check that the qt.conf file is located in the Resources directory.
    QCOMPARE(executableDir.dirName(), QLatin1String("MacOS"));
    QDir contentsDir = executableDir;
    QVERIFY(contentsDir.cdUp());
    // Contents/Resources should exist at this point.
    QString resourcesPath = contentsDir.filePath(QLatin1String("Resources"));
    QDir resourcesDir(resourcesPath);
    QVERIFY(resourcesDir.exists());
    QString qtConfResourcesPath = resourcesDir.filePath(QLatin1String("qt.conf"));
    QVERIFY2(QFileInfo::exists(qtConfResourcesPath),
             qPrintable(QString("qt.conf should exist at: %1").arg(qtConfResourcesPath)));
#endif
}

QTEST_MAIN(tst_scheduler_app)

#include "tst_scheduler_app.moc"
