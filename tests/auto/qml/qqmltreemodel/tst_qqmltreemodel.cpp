// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicktableview_p.h>
#include <QtQuick/private/qquicktreeview_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtLabsQmlModels/private/qqmltreemodel_p.h>

using namespace Qt::Literals::StringLiterals;

class tst_QQmlTreeModel : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQmlTreeModel() : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::FailOnWarnings) {}

private slots:
    void appendToEmptyModel();
    void appendToRoot();
    void appendRow();
    void clear();
    void getRow();
    void removeRow();
    void setDataThroughDelegate();
    void setRowsForEmptyModel();
    void setRowsOnNonEmptyModel();
    void setRowsRejectsNonArray();
    void setRowsFromJSON();
    void setRow();
    void setData();
    void insertRow();
    void insertRowEmptyModel();
    void moveRows();
    void moveRowsAcrossNodes();
};

void tst_QQmlTreeModel::appendToEmptyModel()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("empty.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    int rowsChangedSignalEmissions = 0;

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 0);  // treeView cannot call our treeSize

    // append to an empty model
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendSubTree"));
    // we have a subtree with three nodes
    QCOMPARE(model->treeSize(), 3);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    // now we also have roles
    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 2);
    QVERIFY(roleNames.values().contains("display"));
    QVERIFY(roleNames.values().contains("decoration"));
    // Wait until updatePolish() gets called, which is where the size is recalculated.
    QTRY_COMPARE(treeView->rows(), 1);
    QCOMPARE(treeView->columns(), 5);

    // check the node on the top level
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 4);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"Peach"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"Princess Peach"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.45);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);

    // the model is no longer empty, it can verify the input and reject invalid entries
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* expected the property named"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendInvalid"));
    // the tree does not change
    QCOMPARE(model->treeSize(), 3);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->rows(), 1);
    QCOMPARE(treeView->columns(), 5);

    // this also means that roles that were not present in the initial data will be rejected,
    // but the rest will be appended
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendWithExtraData"));   // TODO - Suppress warning ?
    QCOMPARE(model->treeSize(), 4);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    // Wait until updatePolish() gets called, which is where the size is recalculated.
    QTRY_COMPARE(treeView->rows(), 2);
    QCOMPARE(treeView->columns(), 5);

    // check the new node
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("display")).toString(), u"Williams"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
}

