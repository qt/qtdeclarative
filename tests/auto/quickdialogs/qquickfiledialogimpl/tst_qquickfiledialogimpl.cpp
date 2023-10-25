// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qloggingcategory.h>
#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtQml/qqmlfile.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickControlsTestUtils/private/dialogstestutils_p.h>
#include <QtQuickDialogs2/private/qquickfiledialog_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickplatformfiledialog_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickfiledialogdelegate_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickfiledialogimpl_p_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickfolderbreadcrumbbar_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickfolderbreadcrumbbar_p_p.h>
#include <QtQuickDialogs2Utils/private/qquickfilenamefilter_p.h>
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

Q_LOGGING_CATEGORY(lcTest, "qt.quick.dialogs.tests.quickfiledialogimpl")

class tst_QQuickFileDialogImpl : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickFileDialogImpl();
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
    void init() override;
    void cleanupTestCase();

    void defaults();
    void chooseFileViaStandardButtons();
    void chooseFileViaDoubleClick();
    void chooseFileViaTextEdit();
    void chooseFileViaEnter();
    void bindCurrentFolder_data();
    void bindCurrentFolder();
    void changeFolderViaStandardButtons();
    void changeFolderViaDoubleClick_data();
    void changeFolderViaDoubleClick();
    void chooseFolderViaTextEdit();
    void chooseFolderViaEnter();
    void chooseFileAndThenFolderViaTextEdit();
    void cancelDialogWhileTextEditHasFocus();
    void closingDialogCancels();
    void goUp_data();
    void goUp();
    void goUpWhileTextEditHasFocus();
    void goIntoLargeFolder();
    void goUpIntoLargeFolder();
    void keyAndShortcutHandling();
    void bindNameFilters();
    void changeNameFilters();
    void changeNameFiltersAfterChangingFolder();
    void tabFocusNavigation();
    void acceptRejectLabel();
    void bindTitle();
    void itemsDisabledWhenNecessary();
    void fileMode_data();
    void fileMode();
    void defaultSuffix_data();
    void defaultSuffix();
    void done_data();
    void done();
    void setSelectedFile_data();
    void setSelectedFile();
    void selectNewFileViaTextField_data();
    void selectNewFileViaTextField();

private:
    enum DelegateOrderPolicy
    {
        ShowDirectoriesFirst,
        ShowFilesFirst
    };

    QStringList tempDirExpectedVisibleFiles(DelegateOrderPolicy order) const;
    QStringList tempSubDirExpectedVisibleFiles(DelegateOrderPolicy order) const;

    QTemporaryDir tempDir;
    QScopedPointer<QFile> tempFile1;
    QScopedPointer<QFile> tempFile2;
    QDir tempSubDir;
    QDir tempSubSubDir;
    QScopedPointer<QFile> tempSubFile1;
    QScopedPointer<QFile> tempSubFile2;

    QTemporaryDir largeTempDir;
    QStringList largeTempDirPaths;
    QDir largeTempDirLargeSubDir;
    const int largeTempDirLargeSubDirIndex = 80;

    QDir oldCurrentDir;

    const QKeySequence goUpKeySequence = QKeySequence(Qt::ALT | Qt::Key_Up);
    const QKeySequence editPathKeySequence = QKeySequence(Qt::CTRL | Qt::Key_L);
};

QStringList tst_QQuickFileDialogImpl::tempDirExpectedVisibleFiles(DelegateOrderPolicy order) const
{
    return order == ShowDirectoriesFirst
        ? QStringList { tempSubDir.path(), tempFile1->fileName(), tempFile2->fileName() }
        : QStringList { tempFile1->fileName(), tempFile2->fileName(), tempSubDir.path() };
}

QStringList tst_QQuickFileDialogImpl::tempSubDirExpectedVisibleFiles(DelegateOrderPolicy order) const
{
    return order == ShowDirectoriesFirst
        ? QStringList { tempSubSubDir.path(), tempSubFile1->fileName(), tempSubFile2->fileName() }
        : QStringList { tempSubFile1->fileName(), tempSubFile2->fileName(), tempSubSubDir.path() };
}

// We don't want to fail on warnings until QTBUG-98964 is fixed,
// as we deliberately prevent deferred execution in some of the tests here,
// which causes warnings.
tst_QQuickFileDialogImpl::tst_QQuickFileDialogImpl()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::DoNotFailOnWarnings)
{
}

void tst_QQuickFileDialogImpl::initTestCase()
{
    QQmlDataTest::initTestCase();

    qputenv("QT_QUICK_DIALOGS_PRESELECT_FIRST_FILE", "1");

    QVERIFY2(tempDir.isValid(), qPrintable(tempDir.errorString()));
    // QTEST_QUICKCONTROLS_MAIN constructs the test case object once,
    // and then calls qRun() for each style, and qRun() calls initTestCase().
    // So, we need to check if we've already made the temporary directory.
    // Note that this is only necessary if the test is run with more than one style.
    if (!QDir(tempDir.path()).isEmpty())
        return;

    /*
        Create a couple of files within the temporary directory. The structure is:

        [temp directory] (tempDir)
            ├── sub-dir (tempSubDir)
            │   ├── sub-sub-dir (tempSubSubDir)
            │   ├── sub-file1.txt (tempSubFile1)
            │   └── sub-file2.txt (tempSubFile2)
            ├── file1.txt (tempFile1)
            └── file2.txt (tempFile2)
    */
    tempSubDir = QDir(tempDir.path());
    QVERIFY2(tempSubDir.mkdir("sub-dir"), qPrintable(QString::fromLatin1(
        "Failed to make sub-directory \"sub-dir\" in %1. Permissions are: %2")
            .arg(tempDir.path()).arg(QDebug::toString(QFileInfo(tempDir.path()).permissions()))));
    QVERIFY(tempSubDir.cd("sub-dir"));

    tempSubSubDir = QDir(tempSubDir.path());
    QVERIFY2(tempSubSubDir.mkdir("sub-sub-dir"), qPrintable(QString::fromLatin1(
        "Failed to make sub-directory \"sub-sub-dir\" in %1. Permissions are: %2")
            .arg(tempSubDir.path()).arg(QDebug::toString(QFileInfo(tempSubDir.path()).permissions()))));
    QVERIFY(tempSubSubDir.cd("sub-sub-dir"));

    tempSubFile1.reset(new QFile(tempSubDir.path() + "/sub-file1.txt"));
    QVERIFY(tempSubFile1->open(QIODevice::ReadWrite));

    tempSubFile2.reset(new QFile(tempSubDir.path() + "/sub-file2.txt"));
    QVERIFY(tempSubFile2->open(QIODevice::ReadWrite));

    tempFile1.reset(new QFile(tempDir.path() + "/file1.txt"));
    QVERIFY(tempFile1->open(QIODevice::ReadWrite));

    tempFile2.reset(new QFile(tempDir.path() + "/file2.txt"));
    QVERIFY(tempFile2->open(QIODevice::ReadWrite));

    /*
        Create another temporary directory that contains a large amount of folders.
    */
    QVERIFY2(largeTempDir.isValid(), qPrintable(largeTempDir.errorString()));
    const static int largeFileCount = 100;
    const QDir largeTempDirectory(largeTempDir.path());
    for (int i = 0; i < largeFileCount; ++i) {
        // Pad with zeroes so that the directories are ordered as we expect.
        const QString dirName = QString::fromLatin1("dir%1").arg(i, 3, 10, QLatin1Char('0'));
        QVERIFY(largeTempDirectory.mkdir(dirName));
        largeTempDirPaths.append(largeTempDirectory.filePath(dirName));
    }

    // ... and within one of those folders, more folders.
    largeTempDirLargeSubDir = QDir(largeTempDir.path() + "/dir"
        + QString::fromLatin1("%1").arg(largeTempDirLargeSubDirIndex, 3, 10, QLatin1Char('0')));
    QVERIFY(largeTempDirLargeSubDir.exists());
    const QDir largeTempSubDirectory = QDir(largeTempDirLargeSubDir.path());
    for (int i = 0; i < largeFileCount; ++i)
        QVERIFY(largeTempSubDirectory.mkdir(QString::fromLatin1("sub-dir%1").arg(i, 3, 10, QLatin1Char('0'))));

    // Ensure that each test starts off in the temporary directory.
    oldCurrentDir = QDir::current();
    QDir::setCurrent(tempDir.path());
}

