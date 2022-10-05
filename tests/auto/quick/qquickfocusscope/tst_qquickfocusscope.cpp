// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QSignalSpy>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <private/qquicktextedit_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickfocusscope_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

using namespace QQuickVisualTestUtils;

class tst_qquickfocusscope : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickfocusscope() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void basic();
    void nested();
    void noFocus();
    void textEdit();
    void forceFocus();
    void noParentFocus();
    void signalEmission();
    void qtBug13380();
    void forceActiveFocus();
    void canvasFocus();
};

void tst_qquickfocusscope::basic()
{
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("test.qml"));

    QQuickFocusScope *item0 = findItem<QQuickFocusScope>(view->rootObject(), QLatin1String("item0"));
    QQuickRectangle *item1 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item1"));
    QQuickRectangle *item2 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item2"));
    QQuickRectangle *item3 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item3"));
    QVERIFY(item0 != nullptr);
    QVERIFY(item1 != nullptr);
    QVERIFY(item2 != nullptr);
    QVERIFY(item3 != nullptr);

    view->show();
    view->requestActivate();

    QVERIFY(QTest::qWaitForWindowActive(view));
    QTRY_COMPARE(view, qGuiApp->focusWindow());

    QVERIFY(view->isTopLevel());
    QVERIFY(item0->hasActiveFocus());
    QVERIFY(item1->hasActiveFocus());
    QVERIFY(!item2->hasActiveFocus());
    QVERIFY(!item3->hasActiveFocus());

    QTest::keyClick(view, Qt::Key_Right);
    QTest::qWait(50);
    QVERIFY(item0->hasActiveFocus());
    QVERIFY(!item1->hasActiveFocus());
    QVERIFY(item2->hasActiveFocus());
    QVERIFY(!item3->hasActiveFocus());

    QTest::keyClick(view, Qt::Key_Down);
    QTest::qWait(50);
    QVERIFY(!item0->hasActiveFocus());
    QVERIFY(!item1->hasActiveFocus());
    QVERIFY(!item2->hasActiveFocus());
    QVERIFY(item3->hasActiveFocus());

    delete view;
}

void tst_qquickfocusscope::nested()
{
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("test2.qml"));

    QQuickFocusScope *item1 = findItem<QQuickFocusScope>(view->rootObject(), QLatin1String("item1"));
    QQuickFocusScope *item2 = findItem<QQuickFocusScope>(view->rootObject(), QLatin1String("item2"));
    QQuickFocusScope *item3 = findItem<QQuickFocusScope>(view->rootObject(), QLatin1String("item3"));
    QQuickFocusScope *item4 = findItem<QQuickFocusScope>(view->rootObject(), QLatin1String("item4"));
    QQuickFocusScope *item5 = findItem<QQuickFocusScope>(view->rootObject(), QLatin1String("item5"));
    QVERIFY(item1 != nullptr);
    QVERIFY(item2 != nullptr);
    QVERIFY(item3 != nullptr);
    QVERIFY(item4 != nullptr);
    QVERIFY(item5 != nullptr);

    view->show();
    view->requestActivate();

    QVERIFY(QTest::qWaitForWindowActive(view));
    QTRY_COMPARE(view, qGuiApp->focusWindow());

    QVERIFY(item1->hasActiveFocus());
    QVERIFY(item2->hasActiveFocus());
    QVERIFY(item3->hasActiveFocus());
    QVERIFY(item4->hasActiveFocus());
    QVERIFY(item5->hasActiveFocus());
    delete view;
}

