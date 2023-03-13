// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtTest/QtTest>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>
#include <QtQml/private/qqmltimer_p.h>
#include <QtQmlModels/private/qqmllistmodel_p.h>
#include <QtQml/private/qanimationgroupjob_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquickitemanimation_p.h>
#include <QtQuick/private/qquickitemanimation_p_p.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquickanimatorjob_p.h>
#include <QtQuick/private/qquickpathinterpolator_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquickframeanimation_p.h>
#include <QEasingCurve>

#include <limits.h>
#include <math.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickanimations : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickanimations() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override
    {
        QQmlEngine engine;  // ensure types are registered
        QQmlDataTest::initTestCase();
    }

    void simpleProperty();
    void simpleNumber();
    void simpleColor();
    void simpleRotation();
    void simplePath();
    void simpleAnchor();
    void reparent();
    void pathInterpolator();
    void pathInterpolatorBackwardJump();
    void pathWithNoStart();
    void alwaysRunToEnd();
    void complete();
    void resume();
    void dotProperty();
    void badTypes();
    void badProperties();
    void mixedTypes();
    void properties();
    void propertiesTransition();
    void pathTransition();
    void disabledTransition();
    void invalidDuration();
    void attached();
    void propertyValueSourceDefaultStart();
    void dontStart();
    void easingProperties();
    void rotation();
    void startStopSignals();
    void signalOrder_data();
    void signalOrder();
    void runningTrueBug();
    void nonTransitionBug();
    void registrationBug();
    void doubleRegistrationBug();
    void alwaysRunToEndRestartBug();
    void transitionAssignmentBug();
    void pauseBindingBug();
    void pauseBug();
    void loopingBug();
    void anchorBug();
    void pathAnimationInOutBackBug();
    void scriptActionBug();
    void groupAnimationNullChildBug();
    void scriptActionCrash();
    void animatorInvalidTargetCrash();
    void defaultPropertyWarning();
    void pathSvgAnimation();
    void pathLineUnspecifiedXYBug();
    void unsetAnimatorProxyJobWindow();
    void finished();
    void replacingTransitions();
    void animationJobSelfDestruction();
    void fastFlickingBug();
    void opacityAnimationFromZero();
    void alwaysRunToEndInSequentialAnimationBug();
    void cleanupWhenRenderThreadStops();
    void changePropertiesDuringAnimation_data();
    void changePropertiesDuringAnimation();
    void infiniteLoopsWithoutFrom();
    void frameAnimation1();
    void frameAnimation2();
    void restartAnimationGroupWhenDirty();
    void restartNestedAnimationGroupWhenDirty();
    void targetsDeletedNotRemoved();
};

#define QTIMED_COMPARE(lhs, rhs) do { \
    for (int ii = 0; ii < 5; ++ii) { \
        if (lhs == rhs)  \
            break; \
        QTest::qWait(50); \
    } \
    QCOMPARE(lhs, rhs); \
} while (false)

void tst_qquickanimations::simpleProperty()
{
    QQuickRectangle rect;
    QQuickPropertyAnimation animation;
    QSignalSpy fromChangedSpy(&animation, &QQuickPropertyAnimation::fromChanged);
    QSignalSpy toChangedSpy(&animation, &QQuickPropertyAnimation::toChanged);
    animation.setTargetObject(&rect);
    animation.setProperty("x");
    animation.setTo(200);
    QCOMPARE(animation.target(), &rect);
    QCOMPARE(animation.property(), QLatin1String("x"));
    QCOMPARE(animation.to().toReal(), 200.0);
    QCOMPARE(fromChangedSpy.size(), 0);
    QCOMPARE(toChangedSpy.size(), 1);
    animation.start();
    QVERIFY(animation.isRunning());
    QTest::qWait(animation.duration());
    QTIMED_COMPARE(rect.x(), 200.0);

    rect.setPosition(QPointF(0,0));
    animation.start();
    QVERIFY(animation.isRunning());
    animation.pause();
    QVERIFY(animation.isPaused());
    animation.setCurrentTime(125);
    QCOMPARE(animation.currentTime(), 125);
    QCOMPARE(rect.x(),100.0);
    animation.setFrom(100);
    QCOMPARE(fromChangedSpy.size(), 1);
    QCOMPARE(toChangedSpy.size(), 1);
}

void tst_qquickanimations::simpleNumber()
{
    QQuickRectangle rect;
    QQuickNumberAnimation animation;
    QSignalSpy fromChangedSpy(&animation, &QQuickNumberAnimation::fromChanged);
    QSignalSpy toChangedSpy(&animation, &QQuickNumberAnimation::toChanged);
    animation.setTargetObject(&rect);
    animation.setProperty("x");
    animation.setTo(200);
    QCOMPARE(animation.target(), &rect);
    QCOMPARE(animation.property(), QLatin1String("x"));
    QCOMPARE(animation.to(), qreal(200));
    QCOMPARE(fromChangedSpy.size(), 0);
    QCOMPARE(toChangedSpy.size(), 1);
    animation.start();
    QVERIFY(animation.isRunning());
    QTest::qWait(animation.duration());
    QTIMED_COMPARE(rect.x(), qreal(200));

    rect.setX(0);
    animation.start();
    animation.pause();
    QVERIFY(animation.isRunning());
    QVERIFY(animation.isPaused());
    animation.setCurrentTime(125);
    QCOMPARE(animation.currentTime(), 125);
    QCOMPARE(rect.x(), qreal(100));
    animation.setFrom(100);
    QCOMPARE(fromChangedSpy.size(), 1);
    QCOMPARE(toChangedSpy.size(), 1);
}

void tst_qquickanimations::simpleColor()
{
    QQuickRectangle rect;
    QQuickColorAnimation animation;
    QSignalSpy fromChangedSpy(&animation, &QQuickColorAnimation::fromChanged);
    QSignalSpy toChangedSpy(&animation, &QQuickColorAnimation::toChanged);
    animation.setTargetObject(&rect);
    animation.setProperty("color");
    animation.setTo(QColor("red"));
    QCOMPARE(animation.target(), &rect);
    QCOMPARE(animation.property(), QLatin1String("color"));
    QCOMPARE(animation.to(), QColor("red"));
    QCOMPARE(fromChangedSpy.size(), 0);
    QCOMPARE(toChangedSpy.size(), 1);
    animation.start();
    QVERIFY(animation.isRunning());
    QTest::qWait(animation.duration());
    QTIMED_COMPARE(rect.color(), QColor("red"));

    rect.setColor(QColor("blue"));
    animation.start();
    animation.pause();
    QVERIFY(animation.isRunning());
    QVERIFY(animation.isPaused());
    animation.setCurrentTime(125);
    QCOMPARE(animation.currentTime(), 125);
    QCOMPARE(rect.color(), QColor::fromRgbF(0.498039f, 0, 0.498039f, 1));

    rect.setColor(QColor("green"));
    animation.setFrom(QColor("blue"));
    QCOMPARE(animation.from(), QColor("blue"));
    QCOMPARE(fromChangedSpy.size(), 1);
    QCOMPARE(toChangedSpy.size(), 1);
    animation.restart();
    QCOMPARE(rect.color(), QColor("blue"));
    QVERIFY(animation.isRunning());
    animation.setCurrentTime(125);
    QCOMPARE(rect.color(), QColor::fromRgbF(0.498039f, 0, 0.498039f, 1));
}

void tst_qquickanimations::simpleRotation()
{
    QQuickRectangle rect;
    QQuickRotationAnimation animation;
    QSignalSpy fromChangedSpy(&animation, &QQuickRotationAnimation::fromChanged);
    QSignalSpy toChangedSpy(&animation, &QQuickRotationAnimation::toChanged);
    animation.setTargetObject(&rect);
    animation.setProperty("rotation");
    animation.setTo(270);
    QCOMPARE(animation.target(), &rect);
    QCOMPARE(animation.property(), QLatin1String("rotation"));
    QCOMPARE(animation.to(), qreal(270));
    QCOMPARE(animation.direction(), QQuickRotationAnimation::Numerical);
    QCOMPARE(fromChangedSpy.size(), 0);
    QCOMPARE(toChangedSpy.size(), 1);
    animation.start();
    QVERIFY(animation.isRunning());
    QTest::qWait(animation.duration());
    QTIMED_COMPARE(rect.rotation(), qreal(270));

    rect.setRotation(0);
    animation.start();
    animation.pause();
    QVERIFY(animation.isRunning());
    QVERIFY(animation.isPaused());
    animation.setCurrentTime(125);
    QCOMPARE(animation.currentTime(), 125);
    QCOMPARE(rect.rotation(), qreal(135));
    animation.setFrom(90);
    QCOMPARE(fromChangedSpy.size(), 1);
    QCOMPARE(toChangedSpy.size(), 1);
}

