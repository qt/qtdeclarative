/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QtTest>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qdeclarativeanimation_p.h>
#include <QtQuick/private/qdeclarativetransition_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qdeclarativepathinterpolator_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QVariantAnimation>
#include <QEasingCurve>

#include <limits.h>
#include <math.h>

#include "../../shared/util.h"

class tst_qdeclarativeanimations : public QDeclarativeDataTest
{
    Q_OBJECT
public:
    tst_qdeclarativeanimations() {}

private slots:
    void initTestCase()
    {
        QDeclarativeEngine engine;  // ensure types are registered
        QDeclarativeDataTest::initTestCase();
    }

    void simpleProperty();
    void simpleNumber();
    void simpleColor();
    void simpleRotation();
    void simplePath();
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
    void runningTrueBug();
    void nonTransitionBug();
    void registrationBug();
    void doubleRegistrationBug();
    void alwaysRunToEndRestartBug();
    void transitionAssignmentBug();
    void pauseBindingBug();
    void pauseBug();
};

#define QTIMED_COMPARE(lhs, rhs) do { \
    for (int ii = 0; ii < 5; ++ii) { \
        if (lhs == rhs)  \
            break; \
        QTest::qWait(50); \
    } \
    QCOMPARE(lhs, rhs); \
} while (false)

void tst_qdeclarativeanimations::simpleProperty()
{
    QQuickRectangle rect;
    QDeclarativePropertyAnimation animation;
    animation.setTarget(&rect);
    animation.setProperty("x");
    animation.setTo(200);
    QVERIFY(animation.target() == &rect);
    QVERIFY(animation.property() == "x");
    QVERIFY(animation.to().toReal() == 200.0);
    animation.start();
    QVERIFY(animation.isRunning());
    QTest::qWait(animation.duration());
    QTIMED_COMPARE(rect.x(), 200.0);

    rect.setPos(QPointF(0,0));
    animation.start();
    animation.pause();
    QVERIFY(animation.isRunning());
    QVERIFY(animation.isPaused());
    animation.setCurrentTime(125);
    QVERIFY(animation.currentTime() == 125);
    QCOMPARE(rect.x(),100.0);
}

void tst_qdeclarativeanimations::simpleNumber()
{
    QQuickRectangle rect;
    QDeclarativeNumberAnimation animation;
    animation.setTarget(&rect);
    animation.setProperty("x");
    animation.setTo(200);
    QVERIFY(animation.target() == &rect);
    QVERIFY(animation.property() == "x");
    QVERIFY(animation.to() == 200);
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
    QVERIFY(animation.currentTime() == 125);
    QCOMPARE(rect.x(), qreal(100));
}

void tst_qdeclarativeanimations::simpleColor()
{
    QQuickRectangle rect;
    QDeclarativeColorAnimation animation;
    animation.setTarget(&rect);
    animation.setProperty("color");
    animation.setTo(QColor("red"));
    QVERIFY(animation.target() == &rect);
    QVERIFY(animation.property() == "color");
    QVERIFY(animation.to() == QColor("red"));
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
    QVERIFY(animation.currentTime() == 125);
#if defined(UBUNTU_ONEIRIC) && defined(__x86_64__)
    QEXPECT_FAIL("", "Fails on this platform - QTBUG-23385", Abort);
#endif
    QCOMPARE(rect.color(), QColor::fromRgbF(0.498039, 0, 0.498039, 1));

    rect.setColor(QColor("green"));
    animation.setFrom(QColor("blue"));
    QVERIFY(animation.from() == QColor("blue"));
    animation.restart();
    QCOMPARE(rect.color(), QColor("blue"));
    QVERIFY(animation.isRunning());
    animation.setCurrentTime(125);
    QCOMPARE(rect.color(), QColor::fromRgbF(0.498039, 0, 0.498039, 1));
}

