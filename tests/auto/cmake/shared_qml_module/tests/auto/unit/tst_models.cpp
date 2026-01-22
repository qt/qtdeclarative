// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QLibraryInfo>
#include <QDebug>

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
         [](const QString &path) { return QDir(path).exists("Scheduler"); }}};

    for (const PathCheck &currentCase : requiredCases) {
        bool found = std::any_of(importPaths.cbegin(), importPaths.cend(), currentCase.predicate);
        if (!found)
            QFAIL(qPrintable(QString("Expected %1 not found").arg(currentCase.caseDescription)));
    }
}

QTEST_MAIN(tst_models)

#include "tst_models.moc"