void tst_qquickanimations::simplePath()
{
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("pathAnimation.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *redRect = rect->findChild<QQuickRectangle*>();
        QVERIFY(redRect);
        QQuickPathAnimation *pathAnim = rect->findChild<QQuickPathAnimation*>();
        QVERIFY(pathAnim);

        QCOMPARE(pathAnim->duration(), 100);
        QCOMPARE(pathAnim->target(), redRect);

        pathAnim->start();
        pathAnim->pause();

        pathAnim->setCurrentTime(30);
        QCOMPARE(redRect->x(), qreal(167));
        QCOMPARE(redRect->y(), qreal(104));

        pathAnim->setCurrentTime(100);
        QCOMPARE(redRect->x(), qreal(300));
        QCOMPARE(redRect->y(), qreal(300));

        //verify animation runs to end
        pathAnim->start();
        QCOMPARE(redRect->x(), qreal(50));
        QCOMPARE(redRect->y(), qreal(50));
        QTRY_COMPARE(redRect->x(), qreal(300));
        QCOMPARE(redRect->y(), qreal(300));

        pathAnim->setOrientation(QQuickPathAnimation::RightFirst);
        QCOMPARE(pathAnim->orientation(), QQuickPathAnimation::RightFirst);
        pathAnim->start();
        QTRY_VERIFY(redRect->rotation() != 0);
        pathAnim->stop();
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("pathAnimation2.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *redRect = rect->findChild<QQuickRectangle*>();
        QVERIFY(redRect);
        QQuickPathAnimation *pathAnim = rect->findChild<QQuickPathAnimation*>();
        QVERIFY(pathAnim);

        QCOMPARE(pathAnim->orientation(), QQuickPathAnimation::RightFirst);
        QCOMPARE(pathAnim->endRotation(), qreal(0));
        QCOMPARE(pathAnim->orientationEntryDuration(), 10);
        QCOMPARE(pathAnim->orientationExitDuration(), 10);

        pathAnim->start();
        pathAnim->pause();
        QCOMPARE(redRect->x(), qreal(50));
        QCOMPARE(redRect->y(), qreal(50));
        QCOMPARE(redRect->rotation(), qreal(-360));

        pathAnim->setCurrentTime(50);
        QCOMPARE(redRect->x(), qreal(175));
        QCOMPARE(redRect->y(), qreal(175));
        QCOMPARE(redRect->rotation(), qreal(-315));

        pathAnim->setCurrentTime(100);
        QCOMPARE(redRect->x(), qreal(300));
        QCOMPARE(redRect->y(), qreal(300));
        QCOMPARE(redRect->rotation(), qreal(0));
    }
}

void tst_qquickanimations::simpleAnchor()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("reanchor.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect);

    QQuickRectangle *greenRect = rect->findChild<QQuickRectangle*>();
    QVERIFY(greenRect);

    QCOMPARE(rect->state(), QLatin1String("reanchored"));
    QCOMPARE(greenRect->x(), qreal(10));
    QCOMPARE(greenRect->y(), qreal(0));
    QCOMPARE(greenRect->width(), qreal(190));
    QCOMPARE(greenRect->height(), qreal(150));

    rect->setState("");

    //verify animation in progress
    QTRY_VERIFY(greenRect->x() < 10 && greenRect->x() > 0);
    QVERIFY(greenRect->y() > 0 && greenRect->y() < 10);
    QVERIFY(greenRect->width() < 190 && greenRect->width() > 150);
    QVERIFY(greenRect->height() > 150 && greenRect->height() < 190);

    //verify end state ("")
    QTRY_COMPARE(greenRect->x(), qreal(0));
    QCOMPARE(greenRect->y(), qreal(10));
    QCOMPARE(greenRect->width(), qreal(150));
    QCOMPARE(greenRect->height(), qreal(190));

    rect->setState("reanchored2");

    //verify animation in progress
    QTRY_VERIFY(greenRect->y() > 10 && greenRect->y() < 50);
    QVERIFY(greenRect->height() > 125 && greenRect->height() < 190);
    //NOTE: setting left/right anchors to undefined removes the anchors, but does not resize.
    QCOMPARE(greenRect->x(), qreal(0));
    QCOMPARE(greenRect->width(), qreal(150));

    //verify end state ("reanchored2")
    QTRY_COMPARE(greenRect->y(), qreal(50));
    QCOMPARE(greenRect->height(), qreal(125));
    QCOMPARE(greenRect->x(), qreal(0));
    QCOMPARE(greenRect->width(), qreal(150));

    rect->setState("reanchored");

    //verify animation in progress
    QTRY_VERIFY(greenRect->x() < 10 && greenRect->x() > 0);
    QVERIFY(greenRect->y() > 0 && greenRect->y() < 50);
    QVERIFY(greenRect->width() < 190 && greenRect->width() > 150);
    QVERIFY(greenRect->height() > 125 && greenRect->height() < 150);

    //verify end state ("reanchored")
    QTRY_COMPARE(greenRect->x(), qreal(10));
    QCOMPARE(greenRect->y(), qreal(0));
    QCOMPARE(greenRect->width(), qreal(190));
    QCOMPARE(greenRect->height(), qreal(150));

    rect->setState("reanchored2");

    //verify animation in progress
    QTRY_VERIFY(greenRect->x() < 10 && greenRect->x() > 0);
    QVERIFY(greenRect->y() > 0 && greenRect->y() < 50);
    QVERIFY(greenRect->width() < 190 && greenRect->width() > 150);
    QVERIFY(greenRect->height() > 125 && greenRect->height() < 150);

    //verify end state ("reanchored2")
    QTRY_COMPARE(greenRect->x(), qreal(0));
    QCOMPARE(greenRect->y(), qreal(50));
    QCOMPARE(greenRect->width(), qreal(150));
    QCOMPARE(greenRect->height(), qreal(125));
}

void tst_qquickanimations::reparent()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("reparent.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect);

    QQuickRectangle *target = rect->findChild<QQuickRectangle*>("target");
    QVERIFY(target);

    QCOMPARE(target->parentItem(), rect);
    QCOMPARE(target->x(), qreal(0));
    QCOMPARE(target->y(), qreal(0));
    QCOMPARE(target->width(), qreal(50));
    QCOMPARE(target->height(), qreal(50));
    QCOMPARE(target->rotation(), qreal(0));
    QCOMPARE(target->scale(), qreal(1));

    rect->setState("state1");

    QQuickRectangle *viaParent = rect->findChild<QQuickRectangle*>("viaParent");
    QVERIFY(viaParent);

    QQuickRectangle *newParent = rect->findChild<QQuickRectangle*>("newParent");
    QVERIFY(newParent);

    QTest::qWait(100);

    //animation in progress
    QTRY_COMPARE(target->parentItem(), viaParent);
    QVERIFY(target->x() > -100 && target->x() < 50);
    QVERIFY(target->y() > -100 && target->y() < 50);
    QVERIFY(target->width() > 50 && target->width() < 100);
    QCOMPARE(target->height(), qreal(50));
    QCOMPARE(target->rotation(), qreal(-45));
    QCOMPARE(target->scale(), qreal(.5));

    //end state
    QTRY_COMPARE(target->parentItem(), newParent);
    QCOMPARE(target->x(), qreal(50));
    QCOMPARE(target->y(), qreal(50));
    QCOMPARE(target->width(), qreal(100));
    QCOMPARE(target->height(), qreal(50));
    QCOMPARE(target->rotation(), qreal(0));
    QCOMPARE(target->scale(), qreal(1));
}

void tst_qquickanimations::pathInterpolator()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathInterpolator.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *interpolator = qobject_cast<QQuickPathInterpolator*>(obj.data());
    QVERIFY(interpolator);

    QCOMPARE(interpolator->progress(), qreal(0));
    QCOMPARE(interpolator->x(), qreal(50));
    QCOMPARE(interpolator->y(), qreal(50));
    QCOMPARE(interpolator->angle(), qreal(0));

    interpolator->setProgress(.5);
    QCOMPARE(interpolator->progress(), qreal(.5));
    QCOMPARE(interpolator->x(), qreal(175));
    QCOMPARE(interpolator->y(), qreal(175));
    QCOMPARE(interpolator->angle(), qreal(90));

    interpolator->setProgress(1);
    QCOMPARE(interpolator->progress(), qreal(1));
    QCOMPARE(interpolator->x(), qreal(300));
    QCOMPARE(interpolator->y(), qreal(300));
    QCOMPARE(interpolator->angle(), qreal(0));

    //for path interpulator the progress value must be [0,1] range.
    interpolator->setProgress(1.1);
    QCOMPARE(interpolator->progress(), qreal(1));

    interpolator->setProgress(-0.000123);
    QCOMPARE(interpolator->progress(), qreal(0));
}

void tst_qquickanimations::pathInterpolatorBackwardJump()
{
#ifdef Q_CC_MINGW
    QSKIP("QTBUG-36290 - MinGW Animation tests are flaky.");
#endif
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("pathInterpolatorBack.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *interpolator = qobject_cast<QQuickPathInterpolator*>(obj.data());
        QVERIFY(interpolator);

        QCOMPARE(interpolator->progress(), qreal(0));
        QCOMPARE(interpolator->x(), qreal(50));
        QCOMPARE(interpolator->y(), qreal(50));
        QCOMPARE(interpolator->angle(), qreal(90));

        interpolator->setProgress(.5);
        QCOMPARE(interpolator->progress(), qreal(.5));
        QCOMPARE(interpolator->x(), qreal(100));
        QCOMPARE(interpolator->y(), qreal(75));
        QCOMPARE(interpolator->angle(), qreal(270));

        interpolator->setProgress(1);
        QCOMPARE(interpolator->progress(), qreal(1));
        QCOMPARE(interpolator->x(), qreal(200));
        QCOMPARE(interpolator->y(), qreal(50));
        QCOMPARE(interpolator->angle(), qreal(0));

        //make sure we don't get caught in infinite loop here
        interpolator->setProgress(0);
        QCOMPARE(interpolator->progress(), qreal(0));
        QCOMPARE(interpolator->x(), qreal(50));
        QCOMPARE(interpolator->y(), qreal(50));
        QCOMPARE(interpolator->angle(), qreal(90));
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("pathInterpolatorBack2.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *interpolator = qobject_cast<QQuickPathInterpolator*>(obj.data());
        QVERIFY(interpolator);

        QCOMPARE(interpolator->progress(), qreal(0));
        QCOMPARE(interpolator->x(), qreal(200));
        QCOMPARE(interpolator->y(), qreal(280));
        QCOMPARE(interpolator->angle(), qreal(180));

        interpolator->setProgress(1);
        QCOMPARE(interpolator->progress(), qreal(1));
        QCOMPARE(interpolator->x(), qreal(0));
        QCOMPARE(interpolator->y(), qreal(80));
        QCOMPARE(interpolator->angle(), qreal(180));

        //make sure we don't get caught in infinite loop here
        interpolator->setProgress(0);
        QCOMPARE(interpolator->progress(), qreal(0));
        QCOMPARE(interpolator->x(), qreal(200));
        QCOMPARE(interpolator->y(), qreal(280));
        QCOMPARE(interpolator->angle(), qreal(180));
    }
}

void tst_qquickanimations::pathWithNoStart()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathAnimationNoStart.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect);

    QQuickRectangle *redRect = rect->findChild<QQuickRectangle*>();
    QVERIFY(redRect);
    QQuickPathAnimation *pathAnim = rect->findChild<QQuickPathAnimation*>();
    QVERIFY(pathAnim);

    pathAnim->start();
    pathAnim->pause();
    QCOMPARE(redRect->x(), qreal(50));
    QCOMPARE(redRect->y(), qreal(50));

    pathAnim->setCurrentTime(50);
    QCOMPARE(redRect->x(), qreal(175));
    QCOMPARE(redRect->y(), qreal(175));

    pathAnim->setCurrentTime(100);
    QCOMPARE(redRect->x(), qreal(300));
    QCOMPARE(redRect->y(), qreal(300));

    redRect->setX(100);
    redRect->setY(100);
    pathAnim->start();
    QCOMPARE(redRect->x(), qreal(100));
    QCOMPARE(redRect->y(), qreal(100));
    QTRY_COMPARE(redRect->x(), qreal(300));
    QCOMPARE(redRect->y(), qreal(300));
}