void tst_QQuickFileDialogImpl::init()
{
    // Do this before each test function in case the test sets it.
    qputenv("QT_QUICK_DIALOGS_SHOW_DIRS_FIRST", "1");
}

void tst_QQuickFileDialogImpl::cleanupTestCase()
{
    // Just in case...
    QDir::setCurrent(oldCurrentDir.path());
}

#define VERIFY_FILE_SELECTED(expectedCurrentFolderUrl, expectedCurrentFileUrl) \
{ \
    COMPARE_URL(dialogHelper.dialog->selectedFile(), expectedCurrentFileUrl); \
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), { expectedCurrentFileUrl }); \
    /* We also test the deprecated currentFile* API until it's removed. */ \
    COMPARE_URL(dialogHelper.dialog->currentFile(), expectedCurrentFileUrl); \
    COMPARE_URLS(dialogHelper.dialog->currentFiles(), { expectedCurrentFileUrl }); \
    COMPARE_URL(dialogHelper.quickDialog->selectedFile(), expectedCurrentFileUrl); \
    COMPARE_URL(dialogHelper.dialog->currentFolder(), expectedCurrentFolderUrl); \
    COMPARE_URL(dialogHelper.quickDialog->currentFolder(), expectedCurrentFolderUrl); \
}

#define VERIFY_DELEGATE_CURRENT(expectedCurrentFileUrl, expectedCurrentIndex) \
{ \
    QTRY_COMPARE(dialogHelper.fileDialogListView->currentIndex(), expectedCurrentIndex); \
    QQuickFileDialogDelegate *fileDelegate = nullptr; \
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, expectedCurrentIndex, fileDelegate)); \
    COMPARE_URL(fileDelegate->file(), expectedCurrentFileUrl); \
}

#define VERIFY_DELEGATE_FOCUSED(expectedCurrentIndex) \
{ \
    QTRY_COMPARE(dialogHelper.fileDialogListView->currentIndex(), expectedCurrentIndex); \
    QQuickFileDialogDelegate *fileDelegate = nullptr; \
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, expectedCurrentIndex, fileDelegate)); \
    QVERIFY2(fileDelegate->hasActiveFocus(), qPrintable(QString::fromLatin1( \
        "Expected delegate at index %1 to have focus, but %2 has it") \
            .arg(QString::number(expectedCurrentIndex), \
                QDebug::toString(dialogHelper.window()->activeFocusItem())))); \
    QVERIFY(fileDelegate->isHighlighted()); \
}

/*
    Checks that currentFolder, selectedFile, and currentIndex are what we expect.

    It also checks that the relevant delegate in the view is current, has focus, and is highlighted.
    As the FolderListModel (the view's model) is asynchronous, we need to wait for the currentIndex to change.

    Can only be called when the dialog is open.
*/
#define VERIFY_FILE_SELECTED_AND_FOCUSED(expectedCurrentFolderUrl, expectedCurrentFileUrl, expectedCurrentIndex) \
VERIFY_FILE_SELECTED(expectedCurrentFolderUrl, expectedCurrentFileUrl) \
VERIFY_DELEGATE_CURRENT(expectedCurrentFileUrl, expectedCurrentIndex) \
VERIFY_DELEGATE_FOCUSED(expectedCurrentIndex)

class FileDialogTestHelper : public DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl>
{
public:
    FileDialogTestHelper(QQmlDataTest *testCase, const QString &testFilePath,
        const QStringList &qmlImportPaths = {}, const QVariantMap &initialProperties = {});

    bool openDialog() override;

    QPointer<QQuickListView> fileDialogListView;
};

FileDialogTestHelper::FileDialogTestHelper(QQmlDataTest *testCase, const QString &testFilePath,
        const QStringList &qmlImportPaths, const QVariantMap &initialProperties)
    : DialogTestHelper(testCase, testFilePath, qmlImportPaths, initialProperties)
{
}

bool FileDialogTestHelper::openDialog()
{
    if (!DialogTestHelper::openDialog())
        return false;

    fileDialogListView = quickDialog->findChild<QQuickListView*>("fileDialogListView");
    return fileDialogListView != nullptr;
}

void tst_QQuickFileDialogImpl::defaults()
{
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl());
    // It should default to the current directory.
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(QDir().absolutePath()));
    // The first file in the directory should be selected, but not until the dialog is actually open,
    // as QQuickFileDialogImpl hasn't been created yet.
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl());
    COMPARE_URLS(dialogHelper.dialog->currentFiles(), {});
    QCOMPARE(dialogHelper.dialog->title(), QString());

    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    QQuickFileDialogImpl *quickDialog = dialogHelper.window()->findChild<QQuickFileDialogImpl*>();
    QVERIFY(quickDialog);
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(QDir().absolutePath()), QUrl::fromLocalFile(tempSubDir.path()), 0);
    QCOMPARE(quickDialog->title(), QString());
}

void tst_QQuickFileDialogImpl::chooseFileViaStandardButtons()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();

    // Select the delegate by clicking once.
    QSignalSpy dialogSelectedFileChangedSpy(dialogHelper.dialog, SIGNAL(selectedFileChanged()));
    QVERIFY(dialogSelectedFileChangedSpy.isValid());
    QSignalSpy dialogCurrentFileChangedSpy(dialogHelper.dialog, SIGNAL(currentFileChanged()));
    QVERIFY(dialogCurrentFileChangedSpy.isValid());

    QQuickFileDialogDelegate *delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, 2, delegate));
    COMPARE_URL(delegate->file(), QUrl::fromLocalFile(tempFile2->fileName()));
    QVERIFY(clickButton(delegate));
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), QUrl::fromLocalFile(tempFile2->fileName()), 2);
    QCOMPARE(dialogSelectedFileChangedSpy.size(), 1);
    QCOMPARE(dialogCurrentFileChangedSpy.size(), 1);

    // Click the "Open" button.
    QVERIFY(dialogHelper.quickDialog->footer());
    auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
    QVERIFY(dialogButtonBox);
    QQuickAbstractButton* openButton = findDialogButton(dialogButtonBox, "Open");
    QVERIFY(openButton);
    QVERIFY(clickButton(openButton));
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl::fromLocalFile(tempFile2->fileName()));
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), { QUrl::fromLocalFile(tempFile2->fileName()) });
    COMPARE_URL(dialogHelper.quickDialog->selectedFile(), QUrl::fromLocalFile(tempFile2->fileName()));
    QCOMPARE(dialogSelectedFileChangedSpy.size(), 1);
    QCOMPARE(dialogCurrentFileChangedSpy.size(), 1);
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
    QVERIFY(!dialogHelper.dialog->isVisible());
}