void tst_qdeclarativeanimations::simpleRotation()
{
    QQuickRectangle rect;
    QDeclarativeRotationAnimation animation;
    animation.setTarget(&rect);
    animation.setProperty("rotation");
    animation.setTo(270);
    QVERIFY(animation.target() == &rect);
    QVERIFY(animation.property() == "rotation");
    QVERIFY(animation.to() == 270);
    QVERIFY(animation.direction() == QDeclarativeRotationAnimation::Numerical);
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
    QVERIFY(animation.currentTime() == 125);
    QCOMPARE(rect.rotation(), qreal(135));
}

void tst_qdeclarativeanimations::simplePath()
{
    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("pathAnimation.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickRectangle *redRect = rect->findChild<QQuickRectangle*>();
        QVERIFY(redRect);
        QQuickPathAnimation *pathAnim = rect->findChild<QQuickPathAnimation*>();
        QVERIFY(pathAnim);

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
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("pathAnimation2.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickRectangle *redRect = rect->findChild<QQuickRectangle*>();
        QVERIFY(redRect);
        QQuickPathAnimation *pathAnim = rect->findChild<QQuickPathAnimation*>();
        QVERIFY(pathAnim);

        QCOMPARE(pathAnim->orientation(), QQuickPathAnimation::RightFirst);

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

void tst_qdeclarativeanimations::pathInterpolator()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("pathInterpolator.qml"));
    QDeclarativePathInterpolator *interpolator = qobject_cast<QDeclarativePathInterpolator*>(c.create());
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
}

void tst_qdeclarativeanimations::pathInterpolatorBackwardJump()
{
    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("pathInterpolatorBack.qml"));
        QDeclarativePathInterpolator *interpolator = qobject_cast<QDeclarativePathInterpolator*>(c.create());
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
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("pathInterpolatorBack2.qml"));
        QDeclarativePathInterpolator *interpolator = qobject_cast<QDeclarativePathInterpolator*>(c.create());
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

void tst_qdeclarativeanimations::pathWithNoStart()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("pathAnimationNoStart.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
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

void tst_qdeclarativeanimations::alwaysRunToEnd()
{
    QQuickRectangle rect;
    QDeclarativePropertyAnimation animation;
    animation.setTarget(&rect);
    animation.setProperty("x");
    animation.setTo(200);
    animation.setDuration(1000);
    animation.setLoops(-1);
    animation.setAlwaysRunToEnd(true);
    QVERIFY(animation.loops() == -1);
    QVERIFY(animation.alwaysRunToEnd() == true);
    animation.start();
    QTest::qWait(1500);
    animation.stop();
    QVERIFY(rect.x() != qreal(200));
    QTest::qWait(500);
    QTIMED_COMPARE(rect.x(), qreal(200));
}

void tst_qdeclarativeanimations::complete()
{
    QQuickRectangle rect;
    QDeclarativePropertyAnimation animation;
    animation.setTarget(&rect);
    animation.setProperty("x");
    animation.setFrom(1);
    animation.setTo(200);
    animation.setDuration(500);
    QVERIFY(animation.from() == 1);
    animation.start();
    QTest::qWait(50);
    animation.stop();
    QVERIFY(rect.x() != qreal(200));
    animation.start();
    QTest::qWait(50);
    QVERIFY(animation.isRunning());
    animation.complete();
    QCOMPARE(rect.x(), qreal(200));
}

void tst_qdeclarativeanimations::resume()
{
    QQuickRectangle rect;
    QDeclarativePropertyAnimation animation;
    animation.setTarget(&rect);
    animation.setProperty("x");
    animation.setFrom(10);
    animation.setTo(200);
    animation.setDuration(1000);
    QVERIFY(animation.from() == 10);

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
}

void tst_qdeclarativeanimations::dotProperty()
{
    QQuickRectangle rect;
    QDeclarativeNumberAnimation animation;
    animation.setTarget(&rect);
    animation.setProperty("border.width");
    animation.setTo(10);
    animation.start();
    QTest::qWait(animation.duration()+50);
    QTIMED_COMPARE(rect.border()->width(), 10.0);

    rect.border()->setWidth(0);
    animation.start();
    animation.pause();
    animation.setCurrentTime(125);
    QVERIFY(animation.currentTime() == 125);
    QCOMPARE(rect.border()->width(), 5.0);
}

void tst_qdeclarativeanimations::badTypes()
{
    //don't crash
    {
        QQuickView *view = new QQuickView;
        view->setSource(testFileUrl("badtype1.qml"));

        qApp->processEvents();

        delete view;
    }

    //make sure we get a compiler error
    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("badtype2.qml"));
        QTest::ignoreMessage(QtWarningMsg, "QDeclarativeComponent: Component is not ready");
        c.create();

        QVERIFY(c.errors().count() == 1);
        QCOMPARE(c.errors().at(0).description(), QLatin1String("Invalid property assignment: number expected"));
    }

    //make sure we get a compiler error
    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("badtype3.qml"));
        QTest::ignoreMessage(QtWarningMsg, "QDeclarativeComponent: Component is not ready");
        c.create();

        QVERIFY(c.errors().count() == 1);
        QCOMPARE(c.errors().at(0).description(), QLatin1String("Invalid property assignment: color expected"));
    }

    //don't crash
    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("badtype4.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickItemPrivate::get(rect)->setState("state1");
        QTest::qWait(1000 + 50);
        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("MyRect");
        QVERIFY(myRect);
        QCOMPARE(myRect->x(),qreal(200));
    }
}

