/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include "QtTest/qtestaccessible.h"

#include <QtGui/qaccessible.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>
#include <QtQuick/private/qquickaccessibleattached_p.h>

#include "../../shared/util.h"


#define EXPECT(cond) \
    do { \
        if (!errorAt && !(cond)) { \
            errorAt = __LINE__; \
            qWarning("level: %d, middle: %d, role: %d (%s)", treelevel, middle, iface->role(), #cond); \
        } \
    } while (0)

static int verifyHierarchy(QAccessibleInterface *iface)
{
    int errorAt = 0;
    static int treelevel = 0;   // for error diagnostics
    QAccessibleInterface *middleChild, *if2;
    middleChild = 0;
    ++treelevel;
    int middle = iface->childCount()/2 + 1;
    if (iface->childCount() >= 2) {
        middleChild = iface->child(middle - 1);
    }
    for (int i = 0; i < iface->childCount() && !errorAt; ++i) {
        if2 = iface->child(i);
        EXPECT(if2 != 0);
        // navigate Ancestor...
        QAccessibleInterface *parent = if2->parent();
        EXPECT(iface->object() == parent->object());

        // verify children...
        if (!errorAt)
            errorAt = verifyHierarchy(if2);
    }

    --treelevel;
    return errorAt;
}


//TESTED_FILES=

class tst_QQuickAccessible : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickAccessible();
    virtual ~tst_QQuickAccessible();

private slots:
    void commonTests_data();
    void commonTests();

    void quickAttachedProperties();
    void basicPropertiesTest();
    void hitTest();
    void checkableTest();
};

tst_QQuickAccessible::tst_QQuickAccessible()
{

}

tst_QQuickAccessible::~tst_QQuickAccessible()
{

}

void tst_QQuickAccessible::commonTests_data()
{
    QTest::addColumn<QString>("accessibleRoleFileName");

    QTest::newRow("StaticText") << "statictext.qml";
    QTest::newRow("PushButton") << "pushbutton.qml";
}

void tst_QQuickAccessible::commonTests()
{
    QFETCH(QString, accessibleRoleFileName);

    qDebug() << "testing" << accessibleRoleFileName;

    QQuickView *view = new QQuickView();
//    view->setFixedSize(240,320);
    view->setSource(testFileUrl(accessibleRoleFileName));
    view->show();
//    view->setFocus();
    QVERIFY(view->rootObject() != 0);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(view);
    QVERIFY(iface);

    delete view;
}



QString eventName(const int ev)
{
    switch (ev) {
    case 0x0001: return "SoundPlayed";
    case 0x0002: return "Alert";
    case 0x0003: return "ForegroundChanged";
    case 0x0004: return "MenuStart";
    case 0x0005: return "MenuEnd";
    case 0x0006: return "PopupMenuStart";
    case 0x0007: return "PopupMenuEnd";
    case 0x000C: return "ContextHelpStart";
    case 0x000D: return "ContextHelpEnd";
    case 0x000E: return "DragDropStart";
    case 0x000F: return "DragDropEnd";
    case 0x0010: return "DialogStart";
    case 0x0011: return "DialogEnd";
    case 0x0012: return "ScrollingStart";
    case 0x0013: return "ScrollingEnd";
    case 0x0018: return "MenuCommand";
    case 0x8000: return "ObjectCreated";
    case 0x8001: return "ObjectDestroyed";
    case 0x8002: return "ObjectShow";
    case 0x8003: return "ObjectHide";
    case 0x8004: return "ObjectReorder";
    case 0x8005: return "Focus";
    case 0x8006: return "Selection";
    case 0x8007: return "SelectionAdd";
    case 0x8008: return "SelectionRemove";
    case 0x8009: return "SelectionWithin";
    case 0x800A: return "StateChanged";
    case 0x800B: return "LocationChanged";
    case 0x800C: return "NameChanged";
    case 0x800D: return "DescriptionChanged";
    case 0x800E: return "ValueChanged";
    case 0x800F: return "ParentChanged";
    case 0x80A0: return "HelpChanged";
    case 0x80B0: return "DefaultActionChanged";
    case 0x80C0: return "AcceleratorChanged";
    default: return "Unknown Event";
    }
}