void tst_QQuickFileDialogImpl::chooseFileViaDoubleClick()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();

    // Select the delegate by double-clicking.
    QQuickFileDialogDelegate *delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, 2, delegate));
    COMPARE_URL(delegate->file(), QUrl::fromLocalFile(tempFile2->fileName()))
    QVERIFY(doubleClickButton(delegate));
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempFile2->fileName()))
    COMPARE_URLS(dialogHelper.dialog->currentFiles(), { QUrl::fromLocalFile(tempFile2->fileName()) })
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl::fromLocalFile(tempFile2->fileName()))
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), { QUrl::fromLocalFile(tempFile2->fileName()) })
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::chooseFileViaTextEdit()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();
    // Ensure that fileDialogListView has loaded its items, as we force active focus
    // on the current item when we set it in setFileDialogListViewCurrentIndex(),
    // which can make the TextField's visibility check
    // below fail due to it being hidden when it loses activeFocus.
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), QUrl::fromLocalFile(tempSubDir.path()), 0);

    // Get the text edit visible with Ctrl+L.
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(breadcrumbBar->textField()->isVisible());
    QCOMPARE(breadcrumbBar->textField()->text(), dialogHelper.dialog->currentFolder().toLocalFile());
    QCOMPARE(breadcrumbBar->textField()->selectedText(), breadcrumbBar->textField()->text());

    // Enter the path to the file in the text edit.
    enterText(dialogHelper.window(), tempFile2->fileName());
    QCOMPARE(breadcrumbBar->textField()->text(), tempFile2->fileName());

    // Hit enter to accept.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    COMPARE_URL(dialogHelper.quickDialog->selectedFile(), QUrl::fromLocalFile(tempFile2->fileName()));
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl::fromLocalFile(tempFile2->fileName()));
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), { QUrl::fromLocalFile(tempFile2->fileName()) });
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::chooseFileViaEnter()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();

    // Before moving down, the first delegate in the view should be selected and have focus.
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), QUrl::fromLocalFile(tempSubDir.path()), 0);

    // Select the first file in the view by navigating with the down key.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Down);
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempFile1->fileName()));

    // Select the delegate by pressing enter.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl::fromLocalFile(tempFile1->fileName()));
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), { QUrl::fromLocalFile(tempFile1->fileName()) });
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
    QCOMPARE(dialogHelper.dialog->result(), QQuickFileDialog::Accepted);
}

void tst_QQuickFileDialogImpl::bindCurrentFolder_data()
{
    QTest::addColumn<QUrl>("initialFolder");
    QTest::addColumn<QUrl>("expectedFolder");
    QTest::addColumn<QStringList>("expectedVisibleFiles");

    const auto currentDirUrl = QUrl::fromLocalFile(QDir::current().path());
    const auto tempSubDirUrl = QUrl::fromLocalFile(tempSubDir.path());
    const auto tempSubFile1Url = QUrl::fromLocalFile(tempSubFile1->fileName());

    const QStringList currentDirFiles = tempDirExpectedVisibleFiles(ShowDirectoriesFirst);
    const QStringList tempSubDirFiles = tempSubDirExpectedVisibleFiles(ShowDirectoriesFirst);

    // Setting the folder to "sub-dir" should result in "sub-file1.txt" and "sub-file2.txt" being visible.
    QTest::addRow("sub-dir") << tempSubDirUrl << tempSubDirUrl << tempSubDirFiles;
    // Setting a file as the folder shouldn't work, and the dialog shouldn't change its folder.
    QTest::addRow("sub-dir/sub-file1.txt") << tempSubFile1Url << currentDirUrl << currentDirFiles;
}

void tst_QQuickFileDialogImpl::bindCurrentFolder()
{
    QFETCH(QUrl, initialFolder);
    QFETCH(QUrl, expectedFolder);
    QFETCH(QStringList, expectedVisibleFiles);

    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", initialFolder }});
    OPEN_QUICK_DIALOG();
    COMPARE_URL(dialogHelper.dialog->currentFolder(), expectedFolder);

    QString failureMessage;
    // Even waiting for ListView polish and that the FolderListModel's status is ready aren't enough
    // on Windows, apparently, as sometimes there just aren't any delegates by the time we do the check.
    // So, we use QTRY_VERIFY2 each time we call this function just to be safe.
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView, expectedVisibleFiles, failureMessage), qPrintable(failureMessage));

    // Check that the breadcrumb bar is correct by constructing the expected files from the expectedFolder.
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY2(verifyBreadcrumbDelegates(breadcrumbBar, expectedFolder, failureMessage), qPrintable(failureMessage));
}

void tst_QQuickFileDialogImpl::changeFolderViaStandardButtons()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();

    // Select the delegate by clicking once.
    QQuickFileDialogDelegate *delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, 0, delegate));
    COMPARE_URL(delegate->file(), QUrl::fromLocalFile(tempSubDir.path()));
    QVERIFY(clickButton(delegate));
    // The selectedFile should change, but not currentFolder.
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), QUrl::fromLocalFile(tempSubDir.path()), 0);

    // Click the "Open" button. The dialog should navigate to that directory, but still be open.
    QVERIFY(dialogHelper.quickDialog->footer());
    auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
    QVERIFY(dialogButtonBox);
    QQuickAbstractButton* openButton = findDialogButton(dialogButtonBox, "Open");
    QVERIFY(openButton);
    QVERIFY(clickButton(openButton));
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl::fromLocalFile(tempSubSubDir.path()));
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubDir.path()));
    QVERIFY(dialogHelper.dialog->isVisible());

    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::changeFolderViaDoubleClick_data()
{
    QTest::addColumn<bool>("showDirsFirst");

    QTest::newRow("showDirsFirst=true") << true;
    QTest::newRow("showDirsFirst=false") << false;
}

