/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicktableview_p.h>

#include "../../shared/util.h"

class tst_QQmlTableModel : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQmlTableModel() {}

private slots:
    void appendRemoveRow();
    void appendRowToEmptyModel();
    void clear();
    void getRow();
    void insertRow();
    void moveRow();
    void setRow();
    void setDataThroughDelegate();
    void setRowsImperatively();
    void setRowsMultipleTimes();
    void dataAndEditing();
    void omitTableModelColumnIndex();
    void complexRow();
    void appendRowWithDouble();
};

void tst_QQmlTableModel::appendRemoveRow()
{
    QQuickView view(testFileUrl("common.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("testModel") .value<QAbstractTableModel *>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());
    int rowCountSignalEmissions = 0;

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 1);
    QVERIFY(roleNames.values().contains("display"));
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);

    // Call remove() with a negative rowIndex.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*removeRow\\(\\): \"rowIndex\" cannot be negative"));
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, -1)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Call remove() with an rowIndex that is too large.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*removeRow\\(\\): \"rowIndex\" 2 is greater than or equal to rowCount\\(\\) of 2"));
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, 2)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Call remove() with a valid rowIndex but negative rows.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*removeRow\\(\\): \"rows\" is less than or equal to zero"));
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, 0), Q_ARG(int, -1)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Call remove() with a valid rowIndex but excessive rows.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*removeRow\\(\\): \"rows\" 3 exceeds available rowCount\\(\\) of 2 when removing from \"rowIndex\" 0"));
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, 0), Q_ARG(int, 3)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Call remove() without specifying the number of rows to remove; it should remove one row.
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, 0)));
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++rowCountSignalEmissions);

    // Call append() with a row that has an unexpected role; the row should be added and the extra data ignored.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRowExtraData"));
    // Nothing should change.
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Foo"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++rowCountSignalEmissions);

    // Call append() with a row that is an int.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*appendRow\\(\\): expected \"row\" argument to be a QJSValue, but got int instead:\nQVariant\\(int, 123\\)"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRowInvalid1"));
    // Nothing should change.
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Foo"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Call append() with a row with a role of the wrong type.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*appendRow\\(\\): expected the property named \"age\" to be of type \"int\", but got \"QVariantList\" instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRowInvalid2"));
    // Nothing should change.
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Foo"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Call append() with a row that is an array instead of a simple object.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*appendRow\\(\\): row manipulation functions do not support complex rows \\(row index: -1\\)"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRowInvalid3"));
    // Nothing should change.
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Call append() to insert one row.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRow", Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40)));
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Foo"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 40);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++rowCountSignalEmissions);

    // Call remove() and specify rowIndex and rows, removing all remaining rows.
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, 0), Q_ARG(int, 3)));
    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")), QVariant());
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")), QVariant());
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++rowCountSignalEmissions);
}

void tst_QQmlTableModel::appendRowToEmptyModel()
{
    QQuickView view(testFileUrl("empty.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("testModel").value<QAbstractTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());

    QQuickTableView *tableView = view.rootObject()->property("tableView").value<QQuickTableView*>();
    QVERIFY(tableView);
    QCOMPARE(tableView->rows(), 0);
    QCOMPARE(tableView->columns(), 2);

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendJohn"));
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->columnCount(), 2);
    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 1);
    QTRY_COMPARE(tableView->rows(), 1);
    QCOMPARE(tableView->columns(), 2);
}

