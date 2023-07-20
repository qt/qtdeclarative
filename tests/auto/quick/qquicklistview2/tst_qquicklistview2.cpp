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

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

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
    void noCrashOnIndexChange();
    void tapDelegateDuringFlicking_data();
    void tapDelegateDuringFlicking();
    void flickDuringFlicking_data();
    void flickDuringFlicking();
    void isCurrentItem_DelegateModel();
    void isCurrentItem_NoRegressionWithDelegateModelGroups();

    void fetchMore_data();
    void fetchMore();

private:
    void flickWithTouch(QQuickWindow *window, const QPoint &from, const QPoint &to);
    QScopedPointer<QPointingDevice> touchDevice = QScopedPointer<QPointingDevice>(QTest::createTouchDevice());
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

void tst_QQuickListView2::tapDelegateDuringFlicking_data()
{
    QTest::addColumn<QByteArray>("qmlFile");
    QTest::addColumn<QQuickFlickable::BoundsBehavior>("boundsBehavior");
    QTest::addColumn<bool>("expectCanceled");

    QTest::newRow("Button StopAtBounds") << QByteArray("buttonDelegate.qml")
                                         << QQuickFlickable::BoundsBehavior(QQuickFlickable::StopAtBounds) << false;
    QTest::newRow("MouseArea StopAtBounds") << QByteArray("mouseAreaDelegate.qml")
                                            << QQuickFlickable::BoundsBehavior(QQuickFlickable::StopAtBounds) << true;
    QTest::newRow("Button DragOverBounds") << QByteArray("buttonDelegate.qml")
                                           << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragOverBounds) << false;
    QTest::newRow("MouseArea DragOverBounds") << QByteArray("mouseAreaDelegate.qml")
                                              << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragOverBounds) << true;
    QTest::newRow("Button OvershootBounds") << QByteArray("buttonDelegate.qml")
                                            << QQuickFlickable::BoundsBehavior(QQuickFlickable::OvershootBounds) << false;
    QTest::newRow("MouseArea OvershootBounds") << QByteArray("mouseAreaDelegate.qml")
                                               << QQuickFlickable::BoundsBehavior(QQuickFlickable::OvershootBounds) << true;
    QTest::newRow("Button DragAndOvershootBounds") << QByteArray("buttonDelegate.qml")
                                                   << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragAndOvershootBounds) << false;
    QTest::newRow("MouseArea DragAndOvershootBounds") << QByteArray("mouseAreaDelegate.qml")
                                                      << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragAndOvershootBounds) << true;
}

void tst_QQuickListView2::tapDelegateDuringFlicking() // QTBUG-103832
{
    QFETCH(QByteArray, qmlFile);
    QFETCH(QQuickFlickable::BoundsBehavior, boundsBehavior);
    QFETCH(bool, expectCanceled);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl(qmlFile.constData())));
    QQuickListView *listView = qobject_cast<QQuickListView*>(window.rootObject());
    QVERIFY(listView);
    listView->setBoundsBehavior(boundsBehavior);

    flickWithTouch(&window, {100, 400}, {100, 100});
    QTRY_VERIFY(listView->contentY() > 501); // let it flick some distance
    QVERIFY(listView->isFlicking()); // we want to test the case when it's still moving while we tap
    // @y = 400 we pressed the 4th delegate; started flicking, and the press was canceled
    QCOMPARE(listView->property("pressedDelegates").toList().first(), 4);
    // At first glance one would expect MouseArea and Button would be consistent about this;
    // but in fact, before ListView takes over the grab via filtering,
    // Button.pressed transitions to false because QQuickAbstractButtonPrivate::handleMove
    // sees that the touchpoint has strayed outside its bounds, but it does NOT emit the canceled signal
    if (expectCanceled) {
        const QVariantList canceledDelegates = listView->property("canceledDelegates").toList();
        QCOMPARE(canceledDelegates.size(), 1);
        QCOMPARE(canceledDelegates.first(), 4);
    }
    QCOMPARE(listView->property("releasedDelegates").toList().size(), 0);

    // press a delegate during flicking (at y > 501 + 100, so likely delegate 6)
    QTest::touchEvent(&window, touchDevice.data()).press(0, {100, 100});
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, touchDevice.data()).release(0, {100, 100});
    QQuickTouchUtils::flush(&window);

    const QVariantList pressedDelegates = listView->property("pressedDelegates").toList();
    const QVariantList releasedDelegates = listView->property("releasedDelegates").toList();
    const QVariantList tappedDelegates = listView->property("tappedDelegates").toList();
    const QVariantList canceledDelegates = listView->property("canceledDelegates").toList();

    qCDebug(lcTests) << "pressed" << pressedDelegates; // usually [4, 6]
    qCDebug(lcTests) << "released" << releasedDelegates;
    qCDebug(lcTests) << "tapped" << tappedDelegates;
    qCDebug(lcTests) << "canceled" << canceledDelegates;

    // which delegate received the second press, during flicking?
    const int lastPressed = pressedDelegates.last().toInt();
    QVERIFY(lastPressed > 5);
    QCOMPARE(releasedDelegates.last(), lastPressed);
    QCOMPARE(tappedDelegates.last(), lastPressed);
    QCOMPARE(canceledDelegates.size(), expectCanceled ? 1 : 0); // only the first press was canceled, not the second
}

