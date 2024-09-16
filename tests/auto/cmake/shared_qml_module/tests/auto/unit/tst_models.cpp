// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest>


class tst_models : public QObject
{
    Q_OBJECT

public:
    tst_models();
    ~tst_models();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void qtconf();
};

tst_models::tst_models() {}

tst_models::~tst_models() {}

void tst_models::initTestCase() {}

void tst_models::cleanupTestCase() {}

void tst_models::qtconf()
{
    auto importPaths = QLibraryInfo::paths(QLibraryInfo::QmlImportsPath);
    QCOMPARE_GE(importPaths.size(), 2);
    if (importPaths.at(0).endsWith("nested")) {
        QDir dir(importPaths.at(1));
        QVERIFY(dir.exists("Scheduler"));
    } else if (importPaths.at(1).endsWith("nested")) {
        QDir dir(importPaths.at(0));
        QVERIFY(dir.exists("Scheduler"));
    } else {
        QFAIL("Expected import paths were not found");
    }
}

QTEST_MAIN(tst_models)

#include "tst_models.moc"