void tst_qdeclarativeanimations::badProperties()
{
    //make sure we get a runtime error
    {
        QDeclarativeEngine engine;

        QDeclarativeComponent c1(&engine, testFileUrl("badproperty1.qml"));
        QByteArray message = testFileUrl("badproperty1.qml").toString().toUtf8() + ":18:9: QML ColorAnimation: Cannot animate non-existent property \"border.colr\"";
        QTest::ignoreMessage(QtWarningMsg, message);
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c1.create());
        QVERIFY(rect);

        QDeclarativeComponent c2(&engine, testFileUrl("badproperty2.qml"));
        message = testFileUrl("badproperty2.qml").toString().toUtf8() + ":18:9: QML ColorAnimation: Cannot animate read-only property \"border\"";
        QTest::ignoreMessage(QtWarningMsg, message);
        rect = qobject_cast<QQuickRectangle*>(c2.create());
        QVERIFY(rect);

        //### should we warn here are well?
        //rect->setState("state1");
    }
}

//test animating mixed types with property animation in a transition
//for example, int + real; color + real; etc
void tst_qdeclarativeanimations::mixedTypes()
{
    //assumes border.width stays a real -- not real robust
    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("mixedtype1.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickItemPrivate::get(rect)->setState("state1");
        QTest::qWait(500);
        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("MyRect");
        QVERIFY(myRect);

        //rather inexact -- is there a better way?
        QVERIFY(myRect->x() > 100 && myRect->x() < 200);
        QVERIFY(myRect->border()->width() > 1 && myRect->border()->width() < 10);
    }

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("mixedtype2.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickItemPrivate::get(rect)->setState("state1");
        QTest::qWait(500);
        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("MyRect");
        QVERIFY(myRect);

        //rather inexact -- is there a better way?
        QVERIFY(myRect->x() > 100 && myRect->x() < 200);
#if defined(UBUNTU_ONEIRIC) && defined(__x86_64__)
        QEXPECT_FAIL("", "Fails on this platform - QTBUG-23385", Continue);
#endif
        QVERIFY(myRect->color() != QColor("red") && myRect->color() != QColor("blue"));
    }
}