void tst_qquickfocusscope::noFocus()
{
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("test4.qml"));

    QQuickRectangle *item0 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item0"));
    QQuickRectangle *item1 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item1"));
    QQuickRectangle *item2 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item2"));
    QQuickRectangle *item3 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item3"));
    QVERIFY(item0 != nullptr);
    QVERIFY(item1 != nullptr);
    QVERIFY(item2 != nullptr);
    QVERIFY(item3 != nullptr);

    view->show();
    view->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(view));
    QCOMPARE(view, qGuiApp->focusWindow());

    QVERIFY(!item0->hasActiveFocus());
    QVERIFY(!item1->hasActiveFocus());
    QVERIFY(!item2->hasActiveFocus());
    QVERIFY(!item3->hasActiveFocus());

    QTest::keyClick(view, Qt::Key_Right);
    QVERIFY(!item0->hasActiveFocus());
    QVERIFY(!item1->hasActiveFocus());
    QVERIFY(!item2->hasActiveFocus());
    QVERIFY(!item3->hasActiveFocus());

    QTest::keyClick(view, Qt::Key_Down);
    QVERIFY(!item0->hasActiveFocus());
    QVERIFY(!item1->hasActiveFocus());
    QVERIFY(!item2->hasActiveFocus());
    QVERIFY(!item3->hasActiveFocus());

    delete view;
}

void tst_qquickfocusscope::textEdit()
{
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("test5.qml"));

    QQuickFocusScope *item0 = findItem<QQuickFocusScope>(view->rootObject(), QLatin1String("item0"));
    QQuickTextEdit *item1 = findItem<QQuickTextEdit>(view->rootObject(), QLatin1String("item1"));
    QQuickRectangle *item2 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item2"));
    QQuickTextEdit *item3 = findItem<QQuickTextEdit>(view->rootObject(), QLatin1String("item3"));
    QVERIFY(item0 != nullptr);
    QVERIFY(item1 != nullptr);
    QVERIFY(item2 != nullptr);
    QVERIFY(item3 != nullptr);

    view->show();
    view->requestActivate();

    QVERIFY(QTest::qWaitForWindowActive(view));

    QTRY_COMPARE(view, qGuiApp->focusWindow());
    QVERIFY(item0->hasActiveFocus());
    QVERIFY(item1->hasActiveFocus());
    QVERIFY(!item2->hasActiveFocus());
    QVERIFY(!item3->hasActiveFocus());

    QTest::keyClick(view, Qt::Key_Right);
    QVERIFY(item0->hasActiveFocus());
    QVERIFY(item1->hasActiveFocus());
    QVERIFY(!item2->hasActiveFocus());
    QVERIFY(!item3->hasActiveFocus());

    QTest::keyClick(view, Qt::Key_Right);
    QTest::keyClick(view, Qt::Key_Right);
    QTest::keyClick(view, Qt::Key_Right);
    QTest::keyClick(view, Qt::Key_Right);
    QTest::keyClick(view, Qt::Key_Right);
    QVERIFY(item0->hasActiveFocus());
    QVERIFY(!item1->hasActiveFocus());
    QVERIFY(item2->hasActiveFocus());
    QVERIFY(!item3->hasActiveFocus());

    QTest::keyClick(view, Qt::Key_Down);
    QVERIFY(!item0->hasActiveFocus());
    QVERIFY(!item1->hasActiveFocus());
    QVERIFY(!item2->hasActiveFocus());
    QVERIFY(item3->hasActiveFocus());

    delete view;
}

void tst_qquickfocusscope::forceFocus()
{
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("forcefocus.qml"));

    QQuickFocusScope *item0 = findItem<QQuickFocusScope>(view->rootObject(), QLatin1String("item0"));
    QQuickRectangle *item1 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item1"));
    QQuickRectangle *item2 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item2"));
    QQuickFocusScope *item3 = findItem<QQuickFocusScope>(view->rootObject(), QLatin1String("item3"));
    QQuickRectangle *item4 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item4"));
    QQuickRectangle *item5 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item5"));
    QVERIFY(item0 != nullptr);
    QVERIFY(item1 != nullptr);
    QVERIFY(item2 != nullptr);
    QVERIFY(item3 != nullptr);
    QVERIFY(item4 != nullptr);
    QVERIFY(item5 != nullptr);

    view->show();
    view->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(view));
    QTRY_COMPARE(view, qGuiApp->focusWindow());

    QVERIFY(item0->hasActiveFocus());
    QVERIFY(item1->hasActiveFocus());
    QVERIFY(!item2->hasActiveFocus());
    QVERIFY(!item3->hasActiveFocus());
    QVERIFY(!item4->hasActiveFocus());
    QVERIFY(!item5->hasActiveFocus());

    QTest::keyClick(view, Qt::Key_4);
    QVERIFY(item0->hasActiveFocus());
    QVERIFY(item1->hasActiveFocus());
    QVERIFY(!item2->hasActiveFocus());
    QVERIFY(!item3->hasActiveFocus());
    QVERIFY(!item4->hasActiveFocus());
    QVERIFY(!item5->hasActiveFocus());

    QTest::keyClick(view, Qt::Key_5);
    QVERIFY(!item0->hasActiveFocus());
    QVERIFY(!item1->hasActiveFocus());
    QVERIFY(!item2->hasActiveFocus());
    QVERIFY(item3->hasActiveFocus());
    QVERIFY(!item4->hasActiveFocus());
    QVERIFY(item5->hasActiveFocus());

    delete view;
}

