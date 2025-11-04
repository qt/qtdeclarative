// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtQuickTest/QtQuickTest>

#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlapplicationengine.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquicksafearea_p.h>

#include <QtQuickTestUtils/private/visualtestutils_p.h>

#include <QtQuickTemplates2/private/qquickcontrol_p.h>

#if defined(Q_OS_MACOS)
#include <AppKit/NSView.h>
#endif

class tst_QQuickSafeArea : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickSafeArea()
        : QQmlDataTest(QT_QMLTEST_DATADIR,
            FailOnWarningsPolicy::FailOnWarnings)
    {
    }

private slots:
    void properties();

#if defined(Q_OS_MACOS)
    void margins_data();
    void margins();
#endif

    void additionalMargins();
    void updateFlipFlop();

    void independentMargins_data();
    void independentMargins();

    void bindingLoop();
    void bindingLoopApplicationWindow();

    void safeAreaReuse();

    void controlBindingLoop_data();
    void controlBindingLoop();

    void flickable();
};

using namespace QQuickVisualTestUtils;

void tst_QQuickSafeArea::properties()
{
    QQuickApplicationHelper helper(this, "properties.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;

    QCOMPARE(window->property("margins").metaType(), QMetaType::fromType<QMarginsF>());
    QCOMPARE(window->property("marginsTop").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(window->property("marginsLeft").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(window->property("marginsRight").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(window->property("marginsBottom").metaType(), QMetaType::fromType<qreal>());

    auto *item = window->findChild<QQuickItem*>("item");
    QVERIFY(item);
    QCOMPARE(item->property("margins").metaType(), QMetaType::fromType<QMarginsF>());
    QCOMPARE(item->property("marginsTop").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(item->property("marginsLeft").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(item->property("marginsRight").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(item->property("marginsBottom").metaType(), QMetaType::fromType<qreal>());
}

#if defined(Q_OS_MACOS)

static constexpr NSEdgeInsets additionalInsets = { 10, 20, 30, 40 };

void tst_QQuickSafeArea::margins_data()
{
    QTest::addColumn<QString>("itemName");
    QTest::addColumn<QMarginsF>("expectedMargins");

    QTest::newRow("fill") << "fillItem" << QMarginsF(
        additionalInsets.left, additionalInsets.top,
        additionalInsets.right, additionalInsets.bottom
    );

    QTest::newRow("top") << "topItem" << QMarginsF(0, additionalInsets.top, 0, 0);
    QTest::newRow("left") << "leftItem" << QMarginsF(additionalInsets.left, 0, 0, 0);
    QTest::newRow("right") << "rightItem" << QMarginsF(0, 0, additionalInsets.right, 0);
    QTest::newRow("bottom") << "bottomItem" << QMarginsF(0, 0, 0, additionalInsets.bottom);
    QTest::newRow("center") << "centerItem" << QMarginsF(0, 0, 0, 0);

    QTest::newRow("topChild") << "topChildItem" <<  QMarginsF(0, additionalInsets.top - 3, 0, 0);
    QTest::newRow("leftChild") << "leftChildItem" << QMarginsF(additionalInsets.left - 3, 0, 0, 0);
    QTest::newRow("rightChild") << "rightChildItem" << QMarginsF(0, 0, additionalInsets.right - 3, 0);
    QTest::newRow("bottomChild") << "bottomChildItem" << QMarginsF(0, 0, 0, additionalInsets.bottom - 3);
    QTest::newRow("centerChild") << "centerChildItem" << QMarginsF(0, 0, 0, 0);
}

void tst_QQuickSafeArea::margins()
{
    QQuickApplicationHelper helper(this, "margins.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QCOMPARE(window->property("margins").value<QMarginsF>(), QMarginsF());

    auto *fillItem = window->findChild<QQuickItem*>("fillItem");
    QVERIFY(fillItem);

    QCOMPARE(fillItem->property("margins").value<QMarginsF>(), QMarginsF());

    // Mock changes on the QWindow level by adjusting the NSView
    auto *view = reinterpret_cast<NSView*>(window->winId());
    view.additionalSafeAreaInsets = additionalInsets;

    QTRY_COMPARE(window->property("margins").value<QMarginsF>(),
        QMarginsF(additionalInsets.left, additionalInsets.top,
            additionalInsets.right, additionalInsets.bottom));

    QFETCH(QString, itemName);

    auto *item = window->findChild<QQuickItem*>(itemName);
    QVERIFY(item);

    QFETCH(QMarginsF, expectedMargins);
    QCOMPARE(item->property("margins").value<QMarginsF>(), expectedMargins);
}
#endif

void tst_QQuickSafeArea::additionalMargins()
{
    QQuickApplicationHelper helper(this, "additionalMargins.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QCOMPARE(window->property("margins").value<QMarginsF>(),
        QMarginsF(20, 10, 40, 30));

    auto *additionalItem = window->findChild<QQuickItem*>("additionalItem");
    QCOMPARE(additionalItem->property("margins").value<QMarginsF>(),
        QMarginsF(140, 120, 180, 160));

    auto *additionalChild = additionalItem->findChild<QQuickItem*>("additionalChild");
    QCOMPARE(additionalChild->property("margins").value<QMarginsF>(),
        QMarginsF(137, 117, 177, 157));

    auto *additionalSibling = window->findChild<QQuickItem*>("additionalSibling");
    QCOMPARE(additionalSibling->property("margins").value<QMarginsF>(),
        QMarginsF(20, 10, 40, 30));

    auto *negativeItem = window->findChild<QQuickItem*>("negativeItem");
    auto *negativeSafeArea = qobject_cast<QQuickSafeArea*>(
        qmlAttachedPropertiesObject<QQuickSafeArea>(negativeItem));
    QVERIFY(negativeSafeArea);
    QCOMPARE(negativeSafeArea->additionalMargins(), QMarginsF());

    QCOMPARE(negativeItem->property("margins").value<QMarginsF>(),
        QMarginsF(20, 10, 40, 30));

    auto *negativeChild = negativeItem->findChild<QQuickItem*>("negativeChild");
    QCOMPARE(negativeChild->property("margins").value<QMarginsF>(),
        QMarginsF(17, 7, 37, 27));
}

void tst_QQuickSafeArea::independentMargins_data()
{
    QTest::addColumn<QString>("itemName");
    QTest::addColumn<QMarginsF>("expectedMargins");

    QTest::newRow("top") << "topMarginItem" << QMarginsF(0, 50, 0, 0);
    QTest::newRow("left") << "leftMarginItem" << QMarginsF(50, 0, 0, 0);
    QTest::newRow("right") << "rightMarginItem" << QMarginsF(0, 0, 50, 0);
    QTest::newRow("bottom") << "bottomMarginItem" << QMarginsF(0, 0, 0, 50);
}

void tst_QQuickSafeArea::independentMargins()
{
    QQuickApplicationHelper helper(this, "independentMargins.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QFETCH(QString, itemName);

    auto *item = window->findChild<QQuickItem*>(itemName);
    QVERIFY(item);

    QFETCH(QMarginsF, expectedMargins);
    QCOMPARE(item->property("margins").value<QMarginsF>(), expectedMargins);
}

void tst_QQuickSafeArea::updateFlipFlop()
{
    QQuickApplicationHelper helper(this, "updateFlipFlop.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QSignalSpy widthChangedSpy(window, SIGNAL(itemWidthChanged()));
    QSignalSpy marginChangeSpy(window, SIGNAL(safeAreaRightMarginChanged()));
    window->resize(window->width() - 1, window->height());
    QTRY_COMPARE(widthChangedSpy.count(), 1);
    QCOMPARE(marginChangeSpy.count(), 0);
}

void tst_QQuickSafeArea::bindingLoop()
{
    QQuickApplicationHelper helper(this, "bindingLoop.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    helper.engine.setOutputWarningsToStandardError(false);

    QList<QQmlError> warnings;
    QObject::connect(&helper.engine, &QQmlEngine::warnings, this,
                     [&warnings](auto w) { warnings = w; });
    auto *windowSafeArea = qobject_cast<QQuickSafeArea*>(qmlAttachedPropertiesObject<QQuickSafeArea>(window));
    QVERIFY(windowSafeArea);
    windowSafeArea->setAdditionalMargins(QMarginsF(50, 0, 0, 0));
    QCOMPARE(warnings.size(), 1);
    QVERIFY(warnings.at(0).description().endsWith("Safe area binding loop detected"));
}

void tst_QQuickSafeArea::bindingLoopApplicationWindow()
{
    QQuickApplicationHelper helper(this, "bindingLoopApplicationWindow.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    helper.engine.setOutputWarningsToStandardError(false);

    QList<QQmlError> warnings;
    QObject::connect(&helper.engine, &QQmlEngine::warnings, this,
                     [&warnings](auto w) { warnings = w; });
    auto *windowSafeArea = qobject_cast<QQuickSafeArea*>(qmlAttachedPropertiesObject<QQuickSafeArea>(window));
    QVERIFY(windowSafeArea);

    // Control in footer should stabilize and not cause warning
    windowSafeArea->setAdditionalMargins(QMarginsF(50, 0, 50, 0));
    QSignalSpy widthChangedSpy(window, SIGNAL(itemWidthChanged()));
    window->resize(window->width() - 10, window->height());
    QTRY_COMPARE(widthChangedSpy.count(), 1);
    QCOMPARE(warnings.size(), 0);
}

void tst_QQuickSafeArea::safeAreaReuse()
{
    QQuickApplicationHelper helper(this, "safeAreaReuse.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *windowSafeArea = qobject_cast<QQuickSafeArea*>(
        qmlAttachedPropertiesObject<QQuickSafeArea>(window, false));
    QVERIFY(windowSafeArea);

    auto *windowContentItemSafeArea = qobject_cast<QQuickSafeArea*>(
        qmlAttachedPropertiesObject<QQuickSafeArea>(window->contentItem(), false));
    QVERIFY(windowContentItemSafeArea);

    QCOMPARE(windowSafeArea, windowContentItemSafeArea);
}

void tst_QQuickSafeArea::controlBindingLoop_data()
{
    QTest::addColumn<QSize>("windowSize");
    QTest::addColumn<QMargins>("safeAreaMargins");
    QTest::addColumn<QMargins>("extraPadding");
    QTest::addColumn<QPoint>("anchorPosition");
    QTest::addColumn<QRect>("expectedControlGeometry");
    QTest::addColumn<QRect>("expectedContentItemGeometry");

    // Top/bottom

    QTest::newRow("top margin, anchors bottom, inside safe area")
        << QSize(500, 500) << QMargins(0, 100, 0, 0) << QMargins() << QPoint(0, 500)
        << QRect(0, 400, 100, 100) << QRect(0, 0, 100, 100);

    QTest::newRow("top margin, anchors top, overlapping non-safe area")
        << QSize(500, 500) << QMargins(0, 100, 0, 0) << QMargins() << QPoint(0, 0)
        << QRect(0, 0, 100, 200) << QRect(0, 100, 100, 100);

    QTest::newRow("top margin, anchors bottom, overlapping non-safe area")
        << QSize(500, 150) << QMargins(0, 100, 0, 0) << QMargins() << QPoint(0, 150)
        << QRect(0, 50, 100, 100) << QRect(0, 50, 100, 50);

    QTest::newRow("top margin, anchors top, overlapping non-safe area, extra padding")
        << QSize(500, 500) << QMargins(0, 100, 0, 0) << QMargins(0, 10, 0, 10) << QPoint(0, 0)
        << QRect(0, 0, 100, 220) << QRect(0, 110, 100, 100);

    QTest::newRow("bottom margin, anchors top, inside safe area")
        << QSize(500, 500) << QMargins(0, 0, 0, 100) << QMargins() << QPoint(0, 0)
        << QRect(0, 0, 100, 100) << QRect(0, 0, 100, 100);

    QTest::newRow("bottom margin, anchors bottom, overlapping non-safe area")
        << QSize(500, 500) << QMargins(0, 0, 0, 100) << QMargins() << QPoint(0, 500)
        << QRect(0, 300, 100, 200) << QRect(0, 0, 100, 100);

    QTest::newRow("bottom margin, anchors top, overlapping non-safe area")
        << QSize(500, 150) << QMargins(0, 0, 0, 100) << QMargins() << QPoint(0, 0)
        << QRect(0, 0, 100, 100) << QRect(0, 0, 100, 50);

    QTest::newRow("bottom margin, anchors bottom, overlapping non-safe area, extra padding")
        << QSize(500, 500) << QMargins(0, 0, 0, 100) << QMargins(0, 10, 0, 10) << QPoint(0, 500)
        << QRect(0, 280, 100, 220) << QRect(0, 10, 100, 100);

    QTest::newRow("top and bottom margin, anchors top, overlapping non-safe area")
        << QSize(500, 250) << QMargins(0, 100, 0, 100) << QMargins() << QPoint(0, 0)
        << QRect(0, 0, 100, 200) << QRect(0, 100, 100, 50);

    QTest::newRow("top and bottom margin, anchors top, overlapping non-safe area, extra padding")
        << QSize(500, 250) << QMargins(0, 100, 0, 100) << QMargins(0, 10, 0, 10) << QPoint(0, 0)
        << QRect(0, 0, 100, 220) << QRect(0, 110, 100, 30);

    // Left/right

    QTest::newRow("left margin, anchors right, inside safe area")
        << QSize(500, 500) << QMargins(100, 0, 0, 0) << QMargins() << QPoint(500, 0)
        << QRect(400, 0, 100, 100) << QRect(0, 0, 100, 100);

    QTest::newRow("left margin, anchors left, overlapping non-safe area")
        << QSize(500, 500) << QMargins(100, 0, 0, 0) << QMargins() << QPoint(0, 0)
        << QRect(0, 0, 200, 100) << QRect(100, 0, 100, 100);

    QTest::newRow("left margin, anchors right, overlapping non-safe area")
        << QSize(150, 500) << QMargins(100, 0, 0, 0) << QMargins() << QPoint(150, 0)
        << QRect(50, 0, 100, 100) << QRect(50, 0, 50, 100);

    QTest::newRow("left margin, anchors left, overlapping non-safe area, extra padding")
        << QSize(500, 500) << QMargins(100, 0, 0, 0) << QMargins(10, 0, 10, 0) << QPoint(0, 0)
        << QRect(0, 0, 220, 100) << QRect(110, 0, 100, 100);

    QTest::newRow("right margin, anchors left, inside safe area")
        << QSize(500, 500) << QMargins(0, 0, 100, 0) << QMargins() << QPoint(0, 0)
        << QRect(0, 0, 100, 100) << QRect(0, 0, 100, 100);

    QTest::newRow("right margin, anchors right, overlapping non-safe area")
        << QSize(500, 500) << QMargins(0, 0, 100, 0) << QMargins() << QPoint(500, 0)
        << QRect(300, 0, 200, 100) << QRect(0, 0, 100, 100);

    QTest::newRow("right margin, anchors left, overlapping non-safe area")
        << QSize(150, 500) << QMargins(0, 0, 100, 0) << QMargins() << QPoint(0, 0)
        << QRect(0, 0, 100, 100) << QRect(0, 0, 50, 100);

    QTest::newRow("right margin, anchors right, overlapping non-safe area, extra padding")
        << QSize(500, 500) << QMargins(0, 0, 100, 0) << QMargins(10, 0, 10, 0) << QPoint(500, 0)
        << QRect(280, 0, 220, 100) << QRect(10, 0, 100, 100);

    QTest::newRow("left and right margin, anchors left, overlapping non-safe area")
        << QSize(250, 500) << QMargins(100, 0, 100, 0) << QMargins() << QPoint(0, 0)
        << QRect(0, 0, 200, 100) << QRect(100, 0, 50, 100);

    QTest::newRow("left and right margin, anchors left, overlapping non-safe area, extra padding")
        << QSize(250, 500) << QMargins(100, 0, 100, 0) << QMargins(10, 0, 10, 0) << QPoint(0, 0)
        << QRect(0, 0, 220, 100) << QRect(110, 0, 30, 100);

}

void tst_QQuickSafeArea::controlBindingLoop()
{
    QFETCH(QSize, windowSize);
    QFETCH(QMargins, safeAreaMargins);
    QFETCH(QMargins, extraPadding);
    QFETCH(QPoint, anchorPosition);
    QFETCH(QRect, expectedControlGeometry);
    QFETCH(QRect, expectedContentItemGeometry);

    // Try first with explicit size set, and then verify
    // that the implicit size behaves the same.
    for (int i = 0; i < 2; ++i) {
        const bool explicitSize = i == 0;
        QQuickApplicationHelper helper(this, "controlBindingLoop.qml", {
            { "windowSize", QVariant::fromValue(windowSize) },
            { "safeAreaMargins", QVariant::fromValue(safeAreaMargins) },
            { "extraPadding", QVariant::fromValue(extraPadding) },
            { "anchorPosition", anchorPosition },
            { "explicitSize", explicitSize ?
                QVariant::fromValue(expectedControlGeometry.size())
              : QVariant() },
            { "currentDataTag", QTest::currentDataTag() }
        });
        QVERIFY2(helper.ready, helper.failureMessage());
        QQuickWindow *window = helper.window;
        window->show();
        QVERIFY(QTest::qWaitForWindowExposed(window));

        auto *control = window->findChild<QQuickControl*>("control");
        QVERIFY(control);

        QRectF controlGeometry(control->position(), control->size());
        QCOMPARE(controlGeometry, expectedControlGeometry);

        auto *contentItem = control->contentItem();
        QRectF contentItemGeometry(contentItem->position(), contentItem->size());
        QCOMPARE(contentItemGeometry, expectedContentItemGeometry);
    }
}

void tst_QQuickSafeArea::flickable()
{
    QQuickApplicationHelper helper(this, "flickable.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *flickable = window->findChild<QQuickItem*>("flickable");
    auto *flickableSafeArea = qobject_cast<QQuickSafeArea*>(
        qmlAttachedPropertiesObject<QQuickSafeArea>(flickable));
    QVERIFY(flickableSafeArea);
    QCOMPARE(flickableSafeArea->margins(), QMarginsF(50, 200, 0, 0));

    auto *contentChild = window->findChild<QQuickItem*>("contentChild");
    auto *contentChildSafeArea = qobject_cast<QQuickSafeArea*>(
        qmlAttachedPropertiesObject<QQuickSafeArea>(contentChild));
    QVERIFY(contentChildSafeArea);
    QCOMPARE(contentChildSafeArea->margins(), QMarginsF(0, 100, 0, 0));
}

QTEST_MAIN(tst_QQuickSafeArea)

#include "tst_qquicksafearea.moc"
