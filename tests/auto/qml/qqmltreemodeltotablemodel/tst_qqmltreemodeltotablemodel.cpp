// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QAbstractItemModelTester>

#include <QtQmlModels/private/qqmltreemodeltotablemodel_p_p.h>

#include "testmodel.h"

/*
 * Note: Out of practical reasons, QQmlTreeModelToTableModel is by and large
 * tested from tst_qquicktreeview.cpp, where TreeView is available.
 */
class tst_QQmlTreeModelToTableModel : public QObject {
    Q_OBJECT

private slots:
    void testTestModel();
    void testTreeModelToTableModel();
};

void tst_QQmlTreeModelToTableModel::testTestModel()
{
    TestModel treeModel;
    QAbstractItemModelTester tester(&treeModel, QAbstractItemModelTester::FailureReportingMode::QtTest);
}

void tst_QQmlTreeModelToTableModel::testTreeModelToTableModel()
{
    QQmlTreeModelToTableModel model;
    TestModel treeModel;
    model.setModel(&treeModel);
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
}

QTEST_MAIN(tst_QQmlTreeModelToTableModel)

#include "tst_qqmltreemodeltotablemodel.moc"