void tst_qquickfocusscope::noParentFocus()
{
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("chain.qml"));
    QVERIFY(view->rootObject());

    view->show();
    view->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(view));
    QTRY_COMPARE(view, qGuiApp->focusWindow());

    QVERIFY(!view->rootObject()->property("focus1").toBool());
    QVERIFY(!view->rootObject()->property("focus2").toBool());
    QVERIFY(view->rootObject()->property("focus3").toBool());
    QVERIFY(view->rootObject()->property("focus4").toBool());
    QVERIFY(view->rootObject()->property("focus5").toBool());

    delete view;
}

void tst_qquickfocusscope::signalEmission()
{
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("signalEmission.qml"));

    QQuickRectangle *item1 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item1"));
    QQuickRectangle *item2 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item2"));
    QQuickRectangle *item3 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item3"));
    QQuickRectangle *item4 = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("item4"));
    QVERIFY(item1 != nullptr);
    QVERIFY(item2 != nullptr);
    QVERIFY(item3 != nullptr);
    QVERIFY(item4 != nullptr);

    view->show();
    view->requestActivate();

    QVERIFY(QTest::qWaitForWindowActive(view));
    QTRY_COMPARE(view, qGuiApp->focusWindow());

    QVariant blue(QColor("blue"));
    QVariant red(QColor("red"));

    item1->setFocus(true);
    QCOMPARE(item1->property("color"), red);
    QCOMPARE(item2->property("color"), blue);
    QCOMPARE(item3->property("color"), blue);
    QCOMPARE(item4->property("color"), blue);

    item2->setFocus(true);
    QCOMPARE(item1->property("color"), blue);
    QCOMPARE(item2->property("color"), red);
    QCOMPARE(item3->property("color"), blue);
    QCOMPARE(item4->property("color"), blue);

    item3->setFocus(true);
    QCOMPARE(item1->property("color"), blue);
    QCOMPARE(item2->property("color"), red);
    QCOMPARE(item3->property("color"), red);
    QCOMPARE(item4->property("color"), blue);

    item4->setFocus(true);
    QCOMPARE(item1->property("color"), blue);
    QCOMPARE(item2->property("color"), red);
    QCOMPARE(item3->property("color"), blue);
    QCOMPARE(item4->property("color"), red);

    item4->setFocus(false);
    QCOMPARE(item1->property("color"), blue);
    QCOMPARE(item2->property("color"), red);
    QCOMPARE(item3->property("color"), blue);
    QCOMPARE(item4->property("color"), blue);

    delete view;
}

void tst_qquickfocusscope::qtBug13380()
{
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("qtBug13380.qml"));

    view->show();
    QVERIFY(view->rootObject());
    view->requestActivate();
    qApp->processEvents();

    QVERIFY(QTest::qWaitForWindowExposed(view));

    QTRY_COMPARE(view, qGuiApp->focusWindow());
    QVERIFY(view->rootObject()->property("noFocus").toBool());

    view->rootObject()->setProperty("showRect", true);
    QVERIFY(view->rootObject()->property("noFocus").toBool());

    delete view;
}

