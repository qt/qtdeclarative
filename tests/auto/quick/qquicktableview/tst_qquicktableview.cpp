/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QtTest/QtTest>

#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicktableview_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlincubator.h>
#include <QtQml/private/qqmlobjectmodel_p.h>
#include <QtQml/private/qqmllistmodel_p.h>
#include <QtQml/private/qqmldelegatemodel_p.h>

#include "testmodel.h"

#include "../../shared/util.h"
#include "../shared/viewtestutil.h"
#include "../shared/visualtestutil.h"

using namespace QQuickViewTestUtil;
using namespace QQuickVisualTestUtil;

#define CREATE_VAR_FROM_PROPERTY(name, T) \
    T *name = view->rootObject()->property(#name).value<T *>(); \
    QVERIFY(name)

#define LOAD_TABLEVIEW(fileName) \
    QScopedPointer<QQuickView> view(createView()); \
    view->setSource(testFileUrl(fileName)); \
    view->show(); \
    QVERIFY(QTest::qWaitForWindowActive(view.data())); \
    CREATE_VAR_FROM_PROPERTY(tableView, QQuickTableView)

class tst_QQuickTableView : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickTableView();

private slots:
    void initTestCase() override;

    void setAndGetQAIM();
};

tst_QQuickTableView::tst_QQuickTableView()
{
}

void tst_QQuickTableView::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<TestModel>("TestModel", 0, 1, "TestModel");
}

void tst_QQuickTableView::setAndGetQAIM()
{
    LOAD_TABLEVIEW("plaintableview.qml");

    TestModel model(2, 2);
    tableView->setModel(QVariant::fromValue<QObject *>(&model));
    auto returnModel = tableView->model().value<TestModel *>();
    QCOMPARE(&model, returnModel);
}

QTEST_MAIN(tst_QQuickTableView)

#include "tst_qquicktableview.moc"
