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
#include <QtTest/QSignalSpy>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtQuick1/qdeclarativeview.h>
#include <private/qdeclarativeflickable_p.h>
#include <private/qdeclarativevaluetype_p.h>
#include <QtGui/qgraphicswidget.h>
#include <math.h>
#include "../../../shared/util.h"

#ifdef Q_OS_SYMBIAN
// In Symbian OS test data is located in applications private dir
#define SRCDIR "."
#endif

class tst_qdeclarativeflickable : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativeflickable();

private slots:
    void create();
    void horizontalViewportSize();
    void verticalViewportSize();
    void properties();
    void boundsBehavior();
    void maximumFlickVelocity();
    void flickDeceleration();
    void pressDelay();
    void disabledContent();
    void nestedPressDelay();
    void flickableDirection();
    void qgraphicswidget();
    void resizeContent();
    void returnToBounds();
    void testQtQuick11Attributes();
    void testQtQuick11Attributes_data();
    void wheel();
    void disabled();

private:
    QDeclarativeEngine engine;

    template<typename T>
    T *findItem(QGraphicsObject *parent, const QString &objectName);
};

tst_qdeclarativeflickable::tst_qdeclarativeflickable()
{
}

void tst_qdeclarativeflickable::create()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/flickable01.qml"));
    QDeclarative1Flickable *obj = qobject_cast<QDeclarative1Flickable*>(c.create());

    QVERIFY(obj != 0);
    QCOMPARE(obj->isAtXBeginning(), true);
    QCOMPARE(obj->isAtXEnd(), false);
    QCOMPARE(obj->isAtYBeginning(), true);
    QCOMPARE(obj->isAtYEnd(), false);
    QCOMPARE(obj->contentX(), 0.);
    QCOMPARE(obj->contentY(), 0.);

    QCOMPARE(obj->horizontalVelocity(), 0.);
    QCOMPARE(obj->verticalVelocity(), 0.);

    QCOMPARE(obj->isInteractive(), true);
    QCOMPARE(obj->boundsBehavior(), QDeclarative1Flickable::DragAndOvershootBounds);
    QCOMPARE(obj->pressDelay(), 0);
    QCOMPARE(obj->maximumFlickVelocity(), 2500.);

    delete obj;
}

void tst_qdeclarativeflickable::horizontalViewportSize()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/flickable02.qml"));
    QDeclarative1Flickable *obj = qobject_cast<QDeclarative1Flickable*>(c.create());

    QVERIFY(obj != 0);
    QCOMPARE(obj->contentWidth(), 800.);
    QCOMPARE(obj->contentHeight(), 300.);
    QCOMPARE(obj->isAtXBeginning(), true);
    QCOMPARE(obj->isAtXEnd(), false);
    QCOMPARE(obj->isAtYBeginning(), true);
    QCOMPARE(obj->isAtYEnd(), false);

    delete obj;
}

void tst_qdeclarativeflickable::verticalViewportSize()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/flickable03.qml"));
    QDeclarative1Flickable *obj = qobject_cast<QDeclarative1Flickable*>(c.create());

    QVERIFY(obj != 0);
    QCOMPARE(obj->contentWidth(), 200.);
    QCOMPARE(obj->contentHeight(), 1200.);
    QCOMPARE(obj->isAtXBeginning(), true);
    QCOMPARE(obj->isAtXEnd(), false);
    QCOMPARE(obj->isAtYBeginning(), true);
    QCOMPARE(obj->isAtYEnd(), false);

    delete obj;
}

void tst_qdeclarativeflickable::properties()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/flickable04.qml"));
    QDeclarative1Flickable *obj = qobject_cast<QDeclarative1Flickable*>(c.create());

    QVERIFY(obj != 0);
    QCOMPARE(obj->isInteractive(), false);
    QCOMPARE(obj->boundsBehavior(), QDeclarative1Flickable::StopAtBounds);
    QCOMPARE(obj->pressDelay(), 200);
    QCOMPARE(obj->maximumFlickVelocity(), 2000.);

    QVERIFY(obj->property("ok").toBool() == false);
    QMetaObject::invokeMethod(obj, "check");
    QVERIFY(obj->property("ok").toBool() == true);

    delete obj;
}

