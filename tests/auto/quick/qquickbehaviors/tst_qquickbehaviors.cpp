// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtTest/QtTest>
#include <qsignalspy.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickbehavior_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquicksmoothedanimation_p.h>
#include <private/qquickitem_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include "bindable.h"

class tst_qquickbehaviors : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickbehaviors() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void init() override;
    void simpleBehavior();
    void scriptTriggered();
    void cppTriggered();
    void loop();
    void colorBehavior();
    void parentBehavior();
    void replaceBinding();
    //void transitionOverrides();
    void group();
    void valueType();
    void emptyBehavior();
    void duplicatedBehavior();
    void explicitSelection();
    void nonSelectingBehavior();
    void reassignedAnimation();
    void disabled();
    void dontStart();
    void startup();
    void groupedPropertyCrash();
    void runningTrue();
    void sameValue();
    void delayedRegistration();
    void startOnCompleted();
    void multipleChangesToValueType();
    void currentValue();
    void disabledWriteWhileRunning();
    void aliasedProperty();
    void innerBehaviorOverwritten();
    void oneWay();
    void safeToDelete();
    void targetProperty();
    void bindableProperty();
    void defaultQProperty();
};

void tst_qquickbehaviors::init()
{
    QQmlDataTest::init();
    //work around animation timer bug (QTBUG-22865)
    qApp->processEvents();
}

void tst_qquickbehaviors::simpleBehavior()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("simple.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));
    QTRY_VERIFY(qobject_cast<QQuickBehavior*>(rect->findChild<QQuickBehavior*>("MyBehavior"))->animation());

    QQuickItemPrivate::get(rect.data())->setState("moved");
    QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->x() > 0);
    QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->x() < 200);
    //i.e. the behavior has been triggered
}

void tst_qquickbehaviors::scriptTriggered()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("scripttrigger.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    rect->setColor(QColor("red"));
    QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->x() > 0);
    QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->x() < 200);
    //i.e. the behavior has been triggered
}

void tst_qquickbehaviors::cppTriggered()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("cpptrigger.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
    QTRY_VERIFY(innerRect);

    innerRect->setProperty("x", 200);
    QTRY_VERIFY(innerRect->x() > 0);
    QTRY_VERIFY(innerRect->x() < 200);  //i.e. the behavior has been triggered
}

void tst_qquickbehaviors::loop()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("loop.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    //don't crash
    QQuickItemPrivate::get(rect.data())->setState("moved");
}

void tst_qquickbehaviors::colorBehavior()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("color.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickItemPrivate::get(rect.data())->setState("red");
    QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->color() != QColor("red"));
    QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->color() != QColor("green"));
    //i.e. the behavior has been triggered
}

void tst_qquickbehaviors::parentBehavior()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("parent.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickItemPrivate::get(rect.data())->setState("reparented");
    QTRY_VERIFY(rect->findChild<QQuickRectangle*>("MyRect")->parentItem() != rect->findChild<QQuickItem*>("NewParent"));
    QTRY_VERIFY(rect->findChild<QQuickRectangle*>("MyRect")->parentItem() == rect->findChild<QQuickItem*>("NewParent"));
}

void tst_qquickbehaviors::replaceBinding()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("binding.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickItemPrivate::get(rect.data())->setState("moved");
    QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
    QTRY_VERIFY(innerRect);
    QTRY_VERIFY(innerRect->x() > 0);
    QTRY_VERIFY(innerRect->x() < 200);
    //i.e. the behavior has been triggered
    QTRY_COMPARE(innerRect->x(), (qreal)200);
    rect->setProperty("basex", 10);
    QTRY_COMPARE(innerRect->x(), (qreal)200);
    rect->setProperty("movedx", 210);
    QTRY_COMPARE(innerRect->x(), (qreal)210);

    QQuickItemPrivate::get(rect.data())->setState("");
    QTRY_VERIFY(innerRect->x() > 10);
    QTRY_VERIFY(innerRect->x() < 210);  //i.e. the behavior has been triggered
    QTRY_COMPARE(innerRect->x(), (qreal)10);
    rect->setProperty("movedx", 200);
    QTRY_COMPARE(innerRect->x(), (qreal)10);
    rect->setProperty("basex", 20);
    QTRY_COMPARE(innerRect->x(), (qreal)20);
}