void tst_qquickfocusscope::forceActiveFocus()
{
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("forceActiveFocus.qml"));

    view->show();
    view->requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(view));
    QTRY_COMPARE(view, qGuiApp->focusWindow());

    QQuickItem *rootObject = view->rootObject();
    QVERIFY(rootObject);

    QQuickItem *scope = findItem<QQuickItem>(rootObject, QLatin1String("scope"));
    QQuickItem *itemA1 = findItem<QQuickItem>(rootObject, QLatin1String("item-a1"));
    QQuickItem *scopeA = findItem<QQuickItem>(rootObject, QLatin1String("scope-a"));
    QQuickItem *itemA2 = findItem<QQuickItem>(rootObject, QLatin1String("item-a2"));
    QQuickItem *itemB1 = findItem<QQuickItem>(rootObject, QLatin1String("item-b1"));
    QQuickItem *scopeB = findItem<QQuickItem>(rootObject, QLatin1String("scope-b"));
    QQuickItem *itemB2 = findItem<QQuickItem>(rootObject, QLatin1String("item-b2"));

    QVERIFY(scope);
    QVERIFY(itemA1);
    QVERIFY(scopeA);
    QVERIFY(itemA2);
    QVERIFY(itemB1);
    QVERIFY(scopeB);
    QVERIFY(itemB2);

    QSignalSpy rootSpy(rootObject, SIGNAL(activeFocusChanged(bool)));
    QSignalSpy scopeSpy(scope, SIGNAL(activeFocusChanged(bool)));
    QSignalSpy scopeASpy(scopeA, SIGNAL(activeFocusChanged(bool)));
    QSignalSpy scopeBSpy(scopeB, SIGNAL(activeFocusChanged(bool)));

    // First, walk the focus from item-a1 down to item-a2 and back again
    itemA1->forceActiveFocus();
    QVERIFY(itemA1->hasActiveFocus());
    QVERIFY(!rootObject->hasActiveFocus());
    QCOMPARE(rootSpy.size(), 0);
    QCOMPARE(scopeSpy.size(), 1);

    scopeA->forceActiveFocus();
    QVERIFY(!itemA1->hasActiveFocus());
    QVERIFY(scopeA->hasActiveFocus());
    QCOMPARE(scopeASpy.size(), 1);
    QCOMPARE(rootSpy.size(), 0);
    QCOMPARE(scopeSpy.size(), 1);

    itemA2->forceActiveFocus();
    QVERIFY(!itemA1->hasActiveFocus());
    QVERIFY(itemA2->hasActiveFocus());
    QVERIFY(scopeA->hasActiveFocus());
    QCOMPARE(scopeASpy.size(), 1);
    QCOMPARE(rootSpy.size(), 0);
    QCOMPARE(scopeSpy.size(), 1);

    scopeA->forceActiveFocus();
    QVERIFY(!itemA1->hasActiveFocus());
    QVERIFY(itemA2->hasActiveFocus());
    QVERIFY(scopeA->hasActiveFocus());
    QCOMPARE(scopeASpy.size(), 1);
    QCOMPARE(rootSpy.size(), 0);
    QCOMPARE(scopeSpy.size(), 1);

    itemA1->forceActiveFocus();
    QVERIFY(itemA1->hasActiveFocus());
    QVERIFY(!scopeA->hasActiveFocus());
    QVERIFY(!itemA2->hasActiveFocus());
    QCOMPARE(scopeASpy.size(), 2);
    QCOMPARE(rootSpy.size(), 0);
    QCOMPARE(scopeSpy.size(), 1);

    // Then jump back and forth between branch 'a' and 'b'
    itemB1->forceActiveFocus();
    QVERIFY(itemB1->hasActiveFocus());
    QCOMPARE(rootSpy.size(), 0);
    QCOMPARE(scopeSpy.size(), 1);

    scopeA->forceActiveFocus();
    QVERIFY(!itemA1->hasActiveFocus());
    QVERIFY(!itemB1->hasActiveFocus());
    QVERIFY(scopeA->hasActiveFocus());
    QCOMPARE(scopeASpy.size(), 3);
    QCOMPARE(rootSpy.size(), 0);
    QCOMPARE(scopeSpy.size(), 1);

    scopeB->forceActiveFocus();
    QVERIFY(!scopeA->hasActiveFocus());
    QVERIFY(!itemB1->hasActiveFocus());
    QVERIFY(scopeB->hasActiveFocus());
    QCOMPARE(scopeASpy.size(), 4);
    QCOMPARE(scopeBSpy.size(), 1);
    QCOMPARE(rootSpy.size(), 0);
    QCOMPARE(scopeSpy.size(), 1);

    itemA2->forceActiveFocus();
    QVERIFY(!scopeB->hasActiveFocus());
    QVERIFY(itemA2->hasActiveFocus());
    QCOMPARE(scopeASpy.size(), 5);
    QCOMPARE(scopeBSpy.size(), 2);
    QCOMPARE(rootSpy.size(), 0);
    QCOMPARE(scopeSpy.size(), 1);

    itemB2->forceActiveFocus();
    QVERIFY(!itemA2->hasActiveFocus());
    QVERIFY(itemB2->hasActiveFocus());
    QCOMPARE(scopeASpy.size(), 6);
    QCOMPARE(scopeBSpy.size(), 3);
    QCOMPARE(rootSpy.size(), 0);
    QCOMPARE(scopeSpy.size(), 1);

    delete view;
}

