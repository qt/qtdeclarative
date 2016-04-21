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

#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickdrawer_p.h>

using namespace QQuickVisualTestUtil;

class tst_Drawer : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void position_data();
    void position();
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
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to);
}

QTEST_MAIN(tst_Drawer)

#include "tst_drawer.moc"
