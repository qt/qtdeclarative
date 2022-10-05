// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtQml/qqmlfile.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickTemplates2/private/qquickitemdelegate_p.h>
#include <QtQuickDialogs2/private/qquickfontdialog_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickfontdialogimpl_p.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/dialogstestutils_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>
#include <QtQuickControls2/qquickstyle.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickDialogTestUtils;
using namespace QQuickControlsTestUtils;

class tst_QQuickFontDialogImpl : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickFontDialogImpl();
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
    void changingWritingSystem();
    void clickAroundInTheFamilyListView();
    void settingUnderlineAndStrikeoutEffects();
    void changeFontSize();
    void changeDialogTitle();
    void searchFamily();
    void setCurrentFontFromApi();
    void sensibleDefaults();

private:
    QQuickAbstractButton *findDialogButton(QQuickDialogButtonBox *box, const QString &buttonText)
    {
        for (int i = 0; i < box->count(); ++i) {
            auto button = qobject_cast<QQuickAbstractButton *>(box->itemAt(i));
            if (button && button->text().toUpper() == buttonText.toUpper())
                return button;
        }
        return nullptr;
    }

    bool closePopup(DialogTestHelper<QQuickFontDialog, QQuickFontDialogImpl> *dialogHelper,
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
            failureMessage =
                    QLatin1String("Couldn't find a button with text '%1'").arg(dialogButton);
            return false;
        }

        bool clicked = clickButton(openButton);

        if (!clicked) {
            failureMessage = QLatin1String("'%1' button was never clicked").arg(dialogButton);
            return false;
        }

        bool visible = dialogHelper->dialog->isVisible();
        if (visible) {
            failureMessage = QLatin1String("Dialog is still visible after clicking the '%1' button")
                                     .arg(dialogButton);
            return false;
        }
        return true;
    }
};

#define CREATE_DIALOG_TEST_HELPER                                                                  \
    DialogTestHelper<QQuickFontDialog, QQuickFontDialogImpl> dialogHelper(this, "fontDialog.qml"); \
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());                   \
    QVERIFY(dialogHelper.waitForWindowActive());

#define CLOSE_DIALOG(BUTTON)                                                                       \
    QString errorMessage;                                                                          \
    QVERIFY2(closePopup(&dialogHelper, BUTTON, errorMessage), qPrintable(errorMessage));           \
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());

// We don't want to fail on warnings until QTBUG-98964 is fixed,
// as we deliberately prevent deferred execution in some of the tests here,
// which causes warnings.
tst_QQuickFontDialogImpl::tst_QQuickFontDialogImpl()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::DoNotFailOnWarnings)
{
}