void tst_qdeclarativeflickable::boundsBehavior()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 1.0; Flickable { boundsBehavior: Flickable.StopAtBounds }", QUrl::fromLocalFile(""));
    QDeclarative1Flickable *flickable = qobject_cast<QDeclarative1Flickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(boundsBehaviorChanged()));

    QVERIFY(flickable);
    QVERIFY(flickable->boundsBehavior() == QDeclarative1Flickable::StopAtBounds);

    flickable->setBoundsBehavior(QDeclarative1Flickable::DragAndOvershootBounds);
    QVERIFY(flickable->boundsBehavior() == QDeclarative1Flickable::DragAndOvershootBounds);
    QCOMPARE(spy.count(),1);
    flickable->setBoundsBehavior(QDeclarative1Flickable::DragAndOvershootBounds);
    QCOMPARE(spy.count(),1);

    flickable->setBoundsBehavior(QDeclarative1Flickable::DragOverBounds);
    QVERIFY(flickable->boundsBehavior() == QDeclarative1Flickable::DragOverBounds);
    QCOMPARE(spy.count(),2);
    flickable->setBoundsBehavior(QDeclarative1Flickable::DragOverBounds);
    QCOMPARE(spy.count(),2);

    flickable->setBoundsBehavior(QDeclarative1Flickable::StopAtBounds);
    QVERIFY(flickable->boundsBehavior() == QDeclarative1Flickable::StopAtBounds);
    QCOMPARE(spy.count(),3);
    flickable->setBoundsBehavior(QDeclarative1Flickable::StopAtBounds);
    QCOMPARE(spy.count(),3);
}

void tst_qdeclarativeflickable::maximumFlickVelocity()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 1.0; Flickable { maximumFlickVelocity: 1.0; }", QUrl::fromLocalFile(""));
    QDeclarative1Flickable *flickable = qobject_cast<QDeclarative1Flickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(maximumFlickVelocityChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->maximumFlickVelocity(), 1.0);

    flickable->setMaximumFlickVelocity(2.0);
    QCOMPARE(flickable->maximumFlickVelocity(), 2.0);
    QCOMPARE(spy.count(),1);
    flickable->setMaximumFlickVelocity(2.0);
    QCOMPARE(spy.count(),1);
}

void tst_qdeclarativeflickable::flickDeceleration()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 1.0; Flickable { flickDeceleration: 1.0; }", QUrl::fromLocalFile(""));
    QDeclarative1Flickable *flickable = qobject_cast<QDeclarative1Flickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(flickDecelerationChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->flickDeceleration(), 1.0);

    flickable->setFlickDeceleration(2.0);
    QCOMPARE(flickable->flickDeceleration(), 2.0);
    QCOMPARE(spy.count(),1);
    flickable->setFlickDeceleration(2.0);
    QCOMPARE(spy.count(),1);
}

void tst_qdeclarativeflickable::pressDelay()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 1.0; Flickable { pressDelay: 100; }", QUrl::fromLocalFile(""));
    QDeclarative1Flickable *flickable = qobject_cast<QDeclarative1Flickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(pressDelayChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->pressDelay(), 100);

    flickable->setPressDelay(200);
    QCOMPARE(flickable->pressDelay(), 200);
    QCOMPARE(spy.count(),1);
    flickable->setPressDelay(200);
    QCOMPARE(spy.count(),1);
}

