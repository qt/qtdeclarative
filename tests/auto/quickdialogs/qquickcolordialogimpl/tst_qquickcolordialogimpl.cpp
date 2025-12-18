// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtQuickTest/quicktest.h>
#include <QtTest/qsignalspy.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/dialogstestutils_p.h>
#include <QtQuickDialogs2/private/qquickcolordialog_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickabstractcolorpicker_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickcolordialogimpl_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickcolorinputs_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>
#include <QtQuickTemplates2/private/qquickslider_p.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickControls2/qquickstyle.h>

#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickDialogTestUtils;
using namespace QQuickControlsTestUtils;

class tst_QQuickColorDialogImpl : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickColorDialogImpl();
    static void initMain()
    {
        // We need to set this attribute.
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        // We don't want to run this test for every style, as each one will have
        // different ways of implementing the dialogs.
        // For now we only test one style.
        QQuickStyle::setStyle("Basic");
    }

private slots:
    void defaults();
    void moveColorPickerHandle();
    void alphaChannel_data();
    void alphaChannel();
    void changeHex();
    void changeColorFromTextFields_data();
    void changeColorFromTextFields();
    void windowTitle_data();
    void windowTitle();
    void workingInsideQQuickViewer_data();
    void workingInsideQQuickViewer();
    void dialogCanMoveBetweenWindows();
    void checkModality_data();
    void checkModality();

private:
    bool closePopup(DialogTestHelper<QQuickColorDialog, QQuickColorDialogImpl> *dialogHelper,
                    QString dialogButton, QString &failureMessage)
    {
        auto dialogButtonBox =
                dialogHelper->quickDialog->footer()->findChild<QQuickDialogButtonBox *>();

        if (!dialogButtonBox) {
            failureMessage = QLatin1String("dialogButtonBox is null");
            return false;
        }

        QQuickAbstractButton *openButton = findDialogButton(dialogButtonBox, dialogButton);
        if (!openButton) {
            failureMessage = QLatin1String("Couldn't find a button with text '%1'").arg(dialogButton);
            return false;
        }

        const bool clicked = clickButton(openButton);
        if (!clicked) {
            failureMessage = QLatin1String("'%1' button was never clicked").arg(dialogButton);
            return false;
        }

        const bool visible = dialogHelper->dialog->isVisible();
        if (visible) {
            failureMessage = QLatin1String("Dialog is still visible after clicking the '%1' button").arg(dialogButton);
            return false;
        }
        return true;
    }
};

#define CLOSE_DIALOG(BUTTON)                                                                       \
    QString errorMessage;                                                                          \
    QVERIFY2(closePopup(&dialogHelper, BUTTON, errorMessage), qPrintable(errorMessage));           \
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible())

#define FUZZYCOMPARE(ACTUAL, EXPECTED, TOLERANCE, ERRORMSG)                                        \
    QVERIFY2(qAbs(ACTUAL - EXPECTED) < TOLERANCE, ERRORMSG)

tst_QQuickColorDialogImpl::tst_QQuickColorDialogImpl() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