void tst_QQuickFontDialogImpl::changingWritingSystem()
{
    CREATE_DIALOG_TEST_HELPER

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    const QQuickTextEdit *sampleEdit =
            dialogHelper.quickDialog->findChild<QQuickTextEdit *>("sampleEdit");
    QVERIFY(sampleEdit);

    QCOMPARE(sampleEdit->text(), QFontDatabase::writingSystemSample(QFontDatabase::Any));

    // Check that the font family list view exist and add signal spy.
    const QQuickListView *fontFamilyListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("familyListView");
    QVERIFY(fontFamilyListView);
    QSignalSpy fontFamilyModelSpy(fontFamilyListView, SIGNAL(modelChanged()));

    // Open the ComboBox's popup.
    const QQuickComboBox *writingSystemComboBox = dialogHelper.quickDialog->findChild<QQuickComboBox*>();
    QVERIFY(writingSystemComboBox);
    const QPoint comboBoxCenterPos = writingSystemComboBox->mapToScene({ writingSystemComboBox->width() / 2, writingSystemComboBox->height() / 2 }).toPoint();
    QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier, comboBoxCenterPos);
    QTRY_VERIFY(writingSystemComboBox->popup()->isOpened());

    // "Any" should be selected by default.
    QQuickListView *comboBoxPopupListView = qobject_cast<QQuickListView*>(writingSystemComboBox->popup()->contentItem());
    QVERIFY(comboBoxPopupListView);
    const int anyIndex = QFontDatabase::Any;
    QQuickAbstractButton *anyDelegate = qobject_cast<QQuickAbstractButton*>(findViewDelegateItem(comboBoxPopupListView, anyIndex));
    QVERIFY(anyDelegate);
    QCOMPARE(anyDelegate->text(), QFontDatabase::writingSystemName(QFontDatabase::Any));

    QCOMPARE(fontFamilyModelSpy.size(), 0);

    // Select "Japanese" from the ComboBox.
    const int japaneseIndex = QFontDatabase::Japanese;
    QQuickAbstractButton *japaneseDelegate = qobject_cast<QQuickAbstractButton*>(findViewDelegateItem(comboBoxPopupListView, japaneseIndex));
    QVERIFY(japaneseDelegate);
    QCOMPARE(japaneseDelegate->text(), QFontDatabase::writingSystemName(QFontDatabase::Japanese));
    QVERIFY(clickButton(japaneseDelegate));
    QTRY_VERIFY(!writingSystemComboBox->popup()->isVisible());

    // Check that the contents of the font family listview changed
    QCOMPARE(fontFamilyModelSpy.size(), 1);

    // And that the sample text is correctly set
    QCOMPARE(sampleEdit->text(), QFontDatabase::writingSystemSample(QFontDatabase::Japanese));

    // Then accept it to close it.
    auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
    QVERIFY(dialogButtonBox);
    QQuickAbstractButton* openButton = findDialogButton(dialogButtonBox, "Ok");
    QVERIFY(openButton);
    QVERIFY(clickButton(openButton));
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFontDialogImpl::clickAroundInTheFamilyListView()
{
    CREATE_DIALOG_TEST_HELPER

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    QQuickListView *fontFamilyListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("familyListView");
    QVERIFY(fontFamilyListView);

    const QQuickListView *fontStyleListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("styleListView");
    QVERIFY(fontStyleListView);

    const QQuickListView *fontSizeListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("sizeListView");
    QVERIFY(fontSizeListView);

    QSignalSpy selectedFontSpy(dialogHelper.dialog, SIGNAL(selectedFontChanged()));
    QSignalSpy styleModelSpy(fontStyleListView, SIGNAL(modelChanged()));
    QSignalSpy sizeModelSpy(fontSizeListView, SIGNAL(modelChanged()));

    QCOMPARE(fontFamilyListView->currentIndex(), 0);
    QCOMPARE(fontStyleListView->currentIndex(), 0);
    QCOMPARE(fontSizeListView->currentIndex(), 0);

    auto fontListModel = QFontDatabase::families(QFontDatabase::WritingSystem::Any);
    fontListModel.removeIf(QFontDatabase::isPrivateFamily);

    // In case the font database contains a significant number of font families
    const int maxNumberOfFamiliesToTest = 10;

    for (auto i = 1; i < qMin(fontListModel.size(), maxNumberOfFamiliesToTest); ++i) {
        selectedFontSpy.clear();
        styleModelSpy.clear();
        sizeModelSpy.clear();

        const QString err = QString("LOOP INDEX %1, EXPECTED %2, ACTUAL %3").arg(i);

        const QFont selectedFont = dialogHelper.dialog->selectedFont();

        const auto oldStyleModel = fontStyleListView->model();
        const auto oldSizeModel = fontSizeListView->model();

        const QString expected1 = fontListModel[i - 1], actual1 = selectedFont.family();
        QVERIFY2(expected1 == actual1, qPrintable(err.arg(expected1, actual1)));

        QQuickAbstractButton *fontDelegate =
                qobject_cast<QQuickAbstractButton *>(findViewDelegateItem(fontFamilyListView, i));
        QVERIFY2(fontDelegate, qPrintable(QString("LOOP INDEX %1").arg(i)));

        QVERIFY2(clickButton(fontDelegate), qPrintable(QString("LOOP INDEX %1").arg(i)));

        const QString expected2 = fontListModel[i],
                      actual2 = dialogHelper.dialog->selectedFont().family();
        QVERIFY2(expected2 == actual2, qPrintable(err.arg(expected2, actual2).append(", FONT ").append(fontDelegate->text())));
        const int selectedFontSpyCount = selectedFontSpy.size();
        QVERIFY2(selectedFontSpyCount == 1, qPrintable(err.arg(1).arg(selectedFontSpyCount).append(", FONT ").append(fontDelegate->text())));
        QVERIFY2((oldStyleModel == fontStyleListView->model()) != (styleModelSpy.size() == 1),
                 qPrintable(QString("LOOP INDEX %1").arg(i)));
        QVERIFY2((oldSizeModel == fontSizeListView->model()) != (sizeModelSpy.size() == 1),
                 qPrintable(QString("LOOP INDEX %1").arg(i)));
    }

    CLOSE_DIALOG("Ok");
}