// QT-4677
void tst_qdeclarativeflickable::disabledContent()
{
    QDeclarativeView *canvas = new QDeclarativeView;
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/disabledcontent.qml"));
    canvas->show();
    canvas->setFocus();
    QVERIFY(canvas->rootObject() != 0);

    QDeclarative1Flickable *flickable = qobject_cast<QDeclarative1Flickable*>(canvas->rootObject());
    QVERIFY(flickable != 0);

    QVERIFY(flickable->contentX() == 0);
    QVERIFY(flickable->contentY() == 0);

    QTest::mousePress(canvas->viewport(), Qt::LeftButton, 0, canvas->mapFromScene(QPoint(50, 50)));
    {
        QMouseEvent mv(QEvent::MouseMove, canvas->mapFromScene(QPoint(70,70)), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(canvas->viewport(), &mv);
    }
    {
        QMouseEvent mv(QEvent::MouseMove, canvas->mapFromScene(QPoint(90,90)), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(canvas->viewport(), &mv);
    }
    {
        QMouseEvent mv(QEvent::MouseMove, canvas->mapFromScene(QPoint(100,100)), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(canvas->viewport(), &mv);
    }

    QVERIFY(flickable->contentX() < 0);
    QVERIFY(flickable->contentY() < 0);

    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, 0, canvas->mapFromScene(QPoint(90, 90)));

    delete canvas;
}


// QTBUG-17361
void tst_qdeclarativeflickable::nestedPressDelay()
{
    QDeclarativeView *canvas = new QDeclarativeView;
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/nestedPressDelay.qml"));
    canvas->show();
    canvas->setFocus();
    QVERIFY(canvas->rootObject() != 0);

    QDeclarative1Flickable *outer = qobject_cast<QDeclarative1Flickable*>(canvas->rootObject());
    QVERIFY(outer != 0);

    QDeclarative1Flickable *inner = canvas->rootObject()->findChild<QDeclarative1Flickable*>("innerFlickable");
    QVERIFY(inner != 0);

    QTest::mousePress(canvas->viewport(), Qt::LeftButton, 0, canvas->mapFromScene(QPoint(150, 150)));
    // the MouseArea is not pressed immediately
    QVERIFY(outer->property("pressed").toBool() == false);

    // The outer pressDelay will prevail (50ms, vs. 10sec)
    // QTRY_VERIFY() has 5sec timeout, so will timeout well within 10sec.
    QTRY_VERIFY(outer->property("pressed").toBool() == true);

    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, 0, canvas->mapFromScene(QPoint(150, 150)));

    delete canvas;
}

void tst_qdeclarativeflickable::flickableDirection()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 1.0; Flickable { flickableDirection: Flickable.VerticalFlick; }", QUrl::fromLocalFile(""));
    QDeclarative1Flickable *flickable = qobject_cast<QDeclarative1Flickable*>(component.create());
    QSignalSpy spy(flickable, SIGNAL(flickableDirectionChanged()));

    QVERIFY(flickable);
    QCOMPARE(flickable->flickableDirection(), QDeclarative1Flickable::VerticalFlick);

    flickable->setFlickableDirection(QDeclarative1Flickable::HorizontalAndVerticalFlick);
    QCOMPARE(flickable->flickableDirection(), QDeclarative1Flickable::HorizontalAndVerticalFlick);
    QCOMPARE(spy.count(),1);

    flickable->setFlickableDirection(QDeclarative1Flickable::AutoFlickDirection);
    QCOMPARE(flickable->flickableDirection(), QDeclarative1Flickable::AutoFlickDirection);
    QCOMPARE(spy.count(),2);

    flickable->setFlickableDirection(QDeclarative1Flickable::HorizontalFlick);
    QCOMPARE(flickable->flickableDirection(), QDeclarative1Flickable::HorizontalFlick);
    QCOMPARE(spy.count(),3);

    flickable->setFlickableDirection(QDeclarative1Flickable::HorizontalFlick);
    QCOMPARE(flickable->flickableDirection(), QDeclarative1Flickable::HorizontalFlick);
    QCOMPARE(spy.count(),3);
}

void tst_qdeclarativeflickable::qgraphicswidget()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/flickableqgraphicswidget.qml"));
    QDeclarative1Flickable *flickable = qobject_cast<QDeclarative1Flickable*>(c.create());

    QVERIFY(flickable != 0);
    QGraphicsWidget *widget = findItem<QGraphicsWidget>(flickable->contentItem(), "widget1");
    QVERIFY(widget);
}

// QtQuick 1.1
void tst_qdeclarativeflickable::resizeContent()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/resize.qml"));
    QDeclarativeItem *root = qobject_cast<QDeclarativeItem*>(c.create());
    QDeclarative1Flickable *obj = findItem<QDeclarative1Flickable>(root, "flick");

    QVERIFY(obj != 0);
    QCOMPARE(obj->contentX(), 0.);
    QCOMPARE(obj->contentY(), 0.);
    QCOMPARE(obj->contentWidth(), 300.);
    QCOMPARE(obj->contentHeight(), 300.);

    QMetaObject::invokeMethod(root, "resizeContent");

    QCOMPARE(obj->contentX(), 100.);
    QCOMPARE(obj->contentY(), 100.);
    QCOMPARE(obj->contentWidth(), 600.);
    QCOMPARE(obj->contentHeight(), 600.);

    delete root;
}

// QtQuick 1.1
void tst_qdeclarativeflickable::returnToBounds()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/resize.qml"));
    QDeclarativeItem *root = qobject_cast<QDeclarativeItem*>(c.create());
    QDeclarative1Flickable *obj = findItem<QDeclarative1Flickable>(root, "flick");

    QVERIFY(obj != 0);
    QCOMPARE(obj->contentX(), 0.);
    QCOMPARE(obj->contentY(), 0.);
    QCOMPARE(obj->contentWidth(), 300.);
    QCOMPARE(obj->contentHeight(), 300.);

    obj->setContentX(100);
    obj->setContentY(400);
    QTRY_COMPARE(obj->contentX(), 100.);
    QTRY_COMPARE(obj->contentY(), 400.);

    QMetaObject::invokeMethod(root, "returnToBounds");

    QTRY_COMPARE(obj->contentX(), 0.);
    QTRY_COMPARE(obj->contentY(), 0.);

    delete root;
}