void tst_QQuickFileDialogImpl::changeFolderViaDoubleClick()
{
    QFETCH(bool, showDirsFirst);

    qputenv("QT_QUICK_DIALOGS_SHOW_DIRS_FIRST", showDirsFirst ? "1" : "0");

    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();

    // Select the "sub-dir" delegate by double-clicking.
    QQuickFileDialogDelegate *subDirDelegate = nullptr;
    const int subDirIndex = showDirsFirst ? 0 : 2;
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, subDirIndex, subDirDelegate));
    COMPARE_URL(subDirDelegate->file(), QUrl::fromLocalFile(tempSubDir.path()));
    QVERIFY(doubleClickButton(subDirDelegate));
    const QStringList expectedVisibleFiles = tempSubDirExpectedVisibleFiles(showDirsFirst ? ShowDirectoriesFirst : ShowFilesFirst);
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView, expectedVisibleFiles, failureMessage), qPrintable(failureMessage));
    // The first file in the directory should now be selected.
    const QUrl firstFileUrl = showDirsFirst ? QUrl::fromLocalFile(tempSubSubDir.path()) : QUrl::fromLocalFile(tempSubFile1->fileName());
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempSubDir.path()), firstFileUrl, 0);
    // Since we only chose a folder, the dialog should still be open.
    QVERIFY(dialogHelper.dialog->isVisible());

    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::chooseFolderViaTextEdit()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();
    // See comment in chooseFileViaTextEdit for why we check for this.
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), QUrl::fromLocalFile(tempSubDir.path()), 0);

    // Get the text edit visible with Ctrl+L.
    const auto editPathKeySequence = QKeySequence(Qt::CTRL | Qt::Key_L);
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(breadcrumbBar->textField()->isVisible());
    QCOMPARE(breadcrumbBar->textField()->text(), dialogHelper.dialog->currentFolder().toLocalFile());
    QCOMPARE(breadcrumbBar->textField()->selectedText(), breadcrumbBar->textField()->text());

    // Enter the path to the folder in the text edit.
    enterText(dialogHelper.window(), tempSubDir.path());
    QCOMPARE(breadcrumbBar->textField()->text(), tempSubDir.path());

    // Hit enter to accept.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView,
        tempSubDirExpectedVisibleFiles(ShowDirectoriesFirst), failureMessage), qPrintable(failureMessage));
    // The first file in the directory should be selected, which is "sub-sub-dir".
    // Note that the TextEdit will still have focus, so we can't use VERIFY_FILE_SELECTED_AND_FOCUSED.
    VERIFY_FILE_SELECTED(QUrl::fromLocalFile(tempSubDir.path()), QUrl::fromLocalFile(tempSubSubDir.path()));
    VERIFY_DELEGATE_CURRENT(QUrl::fromLocalFile(tempSubSubDir.path()), 0)
    QVERIFY(dialogHelper.dialog->isVisible());

    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::chooseFolderViaEnter()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();

    // The first delegate in the view should be selected and have focus.
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), QUrl::fromLocalFile(tempSubDir.path()), 0);

    // Select the delegate by pressing enter.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView,
        tempSubDirExpectedVisibleFiles(ShowDirectoriesFirst), failureMessage), qPrintable(failureMessage));
    // The first file in the new directory should be selected, which is "sub-sub-dir".
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempSubDir.path()), QUrl::fromLocalFile(tempSubSubDir.path()), 0);
    // Since we only chose a folder, the dialog should still be open.
    QVERIFY(dialogHelper.dialog->isVisible());

    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::chooseFileAndThenFolderViaTextEdit()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();
    // See comment in chooseFileViaTextEdit for why we check for this.
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), QUrl::fromLocalFile(tempSubDir.path()), 0);

    // Get the text edit visible with Ctrl+L.
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(breadcrumbBar->textField()->isVisible());
    QCOMPARE(breadcrumbBar->textField()->text(), dialogHelper.dialog->currentFolder().toLocalFile());
    QCOMPARE(breadcrumbBar->textField()->selectedText(), breadcrumbBar->textField()->text());

    // Enter the path to the file in the text edit.
    enterText(dialogHelper.window(), tempFile2->fileName());
    QCOMPARE(breadcrumbBar->textField()->text(), tempFile2->fileName());

    // Hit enter to accept.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    COMPARE_URL(dialogHelper.quickDialog->selectedFile(), QUrl::fromLocalFile(tempFile2->fileName()));
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl::fromLocalFile(tempFile2->fileName()));
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), { QUrl::fromLocalFile(tempFile2->fileName()) });
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
    // Check that the text edit is hidden and breadcrumbs are shown instead.
    QVERIFY(!breadcrumbBar->textField()->isVisible());

    // Re-open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    // The breadcrumbs should be visible after opening, not the text edit.
    QVERIFY(!breadcrumbBar->textField()->isVisible());

    // Get the text edit visible with Ctrl+L.
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    QVERIFY(breadcrumbBar->textField()->isVisible());
    // The text edit should show the directory that contains the last file that was selected.
    QCOMPARE(breadcrumbBar->textField()->text(), tempDir.path());
    QCOMPARE(breadcrumbBar->textField()->selectedText(), breadcrumbBar->textField()->text());

    // Enter the path to the folder in the text edit.
    enterText(dialogHelper.window(), tempSubDir.path());
    QCOMPARE(breadcrumbBar->textField()->text(), tempSubDir.path());

    // Hit enter to accept.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView,
        tempSubDirExpectedVisibleFiles(ShowDirectoriesFirst), failureMessage), qPrintable(failureMessage));
    // The first file in the directory should be selected, which is "sub-sub-dir".
    // Note that the TextEdit will still have focus, so we can't use VERIFY_FILE_SELECTED_AND_FOCUSED.
    VERIFY_FILE_SELECTED(QUrl::fromLocalFile(tempSubDir.path()), QUrl::fromLocalFile(tempSubSubDir.path()));
    VERIFY_DELEGATE_CURRENT(QUrl::fromLocalFile(tempSubSubDir.path()), 0)

    // Close the dialog.
    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::cancelDialogWhileTextEditHasFocus()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();
    // See comment in chooseFileViaTextEdit for why we check for this.
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), QUrl::fromLocalFile(tempSubDir.path()), 0);

    // Get the text edit visible with Ctrl+L.
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(breadcrumbBar->textField()->hasActiveFocus());

    // Close it via the cancel button.
    auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
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

void tst_QQuickFileDialogImpl::closingDialogCancels()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();

    QSignalSpy accepted(dialogHelper.dialog, &QQuickAbstractDialog::accepted);
    QSignalSpy rejected(dialogHelper.dialog, &QQuickAbstractDialog::rejected);

    // Accept the dialog.
    QVERIFY(QMetaObject::invokeMethod(dialogHelper.window(), "doneAccepted"));
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
    QCOMPARE(accepted.size(), 1);
    QCOMPARE(rejected.size(), 0);

    // Re-open the dialog.
    accepted.clear();
    OPEN_QUICK_DIALOG();

    // Close the dialog.
    CLOSE_QUICK_DIALOG();
    QCOMPARE(accepted.size(), 0);
    QCOMPARE(rejected.size(), 1);
}

void tst_QQuickFileDialogImpl::goUp_data()
{
    QTest::addColumn<bool>("showDirsFirst");

    QTest::newRow("showDirsFirst=true") << true;
    QTest::newRow("showDirsFirst=false") << false;
}