void tst_QQuickFontDialogImpl::settingUnderlineAndStrikeoutEffects()
{
    CREATE_DIALOG_TEST_HELPER

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    QSignalSpy selectedFontSpy(dialogHelper.dialog, SIGNAL(selectedFontChanged()));

    QQuickCheckBox *underlineCheckBox =
            dialogHelper.quickDialog->findChild<QQuickCheckBox *>("underlineEffect");
    QVERIFY(underlineCheckBox);

    QQuickCheckBox *strikeoutCheckBox =
            dialogHelper.quickDialog->findChild<QQuickCheckBox *>("strikeoutEffect");
    QVERIFY(strikeoutCheckBox);

    QVERIFY(!dialogHelper.dialog->selectedFont().underline());
    QVERIFY(!dialogHelper.dialog->selectedFont().strikeOut());

    QVERIFY(clickButton(underlineCheckBox));

    QCOMPARE(selectedFontSpy.size(), 1);
    QVERIFY(dialogHelper.dialog->selectedFont().underline());
    QVERIFY(!dialogHelper.dialog->selectedFont().strikeOut());

    QVERIFY(clickButton(underlineCheckBox));

    QCOMPARE(selectedFontSpy.size(), 2);
    QVERIFY(!dialogHelper.dialog->selectedFont().underline());
    QVERIFY(!dialogHelper.dialog->selectedFont().strikeOut());

    QVERIFY(clickButton(strikeoutCheckBox));

    QCOMPARE(selectedFontSpy.size(), 3);
    QVERIFY(!dialogHelper.dialog->selectedFont().underline());
    QVERIFY(dialogHelper.dialog->selectedFont().strikeOut());

    QVERIFY(clickButton(strikeoutCheckBox));

    QCOMPARE(selectedFontSpy.size(), 4);
    QVERIFY(!dialogHelper.dialog->selectedFont().underline());
    QVERIFY(!dialogHelper.dialog->selectedFont().strikeOut());

    // Then accept it to close it.
    CLOSE_DIALOG("Ok");
}

void tst_QQuickFontDialogImpl::changeFontSize()
{
    CREATE_DIALOG_TEST_HELPER

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    QQuickTextField *sizeEdit = dialogHelper.quickDialog->findChild<QQuickTextField *>("sizeEdit");
    QVERIFY(sizeEdit);

    QQuickListView *fontSizeListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("sizeListView");
    QVERIFY(fontSizeListView);

    const QQuickItemDelegate *firstSizeDelegate =
            qobject_cast<QQuickItemDelegate *>(findViewDelegateItem(fontSizeListView, 0));

    QCOMPARE(dialogHelper.dialog->selectedFont().pointSize(), firstSizeDelegate->text().toInt());

    sizeEdit->setText("15");
    QCOMPARE(dialogHelper.dialog->selectedFont().pointSize(), 15);

    sizeEdit->setText("22");
    QCOMPARE(dialogHelper.dialog->selectedFont().pointSize(), 22);

    // Then accept it to close it.
    CLOSE_DIALOG("Ok");
}

void tst_QQuickFontDialogImpl::changeDialogTitle()
{
    CREATE_DIALOG_TEST_HELPER

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    const auto dialogButtonBox =
            dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox *>();
    QVERIFY(dialogButtonBox);
    QQuickAbstractButton *cancelButton = findDialogButton(dialogButtonBox, "Cancel");
    QVERIFY(cancelButton);

    const QQuickLabel *titleLabel = dialogHelper.quickDialog->header()->findChild<QQuickLabel *>();
    QVERIFY(titleLabel);

    QCOMPARE(titleLabel->text(), QString());

    const QString newTitle1 = QLatin1String("Some random title #1");

    // Dialog must be closed for the title to update
    QVERIFY(clickButton(cancelButton));
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());

    dialogHelper.dialog->setTitle(newTitle1);

    // Reopen the dialog
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    QVERIFY(dialogHelper.dialog->isVisible());
    QTRY_VERIFY(dialogHelper.quickDialog->isVisible());

    QCOMPARE(titleLabel->text(), newTitle1);

    const QString newTitle2 = QLatin1String("Some random other title #2");

    dialogHelper.dialog->setTitle(newTitle2);

    // Shouldn't update unless you reopen the dialog
    QCOMPARE(titleLabel->text(), newTitle1);

    QVERIFY(clickButton(cancelButton));
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Should now be updated
    QCOMPARE(titleLabel->text(), newTitle2);

    CLOSE_DIALOG("Ok");
}