void tst_qquickbehaviors::group()
{
    /* XXX TODO Create a test element for this case.
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("groupProperty.qml")));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
        QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

        QQuickItemPrivate::get(rect.data())->setState("moved");
        //QTest::qWait(200);
        QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->x() > 0);
        QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->x() < 200);
        //i.e. the behavior has been triggered
    }
    */

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("groupProperty2.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
        QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

        QQuickItemPrivate::get(rect.data())->setState("moved");
        QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->border()->width() > 0);
        QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->border()->width() < 4);
        //i.e. the behavior has been triggered
    }
}

void tst_qquickbehaviors::valueType()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("valueType.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    //QTBUG-20827
    QCOMPARE(rect->color(), QColor::fromRgb(255,0,255));
}

void tst_qquickbehaviors::emptyBehavior()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("empty.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickItemPrivate::get(rect.data())->setState("moved");
    qreal x = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->x();
    QCOMPARE(x, qreal(200));    //should change immediately
}

void tst_qquickbehaviors::duplicatedBehavior()
{
    QTest::failOnWarning(QRegularExpression(".*"));
    QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                         QRegularExpression("Attempting to set another interceptor on.*"));
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("duplicated.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle *>(c.create()));
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    // Expecting no crash
    QQuickItemPrivate::get(rect.data())->setState("moved");
}

void tst_qquickbehaviors::explicitSelection()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("explicit.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickItemPrivate::get(rect.data())->setState("moved");
    QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->x() > 0);
    QTRY_VERIFY(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->x() < 200);
    //i.e. the behavior has been triggered
}

void tst_qquickbehaviors::nonSelectingBehavior()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("nonSelecting2.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickItemPrivate::get(rect.data())->setState("moved");
    qreal x = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->x();
    QCOMPARE(x, qreal(200));    //should change immediately
}

void tst_qquickbehaviors::reassignedAnimation()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("reassignedAnimation.qml"));
    QString warning = testFileUrl("reassignedAnimation.qml").toString() + ":9:9: QML Behavior: Cannot change the animation assigned to a Behavior.";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));
    QCOMPARE(qobject_cast<QQuickNumberAnimation*>(
                 rect->findChild<QQuickBehavior*>("MyBehavior")->animation())->duration(), 200);
}

void tst_qquickbehaviors::disabled()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("disabled.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));
    QCOMPARE(rect->findChild<QQuickBehavior*>("MyBehavior")->enabled(), false);

    QQuickItemPrivate::get(rect.data())->setState("moved");
    qreal x = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"))->x();
    QCOMPARE(x, qreal(200));    //should change immediately
}

void tst_qquickbehaviors::dontStart()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("dontStart.qml"));

    QString warning = c.url().toString() + ":13:13: QML NumberAnimation: setRunning() cannot be used on non-root animation nodes.";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickAbstractAnimation *myAnim = rect->findChild<QQuickAbstractAnimation*>("MyAnim");
    QVERIFY(myAnim);
    QVERIFY(!myAnim->qtAnimation());
}

