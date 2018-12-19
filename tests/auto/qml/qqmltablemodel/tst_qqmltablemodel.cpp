/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/private/qqmltablemodel_p.h>
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
    void clear();
    void getRow();
    void insertRow();
    void moveRow();
    void setRow();
    void setDataThroughDelegate();
    void setRowsImperatively();
    void setRowsMultipleTimes();
    void defaultDisplayRoles();
    void roleDataProvider();
};

void tst_QQmlTableModel::appendRemoveRow()
{
    QQuickView view(testFileUrl("common.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQmlTableModel *model = view.rootObject()->property("testModel").value<QQmlTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());
    int heightSignalEmissions = 0;

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 3);
    QVERIFY(roleNames.values().contains("name"));
    QVERIFY(roleNames.values().contains("age"));
    QVERIFY(roleNames.values().contains("display"));

    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);

    // Call remove() with a negative rowIndex.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*removeRow\\(\\): \"rowIndex\" cannot be negative"));
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, -1)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Call remove() with an rowIndex that is too large.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*removeRow\\(\\): \"rowIndex\" 2 is greater than or equal to rowCount\\(\\) of 2"));
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, 2)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Call remove() with a valid rowIndex but negative rows.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*removeRow\\(\\): \"rows\" is less than or equal to zero"));
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, 0), Q_ARG(int, -1)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Call remove() with a valid rowIndex but excessive rows.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*removeRow\\(\\): \"rows\" 3 exceeds available rowCount\\(\\) of 2 when removing from \"rowIndex\" 0"));
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, 0), Q_ARG(int, 3)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Call remove() without specifying the number of rows to remove; it should remove one row.
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, 0)));
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++heightSignalEmissions);

    // Call append() with a row that has a new (and hence unexpected) role.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*appendRow\\(\\): expected 2 columns, but got 3"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRowInvalid1"));
    // Nothing should change.
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Call append() with a row that is not an array.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*appendRow\\(\\): expected \"row\" argument to be an array, but got int instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRowInvalid2"));
    // Nothing should change.
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Call append() with a row with a role that is of the wrong type.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*appendRow\\(\\): expected property with type int at column index 1, but got QVariantList instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRowInvalid3"));
    // Nothing should change.
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Call append() to insert one row.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRow", Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 40);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++heightSignalEmissions);

    // Call remove() and specify rowIndex and rows, removing all remaining rows.
    QVERIFY(QMetaObject::invokeMethod(model, "removeRow", Q_ARG(int, 0), Q_ARG(int, 2)));
    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")), QVariant());
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")), QVariant());
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++heightSignalEmissions);
}

void tst_QQmlTableModel::clear()
{
    QQuickView view(testFileUrl("common.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQmlTableModel *model = view.rootObject()->property("testModel").value<QQmlTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 3);
    QVERIFY(roleNames.values().contains("name"));
    QVERIFY(roleNames.values().contains("age"));
    QVERIFY(roleNames.values().contains("display"));

    QQuickTableView *tableView = view.rootObject()->property("tableView").value<QQuickTableView*>();
    QVERIFY(tableView);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    QVERIFY(QMetaObject::invokeMethod(model, "clear"));
    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")), QVariant());
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")), QVariant());
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

    QQmlTableModel *model = view.rootObject()->property("testModel").value<QQmlTableModel*>();
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
    const QVariantList rowAsVariantList = returnValue.toList();
    QCOMPARE(rowAsVariantList.at(0).toMap().value(QLatin1String("name")), QLatin1String("John"));
    QCOMPARE(rowAsVariantList.at(1).toMap().value(QLatin1String("age")), 22);
}

void tst_QQmlTableModel::insertRow()
{
    QQuickView view(testFileUrl("common.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQmlTableModel *model = view.rootObject()->property("testModel").value<QQmlTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());
    int heightSignalEmissions = 0;

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
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert past the last allowed index.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*insertRow\\(\\): \"rowIndex\" 3 is greater than rowCount\\(\\) of 2"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRow",
        Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40), Q_ARG(QVariant, 3)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row that has a new (and hence unexpected) role.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*insertRow\\(\\): expected 2 columns, but got 3"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowInvalid1"));
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row that is not an array.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*insertRow\\(\\): expected \"row\" argument to be an array, but got int instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowInvalid2"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row with a role that is of the wrong type.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*insertRow\\(\\): expected property with type int at column index 1, but got QVariantList instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRowInvalid3"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Insert a row at the bottom of the table.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRow",
        Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40), Q_ARG(QVariant, 2)));
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("age")).toInt(), 40);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++heightSignalEmissions);
    QTRY_COMPARE(tableView->rows(), 3);
    QCOMPARE(tableView->columns(), 2);

    // Insert a row in the middle of the table.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "insertRow",
        Q_ARG(QVariant, QLatin1String("Daisy")), Q_ARG(QVariant, 30), Q_ARG(QVariant, 1)));
    QCOMPARE(model->rowCount(), 4);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 30);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("age")).toInt(), 40);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++heightSignalEmissions);
    QTRY_COMPARE(tableView->rows(), 4);
    QCOMPARE(tableView->columns(), 2);
}

