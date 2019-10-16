/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <private/qqmlbind_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include "../../shared/util.h"

class tst_qqmlbinding : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlbinding();

private slots:
    void binding();
    void whenAfterValue();
    void restoreBinding();
    void restoreBindingValue();
    void restoreBindingVarValue();
    void restoreBindingJSValue();
    void restoreBindingWithLoop();
    void restoreBindingWithoutCrash();
    void deletedObject();
    void warningOnUnknownProperty();
    void warningOnReadOnlyProperty();
    void disabledOnUnknownProperty();
    void disabledOnReadonlyProperty();
    void delayed();
    void bindingOverwriting();
    void bindToQmlComponent();
    void bindingDoesNoWeirdConversion();
    void bindNaNToInt();

private:
    QQmlEngine engine;
};

tst_qqmlbinding::tst_qqmlbinding()
{
}

void tst_qqmlbinding::binding()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("test-binding.qml"));
    QScopedPointer<QQuickRectangle> rect { qobject_cast<QQuickRectangle*>(c.create()) };
    QVERIFY(rect != nullptr);

    QQmlBind *binding3 = qobject_cast<QQmlBind*>(rect->findChild<QQmlBind*>("binding3"));
    QVERIFY(binding3 != nullptr);

    QCOMPARE(rect->color(), QColor("yellow"));
    QCOMPARE(rect->property("text").toString(), QString("Hello"));
    QCOMPARE(binding3->when(), false);

    rect->setProperty("changeColor", true);
    QCOMPARE(rect->color(), QColor("red"));

    QCOMPARE(binding3->when(), true);

    QQmlBind *binding = qobject_cast<QQmlBind*>(rect->findChild<QQmlBind*>("binding1"));
    QVERIFY(binding != nullptr);
    QCOMPARE(binding->object(), qobject_cast<QObject*>(rect.get()));
    QCOMPARE(binding->property(), QLatin1String("text"));
    QCOMPARE(binding->value().toString(), QLatin1String("Hello"));
}

void tst_qqmlbinding::whenAfterValue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("test-binding2.qml"));
    QScopedPointer<QQuickRectangle> rect {qobject_cast<QQuickRectangle*>(c.create())};

    QVERIFY(rect != nullptr);
    QCOMPARE(rect->color(), QColor("yellow"));
    QCOMPARE(rect->property("text").toString(), QString("Hello"));

    rect->setProperty("changeColor", true);
    QCOMPARE(rect->color(), QColor("red"));
}

void tst_qqmlbinding::restoreBinding()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBinding.qml"));
    QScopedPointer<QQuickRectangle> rect { qobject_cast<QQuickRectangle*>(c.create()) };
    QVERIFY(rect != nullptr);

    QQuickRectangle *myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

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
}

void tst_qqmlbinding::restoreBindingValue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBinding2.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(!rect.isNull());

    auto myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

    QCOMPARE(myItem->height(), 100);
    myItem->setProperty("when", QVariant(false));
    QCOMPARE(myItem->height(), 300); // make sure the original value was restored

    myItem->setProperty("when", QVariant(true));
    QCOMPARE(myItem->height(), 100); // make sure the value specified in Binding is set
    rect->setProperty("boundValue", 200);
    QCOMPARE(myItem->height(), 200); // make sure the changed binding value is set
    myItem->setProperty("when", QVariant(false));
    // make sure that the original value is back, not e.g. the value from before the
    // change (i.e. 100)
    QCOMPARE(myItem->height(), 300);
}

void tst_qqmlbinding::restoreBindingVarValue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBinding3.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(!rect.isNull());

    auto myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

    QCOMPARE(myItem->property("foo"), 13);
    myItem->setProperty("when", QVariant(false));
    QCOMPARE(myItem->property("foo"), 42); // make sure the original value was restored

    myItem->setProperty("when", QVariant(true));
    QCOMPARE(myItem->property("foo"), 13); // make sure the value specified in Binding is set
    rect->setProperty("boundValue", 31337);
    QCOMPARE(myItem->property("foo"), 31337); // make sure the changed binding value is set
    myItem->setProperty("when", QVariant(false));
    // make sure that the original value is back, not e.g. the value from before the
    // change (i.e. 100)
    QCOMPARE(myItem->property("foo"), 42);
}

void tst_qqmlbinding::restoreBindingJSValue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBinding4.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(!rect.isNull());

    auto myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

    QCOMPARE(myItem->property("fooCheck"), false);
    myItem->setProperty("when", QVariant(false));
    QCOMPARE(myItem->property("fooCheck"), true); // make sure the original value was restored

    myItem->setProperty("when", QVariant(true));
    QCOMPARE(myItem->property("fooCheck"), false); // make sure the value specified in Binding is set
    rect->setProperty("boundValue", 31337);
    QCOMPARE(myItem->property("fooCheck"), false); // make sure the changed binding value is set
    myItem->setProperty("when", QVariant(false));
    // make sure that the original value is back, not e.g. the value from before the change
    QCOMPARE(myItem->property("fooCheck"), true);

}

void tst_qqmlbinding::restoreBindingWithLoop()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBindingWithLoop.qml"));
    QScopedPointer<QQuickRectangle> rect {qobject_cast<QQuickRectangle*>(c.create())};
    QVERIFY(rect != nullptr);

    QQuickRectangle *myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

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
}