void tst_QQuickAccessible::quickAttachedProperties()
{
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nItem {\n"
                                "}", QUrl());
        QObject *object = component.create();
        QVERIFY(object != 0);

        QObject *attachedObject = QQuickAccessibleAttached::attachedProperties(object);
        QCOMPARE(attachedObject, static_cast<QObject*>(0));
        delete object;
    }

    // Attached property
    {
        QObject parent;
        QQuickAccessibleAttached *attachedObj = new QQuickAccessibleAttached(&parent);

        attachedObj->name();

        QVariant pp = attachedObj->property("name");
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nItem {\n"
                                "Accessible.role: Accessible.Button\n"
                                "}", QUrl());
        QObject *object = component.create();
        QVERIFY(object != 0);

        QObject *attachedObject = QQuickAccessibleAttached::attachedProperties(object);
        QVERIFY(attachedObject);
        if (attachedObject) {
            QVariant p = attachedObject->property("role");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toInt(), int(QAccessible::PushButton));
            p = attachedObject->property("name");
            QCOMPARE(p.isNull(), true);
            p = attachedObject->property("description");
            QCOMPARE(p.isNull(), true);
        }
        delete object;
    }

    // Attached property
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nItem {\n"
                                "Accessible.role: Accessible.Button\n"
                                "Accessible.name: \"Donald\"\n"
                                "Accessible.description: \"Duck\"\n"
                                "}", QUrl());
        QObject *object = component.create();
        QVERIFY(object != 0);

        QObject *attachedObject = QQuickAccessibleAttached::attachedProperties(object);
        QVERIFY(attachedObject);
        if (attachedObject) {
            QVariant p = attachedObject->property("role");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toInt(), int(QAccessible::PushButton));
            p = attachedObject->property("name");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("Donald"));
            p = attachedObject->property("description");
            QCOMPARE(p.isNull(), false);
            QCOMPARE(p.toString(), QLatin1String("Duck"));
        }
        delete object;
    }
}


void tst_QQuickAccessible::basicPropertiesTest()
{
    QAccessibleInterface *app = QAccessible::queryAccessibleInterface(qApp);
    QCOMPARE(app->childCount(), 0);

    QQuickView *window = new QQuickView();
    window->setSource(testFileUrl("statictext.qml"));
    window->show();
    QCOMPARE(app->childCount(), 1);

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(window);
    QVERIFY(iface);
    QCOMPARE(iface->childCount(), 1);

    QAccessibleInterface *item = iface->child(0);
    QVERIFY(item);
    QCOMPARE(item->childCount(), 2);
    QCOMPARE(item->rect().size(), QSize(400, 400));
    QCOMPARE(item->role(), QAccessible::Pane);
    QCOMPARE(iface->indexOfChild(item), 0);

    QAccessibleInterface *text = item->child(0);
    QVERIFY(text);
    QCOMPARE(text->childCount(), 0);

    QCOMPARE(text->text(QAccessible::Name), QLatin1String("Hello Accessibility"));
    QCOMPARE(text->rect().size(), QSize(200, 50));
    QCOMPARE(text->rect().x(), item->rect().x() + 100);
    QCOMPARE(text->rect().y(), item->rect().y() + 20);
    QCOMPARE(text->role(), QAccessible::StaticText);
    QCOMPARE(item->indexOfChild(text), 0);

    QAccessibleInterface *text2 = item->child(1);
    QVERIFY(text2);
    QCOMPARE(text2->childCount(), 0);

    QCOMPARE(text2->text(QAccessible::Name), QLatin1String("The Hello 2 accessible text"));
    QCOMPARE(text2->rect().size(), QSize(100, 40));
    QCOMPARE(text2->rect().x(), item->rect().x() + 100);
    QCOMPARE(text2->rect().y(), item->rect().y() + 40);
    QCOMPARE(text2->role(), QAccessible::StaticText);
    QCOMPARE(item->indexOfChild(text2), 1);

    QCOMPARE(iface->indexOfChild(text2), -1);
    QCOMPARE(text2->indexOfChild(item), -1);

    delete window;
}