void tst_QQmlTableModel::clear()
{
    QQuickView view(testFileUrl("common.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("testModel").value<QAbstractTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QVERIFY(roleNames.values().contains("display"));
    QCOMPARE(roleNames.size(), 1);

    QQuickTableView *tableView = view.rootObject()->property("tableView").value<QQuickTableView*>();
    QVERIFY(tableView);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    QVERIFY(QMetaObject::invokeMethod(model, "clear"));
    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")), QVariant());
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")), QVariant());
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 1);
    // Wait until updatePolish() gets called, which is where the size is recalculated.
    QTRY_COMPARE(tableView->rows(), 0);
    QCOMPARE(tableView->columns(), 2);
}

void tst_QQmlTableModel::getRow()
{
    QQuickView view(testFileUrl("common.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("testModel").value<QAbstractTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    // Call get() with a negative row index.
    QVariant returnValue;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*getRow\\(\\): \"rowIndex\" cannot be negative"));
    QVERIFY(QMetaObject::invokeMethod(model, "getRow", Q_RETURN_ARG(QVariant, returnValue), Q_ARG(int, -1)));
    QVERIFY(!returnValue.isValid());

    // Call get() with a row index that is too large.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*getRow\\(\\): \"rowIndex\" 2 is greater than or equal to rowCount\\(\\) of 2"));
    QVERIFY(QMetaObject::invokeMethod(model, "getRow", Q_RETURN_ARG(QVariant, returnValue), Q_ARG(int, 2)));
    QVERIFY(!returnValue.isValid());

    // Call get() with a valid row index.
    QVERIFY(QMetaObject::invokeMethod(model, "getRow", Q_RETURN_ARG(QVariant, returnValue), Q_ARG(int, 0)));
    const QVariantMap rowAsVariantMap = returnValue.toMap();
    QCOMPARE(rowAsVariantMap.value(QLatin1String("name")).toString(), QLatin1String("John"));
    QCOMPARE(rowAsVariantMap.value(QLatin1String("age")).toInt(), 22);
}

void tst_QQmlTableModel::insertRow()
{
    QQuickView view(testFileUrl("common.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("testModel").value<QAbstractTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());
    int rowCountSignalEmissions = 0;

    QQuickTableView *tableView = view.rootObject()->property("tableView").value<QQuickTableView*>();
    QVERIFY(tableView);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert with a negative index.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*insertRow\\(\\): \"rowIndex\" cannot be negative"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRow",
        Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40), Q_ARG(QVariant, -1)));
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->rowCount(), 2);
    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert past the last allowed index.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*insertRow\\(\\): \"rowIndex\" 3 is greater than rowCount\\(\\) of 2"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRow",
        Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40), Q_ARG(QVariant, 3)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Call insert() with a row that is an int.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*insertRow\\(\\): expected \"row\" argument to be a QJSValue, but got int instead:\nQVariant\\(int, 123\\)"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowInvalid1"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row with a role of the wrong type.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*insertRow\\(\\): expected the property named \"age\" to be of type \"int\", but got \"QVariantList\" instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowInvalid2"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row that is an array instead of a simple object.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*insertRow\\(\\): row manipulation functions do not support complex rows \\(row index: 0\\)"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowInvalid3"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row has an unexpected role; the row should be added and the extra data ignored.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowExtraData"));
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Foo"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++rowCountSignalEmissions);
    QTRY_COMPARE(tableView->rows(), 3);
    QCOMPARE(tableView->columns(), 2);

    // Insert a row at the bottom of the table.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRow",
        Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40), Q_ARG(QVariant, 3)));
    QCOMPARE(model->rowCount(), 4);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Foo"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("display")).toInt(), 40);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++rowCountSignalEmissions);
    QTRY_COMPARE(tableView->rows(), 4);
    QCOMPARE(tableView->columns(), 2);

    // Insert a row in the middle of the table.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRow",
        Q_ARG(QVariant, QLatin1String("Daisy")), Q_ARG(QVariant, 30), Q_ARG(QVariant, 2)));
    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Foo"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 30);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("display")).toInt(), 40);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++rowCountSignalEmissions);
    QTRY_COMPARE(tableView->rows(), 5);
    QCOMPARE(tableView->columns(), 2);
}

