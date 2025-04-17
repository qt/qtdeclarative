// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtGui/qpa/qwindowsysteminterface.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>
#include <QtQuick/private/qquicktext_p_p.h>

using namespace QQuickVisualTestUtils;

class tst_QQuickControl : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickControl();

private slots:
    void initTestCase() override;
    void flickable();
    void fractionalFontSize();
    void resizeBackgroundKeepsBindings();
    void hoverInMouseArea();

private:
    QScopedPointer<QPointingDevice> touchDevice;
};

tst_QQuickControl::tst_QQuickControl()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickControl::initTestCase()
{
    QQmlDataTest::initTestCase();
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");

    touchDevice.reset(QTest::createTouchDevice());
}

void tst_QQuickControl::flickable()
{
    // Check that when a Button that is inside a Flickable with a pressDelay
    // still gets the released and clicked signals sent due to the fact that
    // Flickable sends a mouse event for the delay and not a touch event
    QQuickApplicationHelper helper(this, QStringLiteral("flickable.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickButton *button = window->property("button").value<QQuickButton *>();
    QVERIFY(button);

    QSignalSpy buttonPressedSpy(button, SIGNAL(pressed()));
    QVERIFY(buttonPressedSpy.isValid());

    QSignalSpy buttonReleasedSpy(button, SIGNAL(released()));
    QVERIFY(buttonReleasedSpy.isValid());

    QSignalSpy buttonClickedSpy(button, SIGNAL(clicked()));
    QVERIFY(buttonClickedSpy.isValid());

    QPoint p(button->width() / 2, button->height() / 2);
    QTest::touchEvent(window, touchDevice.data()).press(0, p);
    QTRY_COMPARE(buttonPressedSpy.size(), 1);
    p += QPoint(1, 1); // less than the drag threshold
    QTest::touchEvent(window, touchDevice.data()).move(0, p);
    QTest::touchEvent(window, touchDevice.data()).release(0, p);
    QTRY_COMPARE(buttonReleasedSpy.size(), 1);
    QTRY_COMPARE(buttonClickedSpy.size(), 1);
}

void tst_QQuickControl::fractionalFontSize()
{
    QQuickApplicationHelper helper(this, QStringLiteral("fractionalFontSize.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    const QQuickControl *control = window->property("control").value<QQuickControl *>();
    QVERIFY(control);
    QQuickText *contentItem = qobject_cast<QQuickText *>(control->contentItem());
    QVERIFY(contentItem);

    QVERIFY(!contentItem->truncated());

    QVERIFY2(qFuzzyCompare(contentItem->contentWidth(),
            QQuickTextPrivate::get(contentItem)->layout.boundingRect().width()),
            "The QQuickText::contentWidth() doesn't match the layout's preferred text width");
}

void tst_QQuickControl::resizeBackgroundKeepsBindings()
{
    QQuickApplicationHelper helper(this, QStringLiteral("resizeBackgroundKeepsBindings.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    auto ctxt = qmlContext(window);
    QVERIFY(ctxt);
    auto background = qobject_cast<QQuickItem *>(ctxt->objectForName("background"));
    QVERIFY(background);
    QCOMPARE(background->height(), 4);
    QVERIFY(background->bindableHeight().hasBinding());
}

void tst_QQuickControl::hoverInMouseArea()
{
    SKIP_IF_NO_MOUSE_HOVER;

    QQuickApplicationHelper helper(this, QStringLiteral("hoverInMouseArea.qml"));
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    const auto *control = window->property("control").value<QQuickControl *>();
    QVERIFY(control);
    PointLerper pointLerper(window);
    pointLerper.move(mapCenterToWindow(control));
    // It's necessary to use PointLerper here,
    // otherwise the isHovered checks are flaky on most platforms.
    QVERIFY(control->isHovered());

    const auto *textArea = window->property("textArea").value<QQuickTextArea *>();
    QVERIFY(textArea);
    pointLerper.move(mapCenterToWindow(textArea));
    QVERIFY(textArea->isHovered());

    const auto *textField = window->property("textField").value<QQuickTextField *>();
    QVERIFY(textField);
    pointLerper.move(mapCenterToWindow(textField));
    QVERIFY(textField->isHovered());
}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickControl)

#include "tst_qquickcontrol.moc"
