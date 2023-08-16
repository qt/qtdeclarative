// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtQml/qqmlfile.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/dialogstestutils_p.h>
#include <QtQuickDialogs2/private/qquickfolderdialog_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickplatformfolderdialog_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickfiledialogdelegate_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickfolderbreadcrumbbar_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickfolderbreadcrumbbar_p_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickfolderdialogimpl_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>
#include <QtQuickControls2/qquickstyle.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickDialogTestUtils;
using namespace QQuickControlsTestUtils;

class tst_QQuickFolderDialogImpl : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickFolderDialogImpl();
    static void initMain()
    {
        // We need to set this attribute.
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        // We also don't want to run this for every style, as each one will have
        // different ways of implementing the dialogs.
        // For now we only test one style.
        QQuickStyle::setStyle("Basic");
    }

private slots:
    void initTestCase() override;
    void cleanupTestCase();

    void defaults();
    void chooseFolderViaStandardButtons();
    void bindCurrentFolder_data();
    void bindCurrentFolder();
    void changeFolderViaDoubleClick();
    void changeFolderViaTextEdit();
    void changeFolderViaEnter();
    void cancelDialogWhileTextEditHasFocus();
    void goUp();
    void goUpWhileTextEditHasFocus();
    void goIntoLargeFolder();
    void clickOnBreadcrumb();
    void keyAndShortcutHandling();
    void tabFocusNavigation();
    void acceptRejectLabel();
    void bindTitle();
    void itemsDisabledWhenNecessary();
    void done_data();
    void done();

private:
    QTemporaryDir tempDir;
    QScopedPointer<QFile> tempFile1;
    QScopedPointer<QFile> tempFile2;
    QDir tempSubDir1;
    QDir tempSubSubDir;
    QScopedPointer<QFile> tempSubFile1;
    QScopedPointer<QFile> tempSubFile2;
    QDir tempSubDir2;
    QDir oldCurrentDir;
};

tst_QQuickFolderDialogImpl::tst_QQuickFolderDialogImpl()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickFolderDialogImpl::initTestCase()
{
    QQmlDataTest::initTestCase();

    QVERIFY(tempDir.isValid());
    // QTEST_QUICKCONTROLS_MAIN constructs the test case object once,
    // and then calls qRun() for each style, and qRun() calls initTestCase().
    // So, we need to check if we've already made the temporary directory.
    // Note that this is only necessary if the test is run with more than one style.
    if (!QDir(tempDir.path()).isEmpty())
        return;

    /*
        Create a couple of files within the temporary directory. The structure is:

        [temp directory]
            ├── sub-dir-1
            │   ├── sub-sub-dir
            │   ├── sub-file1.txt
            │   └── sub-file2.txt
            ├── sub-dir-2
            ├── file1.txt
            └── file2.txt
    */
    tempSubDir1 = QDir(tempDir.path());
    QVERIFY2(tempSubDir1.mkdir("sub-dir-1"), qPrintable(QString::fromLatin1(
        "Failed to make sub-directory \"sub-dir-1\" in %1. Permissions are: %2")
            .arg(tempDir.path(), QDebug::toString(QFileInfo(tempDir.path()).permissions()))));
    QVERIFY(tempSubDir1.cd("sub-dir-1"));

    tempSubSubDir = QDir(tempSubDir1.path());
    QVERIFY2(tempSubSubDir.mkdir("sub-sub-dir"), qPrintable(QString::fromLatin1(
        "Failed to make sub-directory \"sub-sub-dir\" in %1. Permissions are: %2")
            .arg(tempSubDir1.path(), QDebug::toString(QFileInfo(tempSubDir1.path()).permissions()))));
    QVERIFY(tempSubSubDir.cd("sub-sub-dir"));

    tempSubFile1.reset(new QFile(tempSubDir1.path() + "/sub-file1.txt"));
    QVERIFY(tempSubFile1->open(QIODevice::ReadWrite));

    tempSubFile2.reset(new QFile(tempSubDir1.path() + "/sub-file2.txt"));
    QVERIFY(tempSubFile2->open(QIODevice::ReadWrite));

    tempSubDir2 = QDir(tempDir.path());
    QVERIFY2(tempSubDir2.mkdir("sub-dir-2"), qPrintable(QString::fromLatin1(
        "Failed to make sub-directory \"sub-dir-2\" in %1. Permissions are: %2")
            .arg(tempDir.path(), QDebug::toString(QFileInfo(tempDir.path()).permissions()))));
    QVERIFY(tempSubDir2.cd("sub-dir-2"));

    tempFile1.reset(new QFile(tempDir.path() + "/file1.txt"));
    QVERIFY(tempFile1->open(QIODevice::ReadWrite));

    tempFile2.reset(new QFile(tempDir.path() + "/file2.txt"));
    QVERIFY(tempFile2->open(QIODevice::ReadWrite));

    // Ensure that each test starts off in the temporary directory.
    oldCurrentDir = QDir::current();
    QDir::setCurrent(tempDir.path());
}

