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
#include <qtest.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <private/qdeclarativebind_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include "../../shared/util.h"

class tst_qdeclarativebinding : public QDeclarativeDataTest
{
    Q_OBJECT
public:
    tst_qdeclarativebinding();

private slots:
    void binding();
    void whenAfterValue();
    void restoreBinding();
    void restoreBindingWithLoop();
    void deletedObject();

private:
    QDeclarativeEngine engine;
};

tst_qdeclarativebinding::tst_qdeclarativebinding()
{
}

void tst_qdeclarativebinding::binding()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("test-binding.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect != 0);

    QDeclarativeBind *binding3 = qobject_cast<QDeclarativeBind*>(rect->findChild<QDeclarativeBind*>("binding3"));
    QVERIFY(binding3 != 0);

    QCOMPARE(rect->color(), QColor("yellow"));
    QCOMPARE(rect->property("text").toString(), QString("Hello"));
    QCOMPARE(binding3->when(), false);

    rect->setProperty("changeColor", true);
    QCOMPARE(rect->color(), QColor("red"));

    QCOMPARE(binding3->when(), true);

    QDeclarativeBind *binding = qobject_cast<QDeclarativeBind*>(rect->findChild<QDeclarativeBind*>("binding1"));
    QVERIFY(binding != 0);
    QCOMPARE(binding->object(), qobject_cast<QObject*>(rect));
    QCOMPARE(binding->property(), QLatin1String("text"));
    QCOMPARE(binding->value().toString(), QLatin1String("Hello"));

    delete rect;
}

void tst_qdeclarativebinding::whenAfterValue()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("test-binding2.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());

    QVERIFY(rect != 0);
    QCOMPARE(rect->color(), QColor("yellow"));
    QCOMPARE(rect->property("text").toString(), QString("Hello"));

    rect->setProperty("changeColor", true);
    QCOMPARE(rect->color(), QColor("red"));

    delete rect;
}

void tst_qdeclarativebinding::restoreBinding()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("restoreBinding.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect != 0);

    QQuickRectangle *myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != 0);

    myItem->setY(25);
    QCOMPARE(myItem->x(), qreal(100-25));

    myItem->setY(13);
    QCOMPARE(myItem->x(), qreal(100-13));

    //Binding takes effect
    myItem->setY(51);
    QCOMPARE(myItem->x(), qreal(51));

    myItem->setY(88);
    QCOMPARE(myItem->x(), qreal(88));

    //original binding restored
    myItem->setY(49);
    QCOMPARE(myItem->x(), qreal(100-49));

    delete rect;
}

void tst_qdeclarativebinding::restoreBindingWithLoop()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("restoreBindingWithLoop.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect != 0);

    QQuickRectangle *myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != 0);

    myItem->setY(25);
    QCOMPARE(myItem->x(), qreal(25 + 100));

    myItem->setY(13);
    QCOMPARE(myItem->x(), qreal(13 + 100));

    //Binding takes effect
    rect->setProperty("activateBinding", true);
    myItem->setY(51);
    QCOMPARE(myItem->x(), qreal(51));

    myItem->setY(88);
    QCOMPARE(myItem->x(), qreal(88));

    //original binding restored
    QString warning = c.url().toString() + QLatin1String(":9:5: QML Rectangle: Binding loop detected for property \"x\"");
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));
    rect->setProperty("activateBinding", false);
    QCOMPARE(myItem->x(), qreal(88 + 100)); //if loop handling changes this could be 90 + 100

    myItem->setY(49);
    QCOMPARE(myItem->x(), qreal(49 + 100));

    delete rect;
}

//QTBUG-20692
void tst_qdeclarativebinding::deletedObject()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("deletedObject.qml"));
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(c.create());
    QVERIFY(rect != 0);

    QGuiApplication::sendPostedEvents(0, QEvent::DeferredDelete);

    //don't crash
    rect->setProperty("activateBinding", true);

    delete rect;
}

QTEST_MAIN(tst_qdeclarativebinding)

#include "tst_qdeclarativebinding.moc"
