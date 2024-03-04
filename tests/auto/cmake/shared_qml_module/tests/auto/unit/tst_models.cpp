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

    void dummy();
};

tst_models::tst_models() {}

tst_models::~tst_models() {}

void tst_models::initTestCase() {}

void tst_models::cleanupTestCase() {}

void tst_models::dummy()
{
}

QTEST_MAIN(tst_models)

#include "tst_models.moc"
