/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <qsignalspy.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtQuick1/qdeclarativeview.h>
#include <QtQuick1/private/qdeclarativerectangle_p.h>
#include <QtQuick1/private/qdeclarativetext_p.h>
#include <QtQuick1/private/qdeclarativebehavior_p.h>
#include <QtQuick1/private/qdeclarativeanimation_p.h>
#include <QtQuick1/private/qdeclarativeitem_p.h>
#include "../../../shared/util.h"

class tst_qdeclarativebehaviors : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativebehaviors() {}

private slots:
    void simpleBehavior();
    void scriptTriggered();
    void cppTriggered();
    void loop();
    void colorBehavior();
    void parentBehavior();
    void replaceBinding();
    //void transitionOverrides();
    void group();
    void emptyBehavior();
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
};

void tst_qdeclarativebehaviors::simpleBehavior()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/simple.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QTRY_VERIFY(rect);
    QTRY_VERIFY(qobject_cast<QDeclarative1Behavior*>(rect->findChild<QDeclarative1Behavior*>("MyBehavior"))->animation());

    QDeclarativeItemPrivate::get(rect)->setState("moved");
    QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x() > 0);
    QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x() < 200);
    //i.e. the behavior has been triggered

    delete rect;
}

void tst_qdeclarativebehaviors::scriptTriggered()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/scripttrigger.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QTRY_VERIFY(rect);

    rect->setColor(QColor("red"));
    QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x() > 0);
    QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x() < 200);
    //i.e. the behavior has been triggered

    delete rect;
}

void tst_qdeclarativebehaviors::cppTriggered()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/cpptrigger.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QTRY_VERIFY(rect);

    QDeclarative1Rectangle *innerRect = qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"));
    QTRY_VERIFY(innerRect);

    innerRect->setProperty("x", 200);
    QTRY_VERIFY(innerRect->x() > 0);
    QTRY_VERIFY(innerRect->x() < 200);  //i.e. the behavior has been triggered

    delete rect;
}

void tst_qdeclarativebehaviors::loop()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/loop.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QTRY_VERIFY(rect);

    //don't crash
    QDeclarativeItemPrivate::get(rect)->setState("moved");

    delete rect;
}

void tst_qdeclarativebehaviors::colorBehavior()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/color.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QTRY_VERIFY(rect);

    QDeclarativeItemPrivate::get(rect)->setState("red");
    QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->color() != QColor("red"));
    QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->color() != QColor("green"));
    //i.e. the behavior has been triggered

    delete rect;
}

void tst_qdeclarativebehaviors::parentBehavior()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/parent.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QTRY_VERIFY(rect);

    QDeclarativeItemPrivate::get(rect)->setState("reparented");
    QTRY_VERIFY(rect->findChild<QDeclarative1Rectangle*>("MyRect")->parentItem() != rect->findChild<QDeclarativeItem*>("NewParent"));
    QTRY_VERIFY(rect->findChild<QDeclarative1Rectangle*>("MyRect")->parentItem() == rect->findChild<QDeclarativeItem*>("NewParent"));

    delete rect;
}

void tst_qdeclarativebehaviors::replaceBinding()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/binding.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QTRY_VERIFY(rect);

    QDeclarativeItemPrivate::get(rect)->setState("moved");
    QDeclarative1Rectangle *innerRect = qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"));
    QTRY_VERIFY(innerRect);
    QTRY_VERIFY(innerRect->x() > 0);
    QTRY_VERIFY(innerRect->x() < 200);
    //i.e. the behavior has been triggered
    QTRY_COMPARE(innerRect->x(), (qreal)200);
    rect->setProperty("basex", 10);
    QTRY_COMPARE(innerRect->x(), (qreal)200);
    rect->setProperty("movedx", 210);
    QTRY_COMPARE(innerRect->x(), (qreal)210);

    QDeclarativeItemPrivate::get(rect)->setState("");
    QTRY_VERIFY(innerRect->x() > 10);
    QTRY_VERIFY(innerRect->x() < 210);  //i.e. the behavior has been triggered
    QTRY_COMPARE(innerRect->x(), (qreal)10);
    rect->setProperty("movedx", 200);
    QTRY_COMPARE(innerRect->x(), (qreal)10);
    rect->setProperty("basex", 20);
    QTRY_COMPARE(innerRect->x(), (qreal)20);

    delete rect;
}

void tst_qdeclarativebehaviors::group()
{
    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/groupProperty.qml"));
        QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
        QTRY_VERIFY(rect);

        QDeclarativeItemPrivate::get(rect)->setState("moved");
        //QTest::qWait(200);
        QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x() > 0);
        QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x() < 200);
        //i.e. the behavior has been triggered

        delete rect;
    }

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/groupProperty2.qml"));
        QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
        QTRY_VERIFY(rect);

        QDeclarativeItemPrivate::get(rect)->setState("moved");
        QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x() > 0);
        QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x() < 200);
        //i.e. the behavior has been triggered

        delete rect;
    }
}

