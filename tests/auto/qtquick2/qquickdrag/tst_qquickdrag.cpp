/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativeexpression.h>

template <typename T> static T evaluate(QObject *scope, const QString &expression)
{
    QDeclarativeExpression expr(qmlContext(scope), scope, expression);
    QVariant result = expr.evaluate();
    if (expr.hasError())
        qWarning() << expr.error().toString();
    return result.value<T>();
}

template <> void evaluate<void>(QObject *scope, const QString &expression)
{
    QDeclarativeExpression expr(qmlContext(scope), scope, expression);
    expr.evaluate();
    if (expr.hasError())
        qWarning() << expr.error().toString();
}

Q_DECLARE_METATYPE(Qt::DropActions)

class TestDropTarget : public QQuickItem
{
    Q_OBJECT
public:
    TestDropTarget(QQuickItem *parent = 0)
        : QQuickItem(parent)
        , enterEvents(0)
        , moveEvents(0)
        , leaveEvents(0)
        , dropEvents(0)
        , acceptAction(Qt::MoveAction)
        , defaultAction(Qt::IgnoreAction)
        , proposedAction(Qt::IgnoreAction)
        , accept(true)
    {
        setFlags(ItemAcceptsDrops);
    }

    void reset()
    {
        enterEvents = 0;
        moveEvents = 0;
        leaveEvents = 0;
        dropEvents = 0;
        defaultAction = Qt::IgnoreAction;
        proposedAction = Qt::IgnoreAction;
        supportedActions = Qt::IgnoreAction;
    }

    void dragEnterEvent(QDragEnterEvent *event)
    {
        ++enterEvents;
        position = event->pos();
        defaultAction = event->dropAction();
        proposedAction = event->proposedAction();
        supportedActions = event->possibleActions();
        event->setAccepted(accept);
    }

    void dragMoveEvent(QDragMoveEvent *event)
    {
        ++moveEvents;
        position = event->pos();
        defaultAction = event->dropAction();
        proposedAction = event->proposedAction();
        supportedActions = event->possibleActions();
        event->setAccepted(accept);
    }

    void dragLeaveEvent(QDragLeaveEvent *event)
    {
        ++leaveEvents;
        event->setAccepted(accept);
    }

    void dropEvent(QDropEvent *event)
    {
        ++dropEvents;
        position = event->pos();
        defaultAction = event->dropAction();
        proposedAction = event->proposedAction();
        supportedActions = event->possibleActions();
        event->setDropAction(acceptAction);
        event->setAccepted(accept);
    }

    int enterEvents;
    int moveEvents;
    int leaveEvents;
    int dropEvents;
    Qt::DropAction acceptAction;
    Qt::DropAction defaultAction;
    Qt::DropAction proposedAction;
    Qt::DropActions supportedActions;
    QPointF position;
    bool accept;
};

class tst_QQuickDrag: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void active();
    void drop();
    void move();
    void hotSpot();
    void supportedActions();
    void proposedAction();
    void keys();
    void source();

private:
    QDeclarativeEngine engine;
};

void tst_QQuickDrag::initTestCase()
{

}

void tst_QQuickDrag::cleanupTestCase()
{

}