void tst_QQuickListView2::flickDuringFlicking_data()
{
    QTest::addColumn<QByteArray>("qmlFile");
    QTest::addColumn<QQuickFlickable::BoundsBehavior>("boundsBehavior");

    QTest::newRow("Button StopAtBounds") << QByteArray("buttonDelegate.qml")
                                         << QQuickFlickable::BoundsBehavior(QQuickFlickable::StopAtBounds);
    QTest::newRow("MouseArea StopAtBounds") << QByteArray("mouseAreaDelegate.qml")
                                            << QQuickFlickable::BoundsBehavior(QQuickFlickable::StopAtBounds);
    QTest::newRow("Button DragOverBounds") << QByteArray("buttonDelegate.qml")
                                           << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragOverBounds);
    QTest::newRow("MouseArea DragOverBounds") << QByteArray("mouseAreaDelegate.qml")
                                              << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragOverBounds);
    QTest::newRow("Button OvershootBounds") << QByteArray("buttonDelegate.qml")
                                            << QQuickFlickable::BoundsBehavior(QQuickFlickable::OvershootBounds);
    QTest::newRow("MouseArea OvershootBounds") << QByteArray("mouseAreaDelegate.qml")
                                               << QQuickFlickable::BoundsBehavior(QQuickFlickable::OvershootBounds);
    QTest::newRow("Button DragAndOvershootBounds") << QByteArray("buttonDelegate.qml")
                                                   << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragAndOvershootBounds);
    QTest::newRow("MouseArea DragAndOvershootBounds") << QByteArray("mouseAreaDelegate.qml")
                                                      << QQuickFlickable::BoundsBehavior(QQuickFlickable::DragAndOvershootBounds);
}

void tst_QQuickListView2::flickDuringFlicking() // QTBUG-103832
{
    QFETCH(QByteArray, qmlFile);
    QFETCH(QQuickFlickable::BoundsBehavior, boundsBehavior);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl(qmlFile.constData())));
    QQuickListView *listView = qobject_cast<QQuickListView*>(window.rootObject());
    QVERIFY(listView);
    listView->setBoundsBehavior(boundsBehavior);

    flickWithTouch(&window, {100, 400}, {100, 100});
    QTRY_VERIFY(listView->contentY() > 1000); // let it flick some distance
    QVERIFY(listView->isFlicking()); // we want to test the case when it's moving and then we flick again
    const qreal posBeforeSecondFlick = listView->contentY();

    // flick again during flicking, and make sure that it doesn't jump back to the first delegate,
    // but flicks incrementally further from the position at that time
    QTest::touchEvent(&window, touchDevice.data()).press(0, {100, 400});
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "second press: contentY" << posBeforeSecondFlick << "->" << listView->contentY();
    qCDebug(lcTests) << "pressed delegates" << listView->property("pressedDelegates").toList();
    QVERIFY(listView->contentY() >= posBeforeSecondFlick);

    QTest::qWait(20);
    QTest::touchEvent(&window, touchDevice.data()).move(0, {100, 300});
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "first move after second press: contentY" << posBeforeSecondFlick << "->" << listView->contentY();
    QVERIFY(listView->contentY() >= posBeforeSecondFlick);

    QTest::qWait(20);
    QTest::touchEvent(&window, touchDevice.data()).move(0, {100, 200});
    QQuickTouchUtils::flush(&window);
    qCDebug(lcTests) << "second move after second press: contentY" << posBeforeSecondFlick << "->" << listView->contentY();
    QVERIFY(listView->contentY() >= posBeforeSecondFlick + 100);

    QTest::touchEvent(&window, touchDevice.data()).release(0, {100, 100});
}