void tst_QQmlTableModel::moveRow()
{
    QQuickView view(testFileUrl("common.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQmlTableModel *model = view.rootObject()->property("testModel").value<QQmlTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->rowCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());
    int heightSignalEmissions = 0;

    const QHash<int, QByteArray> roleNames = model->roleNames();

    // Append some rows.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRow", Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40)));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRow", Q_ARG(QVariant, QLatin1String("Daisy")), Q_ARG(QVariant, 30)));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendRow", Q_ARG(QVariant, QLatin1String("Trev")), Q_ARG(QVariant, 48)));
    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("age")).toInt(), 40);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("age")).toInt(), 30);
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("age")).toInt(), 48);
    heightSignalEmissions = 3;
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Try to move with a fromRowIndex that is negative.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*moveRow\\(\\): \"fromRowIndex\" cannot be negative"));
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, -1), Q_ARG(int, 1)));
    // Shouldn't have changed.
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("age")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Try to move with a fromRowIndex that is too large.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*moveRow\\(\\): \"fromRowIndex\" 5 is greater than or equal to rowCount\\(\\)"));
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 5), Q_ARG(int, 1)));
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("age")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Try to move with a toRowIndex that is negative.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*moveRow\\(\\): \"toRowIndex\" cannot be negative"));
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 0), Q_ARG(int, -1)));
    // Shouldn't have changed.
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("age")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Try to move with a toRowIndex that is too large.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*moveRow\\(\\): \"toRowIndex\" 5 is greater than or equal to rowCount\\(\\)"));
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 0), Q_ARG(int, 5)));
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("age")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Move the first row to the end.
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 0), Q_ARG(int, 4)));
    // The counts shouldn't have changed.
    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 40);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("age")).toInt(), 30);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("age")).toInt(), 48);
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Move it back again.
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 4), Q_ARG(int, 0)));
    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("age")).toInt(), 40);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("age")).toInt(), 30);
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("age")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);

    // Move the first row down one by one row.
    QVERIFY(QMetaObject::invokeMethod(model, "moveRow", Q_ARG(int, 0), Q_ARG(int, 1)));
    QCOMPARE(model->rowCount(), 5);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("age")).toInt(), 40);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("age")).toInt(), 30);
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Trev"));
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("age")).toInt(), 48);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
}

void tst_QQmlTableModel::setRow()
{
    QQuickView view(testFileUrl("common.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQmlTableModel *model = view.rootObject()->property("testModel").value<QQmlTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->rowCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());
    int heightSignalEmissions = 0;

    QQuickTableView *tableView = view.rootObject()->property("tableView").value<QQuickTableView*>();
    QVERIFY(tableView);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert with a negative index.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*setRow\\(\\): \"rowIndex\" cannot be negative"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRow",
        Q_ARG(QVariant, -1), Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert past the last allowed index.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*setRow\\(\\): \"rowIndex\" 3 is greater than rowCount\\(\\) of 2"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRow",
        Q_ARG(QVariant, 3), Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row that has a new (and hence unexpected) role.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*setRow\\(\\): expected 2 columns, but got 3"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRowInvalid1"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row that is not an array.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*setRow\\(\\): expected \"row\" argument to be an array, but got int instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRowInvalid2"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Try to insert a row with a role that is of the wrong type.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*setRow\\(\\): expected property with type int at column index 1, but got QVariantList instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRowInvalid3"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Set the first row.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRow",
        Q_ARG(QVariant, 0), Q_ARG(QVariant, QLatin1String("Max")), Q_ARG(QVariant, 40)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 40);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Set the last row.
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRow",
        Q_ARG(QVariant, 1), Q_ARG(QVariant, QLatin1String("Daisy")), Q_ARG(QVariant, 30)));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 40);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 30);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), heightSignalEmissions);
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 2);

    // Append a row by passing an index that is equal to rowCount().
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRow",
        Q_ARG(QVariant, 2), Q_ARG(QVariant, QLatin1String("Wot")), Q_ARG(QVariant, 99)));
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 40);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Daisy"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 30);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Wot"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("age")).toInt(), 99);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), ++heightSignalEmissions);
    QTRY_COMPARE(tableView->rows(), 3);
    QCOMPARE(tableView->columns(), 2);
}