void tst_qquickanimations::alwaysRunToEnd()
{
    QQuickRectangle rect;
    QQuickPropertyAnimation animation;
    animation.setTargetObject(&rect);
    animation.setProperty("x");
    animation.setTo(200);
    animation.setDuration(1000);
    animation.setLoops(-1);
    animation.setAlwaysRunToEnd(true);
    QCOMPARE(animation.loops(), -1);
    QVERIFY(animation.alwaysRunToEnd());

    QElapsedTimer timer;
    timer.start();
    animation.start();

    // Make sure the animation has started but is not finished, yet.
    QTRY_VERIFY(rect.x() > qreal(0) && rect.x() != qreal(200));

    animation.stop();

    // Make sure it didn't just jump to the end and also didn't revert to the start.
    QVERIFY(rect.x() > qreal(0) && rect.x() != qreal(200));

    // Make sure it eventually reaches the end.
    QTRY_COMPARE(rect.x(), qreal(200));

    // This should have taken at least 1s but less than 2s
    // (otherwise it has run the animation twice).
    qint64 elapsed = timer.elapsed();
    QVERIFY(elapsed >= 1000 && elapsed < 2000);
}

void tst_qquickanimations::complete()
{
    QQuickRectangle rect;
    QQuickPropertyAnimation animation;
    animation.setTargetObject(&rect);
    animation.setProperty("x");
    animation.setFrom(1);
    animation.setTo(200);
    animation.setDuration(500);
    QCOMPARE(animation.from().toInt(), 1);
    animation.start();
    QTest::qWait(50);
    animation.stop();
    QVERIFY(rect.x() != qreal(200));
    animation.start();
    QTRY_VERIFY(animation.isRunning());
    animation.complete();
    QCOMPARE(rect.x(), qreal(200));
}

void tst_qquickanimations::resume()
{
    QQuickRectangle rect;
    QQuickPropertyAnimation animation;
    animation.setTargetObject(&rect);
    animation.setProperty("x");
    animation.setFrom(10);
    animation.setTo(200);
    animation.setDuration(1000);
    QCOMPARE(animation.from().toInt(), 10);

    animation.start();
    QTest::qWait(400);
    animation.pause();
    qreal x = rect.x();
    QVERIFY(x != qreal(200) && x != qreal(10));
    QVERIFY(animation.isRunning());
    QVERIFY(animation.isPaused());

    animation.resume();
    QVERIFY(animation.isRunning());
    QVERIFY(!animation.isPaused());
    QTest::qWait(400);
    animation.stop();
    QVERIFY(rect.x() > x);

    animation.start();
    QVERIFY(animation.isRunning());
    animation.pause();
    QVERIFY(animation.isPaused());
    animation.resume();
    QVERIFY(!animation.isPaused());

    QSignalSpy spy(&animation, SIGNAL(pausedChanged(bool)));
    animation.pause();
    QCOMPARE(spy.size(), 1);
    QVERIFY(animation.isPaused());
    animation.stop();
    QVERIFY(!animation.isPaused());
    QCOMPARE(spy.size(), 2);

    // Load QtQuick to ensure that QQuickPropertyAnimation is registered as PropertyAnimation
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nQtObject {}\n", QUrl());
    }

    QByteArray message = "<Unknown File>: QML PropertyAnimation: setPaused() cannot be used when animation isn't running.";
    QTest::ignoreMessage(QtWarningMsg, message);
    animation.pause();
    QCOMPARE(spy.size(), 2);
    QVERIFY(!animation.isPaused());
    animation.resume();
    QVERIFY(!animation.isPaused());
    QVERIFY(!animation.isRunning());
    QCOMPARE(spy.size(), 2);
}

void tst_qquickanimations::dotProperty()
{
    QQuickRectangle rect;
    QQuickNumberAnimation animation;
    animation.setTargetObject(&rect);
    animation.setProperty("border.width");
    animation.setTo(10);
    animation.start();
    QTest::qWait(animation.duration()+50);
    QTIMED_COMPARE(rect.border()->width(), 10.0);

    rect.border()->setWidth(0);
    animation.start();
    animation.pause();
    animation.setCurrentTime(125);
    QCOMPARE(animation.currentTime(), 125);
    QCOMPARE(rect.border()->width(), 5.0);
}

void tst_qquickanimations::badTypes()
{
    //don't crash
    {
        QScopedPointer<QQuickView> view(new QQuickView);
        view->setSource(testFileUrl("badtype1.qml"));
        qApp->processEvents();
    }

    //make sure we get a compiler error
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("badtype2.qml"));
        QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
        QScopedPointer<QObject> obj(c.create());
        QVERIFY(obj.isNull());

        QCOMPARE(c.errors().size(), 1);
        QCOMPARE(c.errors().at(0).description(), QLatin1String("Invalid property assignment: number expected"));
    }

    //make sure we get a compiler error
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("badtype3.qml"));
        QTest::ignoreMessage(QtWarningMsg, "QQmlComponent: Component is not ready");
        QScopedPointer<QObject> obj(c.create());
        QVERIFY(obj.isNull());

        QCOMPARE(c.errors().size(), 1);
        QCOMPARE(c.errors().at(0).description(), QLatin1String("Invalid property assignment: color expected"));
    }

    //don't crash
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("badtype4.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickItemPrivate::get(rect)->setState("state1");

        QQuickRectangle *myRect = nullptr;
        QTRY_VERIFY(myRect = rect->findChild<QQuickRectangle*>("MyRect"));
        QTRY_COMPARE(myRect->x(),qreal(200));
    }
}

