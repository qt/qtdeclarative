// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtTest/QtTest>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuick/qquickview.h>
#include <private/qquickspritesequence_p.h>

class tst_qquickspritesequence : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickspritesequence() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void test_properties();
    void test_framerateAdvance();//Separate codepath for QQuickSpriteEngine
    void test_huge();//Separate codepath for QQuickSpriteEngine
    void test_jumpToCrash();
    void test_spriteBeforeGoal();
    void test_spriteAfterGoal();
};

void tst_qquickspritesequence::test_properties()
{
    QQuickView *window = new QQuickView(nullptr);

    window->setSource(testFileUrl("basic.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QVERIFY(window->rootObject());
    QQuickSpriteSequence* sprite = window->rootObject()->findChild<QQuickSpriteSequence*>("sprite");
    QVERIFY(sprite);

    QVERIFY(sprite->running());
    QVERIFY(sprite->interpolate());

    sprite->setRunning(false);
    QVERIFY(!sprite->running());
    sprite->setInterpolate(false);
    QVERIFY(!sprite->interpolate());

    delete window;
}

void tst_qquickspritesequence::test_huge()
{
    /* Merely tests that it doesn't crash, as waiting for it to complete
       (or even having something to watch) would bloat CI.
       The large allocations of memory involved and separate codepath does make
       a doesn't crash test worthwhile.
    */
    QQuickView *window = new QQuickView(nullptr);

    window->setSource(testFileUrl("huge.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QVERIFY(window->rootObject());
    QQuickSpriteSequence* sprite = window->rootObject()->findChild<QQuickSpriteSequence*>("sprite");
    QVERIFY(sprite);

    delete window;
}

void tst_qquickspritesequence::test_framerateAdvance()
{
    QQuickView *window = new QQuickView(nullptr);

    window->setSource(testFileUrl("advance.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QVERIFY(window->rootObject());
    QQuickSpriteSequence* sprite = window->rootObject()->findChild<QQuickSpriteSequence*>("sprite");
    QVERIFY(sprite);

    QTRY_COMPARE(sprite->currentSprite(), QLatin1String("secondState"));
    delete window;
}

void tst_qquickspritesequence::test_jumpToCrash()
{
    QQuickView *window = new QQuickView(nullptr);

    window->setSource(testFileUrl("crashonstart.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    //verify: Don't crash

    delete window;
}

void tst_qquickspritesequence::test_spriteBeforeGoal()
{
    QQuickView *window = new QQuickView(nullptr);

    window->setSource(testFileUrl("spritebeforegoal.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    //verify: Don't crash

    delete window;
}

void tst_qquickspritesequence::test_spriteAfterGoal()
{
    QQuickView *window = new QQuickView(nullptr);

    window->setSource(testFileUrl("spriteaftergoal.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    //verify: Don't crash

    delete window;
}

QTEST_MAIN(tst_qquickspritesequence)

#include "tst_qquickspritesequence.moc"