void tst_QQmlTreeModel::appendToRoot()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("common.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    int rowsChangedSignalEmissions = 0;

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 2);
    QVERIFY(roleNames.values().contains("display"));
    QVERIFY(roleNames.values().contains("decoration"));

    // first top level node
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"Apple"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"Granny Smith"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);

    // second top level node
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 4);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("display")).toString(), u"Peach"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("display")).toString(), u"Princess Peach"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.45);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);

    // append node to top level
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendNodeToRoot"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 9);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);

    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 2, QModelIndex()), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index(2, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 3, QModelIndex()), roleNames.key("display")).toString(), u"Williams"_s);
    QCOMPARE(model->data(model->index(2, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(2, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);

    // append subtree to top level
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendSubTreeToRoot"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 12);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);

    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("display")).toInt(), 4);
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(3, 2, QModelIndex()), roleNames.key("display")).toString(), u"Peach"_s);
    QCOMPARE(model->data(model->index(3, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(3, 3, QModelIndex()), roleNames.key("display")).toString(), u"Princess Peach"_s);
    QCOMPARE(model->data(model->index(3, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(3, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.45);
    QCOMPARE(model->data(model->index(3, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);

    // {3,0} is the tree index, 0 is the col index
    QCOMPARE(model->data(model->index({3,0}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({3,0}, 0), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({3,0}, 1), roleNames.key("display")).toInt(), 5);
    QCOMPARE(model->data(model->index({3,0}, 1), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({3,0}, 2), roleNames.key("display")).toString(), u"Strawberry"_s);
    QCOMPARE(model->data(model->index({3,0}, 2), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({3,0}, 3), roleNames.key("display")).toString(), u"Perry the Berry"_s);
    QCOMPARE(model->data(model->index({3,0}, 3), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({3,0}, 4), roleNames.key("display")).toDouble(), 3.8);
    QCOMPARE(model->data(model->index({3,0}, 4), roleNames.key("decoration")).toString(), u"red"_s);

    // {3,1} is the tree index, 0 is the col index
    QCOMPARE(model->data(model->index({3,1}, 0), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index({3,1}, 0), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({3,1}, 1), roleNames.key("display")).toInt(), 6);
    QCOMPARE(model->data(model->index({3,1}, 1), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({3,1}, 2), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index({3,1}, 2), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({3,1}, 3), roleNames.key("display")).toString(), u"Bear Pear"_s);
    QCOMPARE(model->data(model->index({3,1}, 3), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({3,1}, 4), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index({3,1}, 4), roleNames.key("decoration")).toString(), u"green"_s);

    // Try to append something invalid - an int
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* got int instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendToRootInvalid1"));
    // nothing gets appended
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 12);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // Try to append something invalid - fruitName is an []
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* expected the property named"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendToRootInvalid2"));
    // nothing gets appended
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 12);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // Call append with something invalid - the input is an array instead of a simple object.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* row manipulation functions do not support complex rows"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendToRootInvalid3"));
    // nothing gets appended
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 12);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // Call append with a node that has an unexpected role;
    // the node should be added and the extra data ignored.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* append: could not find any node at the specified index"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendNodeToRootWithExtraData"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 13);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);

    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(4, 2, QModelIndex()), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index(4, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(4, 3, QModelIndex()), roleNames.key("display")).toString(), u"Williams"_s);
    QCOMPARE(model->data(model->index(4, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(4, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(4, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
}

void tst_QQmlTreeModel::appendRow()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("common.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    int rowsChangedSignalEmissions = 0;

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 2);
    QVERIFY(roleNames.values().contains("display"));
    QVERIFY(roleNames.values().contains("decoration"));

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);  // treeView cannot call our treeSize

    // Try to append something invalid - an int
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* got int instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendInvalidNode1"));
    // Nothing is inserted
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // Try to append something invalid - a subtree, but one child has an invalid data type
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* expected the property named"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendInvalidNode2"));
    // Nothing is inserted
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // Call append with something invalid - the input is an array instead of a simple object.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* row manipulation functions do not support complex rows"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendInvalidNode3"));
    // Nothing is inserted
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // Append a node to a leaf
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendToLeaf"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 9);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // Appending to {1,1} - the index of the new node is {1,1,0}
    // {1,1,0} is the tree index, 0 is the col index
    QCOMPARE(model->data(model->index({1,0,0}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({1,0,0}, 0), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({1,0,0}, 1), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index({1,0,0}, 1), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({1,0,0}, 2), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index({1,0,0}, 2), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({1,0,0}, 3), roleNames.key("display")).toString(), u"Bear Pear"_s);
    QCOMPARE(model->data(model->index({1,0,0}, 3), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({1,0,0}, 4), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index({1,0,0}, 4), roleNames.key("decoration")).toString(), u"green"_s);

    // Append to an "intermediate" node
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendToMiddle"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 10);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // Appending to {0,0} - the index of the new node is {0,0,0}
    // {0,0,0} is the tree index, 0 is the col index
    QCOMPARE(model->data(model->index({0,0,0}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({0,0,0}, 0), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({0,0,0}, 1), roleNames.key("display")).toInt(), 5);
    QCOMPARE(model->data(model->index({0,0,0}, 1), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({0,0,0}, 2), roleNames.key("display")).toString(), u"Strawberry"_s);
    QCOMPARE(model->data(model->index({0,0,0}, 2), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({0,0,0}, 3), roleNames.key("display")).toString(), u"Perry the Berry"_s);
    QCOMPARE(model->data(model->index({0,0,0}, 3), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({0,0,0}, 4), roleNames.key("display")).toDouble(), 3.8);
    QCOMPARE(model->data(model->index({0,0,0}, 4), roleNames.key("decoration")).toString(), u"red"_s);

    // Call append with a node that has an unexpected role;
    // the node should be added and the extra data ignored.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendToNodeWithExtraData"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 11);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // Appending to {1} - the index of the new node is {1,2}
    // {1,2} is the tree index, 0 is the col index
    QCOMPARE(model->data(model->index({1,2}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({1,2}, 0), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({1,2}, 1), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index({1,2}, 1), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({1,2}, 2), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index({1,2}, 2), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({1,2}, 3), roleNames.key("display")).toString(), u"Williams"_s);
    QCOMPARE(model->data(model->index({1,2}, 3), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({1,2}, 4), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index({1,2}, 4), roleNames.key("decoration")).toString(), u"green"_s);

    // Try to append to an invalid index - the index contains a negtive number
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendToNegativeIndex"));
    // Node is appended to the root
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 12);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QTRY_COMPARE(treeView->rows(), 3);

    QCOMPARE(model->data(model->index(2, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(2, 0), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 1), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(2, 1), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 2), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index(2, 2), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 3), roleNames.key("display")).toString(), u"Williams"_s);
    QCOMPARE(model->data(model->index(2, 3), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 4), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(2, 4), roleNames.key("decoration")).toString(), u"green"_s);

    // Try to append to an invalid index - the node does not exist
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendToInvalidIndex"));   // TODO - ignore waning message ?
    // Node is appended to the root
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 13);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QTRY_COMPARE(treeView->rows(), 4);

    QCOMPARE(model->data(model->index(3, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(3, 0), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(3, 1), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(3, 1), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(3, 2), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index(3, 2), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(3, 3), roleNames.key("display")).toString(), u"Williams"_s);
    QCOMPARE(model->data(model->index(3, 3), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(3, 4), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(3, 4), roleNames.key("decoration")).toString(), u"green"_s);
}

void tst_QQmlTreeModel::clear()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("common.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);  // treeView cannot call our treeSize

    model->clear();
    // the rows should be cleared but the columns should not change
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), 1);
    // Wait until updatePolish() gets called, which is where the size is recalculated.
    QTRY_COMPARE(treeView->rows(), 0);
    QCOMPARE(treeView->columns(), 5);
}