void tst_QQuickFileDialogImpl::goUp()
{
    QFETCH(bool, showDirsFirst);

    qputenv("QT_QUICK_DIALOGS_SHOW_DIRS_FIRST", showDirsFirst ? "1" : "0");

    // Open the dialog. Start off in "sub-dir".
    FileDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(tempSubDir.path()) }});
    OPEN_QUICK_DIALOG();

    // Go up a directory via the button next to the breadcrumb bar.
    qCDebug(lcTest) << "going up to" << tempDir.path() << "- files in that dir:\n"
        << QQuickFileDialogImplPrivate::get(dialogHelper.quickDialog)->fileList(QDir(tempDir.path()));
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    auto barListView = qobject_cast<QQuickListView*>(breadcrumbBar->contentItem());
    QVERIFY(barListView);
    if (QQuickTest::qIsPolishScheduled(barListView))
        QVERIFY(QQuickTest::qWaitForPolish(barListView));
    QVERIFY(clickButton(breadcrumbBar->upButton()));
    // The previous directory that we were in should now be selected (matches e.g. Windows and Ubuntu).
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView,
        tempDirExpectedVisibleFiles(showDirsFirst ? ShowDirectoriesFirst : ShowFilesFirst), failureMessage), qPrintable(failureMessage));
    int expectedCurrentIndex = showDirsFirst ? 0 : 2;
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), QUrl::fromLocalFile(tempSubDir.path()), expectedCurrentIndex);

    // Go up a directory via the keyboard shortcut.
    QDir tempParentDir(tempDir.path());
    QVERIFY(tempParentDir.cdUp());
    const auto filesInTempParentDir = QQuickFileDialogImplPrivate::get(dialogHelper.quickDialog)->fileList(tempParentDir);
    qCDebug(lcTest) << "going up to" << tempParentDir.path() << "- files in that dir:\n" << filesInTempParentDir;
    QTest::keySequence(dialogHelper.window(), goUpKeySequence);
    // Ubuntu on QEMU arm shows no files in /tmp even if there are.
    if (!filesInTempParentDir.isEmpty()) {
        expectedCurrentIndex = filesInTempParentDir.indexOf(QFileInfo(tempDir.path()));
        QVERIFY(expectedCurrentIndex != -1);
        VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempParentDir.path()), QUrl::fromLocalFile(tempDir.path()), expectedCurrentIndex);
    }
}

void tst_QQuickFileDialogImpl::goUpWhileTextEditHasFocus()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(tempSubDir.path()) }});
    OPEN_QUICK_DIALOG();
    // See comment in chooseFileViaTextEdit for why we check for this.
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempSubDir.path()), QUrl::fromLocalFile(tempSubSubDir.path()), 0);

    // Get the text edit visible with Ctrl+L.
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(breadcrumbBar->textField()->hasActiveFocus());

    // Go up a directory via the button next to the breadcrumb bar.
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
    QQuickFileDialogDelegate *firstDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, 0, firstDelegate));
    QCOMPARE(dialogHelper.window()->activeFocusItem(), firstDelegate);
}

void tst_QQuickFileDialogImpl::goIntoLargeFolder()
{
    // Open the dialog. Start off in the large directory.
    FileDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(largeTempDir.path()) }});
    OPEN_QUICK_DIALOG();

    // If the screen is so tall that the contentItem is not vertically larger than the view,
    // then the test makes no sense.
    if (QQuickTest::qIsPolishScheduled(dialogHelper.fileDialogListView))
        QVERIFY(QQuickTest::qWaitForPolish(dialogHelper.fileDialogListView));
    // Just to be safe, make sure it's at least twice as big.
    if (dialogHelper.fileDialogListView->contentItem()->height() < dialogHelper.fileDialogListView->height() * 2) {
        QSKIP(qPrintable(QString::fromLatin1("Expected height of dialogHelper.fileDialogListView's contentItem (%1)" \
            " to be at least twice as big as the ListView's height (%2)")
                .arg(dialogHelper.fileDialogListView->contentItem()->height()).arg(dialogHelper.fileDialogListView->height())));
    }

    // Scroll down to largeTempDirLargeSubDirIndex. The view should be somewhere towards the end.
    QVERIFY(QMetaObject::invokeMethod(dialogHelper.fileDialogListView, "positionViewAtIndex", Qt::DirectConnection,
        Q_ARG(int, largeTempDirLargeSubDirIndex), Q_ARG(int, QQuickItemView::PositionMode::Center)));
    QVERIFY(dialogHelper.fileDialogListView->contentY() > 0);

    // Go into it. The view should start at the top of the directory, not at the same contentY
    // that it had in the previous directory.
    QQuickFileDialogDelegate *subDirDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, largeTempDirLargeSubDirIndex, subDirDelegate));
    COMPARE_URL(subDirDelegate->file(), QUrl::fromLocalFile(largeTempDirLargeSubDir.path()));
    QVERIFY(doubleClickButton(subDirDelegate));
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(largeTempDirLargeSubDir.path()));
    COMPARE_URL(dialogHelper.quickDialog->currentFolder(), QUrl::fromLocalFile(largeTempDirLargeSubDir.path()));
    QCOMPARE(dialogHelper.fileDialogListView->contentY(), 0);
}

void tst_QQuickFileDialogImpl::goUpIntoLargeFolder()
{
    // Open the dialog. Start off in the large sub-directory.
    FileDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(largeTempDirLargeSubDir.path()) }});
    OPEN_QUICK_DIALOG();
    // We didn't set an initial selectedFile, so the first file in the directory will be selected.
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(largeTempDirLargeSubDir.path()),
        QUrl::fromLocalFile(largeTempDirLargeSubDir.path() + "/sub-dir000"), 0);

    // Go up a directory via the keyboard shortcut.
    QTest::keySequence(dialogHelper.window(), goUpKeySequence);
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView,
        largeTempDirPaths, failureMessage), qPrintable(failureMessage));
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(largeTempDir.path()),
        QUrl::fromLocalFile(largeTempDirLargeSubDir.path()), largeTempDirLargeSubDirIndex);
}

void tst_QQuickFileDialogImpl::keyAndShortcutHandling()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), QUrl::fromLocalFile(tempSubDir.path()), 0);

    // Get the text edit visible with Ctrl+L.
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

void tst_QQuickFileDialogImpl::bindNameFilters()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "bindTxtHtmlNameFilters.qml");
    OPEN_QUICK_DIALOG();

    // Only "sub-dir", "text1.txt" and "text2.txt" should be visible, since *.txt is the first filter.
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView,
        tempDirExpectedVisibleFiles(ShowDirectoriesFirst), failureMessage), qPrintable(failureMessage));
}

