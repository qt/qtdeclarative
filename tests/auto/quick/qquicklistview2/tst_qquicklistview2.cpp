// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    void wheelSnap();
    void wheelSnap_data();

    void sectionsNoOverlap();
    void metaSequenceAsModel();
    void noCrashOnIndexChange();
    void innerRequired();
    void boundDelegateComponent();
    void tapDelegateDuringFlicking_data();
    void tapDelegateDuringFlicking();
    void flickDuringFlicking_data();
    void flickDuringFlicking();
    void maxExtent_data();
    void maxExtent();
    void isCurrentItem_DelegateModel();
    void isCurrentItem_NoRegressionWithDelegateModelGroups();

    void pullbackSparseList();
    void highlightWithBound();
    void sectionIsCompatibleWithBoundComponents();

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
        QVERIFY(QQuickTest::qWaitForPolish(view));
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

    QVERIFY(QQuickTest::qWaitForPolish(listview));

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
    QVERIFY(QQuickTest::qWaitForPolish(listview));
    listview->setCurrentIndex(1);
    QVERIFY(QQuickTest::qWaitForPolish(listview));
    listview->setCurrentIndex(2);
    QVERIFY(QQuickTest::qWaitForPolish(listview));
    listview->setCurrentIndex(3);
    QVERIFY(QQuickTest::qWaitForPolish(listview));
    QTest::qWait(500);
    listview->setCurrentIndex(10);
    QVERIFY(QQuickTest::qWaitForPolish(listview));
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
    QVERIFY(QQuickTest::qWaitForPolish(listview));
    QQuickItem *footer = listview->footerItem();
    QTRY_VERIFY(footer);
    QVERIFY(QQuickTest::qWaitForPolish(footer));
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
    QVERIFY(QQuickTest::qWaitForPolish(listview));

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
    QCOMPARE(strings.size(), 2);
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

void tst_QQuickListView2::tapDelegateDuringFlicking_data()
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