void tst_QQuickFolderDialogImpl::cleanupTestCase()
{
    // Just in case...
    QDir::setCurrent(oldCurrentDir.path());
}

void tst_QQuickFolderDialogImpl::defaults()
{
    QQuickApplicationHelper helper(this, "folderDialog.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickFolderDialog *dialog = window->property("dialog").value<QQuickFolderDialog*>();
    QVERIFY(dialog);
    COMPARE_URL(dialog->currentFolder(), QUrl::fromLocalFile(QDir().absolutePath()));
    // The first file in the directory should be selected, but not until the dialog is actually open,
    // as QQuickFileDialogImpl hasn't been created yet.
    COMPARE_URL(dialog->selectedFolder(), QUrl());
    QCOMPARE(dialog->title(), QString());

    dialog->open();
    QQuickFolderDialogImpl *quickDialog = window->findChild<QQuickFolderDialogImpl*>();
    QVERIFY(quickDialog);
    QTRY_VERIFY(quickDialog->isOpened());
    QVERIFY(quickDialog);
    COMPARE_URL(quickDialog->selectedFolder(), QUrl::fromLocalFile(tempSubDir1.path()));
    COMPARE_URL(quickDialog->currentFolder(), QUrl::fromLocalFile(QDir().absolutePath()));
    COMPARE_URL(dialog->selectedFolder(), QUrl::fromLocalFile(tempSubDir1.path()));
    COMPARE_URL(dialog->currentFolder(), QUrl::fromLocalFile(QDir().absolutePath()));
    QCOMPARE(quickDialog->title(), QString());
}

typedef DialogTestHelper<QQuickFolderDialog, QQuickFolderDialogImpl> FolderDialogTestHelper;

class FolderDialogSignalHelper
{
public:
    FolderDialogSignalHelper(const FolderDialogTestHelper &dialogTestHelper) :
        dialogSelectedFolderChangedSpy(dialogTestHelper.dialog, SIGNAL(selectedFolderChanged())),
        quickDialogSelectedFolderChangedSpy(dialogTestHelper.quickDialog, SIGNAL(selectedFolderChanged(QUrl))),
        dialogCurrentFolderChangedSpy(dialogTestHelper.dialog, SIGNAL(currentFolderChanged())),
        quickDialogCurrentFolderChangedSpy(dialogTestHelper.quickDialog, SIGNAL(currentFolderChanged(QUrl)))
    {
        if (!dialogSelectedFolderChangedSpy.isValid())
            errorMessage = "selectedFolderChanged signal of dialog is not valid";
        if (!quickDialogSelectedFolderChangedSpy.isValid())
            errorMessage = "selectedFolderChanged signal of quickDialog is not valid";
        if (!dialogCurrentFolderChangedSpy.isValid())
            errorMessage = "currentFolderChanged signal of dialog is not valid";
        if (!quickDialogCurrentFolderChangedSpy.isValid())
            errorMessage = "currentFolderChanged signal of quickDialog is not valid";
    }

    QSignalSpy dialogSelectedFolderChangedSpy;
    QSignalSpy quickDialogSelectedFolderChangedSpy;
    QSignalSpy dialogCurrentFolderChangedSpy;
    QSignalSpy quickDialogCurrentFolderChangedSpy;

    QByteArray errorMessage;
};

void tst_QQuickFolderDialogImpl::chooseFolderViaStandardButtons()
{
    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "folderDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Select the delegate by clicking once.
    const FolderDialogSignalHelper signalHelper(dialogHelper);
    QVERIFY2(signalHelper.errorMessage.isEmpty(), signalHelper.errorMessage);
    auto folderDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("folderDialogListView");
    QVERIFY(folderDialogListView);
    QQuickFileDialogDelegate *delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(folderDialogListView, 1, delegate));
    COMPARE_URL(delegate->file(), QUrl::fromLocalFile(tempSubDir2.path()));
    QVERIFY(clickButton(delegate));
    // currentFolder shouldn't change just from a single click.
    COMPARE_URL(dialogHelper.quickDialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    COMPARE_URL(dialogHelper.quickDialog->selectedFolder(), QUrl::fromLocalFile(tempSubDir2.path()));
    COMPARE_URL(dialogHelper.dialog->selectedFolder(), QUrl::fromLocalFile(tempSubDir2.path()));
    // Only selectedFile-related signals should be emitted.
    QCOMPARE(signalHelper.dialogSelectedFolderChangedSpy.size(), 1);
    QCOMPARE(signalHelper.quickDialogSelectedFolderChangedSpy.size(), 1);
    QCOMPARE(signalHelper.dialogCurrentFolderChangedSpy.size(), 0);
    QCOMPARE(signalHelper.quickDialogCurrentFolderChangedSpy.size(), 0);

    // Click the "Open" button.
    QVERIFY(dialogHelper.quickDialog->footer());
    auto dialogButtonBox = qobject_cast<QQuickDialogButtonBox*>(dialogHelper.quickDialog->footer());
    QVERIFY(dialogButtonBox);
    QQuickAbstractButton* openButton = findDialogButton(dialogButtonBox, "Open");
    QVERIFY(openButton);
    QVERIFY(clickButton(openButton));
    COMPARE_URL(dialogHelper.dialog->selectedFolder(), QUrl::fromLocalFile(tempSubDir2.path()));
    COMPARE_URL(dialogHelper.quickDialog->selectedFolder(), QUrl::fromLocalFile(tempSubDir2.path()));
    QCOMPARE(signalHelper.dialogSelectedFolderChangedSpy.size(), 1);
    QCOMPARE(signalHelper.quickDialogSelectedFolderChangedSpy.size(), 1);
    QCOMPARE(signalHelper.dialogCurrentFolderChangedSpy.size(), 0);
    QCOMPARE(signalHelper.quickDialogCurrentFolderChangedSpy.size(), 0);
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
    QVERIFY(!dialogHelper.dialog->isVisible());
}

void tst_QQuickFolderDialogImpl::bindCurrentFolder_data()
{
    QTest::addColumn<QUrl>("initialFolder");
    QTest::addColumn<QUrl>("expectedFolder");
    QTest::addColumn<QStringList>("expectedVisibleFiles");

    const auto currentDirUrl = QUrl::fromLocalFile(QDir::current().path());
    const auto tempSubDirUrl = QUrl::fromLocalFile(tempSubDir1.path());
    const auto tempSubFile1Url = QUrl::fromLocalFile(tempSubFile1->fileName());

    const QStringList currentDirFiles = { tempSubDir1.path(), tempSubDir2.path() };
    const QStringList tempSubDirFiles = { tempSubSubDir.path() };

    // Setting the folder to "sub-dir-1" should result in "sub-sub-dir" being visible.
    QTest::addRow("sub-dir-1") << tempSubDirUrl << tempSubDirUrl << tempSubDirFiles;
    // Setting a file as the folder shouldn't work, and the dialog shouldn't change its folder.
    QTest::addRow("sub-dir/sub-file1.txt") << tempSubFile1Url << currentDirUrl << currentDirFiles;
}

void tst_QQuickFolderDialogImpl::bindCurrentFolder()
{
    QFETCH(QUrl, initialFolder);
    QFETCH(QUrl, expectedFolder);
    QFETCH(QStringList, expectedVisibleFiles);

    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", initialFolder }});
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    COMPARE_URL(dialogHelper.dialog->currentFolder(), expectedFolder);

    auto folderDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("folderDialogListView");
    QVERIFY(folderDialogListView);
    QString failureMessage;
    // Even waiting for ListView polish and that the FolderListModel's status is ready aren't enough
    // on Windows, apparently, as sometimes there just aren't any delegates by the time we do the check.
    // So, we use QTRY_VERIFY2 each time we call this function just to be safe.
    QTRY_VERIFY2(verifyFileDialogDelegates(folderDialogListView, expectedVisibleFiles, failureMessage), qPrintable(failureMessage));

    // Check that the breadcrumb bar is correct by constructing the expected files from the expectedFolder.
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY2(verifyBreadcrumbDelegates(breadcrumbBar, expectedFolder, failureMessage), qPrintable(failureMessage));
}

void tst_QQuickFolderDialogImpl::changeFolderViaDoubleClick()
{
    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "folderDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Double-click on the "sub-dir-2" delegate.
    const FolderDialogSignalHelper signalHelper(dialogHelper);
    QVERIFY2(signalHelper.errorMessage.isEmpty(), signalHelper.errorMessage);
    auto folderDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("folderDialogListView");
    QVERIFY(folderDialogListView);
    QQuickFileDialogDelegate *delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(folderDialogListView, 1, delegate));
    COMPARE_URL(delegate->file(), QUrl::fromLocalFile(tempSubDir2.path()))
    QVERIFY(doubleClickButton(delegate));
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubDir2.path()))
    COMPARE_URL(dialogHelper.dialog->selectedFolder(), QUrl());
    // selectedFolder is set to the folder when clicked and then set to an empty URL after
    // the double click.
    QCOMPARE(signalHelper.dialogSelectedFolderChangedSpy.size(), 2);
    QCOMPARE(signalHelper.quickDialogSelectedFolderChangedSpy.size(), 2);
    QCOMPARE(signalHelper.dialogCurrentFolderChangedSpy.size(), 1);
    QCOMPARE(signalHelper.quickDialogCurrentFolderChangedSpy.size(), 1);
    // Since we only changed the current folder, the dialog should still be open.
    QVERIFY(dialogHelper.dialog->isVisible());

    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFolderDialogImpl::changeFolderViaTextEdit()
{
    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "folderDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Get the text edit visible with Ctrl+L.
    const auto editPathKeySequence = QKeySequence(Qt::CTRL | Qt::Key_L);
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(breadcrumbBar->textField()->isVisible());
    QCOMPARE(breadcrumbBar->textField()->text(), dialogHelper.dialog->currentFolder().toLocalFile());
    QCOMPARE(breadcrumbBar->textField()->selectedText(), breadcrumbBar->textField()->text());

    // Enter the path to the folder in the text edit.
    enterText(dialogHelper.window(), tempSubDir2.path());
    QCOMPARE(breadcrumbBar->textField()->text(), tempSubDir2.path());

    // Hit enter to accept.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubDir2.path()));
    COMPARE_URL(dialogHelper.quickDialog->currentFolder(), QUrl::fromLocalFile(tempSubDir2.path()));
    // We changed into a directory with no folders, so the selected folder should be invalid.
    COMPARE_URL(dialogHelper.dialog->selectedFolder(), QUrl());
    COMPARE_URL(dialogHelper.quickDialog->selectedFolder(), QUrl());
    QTRY_VERIFY(dialogHelper.dialog->isVisible());

    // Close the dialog.
    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFolderDialogImpl::changeFolderViaEnter()
{
    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "folderDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // The first delegate in the view should be selected and have focus.
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    auto folderDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("folderDialogListView");
    QVERIFY(folderDialogListView);
    QQuickFileDialogDelegate *subDir1Delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(folderDialogListView, 0, subDir1Delegate));
    COMPARE_URL(subDir1Delegate->file(), QUrl::fromLocalFile(tempSubDir1.path()));
    QVERIFY(subDir1Delegate->hasActiveFocus());

    // Select the delegate by pressing enter.
    const FolderDialogSignalHelper signalHelper(dialogHelper);
    QVERIFY2(signalHelper.errorMessage.isEmpty(), signalHelper.errorMessage);
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubDir1.path()));
    COMPARE_URL(dialogHelper.dialog->selectedFolder(), QUrl::fromLocalFile(tempSubSubDir.path()));
    QCOMPARE(signalHelper.dialogSelectedFolderChangedSpy.size(), 1);
    QCOMPARE(signalHelper.quickDialogSelectedFolderChangedSpy.size(), 1);
    QCOMPARE(signalHelper.dialogCurrentFolderChangedSpy.size(), 1);
    QCOMPARE(signalHelper.quickDialogCurrentFolderChangedSpy.size(), 1);
    // Since we only changed the current folder, the dialog should still be open.
    QVERIFY(dialogHelper.dialog->isVisible());

    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFolderDialogImpl::cancelDialogWhileTextEditHasFocus()
{
    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "folderDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Get the text edit visible with Ctrl+L.
    const auto editPathKeySequence = QKeySequence(Qt::CTRL | Qt::Key_L);
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(breadcrumbBar->textField()->isVisible());
    QCOMPARE(breadcrumbBar->textField()->text(), dialogHelper.dialog->currentFolder().toLocalFile());
    QCOMPARE(breadcrumbBar->textField()->selectedText(), breadcrumbBar->textField()->text());

    // Close it via the cancel button.
    auto dialogButtonBox = qobject_cast<QQuickDialogButtonBox*>(dialogHelper.quickDialog->footer());
    QVERIFY(dialogButtonBox);
    const QString cancelText = QQuickDialogButtonBoxPrivate::buttonText(QPlatformDialogHelper::Cancel);
    auto cancelButton = findDialogButton(dialogButtonBox, cancelText);
    QVERIFY(cancelButton);
    QVERIFY(clickButton(cancelButton));

    // Open it again. The text field should not be visible, but the breadcrumb bar itself should be.
    dialogHelper.dialog->open();
    QVERIFY(dialogHelper.dialog->isVisible());
    QTRY_VERIFY(dialogHelper.quickDialog->isOpened());
    QVERIFY(breadcrumbBar->isVisible());
    // The ListView that contains the breadcrumb delegates should be visible.
    QVERIFY(breadcrumbBar->contentItem()->isVisible());
    QVERIFY(!breadcrumbBar->textField()->isVisible());
}

