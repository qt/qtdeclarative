// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpointingdevice.h>
#include <QtGui/qstylehints.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;

class tst_focus : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_focus();

private slots:
    void initTestCase() override;

    void navigation_data();
    void navigation();

    void policy_data();
    void policy();

    void reason();

    void visualFocus();

    void scope_data();
    void scope();
};

tst_focus::tst_focus()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_focus::initTestCase()
{
    QQuickStyle::setStyle("Basic");
    QQmlDataTest::initTestCase();
}

void tst_focus::navigation_data()
{
    QTest::addColumn<Qt::Key>("key");
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<Qt::TabFocusBehavior>("behavior");
    QTest::addColumn<QStringList>("order");

    QTest::newRow("tab-all-controls") << Qt::Key_Tab << QString("activeFocusOnTab.qml") << Qt::TabFocusAllControls << (QStringList() << "button2" << "checkbox" << "checkbox1" << "checkbox2" << "radiobutton" << "radiobutton1" << "radiobutton2" << "rangeslider.first" << "rangeslider.second" << "slider" << "spinbox" << "switch" << "tabbutton1" << "tabbutton2" << "textfield" << "toolbutton" << "textarea" << "button1");
    QTest::newRow("backtab-all-controls") << Qt::Key_Backtab << QString("activeFocusOnTab.qml") << Qt::TabFocusAllControls << (QStringList() << "textarea" << "toolbutton" << "textfield" << "tabbutton2" << "tabbutton1" << "switch" << "spinbox" << "slider" << "rangeslider.second" << "rangeslider.first" << "radiobutton2" << "radiobutton1" << "radiobutton" << "checkbox2" << "checkbox1" << "checkbox" << "button2" << "button1");

    QTest::newRow("tab-text-controls") << Qt::Key_Tab << QString("activeFocusOnTab.qml") << Qt::TabFocusTextControls << (QStringList() << "spinbox" << "textfield" << "textarea");
    QTest::newRow("backtab-text-controls") << Qt::Key_Backtab << QString("activeFocusOnTab.qml") << Qt::TabFocusTextControls << (QStringList() << "textarea" << "textfield" << "spinbox");

    QTest::newRow("key-up") << Qt::Key_Up << QString("keyNavigation.qml") << Qt::TabFocusAllControls << (QStringList() << "textarea" << "toolbutton" << "textfield" << "tabbutton2" << "tabbutton1" << "switch" << "slider" << "rangeslider.first" << "radiobutton2" << "radiobutton1" << "radiobutton" << "checkbox2" << "checkbox1" << "checkbox" << "button2" << "button1");
    QTest::newRow("key-down") << Qt::Key_Down << QString("keyNavigation.qml") << Qt::TabFocusAllControls << (QStringList() << "button2" << "checkbox" << "checkbox1" << "checkbox2" << "radiobutton" << "radiobutton1" << "radiobutton2" << "rangeslider.first" << "slider" << "switch" << "tabbutton1" << "tabbutton2" << "textfield" << "toolbutton" << "textarea" << "button1");
    QTest::newRow("key-left") << Qt::Key_Left << QString("keyNavigation.qml") << Qt::TabFocusAllControls << (QStringList() << "toolbutton" << "tabbutton2" << "tabbutton1" << "switch" << "spinbox" << "radiobutton2" << "radiobutton1" << "radiobutton" << "checkbox2" << "checkbox1" << "checkbox" << "button2" << "button1");
    QTest::newRow("key-right") << Qt::Key_Right << QString("keyNavigation.qml") << Qt::TabFocusAllControls << (QStringList() << "button2" << "checkbox" << "checkbox1" << "checkbox2" << "radiobutton" << "radiobutton1" << "radiobutton2" << "spinbox" << "switch" << "tabbutton1" << "tabbutton2" << "toolbutton" << "button1");
}