void tst_QQmlTableModel::moveRow()
{
    QQuickView view(testFileUrl("common.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("testModel").value<QAbstractTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->rowCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());
    int rowCountSignalEmissions = 0;

    const QHash<int, QByteArray> roleNames = model->roleNames();

    // Append some rows.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRow", Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40)));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRow", Q_ARG(QVariant, QLatin1String("Daisy")), Q_ARG(QVariant, 30)));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRow", Q_ARG(QVariant, QLatin1String("Trev")), Q_ARG(QVariant, 48)));
    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 40);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("display")).toInt(), 30);
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("display")).toInt(), 48);
    rowCountSignalEmissions = 3;
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Try to move with a fromRowIndex that is negative.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*moveRow\\(\\): \"fromRowIndex\" cannot be negative"));
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, -1), Q_ARG(int, 1)));
    // Shouldn't have changed.
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("display")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Try to move with a fromRowIndex that is too large.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*moveRow\\(\\): \"fromRowIndex\" 5 is greater than or equal to rowCount\\(\\)"));
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 5), Q_ARG(int, 1)));
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("display")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Try to move with a toRowIndex that is negative.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*moveRow\\(\\): \"toRowIndex\" cannot be negative"));
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 0), Q_ARG(int, -1)));
    // Shouldn't have changed.
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("display")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Try to move with a toRowIndex that is too large.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*moveRow\\(\\): \"toRowIndex\" 5 is greater than or equal to rowCount\\(\\)"));
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 0), Q_ARG(int, 5)));
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("display")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Move the first row to the end.
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 0), Q_ARG(int, 4)));
    // The counts shouldn't have changed.
    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 40);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 30);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("display")).toInt(), 48);
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Move it back again.
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 4), Q_ARG(int, 0)));
    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 40);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("display")).toInt(), 30);
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("display")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);

    // Move the first row down one by one row.
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 0), Q_ARG(int, 1)));
    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 40);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("display")).toInt(), 30);
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("display")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
}

void tst_QQmlTableModel::setRow()
{
    QQuickView view(testFileUrl("common.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("testModel").value<QAbstractTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->rowCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());
    int rowCountSignalEmissions = 0;

    QQuickTableView *tableView = view.rootObject()->property("tableView").value<QQuickTableView*>();
    QVERIFY(tableView);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to set with a negative index.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*setRow\\(\\): \"rowIndex\" cannot be negative"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRow",
        Q_ARG(QVariant, -1), Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to set at an index past the last allowed index.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*setRow\\(\\): \"rowIndex\" 3 is greater than rowCount\\(\\) of 2"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRow",
        Q_ARG(QVariant, 3), Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to set a row that has an unexpected role; the row should be set and the extra data ignored.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRowExtraData"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Foo"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row that is not an array.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*setRow\\(\\): expected \"row\" argument to be a QJSValue, but got int instead:\nQVariant\\(int, 123\\)"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRowInvalid1"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Foo"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row with a role that is of the wrong type.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*setRow\\(\\): expected the property named \"age\" to be of type \"int\", but got \"QVariantList\" instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRowInvalid2"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Foo"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row that is an array instead of a simple object.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*setRow\\(\\): row manipulation functions do not support complex rows \\(row index: 0\\)"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRowInvalid3"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Foo"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Set the first row.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRow",
        Q_ARG(QVariant, 0), Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 40);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Set the last row.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRow",
        Q_ARG(QVariant, 1), Q_ARG(QVariant, QLatin1String("Daisy")), Q_ARG(QVariant, 30)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 40);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 30);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), rowCountSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Append a row by passing an index that is equal to rowCount().
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRow",
        Q_ARG(QVariant, 2), Q_ARG(QVariant, QLatin1String("Wot")), Q_ARG(QVariant, 99)));
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 40);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 30);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Wot"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 99);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++rowCountSignalEmissions);
    QTRY_COMPARE(tableView->rows(), 3);
    QCOMPARE(tableView->columns(), 2);
}

void tst_QQmlTableModel::setDataThroughDelegate()
{
    QQuickView view(testFileUrl("setDataThroughDelegate.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("testModel").value<QAbstractTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 1);
    QVERIFY(roleNames.values().contains("display"));
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 0);

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "modify"));
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 18);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 18);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 0);

    // Test setting a role that doesn't exist for a certain column.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "modifyInvalidRole"));
    // Should be unchanged.
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 18);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 18);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 0);

    // Test setting a role with a value of the wrong type.
    // There are two rows, so two delegates respond to the signal, which means we need to ignore two warnings.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*setData\\(\\): failed converting value QVariant\\(QString, \"Whoops\"\\) " \
        "set at row 0 column 1 with role \"display\" to \"int\""));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*setData\\(\\): failed converting value QVariant\\(QString, \"Whoops\"\\) " \
        "set at row 1 column 1 with role \"display\" to \"int\""));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "modifyInvalidType"));
    // Should be unchanged.
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 18);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 18);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 0);
}