void tst_QQmlTreeModel::getRow()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("common.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QVariant treeRow;
    QVariantMap rowValues;

    // Call getRow with an invalid tree index (contains negative number).
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    treeRow = model->getRow(model->index({0,1,-1}, 0));
    QVERIFY(!treeRow.isValid());

    // Call getRow with another invalid tree index (points to nonexistent element).
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    treeRow = model->getRow(model->index({1,2}, 0));
    QVERIFY(!treeRow.isValid());

    // Call getRow with a valid tree index - this is a leaf.
    treeRow = model->getRow(model->index({0,1,0}, 0));
    QVERIFY(treeRow.isValid());
    rowValues = treeRow.toMap();

    QCOMPARE(rowValues.value("amount"), 4);
    QCOMPARE(rowValues.value("checked"), true);
    QCOMPARE(rowValues.value("color"), "orange");
    QCOMPARE(rowValues.value("fruitName"), "Navel");
    QCOMPARE(rowValues.value("fruitPrice"), 2.5);
    QCOMPARE(rowValues.value("fruitType"), "Orange");

    // Call getRow with a valid tree index - another leaf.
    treeRow = model->getRow(model->index({1,1}, 0));
    QVERIFY(treeRow.isValid());
    rowValues = treeRow.toMap();

    QCOMPARE(rowValues.value("amount"), 6);
    QCOMPARE(rowValues.value("checked"), false);
    QCOMPARE(rowValues.value("color"), "green");
    QCOMPARE(rowValues.value("fruitName"), "Bear Pear");
    QCOMPARE(rowValues.value("fruitPrice"), 1.5);
    QCOMPARE(rowValues.value("fruitType"), "Pear");

    // Call getRow with a valid tree index - from top level
    std::vector<int> treeIndex = {0};
    treeRow = model->getRow(model->index(treeIndex, 0));
    QVERIFY(treeRow.isValid());
    rowValues = treeRow.toMap();

    QCOMPARE(rowValues.value("amount"), 1);
    QCOMPARE(rowValues.value("checked"), false);
    QCOMPARE(rowValues.value("color"), "red");
    QCOMPARE(rowValues.value("fruitName"), "Granny Smith");
    QCOMPARE(rowValues.value("fruitPrice"), 1.5);
    QCOMPARE(rowValues.value("fruitType"), "Apple");

    // Call getRow with a valid tree index - "intermediate node"
    treeRow = model->getRow(model->index({0,1}, 0));
    QVERIFY(treeRow.isValid());
    rowValues = treeRow.toMap();

    QCOMPARE(rowValues.value("amount"), 1);
    QCOMPARE(rowValues.value("checked"), false);
    QCOMPARE(rowValues.value("color"), "yellow");
    QCOMPARE(rowValues.value("fruitName"), "Cavendish");
    QCOMPARE(rowValues.value("fruitPrice"), 3.5);
    QCOMPARE(rowValues.value("fruitType"), "Banana");
}

void tst_QQmlTreeModel::removeRow()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("common.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    int rowsChangedSignalEmissions = 0;

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);  // treeView cannot call our treeSize

    // Call removeRow with an invalid tree index (contains negative number)
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    model->removeRow(model->index({0,-5, 1}, 0));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // invalid tree index (nonexistent element)
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    model->removeRow(model->index({1, 13, 9}, 0));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // remove a leaf
    model->removeRow(model->index({0,1,0}, 0));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 7);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // remove a subtree
    std::vector<int> treeIndex = {1};
    model->removeRow(model->index(treeIndex, 0));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 4);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    // Wait until updatePolish() gets called, which is where the size is recalculated.
    QTRY_COMPARE(treeView->rows(), 1);
    QCOMPARE(treeView->columns(), 5);
}

void tst_QQmlTreeModel::setDataThroughDelegate()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("setDataThroughDelegate.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 2);
    QVERIFY(roleNames.values().contains("display"));
    QVERIFY(roleNames.values().contains("decoration"));

    // check the node on the top level
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"Apple"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"Granny Smith"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);

    // Check the leaf node at the index {0,1,1}
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("display")).toString(), u"Banana"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("display")).toString(), u"Cavendish"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("display")).toDouble(), 3.5);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("decoration")).toString(), u"yellow"_s);

    // in this example the tree is expanded, so every element should change
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "modify"));
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), 8);

    // check the node on the top level
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 18);     // here is the change, everything else is the same
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"Apple"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"Granny Smith"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);

    // Check the leaf node at the index {0,1,1}
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("display")).toInt(), 18);   // here is the change, everything else is the same
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("display")).toString(), u"Banana"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("display")).toString(), u"Cavendish"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("display")).toDouble(), 3.5);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("decoration")).toString(), u"yellow"_s);

    // Test setting a role that doesn't exist for a certain column.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "modifyInvalidRole"));
    // Everything is unchanged
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), 8);

    // check the node on the top level
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 18);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"Apple"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"Granny Smith"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);

    // Check the leaf node at the index {0,1,1}
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("display")).toInt(), 18);
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("display")).toString(), u"Banana"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("display")).toString(), u"Cavendish"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("display")).toDouble(), 3.5);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("decoration")).toString(), u"yellow"_s);

    // Test setting a role with a value of the wrong type.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* failed converting value"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* failed converting value"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* failed converting value"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* failed converting value"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* failed converting value"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* failed converting value"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* failed converting value"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* failed converting value"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "modifyInvalidType"));
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), 8);

    // check the node on the top level
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 18);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"Apple"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"Granny Smith"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);

    // Check the leaf node at the index {0,1,1}
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("display")).toInt(), 18);
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("display")).toString(), u"Banana"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("display")).toString(), u"Cavendish"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("display")).toDouble(), 3.5);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("decoration")).toString(), u"yellow"_s);
}