void tst_focus::navigation()
{
    QFETCH(Qt::Key, key);
    QFETCH(QString, testFile);
    QFETCH(Qt::TabFocusBehavior, behavior);
    QFETCH(QStringList, order);

    QGuiApplication::styleHints()->setTabFocusBehavior(behavior);

    QQuickView view;
    view.contentItem()->setObjectName("contentItem");

    view.setSource(testFileUrl(testFile));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QVERIFY(QGuiApplication::focusWindow() == &view);

    for (const QString &name : std::as_const(order)) {
        QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier);
        QGuiApplication::sendEvent(&view, &event);
        QVERIFY(event.isAccepted());

        QQuickItem *item = findItem<QQuickItem>(view.rootObject(), name);
        QVERIFY2(item, qPrintable(name));
        QVERIFY2(item->hasActiveFocus(), qPrintable(QString("expected: '%1', actual: '%2'").arg(name).arg(view.activeFocusItem() ? view.activeFocusItem()->objectName() : "null")));
    }

    QGuiApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusBehavior(-1));
}

void tst_focus::policy_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("Control") << "Control";
    QTest::newRow("ComboBox") << "ComboBox";
    QTest::newRow("Button") << "Button";
    QTest::newRow("Slider") << "Slider";
    QTest::newRow("ScrollBar") << "ScrollBar";
}

void tst_focus::policy()
{
    QFETCH(QString, name);

    QQmlEngine engine;
    QScopedPointer<QPointingDevice> device(QTest::createTouchDevice());
    QQmlComponent component(&engine);
    component.setData(QString("import QtQuick.Controls; ApplicationWindow { width: 100; height: 100; %1 { anchors.fill: parent } }").arg(name).toUtf8(), QUrl());

    QScopedPointer<QQuickApplicationWindow> window(qobject_cast<QQuickApplicationWindow *>(component.create()));
    QVERIFY(window);

    QQuickControl *control = qobject_cast<QQuickControl *>(window->contentItem()->childItems().first());
    QVERIFY(control);

    QVERIFY(!control->hasActiveFocus());
    QVERIFY(!control->hasVisualFocus());

    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));

    control->setFocusPolicy(Qt::NoFocus);
    QCOMPARE(control->focusPolicy(), Qt::NoFocus);

    // Qt::TabFocus vs. QQuickItem::activeFocusOnTab
    control->setActiveFocusOnTab(true);
    QCOMPARE(control->focusPolicy(), Qt::TabFocus);
    control->setActiveFocusOnTab(false);
    QCOMPARE(control->focusPolicy(), Qt::NoFocus);

    control->setFocusPolicy(Qt::TabFocus);
    QCOMPARE(control->focusPolicy(), Qt::TabFocus);
    QCOMPARE(control->activeFocusOnTab(), true);

    // Qt::TabFocus
    QGuiApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusAllControls);
    QTest::keyClick(window.data(), Qt::Key_Tab);
    QVERIFY(control->hasActiveFocus());
    QVERIFY(control->hasVisualFocus());
    QGuiApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusBehavior(-1));

    // reset
    control->setFocus(false);
    QVERIFY(!control->hasActiveFocus());

    // Qt::ClickFocus (mouse)
    control->setFocusPolicy(Qt::NoFocus);
    control->setAcceptedMouseButtons(Qt::LeftButton);
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(control->width() / 2, control->height() / 2));
    QVERIFY(!control->hasActiveFocus());
    QVERIFY(!control->hasVisualFocus());

    control->setFocusPolicy(Qt::ClickFocus);
    QCOMPARE(control->focusPolicy(), Qt::ClickFocus);
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(control->width() / 2, control->height() / 2));
    QVERIFY(control->hasActiveFocus());
    QVERIFY(!control->hasVisualFocus());

    // reset
    control->setFocus(false);
    QVERIFY(!control->hasActiveFocus());

    // Qt::ClickFocus (touch)
    control->setFocusPolicy(Qt::NoFocus);
    QTest::touchEvent(window.data(), device.data()).press(0, QPoint(control->width() / 2, control->height() / 2));
    QTest::touchEvent(window.data(), device.data()).release(0, QPoint(control->width() / 2, control->height() / 2));
    QVERIFY(!control->hasActiveFocus());
    QVERIFY(!control->hasVisualFocus());

    control->setFocusPolicy(Qt::ClickFocus);
    QCOMPARE(control->focusPolicy(), Qt::ClickFocus);
    QTest::touchEvent(window.data(), device.data()).press(0, QPoint(control->width() / 2, control->height() / 2));
    QTest::touchEvent(window.data(), device.data()).release(0, QPoint(control->width() / 2, control->height() / 2));
    QVERIFY(control->hasActiveFocus());
    QVERIFY(!control->hasVisualFocus());

    // reset
    control->setFocus(false);
    QVERIFY(!control->hasActiveFocus());

    // Qt::WheelFocus
    QWheelEvent wheelEvent(QPointF(control->width() / 2, control->height() / 2), QPointF(),
                           QPoint(), QPoint(0, 10), Qt::NoButton, Qt::NoModifier,
                           Qt::NoScrollPhase, false);
    QGuiApplication::sendEvent(control, &wheelEvent);
    QVERIFY(!control->hasActiveFocus());
    QVERIFY(!control->hasVisualFocus());

    control->setFocusPolicy(Qt::WheelFocus);
    QCOMPARE(control->focusPolicy(), Qt::WheelFocus);

    QGuiApplication::sendEvent(control, &wheelEvent);
    QVERIFY(control->hasActiveFocus());
    QVERIFY(!control->hasVisualFocus());
}

