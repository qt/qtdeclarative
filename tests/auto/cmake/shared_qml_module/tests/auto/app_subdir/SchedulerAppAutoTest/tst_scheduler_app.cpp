// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QLibraryInfo>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

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


QTEST_MAIN(tst_scheduler_app)

#include "tst_scheduler_app.moc"