void tst_QQmlTreeModel::setRowsForEmptyModel()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("empty.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    int rowsChangedSignalEmissions = 0;

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 0);  // treeView cannot call our treeSize

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRows"));
    QCOMPARE(model->treeSize(), 2);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QTRY_COMPARE(treeView->rows(), 2);
    QCOMPARE(treeView->columns(), 5);

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 2);
    QVERIFY(roleNames.values().contains("display"));
    QVERIFY(roleNames.values().contains("decoration"));

    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 5);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"Strawberry"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"Perry the Berry"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 3.8);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);

    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 6);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("display")).toString(), u"Bear Pear"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);

    // the model is no longer empty, it can verify the input and reject invalid entries
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* expected the property named"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendInvalid"));
    // the tree does not change
    QCOMPARE(model->treeSize(), 2);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->rows(), 2);
    QCOMPARE(treeView->columns(), 5);

    // this also means that roles that were not present in the initial data will be rejected,
    // but the rest will be appended
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendWithExtraData"));
    QCOMPARE(model->treeSize(), 3);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    // Wait until updatePolish() gets called, which is where the size is recalculated.
    QTRY_COMPARE(treeView->rows(), 3);
    QCOMPARE(treeView->columns(), 5);

    // check the new node
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 2, QModelIndex()), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index(2, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 3, QModelIndex()), roleNames.key("display")).toString(), u"Williams"_s);
    QCOMPARE(model->data(model->index(2, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(2, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
}

void tst_QQmlTreeModel::setRowsOnNonEmptyModel()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("setRowsMultipleTimes.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(model->columnCount(), 5);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    int rowsChangedSignalEmissions = 0;

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);  // treeView cannot call our treeSize

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 2);
    QVERIFY(roleNames.values().contains("display"));
    QVERIFY(roleNames.values().contains("decoration"));

    // Try to set invalid data - it will be rejected, nothing changes
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* expected the property named"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRowsInvalid"));
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);  // treeView cannot call our treeSize

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRows"));
    QCOMPARE(model->treeSize(), 3);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    // Wait until updatePolish() gets called, which is where the size is recalculated.
    QTRY_COMPARE(treeView->rows(), 3);
    QCOMPARE(treeView->columns(), 5);

    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 5);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"Strawberry"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"Perry the Berry"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 3.8);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);

    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 6);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("display")).toString(), u"Bear Pear"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);

    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 4);
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index(2, 2, QModelIndex()), roleNames.key("display")).toString(), u"Orange"_s);
    QCOMPARE(model->data(model->index(2, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index(2, 3, QModelIndex()), roleNames.key("display")).toString(), u"Navel"_s);
    QCOMPARE(model->data(model->index(2, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index(2, 4, QModelIndex()), roleNames.key("display")).toDouble(), 2.5);
    QCOMPARE(model->data(model->index(2, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"orange"_s);
}

void tst_QQmlTreeModel::setRowsRejectsNonArray()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("empty.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    int rowsChangedSignalEmissions = 0;

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 0);  // treeView cannot call our treeSize

    // try to set a number
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*must be an array.*"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setInvalidRowsNumber"));
    // setRows is returning early, nothing changes
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // try to set a string
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*must be an array.*"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setInvalidRowsString"));
    // setRows is returning early, nothing changes
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // try to set a JSObject
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*but an array is expected"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setInvalidRowsObject"));
    // setRows is returning early, nothing changes
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // try to set an array that does not contain key-value pairs
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*does not contain.*"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*does not contain.*"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*does not contain.*"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setInvalidRowsArray"));
    // setRows is not returning early
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // try to append to an invalid row - int
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*does not contain.*"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendInvalidNumber"));
    // appendRow returns early - nothing changes
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // try to append to an invalid row - string
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*does not contain.*"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendInvalidString"));
    // appendRow returns early - nothing changes
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // try to append to an invalid row - array
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*does not contain.*"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendInvalidArray"));
    // appendRow returns early - nothing changes
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
}

