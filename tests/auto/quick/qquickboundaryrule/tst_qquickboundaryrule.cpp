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
#include <QtTest/QtTest>
#include <qsignalspy.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickdraghandler_p.h>
#include "../../shared/util.h"
#include "../shared/viewtestutil.h"

class tst_qquickboundaryrule : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickboundaryrule() {}

private slots:
    void init() { qApp->processEvents(); }  //work around animation timer bug (QTBUG-22865)
    void dragHandler();
};

void tst_qquickboundaryrule::dragHandler()
{
    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("dragHandler.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QQuickItem *target = window.rootObject();
    QVERIFY(target);
    QQuickDragHandler *dragHandler = target->findChild<QQuickDragHandler*>();
    QVERIFY(dragHandler);
    QObject *boundaryRule = target->findChild<QObject *>(QLatin1String("boundaryRule"));
    QVERIFY(boundaryRule);
    QSignalSpy overshootChangedSpy(boundaryRule, SIGNAL(currentOvershootChanged()));

    QPoint p1(10, 10);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p1);
    // unrestricted drag
    p1 += QPoint(100, 0);
    QTest::mouseMove(&window, p1);
    QTRY_VERIFY(dragHandler->active());
    QCOMPARE(target->position().x(), 100);
    bool ok = false;
    QCOMPARE(boundaryRule->property("currentOvershoot").toReal(&ok), 0);
    QVERIFY(ok);
    QCOMPARE(boundaryRule->property("peakOvershoot").toReal(&ok), 0);
    QVERIFY(ok);
    QCOMPARE(overshootChangedSpy.count(), 0);
    // restricted drag: halfway into overshoot
    p1 += QPoint(20, 0);
    QTest::mouseMove(&window, p1);
    QCOMPARE(target->position().x(), 117.5);
    QCOMPARE(boundaryRule->property("currentOvershoot").toReal(), 20);
    QCOMPARE(boundaryRule->property("peakOvershoot").toReal(), 20);
    QCOMPARE(overshootChangedSpy.count(), 1);
    // restricted drag: maximum overshoot
    p1 += QPoint(80, 0);
    QTest::mouseMove(&window, p1);
    QCOMPARE(target->position().x(), 140);
    QCOMPARE(boundaryRule->property("currentOvershoot").toReal(), 100);
    QCOMPARE(boundaryRule->property("peakOvershoot").toReal(), 100);
    QCOMPARE(overshootChangedSpy.count(), 2);
    // release and let it return to bounds
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(dragHandler->active(), false);
    QTRY_COMPARE(overshootChangedSpy.count(), 3);
    QCOMPARE(boundaryRule->property("currentOvershoot").toReal(&ok), 0);
    QVERIFY(ok);
    QCOMPARE(boundaryRule->property("peakOvershoot").toReal(&ok), 0);
    QVERIFY(ok);
    QCOMPARE(target->position().x(), 100);
}

QTEST_MAIN(tst_qquickboundaryrule)

#include "tst_qquickboundaryrule.moc"
