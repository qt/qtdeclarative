/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtLabsTemplates/private/qquickapplicationwindow_p.h>
#include "../shared/util.h"
#include "../shared/visualtestutil.h"

using namespace QQuickVisualTestUtil;

class tst_applicationwindow : public QQmlDataTest
{
    Q_OBJECT
public:

private slots:
    void qmlCreation();
    void activeFocusOnTab1();
    void activeFocusOnTab2();
    void defaultFocus();
    void implicitFill();
    void attachedProperties();
};

void tst_applicationwindow::qmlCreation()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("basicapplicationwindow.qml"));
    QObject* created = component.create();
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(created);

    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    QVERIFY(!window->isVisible());

    QCOMPARE(created->property("title"), QVariant("Test Application Window"));

    QQuickItem* statusBar = qvariant_cast<QQuickItem*>(created->property("statusBar"));
    QVERIFY(!statusBar);

    QQuickItem* header = qvariant_cast<QQuickItem*>(created->property("header"));
    QVERIFY(!header);

    QQuickItem* footer = qvariant_cast<QQuickItem*>(created->property("footer"));
    QVERIFY(!footer);
}

void tst_applicationwindow::activeFocusOnTab1()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("activefocusontab.qml"));
    QObject* created = component.create();
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(created);

    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);

    QQuickItem* contentItem = window->contentItem();
    QVERIFY(contentItem);
    QVERIFY(contentItem->hasActiveFocus());

    QQuickItem* item = findItem<QQuickItem>(window->contentItem(), "sub1");
    QVERIFY(item);
    QVERIFY(!item->hasActiveFocus());

    // Tab: contentItem->sub1
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->contentItem(), "sub1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: sub1->sub2
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->contentItem(), "sub2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Tab: sub2->sub1
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->contentItem(), "sub1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());
}

void tst_applicationwindow::activeFocusOnTab2()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("activefocusontab.qml"));
    QObject* created = component.create();
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(created);

    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);

    QQuickItem* contentItem = window->contentItem();
    QVERIFY(contentItem);
    QVERIFY(contentItem->hasActiveFocus());

    QQuickItem* item = findItem<QQuickItem>(window->contentItem(), "sub2");
    QVERIFY(item);
    QVERIFY(!item->hasActiveFocus());

    // BackTab: contentItem->sub2
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->contentItem(), "sub2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: sub2->sub1
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->contentItem(), "sub1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // BackTab: sub1->sub2
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
    QGuiApplication::sendEvent(window, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(window->contentItem(), "sub2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());
}

void tst_applicationwindow::defaultFocus()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("defaultFocus.qml"));
    QObject* created = component.create();
    QScopedPointer<QObject> cleanup(created);
    Q_UNUSED(cleanup);
    QVERIFY(created);

    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QVERIFY(QGuiApplication::focusWindow() == window);

    QQuickItem* contentItem = window->contentItem();
    QVERIFY(contentItem);
    QVERIFY(contentItem->hasActiveFocus());

    // A single item in an ApplicationWindow with focus: true should receive focus.
    QQuickItem* item = findItem<QQuickItem>(window->contentItem(), "item");
    QVERIFY(item);
    QVERIFY(item->hasFocus());
    QVERIFY(item->hasActiveFocus());
}

void tst_applicationwindow::implicitFill()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("fill.qml"));
    QObject* created = component.create();
    QScopedPointer<QObject> cleanup(created);
    QVERIFY(created);

    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
    QVERIFY(window);
    QVERIFY(!window->isVisible());
    QCOMPARE(window->width(), 400);
    QCOMPARE(window->height(), 400);

    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickItem *stackView = window->property("stackView").value<QQuickItem*>();
    QVERIFY(stackView);
    QCOMPARE(stackView->width(), 400.0);
    QCOMPARE(stackView->height(), 400.0);

    QQuickItem *nextItem = window->property("nextItem").value<QQuickItem*>();
    QVERIFY(nextItem);

    QVERIFY(QMetaObject::invokeMethod(window, "pushNextItem"));
    QCOMPARE(nextItem->width(), 400.0);
    QCOMPARE(nextItem->height(), 400.0);
}

