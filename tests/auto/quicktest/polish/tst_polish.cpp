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

#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickitem.h>
#include <QQuickView>
#include <QtQuickTest/quicktest.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>

class CustomItem : public QQuickItem
{
    Q_OBJECT

public:
    CustomItem() {}

    void updatePolish() override
    {
        updatePolishCalled = true;
    }

    bool updatePolishCalled = false;
};

class tst_Polish : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_Polish();

private slots:
    void testPolish();
    void testChildPolish();
};

tst_Polish::tst_Polish()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qmlRegisterType<CustomItem>("Test", 1, 0, "CustomItem");
}

void tst_Polish::testPolish()
{
    QQuickView view;
    view.setSource(testFileUrl("polish.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    CustomItem *item = qobject_cast<CustomItem*>(view.rootObject());
    QVERIFY(item);

    // Test qWaitForPolish with QQuickItem.
    item->polish();
    QVERIFY(QQuickTest::qIsPolishScheduled(item));
    QVERIFY(QQuickTest::qIsPolishScheduled(&view));
    QVERIFY(!item->updatePolishCalled);
    QVERIFY(QQuickTest::qWaitForPolish(item));
    QVERIFY(item->updatePolishCalled);

    // Test qWaitForPolish with QQuickWindow.
    item->updatePolishCalled = false;
    item->polish();
    QVERIFY(QQuickTest::qIsPolishScheduled(item));
    QVERIFY(QQuickTest::qIsPolishScheduled(&view));
    QVERIFY(!item->updatePolishCalled);
    QVERIFY(QQuickTest::qWaitForPolish(&view));
    QVERIFY(item->updatePolishCalled);
}

void tst_Polish::testChildPolish()
{
    QQuickView view;
    view.setSource(testFileUrl("childPolish.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    CustomItem *childItem = qobject_cast<CustomItem*>(view.rootObject()->childItems().at(0));
    QVERIFY(childItem);

    // Test qWaitForPolish with QQuickItem.
    childItem->polish();
    QVERIFY(QQuickTest::qIsPolishScheduled(childItem));
    QVERIFY(QQuickTest::qIsPolishScheduled(&view));
    QVERIFY(!childItem->updatePolishCalled);
    QVERIFY(QQuickTest::qWaitForPolish(childItem));
    QVERIFY(childItem->updatePolishCalled);

    // Test qWaitForPolish with QQuickWindow.
    childItem->updatePolishCalled = false;
    childItem->polish();
    QVERIFY(QQuickTest::qIsPolishScheduled(childItem));
    QVERIFY(QQuickTest::qIsPolishScheduled(&view));
    QVERIFY(!childItem->updatePolishCalled);
    QVERIFY(QQuickTest::qWaitForPolish(&view));
    QVERIFY(childItem->updatePolishCalled);
}

QTEST_MAIN(tst_Polish)

#include "tst_polish.moc"