void tst_qquickbehaviors::startup()
{
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("startup.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
        QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

        QQuickRectangle *innerRect = rect->findChild<QQuickRectangle*>("innerRect");
        QVERIFY(innerRect);

        QCOMPARE(innerRect->x(), qreal(100));    //should be set immediately
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("startup2.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
        QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

        QQuickRectangle *innerRect = rect->findChild<QQuickRectangle*>("innerRect");
        QVERIFY(innerRect);

        QQuickText *text = rect->findChild<QQuickText*>();
        QVERIFY(text);

        QCOMPARE(innerRect->x(), text->width());    //should be set immediately
    }
}

//QTBUG-10799
void tst_qquickbehaviors::groupedPropertyCrash()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("groupedPropertyCrash.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));  //don't crash
}

//QTBUG-5491
void tst_qquickbehaviors::runningTrue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("runningTrue.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickAbstractAnimation *animation = rect->findChild<QQuickAbstractAnimation*>("rotAnim");
    QVERIFY(animation);

    QSignalSpy runningSpy(animation, SIGNAL(runningChanged(bool)));
    rect->setProperty("myValue", 180);
    QTRY_VERIFY(runningSpy.size() > 0);
}

//QTBUG-12295
void tst_qquickbehaviors::sameValue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("qtbug12295.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickRectangle *target = rect->findChild<QQuickRectangle*>("myRect");
    QVERIFY(target);

    target->setX(100);
    QCOMPARE(target->x(), qreal(100));

    target->setProperty("x", 0);
    QTRY_VERIFY(target->x() != qreal(0) && target->x() != qreal(100));
    QTRY_VERIFY(target->x() == qreal(0));   //make sure Behavior has finished.

    target->setX(100);
    QCOMPARE(target->x(), qreal(100));

    //this is the main point of the test -- the behavior needs to be triggered again
    //even though we set 0 twice in a row.
    target->setProperty("x", 0);
    QTRY_VERIFY(target->x() != qreal(0) && target->x() != qreal(100));
}

//QTBUG-18362
void tst_qquickbehaviors::delayedRegistration()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("delayedRegistration.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickItem *innerRect = rect->property("myItem").value<QQuickItem*>();
    QVERIFY(innerRect != nullptr);

    QCOMPARE(innerRect->property("x").toInt(), int(0));

    QTRY_COMPARE(innerRect->property("x").toInt(), int(100));
}

//QTBUG-22555
void tst_qquickbehaviors::startOnCompleted()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("startOnCompleted.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickItem *innerRect = rect->findChild<QQuickRectangle*>();
    QVERIFY(innerRect != nullptr);

    QCOMPARE(innerRect->property("x").toInt(), int(0));

    QTRY_COMPARE(innerRect->property("x").toInt(), int(100));
}

//QTBUG-25139
void tst_qquickbehaviors::multipleChangesToValueType()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("multipleChangesToValueType.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle *>(c.create()));
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickText *text = rect->findChild<QQuickText *>();
    QVERIFY(text != nullptr);

    QFont value;
    value.setPointSize(24);
    QCOMPARE(text->property("font").value<QFont>(), value);

    QVERIFY(QMetaObject::invokeMethod(rect.data(), "updateFontProperties"));

    value.setItalic(true);
    value.setWeight(QFont::Bold);
    QCOMPARE(text->property("font").value<QFont>(), value);

    value.setPointSize(48);
    QTRY_COMPARE(text->property("font").value<QFont>(), value);
}

//QTBUG-21549
void tst_qquickbehaviors::currentValue()
{
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("qtbug21549.qml"));
        QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem*>(c.create()));
        QVERIFY2(!item.isNull(), qPrintable(c.errorString()));

        QQuickRectangle *target = item->findChild<QQuickRectangle*>("myRect");
        QVERIFY(target);

        QCOMPARE(target->x(), qreal(0));

        target->setProperty("x", 50);
        QCOMPARE(item->property("behaviorCount").toInt(), 1);
        QCOMPARE(target->x(), qreal(50));

        target->setProperty("x", 50);
        QCOMPARE(item->property("behaviorCount").toInt(), 1);
        QCOMPARE(target->x(), qreal(50));

        target->setX(100);
        target->setProperty("x", 100);
        QCOMPARE(item->property("behaviorCount").toInt(), 1);
        QCOMPARE(target->x(), qreal(100));
    }

    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("qtbug21549-2.qml"));
        QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem*>(c.create()));
        QVERIFY2(!item.isNull(), qPrintable(c.errorString()));

        QQuickRectangle *target = item->findChild<QQuickRectangle*>("myRect");
        QVERIFY(target);

        QCOMPARE(target->x(), qreal(0));

        target->setProperty("x", 100);

        // the spring animation should smoothly transition to the new value triggered
        // in the QML (which should be between 50 and 80);
        QTRY_COMPARE(item->property("animRunning").toBool(), true);
        QTRY_COMPARE(item->property("animRunning").toBool(), false);
        QVERIFY2(target->x() > qreal(50) && target->x() < qreal(80), QByteArray::number(target->x()));
    }
}