/*
    Represents the expected search logic we use when the user types text into
    one of the read-only text edits.

    As we test against real fonts installed on the system and hence cannot use
    a hard-coded list, we need this helper to ensure that the (non-trivial)
    searching behavior matches what we expect.
*/
class ListSearchHelper
{
public:
    ListSearchHelper(const QStringList &model)
        : m_model(model)
    {
    }

    int expectedCurrentIndexForSearch(const QString &searchText)
    {
        bool redo = false;

        do {
            m_searchText.append(searchText);

            for (int i = 0; i < m_model.size(); ++i) {
                if (m_model.at(i).startsWith(m_searchText, Qt::CaseInsensitive))
                    return i;
            }

            m_searchText.clear();

            redo = !redo;
        } while (redo);

        return -1;
    }

private:
    QStringList m_model;
    QString m_searchText;
};

void tst_QQuickFontDialogImpl::searchFamily()
{
    CREATE_DIALOG_TEST_HELPER

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    const QQuickTextField *familyEdit =
            dialogHelper.quickDialog->findChild<QQuickTextField *>("familyEdit");
    QVERIFY(familyEdit);

    QQuickListView *fontFamilyListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("familyListView");
    QVERIFY(fontFamilyListView);

    const QPoint familyEditCenterPos =
            familyEdit->mapToScene({ familyEdit->width() / 2, familyEdit->height() / 2 }).toPoint();
    QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier, familyEditCenterPos);
    QVERIFY(familyEdit->hasActiveFocus());

    QSignalSpy familyListViewCurrentIndexSpy(fontFamilyListView, SIGNAL(currentIndexChanged()));
    QVERIFY(familyListViewCurrentIndexSpy.isValid());

    const QString alphabet("abcdefghijklmnopqrstuvwxyz");
    QStringList model = QFontDatabase::families(QFontDatabase::WritingSystem::Any);
    model.removeIf(QFontDatabase::isPrivateFamily);

    ListSearchHelper listSearchHelper(model);

    // For each letter in the alphabet, press the corresponding key
    // and check that the relevant delegate item in familyListView is selected.
    for (auto alphabet_it = alphabet.cbegin(); alphabet_it != alphabet.cend(); alphabet_it++) {
        const int previousIndex = fontFamilyListView->currentIndex();

        const char keyEntered = alphabet_it->toLatin1();
        QTest::keyClick(dialogHelper.window(), keyEntered);

        int expectedListViewIndex = listSearchHelper.expectedCurrentIndexForSearch(QString(keyEntered));
        if (expectedListViewIndex == -1) {
            // There was no match, so the currentIndex should remain unchanged.
            expectedListViewIndex = previousIndex;
        }
        if (fontFamilyListView->currentIndex() == expectedListViewIndex) {
            // Working as expected; keep testing.
            continue;
        }

        // Get the actual text of the current delegate item and the expected text.
        auto currentDelegateItem = findViewDelegateItem(fontFamilyListView, fontFamilyListView->currentIndex());
        QVERIFY(currentDelegateItem);
        const auto actualDelegateText = currentDelegateItem->property("text").toString();
        const auto expectedDelegateText = expectedListViewIndex != -1 ? model.at(expectedListViewIndex) : "(none)";

        QFAIL(qPrintable(QString::fromLatin1("Expected fontFamilyListView to"
            " change its currentIndex to %1 (%2) after typing '%3', but it is %4 (%5)")
                .arg(expectedListViewIndex)
                .arg(expectedDelegateText)
                .arg(keyEntered)
                .arg(fontFamilyListView->currentIndex())
                .arg(actualDelegateText)));
    }

    CLOSE_DIALOG("Ok");
}