void tst_qquickanimations::badProperties()
{
    //make sure we get a runtime error
    {
        QQmlEngine engine;

        QQmlComponent c1(&engine, testFileUrl("badproperty1.qml"));
        QByteArray message = testFileUrl("badproperty1.qml").toString().toUtf8() + ":18:9: QML ColorAnimation: Cannot animate non-existent property \"border.colr\"";
        QTest::ignoreMessage(QtWarningMsg, message);
        QScopedPointer<QObject> obj(c1.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQmlComponent c2(&engine, testFileUrl("badproperty2.qml"));
        message = testFileUrl("badproperty2.qml").toString().toUtf8() + ":18:9: QML ColorAnimation: Cannot animate read-only property \"border\"";
        QTest::ignoreMessage(QtWarningMsg, message);
        QScopedPointer<QObject> obj2(c2.create());
        rect = qobject_cast<QQuickRectangle*>(obj2.data());
        QVERIFY(rect);

        //### should we warn here are well?
        //rect->setState("state1");
    }
}

//test animating mixed types with property animation in a transition
//for example, int + real; color + real; etc
void tst_qquickanimations::mixedTypes()
{
    //assumes border.width stays a real -- not real robust
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("mixedtype1.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickItemPrivate::get(rect)->setState("state1");
        QTest::qWait(500);
        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("MyRect");
        QVERIFY(myRect);

        // We cannot get that more exact than that without dependable real-time behavior.
        QVERIFY(myRect->x() > 100 && myRect->x() < 200);
        QVERIFY(myRect->border()->width() > 1 && myRect->border()->width() < 10);
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("mixedtype2.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickItemPrivate::get(rect)->setState("state1");
        QTest::qWait(500);
        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("MyRect");
        QVERIFY(myRect);

        // We cannot get that more exact than that without dependable real-time behavior.
        QVERIFY(myRect->x() > 100 && myRect->x() < 200);
        QVERIFY(myRect->color() != QColor("red") && myRect->color() != QColor("blue"));
    }
}

void tst_qquickanimations::properties()
{
    const int waitDuration = 300;
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("properties.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("properties2.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("properties3.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(300));
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("properties4.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->y(),qreal(200));
        QTIMED_COMPARE(myRect->x(),qreal(100));
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("properties5.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(100));
        QTIMED_COMPARE(myRect->y(),qreal(200));
    }
}

void tst_qquickanimations::propertiesTransition()
{
    const int waitDuration = 300;
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("propertiesTransition.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickItemPrivate::get(rect)->setState("moved");
        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("propertiesTransition2.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QQuickItemPrivate::get(rect)->setState("moved");
        QCOMPARE(myRect->x(),qreal(200));
        QCOMPARE(myRect->y(),qreal(100));
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->y(),qreal(200));
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("propertiesTransition3.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QQuickItemPrivate::get(rect)->setState("moved");
        QCOMPARE(myRect->x(),qreal(200));
        QCOMPARE(myRect->y(),qreal(100));
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("propertiesTransition4.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QQuickItemPrivate::get(rect)->setState("moved");
        QCOMPARE(myRect->x(),qreal(100));
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("propertiesTransition5.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QQuickItemPrivate::get(rect)->setState("moved");
        QCOMPARE(myRect->x(),qreal(100));
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

    /*{
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("propertiesTransition6.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QQuickItemPrivate::get(rect)->setState("moved");
        QCOMPARE(myRect->x(),qreal(100));
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(100));
    }*/

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("propertiesTransition7.qml"));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickItemPrivate::get(rect)->setState("moved");
        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

}

void tst_qquickanimations::pathTransition()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathTransition.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect);

    QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("redRect");
    QVERIFY(myRect);

    QQuickItemPrivate::get(rect)->setState("moved");
    QTRY_VERIFY(myRect->x() < 500 && myRect->x() > 100 && myRect->y() > 50 && myRect->y() < 700 );  //animation started
    QTRY_VERIFY(qFuzzyCompare(myRect->x(), qreal(100)) && qFuzzyCompare(myRect->y(), qreal(700)));
    QTest::qWait(100);

    QQuickItemPrivate::get(rect)->setState("");
    QTRY_VERIFY(myRect->x() < 500 && myRect->x() > 100 && myRect->y() > 50 && myRect->y() < 700 );  //animation started
    QTRY_VERIFY(qFuzzyCompare(myRect->x(), qreal(500)) && qFuzzyCompare(myRect->y(), qreal(50)));
}

void tst_qquickanimations::disabledTransition()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("disabledTransition.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect);

    QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
    QVERIFY(myRect);

    QQuickTransition *trans = rect->findChild<QQuickTransition*>();
    QVERIFY(trans);

    QCOMPARE(trans->enabled(), false);

    QQuickItemPrivate::get(rect)->setState("moved");
    QCOMPARE(myRect->x(),qreal(200));

    trans->setEnabled(true);
    QSignalSpy runningSpy(trans, SIGNAL(runningChanged()));
    QQuickItemPrivate::get(rect)->setState("");
    QCOMPARE(myRect->x(),qreal(200));
    QCOMPARE(runningSpy.size(), 1); //stopped -> running
    QVERIFY(trans->running());
    QTest::qWait(300);
    QTIMED_COMPARE(myRect->x(),qreal(100));
    QVERIFY(!trans->running());
    QCOMPARE(runningSpy.size(), 2); //running -> stopped
}

void tst_qquickanimations::invalidDuration()
{
    QScopedPointer<QQuickPropertyAnimation> animation(new QQuickPropertyAnimation);
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML PropertyAnimation: Cannot set a duration of < 0");
    animation->setDuration(-1);
    QCOMPARE(animation->duration(), 250);

    QScopedPointer<QQuickPauseAnimation> pauseAnimation(new QQuickPauseAnimation);
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML PauseAnimation: Cannot set a duration of < 0");
    pauseAnimation->setDuration(-1);
    QCOMPARE(pauseAnimation->duration(), 250);
}

void tst_qquickanimations::attached()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("attached.qml"));
    QTest::ignoreMessage(QtDebugMsg, "off");
    QTest::ignoreMessage(QtDebugMsg, "on");
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect);
}

void tst_qquickanimations::propertyValueSourceDefaultStart()
{
    {
        QQmlEngine engine;

        QQmlComponent c(&engine, testFileUrl("valuesource.qml"));

        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickAbstractAnimation *myAnim = rect->findChild<QQuickAbstractAnimation*>("MyAnim");
        QVERIFY(myAnim);
        QVERIFY(myAnim->isRunning());
    }

    {
        QQmlEngine engine;

        QQmlComponent c(&engine, testFileUrl("valuesource2.qml"));

        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickAbstractAnimation *myAnim = rect->findChild<QQuickAbstractAnimation*>("MyAnim");
        QVERIFY(myAnim);
        QVERIFY(!myAnim->isRunning());
    }

    {
        QQmlEngine engine;

        QQmlComponent c(&engine, testFileUrl("dontAutoStart.qml"));

        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickAbstractAnimation *myAnim = rect->findChild<QQuickAbstractAnimation*>("MyAnim");
        QVERIFY(myAnim && !myAnim->qtAnimation());
        //QCOMPARE(myAnim->qtAnimation()->state(), QAbstractAnimationJob::Stopped);
    }
}


void tst_qquickanimations::dontStart()
{
    {
        QQmlEngine engine;

        QQmlComponent c(&engine, testFileUrl("dontStart.qml"));

        QString warning = c.url().toString() + ":14:13: QML NumberAnimation: setRunning() cannot be used on non-root animation nodes.";
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickAbstractAnimation *myAnim = rect->findChild<QQuickAbstractAnimation*>("MyAnim");
        QVERIFY(myAnim && !myAnim->qtAnimation());
        //QCOMPARE(myAnim->qtAnimation()->state(), QAbstractAnimationJob::Stopped);
    }

    {
        QQmlEngine engine;

        QQmlComponent c(&engine, testFileUrl("dontStart2.qml"));

        QString warning = c.url().toString() + ":15:17: QML NumberAnimation: setRunning() cannot be used on non-root animation nodes.";
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
        QScopedPointer<QObject> obj(c.create());
        auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
        QVERIFY(rect);

        QQuickAbstractAnimation *myAnim = rect->findChild<QQuickAbstractAnimation*>("MyAnim");
        QVERIFY(myAnim && !myAnim->qtAnimation());
        //QCOMPARE(myAnim->qtAnimation()->state(), QAbstractAnimationJob::Stopped);
    }
}

void tst_qquickanimations::easingProperties()
{
    {
        QQmlEngine engine;
        QString componentStr = "import QtQuick 2.0\nNumberAnimation { easing.type: \"InOutQuad\" }";
        QQmlComponent animationComponent(&engine);
        animationComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> obj(animationComponent.create());
        auto *animObject = qobject_cast<QQuickPropertyAnimation *>(obj.data());

        QVERIFY(animObject != nullptr);
        QCOMPARE(animObject->easing().type(), QEasingCurve::InOutQuad);
    }

    {
        QQmlEngine engine;
        QString componentStr = "import QtQuick 2.0\nPropertyAnimation { easing.type: \"OutBounce\"; easing.amplitude: 5.0 }";
        QQmlComponent animationComponent(&engine);
        animationComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> obj(animationComponent.create());
        auto *animObject = qobject_cast<QQuickPropertyAnimation *>(obj.data());

        QVERIFY(animObject != nullptr);
        QCOMPARE(animObject->easing().type(), QEasingCurve::OutBounce);
        QCOMPARE(animObject->easing().amplitude(), 5.0);
    }

    {
        QQmlEngine engine;
        QString componentStr = "import QtQuick 2.0\nPropertyAnimation { easing.type: \"OutElastic\"; easing.amplitude: 5.0; easing.period: 3.0}";
        QQmlComponent animationComponent(&engine);
        animationComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> obj(animationComponent.create());
        auto *animObject = qobject_cast<QQuickPropertyAnimation *>(obj.data());

        QVERIFY(animObject != nullptr);
        QCOMPARE(animObject->easing().type(), QEasingCurve::OutElastic);
        QCOMPARE(animObject->easing().amplitude(), 5.0);
        QCOMPARE(animObject->easing().period(), 3.0);
    }

    {
        QQmlEngine engine;
        QString componentStr = "import QtQuick 2.0\nPropertyAnimation { easing.type: \"InOutBack\"; easing.overshoot: 2 }";
        QQmlComponent animationComponent(&engine);
        animationComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> obj(animationComponent.create());
        auto *animObject = qobject_cast<QQuickPropertyAnimation *>(obj.data());

        QVERIFY(animObject != nullptr);
        QCOMPARE(animObject->easing().type(), QEasingCurve::InOutBack);
        QCOMPARE(animObject->easing().overshoot(), 2.0);
    }

    {
        QQmlEngine engine;
        QString componentStr = "import QtQuick 2.0\nPropertyAnimation { easing.type: \"BezierSpline\"; easing.bezierCurve: [0.5, 0.2, 0.13, 0.65, 1.0, 1.0] }";
        QQmlComponent animationComponent(&engine);
        animationComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> obj(animationComponent.create());
        auto *animObject = qobject_cast<QQuickPropertyAnimation *>(obj.data());

        QVERIFY(animObject != nullptr);
        QCOMPARE(animObject->easing().type(), QEasingCurve::BezierSpline);
        QVector<QPointF> points = animObject->easing().toCubicSpline();
        QCOMPARE(points.size(), 3);
        QCOMPARE(points.at(0), QPointF(0.5, 0.2));
        QCOMPARE(points.at(1), QPointF(0.13, 0.65));
        QCOMPARE(points.at(2), QPointF(1.0, 1.0));
    }
}

void tst_qquickanimations::rotation()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("rotation.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect);

    QQuickRectangle *rr = rect->findChild<QQuickRectangle*>("rr");
    QQuickRectangle *rr2 = rect->findChild<QQuickRectangle*>("rr2");
    QQuickRectangle *rr3 = rect->findChild<QQuickRectangle*>("rr3");
    QQuickRectangle *rr4 = rect->findChild<QQuickRectangle*>("rr4");

    QQuickItemPrivate::get(rect)->setState("state1");
    QTest::qWait(800);
    qreal r1 = rr->rotation();
    qreal r2 = rr2->rotation();
    qreal r3 = rr3->rotation();
    qreal r4 = rr4->rotation();

    QVERIFY(r1 > qreal(0) && r1 < qreal(370));
    QVERIFY(r2 > qreal(0) && r2 < qreal(370));
    QVERIFY(r3 < qreal(0) && r3 > qreal(-350));
    QVERIFY(r4 > qreal(0) && r4 < qreal(10));
    QCOMPARE(r1,r2);
    QVERIFY(r4 < r2);

    QTest::qWait(800);
    QTIMED_COMPARE(rr->rotation() + rr2->rotation() + rr3->rotation() + rr4->rotation(), qreal(370*4));
}

void tst_qquickanimations::startStopSignals()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("signals.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *root = qobject_cast<QQuickItem *>(obj.data());
    QVERIFY(root);

    QCOMPARE(root->property("startedCount").toInt(), 1);    //autostart
    QCOMPARE(root->property("stoppedCount").toInt(), 0);

    QMetaObject::invokeMethod(root, "stop");

    QCOMPARE(root->property("startedCount").toInt(), 1);
    QCOMPARE(root->property("stoppedCount").toInt(), 1);

    QElapsedTimer timer;
    timer.start();
    QMetaObject::invokeMethod(root, "start");

    QCOMPARE(root->property("startedCount").toInt(), 2);
    QCOMPARE(root->property("stoppedCount").toInt(), 1);

    QTRY_COMPARE(root->property("stoppedCount").toInt(), 2);
    QCOMPARE(root->property("startedCount").toInt(), 2);
    QVERIFY(timer.elapsed() >= 200);

    root->setProperty("alwaysRunToEnd", true);

    timer.restart();
    QMetaObject::invokeMethod(root, "start");

    QCOMPARE(root->property("startedCount").toInt(), 3);
    QCOMPARE(root->property("stoppedCount").toInt(), 2);

    QMetaObject::invokeMethod(root, "stop");

    QCOMPARE(root->property("startedCount").toInt(), 3);
    QCOMPARE(root->property("stoppedCount").toInt(), 2);

    QTRY_COMPARE(root->property("stoppedCount").toInt(), 3);
    QCOMPARE(root->property("startedCount").toInt(), 3);
    QVERIFY(timer.elapsed() >= 200);
}

void tst_qquickanimations::signalOrder_data()
{
    QTest::addColumn<QByteArray>("animationType");
    QTest::addColumn<int>("duration");

    QTest::addRow("ColorAnimation, duration = 10") << QByteArray("ColorAnimation") << 10;
    QTest::addRow("ColorAnimation, duration = 0") << QByteArray("ColorAnimation") << 0;
    QTest::addRow("ParallelAnimation, duration = 0") << QByteArray("ParallelAnimation") << 0;
}

void tst_qquickanimations::signalOrder()
{
    QFETCH(QByteArray, animationType);
    QFETCH(int, duration);

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("signalorder.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *root = qobject_cast<QQuickItem *>(obj.data());
    QVERIFY(root);
    QQuickAbstractAnimation *animation = root->findChild<QQuickAbstractAnimation*>(animationType);

    const QVector<void (QQuickAbstractAnimation::*)()> signalsToConnect = {
        &QQuickAbstractAnimation::started,
        &QQuickAbstractAnimation::stopped,
        &QQuickAbstractAnimation::finished
    };
    const QVector<const char*> expectedSignalOrder = {
        "started",
        "stopped",
        "finished"
    };

    QVector<const char*> actualSignalOrder;

    for (int i = 0; i < signalsToConnect.size(); ++i) {
        const char *str = expectedSignalOrder.at(i);
        connect(animation, signalsToConnect.at(i) , [str, &actualSignalOrder] () {
            actualSignalOrder.append(str);
        });
    }
    QSignalSpy finishedSpy(animation, SIGNAL(finished()));
    if (QQuickColorAnimation *colorAnimation = qobject_cast<QQuickColorAnimation*>(animation))
        colorAnimation->setDuration(duration);

    animation->start();
    QTRY_VERIFY(finishedSpy.size());
    QCOMPARE(actualSignalOrder, expectedSignalOrder);
}

void tst_qquickanimations::runningTrueBug()
{
    //ensure we start correctly when "running: true" is explicitly set
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("runningTrueBug.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect);

    QQuickRectangle *cloud = rect->findChild<QQuickRectangle*>("cloud");
    QVERIFY(cloud);
    QTest::qWait(1000);
    QVERIFY(cloud->x() > qreal(0));
}

//QTBUG-24308
void tst_qquickanimations::pathAnimationInOutBackBug()
{
    //ensure we don't pass bad progress value (out of [0,1]) to  QQuickPath::backwardsPointAt()
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathAnimationInOutBackCrash.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *item = qobject_cast<QQuickItem *>(obj.data());
    QVERIFY(item);

    QQuickRectangle *rect = item->findChild<QQuickRectangle *>("rect");
    QVERIFY(rect);
    QTest::qWait(1000);
    QCOMPARE(rect->x(), qreal(0));
    QCOMPARE(rect->y(), qreal(0));
}

//QTBUG-12805
void tst_qquickanimations::nonTransitionBug()
{
    //tests that the animation values from the previous transition are properly cleared
    //in the case where an animation in the transition doesn't match anything (but previously did)
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("nonTransitionBug.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect);
    QQuickRectangle *mover = rect->findChild<QQuickRectangle*>("mover");

    mover->setX(100);
    QCOMPARE(mover->x(), qreal(100));

    rectPrivate->setState("left");
    QTRY_COMPARE(mover->x(), qreal(0));

    mover->setX(100);
    QCOMPARE(mover->x(), qreal(100));

    //make sure we don't try to animate back to 0
    rectPrivate->setState("free");
    QTest::qWait(300);
    QCOMPARE(mover->x(), qreal(100));
}

//QTBUG-14042
void tst_qquickanimations::registrationBug()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("registrationBug.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect != nullptr);
    QTRY_COMPARE(rect->property("value"), QVariant(int(100)));
}

void tst_qquickanimations::doubleRegistrationBug()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("doubleRegistrationBug.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect != nullptr);

    QQuickAbstractAnimation *anim = rect->findChild<QQuickAbstractAnimation*>("animation");
    QVERIFY(anim != nullptr);
    QTRY_COMPARE(anim->qtAnimation()->state(), QAbstractAnimationJob::Stopped);
}

//QTBUG-16736
void tst_qquickanimations::alwaysRunToEndRestartBug()
{
    QQuickRectangle rect;
    QQuickPropertyAnimation animation;
    animation.setTargetObject(&rect);
    animation.setProperty("x");
    animation.setTo(200);
    animation.setDuration(1000);
    animation.setLoops(-1);
    animation.setAlwaysRunToEnd(true);
    QCOMPARE(animation.loops(), -1);
    QVERIFY(animation.alwaysRunToEnd());
    animation.start();
    animation.stop();
    animation.start();
    animation.stop();

    // Waiting for a fixed time here would be dangerous as the starting and stopping itself takes
    // time and clocks are unreliable. The only thing we do know is that the animation should
    // eventually start and eventually stop. As its duration is 1000ms we can be pretty sure to hit
    // an in between state with the 50ms iterations QTRY_VERIFY does.
    QTRY_VERIFY(rect.x() != qreal(200));
    QTRY_COMPARE(rect.x(), qreal(200));
    QCOMPARE(static_cast<QQuickAbstractAnimation*>(&animation)->qtAnimation()->state(), QAbstractAnimationJob::Stopped);
}

//QTBUG-20227
void tst_qquickanimations::transitionAssignmentBug()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("transitionAssignmentBug.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect != nullptr);

    QCOMPARE(rect->property("nullObject").toBool(), false);
}

//QTBUG-19080
void tst_qquickanimations::pauseBindingBug()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("pauseBindingBug.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect != nullptr);
    QQuickAbstractAnimation *anim = rect->findChild<QQuickAbstractAnimation*>("animation");
    QCOMPARE(anim->qtAnimation()->state(), QAbstractAnimationJob::Paused);
}

//QTBUG-13598
void tst_qquickanimations::pauseBug()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("pauseBug.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *anim = qobject_cast<QQuickAbstractAnimation*>(obj.data());
    QVERIFY(anim != nullptr);
    QCOMPARE(anim->qtAnimation()->state(), QAbstractAnimationJob::Paused);
    QCOMPARE(anim->isPaused(), true);
    QCOMPARE(anim->isRunning(), true);
}

//QTBUG-23092
void tst_qquickanimations::loopingBug()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("looping.qml"));
    QScopedPointer<QObject> obj(c.create());

    QQuickAbstractAnimation *anim = obj->findChild<QQuickAbstractAnimation*>();
    QVERIFY(anim != nullptr);
    QCOMPARE(anim->qtAnimation()->totalDuration(), 300);
    QCOMPARE(anim->isRunning(), true);
    QTRY_COMPARE(static_cast<QAnimationGroupJob*>(anim->qtAnimation())->children()->first()->currentLoop(), 2);
    QTRY_COMPARE(anim->isRunning(), false);

    QQuickRectangle *rect = obj->findChild<QQuickRectangle*>();
    QVERIFY(rect != nullptr);
    QCOMPARE(rect->rotation(), qreal(90));
}

//QTBUG-24532
void tst_qquickanimations::anchorBug()
{
    QQuickAnchorAnimation animation;
    animation.setDuration(5000);
    animation.setEasing(QEasingCurve(QEasingCurve::InOutBack));
    animation.start();
    animation.pause();

    QCOMPARE(animation.qtAnimation()->duration(), 5000);
    QCOMPARE(static_cast<QQuickBulkValueAnimator*>(animation.qtAnimation())->easingCurve(), QEasingCurve(QEasingCurve::InOutBack));
}

//ScriptAction should not match a StateChangeScript if no scriptName has been specified
void tst_qquickanimations::scriptActionBug()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("scriptActionBug.qml"));
    QScopedPointer<QObject> obj(c.create());

    //Both the ScriptAction and StateChangeScript should be triggered
    QCOMPARE(obj->property("actionTriggered").toBool(), true);
    QCOMPARE(obj->property("actionTriggered").toBool(), true);
}

//QTBUG-34851
void tst_qquickanimations::groupAnimationNullChildBug()
{
    {
        QQmlEngine engine;

        QQmlComponent c(&engine, testFileUrl("sequentialAnimationNullChildBug.qml"));
        QScopedPointer<QObject> root(c.create());
        QVERIFY(root);
    }

    {
        QQmlEngine engine;

        QQmlComponent c(&engine, testFileUrl("parallelAnimationNullChildBug.qml"));
        QScopedPointer<QObject> root(c.create());
        QVERIFY(root);
    }
}

//ScriptAction should not crash if changing a state in a transition
void tst_qquickanimations::scriptActionCrash()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("scriptActionCrash.qml"));
    QScopedPointer<QObject> obj(c.create());

    //just testing that we don't crash
    QTest::qWait(1000); //5x transition duration
}

