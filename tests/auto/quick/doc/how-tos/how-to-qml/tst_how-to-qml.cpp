// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qregularexpression.h>
#include <QtTest/QtTest>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquickrepeater_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/dialogstestutils_p.h>

QT_BEGIN_NAMESPACE

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;
using namespace QQuickDialogTestUtils;

// Allows us to use test macros outside of test functions.
#define RETURN_IF_FAILED(expression) \
expression; \
if (QTest::currentTestFailed()) \
    return

class tst_HowToQml : public QObject
{
    Q_OBJECT

public:
    tst_HowToQml();

private slots:
    void init();
    void activeFocusDebugging();
    void timePicker();

private:
    QScopedPointer<QPointingDevice> touchScreen = QScopedPointer<QPointingDevice>(QTest::createTouchDevice());
};

tst_HowToQml::tst_HowToQml()
{
    qputenv("QML_NO_TOUCH_COMPRESSION", "1");
}

void tst_HowToQml::init()
{
//    QTest::failOnWarning(QRegularExpression(QStringLiteral(".?")));
}

void tst_HowToQml::activeFocusDebugging()
{
    QQmlApplicationEngine engine;
    engine.loadFromModule("HowToQml", "ActiveFocusDebuggingMain");
    QCOMPARE(engine.rootObjects().size(), 1);

    auto *window = qobject_cast<QQuickWindow*>(engine.rootObjects().at(0));
    window->show();
    QTest::ignoreMessage(QtDebugMsg, QRegularExpression("activeFocusItem: .*\"ActiveFocusDebuggingMain\""));
    QVERIFY(QTest::qWaitForWindowActive(window));

    QTest::ignoreMessage(QtDebugMsg, QRegularExpression("activeFocusItem: .*\"textField1\""));
    auto *textField1 = window->findChild<QQuickTextField*>("textField1");
    QVERIFY(textField1);
    textField1->forceActiveFocus();
    QVERIFY(textField1->hasActiveFocus());

    QTest::ignoreMessage(QtDebugMsg, QRegularExpression("activeFocusItem: .*\"textField2\""));
    auto *textField2 = window->findChild<QQuickTextField*>("textField2");
    QVERIFY(textField2);
    QTest::keyClick(window, Qt::Key_Tab);
    QVERIFY(textField2->hasActiveFocus());
}