void tst_QQuickColorDialogImpl::defaults()
{
    QTest::failOnWarning(QRegularExpression(".*"));
    DialogTestHelper<QQuickColorDialog, QQuickColorDialogImpl> dialogHelper(this, "colorDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    QVERIFY(dialogHelper.waitForPopupWindowActiveAndPolished());

    QQuickAbstractColorPicker *colorPicker =
            dialogHelper.quickDialog->findChild<QQuickAbstractColorPicker *>("colorPicker");
    QVERIFY(colorPicker);

    QQuickSlider *alphaSlider = dialogHelper.quickDialog->findChild<QQuickSlider *>("alphaSlider");
    QVERIFY(alphaSlider);

    QCOMPARE(dialogHelper.dialog->selectedColor().rgba(), QColorConstants::White.rgba());
    QCOMPARE(dialogHelper.quickDialog->color().rgba(), QColorConstants::White.rgba());
    QCOMPARE(colorPicker->color().rgba(), QColorConstants::White.rgba());

    QVERIFY2(!alphaSlider->isVisible(),
             "The AlphaSlider should not be visible unless the ShowAlphaChannel options has "
             "explicitly been set");
    dialogHelper.popupWindow()->close();
    QTRY_VERIFY(!dialogHelper.isQuickDialogOpen());
    dialogHelper.dialog->setOptions(QColorDialogOptions::ShowAlphaChannel);
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    QVERIFY(dialogHelper.waitForPopupWindowActiveAndPolished());
    QVERIFY(alphaSlider->isVisible());

    const bool wayland = QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive) == 0;
    const bool offscreen = qgetenv("QT_QPA_PLATFORM").compare(QLatin1String("offscreen"), Qt::CaseInsensitive) == 0;

    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ScreenWindowGrabbing) && !wayland && !offscreen) {
        QQuickButton *eyeDropperButton = dialogHelper.quickDialog->findChild<QQuickButton *>("eyeDropperButton");
        QVERIFY(eyeDropperButton);

        QVERIFY2(eyeDropperButton->isVisible(),
                 "The Eye Dropper Button should be visible unless the NoEyeDropperButton option has "
                 "explicitly been set");
        dialogHelper.popupWindow()->close();
        QTRY_VERIFY(!dialogHelper.isQuickDialogOpen());
        dialogHelper.dialog->setOptions(QColorDialogOptions::NoEyeDropperButton);
        QVERIFY(dialogHelper.openDialog());
        QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
        QVERIFY(dialogHelper.waitForPopupWindowActiveAndPolished());
        QVERIFY(!eyeDropperButton->isVisible());
    }

    QVERIFY(dialogHelper.quickDialog->isHsl());
    QCOMPARE(dialogHelper.quickDialog->alpha(), 1.0);
    QCOMPARE(dialogHelper.quickDialog->hue(), 0.0);
    QCOMPARE(dialogHelper.quickDialog->saturation(), 0.0);
    QCOMPARE(dialogHelper.quickDialog->value(), 1.0);
    QCOMPARE(dialogHelper.quickDialog->lightness(), 1.0);
    QCOMPARE(colorPicker->alpha(), 1.0);
    QCOMPARE(colorPicker->hue(), 0.0);
    QCOMPARE(colorPicker->saturation(), 0.0);
    QCOMPARE(colorPicker->value(), 1.0);
    QCOMPARE(colorPicker->lightness(), 1.0);

    CLOSE_DIALOG("Ok");
}