void tst_qquickbehaviors::disabledWriteWhileRunning()
{
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("disabledWriteWhileRunning.qml"));
        QScopedPointer<QQuickItem> root(qobject_cast<QQuickItem*>(c.create()));
        QVERIFY2(!root.isNull(), qPrintable(c.errorString()));

        QQuickRectangle *myRect = qobject_cast<QQuickRectangle*>(root->findChild<QQuickRectangle*>("MyRect"));
        QQuickBehavior *myBehavior = qobject_cast<QQuickBehavior*>(root->findChild<QQuickBehavior*>("MyBehavior"));
        QQuickNumberAnimation *myAnimation = qobject_cast<QQuickNumberAnimation*>(root->findChild<QQuickNumberAnimation*>("MyAnimation"));
        QVERIFY(myRect);
        QVERIFY(myBehavior);
        QVERIFY(myAnimation);

        // initial values
        QCOMPARE(myBehavior->enabled(), true);
        QCOMPARE(myAnimation->isRunning(), false);
        QCOMPARE(myRect->x(), qreal(0));

        // start animation
        myRect->setProperty("x", 200);
        QCOMPARE(myAnimation->isRunning(), true);
        QTRY_VERIFY(myRect->x() != qreal(0) && myRect->x() != qreal(200));
        QCOMPARE(myBehavior->targetValue(), 200); // grabbed before starting the animation

        // set disabled while animation is in progress
        myBehavior->setProperty("enabled", false);
        QCOMPARE(myAnimation->isRunning(), true);

        // force new value while disabled and previous animation is in progress.
        // animation should be stopped and value stay at forced value
        myRect->setProperty("x", 100);
        QCOMPARE(myAnimation->isRunning(), false);
        QCOMPARE(myRect->x(), qreal(100));
        QCOMPARE(myBehavior->targetValue(), 100);
        QTest::qWait(200);
        QCOMPARE(myRect->x(), qreal(100));
    }

    //test additional complications with SmoothedAnimation
    {
        QQmlEngine engine;
        QQmlComponent c(&engine, testFileUrl("disabledWriteWhileRunning2.qml"));
        QScopedPointer<QQuickItem> root(qobject_cast<QQuickItem*>(c.create()));
        QVERIFY2(!root.isNull(), qPrintable(c.errorString()));

        QQuickRectangle *myRect = qobject_cast<QQuickRectangle*>(root->findChild<QQuickRectangle*>("MyRect"));
        QQuickBehavior *myBehavior = qobject_cast<QQuickBehavior*>(root->findChild<QQuickBehavior*>("MyBehavior"));
        QQuickSmoothedAnimation *myAnimation = qobject_cast<QQuickSmoothedAnimation*>(root->findChild<QQuickNumberAnimation*>("MyAnimation"));
        QVERIFY(myRect);
        QVERIFY(myBehavior);
        QVERIFY(myAnimation);

        // initial values
        QCOMPARE(myBehavior->enabled(), true);
        QCOMPARE(myBehavior->targetValue(), QVariant());
        QCOMPARE(myAnimation->isRunning(), false);
        QCOMPARE(myRect->x(), qreal(0));

        // start animation
        myRect->setProperty("x", 200);
        QCOMPARE(myAnimation->isRunning(), true);
        QCOMPARE(myBehavior->targetValue(), 200);
        QTRY_VERIFY(myRect->x() != qreal(0) && myRect->x() != qreal(200));

        //set second value
        myRect->setProperty("x", 300);
        QCOMPARE(myAnimation->isRunning(), true);
        QCOMPARE(myBehavior->targetValue(), 300);
        QTRY_VERIFY(myRect->x() != qreal(0) && myRect->x() != qreal(200));

        // set disabled while animation is in progress
        myBehavior->setProperty("enabled", false);
        QCOMPARE(myAnimation->isRunning(), true);

        // force new value while disabled and previous animation is in progress.
        // animation should be stopped and value stay at forced value
        myRect->setProperty("x", 100);
        QCOMPARE(myAnimation->isRunning(), false);
        QCOMPARE(myRect->x(), qreal(100));
        QCOMPARE(myBehavior->targetValue(), 100);
        QTest::qWait(200);
        QCOMPARE(myRect->x(), qreal(100));
    }
}

void tst_qquickbehaviors::aliasedProperty()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("aliased.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));

    QQuickBehavior* behavior =
        qobject_cast<QQuickBehavior*>(rect->findChild<QQuickBehavior*>("behavior"));
    QSignalSpy targetValueSpy(behavior, SIGNAL(targetValueChanged()));
    QQuickItemPrivate::get(rect.data())->setState("moved");
    QCOMPARE(behavior->targetValue(), 400);
    QCOMPARE(targetValueSpy.size(), 1);
    QScopedPointer<QQuickRectangle> acc(qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("acc")));
    QScopedPointer<QQuickRectangle> range(qobject_cast<QQuickRectangle*>(acc->findChild<QQuickRectangle*>("range")));
    QTRY_VERIFY(acc->property("value").toDouble() > 0);
    QTRY_VERIFY(range->width() > 0);
    QTRY_VERIFY(acc->property("value").toDouble() < 400);
    QTRY_VERIFY(range->width() < 400);
    //i.e. the behavior has been triggered
}

void tst_qquickbehaviors::innerBehaviorOverwritten()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("overwrittenbehavior.qml"));
    QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem*>(c.create()));
    QQuickBehavior* behavior =
        qobject_cast<QQuickBehavior*>(item->findChild<QQuickBehavior*>("behavior"));
    QVERIFY(item->property("behaviorTriggered").toBool());
    QCOMPARE(behavior->targetValue(), true);
}