void tst_applicationwindow::attachedProperties()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("attachedProperties.qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    QQuickApplicationWindow *window = qobject_cast<QQuickApplicationWindow *>(object.data());
    QVERIFY(window);

    QQuickItem *childItem = object->property("childItem").value<QQuickItem *>();
    QVERIFY(childItem);
    QCOMPARE(childItem->property("attached_window").value<QQuickApplicationWindow *>(), window);
    QCOMPARE(childItem->property("attached_contentItem").value<QQuickItem *>(), window->contentItem());
    QCOMPARE(childItem->property("attached_activeFocusItem").value<QQuickItem *>(), window->activeFocusItem());
    QCOMPARE(childItem->property("attached_header").value<QQuickItem *>(), window->header());
    QCOMPARE(childItem->property("attached_footer").value<QQuickItem *>(), window->footer());
    QCOMPARE(childItem->property("attached_overlay").value<QQuickItem *>(), window->overlay());

    QObject *childObject = object->property("childObject").value<QObject *>();
    QVERIFY(childObject);
    QVERIFY(!childObject->property("attached_window").value<QQuickApplicationWindow *>());
    QVERIFY(!childObject->property("attached_contentItem").value<QQuickItem *>());
    QVERIFY(!childObject->property("attached_activeFocusItem").value<QQuickItem *>());
    QVERIFY(!childObject->property("attached_header").value<QQuickItem *>());
    QVERIFY(!childObject->property("attached_footer").value<QQuickItem *>());
    QVERIFY(!childObject->property("attached_overlay").value<QQuickItem *>());

    QQuickWindow *childWindow = object->property("childWindow").value<QQuickWindow *>();
    QVERIFY(childWindow);
    QVERIFY(!childWindow->property("attached_window").value<QQuickApplicationWindow *>());
    QVERIFY(!childWindow->property("attached_contentItem").value<QQuickItem *>());
    QVERIFY(!childWindow->property("attached_activeFocusItem").value<QQuickItem *>());
    QVERIFY(!childWindow->property("attached_header").value<QQuickItem *>());
    QVERIFY(!childWindow->property("attached_footer").value<QQuickItem *>());
    QVERIFY(!childWindow->property("attached_overlay").value<QQuickItem *>());

    QQuickItem *childWindowItem = object->property("childWindowItem").value<QQuickItem *>();
    QVERIFY(childWindowItem);
    QVERIFY(!childWindowItem->property("attached_window").value<QQuickApplicationWindow *>());
    QVERIFY(!childWindowItem->property("attached_contentItem").value<QQuickItem *>());
    QVERIFY(!childWindowItem->property("attached_activeFocusItem").value<QQuickItem *>());
    QVERIFY(!childWindowItem->property("attached_header").value<QQuickItem *>());
    QVERIFY(!childWindowItem->property("attached_footer").value<QQuickItem *>());
    QVERIFY(!childWindowItem->property("attached_overlay").value<QQuickItem *>());

    QObject *childWindowObject = object->property("childWindowObject").value<QObject *>();
    QVERIFY(childWindowObject);
    QVERIFY(!childWindowObject->property("attached_window").value<QQuickApplicationWindow *>());
    QVERIFY(!childWindowObject->property("attached_contentItem").value<QQuickItem *>());
    QVERIFY(!childWindowObject->property("attached_activeFocusItem").value<QQuickItem *>());
    QVERIFY(!childWindowObject->property("attached_header").value<QQuickItem *>());
    QVERIFY(!childWindowObject->property("attached_footer").value<QQuickItem *>());
    QVERIFY(!childWindowObject->property("attached_overlay").value<QQuickItem *>());

    QQuickApplicationWindow *childAppWindow = object->property("childAppWindow").value<QQuickApplicationWindow *>();
    QVERIFY(childAppWindow);
    QVERIFY(!childAppWindow->property("attached_window").value<QQuickApplicationWindow *>());
    QVERIFY(!childAppWindow->property("attached_contentItem").value<QQuickItem *>());
    QVERIFY(!childAppWindow->property("attached_activeFocusItem").value<QQuickItem *>());
    QVERIFY(!childAppWindow->property("attached_header").value<QQuickItem *>());
    QVERIFY(!childAppWindow->property("attached_footer").value<QQuickItem *>());
    QVERIFY(!childAppWindow->property("attached_overlay").value<QQuickItem *>());

    QQuickItem *childAppWindowItem = object->property("childAppWindowItem").value<QQuickItem *>();
    QVERIFY(childAppWindowItem);
    QCOMPARE(childAppWindowItem->property("attached_window").value<QQuickApplicationWindow *>(), childAppWindow);
    QCOMPARE(childAppWindowItem->property("attached_contentItem").value<QQuickItem *>(), childAppWindow->contentItem());
    QCOMPARE(childAppWindowItem->property("attached_activeFocusItem").value<QQuickItem *>(), childAppWindow->activeFocusItem());
    QCOMPARE(childAppWindowItem->property("attached_header").value<QQuickItem *>(), childAppWindow->header());
    QCOMPARE(childAppWindowItem->property("attached_footer").value<QQuickItem *>(), childAppWindow->footer());
    QCOMPARE(childAppWindowItem->property("attached_overlay").value<QQuickItem *>(), childAppWindow->overlay());

    QObject *childAppWindowObject = object->property("childAppWindowObject").value<QObject *>();
    QVERIFY(childAppWindowObject);
    QVERIFY(!childAppWindowObject->property("attached_window").value<QQuickApplicationWindow *>());
    QVERIFY(!childAppWindowObject->property("attached_contentItem").value<QQuickItem *>());
    QVERIFY(!childAppWindowObject->property("attached_activeFocusItem").value<QQuickItem *>());
    QVERIFY(!childAppWindowObject->property("attached_header").value<QQuickItem *>());
    QVERIFY(!childAppWindowObject->property("attached_footer").value<QQuickItem *>());
    QVERIFY(!childAppWindowObject->property("attached_overlay").value<QQuickItem *>());

    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QVERIFY(!childItem->hasActiveFocus());
    childItem->forceActiveFocus();
    QTRY_VERIFY(childItem->hasActiveFocus());
    QCOMPARE(window->activeFocusItem(), childItem);
    QCOMPARE(childItem->property("attached_activeFocusItem").value<QQuickItem *>(), childItem);

    QQuickItem *header = new QQuickItem;
    window->setHeader(header);
    QCOMPARE(window->header(), header);
    QCOMPARE(childItem->property("attached_header").value<QQuickItem *>(), header);

    QQuickItem *footer = new QQuickItem;
    window->setFooter(footer);
    QCOMPARE(window->footer(), footer);
    QCOMPARE(childItem->property("attached_footer").value<QQuickItem *>(), footer);

    childAppWindow->show();
    childAppWindow->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(childAppWindow));

    QVERIFY(!childAppWindowItem->hasActiveFocus());
    childAppWindowItem->forceActiveFocus();
    QTRY_VERIFY(childAppWindowItem->hasActiveFocus());
    QCOMPARE(childAppWindow->activeFocusItem(), childAppWindowItem);
    QCOMPARE(childAppWindowItem->property("attached_activeFocusItem").value<QQuickItem *>(), childAppWindowItem);

    childItem->setParentItem(childAppWindow->contentItem());
    QCOMPARE(childItem->window(), childAppWindow);
    QCOMPARE(childItem->property("attached_window").value<QQuickApplicationWindow *>(), childAppWindow);
    QCOMPARE(childItem->property("attached_contentItem").value<QQuickItem *>(), childAppWindow->contentItem());
    QCOMPARE(childItem->property("attached_activeFocusItem").value<QQuickItem *>(), childAppWindow->activeFocusItem());
    QCOMPARE(childItem->property("attached_header").value<QQuickItem *>(), childAppWindow->header());
    QCOMPARE(childItem->property("attached_footer").value<QQuickItem *>(), childAppWindow->footer());
    QCOMPARE(childItem->property("attached_overlay").value<QQuickItem *>(), childAppWindow->overlay());

    childItem->setParentItem(Q_NULLPTR);
    QVERIFY(!childItem->window());
    QVERIFY(!childItem->property("attached_window").value<QQuickApplicationWindow *>());
    QVERIFY(!childItem->property("attached_contentItem").value<QQuickItem *>());
    QVERIFY(!childItem->property("attached_activeFocusItem").value<QQuickItem *>());
    QVERIFY(!childItem->property("attached_header").value<QQuickItem *>());
    QVERIFY(!childItem->property("attached_footer").value<QQuickItem *>());
    QVERIFY(!childItem->property("attached_overlay").value<QQuickItem *>());
}

QTEST_MAIN(tst_applicationwindow)

#include "tst_applicationwindow.moc"
