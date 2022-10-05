// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtQml/qqmlfile.h>
#include <QtQuickDialogs2/private/qquickmessagedialog_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickmessagedialogimpl_p.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/dialogstestutils_p.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickDialogTestUtils;
using namespace QQuickControlsTestUtils;

class tst_QQuickMessageDialogImpl : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickMessageDialogImpl();
    static void initMain()
    {
        // We need to set this attribute.
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        // We don't want to run this test for every style, as each one will have
        // different ways of implementing the dialogs.
        // For now we only test one style.
        // TODO: use Basic
        QQuickStyle::setStyle("Fusion");
    }

private slots:
    void changeText_data();
    void changeText();
    void changeInformativeText_data();
    void changeInformativeText();
    void changeStandardButtons();
    void detailedText();
};

// We don't want to fail on warnings until QTBUG-98964 is fixed,
// as we deliberately prevent deferred execution in some of the tests here,
// which causes warnings.
tst_QQuickMessageDialogImpl::tst_QQuickMessageDialogImpl()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::DoNotFailOnWarnings)
{
}

void tst_QQuickMessageDialogImpl::changeText_data()
{
    QTest::addColumn<QString>("testString1");
    QTest::addColumn<QString>("testString2");
    QTest::newRow("data") << "the quick brown fox jumped over the lazy dog"
                          << "the lazy dog jumped over the quick brown fox";
}