void tst_QQuickFolderDialogImpl::goUp()
{
    // Open the dialog, starting off in "sub-dir-1".
    FolderDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(tempSubDir1.path()) }});
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Go up a directory via the button next to the breadcrumb bar.
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    auto barListView = qobject_cast<QQuickListView*>(breadcrumbBar->contentItem());
    QVERIFY(barListView);
    if (QQuickTest::qIsPolishScheduled(barListView))
        QVERIFY(QQuickTest::qWaitForPolish(barListView));
    QVERIFY(clickButton(breadcrumbBar->upButton()));
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    // The previous directory that we were in should now be selected (matches e.g. Windows and Ubuntu).
    COMPARE_URL(dialogHelper.dialog->selectedFolder(), QUrl::fromLocalFile(tempSubDir1.path()));
    auto folderDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("folderDialogListView");
    QVERIFY(folderDialogListView);
    QQuickFileDialogDelegate *subDirDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(folderDialogListView, 0, subDirDelegate));
    QCOMPARE(subDirDelegate->isHighlighted(), true);

    // Go up a directory via the keyboard shortcut.
    const auto goUpKeySequence = QKeySequence(Qt::ALT | Qt::Key_Up);
    QTest::keySequence(dialogHelper.window(), goUpKeySequence);
    QDir tempParentDir(tempDir.path());
    QVERIFY(tempParentDir.cdUp());
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempParentDir.path()));
    COMPARE_URL(dialogHelper.dialog->selectedFolder(), QUrl::fromLocalFile(tempDir.path()));
}