void tst_QQuickFileDialogImpl::changeNameFilters()
{
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());

    // Open the dialog and check that selectedNameFilter is correct.
    // By default, QFileDialogOptions::defaultNameFilterString() is used.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    QCOMPARE(dialogHelper.dialog->selectedNameFilter()->name(), "All Files");
    QCOMPARE(dialogHelper.quickDialog->selectedNameFilter()->name(), "All Files");
    QCOMPARE(dialogHelper.dialog->selectedNameFilter()->index(), 0);
    QCOMPARE(dialogHelper.quickDialog->selectedNameFilter()->index(), 0);
    QCOMPARE(dialogHelper.dialog->selectedNameFilter()->extensions(), { "*" });
    QCOMPARE(dialogHelper.quickDialog->selectedNameFilter()->extensions(), { "*" });
    QCOMPARE(dialogHelper.dialog->selectedNameFilter()->globs(), { "*" });
    QCOMPARE(dialogHelper.quickDialog->selectedNameFilter()->globs(), { "*" });

    // Close the dialog.
    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());

    // Set .txt and .html filters.
    QSignalSpy nameFiltersChangedSpy(dialogHelper.dialog, SIGNAL(nameFiltersChanged()));
    QVERIFY(nameFiltersChangedSpy.isValid());
    const QStringList nameFilters = { "Text files (*.txt)", "HTML files (*.html)" };
    dialogHelper.dialog->setNameFilters(nameFilters);
    QCOMPARE(dialogHelper.dialog->nameFilters(), nameFilters);
    QCOMPARE(nameFiltersChangedSpy.size(), 1);
    QCOMPARE(dialogHelper.dialog->selectedNameFilter()->name(), "Text files");
    QCOMPARE(dialogHelper.dialog->selectedNameFilter()->index(), 0);
    QCOMPARE(dialogHelper.dialog->selectedNameFilter()->extensions(), { "txt" });
    QCOMPARE(dialogHelper.dialog->selectedNameFilter()->globs(), { "*.txt" });

    // Re-open the dialog.
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    // QQuickFileDialogImpl's values only get set before opening.
    QCOMPARE(dialogHelper.quickDialog->selectedNameFilter()->name(), "Text files");
    QCOMPARE(dialogHelper.quickDialog->selectedNameFilter()->index(), 0);
    QCOMPARE(dialogHelper.quickDialog->selectedNameFilter()->extensions(), { "txt" });
    QCOMPARE(dialogHelper.quickDialog->selectedNameFilter()->globs(), { "*.txt" });

    // Only "sub-dir", "text1.txt" and "text2.txt" should be visible, since *.txt is the first filter.
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView,
        tempDirExpectedVisibleFiles(ShowDirectoriesFirst), failureMessage), qPrintable(failureMessage));

    // Open the ComboBox's popup.
    const QQuickComboBox *comboBox = dialogHelper.quickDialog->findChild<QQuickComboBox*>();
    QVERIFY(comboBox);
    const QPoint comboBoxCenterPos = comboBox->mapToScene({ comboBox->width() / 2, comboBox->height() / 2 }).toPoint();
    QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier, comboBoxCenterPos);
    QTRY_VERIFY(comboBox->popup()->isOpened());

    // Select the .html delegate and close the combobox popup. The only visible entry should be the sub-dir.
    QQuickListView *comboBoxPopupListView = qobject_cast<QQuickListView*>(comboBox->popup()->contentItem());
    QVERIFY(comboBoxPopupListView);
    {
        QQuickAbstractButton *htmlDelegate = nullptr;
        QTRY_VERIFY(findViewDelegateItem(comboBoxPopupListView, 1, htmlDelegate));
        QVERIFY(clickButton(htmlDelegate));
    }
    QTRY_VERIFY(!comboBox->popup()->isVisible());
    // Use QTRY_VERIFY2 here to fix a failure on QEMU armv7 (QT_QPA_PLATFORM=offscreen).
    // Not sure why it's necessary.
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView, { tempSubDir.path() }, failureMessage), qPrintable(failureMessage));

    // Open the popup again.
    QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier, comboBoxCenterPos);
    QTRY_VERIFY(comboBox->popup()->isOpened());

    // Select .txt and close the combobox popup. The original entries should be visible.
    {
        QQuickAbstractButton *txtDelegate = nullptr;
        QTRY_VERIFY(findViewDelegateItem(comboBoxPopupListView, 0, txtDelegate));
        QCOMPARE(txtDelegate->text(), nameFilters.at(0));
        QVERIFY(clickButton(txtDelegate));
    }
    QTRY_VERIFY(!comboBox->popup()->isVisible());
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView,
        tempDirExpectedVisibleFiles(ShowDirectoriesFirst), failureMessage), qPrintable(failureMessage));
}

void tst_QQuickFileDialogImpl::changeNameFiltersAfterChangingFolder()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "bindAllTxtHtmlNameFilters.qml");
    OPEN_QUICK_DIALOG();

    // Go into the "sub-dir" folder.
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView,
        tempDirExpectedVisibleFiles(ShowDirectoriesFirst), failureMessage), qPrintable(failureMessage));
    QQuickFileDialogDelegate *subDirDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, 0, subDirDelegate));
    QVERIFY(doubleClickButton(subDirDelegate));
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubDir.path()));
    COMPARE_URL(dialogHelper.quickDialog->currentFolder(), QUrl::fromLocalFile(tempSubDir.path()));

    // Open the ComboBox's popup.
    const QQuickComboBox *comboBox = dialogHelper.quickDialog->findChild<QQuickComboBox*>();
    QVERIFY(comboBox);
    const QPoint comboBoxCenterPos = comboBox->mapToScene({ comboBox->width() / 2, comboBox->height() / 2 }).toPoint();
    QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier, comboBoxCenterPos);
    QTRY_VERIFY(comboBox->popup()->isOpened());

    // Select the .html delegate, close the combobox popup, and ensure that the change had an effect.
    QQuickListView *comboBoxPopupListView = qobject_cast<QQuickListView*>(comboBox->popup()->contentItem());
    QVERIFY(comboBoxPopupListView);
    {
        QQuickAbstractButton *htmlDelegate = nullptr;
        QTRY_VERIFY(findViewDelegateItem(comboBoxPopupListView, 2, htmlDelegate));
        QVERIFY(clickButton(htmlDelegate));
    }
    QTRY_VERIFY(!comboBox->popup()->isVisible());
    // There are no HTML files in "sub-dir", so there should only be the one "sub-sub-dir" delegate.
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView, { tempSubSubDir.path() }, failureMessage), qPrintable(failureMessage));
}

void tst_QQuickFileDialogImpl::tabFocusNavigation()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "bindTxtHtmlNameFilters.qml");
    OPEN_QUICK_DIALOG();

    QList<QQuickItem*> expectedFocusItems;

    // The initial focus should be on the first delegate.
    QQuickFileDialogDelegate *firstDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, 0, firstDelegate));
    expectedFocusItems << firstDelegate;

    // Tab should move to the name filters combobox.
    QQuickComboBox *comboBox = dialogHelper.quickDialog->findChild<QQuickComboBox*>();
    QVERIFY(comboBox);
    expectedFocusItems << comboBox;

    // Next, the left-most dialog button.
    auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
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
    for (int i = 0; i < dialogHelper.fileDialogListView->count(); ++i) {
        QQuickFileDialogDelegate *delegate = nullptr;
        QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, i, delegate));
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

void tst_QQuickFileDialogImpl::acceptRejectLabel()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "acceptRejectLabel.qml");
    OPEN_QUICK_DIALOG();

    // Check that the accept and reject buttons' labels have changed.
    auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
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

void tst_QQuickFileDialogImpl::bindTitle()
{
    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "bindTitle.qml");
    OPEN_QUICK_DIALOG();

    // Open the dialog and check that the correct title is displayed.
    QQuickFileDialog *dialog = dialogHelper.window()->property("dialog").value<QQuickFileDialog*>();
    QVERIFY(dialog);
    const QString expectedTitle = QLatin1String("Test Title");
    QCOMPARE(dialogHelper.dialog->title(), expectedTitle);
    QCOMPARE(dialogHelper.quickDialog->title(), expectedTitle);
    auto header = dialogHelper.quickDialog->header();
    QVERIFY(header);
    auto dialogTitleBarLabel = dialogHelper.quickDialog->header()->findChild<QQuickLabel*>("dialogTitleBarLabel");
    QVERIFY(dialogTitleBarLabel);
    QCOMPARE(dialogTitleBarLabel->text(), expectedTitle);
}