void tst_focus::reason()
{
    QGuiApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusAllControls);
    auto resetTabFocusBehavior = qScopeGuard([]{
        QGuiApplication::styleHints()->setTabFocusBehavior(Qt::TabFocusBehavior(-1));
    });

    QQuickView view;
    view.setSource(testFileUrl("focusReason.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickControl *control = view.findChild<QQuickControl *>("control");
    QQuickControl *combobox = view.findChild<QQuickControl *>("combobox");
    QQuickControl *editcombo = view.findChild<QQuickControl *>("editcombo");
    QQuickControl *spinbox = view.findChild<QQuickControl *>("spinbox");
    QQuickControl *customText = view.findChild<QQuickControl *>("customText");
    QQuickControl *customItem = view.findChild<QQuickControl *>("customItem");
    // not a QQuickControl subclass
    QQuickItem *textfield = view.findChild<QQuickItem *>("textfield");

    // helper for clicking into a control
    const auto itemCenter = [](const QQuickItem *item) -> QPoint {
        return item->mapToScene(item->clipRect().center()).toPoint();
    };

    QVERIFY(control);
    QVERIFY(combobox);
    QVERIFY(editcombo);
    QVERIFY(spinbox);
    QVERIFY(textfield);
    QVERIFY(customText);
    QVERIFY(customItem);

    // setting focusPolicy to Strong/WheelFocus doesn't implicitly turn on event delivery
    customText->setAcceptedMouseButtons(Qt::LeftButton);
    customItem->setAcceptedMouseButtons(Qt::LeftButton);
    customItem->setWheelEnabled(true);

    // window activation -> ActiveWindowFocusReason
    QVERIFY(control->hasFocus());
    QVERIFY(control->hasActiveFocus());
    if (control->focusReason() != Qt::ActiveWindowFocusReason
     && QStringList{"windows", "offscreen"}.contains(QGuiApplication::platformName())) {
        QEXPECT_FAIL("", "On Windows and offscreen platforms, window activation does not set focus reason", Continue);
    }
    QCOMPARE(control->focusReason(), Qt::ActiveWindowFocusReason);

    // test setter/getter
    control->setFocus(false, Qt::MouseFocusReason);
    QCOMPARE(control->focusReason(), Qt::MouseFocusReason);
    control->setFocus(true, Qt::TabFocusReason);
    QCOMPARE(control->focusReason(), Qt::TabFocusReason);
    control->setFocus(false, Qt::BacktabFocusReason);
    QCOMPARE(control->focusReason(), Qt::BacktabFocusReason);
    control->forceActiveFocus(Qt::ShortcutFocusReason);
    QCOMPARE(control->focusReason(), Qt::ShortcutFocusReason);
    control->setFocusReason(Qt::PopupFocusReason);
    QCOMPARE(control->focusReason(), Qt::PopupFocusReason);

    // programmatic focus changes
    combobox->setFocus(true, Qt::OtherFocusReason);
    QCOMPARE(control->focusReason(), Qt::OtherFocusReason);

    QVERIFY(combobox->hasFocus());
    QVERIFY(combobox->hasActiveFocus());
    QCOMPARE(combobox->focusReason(), Qt::OtherFocusReason);

    // tab focus -> TabFocusReason
    QTest::keyClick(&view, Qt::Key_Tab);
    QVERIFY(editcombo->hasFocus());
    QVERIFY(editcombo->hasActiveFocus());
    QCOMPARE(qApp->focusObject(), editcombo->contentItem());
    QCOMPARE(combobox->focusReason(), Qt::TabFocusReason);
    QCOMPARE(editcombo->focusReason(), Qt::TabFocusReason);
    editcombo->setFocusReason(Qt::NoFocusReason); // reset so that we can verify that focusOut sets it

    QTest::keyClick(&view, Qt::Key_Tab);
    QVERIFY(spinbox->hasFocus());
    QVERIFY(spinbox->hasActiveFocus());
    QCOMPARE(qApp->focusObject(), spinbox->contentItem());
    QCOMPARE(editcombo->focusReason(), Qt::TabFocusReason);
    QCOMPARE(spinbox->focusReason(), Qt::TabFocusReason);
    spinbox->setFocusReason(Qt::NoFocusReason);

    QTest::keyClick(&view, Qt::Key_Tab);
    QVERIFY(customText->hasFocus());
    QVERIFY(customText->hasActiveFocus());
    QCOMPARE(qApp->focusObject(), customText);
    QCOMPARE(spinbox->focusReason(), Qt::TabFocusReason);
    QCOMPARE(customText->focusReason(), Qt::TabFocusReason);
    customText->setFocusReason(Qt::NoFocusReason);

    QTest::keyClick(&view, Qt::Key_Tab);
    QCOMPARE(qApp->focusObject(), customItem);
    QVERIFY(customItem->hasFocus());
    QVERIFY(customItem->hasActiveFocus());
    QCOMPARE(customText->focusReason(), Qt::TabFocusReason);
    QCOMPARE(customItem->focusReason(), Qt::TabFocusReason);
    customItem->setFocusReason(Qt::NoFocusReason);

    QTest::keyClick(&view, Qt::Key_Tab);
    QVERIFY(textfield->hasFocus());
    QVERIFY(textfield->hasActiveFocus());
    QCOMPARE(qApp->focusObject(), textfield);
    QCOMPARE(customItem->focusReason(), Qt::TabFocusReason);

    QTest::keyClick(&view, Qt::Key_Tab);
    QVERIFY(control->hasFocus());
    QVERIFY(control->hasActiveFocus());
    QCOMPARE(control->focusReason(), Qt::TabFocusReason);

    // backtab -> BacktabFocusReason
    QTest::keyClick(&view, Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(textfield->hasFocus());
    QCOMPARE(control->focusReason(), Qt::BacktabFocusReason);

    QTest::keyClick(&view, Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(customItem->hasFocus());
    QCOMPARE(customItem->focusReason(), Qt::BacktabFocusReason);

    QTest::keyClick(&view, Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(customText->hasFocus());
    QCOMPARE(customText->focusReason(), Qt::BacktabFocusReason);

    QTest::keyClick(&view, Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(spinbox->hasFocus());
    QCOMPARE(spinbox->focusReason(), Qt::BacktabFocusReason);

    QTest::keyClick(&view, Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(editcombo->hasFocus());
    QCOMPARE(editcombo->focusReason(), Qt::BacktabFocusReason);

    QTest::keyClick(&view, Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(combobox->hasFocus());
    QCOMPARE(combobox->focusReason(), Qt::BacktabFocusReason);

    // click focus -> MouseFocusReason
    QTest::mouseClick(&view, Qt::LeftButton, {}, itemCenter(editcombo));
    QTRY_VERIFY(editcombo->hasFocus());
    QVERIFY(editcombo->contentItem()->hasFocus());
    QVERIFY(editcombo->hasActiveFocus());
    QVERIFY(editcombo->contentItem()->hasActiveFocus());
    QCOMPARE(editcombo->focusReason(), Qt::MouseFocusReason);
    editcombo->setFocusReason(Qt::NoFocusReason);

    QTest::mouseClick(&view, Qt::LeftButton, {}, itemCenter(combobox)); // opens popup
    QTest::mouseClick(&view, Qt::LeftButton, {}, itemCenter(combobox)); // closes popup

    QVERIFY(combobox->hasFocus());
    QVERIFY(combobox->hasActiveFocus());
    QCOMPARE(editcombo->focusReason(), Qt::MouseFocusReason);
    QCOMPARE(combobox->focusReason(), Qt::MouseFocusReason);
    combobox->setFocusReason(Qt::NoFocusReason);

    QTest::mouseClick(&view, Qt::LeftButton, {}, itemCenter(spinbox));
    QVERIFY(spinbox->hasFocus());
    QVERIFY(spinbox->hasActiveFocus());
    QCOMPARE(combobox->focusReason(), Qt::MouseFocusReason);
    QCOMPARE(spinbox->focusReason(), Qt::MouseFocusReason);
    spinbox->setFocusReason(Qt::NoFocusReason);

    QTest::mouseClick(&view, Qt::LeftButton, {}, itemCenter(customText));
    QTRY_VERIFY2(customText->contentItem()->hasFocus(), qPrintable(qApp->focusObject()->objectName()));
    QVERIFY(customText->contentItem()->hasActiveFocus());
    QCOMPARE(spinbox->focusReason(), Qt::MouseFocusReason);
    QCOMPARE(customText->focusReason(), Qt::MouseFocusReason);
    customText->setFocusReason(Qt::NoFocusReason);

    QTest::mouseClick(&view, Qt::LeftButton, {}, itemCenter(customItem));
    QVERIFY(customItem->hasFocus());
    QVERIFY(customItem->hasActiveFocus());
    QCOMPARE(customText->focusReason(), Qt::MouseFocusReason);
    QCOMPARE(customItem->focusReason(), Qt::MouseFocusReason);
    customItem->setFocusReason(Qt::NoFocusReason);

    QTest::mouseClick(&view, Qt::LeftButton, {}, itemCenter(textfield));
    QCOMPARE(customItem->focusReason(), Qt::MouseFocusReason);
    customItem->setFocusReason(Qt::NoFocusReason);
    customText->setFocusReason(Qt::NoFocusReason);

    // Wheel focus -> MouseFocusReason
    QWheelEvent wheelEvent(QPointF(customItem->width() / 2, customItem->height() / 2), QPointF(),
                           QPoint(), QPoint(0, 10), Qt::NoButton, Qt::NoModifier,
                           Qt::NoScrollPhase, false);
    QGuiApplication::sendEvent(customItem, &wheelEvent);
    QVERIFY(customItem->hasActiveFocus());
    QCOMPARE(customItem->focusReason(), Qt::MouseFocusReason);

    // Popup opens -> PopupFocusReason
    QTest::mouseClick(&view, Qt::RightButton, {}, itemCenter(control));
    QTRY_VERIFY(!customItem->hasActiveFocus());
    QCOMPARE(customItem->focusReason(), Qt::PopupFocusReason);
    QTest::keyClick(&view, Qt::Key_Escape); // close the popup
}

void tst_focus::visualFocus()
{
    QQuickView view;
    view.setSource(testFileUrl("visualFocus.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickItem *column = view.rootObject();
    QVERIFY(column);
    QCOMPARE(column->childItems().size(), 2);

    QQuickControl *button = qobject_cast<QQuickControl *>(column->childItems().first());
    QVERIFY(button);

    QQuickItem *textfield = column->childItems().last();
    QVERIFY(textfield);

    button->forceActiveFocus(Qt::TabFocusReason);
    QVERIFY(button->hasActiveFocus());
    QVERIFY(button->hasVisualFocus());
    QVERIFY(button->property("showFocus").toBool());

    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, QPoint(textfield->x() + textfield->width() / 2, textfield->y() + textfield->height() / 2));
    QVERIFY(!button->hasActiveFocus());
    QVERIFY(!button->hasVisualFocus());
    QVERIFY(!button->property("showFocus").toBool());
}

void tst_focus::scope_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("Frame") << "Frame";
    QTest::newRow("GroupBox") << "Frame";
    QTest::newRow("Page") << "Page";
    QTest::newRow("Pane") << "Pane";
    QTest::newRow("StackView") << "StackView";
}

void tst_focus::scope()
{
    QFETCH(QString, name);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QString("import QtQuick; import QtQuick.Controls; ApplicationWindow { property alias child: child; width: 100; height: 100; %1 { anchors.fill: parent; Item { id: child; width: 10; height: 10 } } }").arg(name).toUtf8(), QUrl());

    QScopedPointer<QPointingDevice> device(QTest::createTouchDevice());
    QScopedPointer<QQuickApplicationWindow> window(qobject_cast<QQuickApplicationWindow *>(component.create()));
    QVERIFY2(window, qPrintable(component.errorString()));

    QQuickControl *control = qobject_cast<QQuickControl *>(window->contentItem()->childItems().first());
    QVERIFY(control);

    control->setFocusPolicy(Qt::WheelFocus);
    control->setAcceptedMouseButtons(Qt::LeftButton);

    QQuickItem *child = window->property("child").value<QQuickItem *>();
    QVERIFY(child);

    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));


    child->forceActiveFocus();
    QVERIFY(child->hasActiveFocus());
    QVERIFY(control->hasActiveFocus());

    // Qt::ClickFocus (mouse)
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(control->width() / 2, control->height() / 2));
    QVERIFY(!child->hasActiveFocus());
    QVERIFY(control->hasActiveFocus());

    // reset
    child->forceActiveFocus();
    QVERIFY(child->hasActiveFocus());
    QVERIFY(control->hasActiveFocus());

    // Qt::ClickFocus (touch)
    QTest::touchEvent(window.data(), device.data()).press(0, QPoint(control->width() / 2, control->height() / 2));
    QTest::touchEvent(window.data(), device.data()).release(0, QPoint(control->width() / 2, control->height() / 2));
    QVERIFY(!child->hasActiveFocus());
    QVERIFY(control->hasActiveFocus());

    // reset
    child->forceActiveFocus();
    QVERIFY(child->hasActiveFocus());
    QVERIFY(control->hasActiveFocus());

    // Qt::WheelFocus
    QWheelEvent wheelEvent(QPointF(control->width() / 2, control->height() / 2), QPointF(),
                           QPoint(), QPoint(0, 10), Qt::NoButton, Qt::NoModifier,
                           Qt::NoScrollPhase, false);
    QGuiApplication::sendEvent(control, &wheelEvent);
    QVERIFY(!child->hasActiveFocus());
    QVERIFY(control->hasActiveFocus());
}

QTEST_MAIN(tst_focus)

#include "tst_focus.moc"