void tst_qqmlbinding::restoreBindingWithoutCrash()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBindingWithoutCrash.qml"));
    QScopedPointer<QQuickRectangle> rect {qobject_cast<QQuickRectangle*>(c.create())};
    QVERIFY(rect != nullptr);

    QQuickRectangle *myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

    myItem->setY(25);
    QCOMPARE(myItem->x(), qreal(100-25));

    myItem->setY(13);
    QCOMPARE(myItem->x(), qreal(100-13));

    //Binding takes effect
    myItem->setY(51);
    QCOMPARE(myItem->x(), qreal(51));

    myItem->setY(88);
    QCOMPARE(myItem->x(), qreal(88));

    //state sets a new binding
    rect->setState("state1");
    //this binding temporarily takes effect. We may want to change this behavior in the future
    QCOMPARE(myItem->x(), qreal(112));

    //Binding still controls this value
    myItem->setY(104);
    QCOMPARE(myItem->x(), qreal(104));

    //original binding restored
    myItem->setY(49);
    QCOMPARE(myItem->x(), qreal(100-49));
}

//QTBUG-20692
void tst_qqmlbinding::deletedObject()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("deletedObject.qml"));
    QScopedPointer<QQuickRectangle> rect {qobject_cast<QQuickRectangle*>(c.create())};
    QVERIFY(rect != nullptr);

    QGuiApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    //don't crash
    rect->setProperty("activateBinding", true);
}

void tst_qqmlbinding::warningOnUnknownProperty()
{
    QQmlTestMessageHandler messageHandler;

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("unknownProperty.qml"));
    QScopedPointer<QQuickItem> item { qobject_cast<QQuickItem *>(c.create()) };
    QVERIFY(item);

    QCOMPARE(messageHandler.messages().count(), 1);

    const QString expectedMessage = c.url().toString() + QLatin1String(":6:5: QML Binding: Property 'unknown' does not exist on Item.");
    QCOMPARE(messageHandler.messages().first(), expectedMessage);
}

void tst_qqmlbinding::warningOnReadOnlyProperty()
{
    QQmlTestMessageHandler messageHandler;

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("readonlyProperty.qml"));
    QScopedPointer<QQuickItem> item { qobject_cast<QQuickItem *>(c.create()) };
    QVERIFY(item);

    QCOMPARE(messageHandler.messages().count(), 1);

    const QString expectedMessage = c.url().toString() + QLatin1String(":8:5: QML Binding: Property 'name' on Item is read-only.");
    QCOMPARE(messageHandler.messages().first(), expectedMessage);
}

void tst_qqmlbinding::disabledOnUnknownProperty()
{
    QQmlTestMessageHandler messageHandler;

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("disabledUnknown.qml"));
    QScopedPointer<QQuickItem> item { qobject_cast<QQuickItem *>(c.create()) };
    QVERIFY(item);

    QCOMPARE(messageHandler.messages().count(), 0);
}

void tst_qqmlbinding::disabledOnReadonlyProperty()
{
    QQmlTestMessageHandler messageHandler;

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("disabledReadonly.qml"));
    QScopedPointer<QQuickItem> item { qobject_cast<QQuickItem *>(c.create()) };
    QVERIFY(item);
    QCOMPARE(messageHandler.messages().count(), 0);
}

void tst_qqmlbinding::delayed()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("delayed.qml"));
    QScopedPointer<QQuickItem> item {qobject_cast<QQuickItem*>(c.create())};

    QVERIFY(item != nullptr);
    // update on creation
    QCOMPARE(item->property("changeCount").toInt(), 1);

    QMetaObject::invokeMethod(item.get(), "updateText");
    // doesn't update immediately
    QCOMPARE(item->property("changeCount").toInt(), 1);

    QCoreApplication::processEvents();
    // only updates once (non-delayed would update twice)
    QCOMPARE(item->property("changeCount").toInt(), 2);
}

void tst_qqmlbinding::bindingOverwriting()
{
    QQmlTestMessageHandler messageHandler;
    QLoggingCategory::setFilterRules(QStringLiteral("qt.qml.binding.removal.info=true"));

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("bindingOverwriting.qml"));
    QScopedPointer<QQuickItem> item {qobject_cast<QQuickItem*>(c.create())};
    QVERIFY(item);

    QLoggingCategory::setFilterRules(QString());
    QCOMPARE(messageHandler.messages().count(), 2);
}

void tst_qqmlbinding::bindToQmlComponent()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("bindToQMLComponent.qml"));
    QVERIFY(c.create());
}

// QTBUG-78943
void tst_qqmlbinding::bindingDoesNoWeirdConversion()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("noUnexpectedStringConversion.qml"));
    QScopedPointer<QObject> o {c.create()};
    QVERIFY(o);
    QObject *colorRect = o->findChild<QObject*>("colorRect");
    QVERIFY(colorRect);
    QCOMPARE(qvariant_cast<QColor>(colorRect->property("color")), QColorConstants::Red);
    QObject *colorLabel = o->findChild<QObject*>("colorLabel");
    QCOMPARE(colorLabel->property("text").toString(), QLatin1String("red"));
    QVERIFY(colorLabel);
}

//QTBUG-72442
void tst_qqmlbinding::bindNaNToInt()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("nanPropertyToInt.qml"));
    QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem*>(c.create()));

    QVERIFY(item != nullptr);
    QCOMPARE(item->property("val").toInt(), 0);
}
QTEST_MAIN(tst_qqmlbinding)

#include "tst_qqmlbinding.moc"