void tst_QQuickDrag::active()
{
    QQuickCanvas canvas;
    TestDropTarget dropTarget(canvas.rootItem());
    dropTarget.setSize(QSizeF(100, 100));
    QDeclarativeComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property bool dragActive: Drag.active\n"
                "property Item dragTarget: Drag.target\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&dropTarget);

    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);

    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = false");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 1);

    dropTarget.reset();
    evaluate<void>(item, "Drag.cancel()");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.reset();
    evaluate<void>(item, "Drag.start()");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 0);

    // Start while a drag is active, cancels the previous drag and starts a new one.
    dropTarget.reset();
    evaluate<void>(item, "Drag.start()");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 1);

    dropTarget.reset();
    evaluate<void>(item, "Drag.cancel()");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 1);

    // Enter events aren't sent to items without the QQuickItem::ItemAcceptsDrops flag.
    dropTarget.setFlags(QQuickItem::Flags());

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = false");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.setFlags(QQuickItem::ItemAcceptsDrops);

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.setFlags(QQuickItem::Flags());

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = false");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 1);

    // Follow up events aren't sent to items if the enter event isn't accepted.
    dropTarget.setFlags(QQuickItem::ItemAcceptsDrops);
    dropTarget.accept = false;

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = false");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.accept = true;

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&dropTarget));
    QCOMPARE(dropTarget.enterEvents, 1); QCOMPARE(dropTarget.leaveEvents, 0);

    dropTarget.accept = false;

    dropTarget.reset();
    evaluate<void>(item, "Drag.active = false");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 1);

    // Events are sent to hidden or disabled items.
    dropTarget.accept = true;
    dropTarget.setVisible(false);
    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    evaluate<void>(item, "Drag.active = false");
    dropTarget.setVisible(true);

    dropTarget.setOpacity(0.0);
    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);

    evaluate<void>(item, "Drag.active = false");
    dropTarget.setOpacity(1.0);

    dropTarget.setEnabled(false);
    dropTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(dropTarget.enterEvents, 0); QCOMPARE(dropTarget.leaveEvents, 0);
}

void tst_QQuickDrag::drop()
{
    QQuickCanvas canvas;
    TestDropTarget outerTarget(canvas.rootItem());
    outerTarget.setSize(QSizeF(100, 100));
    outerTarget.acceptAction = Qt::CopyAction;
    TestDropTarget innerTarget(&outerTarget);
    innerTarget.setSize(QSizeF(100, 100));
    innerTarget.acceptAction = Qt::MoveAction;
    QDeclarativeComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property bool dragActive: Drag.active\n"
                "property Item dragTarget: Drag.target\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&outerTarget);

    innerTarget.reset(); outerTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 1); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    innerTarget.reset(); outerTarget.reset();
    QCOMPARE(evaluate<bool>(item, "Drag.drop() == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 1); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 1);

    innerTarget.reset(); outerTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&innerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 1); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    // Inner target declines the drop so it is propagated to the outer target.
    innerTarget.accept = false;

    innerTarget.reset(); outerTarget.reset();
    QCOMPARE(evaluate<bool>(item, "Drag.drop() == Qt.CopyAction"), true);
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 1);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 1);


    // Inner target doesn't accept enter so drop goes directly to outer.
    innerTarget.accept = true;
    innerTarget.setFlags(QQuickItem::Flags());

    innerTarget.reset(); outerTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    innerTarget.reset(); outerTarget.reset();
    QCOMPARE(evaluate<bool>(item, "Drag.drop() == Qt.CopyAction"), true);
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 1);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    // Neither target accepts drop so Qt::IgnoreAction is returned.
    innerTarget.reset(); outerTarget.reset();
    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    outerTarget.accept = false;

    innerTarget.reset(); outerTarget.reset();
    QCOMPARE(evaluate<bool>(item, "Drag.drop() == Qt.IgnoreAction"), true);
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 1);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);

    // drop doesn't send an event and returns Qt.IgnoreAction if not active.
    innerTarget.accept = true;
    outerTarget.accept = true;
    innerTarget.reset(); outerTarget.reset();
    QCOMPARE(evaluate<bool>(item, "Drag.drop() == Qt.IgnoreAction"), true);
    QCOMPARE(evaluate<bool>(item, "Drag.active"), false);
    QCOMPARE(evaluate<bool>(item, "dragActive"), false);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.dropEvents, 0);
    QCOMPARE(innerTarget.enterEvents, 0); QCOMPARE(innerTarget.leaveEvents, 0); QCOMPARE(innerTarget.dropEvents, 0);
}