void tst_QQuickFileDialogImpl::itemsDisabledWhenNecessary()
{
    QTemporaryDir anotherTempDir;
    QVERIFY(anotherTempDir.isValid());
    QDir subDir(anotherTempDir.path());
    QVERIFY(subDir.mkdir("emptyDir"));
    QVERIFY(subDir.cd("emptyDir"));

    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(subDir.path()) }});
    OPEN_QUICK_DIALOG();
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(subDir.path()));
    COMPARE_URL(dialogHelper.quickDialog->currentFolder(), QUrl::fromLocalFile(subDir.path()));

    // We opened it in a folder that has no files, so the Open button should be disabled.
    QVERIFY(dialogHelper.quickDialog->footer());
    auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
    QVERIFY(dialogButtonBox);
    QQuickAbstractButton* openButton = findDialogButton(dialogButtonBox, "Open");
    QVERIFY(openButton);
    QCOMPARE(openButton->isEnabled(), false);

    // Now go up. The Open button should now be enabled.
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(clickButton(breadcrumbBar->upButton()));
    QCOMPARE(openButton->isEnabled(), true);
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(anotherTempDir.path()));
    COMPARE_URL(dialogHelper.quickDialog->currentFolder(), QUrl::fromLocalFile(anotherTempDir.path()));

    // Get the text edit visible with Ctrl+L. The Open button should now be disabled.
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    QVERIFY(breadcrumbBar->textField()->isVisible());
    QCOMPARE(openButton->isEnabled(), false);

    // Hide it with the escape key. The Open button should now be enabled.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Escape);
    QVERIFY(!breadcrumbBar->textField()->isVisible());
    QCOMPARE(openButton->isEnabled(), true);
}

void tst_QQuickFileDialogImpl::fileMode_data()
{
    QTest::addColumn<QQuickFileDialog::FileMode>("fileMode");
    QTest::addColumn<QString>("acceptButtonText");

    QTest::newRow("OpenFile") << QQuickFileDialog::OpenFile << "Open";
    QTest::newRow("OpenFiles") << QQuickFileDialog::OpenFiles << "Open";
    QTest::newRow("SaveFile") << QQuickFileDialog::SaveFile << "Save";
}

void tst_QQuickFileDialogImpl::fileMode()
{
    QFETCH(QQuickFileDialog::FileMode, fileMode);
    QFETCH(QString, acceptButtonText);

    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    dialogHelper.dialog->setFileMode(fileMode);
    OPEN_QUICK_DIALOG();

    // Select the first file (not a directory).
    QQuickFileDialogDelegate *tempFile1Delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, 1, tempFile1Delegate));
    COMPARE_URL(tempFile1Delegate->file(), QUrl::fromLocalFile(tempFile1->fileName()));
    QVERIFY(clickButton(tempFile1Delegate));
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempFile1->fileName()));
    COMPARE_URLS(dialogHelper.dialog->currentFiles(), { QUrl::fromLocalFile(tempFile1->fileName()) });

    // All modes should support opening an existing file, so the accept button should be enabled.
    QVERIFY(dialogHelper.quickDialog->footer());
    auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
    QVERIFY(dialogButtonBox);
    QQuickAbstractButton *acceptButton = findDialogButton(dialogButtonBox, acceptButtonText);
    QVERIFY(acceptButton);
    QCOMPARE(acceptButton->isEnabled(), true);

    // Only the OpenFiles mode should allow multiple files to be selected, however.
    QQuickFileDialogDelegate *tempFile2Delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, 2, tempFile2Delegate));
    COMPARE_URL(tempFile2Delegate->file(), QUrl::fromLocalFile(tempFile2->fileName()));
    QTest::keyPress(dialogHelper.window(), Qt::Key_Shift);
    QVERIFY(clickButton(tempFile2Delegate));
    QTest::keyRelease(dialogHelper.window(), Qt::Key_Shift);
    if (fileMode == QQuickFileDialog::OpenFiles) {
        // currentFile() always points to the first file in the list of selected files.
        COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempFile1->fileName()));
        const QList<QUrl> expectedSelectedFiles = {
            QUrl::fromLocalFile(tempFile1->fileName()), QUrl::fromLocalFile(tempFile2->fileName()) };
        COMPARE_URLS(dialogHelper.dialog->currentFiles(), expectedSelectedFiles);
    } else {
        // OpenFile and SaveFile dialogs should have tempFile2 selected since it was clicked,
        // but the shift should do nothing, so tempFile1 should no longer be selected.
        COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempFile2->fileName()));
        COMPARE_URLS(dialogHelper.dialog->currentFiles(), { QUrl::fromLocalFile(tempFile2->fileName()) });
    }

    // Get the text edit visible with Ctrl+L.
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(breadcrumbBar->textField()->isVisible());

    // Typing in the name of an non-existent file should only work for SaveFile.
    const QString nonExistentFilePath = "/foo/bar.txt";
    enterText(dialogHelper.window(), nonExistentFilePath);
    QCOMPARE(breadcrumbBar->textField()->text(), nonExistentFilePath);
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    if (fileMode == QQuickFileDialog::SaveFile) {
        COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl::fromLocalFile(nonExistentFilePath));
        COMPARE_URLS(dialogHelper.dialog->selectedFiles(), { QUrl::fromLocalFile(nonExistentFilePath) });
        QVERIFY(!dialogHelper.dialog->isVisible());
        QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
    } else {
        // For OpenFile(s), we do what Qt Quick Dialogs 1.x did, and restore the previous (valid) dir path.
        // The currentFile(s) should remain unchanged too.
        QVERIFY(dialogHelper.dialog->isVisible());
        COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
        QCOMPARE(breadcrumbBar->textField()->text(), tempDir.path());

        // Should be unchanged from the last time.
        if (fileMode == QQuickFileDialog::OpenFiles) {
            COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempFile1->fileName()));
            const QList<QUrl> expectedSelectedFiles = {
                QUrl::fromLocalFile(tempFile1->fileName()), QUrl::fromLocalFile(tempFile2->fileName()) };
            COMPARE_URLS(dialogHelper.dialog->currentFiles(), expectedSelectedFiles);
        } else { // OpenFile
            COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempFile2->fileName()));
            COMPARE_URLS(dialogHelper.dialog->currentFiles(), { QUrl::fromLocalFile(tempFile2->fileName()) });
        }
    }
}

void tst_QQuickFileDialogImpl::defaultSuffix_data()
{
    QTest::addColumn<QString>("defaultSuffix");

    QTest::newRow("txt") << "txt";
    QTest::newRow(".txt") << ".txt";
}