void tst_QQmlTreeModel::setRowsFromJSON()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("setRowsViaJSON.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    // The tree has been initialized from JSON
    QCOMPARE(model->treeSize(), 4);
    QCOMPARE(model->columnCount(), 5);

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 1);
    QVERIFY(roleNames.values().contains("display"));

    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toString(), QStringLiteral("\u2014"));
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"folder"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"Documents"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toString(), u"2025-07-01"_s);

    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toString(), QStringLiteral("\u2014"));
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("display")).toString(), u"folder"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("display")).toString(), u"Pictures"_s);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("display")).toString(), u"2025-05-30"_s);

    QCOMPARE(model->data(model->index({0,0}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({0,0}, 1), roleNames.key("display")).toString(), u"24 KB"_s);
    QCOMPARE(model->data(model->index({0,0}, 2), roleNames.key("display")).toString(), u"file"_s);
    QCOMPARE(model->data(model->index({0,0}, 3), roleNames.key("display")).toString(), u"Resume.pdf"_s);
    QCOMPARE(model->data(model->index({0,0}, 4), roleNames.key("display")).toString(), u"2025-06-20"_s);

    QCOMPARE(model->data(model->index({1,0}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({1,0}, 1), roleNames.key("display")).toString(), u"3.5 MB"_s);
    QCOMPARE(model->data(model->index({1,0}, 2), roleNames.key("display")).toString(), u"file"_s);
    QCOMPARE(model->data(model->index({1,0}, 3), roleNames.key("display")).toString(), u"Vacation.jpg"_s);
    QCOMPARE(model->data(model->index({1,0}, 4), roleNames.key("display")).toString(), u"2025-05-15"_s);
}

void tst_QQmlTreeModel::setRow()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("common.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    int rowsChangedSignalEmissions = 0;

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 2);
    QVERIFY(roleNames.values().contains("display"));
    QVERIFY(roleNames.values().contains("decoration"));

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);  // treeView cannot call our treeSize

    // Try an invalid index - the index contains a negtive number
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* invalid modelIndex"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setWithNegativeIndex"));
    // Nothing happens
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // This index does not contain negative numbers but the node does not exist
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* invalid modelIndex"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setWithInvalidIndex"));
    // Nothing happens
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // This time the index is valid, but we are trying to insert an int
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* got int instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setInvalidData1"));
    // Nothing happens
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // The index is valid, but the row has children
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* child rows are not allowed"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setInvalidData2"));
    // Nothing happens
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // Valid index but the input is an array instead of a simple object.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* row manipulation functions do not support complex rows"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setInvalidData3"));
    // Nothing happens
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // Check the leaf node at the index {0,1,1}
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("display")).toString(), u"Banana"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("display")).toString(), u"Cavendish"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("display")).toDouble(), 3.5);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("decoration")).toString(), u"yellow"_s);

    // Call set and change the contents of this node
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setLeaf"));
    // the number of rows did not change, but the rows have changed
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    // The node after the change
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({0,1,1}, 0), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index({0,1,1}, 1), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 2), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("display")).toString(), u"Bear Pear"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 3), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index({0,1,1}, 4), roleNames.key("decoration")).toString(), u"green"_s);

    // Call set with a nonexistent role - that role is ignored
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setNodeWithExtraData"));

    // the structure of the tree did not change, only one row did
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);

    QCOMPARE(model->data(model->index({1,0}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({1,0}, 0), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({1,0}, 1), roleNames.key("display")).toInt(), 4);
    QCOMPARE(model->data(model->index({1,0}, 1), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({1,0}, 2), roleNames.key("display")).toString(), u"Orange"_s);
    QCOMPARE(model->data(model->index({1,0}, 2), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({1,0}, 3), roleNames.key("display")).toString(), u"Navel"_s);
    QCOMPARE(model->data(model->index({1,0}, 3), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({1,0}, 4), roleNames.key("display")).toDouble(), 2.5);
    QCOMPARE(model->data(model->index({1,0}, 4), roleNames.key("decoration")).toString(), u"orange"_s);
}

void tst_QQmlTreeModel::setData()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("setData.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    int rowsChangedSignalEmissions = 0;

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 2);
    QVERIFY(roleNames.values().contains("display"));
    QVERIFY(roleNames.values().contains("decoration"));

    // first top level node
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"Apple"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"Granny Smith"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);

    // model->index(0, 2, QModelIndex()): index to the fruit type "Apple"
    model->setData(model->index(0, 2, QModelIndex()), u"Passion fruit"_s, Qt::DisplayRole);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"Passion fruit"_s);

    // Test the other overload
    // model->index(0, 3, QModelIndex()): index to the fruit name "Granny Smith"
    model->setData(model->index(0, 3, QModelIndex()), u"My favorite fruit"_s, u"display"_s);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"My favorite fruit"_s);

    // Everything else is unchanged
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);

    // the other level node
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 4);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("display")).toString(), u"Peach"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("display")).toString(), u"Princess Peach"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.45);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);

    // now change the type on the JS side to "Ananas"
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "changeFruitTypeIntOverload"));
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("display")).toString(), u"Ananas"_s);

    // use the other overload and change the name on the JS side tp "My other favorite fruit"
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "changeFruitNameStringOverload"));
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("display")).toString(), u"My other favorite fruit"_s);

    // like before, everything else is unchanged
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 4);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.45);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);

    // check the first child of the first top level node
    QCOMPARE(model->data(model->index({0,0}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({0,0}, 0), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({0,0}, 1), roleNames.key("display")).toInt(), 4);
    QCOMPARE(model->data(model->index({0,0}, 1), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({0,0}, 2), roleNames.key("display")).toString(), u"Orange"_s);
    QCOMPARE(model->data(model->index({0,0}, 2), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({0,0}, 3), roleNames.key("display")).toString(), u"Navel"_s);
    QCOMPARE(model->data(model->index({0,0}, 3), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({0,0}, 4), roleNames.key("display")).toDouble(), 2.50);
    QCOMPARE(model->data(model->index({0,0}, 4), roleNames.key("decoration")).toString(), u"orange"_s);

    // This is probably not the most common use case, but it won't hurt to test if it works
    QModelIndex idxType = model->index({0,0}, 2);
    QString type = u"Weird fruit"_s;
    int role = Qt::DisplayRole;

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "changeFruitType", Q_ARG(QVariant, idxType), Q_ARG(QVariant, type), Q_ARG(QVariant, role)));
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(model->data(model->index({0,0}, 2), roleNames.key("display")).toString(), u"Weird fruit"_s);

    QModelIndex idxName = model->index({0,0}, 3);
    QString name = u"Unknown fruit"_s;
    QString roleAsString = u"display"_s;

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "changeFruitName", Q_ARG(QVariant, idxName), Q_ARG(QVariant, name), Q_ARG(QVariant, roleAsString)));
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    QCOMPARE(model->data(model->index({0,0}, 3), roleNames.key("display")).toString(), u"Unknown fruit"_s);

    // Everything else is unchanged
    QCOMPARE(model->data(model->index({0,0}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({0,0}, 0), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({0,0}, 1), roleNames.key("display")).toInt(), 4);
    QCOMPARE(model->data(model->index({0,0}, 1), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({0,0}, 2), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({0,0}, 3), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index({0,0}, 4), roleNames.key("display")).toDouble(), 2.50);
    QCOMPARE(model->data(model->index({0,0}, 4), roleNames.key("decoration")).toString(), u"orange"_s);
}