void tst_QQuickDrag::move()
{
    QQuickCanvas canvas;
    TestDropTarget outerTarget(canvas.rootItem());
    outerTarget.setSize(QSizeF(100, 100));
    TestDropTarget leftTarget(&outerTarget);
    leftTarget.setPos(QPointF(0, 35));
    leftTarget.setSize(QSizeF(30, 30));
    TestDropTarget rightTarget(&outerTarget);
    rightTarget.setPos(QPointF(70, 35));
    rightTarget.setSize(QSizeF(30, 30));
    QDeclarativeComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property bool dragActive: Drag.active\n"
                "property Item dragTarget: Drag.target\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&outerTarget);

    evaluate<void>(item, "Drag.active = true");
    QCOMPARE(evaluate<bool>(item, "Drag.active"), true);
    QCOMPARE(evaluate<bool>(item, "dragActive"), true);
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 0);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(50)); QCOMPARE(outerTarget.position.y(), qreal(50));

    // Move within the outer target.
    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(60, 50));
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(60)); QCOMPARE(outerTarget.position.y(), qreal(50));

    // Move into the right target.
    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(75, 50));
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&rightTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&rightTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 1); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(75)); QCOMPARE(outerTarget.position.y(), qreal(50));
    QCOMPARE(rightTarget.position.x(), qreal(5)); QCOMPARE(rightTarget.position.y(), qreal(15));

    // Move into the left target.
    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(25, 50));
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&leftTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&leftTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 1); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 1); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(25)); QCOMPARE(outerTarget.position.y(), qreal(50));
    QCOMPARE(leftTarget.position.x(), qreal(25)); QCOMPARE(leftTarget.position.y(), qreal(15));

    // Move within the left target.
    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(25, 40));
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&leftTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&leftTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 1);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(25)); QCOMPARE(outerTarget.position.y(), qreal(40));
    QCOMPARE(leftTarget.position.x(), qreal(25)); QCOMPARE(leftTarget.position.y(), qreal(5));

    // Move out of all targets.
    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(110, 50));
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(0));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(0));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 1); QCOMPARE(outerTarget.moveEvents, 0);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 1); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);

    // Stop the right target accepting drag events and move into it.
    rightTarget.accept = false;

    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(80, 50));
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 1); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 0);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 1); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(80)); QCOMPARE(outerTarget.position.y(), qreal(50));

    // Stop the outer target accepting drag events after it has accepted an enter event.
    outerTarget.accept = false;

    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(60, 50));
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(60)); QCOMPARE(outerTarget.position.y(), qreal(50));

    // Clear the QQuickItem::ItemAcceptsDrops flag from the outer target after it accepted an enter event.
    outerTarget.setFlags(QQuickItem::Flags());

    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(40, 50));
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(40)); QCOMPARE(outerTarget.position.y(), qreal(50));

    // Clear the QQuickItem::ItemAcceptsDrops flag from the left target before it accepts an enter event.
    leftTarget.setFlags(QQuickItem::Flags());

    outerTarget.reset(); leftTarget.reset(); rightTarget.reset();
    item->setPos(QPointF(25, 50));
    QCOMPARE(evaluate<QObject *>(item, "Drag.target"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(evaluate<QObject *>(item, "dragTarget"), static_cast<QObject *>(&outerTarget));
    QCOMPARE(outerTarget.enterEvents, 0); QCOMPARE(outerTarget.leaveEvents, 0); QCOMPARE(outerTarget.moveEvents, 1);
    QCOMPARE(leftTarget .enterEvents, 0); QCOMPARE(leftTarget .leaveEvents, 0); QCOMPARE(leftTarget .moveEvents, 0);
    QCOMPARE(rightTarget.enterEvents, 0); QCOMPARE(rightTarget.leaveEvents, 0); QCOMPARE(rightTarget.moveEvents, 0);
    QCOMPARE(outerTarget.position.x(), qreal(25)); QCOMPARE(outerTarget.position.y(), qreal(50));
}


void tst_QQuickDrag::hotSpot()
{
    QQuickCanvas canvas;
    TestDropTarget dropTarget(canvas.rootItem());
    dropTarget.setSize(QSizeF(100, 100));
    QDeclarativeComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property real hotSpotX: Drag.hotSpot.x\n"
                "property real hotSpotY: Drag.hotSpot.y\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&dropTarget);

    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.x"), qreal(0));
    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.y"), qreal(0));
    QCOMPARE(evaluate<qreal>(item, "hotSpotX"), qreal(0));
    QCOMPARE(evaluate<qreal>(item, "hotSpotY"), qreal(0));

    evaluate<void>(item, "{ Drag.start(); Drag.cancel() }");
    QCOMPARE(dropTarget.position.x(), qreal(50));
    QCOMPARE(dropTarget.position.y(), qreal(50));

    evaluate<void>(item, "{ Drag.hotSpot.x = 5, Drag.hotSpot.y = 5 }");
    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.x"), qreal(5));
    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.y"), qreal(5));
    QCOMPARE(evaluate<qreal>(item, "hotSpotX"), qreal(5));
    QCOMPARE(evaluate<qreal>(item, "hotSpotY"), qreal(5));

    evaluate<void>(item, "Drag.start()");
    QCOMPARE(dropTarget.position.x(), qreal(55));
    QCOMPARE(dropTarget.position.y(), qreal(55));

    item->setPos(QPointF(30, 20));
    QCOMPARE(dropTarget.position.x(), qreal(35));
    QCOMPARE(dropTarget.position.y(), qreal(25));

    evaluate<void>(item, "{ Drag.hotSpot.x = 10; Drag.hotSpot.y = 10 }");
    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.x"), qreal(10));
    QCOMPARE(evaluate<qreal>(item, "Drag.hotSpot.y"), qreal(10));
    QCOMPARE(evaluate<qreal>(item, "hotSpotX"), qreal(10));
    QCOMPARE(evaluate<qreal>(item, "hotSpotY"), qreal(10));
    // Changing the hotSpot won't generate a move event so the position is unchanged.  Should it?
    QCOMPARE(dropTarget.position.x(), qreal(35));
    QCOMPARE(dropTarget.position.y(), qreal(25));

    item->setPos(QPointF(10, 20));
    QCOMPARE(dropTarget.position.x(), qreal(20));
    QCOMPARE(dropTarget.position.y(), qreal(30));
}

void tst_QQuickDrag::supportedActions()
{
    QQuickCanvas canvas;
    TestDropTarget dropTarget(canvas.rootItem());
    dropTarget.setSize(QSizeF(100, 100));
    QDeclarativeComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property int supportedActions: Drag.supportedActions\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&dropTarget);

    QCOMPARE(evaluate<bool>(item, "Drag.supportedActions == Qt.CopyAction | Qt.MoveAction | Qt.LinkAction"), true);
    QCOMPARE(evaluate<bool>(item, "supportedActions == Qt.CopyAction | Qt.MoveAction | Qt.LinkAction"), true);
    evaluate<void>(item, "{ Drag.start(); Drag.cancel() }");
    QCOMPARE(dropTarget.supportedActions, Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);

    evaluate<void>(item, "Drag.supportedActions = Qt.CopyAction | Qt.MoveAction");
    QCOMPARE(evaluate<bool>(item, "Drag.supportedActions == Qt.CopyAction | Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "supportedActions == Qt.CopyAction | Qt.MoveAction"), true);
    evaluate<void>(item, "Drag.start()");
    QCOMPARE(dropTarget.supportedActions, Qt::CopyAction | Qt::MoveAction);

    // Once a drag is started the proposed actions are locked in for future events.
    evaluate<void>(item, "Drag.supportedActions = Qt.MoveAction");
    QCOMPARE(evaluate<bool>(item, "Drag.supportedActions == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "supportedActions == Qt.MoveAction"), true);
    item->setPos(QPointF(60, 60));
    QCOMPARE(dropTarget.supportedActions, Qt::CopyAction | Qt::MoveAction);

    // Calling start with proposed actions will override the current actions for the next sequence.
    evaluate<void>(item, "Drag.start(Qt.CopyAction)");
    QCOMPARE(evaluate<bool>(item, "Drag.supportedActions == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "supportedActions == Qt.MoveAction"), true);
    QCOMPARE(dropTarget.supportedActions, Qt::CopyAction);

    evaluate<void>(item, "Drag.start()");
    QCOMPARE(evaluate<bool>(item, "Drag.supportedActions == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "supportedActions == Qt.MoveAction"), true);
    QCOMPARE(dropTarget.supportedActions, Qt::MoveAction);
}

void tst_QQuickDrag::proposedAction()
{
    QQuickCanvas canvas;
    TestDropTarget dropTarget(canvas.rootItem());
    dropTarget.setSize(QSizeF(100, 100));
    QDeclarativeComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property int proposedAction: Drag.proposedAction\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setParentItem(&dropTarget);


    QCOMPARE(evaluate<bool>(item, "Drag.proposedAction == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "proposedAction == Qt.MoveAction"), true);
    evaluate<void>(item, "{ Drag.start(); Drag.cancel() }");
    QCOMPARE(dropTarget.defaultAction, Qt::MoveAction);
    QCOMPARE(dropTarget.proposedAction, Qt::MoveAction);

    evaluate<void>(item, "Drag.proposedAction = Qt.CopyAction");
    QCOMPARE(evaluate<bool>(item, "Drag.proposedAction == Qt.CopyAction"), true);
    QCOMPARE(evaluate<bool>(item, "proposedAction == Qt.CopyAction"), true);
    evaluate<void>(item, "Drag.start()");
    QCOMPARE(dropTarget.defaultAction, Qt::CopyAction);
    QCOMPARE(dropTarget.proposedAction, Qt::CopyAction);

    // The proposed action can change during a drag.
    evaluate<void>(item, "Drag.proposedAction = Qt.MoveAction");
    QCOMPARE(evaluate<bool>(item, "Drag.proposedAction == Qt.MoveAction"), true);
    QCOMPARE(evaluate<bool>(item, "proposedAction == Qt.MoveAction"), true);
    item->setPos(QPointF(60, 60));
    QCOMPARE(dropTarget.defaultAction, Qt::MoveAction);
    QCOMPARE(dropTarget.proposedAction, Qt::MoveAction);

    evaluate<void>(item, "Drag.proposedAction = Qt.LinkAction");
    QCOMPARE(evaluate<bool>(item, "Drag.proposedAction == Qt.LinkAction"), true);
    QCOMPARE(evaluate<bool>(item, "proposedAction == Qt.LinkAction"), true);
    evaluate<void>(item, "Drag.drop()");
    QCOMPARE(dropTarget.defaultAction, Qt::LinkAction);
    QCOMPARE(dropTarget.proposedAction, Qt::LinkAction);
}

void tst_QQuickDrag::keys()
{
    QDeclarativeComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property variant keys: Drag.keys\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);

//    QCOMPARE(evaluate<QStringList>(item, "Drag.keys"), QStringList());
//    QCOMPARE(evaluate<QStringList>(item, "keys"), QStringList());
    QCOMPARE(item->property("keys").toStringList(), QStringList());

    evaluate<void>(item, "Drag.keys = [\"red\", \"blue\"]");
//    QCOMPARE(evaluate<QStringList>(item, "Drag.keys"), QStringList() << "red" << "blue");
//    QCOMPARE(evaluate<QStringList>(item, "keys"), QStringList() << "red" << "blue");
    QCOMPARE(item->property("keys").toStringList(), QStringList() << "red" << "blue");
}

void tst_QQuickDrag::source()
{

    QDeclarativeComponent component(&engine);
    component.setData(
            "import QtQuick 2.0\n"
            "Item {\n"
                "property Item source: Drag.source\n"
                "x: 50; y: 50\n"
                "width: 10; height: 10\n"
                "Item { id: proxySource; objectName: \"proxySource\" }\n"
            "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);

    QCOMPARE(evaluate<QObject *>(item, "Drag.source"), static_cast<QObject *>(item));
    QCOMPARE(evaluate<QObject *>(item, "source"), static_cast<QObject *>(item));

    QQuickItem *proxySource = item->findChild<QQuickItem *>("proxySource");
    QVERIFY(proxySource);

    evaluate<void>(item, "Drag.source = proxySource");
    QCOMPARE(evaluate<QObject *>(item, "Drag.source"), static_cast<QObject *>(proxySource));
    QCOMPARE(evaluate<QObject *>(item, "source"), static_cast<QObject *>(proxySource));

    evaluate<void>(item, "Drag.source = undefined");
    QCOMPARE(evaluate<QObject *>(item, "Drag.source"), static_cast<QObject *>(item));
    QCOMPARE(evaluate<QObject *>(item, "source"), static_cast<QObject *>(item));
}

QTEST_MAIN(tst_QQuickDrag)

#include "tst_qquickdrag.moc"