void tst_QQuickFontDialogImpl::setCurrentFontFromApi()
{
    CREATE_DIALOG_TEST_HELPER

    QSignalSpy selectedFontSpy(dialogHelper.dialog, SIGNAL(selectedFontChanged()));

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    QQuickListView *fontFamilyListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("familyListView");
    QVERIFY(fontFamilyListView);

    QQuickListView *fontStyleListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("styleListView");
    QVERIFY(fontStyleListView);

    QQuickListView *fontSizeListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("sizeListView");
    QVERIFY(fontSizeListView);

    QQuickTextField *fontSizeEdit =
            dialogHelper.quickDialog->findChild<QQuickTextField *>("sizeEdit");
    QVERIFY(fontSizeEdit);

    // From when the listviews are populated
    QCOMPARE(selectedFontSpy.size(), 1);

    selectedFontSpy.clear();

    // testing some available fonts
    const auto familyModel = fontFamilyListView->model().toStringList();
    int spyCounter = 0;
    const int maxNumberOfFontFamilies = 10, maxNumberOfStyles = 3;
    int size = 10;
    int familyCounter = 0;
    for (auto family = familyModel.cbegin(); family != familyModel.cend() && familyCounter < maxNumberOfFontFamilies; family++, familyCounter++)
    {
        const QString style = QFontDatabase::styles(*family).constFirst();

        const QFont f = QFontDatabase::font(*family, style, size);
        dialogHelper.dialog->setSelectedFont(f);

        const int familyIndex = fontFamilyListView->currentIndex();
        QVERIFY(familyIndex >= 0);
        QCOMPARE(familyModel.at(familyIndex), *family);

        const QStringList styleModel = fontStyleListView->model().toStringList();
        QVERIFY(!styleModel.isEmpty()); // Any valid font should always have at least one style.
        QCOMPARE(styleModel.at(fontStyleListView->currentIndex()), style);
        QCOMPARE(fontSizeEdit->text(), QString::number(size++));

        QCOMPARE(selectedFontSpy.size(), ++spyCounter);

        for (int styleIt = 0; styleIt < qMin(styleModel.size(), maxNumberOfStyles); ++styleIt) {
            const QString currentStyle = styleModel.at(styleIt);

            const QFont f = QFontDatabase::font(*family, currentStyle, size);
            dialogHelper.dialog->setSelectedFont(f);

            QCOMPARE(styleModel.at(fontStyleListView->currentIndex()), currentStyle);
            QCOMPARE(selectedFontSpy.size(), ++spyCounter);
        }
    }

    CLOSE_DIALOG("Ok");
}

void tst_QQuickFontDialogImpl::sensibleDefaults()
{
    CREATE_DIALOG_TEST_HELPER

    // Open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    QQuickListView *fontFamilyListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("familyListView");
    QVERIFY(fontFamilyListView);
    QQuickListView *fontStyleListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("styleListView");
    QVERIFY(fontStyleListView);
    QQuickListView *fontSizeListView =
            dialogHelper.quickDialog->findChild<QQuickListView *>("sizeListView");
    QVERIFY(fontSizeListView);
    QQuickTextField *fontFamilyTextField =
            dialogHelper.quickDialog->findChild<QQuickTextField *>("familyEdit");
    QVERIFY(fontFamilyTextField);
    QQuickTextField *fontStyleTextField =
            dialogHelper.quickDialog->findChild<QQuickTextField *>("styleEdit");
    QVERIFY(fontStyleTextField);

    // The indexes should initially be 0.
    QCOMPARE(fontFamilyListView->currentIndex(), 0);
    QCOMPARE(fontStyleListView->currentIndex(), 0);
    QCOMPARE(fontSizeListView->currentIndex(), 0);

    // Setting the family to an empty model
    fontFamilyListView->setModel(QStringList());

    // The listviews for font family and style should now have an empty model with indexes set to -1
    QVERIFY(fontFamilyListView->model().toStringList().isEmpty());
    QVERIFY(fontStyleListView->model().toStringList().isEmpty());
    QCOMPARE(fontFamilyListView->currentIndex(), -1);
    QCOMPARE(fontStyleListView->currentIndex(), -1);
    QVERIFY(fontFamilyTextField->text().isEmpty());
    QVERIFY(fontStyleTextField->text().isEmpty());

    CLOSE_DIALOG("Ok");
}

QTEST_MAIN(tst_QQuickFontDialogImpl)

#include "tst_qquickfontdialogimpl.moc"