void tst_QQmlTableModel::setDataThroughDelegate()
{
    QQuickView view(testFileUrl("setDataThroughDelegate.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQmlTableModel *model = view.rootObject()->property("testModel").value<QQmlTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 3);
    QVERIFY(roleNames.values().contains("name"));
    QVERIFY(roleNames.values().contains("age"));
    QVERIFY(roleNames.values().contains("display"));
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 0);

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "modify"));
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 18);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 18);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 0);

    // Test setting a role that doesn't exist for a certain column.
    const auto invalidRoleRegEx = QRegularExpression(".*setData\\(\\): no role named \"age\" at column index 0. " \
        "The available roles for that column are:[\r\n]    - \"name\" \\(QString\\)");
    // There are two rows, so two delegates respond to the signal, which means we need to ignore two warnings.
    QTest::ignoreMessage(QtWarningMsg, invalidRoleRegEx);
    QTest::ignoreMessage(QtWarningMsg, invalidRoleRegEx);
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "modifyInvalidRole"));
    // Should be unchanged.
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 18);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 18);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 0);

    // Test setting a role with a value of the wrong type.
    // There are two rows, so two delegates respond to the signal, which means we need to ignore two warnings.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*setData\\(\\): failed converting value QVariant\\(QString, \"Whoops\"\\) " \
        "set at row 0 column 1 with role \"age\" to \"int\""));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*setData\\(\\): failed converting value QVariant\\(QString, \"Whoops\"\\) " \
        "set at row 1 column 1 with role \"age\" to \"int\""));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "modifyInvalidType"));
    // Should be unchanged.
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 18);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 18);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 0);
}

// Start off with empty rows and append to test widthChanged().
void tst_QQmlTableModel::setRowsImperatively()
{
    QQuickView view(testFileUrl("empty.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQmlTableModel *model = view.rootObject()->property("testModel").value<QQmlTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->columnCount(), 0);
    QCOMPARE(model->rowCount(), 0);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());

    QQuickTableView *tableView = view.rootObject()->property("tableView").value<QQuickTableView*>();
    QVERIFY(tableView);
    QCOMPARE(tableView->rows(), 0);
    QCOMPARE(tableView->columns(), 0);

    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRows"));
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);
    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 33);
    QCOMPARE(columnCountSpy.count(), 1);
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

    QQmlTableModel *model = view.rootObject()->property("testModel").value<QQmlTableModel*>();
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
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 20);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Imum"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 41);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Power"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("age")).toInt(), 89);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 1);
    QTRY_COMPARE(tableView->rows(), 3);
    QCOMPARE(tableView->columns(), 2);

    // Set invalid rows; we should get a warning and nothing should change.
    // TODO: add quotes to the warning message
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*setRows\\(\\): expected property named name at column index 0, but got nope instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "setRowsInvalid"));
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->columnCount(), 2);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Max"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("age")).toInt(), 20);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Imum"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("age")).toInt(), 41);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("name")).toString(), QLatin1String("Power"));
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("age")).toInt(), 89);
    QCOMPARE(columnCountSpy.count(), 0);
    QCOMPARE(rowCountSpy.count(), 1);
    QCOMPARE(tableView->rows(), 3);
    QCOMPARE(tableView->columns(), 2);
}

void tst_QQmlTableModel::defaultDisplayRoles()
{
    QQuickView view(testFileUrl("defaultDisplayRoles.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQmlTableModel *model = view.rootObject()->property("testModel").value<QQmlTableModel*>();
    QVERIFY(model);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->columnCount(), 2);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowCountSpy(model, SIGNAL(rowCountChanged()));
    QVERIFY(rowCountSpy.isValid());

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("John"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 22);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Oliver"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 33);
}

void tst_QQmlTableModel::roleDataProvider()
{
    QQuickView view(testFileUrl("roleDataProvider.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQmlTableModel *model = view.rootObject()->property("testModel").value<QQmlTableModel*>();
    QVERIFY(model);

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QVERIFY(roleNames.values().contains("display"));
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Rex"));
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 3 * 7);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toString(), QLatin1String("Buster"));
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 5 * 7);
}

QTEST_MAIN(tst_QQmlTableModel)

#include "tst_qqmltablemodel.moc"