QAccessibleInterface *topLevelChildAt(QAccessibleInterface *iface, int x, int y)
{
    QAccessibleInterface *child = iface->childAt(x, y);
    if (!child)
        return 0;

    QAccessibleInterface *childOfChild;
    while (childOfChild = child->childAt(x, y)) {
        child = childOfChild;
    }
    return child;
}

void tst_QQuickAccessible::hitTest()
{
    QQuickView *window = new QQuickView;
    window->setSource(testFileUrl("hittest.qml"));
    window->show();

    QAccessibleInterface *windowIface = QAccessible::queryAccessibleInterface(window);
    QVERIFY(windowIface);
    QAccessibleInterface *rootItem = windowIface->child(0);
    QRect rootRect = rootItem->rect();

    // check the root item from app
    QAccessibleInterface *appIface = QAccessible::queryAccessibleInterface(qApp);
    QVERIFY(appIface);
    QAccessibleInterface *itemHit(appIface->childAt(rootRect.x() + 200, rootRect.y() + 50));
    QVERIFY(itemHit);
    QCOMPARE(rootRect, itemHit->rect());

    // hit rect1
    QAccessibleInterface *rect1(rootItem->child(0));
    QRect rect1Rect = rect1->rect();
    QAccessibleInterface *rootItemIface = rootItem->childAt(rect1Rect.x() + 10, rect1Rect.y() + 10);
    QVERIFY(rootItemIface);
    QCOMPARE(rect1Rect, rootItemIface->rect());
    QCOMPARE(rootItemIface->text(QAccessible::Name), QLatin1String("rect1"));

    // should also work from top level (app)
    QAccessibleInterface *app(QAccessible::queryAccessibleInterface(qApp));
    QAccessibleInterface *itemHit2(topLevelChildAt(app, rect1Rect.x() + 10, rect1Rect.y() + 10));
    QVERIFY(itemHit2);
    QCOMPARE(itemHit2->rect(), rect1Rect);
    QCOMPARE(itemHit2->text(QAccessible::Name), QLatin1String("rect1"));

    // hit rect201
    QAccessibleInterface *rect2(rootItem->child(1));
    QVERIFY(rect2);
    // FIXME: This is seems broken on mac
    // QCOMPARE(rect2->rect().translated(rootItem->rect().x(), rootItem->rect().y()), QRect(0, 50, 100, 100));
    QAccessibleInterface *rect20(rect2->child(0));
    QVERIFY(rect20);
    QAccessibleInterface *rect201(rect20->child(1));
    QVERIFY(rect201);

    QRect rect201Rect = rect201->rect();
    rootItemIface = windowIface->childAt(rect201Rect.x() + 20, rect201Rect.y() + 20);
    QVERIFY(rootItemIface);
    QCOMPARE(rootItemIface->rect(), rect201Rect);
    QCOMPARE(rootItemIface->text(QAccessible::Name), QLatin1String("rect201"));

    delete window;
}

void tst_QQuickAccessible::checkableTest()
{
    QQuickView *window = new QQuickView();
    window->setSource(testFileUrl("checkbuttons.qml"));
    window->show();

    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(window);
    QVERIFY(iface);
    QAccessibleInterface *root = iface->child(0);

    QAccessibleInterface *button1 = root->child(0);
    QCOMPARE(button1->role(), QAccessible::Button);
    QVERIFY(!(button1->state().checked));
    QVERIFY(!(button1->state().checkable));

    QAccessibleInterface *button2 = root->child(1);
    QVERIFY(!(button2->state().checked));
    QVERIFY(button2->state().checkable);

    QAccessibleInterface *button3 = root->child(2);
    QVERIFY(button3->state().checked);
    QVERIFY(button3->state().checkable);

    QAccessibleInterface *checkBox1 = root->child(3);
    QCOMPARE(checkBox1->role(), QAccessible::CheckBox);
    QVERIFY((checkBox1->state().checked));
    QVERIFY(checkBox1->state().checkable);

    QAccessibleInterface *checkBox2 = root->child(4);
    QVERIFY(!(checkBox2->state().checked));
    QVERIFY(checkBox2->state().checkable);
}

QTEST_MAIN(tst_QQuickAccessible)

#include "tst_qquickaccessible.moc"