void tst_qdeclarativebehaviors::emptyBehavior()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/empty.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QVERIFY(rect);

    QDeclarativeItemPrivate::get(rect)->setState("moved");
    qreal x = qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x();
    QCOMPARE(x, qreal(200));    //should change immediately

    delete rect;
}

void tst_qdeclarativebehaviors::explicitSelection()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/explicit.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QVERIFY(rect);

    QDeclarativeItemPrivate::get(rect)->setState("moved");
    QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x() > 0);
    QTRY_VERIFY(qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x() < 200);
    //i.e. the behavior has been triggered

    delete rect;
}

void tst_qdeclarativebehaviors::nonSelectingBehavior()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/nonSelecting2.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QVERIFY(rect);

    QDeclarativeItemPrivate::get(rect)->setState("moved");
    qreal x = qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x();
    QCOMPARE(x, qreal(200));    //should change immediately

    delete rect;
}

void tst_qdeclarativebehaviors::reassignedAnimation()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/reassignedAnimation.qml"));
    QString warning = QUrl::fromLocalFile(SRCDIR "/data/reassignedAnimation.qml").toString() + ":9:9: QML Behavior: Cannot change the animation assigned to a Behavior.";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QVERIFY(rect);
    QCOMPARE(qobject_cast<QDeclarative1NumberAnimation*>(
                 rect->findChild<QDeclarative1Behavior*>("MyBehavior")->animation())->duration(), 200);

    delete rect;
}

void tst_qdeclarativebehaviors::disabled()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/disabled.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QVERIFY(rect);
    QCOMPARE(rect->findChild<QDeclarative1Behavior*>("MyBehavior")->enabled(), false);

    QDeclarativeItemPrivate::get(rect)->setState("moved");
    qreal x = qobject_cast<QDeclarative1Rectangle*>(rect->findChild<QDeclarative1Rectangle*>("MyRect"))->x();
    QCOMPARE(x, qreal(200));    //should change immediately

    delete rect;
}

void tst_qdeclarativebehaviors::dontStart()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/dontStart.qml"));

    QString warning = c.url().toString() + ":13:13: QML NumberAnimation: setRunning() cannot be used on non-root animation nodes.";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QVERIFY(rect);

    QDeclarative1AbstractAnimation *myAnim = rect->findChild<QDeclarative1AbstractAnimation*>("MyAnim");
    QVERIFY(myAnim && myAnim->qtAnimation());
    QVERIFY(myAnim->qtAnimation()->state() == QAbstractAnimation::Stopped);

    delete rect;
}

void tst_qdeclarativebehaviors::startup()
{
    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/startup.qml"));
        QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
        QVERIFY(rect);

        QDeclarative1Rectangle *innerRect = rect->findChild<QDeclarative1Rectangle*>("innerRect");
        QVERIFY(innerRect);

        QCOMPARE(innerRect->x(), qreal(100));    //should be set immediately

        delete rect;
    }

    {
        QDeclarativeEngine engine;
        QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/startup2.qml"));
        QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
        QVERIFY(rect);

        QDeclarative1Rectangle *innerRect = rect->findChild<QDeclarative1Rectangle*>("innerRect");
        QVERIFY(innerRect);

        QDeclarative1Text *text = rect->findChild<QDeclarative1Text*>();
        QVERIFY(text);

        QCOMPARE(innerRect->x(), text->width());    //should be set immediately

        delete rect;
    }
}

//QTBUG-10799
void tst_qdeclarativebehaviors::groupedPropertyCrash()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/groupedPropertyCrash.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QVERIFY(rect);  //don't crash
}

//QTBUG-5491
void tst_qdeclarativebehaviors::runningTrue()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/runningTrue.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QVERIFY(rect);

    QDeclarative1AbstractAnimation *animation = rect->findChild<QDeclarative1AbstractAnimation*>("rotAnim");
    QVERIFY(animation);

    QSignalSpy runningSpy(animation, SIGNAL(runningChanged(bool)));
    rect->setProperty("myValue", 180);
    QTRY_VERIFY(runningSpy.count() > 0);
}

//QTBUG-12295
void tst_qdeclarativebehaviors::sameValue()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/qtbug12295.qml"));
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QVERIFY(rect);

    QDeclarative1Rectangle *target = rect->findChild<QDeclarative1Rectangle*>("myRect");
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
void tst_qdeclarativebehaviors::delayedRegistration()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, SRCDIR "/data/delayedRegistration.qml");
    QDeclarative1Rectangle *rect = qobject_cast<QDeclarative1Rectangle*>(c.create());
    QVERIFY(rect != 0);

    QDeclarativeItem *innerRect = rect->property("myItem").value<QDeclarativeItem*>();
    QVERIFY(innerRect != 0);

    QCOMPARE(innerRect->property("x").toInt(), int(0));

    QTRY_COMPARE(innerRect->property("x").toInt(), int(100));
}

QTEST_MAIN(tst_qdeclarativebehaviors)

#include "tst_qdeclarativebehaviors.moc"