void tst_QQmlTreeModel::insertRow()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("common.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);  // treeView cannot call our treeSize

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 2);


    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowTopLevel"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 9);
    QCOMPARE(rowsChangedSpy.size(), 1);
    QCOMPARE(treeView->columns(), 5);
    QTRY_COMPARE(treeView->rows(), 3);

    auto firstIndex = std::vector<int>({0});

    QCOMPARE(model->data(model->index(firstIndex, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(firstIndex, 0), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index(firstIndex, 1), roleNames.key("display")).toInt(), 42);
    QCOMPARE(model->data(model->index(firstIndex, 1), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index(firstIndex, 2), roleNames.key("display")).toString(), u"InsertedOrange"_s);
    QCOMPARE(model->data(model->index(firstIndex, 2), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index(firstIndex, 3), roleNames.key("display")).toString(), u"InsertedNavel"_s);
    QCOMPARE(model->data(model->index(firstIndex, 3), roleNames.key("decoration")).toString(), u"orange"_s);


    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowTop"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 10);
    QCOMPARE(rowsChangedSpy.size(), 2);
    QCOMPARE(treeView->columns(), 5);
    QTRY_COMPARE(treeView->rows(), 3);

    QCOMPARE(model->data(model->index({0, 0}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({0, 0}, 0), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({0, 0}, 1), roleNames.key("display")).toInt(), 420);
    QCOMPARE(model->data(model->index({0, 0}, 2), roleNames.key("display")).toString(), u"InsertedOrange2"_s);
    QCOMPARE(model->data(model->index({0, 0}, 3), roleNames.key("display")).toString(), u"InsertedNavel2"_s);


    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRow"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 11);
    QCOMPARE(rowsChangedSpy.size(), 3);
    QCOMPARE(treeView->columns(), 5);
    QTRY_COMPARE(treeView->rows(), 3);

    QCOMPARE(model->data(model->index({0, 1}, 0), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index({0, 1}, 0), roleNames.key("decoration")).toString(), u"blue"_s);
    QCOMPARE(model->data(model->index({0, 1}, 1), roleNames.key("display")).toInt(), 4200);
    QCOMPARE(model->data(model->index({0, 1}, 2), roleNames.key("display")).toString(), u"InsertedOrange3"_s);
    QCOMPARE(model->data(model->index({0, 1}, 3), roleNames.key("display")).toString(), u"InsertedNavel3"_s);

    QCOMPARE(treeView->rows(), 3);
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowAtEnd"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 12);
    QCOMPARE(rowsChangedSpy.size(), 4);
    QCOMPARE(treeView->columns(), 5);
    QTRY_COMPARE(treeView->rows(), 4);

    auto lasttIndex = std::vector<int>({treeView->rows() - 1});
    QCOMPARE(model->data(model->index(lasttIndex, 0), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(lasttIndex, 0), roleNames.key("decoration")).toString(), u"black"_s);
    QCOMPARE(model->data(model->index(lasttIndex, 1), roleNames.key("display")).toInt(), 100);
    QCOMPARE(model->data(model->index(lasttIndex, 2), roleNames.key("display")).toString(), u"InsertedOrangeEnd"_s);
    QCOMPARE(model->data(model->index(lasttIndex, 3), roleNames.key("display")).toString(), u"InsertedNavelEnd"_s);

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertWithChildren"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 15);
    QCOMPARE(rowsChangedSpy.size(), 5);
    QCOMPARE(treeView->columns(), 5);
    QTRY_COMPARE(treeView->rows(), 4);

    QCOMPARE(model->data(model->index({0, 0}, 2), roleNames.key("display")).toString(), u"ParentFruit"_s);
    QCOMPARE(model->data(model->index({0, 0}, 3), roleNames.key("display")).toString(), u"ParentFruit"_s);

    QCOMPARE(model->data(model->index({0, 0, 0}, 2), roleNames.key("display")).toString(), u"BabyFruit"_s);
    QCOMPARE(model->data(model->index({0, 0, 0}, 3), roleNames.key("display")).toString(), u"BabyFruit"_s);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*is greater than rowCount().*"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowNonExisingIndex"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 15);
    QCOMPARE(rowsChangedSpy.size(), 5);
    QCOMPARE(treeView->columns(), 5);
    QTRY_COMPARE(treeView->rows(), 4);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*expected.*"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertInvalidData"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 15);
    QCOMPARE(rowsChangedSpy.size(), 5);
    QCOMPARE(treeView->columns(), 5);
    QTRY_COMPARE(treeView->rows(), 4);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*expected a property.*"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertInvalidData2"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 15);
    QCOMPARE(rowsChangedSpy.size(), 5);
    QCOMPARE(treeView->columns(), 5);
    QTRY_COMPARE(treeView->rows(), 4);
}