void tst_QQuickFolderDialogImpl::goUpWhileTextEditHasFocus()
{
    // Open the dialog, starting off in "sub-dir-1".
    FolderDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(tempSubDir1.path()) }});
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Go up a directory via the button next to the breadcrumb bar.
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    auto barListView = qobject_cast<QQuickListView*>(breadcrumbBar->contentItem());
    QVERIFY(barListView);
    if (QQuickTest::qIsPolishScheduled(barListView))
        QVERIFY(QQuickTest::qWaitForPolish(barListView));
    QVERIFY(clickButton(breadcrumbBar->upButton()));
    // The path should have changed to the parent directory.
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    // The text edit should be hidden when it loses focus.
    QVERIFY(!breadcrumbBar->textField()->hasActiveFocus());
    QVERIFY(!breadcrumbBar->textField()->isVisible());
    // The focus should be given to the first delegate.
    QVERIFY(dialogHelper.window()->activeFocusItem());
    auto folderDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("folderDialogListView");
    QVERIFY(folderDialogListView);
    QQuickFileDialogDelegate *firstDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(folderDialogListView, 0, firstDelegate));
    QCOMPARE(dialogHelper.window()->activeFocusItem(), firstDelegate);
}

void tst_QQuickFolderDialogImpl::goIntoLargeFolder()
{
    // Create a throwaway directory with a lot of folders within it...
    QTemporaryDir anotherTempDir;
    QVERIFY(anotherTempDir.isValid());
    for (int i = 0; i < 30; ++i) {
        QDir newDir(anotherTempDir.path());
        QVERIFY(newDir.exists());
        // Pad with zeroes so that the directories are ordered as we expect.
        QVERIFY(newDir.mkdir(QString::fromLatin1("dir%1").arg(i, 2, 10, QLatin1Char('0'))));
    }

    // ... and within one of those folders, more folders.
    QDir dir20(anotherTempDir.path() + "/dir20");
    QVERIFY(dir20.exists());
    for (int i = 0; i < 30; ++i) {
        QDir newDir(dir20.path());
        QVERIFY(newDir.exists());
        QVERIFY(newDir.mkdir(QString::fromLatin1("subdir%1").arg(i, 2, 10, QLatin1Char('0'))));
    }

    // Open the dialog. Start off in the throwaway directory.
    FolderDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(anotherTempDir.path()) }});
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // If the screen is so tall that the contentItem is not vertically larger than the view,
    // then the test makes no sense.
    auto folderDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("folderDialogListView");
    QVERIFY(folderDialogListView);
    if (QQuickTest::qIsPolishScheduled(folderDialogListView))
        QVERIFY(QQuickTest::qWaitForPolish(folderDialogListView));
    // Just to be safe, make sure it's at least twice as big.
    if (folderDialogListView->contentItem()->height() < folderDialogListView->height() * 2) {
        QSKIP(qPrintable(QString::fromLatin1("Expected height of folderDialogListView's contentItem (%1)" \
            " to be at least twice as big as the ListView's height (%2)")
                .arg(folderDialogListView->contentItem()->height()).arg(folderDialogListView->height())));
    }

    // Scroll down to dir20. The view should be somewhere past the middle.
    QVERIFY(QMetaObject::invokeMethod(folderDialogListView, "positionViewAtIndex", Qt::DirectConnection,
        Q_ARG(int, 20), Q_ARG(int, QQuickItemView::PositionMode::Center)));
    QVERIFY(folderDialogListView->contentY() > 0);

    // Go into it. The view should start at the top of the directory, not at the same contentY
    // that it had in the previous directory.
    QQuickFileDialogDelegate *dir20Delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(folderDialogListView, 20, dir20Delegate));
    COMPARE_URL(dir20Delegate->file(), QUrl::fromLocalFile(dir20.path()));
    QVERIFY(doubleClickButton(dir20Delegate));
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(dir20.path()));
    COMPARE_URL(dialogHelper.quickDialog->currentFolder(), QUrl::fromLocalFile(dir20.path()));
    QCOMPARE(folderDialogListView->contentY(), 0);
}

