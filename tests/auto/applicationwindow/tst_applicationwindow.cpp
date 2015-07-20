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
#include "../shared/util.h"
#include "../shared/visualtestutil.h"

using namespace QQuickVisualTestUtil;

class tst_applicationwindow : public QQmlDataTest
{
    Q_OBJECT
public:

private slots:
    // TODO: tests fail if a couple have already run. stack trace points to
    // QQuickThemeData construction (possibly something to do with it being
    // global static?) commenting out color: Theme.backgroundColor in
    // ApplicationWindow.qml is a workaround.
//    void qmlCreation();
//    void activeFocusOnTab1();
//    void activeFocusOnTab2();
//    void defaultFocus();
    void implicitFill();
};

//void tst_applicationwindow::qmlCreation()
//{
//    QQmlEngine engine;
//    QQmlComponent component(&engine);
//    component.loadUrl(testFileUrl("basicapplicationwindow.qml"));
//    QObject* created = component.create();
//    QScopedPointer<QObject> cleanup(created);
//    QVERIFY(created);

//    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
//    QVERIFY(window);
//    QVERIFY(!window->isVisible());

//    QCOMPARE(created->property("title"), QVariant("Test Application Window"));

//    QQuickItem* statusBar = qvariant_cast<QQuickItem*>(created->property("statusBar"));
//    QVERIFY(!statusBar);

//    QQuickItem* header = qvariant_cast<QQuickItem*>(created->property("header"));
//    QVERIFY(!header);

//    QQuickItem* footer = qvariant_cast<QQuickItem*>(created->property("footer"));
//    QVERIFY(!footer);
//}

//void tst_applicationwindow::activeFocusOnTab1()
//{
//    QQmlEngine engine;
//    QQmlComponent component(&engine);
//    component.loadUrl(testFileUrl("activefocusontab.qml"));
//    QObject* created = component.create();
//    QScopedPointer<QObject> cleanup(created);
//    QVERIFY(created);

//    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
//    QVERIFY(window);
//    window->show();
//    window->requestActivate();
//    QVERIFY(QTest::qWaitForWindowActive(window));
//    QVERIFY(QGuiApplication::focusWindow() == window);

//    QQuickItem* contentItem = window->contentItem();
//    QVERIFY(contentItem);
//    QVERIFY(contentItem->hasActiveFocus());

//    QQuickItem* item = findItem<QQuickItem>(window->contentItem(), "sub1");
//    QVERIFY(item);
//    QVERIFY(!item->hasActiveFocus());

//    // Tab: contentItem->sub1
//    QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
//    QGuiApplication::sendEvent(window, &key);
//    QVERIFY(key.isAccepted());

//    item = findItem<QQuickItem>(window->contentItem(), "sub1");
//    QVERIFY(item);
//    QVERIFY(item->hasActiveFocus());

//    // Tab: sub1->sub2
//    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
//    QGuiApplication::sendEvent(window, &key);
//    QVERIFY(key.isAccepted());

//    item = findItem<QQuickItem>(window->contentItem(), "sub2");
//    QVERIFY(item);
//    QVERIFY(item->hasActiveFocus());

//    // Tab: sub2->sub1
//    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
//    QGuiApplication::sendEvent(window, &key);
//    QVERIFY(key.isAccepted());

//    item = findItem<QQuickItem>(window->contentItem(), "sub1");
//    QVERIFY(item);
//    QVERIFY(item->hasActiveFocus());
//}

//void tst_applicationwindow::activeFocusOnTab2()
//{
//    QQmlEngine engine;
//    QQmlComponent component(&engine);
//    component.loadUrl(testFileUrl("activefocusontab.qml"));
//    QObject* created = component.create();
//    QScopedPointer<QObject> cleanup(created);
//    QVERIFY(created);

//    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
//    QVERIFY(window);
//    window->show();
//    window->requestActivate();
//    QVERIFY(QTest::qWaitForWindowActive(window));
//    QVERIFY(QGuiApplication::focusWindow() == window);

//    QQuickItem* contentItem = window->contentItem();
//    QVERIFY(contentItem);
//    QVERIFY(contentItem->hasActiveFocus());

//    QQuickItem* item = findItem<QQuickItem>(window->contentItem(), "sub2");
//    QVERIFY(item);
//    QVERIFY(!item->hasActiveFocus());

//    // BackTab: contentItem->sub2
//    QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
//    QGuiApplication::sendEvent(window, &key);
//    QVERIFY(key.isAccepted());

//    item = findItem<QQuickItem>(window->contentItem(), "sub2");
//    QVERIFY(item);
//    QVERIFY(item->hasActiveFocus());

//    // BackTab: sub2->sub1
//    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
//    QGuiApplication::sendEvent(window, &key);
//    QVERIFY(key.isAccepted());

//    item = findItem<QQuickItem>(window->contentItem(), "sub1");
//    QVERIFY(item);
//    QVERIFY(item->hasActiveFocus());

//    // BackTab: sub1->sub2
//    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
//    QGuiApplication::sendEvent(window, &key);
//    QVERIFY(key.isAccepted());

//    item = findItem<QQuickItem>(window->contentItem(), "sub2");
//    QVERIFY(item);
//    QVERIFY(item->hasActiveFocus());
//}

//void tst_applicationwindow::defaultFocus()
//{
//    QQmlEngine engine;
//    QQmlComponent component(&engine);
//    component.loadUrl(testFileUrl("defaultFocus.qml"));
//    QObject* created = component.create();
//    QScopedPointer<QObject> cleanup(created);
//    Q_UNUSED(cleanup);
//    QVERIFY(created);

//    QQuickWindow* window = qobject_cast<QQuickWindow*>(created);
//    QVERIFY(window);
//    window->show();
//    window->requestActivate();
//    QVERIFY(QTest::qWaitForWindowActive(window));
//    QVERIFY(QGuiApplication::focusWindow() == window);

//    QQuickItem* contentItem = window->contentItem();
//    QVERIFY(contentItem);
//    QVERIFY(contentItem->hasActiveFocus());

//    // A single item in an ApplicationWindow with focus: true should receive focus.
//    QQuickItem* item = findItem<QQuickItem>(window->contentItem(), "item");
//    QVERIFY(item);
//    QVERIFY(item->hasFocus());
//    QVERIFY(item->hasActiveFocus());
//}

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
    QEXPECT_FAIL("", "QTBUG-47318", Continue);
    QCOMPARE(nextItem->width(), 400.0);
    QEXPECT_FAIL("", "QTBUG-47318", Continue);
    QCOMPARE(nextItem->height(), 400.0);
}

QTEST_MAIN(tst_applicationwindow)

#include "tst_applicationwindow.moc"