void tst_QQmlTreeModel::insertRowEmptyModel()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("empty.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    int rowsChangedSignalEmissions = 0;

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 0);  // treeView cannot call our treeSize

    // trying to insert to the root
    // since it does not have any children the index check will reject it
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*is greater than rowCount().*"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowAtInvalidIndex"));
    // nothing has changed
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // but even in an empty tree, it is possible to insert a row if it is the
    // first child of the root - at this point everything else is rejected
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowAsFirstChildAsRoot"));
    QCOMPARE(model->treeSize(), 1);
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);
    // Wait until updatePolish() gets called, which is where the size is recalculated.
    QTRY_COMPARE(treeView->rows(), 1);
    QCOMPARE(treeView->columns(), 5);

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 2);
    QVERIFY(roleNames.values().contains("display"));
    QVERIFY(roleNames.values().contains("decoration"));

    // check the node
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 42);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"InsertedOrange"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"InsertedNavel"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"orange"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 2.50);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"orange"_s);
}

void tst_QQmlTreeModel::moveRows()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("simple.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->treeSize(), 28);

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    QCOMPARE(rowsChangedSpy.count(), 0);

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);

    QCOMPARE(treeView->columns(), 2);
    QCOMPARE(treeView->rows(), 2);   //treeView cannot call our treeSize

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 1);

    // rowCount is the same as childCount
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->rowCount(model->index(std::vector<int>{0}, 0)), 5);
    QCOMPARE(model->rowCount(model->index(std::vector<int>{1}, 0)), 5);
    QCOMPARE(model->rowCount(model->index({0,0}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({0,1}, 0)), 6);
    QCOMPARE(model->rowCount(model->index({0,2}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({0,3}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({0,4}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({1,0}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({1,1}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({1,2}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({1,3}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({1,4}, 0)), 10);

    for (int i = 0; i < 10; i++)
        QCOMPARE(model->data(model->index({1, 4, i}, 1), roleNames.key("display")).toInt(), i);

    QModelIndex parentIndex = model->index({1,4}, 0);

    // All destination indices use pre-removal semantics (as required by beginMoveRows).

    model->moveRows(parentIndex, 2, 4, parentIndex, 10);
    std::array<int, 10> target = {0, 1, 6, 7, 8, 9, 2, 3, 4, 5};
    for (int i = 0; i < 10; i++)
        QCOMPARE(model->data(model->index({1, 4, i}, 1), roleNames.key("display")).toInt(), target[i]);

    model->moveRows(parentIndex, 6, 4, parentIndex, 2);
    for (int i = 0; i < 10; i++)
        QCOMPARE(model->data(model->index({1, 4, i}, 1), roleNames.key("display")).toInt(), i);

    model->moveRows(parentIndex, 2, 4, parentIndex, 7);
    target = {0, 1, 6, 2, 3, 4, 5, 7, 8, 9};
    for (int i = 0; i < 10; i++)
        QCOMPARE(model->data(model->index({1, 4, i}, 1), roleNames.key("display")).toInt(), target[i]);

    model->moveRows(parentIndex, 2, 1, parentIndex, 7);
    for (int i = 0; i < 10; i++)
        QCOMPARE(model->data(model->index({1, 4, i}, 1), roleNames.key("display")).toInt(), i);

    model->moveRows(parentIndex, 5, 3, parentIndex, 1);
    target = {0, 5, 6, 7, 1, 2, 3, 4, 8, 9};
    for (int i = 0; i < 10; i++)
        QCOMPARE(model->data(model->index({1, 4, i}, 1), roleNames.key("display")).toInt(), target[i]);

    model->moveRows(parentIndex, 1, 3, parentIndex, 8);
    for (int i = 0; i < 10; i++)
        QCOMPARE(model->data(model->index({1, 4, i}, 1), roleNames.key("display")).toInt(), i);

    model->moveRows(parentIndex, 5, 5, parentIndex, 2);
    target = {0, 1, 5, 6, 7, 8, 9, 2, 3, 4};
    for (int i = 0; i < 10; i++)
        QCOMPARE(model->data(model->index({1, 4, i}, 1), roleNames.key("display")).toInt(), target[i]);

    model->moveRows(parentIndex, 2, 5, parentIndex, 10);
    for (int i = 0; i < 10; i++)
        QCOMPARE(model->data(model->index({1, 4, i}, 1), roleNames.key("display")).toInt(), i);

    QCOMPARE(model->moveRows(parentIndex, 1, 2, parentIndex, 11), false);
    for (int i = 0; i < 10; i++)
        QCOMPARE(model->data(model->index({1, 4, i}, 1), roleNames.key("display")).toInt(), i);
}

void tst_QQmlTreeModel::moveRowsAcrossNodes()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("simple.qml")));
    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->treeSize(), 28);

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    QCOMPARE(rowsChangedSpy.count(), 0);

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 2);
    QCOMPARE(treeView->rows(), 2);

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 1);

    // rowCount is the same as childCount
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->rowCount(model->index(std::vector<int>{0}, 0)), 5);
    QCOMPARE(model->rowCount(model->index(std::vector<int>{1}, 0)), 5);
    QCOMPARE(model->rowCount(model->index({0,0}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({0,1}, 0)), 6);
    QCOMPARE(model->rowCount(model->index({0,2}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({0,3}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({0,4}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({1,0}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({1,1}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({1,2}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({1,3}, 0)), 0);
    QCOMPARE(model->rowCount(model->index({1,4}, 0)), 10);

    // Cross-parent moves: destination indices are unchanged because
    // the removal from one parent does not affect the other parent's
    // child indices.

    // move from [0, 1], 2..3 to [1,0], 0..
    model->moveRows(model->index({0,1}, 0), 2, 2,
                    model->index({1,0}, 0), 0);
    QCOMPARE(rowsChangedSpy.count(), 1);
    QCOMPARE(model->treeSize(), 28);
    QVERIFY(model->rowCount(model->index({0,1}, 0)) == 4); // was 6
    QVERIFY(model->rowCount(model->index({1,0}, 0)) == 2); // was 0
    QCOMPARE(model->data(model->index({1, 0, 0}, 0), roleNames.key("display")).toString(), "[0,1,2]");
    QCOMPARE(model->data(model->index({1, 0, 1}, 0), roleNames.key("display")).toString(), "[0,1,3]");

    // revert
    model->moveRows(model->index({1,0}, 0), 0, 2,
                    model->index({0,1}, 0), 2);
    QCOMPARE(rowsChangedSpy.count(), 2);
    QCOMPARE(model->treeSize(), 28);
    QVERIFY(model->rowCount(model->index({0,1}, 0)) == 6);
    QVERIFY(model->rowCount(model->index({1,0}, 0)) == 0);
    QCOMPARE(model->data(model->index({0, 1, 2}, 0), roleNames.key("display")).toString(), "[0,1,2]");
    QCOMPARE(model->data(model->index({0, 1, 3}, 0), roleNames.key("display")).toString(), "[0,1,3]");

    // move from [0, 1], 2..10 to [0], 0.. — too many rows
    // beginMoveRows rejects silently (sourceRow + count > rowCount)
    QCOMPARE(model->moveRows(model->index({0,1}, 0), 2, 8,
                             model->index(std::vector<int>{0}, 0), 0), false);
    QCOMPARE(rowsChangedSpy.count(), 2);
    QCOMPARE(model->treeSize(), 28);

    // move from [0], 0..2 to [0,1], 0.. — would move parent under its own child
    // beginMoveRows detects ancestor-descendant overlap and rejects
    QCOMPARE(model->moveRows(model->index(std::vector<int>{0}, 0), 0, 2,
                             model->index({0,1}, 0), 0), false);
    QCOMPARE(rowsChangedSpy.count(), 2);
    QCOMPARE(model->treeSize(), 28);

    // move from [0], 0 to [0,1], 0 — sibling of the destination, this is fine
    model->moveRows(model->index(std::vector<int>{0}, 0), 0, 1,
                    model->index({0,1}, 0), 0);
    QCOMPARE(rowsChangedSpy.count(), 3);
    QCOMPARE(model->treeSize(), 28);
    // [0,0] is gone: [0,1] is there now:
    QCOMPARE(model->data(model->index({0, 0}, 0), roleNames.key("display")).toString(), "[0,1]");
    // it is here (this is confusing but ok)
    QCOMPARE(model->data(model->index({0, 0, 0}, 0), roleNames.key("display")).toString(), "[0,0]");

    // revert
    model->moveRows(model->index({0,0}, 0), 0, 1,
                    model->index(std::vector<int>{0}, 0), 0);
    QCOMPARE(rowsChangedSpy.count(), 4);
    QCOMPARE(model->treeSize(), 28);
    QCOMPARE(model->data(model->index({0, 0}, 0), roleNames.key("display")).toString(), "[0,0]");
    QCOMPARE(model->data(model->index({0, 1}, 0), roleNames.key("display")).toString(), "[0,1]");
}

QTEST_MAIN(tst_QQmlTreeModel)

#include "tst_qqmltreemodel.moc"