void tst_QQuickFolderDialogImpl::clickOnBreadcrumb()
{
    // Open the dialog, starting off in "sub-dir-1".
    FolderDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(tempSubDir1.path()) }});
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Find the breadcrumb bar delegate for the temp directory (parent of "sub-dir-1").
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    auto breadcrumbBarListView = qobject_cast<QQuickListView*>(breadcrumbBar->contentItem());
    QVERIFY(breadcrumbBarListView);
    QQuickAbstractButton *breadcrumbDelegate = nullptr;
    // The following breadcrumb bar has 5 delegates:
    // [ / > tmp > deadbeef ]
    // We can assume that, regardless of which system we're running on, we'll have
    // at least 3: one for the temp directory, and one for the > separator, and one for "sub-dir-1".
    QVERIFY(breadcrumbBarListView->count() >= 3);
    // - 1 to make it zero-based, and another 2 to get to the second-last directory, which is the parent of "sub-dir-1".
    QTRY_VERIFY(findViewDelegateItem(breadcrumbBarListView, breadcrumbBarListView->count() - 3, breadcrumbDelegate));

    QVERIFY(clickButton(breadcrumbDelegate));
    QCOMPARE(dialogHelper.quickDialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    QCOMPARE(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    QCOMPARE(dialogHelper.quickDialog->selectedFolder(), QUrl::fromLocalFile(tempSubDir1.path()));
    QCOMPARE(dialogHelper.dialog->selectedFolder(), QUrl::fromLocalFile(tempSubDir1.path()));
}

void tst_QQuickFolderDialogImpl::keyAndShortcutHandling()
{
    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "folderDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Get the text edit visible with Ctrl+L.
    const auto editPathKeySequence = QKeySequence(Qt::CTRL | Qt::Key_L);
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(breadcrumbBar->textField()->isVisible());
    QCOMPARE(breadcrumbBar->textField()->text(), dialogHelper.dialog->currentFolder().toLocalFile());
    QCOMPARE(breadcrumbBar->textField()->selectedText(), breadcrumbBar->textField()->text());

    // Ctrl+L shouldn't hide it.
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    QVERIFY(breadcrumbBar->textField()->isVisible());

    // Cancel it with the escape key.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Escape);
    QVERIFY(!breadcrumbBar->textField()->isVisible());
    QVERIFY(dialogHelper.dialog->isVisible());

    // Make it visible.
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    QVERIFY(breadcrumbBar->textField()->isVisible());

    // Cancel it with the escape key again.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Escape);
    QVERIFY(!breadcrumbBar->textField()->isVisible());
    QVERIFY(dialogHelper.dialog->isVisible());

    // Pressing escape now should close the dialog.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Escape);
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFolderDialogImpl::tabFocusNavigation()
{
    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "folderDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    QList<QQuickItem*> expectedFocusItems;

    // The initial focus should be on the first delegate.
    auto folderDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("folderDialogListView");
    QVERIFY(folderDialogListView);
    QQuickFileDialogDelegate *firstDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(folderDialogListView, 0, firstDelegate));
    expectedFocusItems << firstDelegate;

    // Next, the left-most dialog button.
    auto dialogButtonBox = qobject_cast<QQuickDialogButtonBox*>(dialogHelper.quickDialog->footer());
    QVERIFY(dialogButtonBox);
    QCOMPARE(dialogButtonBox->count(), 2);
    auto leftMostButton = qobject_cast<QQuickAbstractButton*>(dialogButtonBox->itemAt(0));
    QVERIFY(leftMostButton);
    expectedFocusItems << leftMostButton;

    // Then, the right-most dialog button.
    auto rightMostButton = qobject_cast<QQuickAbstractButton*>(dialogButtonBox->itemAt(1));
    QVERIFY(rightMostButton);
    expectedFocusItems << rightMostButton;

    // Then, the up button.
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    expectedFocusItems << breadcrumbBar->upButton();

    // Finally, add each bread crumb delegate.
    for (int i = 0; i < folderDialogListView->count(); ++i) {
        QQuickFileDialogDelegate *delegate = nullptr;
        QTRY_VERIFY(findViewDelegateItem(folderDialogListView, i, delegate));
        expectedFocusItems << delegate;
    }

    // Tab through each item, checking the focus after each.
    for (auto expectedFocusItem : std::as_const(expectedFocusItems)) {
        // Check the focus item first so that we account for the first item.
        // Print detailed failure message as workaround for QTBUG-92102.
        QVERIFY2(dialogHelper.window()->activeFocusItem() == expectedFocusItem, qPrintable(QString::fromLatin1(
            "\n   Actual:   %1\n   Expected: %2").arg(QDebug::toString(dialogHelper.window()->activeFocusItem()))
                .arg(QDebug::toString(expectedFocusItem))));

        if (expectedFocusItem != expectedFocusItems.last())
            QTest::keyClick(dialogHelper.window(), Qt::Key_Tab);
    }

    // Ensure the order is reversed when shift-tabbing.
    std::reverse(expectedFocusItems.begin(), expectedFocusItems.end());
    // We know the first (last) item has focus already, so skip it.
    expectedFocusItems.removeFirst();
    for (auto expectedFocusItem : std::as_const(expectedFocusItems)) {
        QTest::keyClick(dialogHelper.window(), Qt::Key_Tab, Qt::ShiftModifier);

        QCOMPARE(dialogHelper.window()->activeFocusItem(), expectedFocusItem);
    }
}

void tst_QQuickFolderDialogImpl::acceptRejectLabel()
{
    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "acceptRejectLabel.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Check that the accept and reject buttons' labels have changed.
    auto dialogButtonBox = qobject_cast<QQuickDialogButtonBox*>(dialogHelper.quickDialog->footer());
    QVERIFY(dialogButtonBox);
    QVERIFY(findDialogButton(dialogButtonBox, "AcceptTest"));
    QVERIFY(findDialogButton(dialogButtonBox, "RejectTest"));

    // Close the dialog.
    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());

    // Reset back to the default text.
    dialogHelper.dialog->resetAcceptLabel();
    dialogHelper.dialog->resetRejectLabel();

    // Re-open the dialog.
    dialogHelper.dialog->open();
    QVERIFY(dialogHelper.dialog->isVisible());
    QTRY_VERIFY(dialogHelper.quickDialog->isOpened());

    // Check that the defaults are back.
    const QString openText = QQuickDialogButtonBoxPrivate::buttonText(QPlatformDialogHelper::Open);
    QVERIFY2(findDialogButton(dialogButtonBox, openText), qPrintable(QString::fromLatin1(
        "Failed to find dialog button with text \"%1\"").arg(openText)));
    const QString cancelText = QQuickDialogButtonBoxPrivate::buttonText(QPlatformDialogHelper::Cancel);
    QVERIFY2(findDialogButton(dialogButtonBox, cancelText), qPrintable(QString::fromLatin1(
        "Failed to find dialog button with text \"%1\"").arg(cancelText)));
}