void tst_qquickbehaviors::oneWay()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("oneway.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));;
    QVERIFY2(!rect.isNull(), qPrintable(c.errorString()));
    QQuickBehavior* behavior =
        qobject_cast<QQuickBehavior*>(rect->findChild<QQuickBehavior*>("MyBehaviorOneWay"));
    QCOMPARE(behavior->enabled(), false);
    QCOMPARE(behavior->targetValue(), QVariant());

    QSignalSpy targetValueSpy(behavior, SIGNAL(targetValueChanged()));
    QQuickRectangle *myRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRectOneWay"));
    myRect->setProperty("x", 100);
    QCOMPARE(behavior->targetValue(), 100);
    QCOMPARE(targetValueSpy.size(), 1);
    QCOMPARE(behavior->enabled(), false);
    qreal x = myRect->x();
    QCOMPARE(x, qreal(100));    //should change immediately
    QQuickNumberAnimation *myAnimation =
        qobject_cast<QQuickNumberAnimation*>(behavior->findChild<QQuickNumberAnimation*>("MyAnimationOneWay"));
    QCOMPARE(myAnimation->isRunning(), false);

    myRect->setProperty("x", 0);
    QCOMPARE(behavior->targetValue(), 0);
    QCOMPARE(targetValueSpy.size(), 2);
    QCOMPARE(behavior->enabled(), true);
    QCOMPARE(myAnimation->isRunning(), true);
    QVERIFY(myRect->x() > 0.0);
    QTRY_VERIFY(myRect->x() < 100.0);
    QTRY_COMPARE(myRect->x(), qreal(0));
    QCOMPARE(myAnimation->isRunning(), false);
}

// QTBUG-76749
void tst_qquickbehaviors::safeToDelete()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("delete.qml"));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o.data());
}

Q_DECLARE_METATYPE(QQmlProperty)
void tst_qquickbehaviors::targetProperty()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("targetProperty.qml"));
    QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem*>(c.create()));
    QVERIFY2(!item.isNull(), qPrintable(c.errorString()));

    QQuickBehavior* xBehavior =
        qobject_cast<QQuickBehavior*>(item->findChild<QQuickBehavior*>("xBehavior"));
    QCOMPARE(xBehavior->property("targetProperty").value<QQmlProperty>(), QQmlProperty(item.data(), "x"));
    QCOMPARE(item->property("xBehaviorObject").value<QObject*>(), item.data());
    QCOMPARE(item->property("xBehaviorName").toString(), "x");

    QQuickBehavior* emptyBehavior =
        qobject_cast<QQuickBehavior*>(item->findChild<QQuickBehavior*>("emptyBehavior"));
    QCOMPARE(emptyBehavior->property("targetProperty").value<QQmlProperty>().isValid(), false);
    QCOMPARE(item->property("emptyBehaviorObject").value<QObject*>(), nullptr);
    QCOMPARE(item->property("emptyBehaviorName").toString(), "");
}

void tst_qquickbehaviors::bindableProperty()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("bindableProperty.qml"));
    QScopedPointer<QObject> root(c.create());
    QVERIFY2(root, qPrintable(c.errorString()));
    auto testBindable = qobject_cast<TestBindable *>(root.get());
    QVERIFY(testBindable);

    testBindable->setProperty("targetValue", 100);
    QVERIFY(testBindable->prop() != 100);
    QTRY_COMPARE(testBindable->prop(), 100);

    testBindable->setProperty("enableBehavior", false);
    testBindable->setProperty("targetValue", 200);
    QCOMPARE(testBindable->prop(), 200);

    testBindable->setProperty("enableBehavior", true);
    testBindable->setProperty("prop", 300); // write through metaobject system gets intercepted
    QVERIFY(testBindable->prop() != 300);
    QTRY_COMPARE(testBindable->prop(), 300);
}

void tst_qquickbehaviors::defaultQProperty()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("defaultQProperty.qml"));
    QScopedPointer<QObject> root(c.create());
    QVERIFY2(root, qPrintable(c.errorString()));

    QQuickItem *item = qobject_cast<QQuickItem *>(root.data());
    QVERIFY(item);

    QCOMPARE(item->height(), 0.0);
    QCOMPARE(item->width(), 10.0);

    // Both change only once: No intermediate change to 0 on width.
    QCOMPARE(root->property("heightChanges").toInt(), 1);
    QCOMPARE(root->property("widthChanges").toInt(), 1);
}


QTEST_MAIN(tst_qquickbehaviors)

#include "tst_qquickbehaviors.moc"