void tst_QQuickColorDialogImpl::moveColorPickerHandle()
{
    DialogTestHelper<QQuickColorDialog, QQuickColorDialogImpl> dialogHelper(this, "colorDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    QVERIFY(dialogHelper.waitForPopupWindowActiveAndPolished());

    QQuickAbstractColorPicker *colorPicker =
            dialogHelper.quickDialog->findChild<QQuickAbstractColorPicker *>("colorPicker");
    QVERIFY(colorPicker);
    QQuickSlider *hueSlider = dialogHelper.quickDialog->findChild<QQuickSlider *>("hueSlider");
    QVERIFY(hueSlider);

    QSignalSpy colorChangedSpy(colorPicker, SIGNAL(colorChanged(QColor)));

    const QPoint topCenter = colorPicker->mapToScene({ colorPicker->width() / 2, 0 }).toPoint();

    // Move handle to where the saturation is the highest and the lightness is 'neutral'
    QTest::mouseClick(dialogHelper.popupWindow(), Qt::LeftButton, Qt::NoModifier, topCenter);

    QCOMPARE(colorChangedSpy.size(), 1);

    const qreal floatingPointComparisonThreshold = 1.0 / colorPicker->width();
    const QString floatComparisonErrorString(
            "%1 return value of %2 wasn't close enough to %3. A threshold of %4 is used to decide if the floating "
            "point value is close enough.");
    const QString colorComparisonErrorString("The expected color value for %1, didn't match the expected value of %2, was %3.");

    FUZZYCOMPARE(colorPicker->saturation(), 1.0, floatingPointComparisonThreshold,
                qPrintable(floatComparisonErrorString
                .arg("saturation()")
                .arg(colorPicker->saturation())
                .arg(1.0)
                .arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(dialogHelper.quickDialog->saturation(), 1.0, floatingPointComparisonThreshold,
                qPrintable(floatComparisonErrorString
                .arg("saturation()")
                .arg(dialogHelper.quickDialog->saturation())
                .arg(1.0)
                .arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(colorPicker->lightness(), 0.5, floatingPointComparisonThreshold,
                qPrintable(floatComparisonErrorString
                .arg("lightness()")
                .arg(colorPicker->lightness())
                .arg(0.5)
                .arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(dialogHelper.quickDialog->lightness(), 0.5, floatingPointComparisonThreshold,
                qPrintable(floatComparisonErrorString
                .arg("lightness()")
                .arg(dialogHelper.quickDialog->lightness())
                .arg(0.5)
                .arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(dialogHelper.quickDialog->color().red(), QColorConstants::Red.red(), 2,
                qPrintable(colorComparisonErrorString
                .arg("red")
                .arg(QColorConstants::Red.red())
                .arg(dialogHelper.quickDialog->color().red())));
    FUZZYCOMPARE(dialogHelper.quickDialog->color().green(), QColorConstants::Red.green(), 2,
                qPrintable(colorComparisonErrorString
                .arg("green")
                .arg(QColorConstants::Red.green())
                .arg(dialogHelper.quickDialog->color().green())));
    FUZZYCOMPARE(dialogHelper.quickDialog->color().blue(), QColorConstants::Red.blue(), 2,
                qPrintable(colorComparisonErrorString
                .arg("blue")
                .arg(QColorConstants::Red.blue())
                .arg(dialogHelper.quickDialog->color().blue())));
    FUZZYCOMPARE(colorPicker->color().red(), QColorConstants::Red.red(), 2,
                qPrintable(colorComparisonErrorString
                .arg("red")
                .arg(QColorConstants::Red.red())
                .arg(colorPicker->color().red())));
    FUZZYCOMPARE(colorPicker->color().green(), QColorConstants::Red.green(), 2,
                qPrintable(colorComparisonErrorString
                .arg("green")
                .arg(QColorConstants::Red.green())
                .arg(colorPicker->color().green())));
    FUZZYCOMPARE(colorPicker->color().blue(), QColorConstants::Red.blue(), 2,
                qPrintable(colorComparisonErrorString
                .arg("blue")
                .arg(QColorConstants::Red.blue())
                .arg(colorPicker->color().blue())));

    const QPoint hueSliderCenterPosition = hueSlider->mapToScene({ hueSlider->width() / 2, hueSlider->height() / 2 }).toPoint();
    const qreal cyanHue = QColorConstants::Cyan.hslHueF();
    const qreal floatComparisonThresholdForHueSlider = 1.0 / hueSlider->width();
    QTest::mouseClick(dialogHelper.popupWindow(), Qt::LeftButton, Qt::NoModifier, hueSliderCenterPosition);
    FUZZYCOMPARE(hueSlider->value(), cyanHue, floatComparisonThresholdForHueSlider,
                qPrintable(floatComparisonErrorString
                .arg("Slider::value()")
                .arg(hueSlider->value())
                .arg(cyanHue)
                .arg(floatComparisonThresholdForHueSlider)));
    FUZZYCOMPARE(colorPicker->hue(), cyanHue, floatComparisonThresholdForHueSlider,
                qPrintable(floatComparisonErrorString
                .arg("QQuickAbstractColorPicker::hue()")
                .arg(colorPicker->hue())
                .arg(cyanHue)
                .arg(floatComparisonThresholdForHueSlider)));

    FUZZYCOMPARE(dialogHelper.quickDialog->hue(), cyanHue, floatComparisonThresholdForHueSlider,
                qPrintable(floatComparisonErrorString
                .arg("QQuickColorDialogImpl::hue()")
                .arg(dialogHelper.quickDialog->hue())
                .arg(cyanHue)
                .arg(floatComparisonThresholdForHueSlider)));

    FUZZYCOMPARE(colorPicker->color().red(), QColorConstants::Cyan.red(), 3,
                qPrintable(colorComparisonErrorString.arg("red")
                .arg(QColorConstants::Cyan.red())
                .arg(colorPicker->color().red())));

    if (QSysInfo::productType() == "opensuse-leap" && QSysInfo::productVersion() == QLatin1String("16.0"))
        QEXPECT_FAIL("", "QTBUG-142386: opensuse-leap 16.0 fails", Continue);

    FUZZYCOMPARE(colorPicker->color().green(), QColorConstants::Cyan.green(), 3,
                qPrintable(colorComparisonErrorString.arg("green")
                .arg(QColorConstants::Cyan.green())
                .arg(colorPicker->color().green())));
    FUZZYCOMPARE(colorPicker->color().blue(), QColorConstants::Cyan.blue(), 3,
                qPrintable(colorComparisonErrorString
                .arg("blue")
                .arg(QColorConstants::Cyan.blue())
                .arg(colorPicker->color().blue())));

    QCOMPARE(colorChangedSpy.size(), 2);

    QPoint bottomCenter = colorPicker->mapToScene({ colorPicker->width() / 2, colorPicker->height() }).toPoint();

    // Move the handle to where the saturation is the lowest, without affecting lightness
    QTest::mousePress(dialogHelper.popupWindow(), Qt::LeftButton, Qt::NoModifier,
                      { bottomCenter.x(), bottomCenter.y() - 1 });
    QTest::mouseRelease(dialogHelper.popupWindow(), Qt::LeftButton, Qt::NoModifier, bottomCenter);

    // The press and release happened in 2 different positions.
    // This means that the current color was changed twice.
    // (The press happens 1 pixel above the release, to work around an issue where the mouse event
    // wasn't received by the color picker)
    QCOMPARE(colorChangedSpy.size(), 4);
    FUZZYCOMPARE(colorPicker->saturation(), 0.0, floatingPointComparisonThreshold,
                qPrintable(floatComparisonErrorString
                .arg("saturation()")
                .arg(colorPicker->saturation())
                .arg(0.0)
                .arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(dialogHelper.quickDialog->saturation(), 0.0, floatingPointComparisonThreshold,
                qPrintable(floatComparisonErrorString
                .arg("saturation()")
                .arg(dialogHelper.quickDialog->saturation())
                .arg(0.0)
                .arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(colorPicker->lightness(), 0.5, floatingPointComparisonThreshold,
                qPrintable(floatComparisonErrorString
                .arg("lightness()")
                .arg(colorPicker->lightness())
                .arg(0.5).arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(dialogHelper.quickDialog->lightness(), 0.5, floatingPointComparisonThreshold,
                qPrintable(floatComparisonErrorString
                .arg("lightness()")
                .arg(dialogHelper.quickDialog->lightness())
                .arg(0.5)
                .arg(floatingPointComparisonThreshold)));
    QCOMPARE(colorPicker->color().rgba(), QColor::fromRgbF(0.5, 0.5, 0.5).rgba());
    QCOMPARE(dialogHelper.quickDialog->color().rgba(), QColor::fromRgbF(0.5, 0.5, 0.5).rgba());

    // Testing whether the handles position and current color is set correctly when the color is set externally
    colorPicker->setColor(QColorConstants::Green);

    // Click in the middle of the handle, to cause the signal colorPicked() to be emitted
    QTest::mouseClick(dialogHelper.popupWindow(), Qt::LeftButton, Qt::NoModifier,
                      colorPicker->handle()->mapToScene({ colorPicker->handle()->width() / 2,
                                             colorPicker->handle()->height() / 2 }).toPoint());

    FUZZYCOMPARE(dialogHelper.quickDialog->color().red(), QColorConstants::Green.red(), 2,
                qPrintable(colorComparisonErrorString.arg("red").arg(QColorConstants::Green.red()).arg(dialogHelper.quickDialog->color().red())));
    FUZZYCOMPARE(dialogHelper.quickDialog->color().green(), QColorConstants::Green.green(), 2,
                qPrintable(colorComparisonErrorString.arg("green").arg(QColorConstants::Green.green()).arg(dialogHelper.quickDialog->color().green())));
    FUZZYCOMPARE(dialogHelper.quickDialog->color().blue(), QColorConstants::Green.blue(), 2,
                qPrintable(colorComparisonErrorString.arg("blue").arg(QColorConstants::Green.blue()).arg(dialogHelper.quickDialog->color().blue())));

    FUZZYCOMPARE(dialogHelper.dialog->selectedColor().red(), QColorConstants::Green.red(), 2,
                qPrintable(colorComparisonErrorString.arg("red").arg(QColorConstants::Green.red()).arg(dialogHelper.dialog->selectedColor().red())));
    FUZZYCOMPARE(dialogHelper.dialog->selectedColor().green(), QColorConstants::Green.green(), 2,
                qPrintable(colorComparisonErrorString.arg("green").arg(QColorConstants::Green.green()).arg(dialogHelper.dialog->selectedColor().green())));
    FUZZYCOMPARE(dialogHelper.dialog->selectedColor().blue(), QColorConstants::Green.blue(), 2,
                qPrintable(colorComparisonErrorString.arg("blue").arg(QColorConstants::Green.blue()).arg(dialogHelper.dialog->selectedColor().blue())));

    const QColor colorFromColorPickerValues = QColor::fromHslF(colorPicker->hue(), colorPicker->saturation(), colorPicker->lightness());
    const QColor colorFromColorDialogValues = QColor::fromHslF(dialogHelper.quickDialog->hue(), dialogHelper.quickDialog->saturation(), dialogHelper.quickDialog->lightness());

    FUZZYCOMPARE(colorFromColorPickerValues.red(), QColorConstants::Green.red(), 2,
                qPrintable(colorComparisonErrorString.arg("red").arg(QColorConstants::Green.red()).arg(colorFromColorPickerValues.red())));
    FUZZYCOMPARE(colorFromColorPickerValues.green(), QColorConstants::Green.green(), 2,
                qPrintable(colorComparisonErrorString.arg("green").arg(QColorConstants::Green.green()).arg(colorFromColorPickerValues.green())));
    FUZZYCOMPARE(colorFromColorPickerValues.blue(), QColorConstants::Green.blue(), 2,
                qPrintable(colorComparisonErrorString.arg("blue").arg(QColorConstants::Green.blue()).arg(colorFromColorPickerValues.blue())));

    FUZZYCOMPARE(colorFromColorDialogValues.red(), QColorConstants::Green.red(), 2,
                qPrintable(colorComparisonErrorString.arg("red").arg(QColorConstants::Green.red()).arg(colorFromColorDialogValues.red())));
    FUZZYCOMPARE(colorFromColorDialogValues.green(), QColorConstants::Green.green(), 2,
                qPrintable(colorComparisonErrorString.arg("green").arg(QColorConstants::Green.green()).arg(colorFromColorDialogValues.green())));
    FUZZYCOMPARE(colorFromColorDialogValues.blue(), QColorConstants::Green.blue(), 2,
                qPrintable(colorComparisonErrorString.arg("blue").arg(QColorConstants::Green.blue()).arg(colorFromColorDialogValues.blue())));

    const QString handlePositionErrorString("Handle position not updated correctly. x-position was %1, expected %2");
    const qreal expectedHandlePosX = (colorPicker->handle()->x() + colorPicker->handle()->width() / 2) / colorPicker->width();
    FUZZYCOMPARE(QColorConstants::Green.lightnessF(), expectedHandlePosX,
                 floatingPointComparisonThreshold, qPrintable(handlePositionErrorString.arg(expectedHandlePosX).arg(QColorConstants::Green.lightnessF())));

    const qreal expectedHandlePosY = (1.0 - (colorPicker->handle()->y() + colorPicker->handle()->height() / 2) / colorPicker->height());
    FUZZYCOMPARE(QColorConstants::Green.hslSaturationF(), expectedHandlePosY,
                 floatingPointComparisonThreshold, qPrintable(handlePositionErrorString.arg(expectedHandlePosY).arg(QColorConstants::Green.hslSaturationF())));

    CLOSE_DIALOG("Ok");
}


void tst_QQuickColorDialogImpl::alphaChannel_data()
{
    QTest::addColumn<qreal>("targetValue");
    QTest::addColumn<QString>("expectedStringRepresentation");
    QTest::newRow("0.4") << 0.4 << "#66ffffff";
    QTest::newRow("0.7") << 0.7 << "#bbffffff";
}

void tst_QQuickColorDialogImpl::alphaChannel()
{
    QFETCH(qreal, targetValue);
    QFETCH(QString, expectedStringRepresentation);

    DialogTestHelper<QQuickColorDialog, QQuickColorDialogImpl> dialogHelper(this, "colorDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    dialogHelper.dialog->setOptions(QColorDialogOptions::ShowAlphaChannel);

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    QVERIFY(dialogHelper.waitForPopupWindowActiveAndPolished());

    QQuickSlider *alphaSlider = dialogHelper.quickDialog->findChild<QQuickSlider *>("alphaSlider");
    QVERIFY(alphaSlider);

    QQuickColorInputs *colorInputs = dialogHelper.quickDialog->findChild<QQuickColorInputs *>();
    QVERIFY(colorInputs);

    QQuickTextField *colorTextField = qobject_cast<QQuickTextField *>(colorInputs->itemAt(0));
    QVERIFY(colorTextField);

    QVERIFY2(alphaSlider->isVisible(), "alphaSlider should be visible when the ShowAlphaChannel option is set, but it isn't");
    QCOMPARE(dialogHelper.quickDialog->alpha(), 1.0);
    QCOMPARE(dialogHelper.dialog->selectedColor().alphaF(), 1.0);
    QCOMPARE(colorTextField->text(), QStringLiteral("#ffffff")); // Alpha is hidden when FF

    QQuickTest::qWaitForPolish(dialogHelper.popupWindow());

    // Choose the target value from the alpha slider.
    QTest::mouseClick(dialogHelper.popupWindow(), Qt::LeftButton, Qt::NoModifier, alphaSlider->mapToScene({ (alphaSlider->width() + alphaSlider->padding()) * targetValue, alphaSlider->height() / 2 }).toPoint());

    // Compare the new value, with some fuzzyness allowed, since QColor has a precision of 16 bits.
    const qreal threshold = 1.0 / 16.0;
    const QString errorString("The alpha value is not close enouth to the target value of %1. Was %2");
    FUZZYCOMPARE(dialogHelper.quickDialog->alpha(), targetValue, threshold, qPrintable(errorString.arg(targetValue).arg(dialogHelper.quickDialog->alpha())));
    FUZZYCOMPARE(dialogHelper.dialog->selectedColor().alphaF(), targetValue, threshold, qPrintable(errorString.arg(targetValue).arg(dialogHelper.dialog->selectedColor().alphaF())));

    CLOSE_DIALOG("Ok");
}

void tst_QQuickColorDialogImpl::changeHex()
{
    DialogTestHelper<QQuickColorDialog, QQuickColorDialogImpl> dialogHelper(this, "colorDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    QVERIFY(dialogHelper.waitForPopupWindowActiveAndPolished());

    QQuickColorInputs *colorInputs = dialogHelper.quickDialog->findChild<QQuickColorInputs *>();
    QVERIFY(colorInputs);
    QQuickTextField *colorTextField = qobject_cast<QQuickTextField *>(colorInputs->itemAt(0));
    QVERIFY(colorTextField);
    QCOMPARE(colorTextField->text(), QStringLiteral("#ffffff"));

    // Modify the value in the TextField to something else.
    colorTextField->forceActiveFocus();
    colorTextField->select(1, colorTextField->text().size());
    QTRY_VERIFY_ACTIVE_FOCUS(colorTextField);
    QTest::keyClick(dialogHelper.popupWindow(), Qt::Key_Backspace);
    QTest::keyClick(dialogHelper.popupWindow(), '0');
    QTest::keyClick(dialogHelper.popupWindow(), '0');
    QTest::keyClick(dialogHelper.popupWindow(), 'f');
    QTest::keyClick(dialogHelper.popupWindow(), 'f');
    QTest::keyClick(dialogHelper.popupWindow(), '0');
    QTest::keyClick(dialogHelper.popupWindow(), '0');
    QTest::keyClick(dialogHelper.popupWindow(), Qt::Key_Enter);

    // Make sure that the color was updated, to reflect the new hex value.
    QCOMPARE(colorTextField->text(), QStringLiteral("#00ff00"));
    QCOMPARE(dialogHelper.quickDialog->color().toRgb(), QColor(0, 255, 0));
    QCOMPARE(dialogHelper.dialog->selectedColor(), QColor(0, 255, 0));

    CLOSE_DIALOG("Ok");
}

void tst_QQuickColorDialogImpl::changeColorFromTextFields_data()
{
    QTest::addColumn<QQuickColorInputs::Mode>("colorMode");
    QTest::addColumn<QString>("expectedDefaultValue");
    QTest::addColumn<QString>("newValue");
    QTest::addColumn<QColor>("expectedNewColor");
    QTest::addColumn<int>("index");

    QTest::newRow("rgbRed") << QQuickColorInputs::Rgb << "255" << "100" << QColor(100, 255, 255) << 0;
    QTest::newRow("rgbGreen") << QQuickColorInputs::Rgb << "255" << "0" << QColorConstants::Magenta << 1;
    QTest::newRow("rgbBlue") << QQuickColorInputs::Rgb << "255" << "200" << QColor(255, 255, 200) << 2;
    QTest::newRow("rgbAlpha") << QQuickColorInputs::Rgb << "100%" << "50%" << QColor::fromRgbF(1.0f, 1.0f, 1.0f, 0.5f) << 3;

    QTest::newRow("hsvHue") << QQuickColorInputs::Hsv << "0°" << "60°" << QColor::fromHsvF(.2f, 0.0f, 1.0f) << 0;
    QTest::newRow("hsvSaturation") << QQuickColorInputs::Hsv << "0%" << "50%" << QColor::fromHsvF(0.0f, 0.5f, 1.0f) << 1;
    QTest::newRow("hsvValue") << QQuickColorInputs::Hsv << "100%" << "90%" << QColor::fromHsvF(0.0f, 0.0f, 0.9f, 1.0f) << 2;
    QTest::newRow("hsvAlpha") << QQuickColorInputs::Hsv << "100%" << "40%" << QColor::fromHsvF(0.0f, 0.0f, 1.0f, 0.4f) << 3;

    QTest::newRow("hslHue") << QQuickColorInputs::Hsl << "0°" << "90°" << QColor::fromHslF(.25f, 1.0f, 1.0f) << 0;
    QTest::newRow("hslSaturation") << QQuickColorInputs::Hsl << "0%" << "40%" << QColor::fromHslF(0.0f, 0.4f, 1.0f) << 1;
    QTest::newRow("hslLightness") << QQuickColorInputs::Hsl << "100%" << "80%" << QColor::fromHslF(0.0f, 0.0f, 0.8f, 1.0f) << 2;
    QTest::newRow("hslAlpha") << QQuickColorInputs::Hsl << "100%" << "60%" << QColor::fromHslF(0.0f, 0.0f, 1.0f, 0.6f) << 3;
}

void tst_QQuickColorDialogImpl::changeColorFromTextFields()
{
    QFETCH(QQuickColorInputs::Mode, colorMode);
    QFETCH(QString, expectedDefaultValue);
    QFETCH(QString, newValue);
    QFETCH(QColor, expectedNewColor);
    QFETCH(int, index);

    DialogTestHelper<QQuickColorDialog, QQuickColorDialogImpl> dialogHelper(this, "colorDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    if (QString::fromUtf8(QTest::currentDataTag()).endsWith("Alpha"))
        dialogHelper.dialog->setOptions(dialogHelper.dialog->options() | QColorDialogOptions::ShowAlphaChannel);

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    QVERIFY(dialogHelper.waitForPopupWindowActiveAndPolished());

    const QQuickComboBox *colorSystemComboBox = dialogHelper.quickDialog->findChild<QQuickComboBox *>("colorSystemComboBox");
    QVERIFY(colorSystemComboBox);

    // Click on the colorSystemComboBox.
    const QPoint comboBoxCenterPos = colorSystemComboBox->mapToScene( {colorSystemComboBox->width() / 2, colorSystemComboBox->height() /2} ).toPoint();
    QTest::mouseClick(dialogHelper.popupWindow(), Qt::LeftButton, Qt::NoModifier, comboBoxCenterPos);
    QCoreApplication::sendPostedEvents();
    QTRY_VERIFY(colorSystemComboBox->popup()->isOpened());

    auto comboBoxPopupListView = qobject_cast<QQuickListView *>(colorSystemComboBox->popup()->contentItem());
    QVERIFY(comboBoxPopupListView);

    // Find the relevant color system delegate.
    auto delegate = qobject_cast<QQuickAbstractButton *>(findViewDelegateItem(comboBoxPopupListView, static_cast<int>(colorMode)));
    QVERIFY(delegate);
    QVERIFY(clickButton(delegate));
    QTRY_VERIFY(!colorSystemComboBox->popup()->isVisible());

    QQuickColorInputs *colorInputs = dialogHelper.quickDialog->findChild<QQuickColorInputs *>();
    QVERIFY(colorInputs);
    QQuickTextField *textField = qobject_cast<QQuickTextField *>(colorInputs->itemAt(index));
    QVERIFY(textField);
    QCOMPARE(textField->text(), expectedDefaultValue);

    if (colorMode == QQuickColorInputs::Hsv)
        dialogHelper.quickDialog->setHsl(false);

    textField->forceActiveFocus();
    QVERIFY_ACTIVE_FOCUS(textField);

    // Simulate entering a new value.
    textField->setText(newValue);
    QTest::keyClick(dialogHelper.popupWindow(), Qt::Key_Enter);
    QCoreApplication::sendPostedEvents();

    // Check if the color was updated with the correct new value.
    QCOMPARE(textField->text(), newValue);
    QCOMPARE(dialogHelper.quickDialog->color().rgba(), expectedNewColor.rgba());
    QCOMPARE(dialogHelper.dialog->selectedColor().rgba(), expectedNewColor.rgba());

    CLOSE_DIALOG("Ok");
}

void tst_QQuickColorDialogImpl::windowTitle_data()
{
    QTest::addColumn<QString>("title");
    QTest::newRow("test string 1") << "The quick red fox jumped over the lazy brown dog";
    QTest::newRow("test string 2") << "The lazy brown dog jumped over the quick red fox";
}

void tst_QQuickColorDialogImpl::windowTitle()
{
    QFETCH(QString, title);
    DialogTestHelper<QQuickColorDialog, QQuickColorDialogImpl> dialogHelper(this, "colorDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    // set the title.
    dialogHelper.dialog->setTitle(title);

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    QVERIFY(dialogHelper.waitForPopupWindowActiveAndPolished());

    QCOMPARE(dialogHelper.dialog->title(), title);

    // find the label that's being used to display the title.
    QQuickText *titleText = dialogHelper.quickDialog->findChild<QQuickText *>("titleLabel");
    QVERIFY(titleText);

    // verify that the title was set correctly.
    QCOMPARE(titleText->text(), title);

    CLOSE_DIALOG("Ok");
}

void tst_QQuickColorDialogImpl::workingInsideQQuickViewer_data()
{
    QTest::addColumn<bool>("initialVisible");
    QTest::addColumn<QString>("urlToQmlFile");
    QTest::newRow("visible: false") << false << "colorDialogWithoutWindow.qml";
    QTest::newRow("visible: true") << true << "colorDialogWithoutWindowVisibleTrue.qml";
}

void tst_QQuickColorDialogImpl::workingInsideQQuickViewer() // QTBUG-106598
{
    QFETCH(bool, initialVisible);
    QFETCH(QString, urlToQmlFile);

    QQuickView dialogView;
    dialogView.setSource(testFileUrl(urlToQmlFile));
    dialogView.show();

    auto dialog = dialogView.findChild<QQuickColorDialog *>("ColorDialog");
    QVERIFY(dialog);
    QCOMPARE(dialog->isVisible(), initialVisible);

    dialog->open();
    QVERIFY(dialog->isVisible());
}

void tst_QQuickColorDialogImpl::dialogCanMoveBetweenWindows()
{
    DialogTestHelper<QQuickColorDialog, QQuickColorDialogImpl> dialogHelper(this, "windowSwapping.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    QVERIFY(dialogHelper.waitForPopupWindowActiveAndPolished());

    QCOMPARE(dialogHelper.quickDialog->parent(), dialogHelper.window());
    QVariant subWindow1;
    QVariant subWindow2;

    QMetaObject::invokeMethod(dialogHelper.window(), "getSubWindow1", Q_RETURN_ARG(QVariant, subWindow1));
    QMetaObject::invokeMethod(dialogHelper.window(), "getSubWindow2", Q_RETURN_ARG(QVariant, subWindow2));

    // Confirm that the dialog can swap to different windows
    QMetaObject::invokeMethod(dialogHelper.window(), "goToSubWindow1");
    QTRY_COMPARE(dialogHelper.dialog->parentWindow(), qvariant_cast<QQuickWindow *>(subWindow1));

    QMetaObject::invokeMethod(dialogHelper.window(), "goToSubWindow2");
    QTRY_COMPARE(dialogHelper.dialog->parentWindow(), qvariant_cast<QQuickWindow *>(subWindow2));

    QMetaObject::invokeMethod(dialogHelper.window(), "resetParentWindow");
    QTRY_COMPARE(dialogHelper.quickDialog->parent(), dialogHelper.window());

    dialogHelper.popupWindow()->close();
    QVERIFY(dialogHelper.openDialog());
    QVERIFY(dialogHelper.waitForPopupWindowActiveAndPolished());

    CLOSE_DIALOG("Ok");
}

void tst_QQuickColorDialogImpl::checkModality_data()
{
    QTest::addColumn<Qt::WindowModality>("modality");
    QTest::addColumn<QColorDialogOptions::ColorDialogOption>("options");
    QTest::addColumn<int>("expectedRootWindowClickCount");
    QTest::addColumn<int>("expectedChildWindowClickCount");

    QTest::newRow("nonModal") << Qt::NonModal << QColorDialogOptions::DontUseNativeDialog << 1 << 1;
    QTest::newRow("windowModal") << Qt::WindowModal << QColorDialogOptions::DontUseNativeDialog << 0 << 1;
    // Verify application modal case only for the platform which supports multiple windows
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::MultipleWindows))
        QTest::newRow("applicationModal") << Qt::ApplicationModal << QColorDialogOptions::DontUseNativeDialog << 0 << 0;
}

void tst_QQuickColorDialogImpl::checkModality()
{
    QFETCH(Qt::WindowModality, modality);
    QFETCH(QColorDialogOptions::ColorDialogOption, options);
    QFETCH(int, expectedRootWindowClickCount);
    QFETCH(int, expectedChildWindowClickCount);

    DialogTestHelper<QQuickColorDialog, QQuickColorDialogImpl> dialogHelper(this, "checkModality.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    dialogHelper.dialog->setModality(modality);
    QCOMPARE(dialogHelper.dialog->modality(), modality);

    dialogHelper.dialog->setOptions(options);
    QCOMPARE(dialogHelper.dialog->options(), options);

    OPEN_QUICK_DIALOG();
    QVERIFY(dialogHelper.waitForPopupWindowActiveAndPolished());

    auto *childWindow = dialogHelper.window()->property("childWindow").value<QQuickWindow *>();
    QVERIFY(childWindow);

    // Setting the child window transient parent as root window won't allow it to receive mouse
    // events, causing WindowModal case not to be tested. Thus, resetting the transient parent.
    if (modality == Qt::WindowModal)
        childWindow->setTransientParent(nullptr);

    const auto *rootMouseArea = dialogHelper.window()->property("rootMArea").value<QQuickMouseArea *>();
    QVERIFY(rootMouseArea);
    QSignalSpy rmaMouseSpy(rootMouseArea, &QQuickMouseArea::clicked);
    QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QCOMPARE(rmaMouseSpy.size(), expectedRootWindowClickCount);

    const auto *childMouseArea = dialogHelper.window()->property("childMArea").value<QQuickMouseArea *>();
    QVERIFY(childMouseArea);
    QSignalSpy cmaMouseSpy(childMouseArea, &QQuickMouseArea::clicked);
    QTest::mouseClick(childWindow, Qt::LeftButton, Qt::NoModifier, QPoint(5, 5));
    QCOMPARE(cmaMouseSpy.size(), expectedChildWindowClickCount);
}


QTEST_MAIN(tst_QQuickColorDialogImpl)

#include "tst_qquickcolordialogimpl.moc"