// QTBUG-49364
// Test that we don't crash when the target of an Animator becomes
// invalid between the time the animator is started and the time the
// animator job is actually started
void tst_qquickanimations::animatorInvalidTargetCrash()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("animatorInvalidTargetCrash.qml"));
    QScopedPointer<QObject> obj(c.create());

    //just testing that we don't crash
    QTest::qWait(5000); //animator duration
}

Q_DECLARE_METATYPE(QList<QQmlError>)

// QTBUG-22141
void tst_qquickanimations::defaultPropertyWarning()
{
    QQmlEngine engine;

    qRegisterMetaType<QList<QQmlError> >();

    QSignalSpy warnings(&engine, SIGNAL(warnings(QList<QQmlError>)));
    QVERIFY(warnings.isValid());

    QQmlComponent component(&engine, testFileUrl("defaultRotationAnimation.qml"));
    QScopedPointer<QObject> obj(component.create());
    auto *root = qobject_cast<QQuickItem *>(obj.data());
    QVERIFY(root);

    QVERIFY(warnings.isEmpty());
}

// QTBUG-57666
void tst_qquickanimations::pathSvgAnimation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("pathSvgAnimation.qml"));
    QScopedPointer<QObject> obj(component.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect);

    QQuickRectangle *redRect = rect->findChild<QQuickRectangle*>();
    QVERIFY(redRect);
    QQuickPathAnimation *pathAnim = rect->findChild<QQuickPathAnimation*>();
    QVERIFY(pathAnim);

    QCOMPARE(redRect->x(), qreal(50));
    QCOMPARE(redRect->y(), qreal(50));

    pathAnim->start();
    QTRY_COMPARE(redRect->x(), qreal(200));
    QCOMPARE(redRect->y(), qreal(200));
}