void tst_qdeclarativeflickable::testQtQuick11Attributes()
{
    QFETCH(QString, code);
    QFETCH(QString, warning);
    QFETCH(QString, error);

    QDeclarativeEngine engine;
    QObject *obj;

    QDeclarativeComponent invalid(&engine);
    invalid.setData("import QtQuick 1.0; Flickable { " + code.toUtf8() + " }", QUrl(""));
    QTest::ignoreMessage(QtWarningMsg, warning.toUtf8());
    obj = invalid.create();
    QCOMPARE(invalid.errorString(), error);
    delete obj;

    QDeclarativeComponent valid(&engine);
    valid.setData("import QtQuick 1.1; Flickable { " + code.toUtf8() + " }", QUrl(""));
    obj = valid.create();
    QVERIFY(obj);
    QVERIFY(valid.errorString().isEmpty());
    delete obj;
}

void tst_qdeclarativeflickable::testQtQuick11Attributes_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<QString>("error");

    QTest::newRow("resizeContent") << "Component.onCompleted: resizeContent(100,100,Qt.point(50,50))"
            << "<Unknown File>:1: ReferenceError: Can't find variable: resizeContent"
            << "";

    QTest::newRow("returnToBounds") << "Component.onCompleted: returnToBounds()"
            << "<Unknown File>:1: ReferenceError: Can't find variable: returnToBounds"
            << "";

}

void tst_qdeclarativeflickable::wheel()
{
    QDeclarativeView *canvas = new QDeclarativeView;
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/wheel.qml"));
    canvas->show();
    canvas->setFocus();
    QVERIFY(canvas->rootObject() != 0);

    QDeclarative1Flickable *flick = canvas->rootObject()->findChild<QDeclarative1Flickable*>("flick");
    QVERIFY(flick != 0);

    QGraphicsScene *scene = canvas->scene();
    QGraphicsSceneWheelEvent event(QEvent::GraphicsSceneWheel);
    event.setScenePos(QPointF(200, 200));
    event.setDelta(-120);
    event.setOrientation(Qt::Vertical);
    event.setAccepted(false);
    QApplication::sendEvent(scene, &event);

    QTRY_VERIFY(flick->contentY() > 0);
    QVERIFY(flick->contentX() == 0);

    flick->setContentY(0);
    QVERIFY(flick->contentY() == 0);

    event.setScenePos(QPointF(200, 200));
    event.setDelta(-120);
    event.setOrientation(Qt::Horizontal);
    event.setAccepted(false);
    QApplication::sendEvent(scene, &event);

    QTRY_VERIFY(flick->contentX() > 0);
    QVERIFY(flick->contentY() == 0);

    delete canvas;
}

void tst_qdeclarativeflickable::disabled()
{
    QDeclarativeView *canvas = new QDeclarativeView;
    canvas->setSource(QUrl::fromLocalFile(SRCDIR "/data/disabled.qml"));
    canvas->show();
    canvas->setFocus();
    QVERIFY(canvas->rootObject() != 0);

    QDeclarative1Flickable *flick = canvas->rootObject()->findChild<QDeclarative1Flickable*>("flickable");
    QVERIFY(flick != 0);

    QTest::mousePress(canvas->viewport(), Qt::LeftButton, 0, canvas->mapFromScene(QPoint(50,90)));

    QMouseEvent moveEvent(QEvent::MouseMove, canvas->mapFromScene(QPoint(50, 80)), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(canvas, &moveEvent);

    moveEvent = QMouseEvent(QEvent::MouseMove, canvas->mapFromScene(QPoint(50, 70)), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(canvas, &moveEvent);

    moveEvent = QMouseEvent(QEvent::MouseMove, canvas->mapFromScene(QPoint(50, 60)), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(canvas, &moveEvent);

    QVERIFY(flick->isMoving() == false);

    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, 0, canvas->mapFromScene(QPoint(50, 60)));

    // verify that mouse clicks on other elements still work (QTBUG-20584)
    QTest::mousePress(canvas->viewport(), Qt::LeftButton, 0, canvas->mapFromScene(QPoint(50, 10)));
    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, 0, canvas->mapFromScene(QPoint(50, 10)));

    QVERIFY(canvas->rootObject()->property("clicked").toBool() == true);
}

template<typename T>
T *tst_qdeclarativeflickable::findItem(QGraphicsObject *parent, const QString &objectName)
{
    const QMetaObject &mo = T::staticMetaObject;
    //qDebug() << parent->childItems().count() << "children";
    for (int i = 0; i < parent->childItems().count(); ++i) {
        QGraphicsObject *item = qobject_cast<QGraphicsObject*>(parent->childItems().at(i));
        if(!item)
            continue;
        //qDebug() << "try" << item;
        if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName)) {
            return static_cast<T*>(item);
        }
        item = findItem<T>(item, objectName);
        if (item)
            return static_cast<T*>(item);
    }

    return 0;
}

QTEST_MAIN(tst_qdeclarativeflickable)

#include "tst_qdeclarativeflickable.moc"