void tst_QQuickFolderDialogImpl::bindTitle()
{
    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "bindTitle.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Open the dialog and check that the correct title is displayed.
    const QString expectedTitle = QLatin1String("Test Title");
    QCOMPARE(dialogHelper.dialog->title(), expectedTitle);
    QCOMPARE(dialogHelper.quickDialog->title(), expectedTitle);
    auto header = dialogHelper.quickDialog->header();
    QVERIFY(header);
    auto dialogTitleBarLabel = dialogHelper.quickDialog->header()->findChild<QQuickLabel*>("dialogTitleBarLabel");
    QVERIFY(dialogTitleBarLabel);
    QCOMPARE(dialogTitleBarLabel->text(), expectedTitle);
}

void tst_QQuickFolderDialogImpl::itemsDisabledWhenNecessary()
{
    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(tempSubDir2.path()) }});
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubDir2.path()));
    COMPARE_URL(dialogHelper.quickDialog->currentFolder(), QUrl::fromLocalFile(tempSubDir2.path()));

    // We opened it in a folder that has no files, so the Open button should be disabled.
    QVERIFY(dialogHelper.quickDialog->footer());
    auto dialogButtonBox = qobject_cast<QQuickDialogButtonBox*>(dialogHelper.quickDialog->footer());
    QVERIFY(dialogButtonBox);
    QQuickAbstractButton* openButton = findDialogButton(dialogButtonBox, "Open");
    QVERIFY(openButton);
    QCOMPARE(openButton->isEnabled(), false);

    // Now go up. The Open button should now be enabled.
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(clickButton(breadcrumbBar->upButton()));
    QCOMPARE(openButton->isEnabled(), true);
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    COMPARE_URL(dialogHelper.quickDialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));

    // Get the text edit visible with Ctrl+L. The Open button should now be disabled.
    const auto editPathKeySequence = QKeySequence(Qt::CTRL | Qt::Key_L);
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    QVERIFY(breadcrumbBar->textField()->isVisible());
    QCOMPARE(openButton->isEnabled(), false);

    // Hide it with the escape key. The Open button should now be enabled.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Escape);
    QVERIFY(!breadcrumbBar->textField()->isVisible());
    QCOMPARE(openButton->isEnabled(), true);
}

void tst_QQuickFolderDialogImpl::done_data()
{
    QTest::addColumn<QQuickAbstractDialog::StandardCode>("result");

    QTest::newRow("Accepted") << QQuickAbstractDialog::Accepted;
    QTest::newRow("Rejected") << QQuickAbstractDialog::Rejected;
}

void tst_QQuickFolderDialogImpl::done()
{
    QFETCH(QQuickAbstractDialog::StandardCode, result);

    // Open the dialog.
    FolderDialogTestHelper dialogHelper(this, "folderDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Call the "done" slot manually.
    switch (result) {
    case QQuickAbstractDialog::Accepted:
        QVERIFY(QMetaObject::invokeMethod(dialogHelper.window(), "doneAccepted"));
        break;
    case QQuickAbstractDialog::Rejected:
        QVERIFY(QMetaObject::invokeMethod(dialogHelper.window(), "doneRejected"));
        break;
    }

    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
    QCOMPARE(dialogHelper.dialog->result(), result);
}

QTEST_MAIN(tst_QQuickFolderDialogImpl)

#include "tst_qquickfolderdialogimpl.moc"
