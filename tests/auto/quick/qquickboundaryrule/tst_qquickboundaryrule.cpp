// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtTest/QtTest>
#include <qsignalspy.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickdraghandler_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

class tst_qquickboundaryrule : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickboundaryrule() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void init() override;
    void dragHandler();
};

void tst_qquickboundaryrule::init()
{
    QQmlDataTest::init();
    //work around animation timer bug (QTBUG-22865)
    qApp->processEvents();
}

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
    QSignalSpy returnedSpy(boundaryRule, SIGNAL(returnedToBounds()));

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
    QCOMPARE(overshootChangedSpy.size(), 0);
    // restricted drag: halfway into overshoot
    p1 += QPoint(20, 0);
    QTest::mouseMove(&window, p1);
    QCOMPARE(target->position().x(), 117.5);
    QCOMPARE(boundaryRule->property("currentOvershoot").toReal(), 20);
    QCOMPARE(boundaryRule->property("peakOvershoot").toReal(), 20);
    QCOMPARE(overshootChangedSpy.size(), 1);
    // restricted drag: maximum overshoot
    p1 += QPoint(80, 0);
    QTest::mouseMove(&window, p1);
    QCOMPARE(target->position().x(), 140);
    QCOMPARE(boundaryRule->property("currentOvershoot").toReal(), 100);
    QCOMPARE(boundaryRule->property("peakOvershoot").toReal(), 100);
    QCOMPARE(overshootChangedSpy.size(), 2);
    // release and let it return to bounds
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QTRY_COMPARE(dragHandler->active(), false);
    QTRY_COMPARE(returnedSpy.size(), 1);
    QCOMPARE(overshootChangedSpy.size(), 3);
    QCOMPARE(boundaryRule->property("currentOvershoot").toReal(&ok), 0);
    QVERIFY(ok);
    QCOMPARE(boundaryRule->property("peakOvershoot").toReal(&ok), 0);
    QVERIFY(ok);
    QCOMPARE(target->position().x(), 100);
}

QTEST_MAIN(tst_qquickboundaryrule)

#include "tst_qquickboundaryrule.moc"