void tst_qquickfocusscope::canvasFocus()
{
    QQuickView *view = new QQuickView;
    view->setSource(testFileUrl("canvasFocus.qml"));

    QQuickView alternateView;

    QQuickItem *rootObject = view->rootObject();
    QVERIFY(rootObject);

    QQuickItem *rootItem = view->contentItem();
    QQuickItem *scope1 = findItem<QQuickItem>(rootObject, QLatin1String("scope1"));
    QQuickItem *item1 = findItem<QQuickItem>(rootObject, QLatin1String("item1"));
    QQuickItem *scope2 = findItem<QQuickItem>(rootObject, QLatin1String("scope2"));
    QQuickItem *item2 = findItem<QQuickItem>(rootObject, QLatin1String("item2"));

    QVERIFY(scope1);
    QVERIFY(item1);
    QVERIFY(scope2);
    QVERIFY(item2);

    QSignalSpy rootFocusSpy(rootItem, SIGNAL(focusChanged(bool)));
    QSignalSpy scope1FocusSpy(scope1, SIGNAL(focusChanged(bool)));
    QSignalSpy item1FocusSpy(item1, SIGNAL(focusChanged(bool)));
    QSignalSpy scope2FocusSpy(scope2, SIGNAL(focusChanged(bool)));
    QSignalSpy item2FocusSpy(item2, SIGNAL(focusChanged(bool)));
    QSignalSpy rootActiveFocusSpy(rootItem, SIGNAL(activeFocusChanged(bool)));
    QSignalSpy scope1ActiveFocusSpy(scope1, SIGNAL(activeFocusChanged(bool)));
    QSignalSpy item1ActiveFocusSpy(item1, SIGNAL(activeFocusChanged(bool)));
    QSignalSpy scope2ActiveFocusSpy(scope2, SIGNAL(activeFocusChanged(bool)));
    QSignalSpy item2ActiveFocusSpy(item2, SIGNAL(activeFocusChanged(bool)));

    QCOMPARE(rootItem->hasFocus(), false);
    QCOMPARE(rootItem->hasActiveFocus(), false);
    QCOMPARE(scope1->hasFocus(), true);
    QCOMPARE(scope1->hasActiveFocus(), false);
    QCOMPARE(item1->hasFocus(), true);
    QCOMPARE(item1->hasActiveFocus(), false);
    QCOMPARE(scope2->hasFocus(), false);
    QCOMPARE(scope2->hasActiveFocus(), false);
    QCOMPARE(item2->hasFocus(), false);
    QCOMPARE(item2->hasActiveFocus(), false);

    view->show();
    view->requestActivate();

    QVERIFY(QTest::qWaitForWindowActive(view));
    QCOMPARE(view, qGuiApp->focusWindow());

    // Now the window has focus, active focus given to item1
    QCOMPARE(rootItem->hasFocus(), true);
    QCOMPARE(rootItem->hasActiveFocus(), true);
    QCOMPARE(scope1->hasFocus(), true);
    QCOMPARE(scope1->hasActiveFocus(), true);
    QCOMPARE(item1->hasFocus(), true);
    QCOMPARE(item1->hasActiveFocus(), true);
    QCOMPARE(scope2->hasFocus(), false);
    QCOMPARE(scope2->hasActiveFocus(), false);
    QCOMPARE(item2->hasFocus(), false);
    QCOMPARE(item2->hasActiveFocus(), false);

    QCOMPARE(rootFocusSpy.size(), 1);
    QCOMPARE(rootActiveFocusSpy.size(), 1);
    QCOMPARE(scope1FocusSpy.size(), 0);
    QCOMPARE(scope1ActiveFocusSpy.size(), 1);
    QCOMPARE(item1FocusSpy.size(), 0);
    QCOMPARE(item1ActiveFocusSpy.size(), 1);


    //    view->hide(); // seemingly doesn't remove focus, so have an another view steal it.
    alternateView.show();
    alternateView.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&alternateView));
    QCOMPARE(QGuiApplication::focusWindow(), &alternateView);

    QCOMPARE(rootItem->hasFocus(), false);
    QCOMPARE(rootItem->hasActiveFocus(), false);
    QCOMPARE(scope1->hasFocus(), true);
    QCOMPARE(scope1->hasActiveFocus(), false);
    QCOMPARE(item1->hasFocus(), true);
    QCOMPARE(item1->hasActiveFocus(), false);

    QCOMPARE(rootFocusSpy.size(), 2);
    QCOMPARE(rootActiveFocusSpy.size(), 2);
    QCOMPARE(scope1FocusSpy.size(), 0);
    QCOMPARE(scope1ActiveFocusSpy.size(), 2);
    QCOMPARE(item1FocusSpy.size(), 0);
    QCOMPARE(item1ActiveFocusSpy.size(), 2);


    // window does not have focus, so item2 will not get active focus
    item2->forceActiveFocus();

    QCOMPARE(rootItem->hasFocus(), false);
    QCOMPARE(rootItem->hasActiveFocus(), false);
    QCOMPARE(scope1->hasFocus(), false);
    QCOMPARE(scope1->hasActiveFocus(), false);
    QCOMPARE(item1->hasFocus(), true);
    QCOMPARE(item1->hasActiveFocus(), false);
    QCOMPARE(scope2->hasFocus(), true);
    QCOMPARE(scope2->hasActiveFocus(), false);
    QCOMPARE(item2->hasFocus(), true);
    QCOMPARE(item2->hasActiveFocus(), false);

    QCOMPARE(rootFocusSpy.size(), 2);
    QCOMPARE(rootActiveFocusSpy.size(), 2);
    QCOMPARE(scope1FocusSpy.size(), 1);
    QCOMPARE(scope1ActiveFocusSpy.size(), 2);
    QCOMPARE(item1FocusSpy.size(), 0);
    QCOMPARE(item1ActiveFocusSpy.size(), 2);
    QCOMPARE(scope2FocusSpy.size(), 1);
    QCOMPARE(scope2ActiveFocusSpy.size(), 0);
    QCOMPARE(item2FocusSpy.size(), 1);
    QCOMPARE(item2ActiveFocusSpy.size(), 0);

    // give the window focus, and item2 will get active focus
    view->show();
    view->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(view));
    QCOMPARE(QGuiApplication::focusWindow(), view);

    QCOMPARE(rootItem->hasFocus(), true);
    QCOMPARE(rootItem->hasActiveFocus(), true);
    QCOMPARE(scope2->hasFocus(), true);
    QCOMPARE(scope2->hasActiveFocus(), true);
    QCOMPARE(item2->hasFocus(), true);
    QCOMPARE(item2->hasActiveFocus(), true);
    QCOMPARE(rootFocusSpy.size(), 3);
    QCOMPARE(rootActiveFocusSpy.size(), 3);
    QCOMPARE(scope2FocusSpy.size(), 1);
    QCOMPARE(scope2ActiveFocusSpy.size(), 1);
    QCOMPARE(item2FocusSpy.size(), 1);
    QCOMPARE(item2ActiveFocusSpy.size(), 1);

    delete view;
}

QTEST_MAIN(tst_qquickfocusscope)

#include "tst_qquickfocusscope.moc"
