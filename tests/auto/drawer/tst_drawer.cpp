/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include "../shared/util.h"
#include "../shared/visualtestutil.h"

#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickdrawer_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>

using namespace QQuickVisualTestUtil;

class tst_Drawer : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void position_data();
    void position();

    void dragMargin_data();
    void dragMargin();

    void reposition();

    void hover_data();
    void hover();
};

void tst_Drawer::position_data()
{
    QTest::addColumn<Qt::Edge>("edge");
    QTest::addColumn<QPoint>("from");
    QTest::addColumn<QPoint>("to");
    QTest::addColumn<qreal>("position");

    QTest::newRow("top") << Qt::TopEdge << QPoint(100, 0) << QPoint(100, 100) << qreal(0.5);
    QTest::newRow("left") << Qt::LeftEdge << QPoint(0, 100) << QPoint(100, 100) << qreal(0.5);
    QTest::newRow("right") << Qt::RightEdge << QPoint(399, 100) << QPoint(300, 100) << qreal(0.5);
    QTest::newRow("bottom") << Qt::BottomEdge << QPoint(100, 399) << QPoint(100, 300) << qreal(0.5);
}

void tst_Drawer::position()
{
    QFETCH(Qt::Edge, edge);
    QFETCH(QPoint, from);
    QFETCH(QPoint, to);
    QFETCH(qreal, position);

    QQuickApplicationHelper helper(this, QStringLiteral("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickDrawer *drawer = helper.window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);
    drawer->setEdge(edge);

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, from);
    QTest::mouseMove(window, to);
    QCOMPARE(drawer->position(), position);

    // moved half-way open at almost infinite speed => snap to open
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to);
    QTRY_COMPARE(drawer->position(), 1.0);
}

void tst_Drawer::dragMargin_data()
{
    QTest::addColumn<Qt::Edge>("edge");
    QTest::addColumn<qreal>("dragMargin");
    QTest::addColumn<qreal>("dragFromLeft");
    QTest::addColumn<qreal>("dragFromRight");

    QTest::newRow("left:0") << Qt::LeftEdge << qreal(0) << qreal(0) << qreal(0);
    QTest::newRow("left:-1") << Qt::LeftEdge << qreal(-1) << qreal(0) << qreal(0);
    QTest::newRow("left:startDragDistance") << Qt::LeftEdge << qreal(QGuiApplication::styleHints()->startDragDistance()) << qreal(0.45) << qreal(0);
    QTest::newRow("left:startDragDistance*2") << Qt::LeftEdge << qreal(QGuiApplication::styleHints()->startDragDistance() * 2) << qreal(0.45) << qreal(0);

    QTest::newRow("right:0") << Qt::RightEdge << qreal(0) << qreal(0) << qreal(0);
    QTest::newRow("right:-1") << Qt::RightEdge << qreal(-1) << qreal(0) << qreal(0);
    QTest::newRow("right:startDragDistance") << Qt::RightEdge << qreal(QGuiApplication::styleHints()->startDragDistance()) << qreal(0) << qreal(0.75);
    QTest::newRow("right:startDragDistance*2") << Qt::RightEdge << qreal(QGuiApplication::styleHints()->startDragDistance() * 2) << qreal(0) << qreal(0.75);
}

void tst_Drawer::dragMargin()
{
    QFETCH(Qt::Edge, edge);
    QFETCH(qreal, dragMargin);
    QFETCH(qreal, dragFromLeft);
    QFETCH(qreal, dragFromRight);

    QQuickApplicationHelper helper(this, QStringLiteral("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickDrawer *drawer = helper.window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);
    drawer->setEdge(edge);
    drawer->setDragMargin(dragMargin);

    // drag from the left
    int leftX = qMax<int>(0, dragMargin);
    int leftDistance = drawer->width() * 0.45;
    QVERIFY(leftDistance > QGuiApplication::styleHints()->startDragDistance());
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(leftX, drawer->height() / 2));
    QTest::mouseMove(window, QPoint(leftDistance, drawer->height() / 2));
    QCOMPARE(drawer->position(), dragFromLeft);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(leftDistance, drawer->height() / 2));

    drawer->close();
    QTRY_COMPARE(drawer->position(), qreal(0.0));

    // drag from the right
    int rightX = qMin<int>(window->width() - 1, window->width() - dragMargin);
    int rightDistance = drawer->width() * 0.75;
    QVERIFY(rightDistance > QGuiApplication::styleHints()->startDragDistance());
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(rightX, drawer->height() / 2));
    QTest::mouseMove(window, QPoint(window->width() - rightDistance, drawer->height() / 2));
    QCOMPARE(drawer->position(), dragFromRight);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(window->width() - rightDistance, drawer->height() / 2));
}

void tst_Drawer::reposition()
{
    QQuickApplicationHelper helper(this, QStringLiteral("applicationwindow.qml"));

    QQuickApplicationWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickDrawer *drawer = helper.window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);
    drawer->setEdge(Qt::RightEdge);

    drawer->open();
    QTRY_COMPARE(drawer->popupItem()->x(), window->width() - drawer->width());

    drawer->close();
    QTRY_COMPARE(drawer->popupItem()->x(), static_cast<qreal>(window->width()));

    window->setWidth(window->width() + 100);
    QTRY_COMPARE(drawer->popupItem()->x(), static_cast<qreal>(window->width()));
}

void tst_Drawer::hover_data()
{
    QTest::addColumn<bool>("modal");

    QTest::newRow("modal") << true;
    QTest::newRow("modeless") << false;
}

void tst_Drawer::hover()
{
    QFETCH(bool, modal);

    QQuickApplicationHelper helper(this, QStringLiteral("hover.qml"));
    QQuickApplicationWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickDrawer *drawer = helper.window->property("drawer").value<QQuickDrawer*>();
    QVERIFY(drawer);
    drawer->setModal(modal);

    QQuickButton *backgroundButton = helper.window->property("backgroundButton").value<QQuickButton*>();
    QVERIFY(backgroundButton);
    backgroundButton->setHoverEnabled(true);

    QQuickButton *drawerButton = helper.window->property("drawerButton").value<QQuickButton*>();
    QVERIFY(drawerButton);
    drawerButton->setHoverEnabled(true);

    QSignalSpy openedSpy(drawer, SIGNAL(opened()));
    QVERIFY(openedSpy.isValid());
    drawer->open();
    QVERIFY(openedSpy.count() == 1 || openedSpy.wait());

    // hover the background button outside the drawer
    QTest::mouseMove(window, QPoint(window->width() - 1, window->height() - 1));
    QCOMPARE(backgroundButton->isHovered(), !modal);
    QVERIFY(!drawerButton->isHovered());

    // hover the drawer background
    QTest::mouseMove(window, QPoint(1, 1));
    QVERIFY(!backgroundButton->isHovered());
    QVERIFY(!drawerButton->isHovered());

    // hover the button in a drawer
    QTest::mouseMove(window, QPoint(2, 2));
    QVERIFY(!backgroundButton->isHovered());
    QVERIFY(drawerButton->isHovered());

    QSignalSpy closedSpy(drawer, SIGNAL(closed()));
    QVERIFY(closedSpy.isValid());
    drawer->close();
    QVERIFY(closedSpy.count() == 1 || closedSpy.wait());

    // hover the background button after closing the drawer
    QTest::mouseMove(window, QPoint(window->width() / 2, window->height() / 2));
    QVERIFY(backgroundButton->isHovered());
}

QTEST_MAIN(tst_Drawer)

#include "tst_drawer.moc"
