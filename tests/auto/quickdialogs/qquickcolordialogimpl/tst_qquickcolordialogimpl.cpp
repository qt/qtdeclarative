// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtQuickTest/quicktest.h>
#include <QtTest/qsignalspy.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/dialogstestutils_p.h>
#include <QtQuickDialogs2/private/qquickcolordialog_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickcolordialogimpl_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickabstractcolorpicker_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>
#include <QtQuickTemplates2/private/qquickslider_p.h>
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
    void defaultValues();
    void moveColorPickerHandle();
    void alphaChannel_data();
    void alphaChannel();
    void changeHex();
    void changeColorFromTextFields_data();
    void changeColorFromTextFields();
    void windowTitle_data();
    void windowTitle();

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

void tst_QQuickColorDialogImpl::defaultValues()
{
    DialogTestHelper<QQuickColorDialog, QQuickColorDialogImpl> dialogHelper(this, "colorDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

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
    dialogHelper.dialog->close();
    QTRY_VERIFY(!dialogHelper.isQuickDialogOpen());
    dialogHelper.dialog->setOptions(QColorDialogOptions::ShowAlphaChannel);
    QVERIFY(dialogHelper.openDialog());
    QVERIFY(alphaSlider->isVisible());

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

    QTRY_COMPARE(QQuickWindowPrivate::get(dialogHelper.window())->itemsToPolish.size(), 0);

    QQuickAbstractColorPicker *colorPicker =
            dialogHelper.quickDialog->findChild<QQuickAbstractColorPicker *>("colorPicker");
    QVERIFY(colorPicker);
    QQuickSlider *hueSlider = dialogHelper.quickDialog->findChild<QQuickSlider *>("hueSlider");
    QVERIFY(hueSlider);

    QSignalSpy colorChangedSpy(colorPicker, SIGNAL(colorChanged(QColor)));

    const QPoint topCenter = colorPicker->mapToScene({ colorPicker->width() / 2, 0 }).toPoint();

    // Move handle to where the saturation is the highest and the lightness is 'neutral'
    QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier, topCenter);

    QCOMPARE(colorChangedSpy.size(), 1);

    const qreal floatingPointComparisonThreshold = 1.0 / colorPicker->width();
    const QString floatComparisonErrorString(
            "%1 return value of %2 wasn't close enough to %3. A threshold of %4 is used to decide if the floating "
            "point value is close enough.");

    FUZZYCOMPARE(colorPicker->saturation(), 1.0, floatingPointComparisonThreshold,
             qPrintable(floatComparisonErrorString.arg("saturation()").arg(colorPicker->saturation()).arg(1.0).arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(dialogHelper.quickDialog->saturation(), 1.0, floatingPointComparisonThreshold,
             qPrintable(floatComparisonErrorString.arg("saturation()").arg(dialogHelper.quickDialog->saturation()).arg(1.0).arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(colorPicker->lightness(), 0.5, floatingPointComparisonThreshold,
             qPrintable(floatComparisonErrorString.arg("lightness()").arg(colorPicker->lightness()).arg(0.5).arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(dialogHelper.quickDialog->lightness(), 0.5, floatingPointComparisonThreshold,
             qPrintable(floatComparisonErrorString.arg("lightness()").arg(dialogHelper.quickDialog->lightness()).arg(0.5).arg(floatingPointComparisonThreshold)));
    QCOMPARE(colorPicker->color().rgba(), QColorConstants::Red.rgba());
    QCOMPARE(dialogHelper.quickDialog->color().rgba(), QColorConstants::Red.rgba());

    const QPoint hueSliderCenterPosition =
            hueSlider->mapToScene({ hueSlider->width() / 2, hueSlider->height() / 2 }).toPoint();
    QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier,
                      hueSliderCenterPosition);

    QCOMPARE(hueSlider->value(), QColorConstants::Cyan.hslHueF());
    QCOMPARE(dialogHelper.quickDialog->hue(), QColorConstants::Cyan.hslHueF());
    QCOMPARE(colorPicker->hue(), QColorConstants::Cyan.hslHueF());
    QCOMPARE(colorPicker->color().rgba(), QColorConstants::Cyan.rgba());

    QCOMPARE(colorChangedSpy.size(), 2);

    QPoint bottomCenter = colorPicker->mapToScene({ colorPicker->width() / 2, colorPicker->height() }).toPoint();

    // Move the handle to where the saturation is the lowest, without affecting lightness
    QTest::mousePress(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier,
                      { bottomCenter.x(), bottomCenter.y() - 1 });
    QTest::mouseRelease(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier, bottomCenter);

    // The press and release happened in 2 different positions.
    // This means that the current color was changed twice.
    // (The press happens 1 pixel above the release, to work around an issue where the mouse event
    // wasn't received by the color picker)
    QCOMPARE(colorChangedSpy.size(), 4);
    FUZZYCOMPARE(colorPicker->saturation(), 0.0, floatingPointComparisonThreshold,
             qPrintable(floatComparisonErrorString.arg("saturation()").arg(colorPicker->saturation()).arg(0.0).arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(dialogHelper.quickDialog->saturation(), 0.0, floatingPointComparisonThreshold,
             qPrintable(floatComparisonErrorString.arg("saturation()").arg(dialogHelper.quickDialog->saturation()).arg(0.0).arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(colorPicker->lightness(), 0.5, floatingPointComparisonThreshold,
             qPrintable(floatComparisonErrorString.arg("lightness()").arg(colorPicker->lightness()).arg(0.5).arg(floatingPointComparisonThreshold)));
    FUZZYCOMPARE(dialogHelper.quickDialog->lightness(), 0.5, floatingPointComparisonThreshold,
             qPrintable(floatComparisonErrorString.arg("lightness()").arg(dialogHelper.quickDialog->lightness()).arg(0.5).arg(floatingPointComparisonThreshold)));
    QCOMPARE(colorPicker->color().rgba(), QColor::fromRgbF(0.5, 0.5, 0.5).rgba());
    QCOMPARE(dialogHelper.quickDialog->color().rgba(), QColor::fromRgbF(0.5, 0.5, 0.5).rgba());

    // Testing whether the handles position and current color is set correctly when the color is set externally
    colorPicker->setColor(QColorConstants::Green);

    // Click in the middle of the handle, to cause the signal colorPicked() to be emitted
    QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier,
                      colorPicker->handle()->mapToScene({ colorPicker->handle()->width() / 2,
                                             colorPicker->handle()->height() / 2 }).toPoint());

    QCOMPARE(dialogHelper.quickDialog->color().rgba(), QColorConstants::Green.rgba());
    QCOMPARE(dialogHelper.dialog->selectedColor().rgba(), QColorConstants::Green.rgba());
    QCOMPARE(QColor::fromHslF(colorPicker->hue(), colorPicker->saturation(),
                              colorPicker->lightness()).rgba(), QColorConstants::Green.rgba());
    QCOMPARE(QColor::fromHslF(dialogHelper.quickDialog->hue(), dialogHelper.quickDialog->saturation(),
                              dialogHelper.quickDialog->lightness()).rgba(), QColorConstants::Green.rgba());

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

    QQuickSlider *alphaSlider = dialogHelper.quickDialog->findChild<QQuickSlider *>("alphaSlider");
    QVERIFY(alphaSlider);

    QQuickItem *colorParameters = dialogHelper.quickDialog->findChild<QQuickItem *>("colorParameters");
    QVERIFY(colorParameters);

    QQuickTextField *colorTextField = qobject_cast<QQuickTextField *>(colorParameters->children().at(0));
    QVERIFY(colorTextField);

    QVERIFY2(alphaSlider->isVisible(), "alphaSlider should be visible when the ShowAlphaChannel option is set, but it isn't");
    QCOMPARE(dialogHelper.quickDialog->alpha(), 1.0);
    QCOMPARE(dialogHelper.dialog->selectedColor().alphaF(), 1.0);
    QCOMPARE(colorTextField->text(), QStringLiteral("#ffffff")); // Alpha is hidden when FF

    QTRY_COMPARE(QQuickWindowPrivate::get(dialogHelper.window())->itemsToPolish.size(), 0);

    // Choose the target value from the alpha slider.
    QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier, alphaSlider->mapToScene({ (alphaSlider->width() + alphaSlider->padding()) * targetValue, alphaSlider->height() / 2 }).toPoint());

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

    QQuickItem *colorParameters = dialogHelper.quickDialog->findChild<QQuickItem *>("colorParameters");
    QVERIFY(colorParameters);
    QQuickTextField *colorTextField = qobject_cast<QQuickTextField *>(colorParameters->children().at(0));
    QVERIFY(colorTextField);
    QCOMPARE(colorTextField->text(), QStringLiteral("#ffffff"));

    // Modify the value in the TextField to something else.
    colorTextField->forceActiveFocus();
    colorTextField->select(1, colorTextField->text().size());
    QVERIFY(colorTextField->hasActiveFocus());
    QTest::keyClick(dialogHelper.window(), Qt::Key_Backspace);
    QTest::keyClick(dialogHelper.window(), '0');
    QTest::keyClick(dialogHelper.window(), '0');
    QTest::keyClick(dialogHelper.window(), 'f');
    QTest::keyClick(dialogHelper.window(), 'f');
    QTest::keyClick(dialogHelper.window(), '0');
    QTest::keyClick(dialogHelper.window(), '0');
    QTest::keyClick(dialogHelper.window(), Qt::Key_Enter);

    // Make sure that the color was updated, to reflect the new hex value.
    QCOMPARE(colorTextField->text(), QStringLiteral("#00ff00"));
    QCOMPARE(dialogHelper.quickDialog->color().toRgb(), QColor(0, 255, 0));
    QCOMPARE(dialogHelper.dialog->selectedColor(), QColor(0, 255, 0));

    CLOSE_DIALOG("Ok");
}

enum class ColorSpec {
    Rgb = 1,
    Hsv,
    Hsl
};

void tst_QQuickColorDialogImpl::changeColorFromTextFields_data()
{
    QTest::addColumn<ColorSpec>("spec");
    QTest::addColumn<QString>("expectedDefaultValue");
    QTest::addColumn<QString>("newValue");
    QTest::addColumn<QColor>("expectedNewColor");

    QTest::newRow("rgbRed") << ColorSpec::Rgb << "255" << "100" << QColor(100, 255, 255);
    QTest::newRow("rgbGreen") << ColorSpec::Rgb << "255" << "0" << QColorConstants::Magenta;
    QTest::newRow("rgbBlue") << ColorSpec::Rgb << "255" << "200" << QColor(255, 255, 200);
    QTest::newRow("rgbAlpha") << ColorSpec::Rgb << "100%" << "50%" << QColor::fromRgbF(1.0f, 1.0f, 1.0f, 0.5f);

    QTest::newRow("hsvHue") << ColorSpec::Hsv << "0°" << "60°" << QColor::fromHsvF(.2f, 0.0f, 1.0f);
    QTest::newRow("hsvSaturation") << ColorSpec::Hsv << "0%" << "50%" << QColor::fromHsvF(0.0f, 0.5f, 1.0f);
    QTest::newRow("hsvValue") << ColorSpec::Hsv << "100%" << "90%" << QColor::fromHsvF(0.0f, 0.0f, 0.9f, 1.0f);
    QTest::newRow("hsvAlpha") << ColorSpec::Hsv << "100%" << "40%" << QColor::fromHsvF(0.0f, 0.0f, 1.0f, 0.4f);

    QTest::newRow("hslHue") << ColorSpec::Hsl << "0°" << "90°" << QColor::fromHslF(.25f, 1.0f, 1.0f);
    QTest::newRow("hslSaturation") << ColorSpec::Hsl << "0%" << "40%" << QColor::fromHslF(0.0f, 0.4f, 1.0f);
    QTest::newRow("hslLightness") << ColorSpec::Hsl << "100%" << "80%" << QColor::fromHslF(0.0f, 0.0f, 0.8f, 1.0f);
    QTest::newRow("hslAlpha") << ColorSpec::Hsl << "100%" << "60%" << QColor::fromHslF(0.0f, 0.0f, 1.0f, 0.6f);
}

void tst_QQuickColorDialogImpl::changeColorFromTextFields()
{
    QFETCH(ColorSpec, spec);
    QFETCH(QString, expectedDefaultValue);
    QFETCH(QString, newValue);
    QFETCH(QColor, expectedNewColor);

    DialogTestHelper<QQuickColorDialog, QQuickColorDialogImpl> dialogHelper(this, "colorDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    const QQuickComboBox *colorSystemComboBox = dialogHelper.quickDialog->findChild<QQuickComboBox *>("colorSystemComboBox");
    QVERIFY(colorSystemComboBox);

    QTRY_COMPARE(QQuickWindowPrivate::get(dialogHelper.window())->itemsToPolish.size(), 0);

    // Click on the colorSystemComboBox.
    const QPoint comboBoxCenterPos = colorSystemComboBox->mapToScene( {colorSystemComboBox->width() / 2, colorSystemComboBox->height() /2} ).toPoint();
    QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier, comboBoxCenterPos);
    QCoreApplication::sendPostedEvents();
    QTRY_VERIFY(colorSystemComboBox->popup()->isOpened());

    auto comboBoxPopupListView = qobject_cast<QQuickListView *>(colorSystemComboBox->popup()->contentItem());
    QVERIFY(comboBoxPopupListView);

    // Find the relevant color system delegate.
    auto delegate = qobject_cast<QQuickAbstractButton *>(findViewDelegateItem(comboBoxPopupListView, static_cast<int>(spec)));
    QVERIFY(delegate);
    QVERIFY(clickButton(delegate));
    QTRY_VERIFY(!colorSystemComboBox->popup()->isVisible());

    auto textField = dialogHelper.quickDialog->findChild<QQuickTextField *>(QTest::currentDataTag());
    QVERIFY(textField);
    QCOMPARE(textField->text(), expectedDefaultValue);

    if (spec == ColorSpec::Hsv)
        dialogHelper.quickDialog->setHsl(false);

    textField->forceActiveFocus();
    QVERIFY(textField->hasActiveFocus());

    // Simulate entering a new value.
    textField->setText(newValue);
    QTest::keyClick(dialogHelper.window(), Qt::Key_Enter);
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

    QCOMPARE(dialogHelper.dialog->title(), title);

    // find the label that's being used to display the title.
    QQuickText *titleText = dialogHelper.quickDialog->findChild<QQuickText *>("titleLabel");
    QVERIFY(titleText);

    // verify that the title was set correctly.
    QCOMPARE(titleText->text(), title);

    CLOSE_DIALOG("Ok");
}

QTEST_MAIN(tst_QQuickColorDialogImpl)

#include "tst_qquickcolordialogimpl.moc"