void tst_qdeclarativeanimations::properties()
{
    const int waitDuration = 300;
    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("properties.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("properties2.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("properties3.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(300));
    }

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("properties4.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->y(),qreal(200));
        QTIMED_COMPARE(myRect->x(),qreal(100));
    }

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("properties5.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(100));
        QTIMED_COMPARE(myRect->y(),qreal(200));
    }
}

void tst_qdeclarativeanimations::propertiesTransition()
{
    const int waitDuration = 300;
    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("propertiesTransition.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickItemPrivate::get(rect)->setState("moved");
        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("propertiesTransition2.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
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
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("propertiesTransition3.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QQuickItemPrivate::get(rect)->setState("moved");
        QCOMPARE(myRect->x(),qreal(200));
        QCOMPARE(myRect->y(),qreal(100));
    }

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("propertiesTransition4.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QQuickItemPrivate::get(rect)->setState("moved");
        QCOMPARE(myRect->x(),qreal(100));
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("propertiesTransition5.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QQuickItemPrivate::get(rect)->setState("moved");
        QCOMPARE(myRect->x(),qreal(100));
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

    /*{
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("propertiesTransition6.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QQuickItemPrivate::get(rect)->setState("moved");
        QCOMPARE(myRect->x(),qreal(100));
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(100));
    }*/

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, testFileUrl("propertiesTransition7.qml"));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QQuickItemPrivate::get(rect)->setState("moved");
        QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
        QVERIFY(myRect);
        QTest::qWait(waitDuration);
        QTIMED_COMPARE(myRect->x(),qreal(200));
    }

}

void tst_qdeclarativeanimations::pathTransition()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("pathTransition.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
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

void tst_qdeclarativeanimations::disabledTransition()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("disabledTransition.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect);

    QQuickRectangle *myRect = rect->findChild<QQuickRectangle*>("TheRect");
    QVERIFY(myRect);

    QDeclarativeTransition *trans = rect->findChild<QDeclarativeTransition*>();
    QVERIFY(trans);

    QCOMPARE(trans->enabled(), false);

    QQuickItemPrivate::get(rect)->setState("moved");
    QCOMPARE(myRect->x(),qreal(200));

    trans->setEnabled(true);

    QQuickItemPrivate::get(rect)->setState("");
    QCOMPARE(myRect->x(),qreal(200));
    QTest::qWait(300);
    QTIMED_COMPARE(myRect->x(),qreal(100));
}

void tst_qdeclarativeanimations::invalidDuration()
{
    QDeclarativePropertyAnimation *animation = new QDeclarativePropertyAnimation;
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML PropertyAnimation: Cannot set a duration of < 0");
    animation->setDuration(-1);
    QCOMPARE(animation->duration(), 250);

    QDeclarativePauseAnimation *pauseAnimation = new QDeclarativePauseAnimation;
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML PauseAnimation: Cannot set a duration of < 0");
    pauseAnimation->setDuration(-1);
    QCOMPARE(pauseAnimation->duration(), 250);
}

void tst_qdeclarativeanimations::attached()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, testFileUrl("attached.qml"));
    QTest::ignoreMessage(QtDebugMsg, "off");
    QTest::ignoreMessage(QtDebugMsg, "on");
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect);
}

void tst_qdeclarativeanimations::propertyValueSourceDefaultStart()
{
    {
        QDeclarativeEngine engine;

        QDeclarativeComponent c(&engine, testFileUrl("valuesource.qml"));

        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QDeclarativeAbstractAnimation *myAnim = rect->findChild<QDeclarativeAbstractAnimation*>("MyAnim");
        QVERIFY(myAnim);
        QVERIFY(myAnim->isRunning());
    }

    {
        QDeclarativeEngine engine;

        QDeclarativeComponent c(&engine, testFileUrl("valuesource2.qml"));

        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QDeclarativeAbstractAnimation *myAnim = rect->findChild<QDeclarativeAbstractAnimation*>("MyAnim");
        QVERIFY(myAnim);
        QVERIFY(myAnim->isRunning() == false);
    }

    {
        QDeclarativeEngine engine;

        QDeclarativeComponent c(&engine, testFileUrl("dontAutoStart.qml"));

        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QDeclarativeAbstractAnimation *myAnim = rect->findChild<QDeclarativeAbstractAnimation*>("MyAnim");
        QVERIFY(myAnim && myAnim->qtAnimation());
        QVERIFY(myAnim->qtAnimation()->state() == QAbstractAnimation::Stopped);
    }
}


void tst_qdeclarativeanimations::dontStart()
{
    {
        QDeclarativeEngine engine;

        QDeclarativeComponent c(&engine, testFileUrl("dontStart.qml"));

        QString warning = c.url().toString() + ":14:13: QML NumberAnimation: setRunning() cannot be used on non-root animation nodes.";
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QDeclarativeAbstractAnimation *myAnim = rect->findChild<QDeclarativeAbstractAnimation*>("MyAnim");
        QVERIFY(myAnim && myAnim->qtAnimation());
        QVERIFY(myAnim->qtAnimation()->state() == QAbstractAnimation::Stopped);
    }

    {
        QDeclarativeEngine engine;

        QDeclarativeComponent c(&engine, testFileUrl("dontStart2.qml"));

        QString warning = c.url().toString() + ":15:17: QML NumberAnimation: setRunning() cannot be used on non-root animation nodes.";
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
        QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
        QVERIFY(rect);

        QDeclarativeAbstractAnimation *myAnim = rect->findChild<QDeclarativeAbstractAnimation*>("MyAnim");
        QVERIFY(myAnim && myAnim->qtAnimation());
        QVERIFY(myAnim->qtAnimation()->state() == QAbstractAnimation::Stopped);
    }
}

void tst_qdeclarativeanimations::easingProperties()
{
    {
        QDeclarativeEngine engine;
        QString componentStr = "import QtQuick 2.0\nNumberAnimation { easing.type: \"InOutQuad\" }";
        QDeclarativeComponent animationComponent(&engine);
        animationComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QDeclarativePropertyAnimation *animObject = qobject_cast<QDeclarativePropertyAnimation*>(animationComponent.create());

        QVERIFY(animObject != 0);
        QCOMPARE(animObject->easing().type(), QEasingCurve::InOutQuad);
    }

    {
        QDeclarativeEngine engine;
        QString componentStr = "import QtQuick 2.0\nPropertyAnimation { easing.type: \"OutBounce\"; easing.amplitude: 5.0 }";
        QDeclarativeComponent animationComponent(&engine);
        animationComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QDeclarativePropertyAnimation *animObject = qobject_cast<QDeclarativePropertyAnimation*>(animationComponent.create());

        QVERIFY(animObject != 0);
        QCOMPARE(animObject->easing().type(), QEasingCurve::OutBounce);
        QCOMPARE(animObject->easing().amplitude(), 5.0);
    }

    {
        QDeclarativeEngine engine;
        QString componentStr = "import QtQuick 2.0\nPropertyAnimation { easing.type: \"OutElastic\"; easing.amplitude: 5.0; easing.period: 3.0}";
        QDeclarativeComponent animationComponent(&engine);
        animationComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QDeclarativePropertyAnimation *animObject = qobject_cast<QDeclarativePropertyAnimation*>(animationComponent.create());

        QVERIFY(animObject != 0);
        QCOMPARE(animObject->easing().type(), QEasingCurve::OutElastic);
        QCOMPARE(animObject->easing().amplitude(), 5.0);
        QCOMPARE(animObject->easing().period(), 3.0);
    }

    {
        QDeclarativeEngine engine;
        QString componentStr = "import QtQuick 2.0\nPropertyAnimation { easing.type: \"InOutBack\"; easing.overshoot: 2 }";
        QDeclarativeComponent animationComponent(&engine);
        animationComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QDeclarativePropertyAnimation *animObject = qobject_cast<QDeclarativePropertyAnimation*>(animationComponent.create());

        QVERIFY(animObject != 0);
        QCOMPARE(animObject->easing().type(), QEasingCurve::InOutBack);
        QCOMPARE(animObject->easing().overshoot(), 2.0);
    }

    {
        QDeclarativeEngine engine;
        QString componentStr = "import QtQuick 2.0\nPropertyAnimation { easing.type: \"Bezier\"; easing.bezierCurve: [0.5, 0.2, 0.13, 0.65, 1.0, 1.0] }";
        QDeclarativeComponent animationComponent(&engine);
        animationComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QDeclarativePropertyAnimation *animObject = qobject_cast<QDeclarativePropertyAnimation*>(animationComponent.create());

        QVERIFY(animObject != 0);
        QCOMPARE(animObject->easing().type(), QEasingCurve::BezierSpline);
        QList<QPointF> points = animObject->easing().cubicBezierSpline();
        QCOMPARE(points.count(), 3);
        QCOMPARE(points.at(0), QPointF(0.5, 0.2));
        QCOMPARE(points.at(1), QPointF(0.13, 0.65));
        QCOMPARE(points.at(2), QPointF(1.0, 1.0));
    }
}

void tst_qdeclarativeanimations::rotation()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("rotation.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
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

void tst_qdeclarativeanimations::runningTrueBug()
{
    //ensure we start correctly when "running: true" is explicitly set
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("runningTrueBug.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect);

    QQuickRectangle *cloud = rect->findChild<QQuickRectangle*>("cloud");
    QVERIFY(cloud);
    QTest::qWait(1000);
    QVERIFY(cloud->x() > qreal(0));
}

//QTBUG-12805
void tst_qdeclarativeanimations::nonTransitionBug()
{
    //tests that the animation values from the previous transition are properly cleared
    //in the case where an animation in the transition doesn't match anything (but previously did)
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, testFileUrl("nonTransitionBug.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect != 0);
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
void tst_qdeclarativeanimations::registrationBug()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, testFileUrl("registrationBug.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect != 0);
    QTRY_COMPARE(rect->property("value"), QVariant(int(100)));
}

void tst_qdeclarativeanimations::doubleRegistrationBug()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, testFileUrl("doubleRegistrationBug.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect != 0);

    QDeclarativeAbstractAnimation *anim = rect->findChild<QDeclarativeAbstractAnimation*>("animation");
    QVERIFY(anim != 0);
    QTRY_COMPARE(anim->qtAnimation()->state(), QAbstractAnimation::Stopped);
}

//QTBUG-16736
void tst_qdeclarativeanimations::alwaysRunToEndRestartBug()
{
    QQuickRectangle rect;
    QDeclarativePropertyAnimation animation;
    animation.setTarget(&rect);
    animation.setProperty("x");
    animation.setTo(200);
    animation.setDuration(1000);
    animation.setLoops(-1);
    animation.setAlwaysRunToEnd(true);
    QVERIFY(animation.loops() == -1);
    QVERIFY(animation.alwaysRunToEnd() == true);
    animation.start();
    animation.stop();
    animation.start();
    animation.stop();
    QTest::qWait(500);
    QVERIFY(rect.x() != qreal(200));
    QTest::qWait(800);
    QTIMED_COMPARE(rect.x(), qreal(200));
    QCOMPARE(static_cast<QDeclarativeAbstractAnimation*>(&animation)->qtAnimation()->state(), QAbstractAnimation::Stopped);
}

//QTBUG-20227
void tst_qdeclarativeanimations::transitionAssignmentBug()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, testFileUrl("transitionAssignmentBug.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect != 0);

    QCOMPARE(rect->property("nullObject").toBool(), false);
}

//QTBUG-19080
void tst_qdeclarativeanimations::pauseBindingBug()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, testFileUrl("pauseBindingBug.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect != 0);
    QDeclarativeAbstractAnimation *anim = rect->findChild<QDeclarativeAbstractAnimation*>("animation");
    QVERIFY(anim->qtAnimation()->state() == QAbstractAnimation::Paused);

    delete rect;
}

//QTBUG-13598
void tst_qdeclarativeanimations::pauseBug()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, testFileUrl("pauseBug.qml"));
    QDeclarativeAbstractAnimation *anim = qobject_cast<QDeclarativeAbstractAnimation*>(c.create());
    QVERIFY(anim != 0);
    QCOMPARE(anim->qtAnimation()->state(), QAbstractAnimation::Paused);
    QCOMPARE(anim->isPaused(), true);
    QCOMPARE(anim->isRunning(), true);

    delete anim;
}

QTEST_MAIN(tst_qdeclarativeanimations)

#include "tst_qdeclarativeanimations.moc"