// QTBUG-57666
void tst_qquickanimations::pathLineUnspecifiedXYBug()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("pathLineUnspecifiedXYBug.qml"));
    QScopedPointer<QObject> obj(component.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(rect);

    QQuickRectangle *redRect = rect->findChild<QQuickRectangle*>();
    QVERIFY(redRect);
    QQuickPathAnimation *pathAnim = rect->findChild<QQuickPathAnimation*>();
    QVERIFY(pathAnim);

    QCOMPARE(redRect->x(), qreal(50));
    QCOMPARE(redRect->y(), qreal(50));

    pathAnim->start();
    QTRY_COMPARE(redRect->x(), qreal(0));
    QCOMPARE(redRect->y(), qreal(0));
}

void tst_qquickanimations::unsetAnimatorProxyJobWindow()
{
    QQuickWindow window;
    QQuickItem item(window.contentItem());
    QQuickAbstractAnimation animation(&item);
    QAbstractAnimationJob *job = new QAbstractAnimationJob;
    QQuickAnimatorProxyJob proxy(job, &animation);
    QQuickItem dummy;
    item.setParentItem(&dummy);
    QSignalSpy spy(&window, SIGNAL(sceneGraphInitialized()));
    window.show();
    if (spy.size() < 1)
        spy.wait();
    QCOMPARE(proxy.job().data(), job);
}

void tst_qquickanimations::finished()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("finished.qml"));
    QScopedPointer<QObject> obj(component.create());
    auto *root = qobject_cast<QQuickItem *>(obj.data());
    QVERIFY(root);

    // Test that finished() is emitted for a simple top-level animation.
    // (Each test is in its own block so that we can reuse the nice signal names :))
    {
        QQuickAbstractAnimation *simpleTopLevelAnimation
            = root->property("simpleTopLevelAnimation").value<QQuickAbstractAnimation*>();
        QVERIFY(simpleTopLevelAnimation);

        QSignalSpy stoppedSpy(simpleTopLevelAnimation, SIGNAL(stopped()));
        QVERIFY(stoppedSpy.isValid());

        QSignalSpy finishedSpy(simpleTopLevelAnimation, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());

        QVERIFY(simpleTopLevelAnimation->setProperty("running", QVariant(true)));
        QTRY_COMPARE(stoppedSpy.size(), 1);
        QCOMPARE(finishedSpy.size(), 1);

        // Test that the signal is properly revisioned and hence accessible from QML.
        QCOMPARE(root->property("finishedUsableInQml").toBool(), true);
    }

    // Test that finished() is not emitted for animations within a Transition.
    {
        QObject *transition = root->property("transition").value<QObject*>();
        QVERIFY(transition);

        QSignalSpy runningChangedSpy(transition, SIGNAL(runningChanged()));
        QVERIFY(runningChangedSpy.isValid());

        QQuickAbstractAnimation *animationWithinTransition
            = root->property("animationWithinTransition").value<QQuickAbstractAnimation*>();
        QVERIFY(animationWithinTransition);

        QSignalSpy stoppedSpy(animationWithinTransition, SIGNAL(stopped()));
        QVERIFY(stoppedSpy.isValid());

        QSignalSpy finishedSpy(animationWithinTransition, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());

        QObject *transitionRect = root->property("transitionRect").value<QObject*>();
        QVERIFY(transitionRect);
        QVERIFY(transitionRect->setProperty("state", QVariant(QLatin1String("go"))));
        QTRY_COMPARE(runningChangedSpy.size(), 1);
        QCOMPARE(stoppedSpy.size(), 0);
        QCOMPARE(finishedSpy.size(), 0);
    }

    // Test that finished() is not emitted for animations within a Behavior.
    {
        QQuickAbstractAnimation *animationWithinBehavior
            = root->property("animationWithinBehavior").value<QQuickAbstractAnimation*>();
        QVERIFY(animationWithinBehavior);

        QSignalSpy stoppedSpy(animationWithinBehavior, SIGNAL(stopped()));
        QVERIFY(stoppedSpy.isValid());

        QSignalSpy finishedSpy(animationWithinBehavior, SIGNAL(finished()));
        QVERIFY(finishedSpy.isValid());

        QVERIFY(root->setProperty("bar", QVariant(1.0)));
        QTRY_COMPARE(root->property("bar").toReal(), 1.0);
        QCOMPARE(stoppedSpy.size(), 0);
        QCOMPARE(finishedSpy.size(), 0);
    }
}

void tst_qquickanimations::replacingTransitions()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("replacingTransitions.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *rect = qobject_cast<QQuickRectangle*>(obj.data());
    if (!c.errors().isEmpty())
        qDebug() << c.errorString();
    QVERIFY(rect);

    QQmlTimer *addTimer = rect->property("addTimer").value<QQmlTimer*>();
    QVERIFY(addTimer);
    QCOMPARE(addTimer->isRunning(), false);

    QQuickTransition *addTrans = rect->property("addTransition").value<QQuickTransition*>();
    QVERIFY(addTrans);
    QCOMPARE(addTrans->running(), false);

    QQuickTransition *displaceTrans = rect->property("displaceTransition").value<QQuickTransition*>();
    QVERIFY(displaceTrans);
    QCOMPARE(displaceTrans->running(), false);

    QQmlListModel *model = rect->property("model").value<QQmlListModel *>();
    QVERIFY(model);
    QCOMPARE(model->count(), 0);

    addTimer->start();
    QTest::qWait(1000 + 1000 + 10000);

    QTRY_COMPARE(addTimer->isRunning(), false);
    QTRY_COMPARE(addTrans->running(), false);
    QTRY_COMPARE(displaceTrans->running(), false);
    QCOMPARE(model->count(), 3);
}