void tst_QQuickListView2::flickWithTouch(QQuickWindow *window, const QPoint &from, const QPoint &to)
{
    QTest::touchEvent(window, touchDevice.data()).press(0, from, window);
    QQuickTouchUtils::flush(window);

    QPoint diff = to - from;
    for (int i = 1; i <= 8; ++i) {
        QTest::touchEvent(window, touchDevice.data()).move(0, from + i * diff / 8, window);
        QQuickTouchUtils::flush(window);
    }
    QTest::touchEvent(window, touchDevice.data()).release(0, to, window);
    QQuickTouchUtils::flush(window);
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

void tst_QQuickListView2::isCurrentItem_DelegateModel()
{
    QScopedPointer<QQuickView> window(createView());
    window->setSource(testFileUrl("qtbug86744.qml"));
    window->resize(640, 480);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QQuickListView* listView = window->rootObject()->findChild<QQuickListView*>("listView");
    QVERIFY(listView);
    QVariant value = listView->itemAtIndex(1)->property("isCurrent");
    QVERIFY(value.toBool() == true);
}

void tst_QQuickListView2::isCurrentItem_NoRegressionWithDelegateModelGroups()
{
    QScopedPointer<QQuickView> window(createView());
    window->setSource(testFileUrl("qtbug98315.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QQuickListView* listView = window->rootObject()->findChild<QQuickListView*>("listView");
    QVERIFY(listView);

    QQuickItem *item3 = listView->itemAtIndex(1);
    QVERIFY(item3);
    QCOMPARE(item3->property("isCurrent").toBool(), true);

    QObject *item0 = listView->itemAtIndex(0);
    QVERIFY(item0);
    QCOMPARE(item0->property("isCurrent").toBool(), false);

    // Press left arrow key -> Item 1 should become current, Item 3 should not
    // be current anymore. After a previous fix of QTBUG-86744 it was working
    // incorrectly - see QTBUG-98315
    QTest::keyPress(window.get(), Qt::Key_Left);

    QTRY_COMPARE(item0->property("isCurrent").toBool(), true);
    QCOMPARE(item3->property("isCurrent").toBool(), false);
}

class TestFetchMoreModel : public QAbstractListModel
{
    Q_OBJECT

public:
    QVariant data(const QModelIndex& index, int role) const override
    {
        if (role == Qt::DisplayRole)
            return QString::number(index.row());
        return {};
    }

    int columnCount(const QModelIndex&) const override { return 1; }

    int rowCount(const QModelIndex& parent) const override
    {
        return parent.isValid() ? 0 : m_lines;
    }

    QModelIndex parent(const QModelIndex&) const override { return {}; }

    bool canFetchMore(const QModelIndex &) const override { return true; }

    void fetchMore(const QModelIndex & parent) override
    {
        if (Q_UNLIKELY(parent.isValid()))
            return;
        beginInsertRows(parent, m_lines, m_lines);
        m_lines++;
        endInsertRows();
    }

    int m_lines = 3;
};

void tst_QQuickListView2::fetchMore_data()
{
    QTest::addColumn<bool>("reuseItems");
    QTest::addColumn<int>("cacheBuffer");

    QTest::newRow("no reuseItems, default buffer") << false << -1;
    QTest::newRow("reuseItems, default buffer") << true << -1;
    QTest::newRow("no reuseItems, no buffer") << false << 0;
    QTest::newRow("reuseItems, no buffer") << true << 0;
    QTest::newRow("no reuseItems, buffer 100 px") << false << 100;
    QTest::newRow("reuseItems, buffer 100 px") << true << 100;
}

void tst_QQuickListView2::fetchMore() // QTBUG-95107
{
    QFETCH(bool, reuseItems);
    QFETCH(int, cacheBuffer);

    TestFetchMoreModel model;
    qmlRegisterSingletonInstance("org.qtproject.Test", 1, 0, "FetchMoreModel", &model);
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("fetchMore.qml")));
    auto *listView = qobject_cast<QQuickListView*>(window.rootObject());
    QVERIFY(listView);
    listView->setReuseItems(reuseItems);
    if (cacheBuffer >= 0)
        listView->setCacheBuffer(cacheBuffer);

    for (int i = 0; i < 3; ++i) {
        const int rowCount = listView->count();
        if (lcTests().isDebugEnabled()) QTest::qWait(1000);
        listView->flick(0, -5000);
        QTRY_VERIFY(!listView->isMoving());
        qCDebug(lcTests) << "after flick: contentY" << listView->contentY()
                         << "rows" << rowCount << "->" << listView->count();
        QVERIFY(listView->count() > rowCount);
        QVERIFY(model.m_lines >= listView->count()); // fetchMore() was called
    }
}

QTEST_MAIN(tst_QQuickListView2)

#include "tst_qquicklistview2.moc"
