/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtTest/QtTest>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickitemview_p_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickTest/QtQuickTest>
#include <QStringListModel>
#include <QQmlApplicationEngine>

#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

using namespace QQuickViewTestUtils;
using namespace QQuickVisualTestUtils;

class tst_QQuickListView2 : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickListView2();

private slots:
    void urlListModel();
    void dragDelegateWithMouseArea_data();
    void dragDelegateWithMouseArea();
    void delegateChooserEnumRole();
    void QTBUG_92809();
    void footerUpdate();
    void singletonModelLifetime();
    void delegateModelRefresh();

    void sectionsNoOverlap();
    void metaSequenceAsModel();
    void noCrashOnIndexChange();
    void innerRequired();
    void boundDelegateComponent();
};

tst_QQuickListView2::tst_QQuickListView2()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickListView2::urlListModel()
{
    QScopedPointer<QQuickView> window(createView());
    QVERIFY(window);

    QList<QUrl> model = { QUrl::fromLocalFile("abc"), QUrl::fromLocalFile("123") };
    window->setInitialProperties({{ "model", QVariant::fromValue(model) }});

    window->setSource(testFileUrl("urlListModel.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickListView *view = window->rootObject()->property("view").value<QQuickListView*>();
    QVERIFY(view);
    if (QQuickTest::qIsPolishScheduled(view))
        QVERIFY(QQuickTest::qWaitForItemPolished(view));
    QCOMPARE(view->count(), model.size());
}

static void dragListView(QWindow *window, QPoint *startPos, const QPoint &delta)
{
    auto drag_helper = [&](QWindow *window, QPoint *startPos, const QPoint &d) {
        QPoint pos = *startPos;
        const int dragDistance = d.manhattanLength();
        const QPoint unitVector(qBound(-1, d.x(), 1), qBound(-1, d.y(), 1));
        for (int i = 0; i < dragDistance; ++i) {
            QTest::mouseMove(window, pos);
            pos += unitVector;
        }
        // Move to the final position
        pos = *startPos + d;
        QTest::mouseMove(window, pos);
        *startPos = pos;
    };

    if (delta.manhattanLength() == 0)
        return;
    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    const QPoint unitVector(qBound(-1, delta.x(), 1), qBound(-1, delta.y(), 1));
    // go just beyond the drag theshold
    drag_helper(window, startPos, unitVector * (dragThreshold + 1));
    drag_helper(window, startPos, unitVector);

    // next drag will actually scroll the listview
    drag_helper(window, startPos, delta);
}

void tst_QQuickListView2::dragDelegateWithMouseArea_data()
{
    QTest::addColumn<QQuickItemView::LayoutDirection>("layoutDirection");

    for (int layDir = QQuickItemView::LeftToRight; layDir <= (int)QQuickItemView::VerticalBottomToTop; layDir++) {
        const char *enumValueName = QMetaEnum::fromType<QQuickItemView::LayoutDirection>().valueToKey(layDir);
        QTest::newRow(enumValueName) << static_cast<QQuickItemView::LayoutDirection>(layDir);
    }
}

void tst_QQuickListView2::dragDelegateWithMouseArea()
{
    QFETCH(QQuickItemView::LayoutDirection, layoutDirection);

    QScopedPointer<QQuickView> window(createView());
    QVERIFY(window);
    window->setFlag(Qt::FramelessWindowHint);
    window->setSource(testFileUrl("delegateWithMouseArea.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickListView *listview = findItem<QQuickListView>(window->rootObject(), "list");
    QVERIFY(listview != nullptr);

    const bool horizontal = layoutDirection < QQuickItemView::VerticalTopToBottom;
    listview->setOrientation(horizontal ? QQuickListView::Horizontal : QQuickListView::Vertical);

    if (horizontal)
        listview->setLayoutDirection(static_cast<Qt::LayoutDirection>(layoutDirection));
    else
        listview->setVerticalLayoutDirection(static_cast<QQuickItemView::VerticalLayoutDirection>(layoutDirection));

    QVERIFY(QQuickTest::qWaitForItemPolished(listview));

    auto contentPosition = [&](QQuickListView *listview) {
        return (listview->orientation() == QQuickListView::Horizontal ? listview->contentX(): listview->contentY());
    };

    qreal expectedContentPosition = contentPosition(listview);
    QPoint startPos = (QPointF(listview->width(), listview->height())/2).toPoint();
    QTest::mousePress(window.data(), Qt::LeftButton, Qt::NoModifier, startPos, 200);

    QPoint dragDelta(0, -10);

    if (layoutDirection == QQuickItemView::RightToLeft || layoutDirection == QQuickItemView::VerticalBottomToTop)
        dragDelta = -dragDelta;
    expectedContentPosition -= dragDelta.y();
    if (horizontal)
        dragDelta = dragDelta.transposed();

    dragListView(window.data(), &startPos, dragDelta);

    QTest::mouseRelease(window.data(), Qt::LeftButton, Qt::NoModifier, startPos, 200);     // Wait 200 ms before we release to avoid trigger a flick

    // wait for the "fixup" animation to finish
    QVERIFY(QTest::qWaitFor([&]()
        { return !listview->isMoving();}
    ));

    QCOMPARE(contentPosition(listview), expectedContentPosition);
}


void tst_QQuickListView2::delegateChooserEnumRole()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("delegateChooserEnumRole.qml")));
    QQuickListView *listview = qobject_cast<QQuickListView*>(window.rootObject());
    QVERIFY(listview);
    QTRY_COMPARE(listview->count(), 3);
    QCOMPARE(listview->itemAtIndex(0)->property("delegateType").toInt(), 0);
    QCOMPARE(listview->itemAtIndex(1)->property("delegateType").toInt(), 1);
    QCOMPARE(listview->itemAtIndex(2)->property("delegateType").toInt(), 2);
}

void tst_QQuickListView2::QTBUG_92809()
{
    QScopedPointer<QQuickView> window(createView());
    QTRY_VERIFY(window);
    window->setSource(testFileUrl("qtbug_92809.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickListView *listview = findItem<QQuickListView>(window->rootObject(), "list");
    QTRY_VERIFY(listview != nullptr);
    QVERIFY(QQuickTest::qWaitForItemPolished(listview));
    listview->setCurrentIndex(1);
    QVERIFY(QQuickTest::qWaitForItemPolished(listview));
    listview->setCurrentIndex(2);
    QVERIFY(QQuickTest::qWaitForItemPolished(listview));
    listview->setCurrentIndex(3);
    QVERIFY(QQuickTest::qWaitForItemPolished(listview));
    QTest::qWait(500);
    listview->setCurrentIndex(10);
    QVERIFY(QQuickTest::qWaitForItemPolished(listview));
    QTest::qWait(500);
    int currentIndex = listview->currentIndex();
    QTRY_COMPARE(currentIndex, 9);
}

void tst_QQuickListView2::footerUpdate()
{
    QScopedPointer<QQuickView> window(createView());
    QTRY_VERIFY(window);
    window->setSource(testFileUrl("footerUpdate.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickListView *listview = findItem<QQuickListView>(window->rootObject(), "list");
    QTRY_VERIFY(listview != nullptr);
    QVERIFY(QQuickTest::qWaitForItemPolished(listview));
    QQuickItem *footer = listview->footerItem();
    QTRY_VERIFY(footer);
    QVERIFY(QQuickTest::qWaitForItemPolished(footer));
    QTRY_COMPARE(footer->y(), 0);
}

void tst_QQuickListView2::sectionsNoOverlap()
{
    QScopedPointer<QQuickView> window(createView());
    QTRY_VERIFY(window);
    window->setSource(testFileUrl("sectionsNoOverlap.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickListView *listview = findItem<QQuickListView>(window->rootObject(), "list");
    QTRY_VERIFY(listview != nullptr);

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem != nullptr);
    QVERIFY(QQuickTest::qWaitForItemPolished(listview));

    const unsigned int sectionCount = 2, normalDelegateCount = 2;
    const unsigned int expectedSectionHeight = 48;
    const unsigned int expectedNormalDelegateHeight = 40;

    unsigned int normalDelegateCounter = 0;
    for (unsigned int sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex) {
        QQuickItem *sectionDelegate =
                findItem<QQuickItem>(contentItem, "section" + QString::number(sectionIndex + 1));
        QVERIFY(sectionDelegate);

        QCOMPARE(sectionDelegate->height(), expectedSectionHeight);
        QVERIFY(sectionDelegate->isVisible());
        QCOMPARE(sectionDelegate->y(),
                 qreal(sectionIndex * expectedSectionHeight
                       + (sectionIndex * normalDelegateCount * expectedNormalDelegateHeight)));

        for (; normalDelegateCounter < ((sectionIndex + 1) * normalDelegateCount);
             ++normalDelegateCounter) {
            QQuickItem *normalDelegate = findItem<QQuickItem>(
                    contentItem, "element" + QString::number(normalDelegateCounter + 1));
            QVERIFY(normalDelegate);

            QCOMPARE(normalDelegate->height(), expectedNormalDelegateHeight);
            QVERIFY(normalDelegate->isVisible());
            QCOMPARE(normalDelegate->y(),
                     qreal((sectionIndex + 1) * expectedSectionHeight
                           + normalDelegateCounter * expectedNormalDelegateHeight
                           + listview->spacing() * normalDelegateCounter));
        }
    }
}

void tst_QQuickListView2::metaSequenceAsModel()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("metaSequenceAsModel.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QStringList strings = qvariant_cast<QStringList>(o->property("texts"));
    QCOMPARE(strings.length(), 2);
    QCOMPARE(strings[0], QStringLiteral("1/2"));
    QCOMPARE(strings[1], QStringLiteral("5/6"));
}

void tst_QQuickListView2::noCrashOnIndexChange()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("noCrashOnIndexChange.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QObject *delegateModel = qmlContext(o.data())->objectForName("displayDelegateModel");
    QVERIFY(delegateModel);

    QObject *items = qvariant_cast<QObject *>(delegateModel->property("items"));
    QCOMPARE(items->property("name").toString(), QStringLiteral("items"));
    QCOMPARE(items->property("count").toInt(), 4);
}

void tst_QQuickListView2::innerRequired()
{
    QQmlEngine engine;
    const QUrl url(testFileUrl("innerRequired.qml"));
    QQmlComponent component(&engine, url);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> o(component.create());
    QVERIFY2(!o.isNull(), qPrintable(component.errorString()));

    QQuickListView *a = qobject_cast<QQuickListView *>(
            qmlContext(o.data())->objectForName(QStringLiteral("listView")));
    QVERIFY(a);

    QCOMPARE(a->count(), 2);
    QCOMPARE(a->itemAtIndex(0)->property("age").toInt(), 8);
    QCOMPARE(a->itemAtIndex(0)->property("text").toString(), u"meow");
    QCOMPARE(a->itemAtIndex(1)->property("age").toInt(), 5);
    QCOMPARE(a->itemAtIndex(1)->property("text").toString(), u"woof");
}

void tst_QQuickListView2::boundDelegateComponent()
{
    QQmlEngine engine;
    const QUrl url(testFileUrl("boundDelegateComponent.qml"));
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
            QtWarningMsg, qPrintable(QLatin1String("%1:12: ReferenceError: index is not defined")
                                             .arg(url.toString())));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QQmlContext *context = qmlContext(o.data());

    QObject *inner = context->objectForName(QLatin1String("listView"));
    QVERIFY(inner != nullptr);
    QQuickListView *listView = qobject_cast<QQuickListView *>(inner);
    QVERIFY(listView != nullptr);
    QObject *item = listView->itemAtIndex(0);
    QVERIFY(item);
    QCOMPARE(item->objectName(), QLatin1String("fooouterundefined"));

    QObject *inner2 = context->objectForName(QLatin1String("listView2"));
    QVERIFY(inner2 != nullptr);
    QQuickListView *listView2 = qobject_cast<QQuickListView *>(inner2);
    QVERIFY(listView2 != nullptr);
    QObject *item2 = listView2->itemAtIndex(0);
    QVERIFY(item2);
    QCOMPARE(item2->objectName(), QLatin1String("fooouter0"));

    QQmlComponent *comp = qobject_cast<QQmlComponent *>(
            context->objectForName(QLatin1String("outerComponent")));
    QVERIFY(comp != nullptr);

    for (int i = 0; i < 3; ++i) {
        QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(QLatin1String("%1:47:21: ReferenceError: model is not defined")
                                   .arg(url.toString())));
    }

    QScopedPointer<QObject> outerItem(comp->create(context));
    QVERIFY(!outerItem.isNull());
    QQuickListView *innerListView = qobject_cast<QQuickListView *>(
            qmlContext(outerItem.data())->objectForName(QLatin1String("innerListView")));
    QVERIFY(innerListView != nullptr);
    QCOMPARE(innerListView->count(), 3);
    for (int i = 0; i < 3; ++i)
        QVERIFY(innerListView->itemAtIndex(i)->objectName().isEmpty());
}

class SingletonModel : public QStringListModel
{
    Q_OBJECT
public:
    SingletonModel(QObject* parent = nullptr) : QStringListModel(parent) { }
};

void tst_QQuickListView2::singletonModelLifetime()
{
    // this does not really test any functionality of listview, but we do not have a good way
    // to unit test QQmlAdaptorModel in isolation.
    qmlRegisterSingletonType<SingletonModel>("test", 1, 0, "SingletonModel",
            [](QQmlEngine* , QJSEngine*) -> QObject* { return new SingletonModel; });

    QQmlApplicationEngine engine(testFile("singletonModelLifetime.qml"));
    // needs event loop iteration for callLater to execute
    QTRY_VERIFY(engine.rootObjects().first()->property("alive").toBool());
}

void tst_QQuickListView2::delegateModelRefresh()
{
    // Test case originates from QTBUG-100161
    QQmlApplicationEngine engine(testFile("delegateModelRefresh.qml"));
    QVERIFY(!engine.rootObjects().isEmpty());
    // needs event loop iteration for callLater to execute
    QTRY_VERIFY(engine.rootObjects().first()->property("done").toBool());
}

QTEST_MAIN(tst_QQuickListView2)

#include "tst_qquicklistview2.moc"