void tst_QQuickFileDialogImpl::defaultSuffix()
{
    QFETCH(QString, defaultSuffix);

    // Simplify the test by using a directory with no files, and add a single file there.
    QFile tempFile1(tempSubSubDir.path() + "/file1");
    QVERIFY(tempFile1.open(QIODevice::ReadWrite));

    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(tempSubSubDir.path()) }});
    dialogHelper.dialog->setDefaultSuffix(defaultSuffix);
    OPEN_QUICK_DIALOG();
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubSubDir.path()));

    // There should be one extension-less file: "file1".
    QString failureMessage;
    const QStringList expectedVisibleFiles = { tempFile1.fileName() };
    QTRY_VERIFY2(verifyFileDialogDelegates(dialogHelper.fileDialogListView, expectedVisibleFiles, failureMessage), qPrintable(failureMessage));

    // Choose the delegate. The suffix should be added to the delegates.
    QQuickFileDialogDelegate *file1Delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(dialogHelper.fileDialogListView, 0, file1Delegate));
    COMPARE_URL(file1Delegate->file(), QUrl::fromLocalFile(tempFile1.fileName()));
    QVERIFY(doubleClickButton(file1Delegate));
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
    const QUrl fileUrlWithSuffix = QUrl::fromLocalFile(tempFile1.fileName() + ".txt");
    COMPARE_URL(dialogHelper.dialog->selectedFile(), fileUrlWithSuffix);
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), { fileUrlWithSuffix });
}

void tst_QQuickFileDialogImpl::done_data()
{
    QTest::addColumn<QQuickFileDialog::StandardCode>("result");

    QTest::newRow("Accepted") << QQuickFileDialog::Accepted;
    QTest::newRow("Rejected") << QQuickFileDialog::Rejected;
}

void tst_QQuickFileDialogImpl::done()
{
    QFETCH(QQuickFileDialog::StandardCode, result);

    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    OPEN_QUICK_DIALOG();

    switch (result) {
    case QQuickFileDialog::Accepted:
        QVERIFY(QMetaObject::invokeMethod(dialogHelper.window(), "doneAccepted"));
        break;
    case QQuickFileDialog::Rejected:
        QVERIFY(QMetaObject::invokeMethod(dialogHelper.window(), "doneRejected"));
        break;
    }

    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
    QCOMPARE(dialogHelper.dialog->result(), result);
}

void tst_QQuickFileDialogImpl::setSelectedFile_data()
{
    fileMode_data();
}

void tst_QQuickFileDialogImpl::setSelectedFile()
{
    QFETCH(QQuickFileDialog::FileMode, fileMode);

    // Open the dialog.
    const auto tempFile1Url = QUrl::fromLocalFile(tempFile1->fileName());
    const QVariantMap initialProperties = {
        { "tempFile1Url", QVariant::fromValue(tempFile1Url) },
        { "fileMode", QVariant::fromValue(fileMode) }
    };
    FileDialogTestHelper dialogHelper(
        this, "setSelectedFile.qml", {}, initialProperties);
    OPEN_QUICK_DIALOG();

    // The selected file should be what we set.
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), tempFile1Url, 1);

    // Select the next file in the view by navigating with the down key.
    // We know it already has focus, as VERIFY_FILE_SELECTED_AND_FOCUSED checks that.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Down);
    const auto tempFile2Url = QUrl::fromLocalFile(tempFile2->fileName());
    COMPARE_URL(dialogHelper.quickDialog->selectedFile(), tempFile2Url);
    COMPARE_URL(dialogHelper.dialog->selectedFile(), tempFile2Url);

    // Select the delegate by pressing enter.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    COMPARE_URL(dialogHelper.dialog->selectedFile(), tempFile2Url);
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), { tempFile2Url });
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
    QCOMPARE(dialogHelper.dialog->result(), QQuickFileDialog::Accepted);

    // Set a different initial selectedFile and re-open.
    dialogHelper.dialog->setSelectedFile(QUrl::fromLocalFile(tempFile1->fileName()));
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), tempFile1Url, 1);

    // Close it.
    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());

    // Try to set an invalid selectedFile; it should be a no-op for modes other than SaveFile,
    // and the previous selectedFile should still be selected.
    const QString invalidPath = tempDir.path() + "/does-not-exist.txt";
    if (fileMode != QQuickFileDialog::SaveFile) {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QLatin1String(".*QML FileDialog: Cannot set ")
            + invalidPath + QLatin1String(" as a selected file because it doesn't exist")));
    }
    dialogHelper.dialog->setSelectedFile(QUrl::fromLocalFile(invalidPath));
    QVERIFY(dialogHelper.openDialog());
    if (fileMode != QQuickFileDialog::SaveFile) {
        VERIFY_FILE_SELECTED_AND_FOCUSED(QUrl::fromLocalFile(tempDir.path()), tempFile1Url, 1);
    } else {
        QTRY_COMPARE(dialogHelper.fileDialogListView->currentIndex(), -1);
    }
}

void tst_QQuickFileDialogImpl::selectNewFileViaTextField_data()
{
    fileMode_data();
}
void tst_QQuickFileDialogImpl::selectNewFileViaTextField()
{
    QFETCH(QQuickFileDialog::FileMode, fileMode);
    QFETCH(QString, acceptButtonText);

    // Open the dialog.
    FileDialogTestHelper dialogHelper(this, "fileDialog.qml");
    dialogHelper.dialog->setFileMode(fileMode);

    if (fileMode == QQuickFileDialog::SaveFile)
        dialogHelper.dialog->setSelectedFile(QUrl());

    OPEN_QUICK_DIALOG();
    QQuickTest::qWaitForPolish(dialogHelper.window());

    const QQuickTextField *fileNameTextField =
            dialogHelper.quickDialog->findChild<QQuickTextField *>("fileNameTextField");
    QVERIFY(fileNameTextField);

    QVERIFY2(fileNameTextField->isVisible() == (fileMode == QQuickFileDialog::SaveFile),
             "The TextField for file name should only be visible when the FileMode is 'SaveFile'");

    if (fileMode == QQuickFileDialog::SaveFile) {
        QVERIFY(dialogHelper.quickDialog->footer());
        auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
        QVERIFY(dialogButtonBox);
        QQuickAbstractButton *acceptButton = findDialogButton(dialogButtonBox, acceptButtonText);
        QVERIFY(acceptButton);
        QCOMPARE(acceptButton->isEnabled(), false);

        const QPoint textFieldCenterPos =
                fileNameTextField->mapToScene({ fileNameTextField->width() / 2, fileNameTextField->height() / 2 }).toPoint();

        QTest::mouseClick(dialogHelper.window(), Qt::LeftButton, Qt::NoModifier, textFieldCenterPos);
        QTRY_VERIFY(fileNameTextField->hasActiveFocus());
        QCOMPARE(acceptButton->isEnabled(), false);

        const QByteArray newFileName("foo.txt");
        for (const auto &c : newFileName)
            QTest::keyClick(dialogHelper.window(), c);
        QCOMPARE(acceptButton->isEnabled(), true);

        QTest::keyClick(dialogHelper.window(), Qt::Key_Enter);
        QCOMPARE(acceptButton->isEnabled(), true);

        QTRY_COMPARE(fileNameTextField->text(), newFileName);
        QCOMPARE(dialogHelper.dialog->selectedFile().fileName(), newFileName);

        QVERIFY(fileNameTextField->hasActiveFocus());
        for (int i = 0; i < newFileName.size(); i++)
            QTest::keyClick(dialogHelper.window(), Qt::Key_Backspace);
        QCOMPARE(acceptButton->isEnabled(), false);
    }
}

QTEST_MAIN(tst_QQuickFileDialogImpl)

#include "tst_qquickfiledialogimpl.moc"