void tst_QQuickListView2::tapDelegateDuringFlicking() // QTBUG-103832
{
    QFETCH(QByteArray, qmlFile);
    QFETCH(QQuickFlickable::BoundsBehavior, boundsBehavior);

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
    QCOMPARE(listView->property("canceledDelegates").toList().first(), 4);

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
    QCOMPARE(canceledDelegates.size(), 1); // only the first press was canceled, not the second
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
    // let it flick some distance
    QTRY_COMPARE_GT(listView->contentY(), 500);
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

void tst_QQuickListView2::wheelSnap()
{
    QFETCH(QQuickListView::Orientation, orientation);
    QFETCH(Qt::LayoutDirection, layoutDirection);
    QFETCH(QQuickItemView::VerticalLayoutDirection, verticalLayoutDirection);
    QFETCH(QQuickItemView::HighlightRangeMode, highlightRangeMode);
    QFETCH(QPoint, forwardAngleDelta);
    QFETCH(qreal, snapAlignment);
    QFETCH(qreal, endExtent);
    QFETCH(qreal, startExtent);
    QFETCH(qreal, preferredHighlightBegin);
    QFETCH(qreal, preferredHighlightEnd);

    // Helpers begin
    quint64 timestamp = 10;
    auto sendWheelEvent = [&timestamp](QQuickView *window, const QPoint &angleDelta) {
        QPoint pos(100, 100);
        QWheelEvent event(pos, window->mapToGlobal(pos), QPoint(), angleDelta, Qt::NoButton,
                          Qt::NoModifier, Qt::NoScrollPhase, false);
        event.setAccepted(false);
        event.setTimestamp(timestamp);
        QGuiApplication::sendEvent(window, &event);
        timestamp += 50;
    };

    auto atEnd = [&layoutDirection, &orientation,
                  &verticalLayoutDirection](QQuickListView *listview) {
        if (orientation == QQuickListView::Horizontal) {
            if (layoutDirection == Qt::LeftToRight)
                return listview->isAtXEnd();

            return listview->isAtXBeginning();
        } else {
            if (verticalLayoutDirection == QQuickItemView::VerticalLayoutDirection::TopToBottom)
                return listview->isAtYEnd();

            return listview->isAtYBeginning();
        }
    };

    auto atBegin = [&layoutDirection, &orientation,
                    &verticalLayoutDirection](QQuickListView *listview) {
        if (orientation == QQuickListView::Horizontal) {
            if (layoutDirection == Qt::LeftToRight)
                return listview->isAtXBeginning();

            return listview->isAtXEnd();
        } else {
            if (verticalLayoutDirection == QQuickItemView::VerticalLayoutDirection::TopToBottom)
                return listview->isAtYBeginning();

            return listview->isAtYEnd();
        }
    };
    // Helpers end

    QScopedPointer<QQuickView> window(createView());
    QTRY_VERIFY(window);
    QQuickViewTestUtils::moveMouseAway(window.data());
    window->setSource(testFileUrl("snapOneItem.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickListView *listview = qobject_cast<QQuickListView *>(window->rootObject());
    QTRY_VERIFY(listview);

    listview->setOrientation(orientation);
    listview->setVerticalLayoutDirection(verticalLayoutDirection);
    listview->setLayoutDirection(layoutDirection);
    listview->setHighlightRangeMode(highlightRangeMode);
    listview->setPreferredHighlightBegin(preferredHighlightBegin);
    listview->setPreferredHighlightEnd(preferredHighlightEnd);
    QVERIFY(QQuickTest::qWaitForPolish(listview));

    QQuickItem *contentItem = listview->contentItem();
    QTRY_VERIFY(contentItem);

    QSignalSpy currentIndexSpy(listview, &QQuickListView::currentIndexChanged);

    // confirm that a flick hits the next item boundary
    int indexCounter = 0;
    sendWheelEvent(window.data(), forwardAngleDelta);
    QTRY_VERIFY(listview->isMoving() == false); // wait until it stops

    if (orientation == QQuickListView::Vertical)
        QCOMPARE(listview->contentY(), snapAlignment);
    else
        QCOMPARE(listview->contentX(), snapAlignment);

    if (highlightRangeMode == QQuickItemView::StrictlyEnforceRange) {
        ++indexCounter;
        QTRY_VERIFY(listview->currentIndex() == indexCounter);
    }

    // flick to end
    do {
        sendWheelEvent(window.data(), forwardAngleDelta);
        QTRY_VERIFY(listview->isMoving() == false); // wait until it stops
        if (highlightRangeMode == QQuickItemView::StrictlyEnforceRange) {
            ++indexCounter;
            QTRY_VERIFY(listview->currentIndex() == indexCounter);
        }
    } while (!atEnd(listview));

    if (orientation == QQuickListView::Vertical)
        QCOMPARE(listview->contentY(), endExtent);
    else
        QCOMPARE(listview->contentX(), endExtent);

    if (highlightRangeMode == QQuickItemView::StrictlyEnforceRange) {
        QCOMPARE(listview->currentIndex(), listview->count() - 1);
        QCOMPARE(currentIndexSpy.count(), listview->count() - 1);
    }

    // flick to start
    const QPoint backwardAngleDelta(-forwardAngleDelta.x(), -forwardAngleDelta.y());
    do {
        sendWheelEvent(window.data(), backwardAngleDelta);
        QTRY_VERIFY(listview->isMoving() == false); // wait until it stops
        if (highlightRangeMode == QQuickItemView::StrictlyEnforceRange) {
            --indexCounter;
            QTRY_VERIFY(listview->currentIndex() == indexCounter);
        }
    } while (!atBegin(listview));

    if (orientation == QQuickListView::Vertical)
        QCOMPARE(listview->contentY(), startExtent);
    else
        QCOMPARE(listview->contentX(), startExtent);

    if (highlightRangeMode == QQuickItemView::StrictlyEnforceRange) {
        QCOMPARE(listview->currentIndex(), 0);
        QCOMPARE(currentIndexSpy.count(), (listview->count() - 1) * 2);
    }
}

void tst_QQuickListView2::wheelSnap_data()
{
    QTest::addColumn<QQuickListView::Orientation>("orientation");
    QTest::addColumn<Qt::LayoutDirection>("layoutDirection");
    QTest::addColumn<QQuickItemView::VerticalLayoutDirection>("verticalLayoutDirection");
    QTest::addColumn<QQuickItemView::HighlightRangeMode>("highlightRangeMode");
    QTest::addColumn<QPoint>("forwardAngleDelta");
    QTest::addColumn<qreal>("snapAlignment");
    QTest::addColumn<qreal>("endExtent");
    QTest::addColumn<qreal>("startExtent");
    QTest::addColumn<qreal>("preferredHighlightBegin");
    QTest::addColumn<qreal>("preferredHighlightEnd");

    QTest::newRow("vertical, top to bottom")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::NoHighlightRange << QPoint(20, -120) << 200.0 << 600.0 << 0.0 << 0.0
            << 0.0;

    QTest::newRow("vertical, bottom to top")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::BottomToTop
            << QQuickItemView::NoHighlightRange << QPoint(20, 120) << -400.0 << -800.0 << -200.0
            << 0.0 << 0.0;

    QTest::newRow("horizontal, left to right")
            << QQuickListView::Horizontal << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::NoHighlightRange << QPoint(-120, 20) << 200.0 << 600.0 << 0.0 << 0.0
            << 0.0;

    QTest::newRow("horizontal, right to left")
            << QQuickListView::Horizontal << Qt::RightToLeft << QQuickItemView::TopToBottom
            << QQuickItemView::NoHighlightRange << QPoint(120, 20) << -400.0 << -800.0 << -200.0
            << 0.0 << 0.0;

    QTest::newRow("vertical, top to bottom, enforce range")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::StrictlyEnforceRange << QPoint(20, -120) << 200.0 << 600.0 << 0.0
            << 0.0 << 0.0;

    QTest::newRow("vertical, bottom to top, enforce range")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::BottomToTop
            << QQuickItemView::StrictlyEnforceRange << QPoint(20, 120) << -400.0 << -800.0 << -200.0
            << 0.0 << 0.0;

    QTest::newRow("horizontal, left to right, enforce range")
            << QQuickListView::Horizontal << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::StrictlyEnforceRange << QPoint(-120, 20) << 200.0 << 600.0 << 0.0
            << 0.0 << 0.0;

    QTest::newRow("horizontal, right to left, enforce range")
            << QQuickListView::Horizontal << Qt::RightToLeft << QQuickItemView::TopToBottom
            << QQuickItemView::StrictlyEnforceRange << QPoint(120, 20) << -400.0 << -800.0 << -200.0
            << 0.0 << 0.0;

    QTest::newRow("vertical, top to bottom, apply range")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::ApplyRange << QPoint(20, -120) << 200.0 << 600.0 << 0.0 << 0.0
            << 0.0;

    QTest::newRow("vertical, bottom to top, apply range")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::BottomToTop
            << QQuickItemView::ApplyRange << QPoint(20, 120) << -400.0 << -800.0 << -200.0 << 0.0
            << 0.0;

    QTest::newRow("horizontal, left to right, apply range")
            << QQuickListView::Horizontal << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::ApplyRange << QPoint(-120, 20) << 200.0 << 600.0 << 0.0 << 0.0
            << 0.0;

    QTest::newRow("horizontal, right to left, apply range")
            << QQuickListView::Horizontal << Qt::RightToLeft << QQuickItemView::TopToBottom
            << QQuickItemView::ApplyRange << QPoint(120, 20) << -400.0 << -800.0 << -200.0 << 0.0
            << 0.0;

    QTest::newRow("vertical, top to bottom with highlightRange")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::NoHighlightRange << QPoint(20, -120) << 190.0 << 600.0 << 0.0 << 10.0
            << 210.0;

    QTest::newRow("vertical, bottom to top with highlightRange")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::BottomToTop
            << QQuickItemView::NoHighlightRange << QPoint(20, 120) << -390.0 << -800.0 << -200.0
            << 10.0 << 210.0;

    QTest::newRow("horizontal, left to right with highlightRange")
            << QQuickListView::Horizontal << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::NoHighlightRange << QPoint(-120, 20) << 190.0 << 600.0 << 0.0 << 10.0
            << 210.0;

    QTest::newRow("horizontal, right to left with highlightRange")
            << QQuickListView::Horizontal << Qt::RightToLeft << QQuickItemView::TopToBottom
            << QQuickItemView::NoHighlightRange << QPoint(120, 20) << -390.0 << -800.0 << -200.0
            << 10.0 << 210.0;

    QTest::newRow("vertical, top to bottom, enforce range with highlightRange")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::StrictlyEnforceRange << QPoint(20, -120) << 190.0 << 590.0 << -10.0
            << 10.0 << 210.0;

    QTest::newRow("vertical, bottom to top, enforce range with highlightRange")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::BottomToTop
            << QQuickItemView::StrictlyEnforceRange << QPoint(20, 120) << -390.0 << -790.0 << -190.0
            << 10.0 << 210.0;

    QTest::newRow("horizontal, left to right, enforce range with highlightRange")
            << QQuickListView::Horizontal << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::StrictlyEnforceRange << QPoint(-120, 20) << 190.0 << 590.0 << -10.0
            << 10.0 << 210.0;

    QTest::newRow("horizontal, right to left, enforce range with highlightRange")
            << QQuickListView::Horizontal << Qt::RightToLeft << QQuickItemView::TopToBottom
            << QQuickItemView::StrictlyEnforceRange << QPoint(120, 20) << -390.0 << -790.0 << -190.0
            << 10.0 << 210.0;

    QTest::newRow("vertical, top to bottom, apply range with highlightRange")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::ApplyRange << QPoint(20, -120) << 190.0 << 600.0 << 0.0 << 10.0
            << 210.0;

    QTest::newRow("vertical, bottom to top, apply range with highlightRange")
            << QQuickListView::Vertical << Qt::LeftToRight << QQuickItemView::BottomToTop
            << QQuickItemView::ApplyRange << QPoint(20, 120) << -390.0 << -800.0 << -200.0 << 10.0
            << 210.0;

    QTest::newRow("horizontal, left to right, apply range with highlightRange")
            << QQuickListView::Horizontal << Qt::LeftToRight << QQuickItemView::TopToBottom
            << QQuickItemView::ApplyRange << QPoint(-120, 20) << 190.0 << 600.0 << 0.0 << 10.0
            << 210.0;

    QTest::newRow("horizontal, right to left, apply range with highlightRange")
            << QQuickListView::Horizontal << Qt::RightToLeft << QQuickItemView::TopToBottom
            << QQuickItemView::ApplyRange << QPoint(120, 20) << -390.0 << -800.0 << -200.0 << 10.0
            << 210.0;
}

class FriendlyItemView : public QQuickItemView
{
    friend class ItemViewAccessor;
};

class ItemViewAccessor
{
public:
    ItemViewAccessor(QQuickItemView *itemView) :
        mItemView(reinterpret_cast<FriendlyItemView*>(itemView))
    {
    }

    qreal maxXExtent() const
    {
        return mItemView->maxXExtent();
    }

    qreal maxYExtent() const
    {
        return mItemView->maxYExtent();
    }

private:
    FriendlyItemView *mItemView = nullptr;
};

void tst_QQuickListView2::maxExtent_data()
{
    QTest::addColumn<QString>("qmlFilePath");
    QTest::addRow("maxXExtent") << "maxXExtent.qml";
    QTest::addRow("maxYExtent") << "maxYExtent.qml";
}

void tst_QQuickListView2::maxExtent()
{
    QFETCH(QString, qmlFilePath);

    QScopedPointer<QQuickView> window(createView());
    QVERIFY(window);
    window->setSource(testFileUrl(qmlFilePath));
    QVERIFY2(window->status() == QQuickView::Ready, qPrintable(QDebug::toString(window->errors())));
    window->resize(640, 480);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickListView *view = window->rootObject()->property("view").value<QQuickListView*>();
    QVERIFY(view);
    ItemViewAccessor viewAccessor(view);
    if (view->orientation() == QQuickListView::Vertical)
        QCOMPARE(viewAccessor.maxXExtent(), 0);
    else if (view->orientation() == QQuickListView::Horizontal)
        QCOMPARE(viewAccessor.maxYExtent(), 0);
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

void tst_QQuickListView2::pullbackSparseList() // QTBUG_104679
{
    // check if PullbackHeader crashes
    QScopedPointer<QQuickView> window(createView());
    QVERIFY(window);
    window->setSource(testFileUrl("qtbug104679_header.qml"));
    QVERIFY2(window->status() == QQuickView::Ready, qPrintable(QDebug::toString(window->errors())));
    window->resize(640, 480);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    // check if PullbackFooter crashes
    window.reset(createView());
    QVERIFY(window);
    window->setSource(testFileUrl("qtbug104679_footer.qml"));
    QVERIFY2(window->status() == QQuickView::Ready, qPrintable(QDebug::toString(window->errors())));
    window->resize(640, 480);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
}

void tst_QQuickListView2::highlightWithBound()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("highlightWithBound.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QQuickListView *listView = qobject_cast<QQuickListView *>(o.data());
    QVERIFY(listView);
    QQuickItem *highlight = listView->highlightItem();
    QVERIFY(highlight);
    QCOMPARE(highlight->objectName(), QStringLiteral("highlight"));
}

void tst_QQuickListView2::sectionIsCompatibleWithBoundComponents()
{
    QTest::failOnWarning(".?");
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("sectionBoundComponent.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QQuickListView *listView = qobject_cast<QQuickListView *>(o.data());
    QVERIFY(listView);
    QTRY_COMPARE(listView->currentSection(), "42");
}

QTEST_MAIN(tst_QQuickListView2)

#include "tst_qquicklistview2.moc"