void tst_QQuickMessageDialogImpl::changeText()
{
    QFETCH(QString, testString1);
    QFETCH(QString, testString2);
    DialogTestHelper<QQuickMessageDialog, QQuickMessageDialogImpl> dialogHelper(
            this, "messageDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    QSignalSpy textSpy(dialogHelper.dialog, SIGNAL(textChanged()));
    QVERIFY(textSpy.isValid());

    auto textLabel = dialogHelper.quickDialog->findChild<QQuickLabel *>("textLabel");
    QVERIFY(textLabel);

    // default value is empty string
    QCOMPARE(dialogHelper.dialog->text(), "");

    // update the text property
    dialogHelper.dialog->setText(testString1);
    QCOMPARE(textSpy.size(), 1);

    // The textLabel is empty until dialog is re-opened
    QCOMPARE(dialogHelper.dialog->text(), testString1);
    QCOMPARE(textLabel->text(), "");
    dialogHelper.dialog->close();
    dialogHelper.dialog->open();
    QCOMPARE(dialogHelper.dialog->text(), testString1);
    QCOMPARE(textLabel->text(), testString1);

    // The textLabel isn't updated immediately
    dialogHelper.dialog->setText(testString2);
    QCOMPARE(textSpy.size(), 2);
    QCOMPARE(textLabel->text(), testString1);

    dialogHelper.dialog->close();
}

void tst_QQuickMessageDialogImpl::changeInformativeText_data()
{
    QTest::addColumn<QString>("testString1");
    QTest::addColumn<QString>("testString2");
    QTest::newRow("data") << "the quick brown fox jumped over the lazy dog"
                          << "the lazy dog jumped over the quick brown fox";
}

void tst_QQuickMessageDialogImpl::changeInformativeText()
{
    QFETCH(QString, testString1);
    QFETCH(QString, testString2);
    DialogTestHelper<QQuickMessageDialog, QQuickMessageDialogImpl> dialogHelper(
            this, "messageDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    QSignalSpy informativeTextSpy(dialogHelper.dialog, SIGNAL(informativeTextChanged()));
    QVERIFY(informativeTextSpy.isValid());

    auto informativeTextLabel =
            dialogHelper.quickDialog->findChild<QQuickLabel *>("informativeTextLabel");
    QVERIFY(informativeTextLabel);

    // default value is empty string
    QCOMPARE(dialogHelper.dialog->informativeText(), "");

    // update the informativeText property
    dialogHelper.dialog->setInformativeText(testString1);
    QCOMPARE(informativeTextSpy.size(), 1);

    // The textLabel is empty until dialog is re-opened
    QCOMPARE(dialogHelper.dialog->informativeText(), testString1);
    QCOMPARE(informativeTextLabel->text(), "");
    dialogHelper.dialog->close();
    dialogHelper.dialog->open();
    QCOMPARE(dialogHelper.dialog->informativeText(), testString1);
    QCOMPARE(informativeTextLabel->text(), testString1);

    // The textLabel shouldn't update immediately
    dialogHelper.dialog->setInformativeText(testString2);
    QCOMPARE(informativeTextSpy.size(), 2);
    QCOMPARE(informativeTextLabel->text(), testString1);

    dialogHelper.dialog->close();
}

void tst_QQuickMessageDialogImpl::changeStandardButtons()
{
    DialogTestHelper<QQuickMessageDialog, QQuickMessageDialogImpl> dialogHelper(
            this, "messageDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    QSignalSpy buttonBoxSpy(dialogHelper.dialog, SIGNAL(buttonsChanged()));
    QVERIFY(buttonBoxSpy.isValid());

    auto buttonBox = dialogHelper.quickDialog->findChild<QQuickDialogButtonBox *>("buttonBox");
    QVERIFY(buttonBox);
    QCOMPARE(buttonBox->count(), 1);
    QVERIFY2(dialogHelper.dialog->buttons() & QPlatformDialogHelper::StandardButton::Ok,
             "The QMessageDialogOptionsPrivate constructor assures that the Ok button will be "
             "added by default, if no other buttons are set");

    dialogHelper.dialog->setButtons(
            QPlatformDialogHelper::StandardButtons(QPlatformDialogHelper::StandardButton::Save
                                                   | QPlatformDialogHelper::StandardButton::Cancel
                                                   | QPlatformDialogHelper::StandardButton::Apply));
    QCOMPARE(buttonBoxSpy.size(), 1);
    QCOMPARE(buttonBox->count(), 1);
    dialogHelper.dialog->close();
    dialogHelper.dialog->open();
    QCOMPARE(buttonBox->count(), 3);

    auto saveButton = qobject_cast<QQuickAbstractButton *>(buttonBox->itemAt(0));
    QVERIFY(saveButton);
    QCOMPARE(saveButton, buttonBox->standardButton(QPlatformDialogHelper::StandardButton::Save));
    auto applyButton = qobject_cast<QQuickAbstractButton *>(buttonBox->itemAt(1));
    QVERIFY(applyButton);
    QCOMPARE(applyButton, buttonBox->standardButton(QPlatformDialogHelper::StandardButton::Cancel));
    auto cancelButton = qobject_cast<QQuickAbstractButton *>(buttonBox->itemAt(2));
    QVERIFY(cancelButton);
    QCOMPARE(cancelButton, buttonBox->standardButton(QPlatformDialogHelper::StandardButton::Apply));

    // change buttons when the dialog is closed
    dialogHelper.dialog->close();
    dialogHelper.dialog->setButtons(
            QPlatformDialogHelper::StandardButton(QPlatformDialogHelper::StandardButton::Ok
                                                  | QPlatformDialogHelper::StandardButton::Close));
    QCOMPARE(buttonBoxSpy.size(), 2);
    QCOMPARE(buttonBox->count(), 3);
    dialogHelper.dialog->open();
    QCOMPARE(buttonBox->count(), 2);

    auto okButton = qobject_cast<QQuickAbstractButton *>(buttonBox->itemAt(0));
    QVERIFY(okButton);
    QCOMPARE(okButton, buttonBox->standardButton(QPlatformDialogHelper::StandardButton::Ok));
    auto closeButton = qobject_cast<QQuickAbstractButton *>(buttonBox->itemAt(1));
    QVERIFY(closeButton);
    QCOMPARE(closeButton, buttonBox->standardButton(QPlatformDialogHelper::StandardButton::Close));

    dialogHelper.dialog->close();
}

void tst_QQuickMessageDialogImpl::detailedText()
{
    const QString emptyString;
    const QString nonEmptyString("the quick brown fox jumped over the lazy dog");
    DialogTestHelper<QQuickMessageDialog, QQuickMessageDialogImpl> dialogHelper(
            this, "messageDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    QSignalSpy detailedTextSpy(dialogHelper.dialog, SIGNAL(detailedTextChanged()));
    QVERIFY(detailedTextSpy.isValid());

    auto detailedTextArea = dialogHelper.quickDialog->findChild<QQuickTextArea *>("detailedText");
    QVERIFY(detailedTextArea);
    auto detailedTextButton =
            dialogHelper.quickDialog->findChild<QQuickAbstractButton *>("detailedTextButton");
    QVERIFY(detailedTextButton);

    // Should be empty by default
    QCOMPARE(dialogHelper.dialog->detailedText(), emptyString);
    QCOMPARE(detailedTextArea->text(), emptyString);
    QVERIFY(!detailedTextButton->isVisible());

    // Set the detailed text to a non-empty string
    dialogHelper.dialog->setDetailedText(nonEmptyString);
    QCOMPARE(dialogHelper.dialog->detailedText(), nonEmptyString);
    QCOMPARE(detailedTextSpy.size(), 1);
    QCOMPARE(detailedTextArea->text(), emptyString);
    QVERIFY(!detailedTextButton->isVisible());
    dialogHelper.dialog->close();
    dialogHelper.dialog->open();
    QCOMPARE(dialogHelper.dialog->detailedText(), nonEmptyString);
    QCOMPARE(detailedTextArea->text(), nonEmptyString);
    QVERIFY2(detailedTextButton->isVisible(),
             "The 'show details' button should be visible when the detailedText property is not "
             "empty.");

    // Set the detailed text to an empty string
    dialogHelper.dialog->setDetailedText(emptyString);
    QCOMPARE(detailedTextSpy.size(), 2);
    QCOMPARE(dialogHelper.dialog->detailedText(), emptyString);
    QCOMPARE(detailedTextArea->text(), nonEmptyString);
    QVERIFY(detailedTextButton->isVisible());
    dialogHelper.dialog->close();
    dialogHelper.dialog->open();
    QCOMPARE(dialogHelper.dialog->detailedText(), emptyString);
    QCOMPARE(detailedTextArea->text(), emptyString);
    QVERIFY2(!detailedTextButton->isVisible(),
             "The 'show details' button should be invisible when the detailedText property is an empty string");

    // Change the detailed text property while the dialog is already open, should not immediately
    // update the dialog ui
    dialogHelper.dialog->setDetailedText(nonEmptyString);
    QCOMPARE(detailedTextSpy.size(), 3);
    QCOMPARE(dialogHelper.dialog->detailedText(), nonEmptyString);
    QCOMPARE(detailedTextArea->text(), emptyString);
    QVERIFY2(!detailedTextButton->isVisible(),
             "The 'show details' button should only become visible when the dialog is re-opened.");

    dialogHelper.dialog->close();
}

QTEST_MAIN(tst_QQuickMessageDialogImpl)

#include "tst_qquickmessagedialogimpl.moc"