void tst_qquickanimations::animationJobSelfDestruction()
{
    // Don't crash
    QQmlEngine engine;
    engine.clearComponentCache();
    QQmlComponent c(&engine, testFileUrl("animationJobSelfDestructionBug.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *win = qobject_cast<QQuickWindow *>(obj.data());
    if (!c.errors().isEmpty())
        qDebug() << c.errorString();
    QVERIFY(win);
    win->setTitle(QTest::currentTestFunction());
    win->show();
    QVERIFY(QTest::qWaitForWindowExposed(win));
    QQmlTimer *timer = win->property("timer").value<QQmlTimer*>();
    QVERIFY(timer);
    QCOMPARE(timer->isRunning(), false);
    timer->start();
    QTest::qWait(1000);
}

void tst_qquickanimations::fastFlickingBug()
{
    // Don't crash
    QQmlEngine engine;
    engine.clearComponentCache();
    QQmlComponent c(&engine, testFileUrl("fastFlickingBug.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *win = qobject_cast<QQuickWindow *>(obj.data());
    if (!c.errors().isEmpty())
        qDebug() << c.errorString();
    QVERIFY(win);
    win->setTitle(QTest::currentTestFunction());
    win->show();
    QVERIFY(QTest::qWaitForWindowExposed(win));
    auto timer = win->property("timer").value<QQmlTimer*>();
    QVERIFY(timer);
    QCOMPARE(timer->isRunning(), false);
    auto listView = win->property("listView").value<QQuickFlickable*>();
    QVERIFY(listView);
    timer->start();
    // flick listView up and down quickly in the middle of a slow transition
    for (int sign = 1; timer->isRunning(); sign *= -1) {
        listView->flick(0, sign * 4000);
        qApp->processEvents();
        QTest::qWait(53);
        qApp->processEvents();
    }
}

void tst_qquickanimations::opacityAnimationFromZero()
{
    if (QGuiApplication::platformName() == QLatin1String("minimal"))
        QSKIP("Skipping due to grabWindow not functional on minimal platforms");

    // not easy to verify this in threaded render loop
    // since it's difficult to capture the first frame when scene graph
    // is renderred in another thread
    qputenv("QSG_RENDER_LOOP", "basic");
    auto cleanup = qScopeGuard([]() { qputenv("QSG_RENDER_LOOP", ""); });

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("opacityAnimationFromZero.qml"));
    QScopedPointer<QQuickWindow> win(qobject_cast<QQuickWindow*>(c.create()));
    if (!c.errors().isEmpty())
        qDebug() << c.errorString();
    QVERIFY(win);
    win->setTitle(QTest::currentTestFunction());
    win->show();
    QVERIFY(QTest::qWaitForWindowExposed(win.data()));

    QImage img;
    bool firstFrameSwapped = false;
    QObject::connect(win.get(), &QQuickWindow::frameSwapped, win.get(), [&win, &img, &firstFrameSwapped]() {
        if (firstFrameSwapped)
            return;
        else
            firstFrameSwapped = true;
        img = win->grabWindow();
        if (img.width() < win->width())
            QSKIP("Skipping due to grabWindow not functional");
    });
    QTRY_VERIFY(!img.isNull() && img.pixel(100, 100) > qRgb(10, 10, 10));
}

void tst_qquickanimations::alwaysRunToEndInSequentialAnimationBug()
{
    QQuickView view(testFileUrl("alwaysRunToEndInSequentialAnimationBug.qml"));
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(view.rootObject());
    QObject *root = view.rootObject();
    QQuickAbstractAnimation* seqAnim = root->property("seqAnim").value<QQuickAbstractAnimation *>();
    QObject *whiteRect = root->findChild<QObject*>("whiteRect");

    //
    // Tesing the SequentialAnimation with alwaysRunToEnd = true
    //

    //
    // Stopping in the first loop

    QVERIFY(whiteRect->property("opacity").value<qreal>() == 1.0);
    seqAnim->start();
    QTRY_VERIFY(seqAnim->isRunning() == true);

    seqAnim->stop();
    QTRY_VERIFY(seqAnim->isRunning() == false);
    QTRY_VERIFY(!root->property("onStoppedCalled").value<bool>());
    QTRY_VERIFY(!root->property("onFinishedCalled").value<bool>());

    //The animation should still be running
    QTest::qWait(500);
    QTRY_VERIFY(whiteRect->property("opacity").value<qreal>() != 1.0);

    QTest::qWait(500);

    //The animation should now be stopped - after reaching its cycle end
    QTRY_COMPARE(whiteRect->property("opacity").value<qreal>(), 1.0);
    QTRY_VERIFY(root->property("onStoppedCalled").value<bool>());
    QTRY_VERIFY(root->property("onFinishedCalled").value<bool>());

    //
    // Stopping in the second loop

    QVERIFY(whiteRect->property("opacity").value<qreal>() == 1.0);
    seqAnim->start();
    QTRY_VERIFY(seqAnim->isRunning() == true);

    QTRY_COMPARE(root->property("loopsMade").value<int>(), 1); // Wait for cycle no 2 to start

    seqAnim->stop();
    QTRY_VERIFY(seqAnim->isRunning() == false);
    QTRY_VERIFY(!root->property("onStoppedCalled").value<bool>());
    QTRY_VERIFY(!root->property("onFinishedCalled").value<bool>());

    //The animation should still be running
    QTest::qWait(500);
    QTRY_VERIFY(whiteRect->property("opacity").value<qreal>() != 1.0);

    QTest::qWait(500);

    //The animation should now be stopped - after reaching its cycle end
    QTRY_COMPARE(whiteRect->property("opacity").value<qreal>(), 1.0);
    QTRY_VERIFY(root->property("onStoppedCalled").value<bool>());
    QTRY_VERIFY(root->property("onFinishedCalled").value<bool>());


    //
    // Tesing the SequentialAnimation with alwaysRunToEnd = false
    //

    //
    // Stopping in the first loop

    seqAnim->setProperty("alwaysRunToEnd", false);
    QVERIFY(whiteRect->property("opacity").value<qreal>() == 1.0);

    seqAnim->start();
    QTRY_VERIFY(seqAnim->isRunning() == true);

    QTest::qWait(50);
    seqAnim->stop();
    //The animation should be stopped immediately
    QVERIFY(seqAnim->isRunning() == false);
    QVERIFY(root->property("onStoppedCalled").value<bool>());
    QVERIFY(root->property("onFinishedCalled").value<bool>());
    QCOMPARE(whiteRect->property("opacity").value<qreal>(), 1.0);

    //
    // Stopping in the second loop
    QVERIFY(whiteRect->property("opacity").value<qreal>() == 1.0);
    seqAnim->start();
    QTRY_VERIFY(seqAnim->isRunning() == true);

    QTRY_COMPARE(root->property("loopsMade").value<int>(), 1); // Wait for cycle no 2 to start

    QTest::qWait(50);
    seqAnim->stop();
    //The animation should be stopped immediately
    QVERIFY(seqAnim->isRunning() == false);
    QVERIFY(root->property("onStoppedCalled").value<bool>());
    QVERIFY(root->property("onFinishedCalled").value<bool>());
    QCOMPARE(whiteRect->property("opacity").value<qreal>(),1.0);
}

void tst_qquickanimations::cleanupWhenRenderThreadStops()
{
    QQuickView view(testFileUrl("cleanupWhenRenderThreadStops.qml"));
    view.show();
    view.setPersistentGraphics(false);
    view.setPersistentSceneGraph(false);
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTest::qWait(50);
    view.hide();
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
}

// This will be called each frame and should return true for the test to pass.
typedef std::function<bool(QQuickItem *, QString &)> PropertyValidatorFunc;
Q_DECLARE_METATYPE(PropertyValidatorFunc)

void tst_qquickanimations::changePropertiesDuringAnimation_data()
{
    QTest::addColumn<int>("loops");
    QTest::addColumn<QString>("propertyName");
    QTest::addColumn<qreal>("newValue");
    QTest::addColumn<PropertyValidatorFunc>("propertyValidatorFunc");

    // Use a value large enough to ensure that the animation is running for the duration of
    // the test. We test both infinite and non-infinite loop counts.
    const int largeLoopCount = 100;

    const auto fromValidator = PropertyValidatorFunc([](QQuickItem *rect, QString &failureMessage){
        if (rect->x() >= 0)
            return true;
        QDebug(&failureMessage) << "Expected x of rect to never go below new \"from\" value of 0, but it's" << rect->x();
        return false;
    });
    QTest::newRow("from") << largeLoopCount << "from" << 0.0 << fromValidator;
    QTest::newRow("from,infinite") << int(QQuickAbstractAnimation::Infinite) << "from" << 0.0 << fromValidator;

    const auto toValidator = PropertyValidatorFunc([](QQuickItem *rect, QString &failureMessage){
        if (rect->x() <= 100)
            return true;
        QDebug(&failureMessage) << "Expected x of rect to never go above new \"to\" value of 100, but it's" << rect->x();
        return false;
    });
    QTest::newRow("to") << largeLoopCount << "to" << 100.0 << toValidator;
    QTest::newRow("to,infinite") << int(QQuickAbstractAnimation::Infinite) << "to" << 100.0 << toValidator;

    // Duration and easing.type would be difficult/flaky to test in CI so they're left out here.
}

// Tests that changing a NumberAnimation's properties while it's running will result
// in those changes being picked up on the next loop. This is new behavior introduced
// in Qt 6.4.
void tst_qquickanimations::changePropertiesDuringAnimation()
{
    QFETCH(int, loops);
    QFETCH(QString, propertyName);
    QFETCH(qreal, newValue);
    QFETCH(PropertyValidatorFunc, propertyValidatorFunc);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("changePropertiesDuringAnimation.qml"));
    QScopedPointer<QQuickItem> rootItem(qobject_cast<QQuickItem*>(component.createWithInitialProperties({{ "loops", loops }})));
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    auto numberAnimation = rootItem->property("numberAnimation").value<QQuickNumberAnimation*>();
    QVERIFY(numberAnimation);
    QCOMPARE(numberAnimation->from(), -100);
    QCOMPARE(numberAnimation->to(), rootItem->width());

    // Start the animation.
    numberAnimation->start();
    QVERIFY(numberAnimation->isRunning());

    int loopCountBeforeModification = 0;
    // Ensure that it's past the first loop so that we can check that it resumes
    // from that loop after "restarting".
    QTRY_VERIFY(numberAnimation->qtAnimation()->currentLoop() >= 1);
    loopCountBeforeModification = numberAnimation->qtAnimation()->currentLoop();

    QSignalSpy startedSpy(numberAnimation, SIGNAL(started()));
    QVERIFY(startedSpy.isValid());
    QSignalSpy stoppedSpy(numberAnimation, SIGNAL(stopped()));
    QVERIFY(stoppedSpy.isValid());

    // Modify the property.
    // QQuickPropertyAnimation has a setProperty function of its own, and we don't want to call it, hence the cast.
    QVERIFY(static_cast<QObject*>(numberAnimation)->setProperty(propertyName.toLatin1().constData(), QVariant(newValue)));

    // Make sure we've reached the end of the animation.
    auto rect = rootItem->property("rect").value<QQuickItem*>();
    QVERIFY(rect);
    // Ensure that we've passed the loop on which we modified the property, while also checking
    // that currentLoop never gets reset to 0. We can't just use QTRY_VERIFY
    // for this, because it could start at 0 and then pass loopCountBeforeModification;
    // we need to ensure that it never goes below loopCountBeforeModification.
    while (numberAnimation->qtAnimation()->currentLoop() < loopCountBeforeModification + 1) {
        QVERIFY2(numberAnimation->qtAnimation()->currentLoop() >= loopCountBeforeModification,
            qPrintable(QString::fromLatin1("Expected currentLoop to be larger than %1, but it's %2")
                .arg(loopCountBeforeModification).arg(numberAnimation->qtAnimation()->currentLoop())));
        QTest::qWait(0);
    }

    // Now that we know the modification should have been taken into account,
    // check that the animated property never gets set to a value that we wouldn't expect after the change.
    const int previousLoop = numberAnimation->qtAnimation()->currentLoop();
    QString failureMessage;
    while (numberAnimation->qtAnimation()->currentLoop() < previousLoop + 1) {
        if (!propertyValidatorFunc(rect, failureMessage))
            QFAIL(qPrintable(failureMessage));
        QTest::qWait(0);
    }

    // The started and stopped signals should not be emitted when adapting to changes
    // mid-animation.
    if (loops != QQuickAbstractAnimation::Infinite)
        QVERIFY(numberAnimation->qtAnimation()->currentLoop() < numberAnimation->loops());
    QCOMPARE(startedSpy.size(), 0);
    QCOMPARE(stoppedSpy.size(), 0);
}

void tst_qquickanimations::infiniteLoopsWithoutFrom()
{
    // This test checks QTBUG-84375
    QQuickView view(testFileUrl("infiniteAnimationWithoutFrom.qml"));
    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(view.rootObject());

    QObject *root = view.rootObject();
    QQuickAbstractAnimation *animation = root->findChild<QQuickAbstractAnimation *>("anim");
    QVERIFY(animation);
    QQuickRectangle *rectangle = root->findChild<QQuickRectangle *>("rect");
    QVERIFY(rectangle);

    qreal prevRotation = rectangle->rotation();
    int numsCrossedZero = 0;
    connect(rectangle, &QQuickRectangle::rotationChanged, this, [&]() {
        const auto rotation = rectangle->rotation();
        // We take a large range of (180; 360] here, because the animation in
        // the test runs for a short time, and so there can be huge gaps between
        // rotation values.
        const bool prevRotationOldLoop = prevRotation > 180.0 && prevRotation <= 360.0;
        const bool currRotationNewLoop = rotation >= 0.0 && rotation <= 180.0;
        if (prevRotationOldLoop && currRotationNewLoop)
            numsCrossedZero++;
        prevRotation = rotation;
    });

    // The logic in lamdba function above requires at least two positions in
    // a rotation animation - one in [0; 180] range, and another in (180; 360]
    // range
    animation->start();
    animation->pause();
    QCOMPARE(numsCrossedZero, 0);
    animation->setCurrentTime(40);
    animation->setCurrentTime(90);
    QCOMPARE(numsCrossedZero, 0);
    animation->setCurrentTime(140);
    animation->setCurrentTime(190);
    QCOMPARE(numsCrossedZero, 1);
    animation->setCurrentTime(240);
    animation->setCurrentTime(290);
    QCOMPARE(numsCrossedZero, 2);

    animation->stop();
}

void tst_qquickanimations::frameAnimation1()
{
    QQuickFrameAnimation frameAnimation;
    QVERIFY(!frameAnimation.isRunning());
    QVERIFY(!frameAnimation.isPaused());
    QCOMPARE(frameAnimation.currentFrame(), 0);
    QCOMPARE(frameAnimation.frameTime(), 0);
    QCOMPARE(frameAnimation.smoothFrameTime(), 0);
    QCOMPARE(frameAnimation.elapsedTime(), 0);

    frameAnimation.start();
    QVERIFY(frameAnimation.isRunning());
    QVERIFY(!frameAnimation.isPaused());
    frameAnimation.pause();
    QVERIFY(frameAnimation.isRunning());
    QVERIFY(frameAnimation.isPaused());
    frameAnimation.resume();
    QVERIFY(frameAnimation.isRunning());
    QVERIFY(!frameAnimation.isPaused());
    frameAnimation.stop();
    QVERIFY(!frameAnimation.isRunning());
    QVERIFY(!frameAnimation.isPaused());
    frameAnimation.restart();
    QVERIFY(frameAnimation.isRunning());
    QVERIFY(!frameAnimation.isPaused());
}

void tst_qquickanimations::frameAnimation2()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("frameAnimation.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *root = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY(root);

    QQuickFrameAnimation *frameAnimation = root->findChild<QQuickFrameAnimation*>();
    QVERIFY(frameAnimation);
    QSignalSpy spy(frameAnimation, SIGNAL(triggered()));

    // Start the animation and wait at least 1 frame
    frameAnimation->start();
    QVERIFY(frameAnimation->isRunning());
    QVERIFY(spy.wait(500));
    QVERIFY(frameAnimation->currentFrame() > 0);
    QVERIFY(frameAnimation->frameTime() > 0);
    QVERIFY(frameAnimation->smoothFrameTime() > 0);
    QVERIFY(frameAnimation->elapsedTime() > 0);

    // Stopping and reseting should return currentFrame back to 0
    frameAnimation->stop();
    frameAnimation->reset();
    QCOMPARE(frameAnimation->currentFrame(), 0);

    // Start and wait so the animation runs into frame 3 and pauses
    frameAnimation->start();
    QTRY_VERIFY(frameAnimation->isPaused());
    QVERIFY(frameAnimation->currentFrame() >= 3);

    // Then resume the animation
    frameAnimation->resume();
    QVERIFY(!frameAnimation->isPaused());
    QVERIFY(spy.wait(500));
    QVERIFY(frameAnimation->currentFrame() > 3);
}

//QTBUG-110589
void tst_qquickanimations::restartAnimationGroupWhenDirty()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restartAnimationGroupWhenDirty.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *root = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY2(root, qPrintable(c.errorString()));

    QQuickSequentialAnimation *seqAnim0 = root->findChild<QQuickSequentialAnimation*>("seqAnim0");
    QVERIFY(seqAnim0);
    QQuickRectangle *target0 = root->findChild<QQuickRectangle*>("target0");
    QVERIFY(target0);
    QQuickParallelAnimation *parAnim0 = root->findChild<QQuickParallelAnimation*>("parAnim0");
    QVERIFY(parAnim0);
    QQuickRectangle *target1 = root->findChild<QQuickRectangle*>("target1");
    QVERIFY(target1);

    QTRY_VERIFY(seqAnim0->isPaused());
    QTRY_VERIFY(parAnim0->isPaused());
    QTRY_VERIFY(target0->x() > 140);
    QTRY_VERIFY(target1->x() > 140);
    seqAnim0->resume();
    parAnim0->resume();
    QTRY_VERIFY(target0->property("onFinishedCalled").value<bool>());
    QTRY_VERIFY(target1->property("onFinishedCalled").value<bool>());
    QTRY_COMPARE(target0->x(), 140);
    QTRY_COMPARE(target1->x(), 140);
}

//QTBUG-95840
void tst_qquickanimations::restartNestedAnimationGroupWhenDirty()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restartNestedAnimationGroupWhenDirty.qml"));
    QScopedPointer<QObject> obj(c.create());
    auto *root = qobject_cast<QQuickRectangle*>(obj.data());
    QVERIFY2(root, qPrintable(c.errorString()));

    QQuickSequentialAnimation *seqAnim0 = root->findChild<QQuickSequentialAnimation*>("seqAnim0");
    QVERIFY(seqAnim0);
    QQuickRectangle *target0 = root->findChild<QQuickRectangle*>("target0");
    QVERIFY(target0);
    QQuickParallelAnimation *parAnim0 = root->findChild<QQuickParallelAnimation*>("parAnim0");
    QVERIFY(parAnim0);
    QQuickRectangle *target1 = root->findChild<QQuickRectangle*>("target1");
    QVERIFY(target1);

    QTRY_VERIFY(seqAnim0->isPaused());
    QTRY_VERIFY(parAnim0->isPaused());
    QTRY_VERIFY(target0->x() > 140);
    QTRY_VERIFY(target1->x() > 140);
    seqAnim0->resume();
    parAnim0->resume();
    QTRY_VERIFY(target0->property("onFinishedCalled").value<bool>());
    QTRY_VERIFY(target1->property("onFinishedCalled").value<bool>());
    QTRY_COMPARE(target0->x(), 140);
    QTRY_COMPARE(target1->x(), 140);
}

void tst_qquickanimations::targetsDeletedNotRemoved()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("targetsDeletedWithoutRemoval.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY2(obj.get(), qPrintable(component.errorString()));
    {
        QQmlListReference ref(obj.get(), "targets");
        QVERIFY(ref.isValid());
        QCOMPARE(ref.size(), 1);
        QTRY_COMPARE(ref.at(0), nullptr);
    }
    {
        QQmlListReference ref(obj.get(), "animTargets");
        QVERIFY(ref.isValid());
        QCOMPARE(ref.size(), 1);
        QCOMPARE(ref.at(0), nullptr);
    }
}

QTEST_MAIN(tst_qquickanimations)

#include "tst_qquickanimations.moc"