// Start off with empty rows and then set them to test rowCountChanged().
void tst_QQmlTableModel::setRowsImperatively()
{
    QQuickView view(testFileUrl("empty.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("testModel").value<QAbstractTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());

    QQuickTableView *tableView = view.rootObject()->property("tableView").value<QQuickTableView*>();
    QVERIFY(tableView);
    QCOMPARE(tableView->rows(), 0);
    QCOMPARE(tableView->columns(), 2);

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRows"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 1);
    QTRY_COMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);
}

void tst_QQmlTableModel::setRowsMultipleTimes()
{
    QQuickView view(testFileUrl("setRowsMultipleTimes.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("testModel").value<QAbstractTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());

    QQuickTableView *tableView = view.rootObject()->property("tableView").value<QQuickTableView*>();
    QVERIFY(tableView);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Set valid rows after they've already been declared.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRowsValid"));
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->columnCount(), 2);
    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 20);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Imum"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 41);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Power"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 89);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 1);
    QTRY_COMPARE(tableView->rows(), 3);
    QCOMPARE(tableView->columns(), 2);

    // Set invalid rows; we should get a warning and nothing should change.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*setRows\\(\\): expected a property named \"name\" in row at index 0, but couldn't find one"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRowsInvalid"));
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 20);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Imum"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 41);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Power"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 89);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 1);
    QCOMPARE(tableView->rows(), 3);
    QCOMPARE(tableView->columns(), 2);
}

void tst_QQmlTableModel::dataAndEditing()
{
    QQuickView view(testFileUrl("dataAndSetData.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("model").value<QAbstractTableModel*>();
    QVERIFY(model);

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QVERIFY(roleNames.values().contains("display"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
    QVERIFY(QMetaObject::invokeMethod(model, "happyBirthday", Q_ARG(QVariant, QLatin1String("Oliver"))));
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 34);
}

void tst_QQmlTableModel::omitTableModelColumnIndex()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("omitTableModelColumnIndex.qml"));
    QCOMPARE(component.status(), QQmlComponent::Ready);

    QScopedPointer<QAbstractTableModel> model(qobject_cast<QAbstractTableModel*>(component.create()));
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
}

void tst_QQmlTableModel::complexRow()
{
    QQuickView view(testFileUrl("complex.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickTableView *tableView = qobject_cast<QQuickTableView*>(view.rootObject());
    QVERIFY(tableView);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    auto *model = tableView->model().value<QAbstractTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
}

void tst_QQmlTableModel::appendRowWithDouble()
{
    QQuickView view(testFileUrl("intAndDouble.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    auto *model = view.rootObject()->property("testModel").value<QAbstractTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());

    QQuickTableView *tableView = view.rootObject()->property("tableView").value<QQuickTableView*>();
    QVERIFY(tableView);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendBanana"));
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->columnCount(), 2);
    const QHash<int, QByteArray> roleNames = model->roleNames();
    const int roleKey = roleNames.key("display");
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleKey).toString(),
             QLatin1String("1"));
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleKey).toString(),
             QLatin1String("Banana"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleKey).toDouble(), 3.5);
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleKey).toString(),
             QLatin1String("3.5"));
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 1);
    QTRY_COMPARE(tableView->rows(), 3);
    QCOMPARE(tableView->columns(), 2);

    rowCountSpy.clear();

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendStrawberry"));
    QCOMPARE(model->rowCount(), 4);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleKey).toString(),
             QLatin1String("Strawberry"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleKey).toDouble(), 5);
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleKey).toString(),
             QLatin1String("5"));
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 1);
    QTRY_COMPARE(tableView->rows(), 4);
    QCOMPARE(tableView->columns(), 2);

    rowCountSpy.clear();
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(".*appendRow\\(\\): failed converting value "
                                            "QVariant\\(QString, \"Invalid\"\\) set at column 1 with "
                                            "role \"QString\" to \"int\""));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendInvalid"));
    // Nothing should change
    QCOMPARE(model->rowCount(), 4);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 0);
    QCOMPARE(tableView->rows(), 4);
    QCOMPARE(tableView->columns(), 2);
}

QTEST_MAIN(tst_QQmlTableModel)

#include "tst_qqmltablemodel.moc"