void tst_HowToQml::timePicker()
{
    QQmlApplicationEngine engine;
    engine.loadFromModule("HowToQml", "TimePickerMain");
    QCOMPARE(engine.rootObjects().size(), 1);

    auto *window = qobject_cast<QQuickWindow*>(engine.rootObjects().at(0));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *dialog = window->findChild<QQuickDialog *>();
    QVERIFY(dialog);

    auto *timePicker = dialog->findChild<QQuickItem *>("timePicker");
    QVERIFY(timePicker);

    auto *contentContainer = timePicker->findChild<QQuickItem *>("contentContainer");
    QVERIFY(contentContainer);
    const int contentRadius = contentContainer->property("radius").toReal();

    auto *labelRepeater = timePicker->findChild<QQuickRepeater *>();
    QVERIFY(labelRepeater);

    auto *selectionArm = timePicker->findChild<QQuickItem *>("selectionArm");
    QVERIFY(selectionArm);

    auto *selectionIndicator = selectionArm->findChild<QQuickItem *>("selectionIndicator");
    QVERIFY(selectionIndicator);
    const int selectionIndicatorHeight = selectionIndicator->height();

    auto angleForValue = [](int value) -> int {
        return int((value / 60.0) * 360) % 360;
    };

    // Note that is24HourValue should be true if "value" is a 24-hour value,
    // not if the picker's is24Hour property is true.
    auto labelCenterPosForValue = [&](int value, bool is24HourValue = false) -> QPoint {
        if (value < 0 || value > 60)
            return {};

        const qreal angle = angleForValue(value);

        QTransform transform;
        // Translate to the center.
        transform.translate(contentRadius, contentRadius);
        // Rotate to the correct angle.
        transform.rotate(angle);
        // Go outward.
        const int labelDistance = is24HourValue ? selectionIndicatorHeight * 1.5 : selectionIndicatorHeight * 0.5;
        transform.translate(0, -contentRadius + labelDistance);

        const auto centerPos = transform.map(QPoint(0, 0));
        return centerPos;
    };

    enum Mode {
        Hours,
        Minutes
    };

    const int valuesPerLabelStep = 5;
    const bool TwelveHour = false;
    const bool TwentyFourHour = true;

    // Checks that all the labels are in their expected positions and that they have the correct text.
    auto verifyLabels = [&](Mode expectedMode, bool is24Hour, int callerLineNumber) {
        // When not in 24 hour mode, there are always 12 labels, regardless of whether it's showing hours or minutes.
        const int expectedLabelCount = expectedMode == Hours && is24Hour ? 24 : 12;
        for (int i = 0; i < expectedLabelCount; ++i) {
            auto *labelDelegate = labelRepeater->itemAt(i);
            QVERIFY2(labelDelegate, qPrintable(QString::fromLatin1("Expected valid label delegate item at index %1 (caller line %2)")
                .arg(i).arg(callerLineNumber)));
            // Use the waiting variant of the macro because there are opacity animations.
            // TODO: is this causing the failure on line 224?
            QTRY_VERIFY2(qFuzzyCompare(labelDelegate->opacity(), 1.0), qPrintable(QString::fromLatin1(
                "Expected label opacity at index %1 to be 1 but it's %2 (caller line %3)").arg(i)
                .arg(labelDelegate->opacity()).arg(callerLineNumber)));

            const int expectedValue = (i * valuesPerLabelStep) % 60;
            const int actualValue = labelDelegate->property("value").toInt();
            QVERIFY2(expectedValue == actualValue, qPrintable(QString::fromLatin1(
                "Expected label's value at index %1 to be %2 but it's %3 (caller line %4)")
                .arg(i).arg(expectedValue).arg(actualValue).arg(callerLineNumber)));

            const QString expectedText = QString::number(expectedMode == Hours
                ? (i == 0 ? 12 : (i == 12 ? 0 : i)) : i * valuesPerLabelStep);
            // Use QTRY_VERIFY2 rather than QVERIFY2, because text changes are animated.
            QTRY_VERIFY2(expectedText == labelDelegate->property("text").toString(), qPrintable(QString::fromLatin1(
                "Expected label's text at index %1 to be %2 but it's %3 (caller line %4)").arg(i)
                .arg(expectedText, labelDelegate->property("text").toString(), QString::number(callerLineNumber))));
        }
    };

    auto verifySelectionIndicator = [&](int expectedValue, bool expect24HourValue, int callerLineNumber) {
        const qreal actualRotation = int(selectionArm->rotation()) % 360;
        const qreal expectedRotation = angleForValue(expectedValue);
        QVERIFY2(qFuzzyCompare(actualRotation, expectedRotation), qPrintable(QString::fromLatin1(
            "Expected selection arm's rotation to be %1 for expectedValue %2 but it's %3 (caller line %4)")
            .arg(expectedRotation).arg(expectedValue).arg(actualRotation).arg(callerLineNumber)));

        const QPoint expectedIndicatorCenterPos = labelCenterPosForValue(expectedValue, expect24HourValue);
        const QPoint actualIndicatorCenterPos = selectionIndicator->mapToItem(
            contentContainer, selectionIndicator->boundingRect().center().toPoint()).toPoint();
        const QPoint difference = actualIndicatorCenterPos - expectedIndicatorCenterPos;
        QVERIFY2(difference.x() <= 2 && difference.y() <= 2, qPrintable(QString::fromLatin1(
            "Expected selection indicator's center position to be %1 (with 2 pixels of tolerance) but it's %2 (caller line %3)")
            .arg(QDebug::toString(expectedIndicatorCenterPos), QDebug::toString(actualIndicatorCenterPos)).arg(callerLineNumber)));
    };

    auto valueForHour = [&](int hour) {
        return (hour * valuesPerLabelStep) % 60;
    };

    // Open the picker to hours mode by clicking on the label.
    auto *openDialogLabel = window->findChild<QQuickLabel *>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(openDialogLabel);
    QCOMPARE(openDialogLabel->text(), "12:00");
    QTest::touchEvent(window, touchScreen.data()).press(0, mapCenterToWindow(openDialogLabel));
    QTest::touchEvent(window, touchScreen.data()).release(0, mapCenterToWindow(openDialogLabel));
    QTRY_COMPARE(dialog->property("opened").toBool(), true);
    // It should show hours.
    RETURN_IF_FAILED(verifyLabels(Hours, TwelveHour, __LINE__));
    RETURN_IF_FAILED(verifySelectionIndicator(0, TwelveHour, __LINE__));

    // Select the 3rd hour.
    const QPoint thirdHourPos = labelCenterPosForValue(valueForHour(3));
    QTest::touchEvent(window, touchScreen.data()).press(0, mapToWindow(contentContainer, thirdHourPos));
    RETURN_IF_FAILED(verifySelectionIndicator(valueForHour(3), TwelveHour, __LINE__));
    QCOMPARE(timePicker->property("mode").toInt(), Hours);
    QTest::touchEvent(window, touchScreen.data()).release(0, mapToWindow(contentContainer, thirdHourPos));
    QCOMPARE(timePicker->property("hours").toInt(), 3);
    QCOMPARE(timePicker->property("minutes").toInt(), 0);
    // The dialog's values shouldn't change until the dialog has been accepted.
    QCOMPARE(dialog->property("hours").toInt(), 12);
    QCOMPARE(dialog->property("minutes").toInt(), 0);
    // It should be showing minutes now.
    QCOMPARE(timePicker->property("mode").toInt(), Minutes);
    RETURN_IF_FAILED(verifyLabels(Minutes, TwelveHour, __LINE__));
    RETURN_IF_FAILED(verifySelectionIndicator(0, TwelveHour, __LINE__));
    auto *minutesLabel = dialog->findChild<QQuickLabel *>("minutesLabel");
    QVERIFY(minutesLabel);
    QCOMPARE(minutesLabel->property("dim").toBool(), false);

    // Select the 59th minute.
    const QPoint fiftyNinthMinutePos = labelCenterPosForValue(59);
    QTest::touchEvent(window, touchScreen.data()).press(0, mapToWindow(contentContainer, fiftyNinthMinutePos));
    RETURN_IF_FAILED(verifySelectionIndicator(59, TwelveHour, __LINE__));
    QTest::touchEvent(window, touchScreen.data()).release(0, mapToWindow(contentContainer, fiftyNinthMinutePos));
    QCOMPARE(timePicker->property("hours").toInt(), 3);
    QCOMPARE(timePicker->property("minutes").toInt(), 59);
    QCOMPARE(dialog->property("hours").toInt(), 12);
    QCOMPARE(dialog->property("minutes").toInt(), 0);
    // It shouldn't be closed until the OK or Cancel buttons are clicked.
    QCOMPARE(dialog->property("opened").toBool(), true);

    // Accept the dialog to make the changes.
    auto *dialogButtonBox = qobject_cast<QQuickDialogButtonBox *>(dialog->footer());
    QVERIFY(dialogButtonBox);
    auto *okButton = findDialogButton(dialogButtonBox, "OK");
    QTest::ignoreMessage(QtDebugMsg, "A time was chosen - do something here!");
    QVERIFY(clickButton(okButton));
    QTRY_COMPARE(dialog->property("visible").toBool(), false);
    QCOMPARE(dialog->property("hours").toInt(), 3);
    QCOMPARE(dialog->property("minutes").toInt(), 59);
    QCOMPARE(openDialogLabel->text(), "03:59");

    // Open it again.
    QTest::touchEvent(window, touchScreen.data()).press(0, mapCenterToWindow(openDialogLabel));
    QTest::touchEvent(window, touchScreen.data()).release(0, mapCenterToWindow(openDialogLabel));
    QTRY_COMPARE(dialog->property("opened"), true);
    RETURN_IF_FAILED(verifyLabels(Hours, TwelveHour, __LINE__));
    RETURN_IF_FAILED(verifySelectionIndicator(valueForHour(3), TwelveHour, __LINE__));
    // The time label should be unchanged.
    QCOMPARE(openDialogLabel->text(), "03:59");

    // Switch from hours to minutes by clicking on the minutes label.
    QTest::touchEvent(window, touchScreen.data()).press(0, mapCenterToWindow(minutesLabel));
    QTest::touchEvent(window, touchScreen.data()).release(0, mapCenterToWindow(minutesLabel));
    RETURN_IF_FAILED(verifyLabels(Minutes, TwelveHour, __LINE__));
    RETURN_IF_FAILED(verifySelectionIndicator(59, TwelveHour, __LINE__));

    // Select the 1st minute.
    const QPoint firstMinutePos = labelCenterPosForValue(1);
    QTest::touchEvent(window, touchScreen.data()).press(0, mapToWindow(contentContainer, firstMinutePos));
    RETURN_IF_FAILED(verifySelectionIndicator(1, TwelveHour, __LINE__));
    QTest::touchEvent(window, touchScreen.data()).release(0, mapToWindow(contentContainer, firstMinutePos));
    QCOMPARE(timePicker->property("hours").toInt(), 3);
    QCOMPARE(timePicker->property("minutes").toInt(), 1);
    // It shouldn't be closed until the OK or Cancel buttons are clicked.
    QCOMPARE(dialog->property("opened").toBool(), true);

    // Accept the dialog to make the changes.
    QTest::ignoreMessage(QtDebugMsg, "A time was chosen - do something here!");
    QVERIFY(clickButton(okButton));
    QTRY_COMPARE(dialog->property("visible").toBool(), false);
    QCOMPARE(dialog->property("hours").toInt(), 3);
    QCOMPARE(dialog->property("minutes").toInt(), 1);
    QCOMPARE(openDialogLabel->text(), "03:01");

    // Check that hours and minutes set programmatically on the picker and dialog are respected.
    QVERIFY(dialog->setProperty("hours", QVariant::fromValue(7)));
    QVERIFY(dialog->setProperty("minutes", QVariant::fromValue(8)));
    QCOMPARE(openDialogLabel->text(), "07:08");
    // Open the picker to hours mode by clicking on the label.
    QTest::touchEvent(window, touchScreen.data()).press(0, mapCenterToWindow(openDialogLabel));
    QTest::touchEvent(window, touchScreen.data()).release(0, mapCenterToWindow(openDialogLabel));
    QTRY_COMPARE(dialog->property("opened").toBool(), true);
    RETURN_IF_FAILED(verifyLabels(Hours, TwelveHour, __LINE__));
    RETURN_IF_FAILED(verifySelectionIndicator(valueForHour(7), TwelveHour, __LINE__));
    QCOMPARE(timePicker->property("hours").toInt(), 7);
    QCOMPARE(timePicker->property("minutes").toInt(), 8);

    // Check that cancelling the dialog cancels any changes.
    // Select the fourth hour.
    const QPoint fourthHourPos = labelCenterPosForValue(20);
    QTest::touchEvent(window, touchScreen.data()).press(0, mapToWindow(contentContainer, fourthHourPos));
    RETURN_IF_FAILED(verifySelectionIndicator(valueForHour(4), TwelveHour, __LINE__));
    QTest::touchEvent(window, touchScreen.data()).release(0, mapToWindow(contentContainer, fourthHourPos));
    QCOMPARE(timePicker->property("hours").toInt(), 4);
    QCOMPARE(timePicker->property("minutes").toInt(), 8);
    auto *cancelButton = findDialogButton(dialogButtonBox, "Cancel");
    QVERIFY(clickButton(cancelButton));
    QTRY_COMPARE(dialog->property("visible").toBool(), false);
    QCOMPARE(dialog->property("hours").toInt(), 7);
    QCOMPARE(dialog->property("minutes").toInt(), 8);

    // Test that the 24 hour mode works.
    QVERIFY(dialog->setProperty("is24Hour", QVariant::fromValue(true)));
    QTest::touchEvent(window, touchScreen.data()).press(0, mapCenterToWindow(openDialogLabel));
    QTest::touchEvent(window, touchScreen.data()).release(0, mapCenterToWindow(openDialogLabel));
    QTRY_COMPARE(dialog->property("opened").toBool(), true);
    QCOMPARE(timePicker->property("mode").toInt(), Hours);
    RETURN_IF_FAILED(verifyLabels(Hours, TwentyFourHour, __LINE__));
    // TwelveHour because the selected value (7) is on the 12 hour "ring".
    RETURN_IF_FAILED(verifySelectionIndicator(valueForHour(7), TwelveHour, __LINE__));
    // It should still show the old time.
    QCOMPARE(timePicker->property("hours").toInt(), 7);
    QCOMPARE(timePicker->property("minutes").toInt(), 8);
    QCOMPARE(dialog->property("hours").toInt(), 7);
    QCOMPARE(dialog->property("minutes").toInt(), 8);

    // Select the 23rd hour.
    const QPoint twentyThirdHourPos = labelCenterPosForValue(valueForHour(11), TwentyFourHour);
    QTest::touchEvent(window, touchScreen.data()).press(0, mapToWindow(contentContainer, twentyThirdHourPos));
    RETURN_IF_FAILED(verifySelectionIndicator(valueForHour(23), TwentyFourHour, __LINE__));
    QTest::touchEvent(window, touchScreen.data()).release(0, mapToWindow(contentContainer, twentyThirdHourPos));
    QCOMPARE(timePicker->property("hours").toInt(), 23);
    QCOMPARE(timePicker->property("minutes").toInt(), 8);
    QCOMPARE(dialog->property("hours").toInt(), 7);
    QCOMPARE(dialog->property("minutes").toInt(), 8);
    RETURN_IF_FAILED(verifyLabels(Minutes, TwentyFourHour, __LINE__));
    RETURN_IF_FAILED(verifySelectionIndicator(8, TwelveHour, __LINE__));

    // Select the 20th minute.
    const QPoint twentiethMinutePos = labelCenterPosForValue(20);
    QTest::touchEvent(window, touchScreen.data()).press(0, mapToWindow(contentContainer, twentiethMinutePos));
    RETURN_IF_FAILED(verifySelectionIndicator(20, TwelveHour, __LINE__));
    QTest::touchEvent(window, touchScreen.data()).release(0, mapToWindow(contentContainer, twentiethMinutePos));
    QCOMPARE(timePicker->property("hours").toInt(), 23);
    QCOMPARE(timePicker->property("minutes").toInt(), 20);

    // Go back to hours and make sure that the selection indicator is correct.
    auto *hoursLabel = dialog->findChild<QQuickLabel *>("hoursLabel");
    QVERIFY(hoursLabel);
    QTest::touchEvent(window, touchScreen.data()).press(0, mapCenterToWindow(hoursLabel));
    QTest::touchEvent(window, touchScreen.data()).release(0, mapCenterToWindow(hoursLabel));
    RETURN_IF_FAILED(verifyLabels(Hours, TwentyFourHour, __LINE__));
    RETURN_IF_FAILED(verifySelectionIndicator(valueForHour(23), TwentyFourHour, __LINE__));

    // Accept.
    QTest::ignoreMessage(QtDebugMsg, "A time was chosen - do something here!");
    QVERIFY(clickButton(okButton));
    QTRY_COMPARE(dialog->property("visible").toBool(), false);
    QCOMPARE(dialog->property("hours").toInt(), 23);
    QCOMPARE(dialog->property("minutes").toInt(), 20);
}

QT_END_NAMESPACE

QTEST_MAIN(tst_HowToQml)

#include "tst_how-to-qml.moc"
