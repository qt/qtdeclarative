/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    void cleanupTestCase();

    void defaults();
    void chooseFileViaStandardButtons();
    void chooseFileViaDoubleClick();
    void chooseFileViaTextEdit();
    void chooseFileViaEnter();
    void bindCurrentFolder_data();
    void bindCurrentFolder();
    void changeFolderViaStandardButtons();
    void changeFolderViaDoubleClick();
    void chooseFolderViaTextEdit();
    void chooseFolderViaEnter();
    void chooseFileAndThenFolderViaTextEdit();
    void cancelDialogWhileTextEditHasFocus();
    void goUp();
    void goUpWhileTextEditHasFocus();
    void goIntoLargeFolder();
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

private:
    QQuickAbstractButton *findDialogButton(QQuickDialogButtonBox *box, const QString &buttonText);
    void enterText(QWindow *window, const QString &textToEnter);

    QTemporaryDir tempDir;
    QScopedPointer<QFile> tempFile1;
    QScopedPointer<QFile> tempFile2;
    QDir tempSubDir;
    QDir tempSubSubDir;
    QScopedPointer<QFile> tempSubFile1;
    QScopedPointer<QFile> tempSubFile2;
    QDir oldCurrentDir;
};

QQuickAbstractButton *tst_QQuickFileDialogImpl::findDialogButton(QQuickDialogButtonBox *box, const QString &buttonText)
{
    for (int i = 0; i < box->count(); ++i) {
        auto button = qobject_cast<QQuickAbstractButton*>(box->itemAt(i));
        if (button && button->text().toUpper() == buttonText.toUpper())
            return button;
    }
    return nullptr;
}

void tst_QQuickFileDialogImpl::enterText(QWindow *window, const QString &textToEnter)
{
    for (int i = 0; i < textToEnter.size(); ++i) {
        const QChar key = textToEnter.at(i);
        QTest::keyClick(window, key.toLatin1());
    }
}

tst_QQuickFileDialogImpl::tst_QQuickFileDialogImpl()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickFileDialogImpl::initTestCase()
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
            ├── sub-dir
            │   ├── sub-sub-dir
            │   ├── sub-file1.txt
            │   └── sub-file2.txt
            ├── file1.txt
            └── file2.txt
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

    // Ensure that each test starts off in the temporary directory.
    oldCurrentDir = QDir::current();
    QDir::setCurrent(tempDir.path());
}

void tst_QQuickFileDialogImpl::cleanupTestCase()
{
    // Just in case...
    QDir::setCurrent(oldCurrentDir.path());
}

bool verifyFileDialogDelegates(QQuickListView *fileDialogListView, const QStringList &expectedFiles,
    QString &failureMessage)
{
    if (QQuickTest::qIsPolishScheduled(fileDialogListView)) {
        if (!QQuickTest::qWaitForItemPolished(fileDialogListView)) {
            failureMessage = QLatin1String("Failed to polish fileDialogListView");
            return false;
        }
    }

    QStringList actualFiles;
    for (int i = 0; i < fileDialogListView->count(); ++i) {
        auto delegate = qobject_cast<QQuickFileDialogDelegate*>(findViewDelegateItem(fileDialogListView, i));
        if (!delegate) {
            failureMessage = QString::fromLatin1("Delegate at index %1 is null").arg(i);
            return false;
        }

        // Need to call absoluteFilePath on Windows; see comment in dialogtestutil.h.
        actualFiles.append(QFileInfo(delegate->file().toLocalFile()).absoluteFilePath());
    }

    if (actualFiles != expectedFiles) {
        failureMessage = QString::fromLatin1("Mismatch in actual vs expected "
            "delegates in fileDialogListView:\n    expected: %1\n      actual: %2")
            .arg(QDebug::toString(expectedFiles)).arg(QDebug::toString(actualFiles));
        return false;
    }

    return true;
}

bool verifyBreadcrumbDelegates(QQuickFolderBreadcrumbBar *breadcrumbBar, const QUrl &expectedFolder,
    QString &failureMessage)
{
    if (!breadcrumbBar) {
        failureMessage = QLatin1String("breadcrumbBar is null");
        return false;
    }

    auto breadcrumbBarListView = qobject_cast<QQuickListView*>(breadcrumbBar->contentItem());
    if (!breadcrumbBarListView) {
        failureMessage = QLatin1String("breadcrumbBar's ListView is null");
        return false;
    }

    if (QQuickTest::qIsPolishScheduled(breadcrumbBarListView)) {
        if (!QQuickTest::qWaitForItemPolished(breadcrumbBarListView)) {
            failureMessage = QLatin1String("Failed to polish breadcrumbBarListView");
            return false;
        }
    }

    QStringList actualCrumbs;
    for (int i = 0; i < breadcrumbBarListView->count(); ++i) {
        auto delegate = qobject_cast<QQuickAbstractButton*>(findViewDelegateItem(breadcrumbBarListView, i));
        if (!delegate) {
            // It's a separator or some other non-crumb item.
            continue;
        }

        actualCrumbs.append(delegate->text());
    }

    QStringList expectedCrumbs = QQuickFolderBreadcrumbBarPrivate::crumbPathsForFolder(expectedFolder);
    for (int i = 0; i < expectedCrumbs.size(); ++i) {
        QString &crumbPath = expectedCrumbs[i];
        crumbPath = QQuickFolderBreadcrumbBarPrivate::folderBaseName(crumbPath);
    }

    if (actualCrumbs != expectedCrumbs) {
        failureMessage = QString::fromLatin1("Mismatch in actual vs expected "
            "delegates in breadcrumbBarListView:\n    expected: %1\n      actual: %2")
            .arg(QDebug::toString(expectedCrumbs)).arg(QDebug::toString(actualCrumbs));
        return false;
    }

    return true;
}

void tst_QQuickFileDialogImpl::defaults()
{
    QQuickApplicationHelper helper(this, "fileDialog.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickFileDialog *dialog = window->property("dialog").value<QQuickFileDialog*>();
    QVERIFY(dialog);
    COMPARE_URL(dialog->selectedFile(), QUrl());
    // It should default to the current directory.
    COMPARE_URL(dialog->currentFolder(), QUrl::fromLocalFile(QDir().absolutePath()));
    // The first file in the directory should be selected, but not until the dialog is actually open,
    // as QQuickFileDialogImpl hasn't been created yet.
    COMPARE_URL(dialog->currentFile(), QUrl());
    COMPARE_URLS(dialog->currentFiles(), {});
    QCOMPARE(dialog->title(), QString());

    dialog->open();
    QQuickFileDialogImpl *quickDialog = window->findChild<QQuickFileDialogImpl*>();
    QTRY_VERIFY(quickDialog->isOpened());
    QVERIFY(quickDialog);
    COMPARE_URL(quickDialog->selectedFile(), QUrl());
    COMPARE_URL(quickDialog->currentFolder(), QUrl::fromLocalFile(QDir().absolutePath()));
    COMPARE_URL(dialog->currentFile(), QUrl::fromLocalFile(tempSubDir.path()));
    COMPARE_URLS(dialog->currentFiles(), { QUrl::fromLocalFile(tempSubDir.path()) });
    COMPARE_URL(quickDialog->currentFile(), QUrl::fromLocalFile(tempSubDir.path()));
    QCOMPARE(quickDialog->title(), QString());
}

void tst_QQuickFileDialogImpl::chooseFileViaStandardButtons()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Select the delegate by clicking once.
    QSignalSpy dialogFileChangedSpy(dialogHelper.dialog, SIGNAL(selectedFileChanged()));
    QVERIFY(dialogFileChangedSpy.isValid());
    QSignalSpy dialogCurrentFileChangedSpy(dialogHelper.dialog, SIGNAL(currentFileChanged()));
    QVERIFY(dialogCurrentFileChangedSpy.isValid());
    QSignalSpy quickDialogCurrentFileChangedSpy(dialogHelper.quickDialog, SIGNAL(currentFileChanged(const QUrl &)));
    QVERIFY(quickDialogCurrentFileChangedSpy.isValid());

    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 2, delegate));
    COMPARE_URL(delegate->file(), QUrl::fromLocalFile(tempFile2->fileName()));
    QVERIFY(clickButton(delegate));
    COMPARE_URL(dialogHelper.quickDialog->currentFile(), QUrl::fromLocalFile(tempFile2->fileName()));
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempFile2->fileName()));
    COMPARE_URLS(dialogHelper.dialog->currentFiles(), { QUrl::fromLocalFile(tempFile2->fileName()) });
    // Only currentFile-related signals should be emitted.
    QCOMPARE(dialogFileChangedSpy.count(), 0);
    QCOMPARE(dialogCurrentFileChangedSpy.count(), 1);
    QCOMPARE(quickDialogCurrentFileChangedSpy.count(), 1);

    // Click the "Open" button.
    QVERIFY(dialogHelper.quickDialog->footer());
    auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
    QVERIFY(dialogButtonBox);
    QQuickAbstractButton* openButton = findDialogButton(dialogButtonBox, "Open");
    QVERIFY(openButton);
    QVERIFY(clickButton(openButton));
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl::fromLocalFile(tempFile2->fileName()));
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), { QUrl::fromLocalFile(tempFile2->fileName()) });
    QCOMPARE(dialogFileChangedSpy.count(), 1);
    COMPARE_URL(dialogHelper.quickDialog->selectedFile(), QUrl::fromLocalFile(tempFile2->fileName()));
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
    QVERIFY(!dialogHelper.dialog->isVisible());
}

void tst_QQuickFileDialogImpl::chooseFileViaDoubleClick()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Select the delegate by double-clicking.
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 2, delegate));
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
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
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
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Before moving down, the first delegate in the view should be selected and have focus.
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempSubDir.path()));
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *subDirDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, subDirDelegate));
    COMPARE_URL(subDirDelegate->file(), QUrl::fromLocalFile(tempSubDir.path()));
    QVERIFY(subDirDelegate->hasActiveFocus());

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

    const QStringList currentDirFiles = { tempSubDir.path(), tempFile1->fileName(), tempFile2->fileName() };
    const QStringList tempSubDirFiles = { tempSubSubDir.path(), tempSubFile1->fileName(), tempSubFile2->fileName() };

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
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", initialFolder }});
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    COMPARE_URL(dialogHelper.dialog->currentFolder(), expectedFolder);

    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QString failureMessage;
    // Even waiting for ListView polish and that the FolderListModel's status is ready aren't enough
    // on Windows, apparently, as sometimes there just aren't any delegates by the time we do the check.
    // So, we use QTRY_VERIFY2 each time we call this function just to be safe.
    QTRY_VERIFY2(verifyFileDialogDelegates(fileDialogListView, expectedVisibleFiles, failureMessage), qPrintable(failureMessage));

    // Check that the breadcrumb bar is correct by constructing the expected files from the expectedFolder.
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY2(verifyBreadcrumbDelegates(breadcrumbBar, expectedFolder, failureMessage), qPrintable(failureMessage));
}

void tst_QQuickFileDialogImpl::changeFolderViaStandardButtons()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Select the delegate by clicking once.
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, delegate));
    COMPARE_URL(delegate->file(), QUrl::fromLocalFile(tempSubDir.path()));
    QVERIFY(clickButton(delegate));
    // The selectedFile and currentFolder shouldn't change yet.
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl());
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), {});
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    // The currentFile should, though.
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempSubDir.path()));
    COMPARE_URLS(dialogHelper.dialog->currentFiles(), { QUrl::fromLocalFile(tempSubDir.path()) });

    // Click the "Open" button. The dialog should navigate to that directory, but still be open.
    QVERIFY(dialogHelper.quickDialog->footer());
    auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
    QVERIFY(dialogButtonBox);
    QQuickAbstractButton* openButton = findDialogButton(dialogButtonBox, "Open");
    QVERIFY(openButton);
    QVERIFY(clickButton(openButton));
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl());
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubDir.path()));
    QVERIFY(dialogHelper.dialog->isVisible());

    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::changeFolderViaDoubleClick()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Select the "sub-dir" delegate by double-clicking.
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *subDirDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, subDirDelegate));
    COMPARE_URL(subDirDelegate->file(), QUrl::fromLocalFile(tempSubDir.path()));
    QVERIFY(doubleClickButton(subDirDelegate));
    // The first file in the directory should be selected, which is "sub-sub-dir".
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempSubSubDir.path()));
    COMPARE_URLS(dialogHelper.dialog->currentFiles(), { QUrl::fromLocalFile(tempSubSubDir.path()) });
    QQuickFileDialogDelegate *subSubDirDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, subSubDirDelegate));
    QCOMPARE(subSubDirDelegate->isHighlighted(), true);
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl());
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), {});
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubDir.path()));
    // Since we only chose a folder, the dialog should still be open.
    QVERIFY(dialogHelper.dialog->isVisible());

    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::chooseFolderViaTextEdit()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
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
    enterText(dialogHelper.window(), tempSubDir.path());
    QCOMPARE(breadcrumbBar->textField()->text(), tempSubDir.path());

    // Hit enter to accept.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    // The first file in the directory should be selected, which is "sub-sub-dir".
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempSubSubDir.path()));
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *subSubDirDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, subSubDirDelegate));
    QCOMPARE(subSubDirDelegate->isHighlighted(), true);
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl());
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), {});
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubDir.path()));
    QTRY_VERIFY(dialogHelper.dialog->isVisible());

    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::chooseFolderViaEnter()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Fhe first delegate in the view should be selected and have focus.
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempSubDir.path()));
    COMPARE_URLS(dialogHelper.dialog->currentFiles(), { QUrl::fromLocalFile(tempSubDir.path()) });
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *subDirDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, subDirDelegate));
    COMPARE_URL(subDirDelegate->file(), QUrl::fromLocalFile(tempSubDir.path()));
    QVERIFY(subDirDelegate->hasActiveFocus());

    // Select the delegate by pressing enter.
    QTest::keyClick(dialogHelper.window(), Qt::Key_Return);
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubDir.path()));
    // The first file in the new directory should be selected, which is "sub-sub-dir".
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempSubSubDir.path()));
    // Since we only chose a folder, the dialog should still be open.
    QVERIFY(dialogHelper.dialog->isVisible());

    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::chooseFileAndThenFolderViaTextEdit()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
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
    // The first file in the directory should be selected, which is "sub-sub-dir".
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempSubSubDir.path()));
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *subSubDirDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, subSubDirDelegate));
    QCOMPARE(subSubDirDelegate->isHighlighted(), true);
    // We just changed directories, so the actual selected file shouldn't change.
    COMPARE_URL(dialogHelper.dialog->selectedFile(), QUrl::fromLocalFile(tempFile2->fileName()));
    COMPARE_URLS(dialogHelper.dialog->selectedFiles(), { QUrl::fromLocalFile(tempFile2->fileName()) });
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubDir.path()));
    QTRY_VERIFY(dialogHelper.dialog->isVisible());

    // Close the dialog.
    dialogHelper.dialog->close();
    QVERIFY(!dialogHelper.dialog->isVisible());
    QTRY_VERIFY(!dialogHelper.quickDialog->isVisible());
}

void tst_QQuickFileDialogImpl::cancelDialogWhileTextEditHasFocus()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Get the text edit visible with Ctrl+L.
    const auto editPathKeySequence = QKeySequence(Qt::CTRL | Qt::Key_L);
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

void tst_QQuickFileDialogImpl::goUp()
{
    // Open the dialog. Start off in "sub-dir".
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(tempSubDir.path()) }});
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
        QVERIFY(QQuickTest::qWaitForItemPolished(barListView));
    QVERIFY(clickButton(breadcrumbBar->upButton()));
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    // The previous directory that we were in should now be selected (matches e.g. Windows and Ubuntu).
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempSubDir.path()));
    COMPARE_URLS(dialogHelper.dialog->currentFiles(), { QUrl::fromLocalFile(tempSubDir.path()) });
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *subDirDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, subDirDelegate));
    QCOMPARE(subDirDelegate->isHighlighted(), true);

    // Go up a directory via the keyboard shortcut next to the breadcrumb bar.
    const auto goUpKeySequence = QKeySequence(Qt::ALT | Qt::Key_Up);
    QTest::keySequence(dialogHelper.window(), goUpKeySequence);
    QDir tempParentDir(tempDir.path());
    QVERIFY(tempParentDir.cdUp());
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempParentDir.path()));
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempDir.path()));
    COMPARE_URLS(dialogHelper.dialog->currentFiles(), { QUrl::fromLocalFile(tempDir.path()) });
}

void tst_QQuickFileDialogImpl::goUpWhileTextEditHasFocus()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(tempSubDir.path()) }});
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Get the text edit visible with Ctrl+L.
    const auto editPathKeySequence = QKeySequence(Qt::CTRL | Qt::Key_L);
    QTest::keySequence(dialogHelper.window(), editPathKeySequence);
    auto breadcrumbBar = dialogHelper.quickDialog->findChild<QQuickFolderBreadcrumbBar*>();
    QVERIFY(breadcrumbBar);
    QVERIFY(breadcrumbBar->textField()->hasActiveFocus());

    // Go up a directory via the button next to the breadcrumb bar.
    auto barListView = qobject_cast<QQuickListView*>(breadcrumbBar->contentItem());
    QVERIFY(barListView);
    if (QQuickTest::qIsPolishScheduled(barListView))
        QVERIFY(QQuickTest::qWaitForItemPolished(barListView));
    QVERIFY(clickButton(breadcrumbBar->upButton()));
    // The path should have changed to the parent directory.
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempDir.path()));
    // The text edit should be hidden when it loses focus.
    QVERIFY(!breadcrumbBar->textField()->hasActiveFocus());
    QVERIFY(!breadcrumbBar->textField()->isVisible());
    // The focus should be given to the first delegate.
    QVERIFY(dialogHelper.window()->activeFocusItem());
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *firstDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, firstDelegate));
    QCOMPARE(dialogHelper.window()->activeFocusItem(), firstDelegate);
}

void tst_QQuickFileDialogImpl::goIntoLargeFolder()
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
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(anotherTempDir.path()) }});
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // If the screen is so tall that the contentItem is not vertically larger than the view,
    // then the test makes no sense.
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    if (QQuickTest::qIsPolishScheduled(fileDialogListView))
        QVERIFY(QQuickTest::qWaitForItemPolished(fileDialogListView));
    // Just to be safe, make sure it's at least twice as big.
    if (fileDialogListView->contentItem()->height() < fileDialogListView->height() * 2) {
        QSKIP(qPrintable(QString::fromLatin1("Expected height of fileDialogListView's contentItem (%1)" \
            " to be at least twice as big as the ListView's height (%2)")
                .arg(fileDialogListView->contentItem()->height()).arg(fileDialogListView->height())));
    }

    // Scroll down to dir20. The view should be somewhere past the middle.
    QVERIFY(QMetaObject::invokeMethod(fileDialogListView, "positionViewAtIndex", Qt::DirectConnection,
        Q_ARG(int, 20), Q_ARG(int, QQuickItemView::PositionMode::Center)));
    QVERIFY(fileDialogListView->contentY() > 0);

    // Go into it. The view should start at the top of the directory, not at the same contentY
    // that it had in the previous directory.
    QQuickFileDialogDelegate *dir20Delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 20, dir20Delegate));
    COMPARE_URL(dir20Delegate->file(), QUrl::fromLocalFile(dir20.path()));
    QVERIFY(doubleClickButton(dir20Delegate));
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(dir20.path()));
    COMPARE_URL(dialogHelper.quickDialog->currentFolder(), QUrl::fromLocalFile(dir20.path()));
    QCOMPARE(fileDialogListView->contentY(), 0);
}

void tst_QQuickFileDialogImpl::keyAndShortcutHandling()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
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

void tst_QQuickFileDialogImpl::bindNameFilters()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "bindTxtHtmlNameFilters.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Only "sub-dir", "text1.txt" and "text2.txt" should be visible, since *.txt is the first filter.
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(fileDialogListView,
        { tempSubDir.path(), tempFile1->fileName(), tempFile2->fileName() }, failureMessage), qPrintable(failureMessage));
}

void tst_QQuickFileDialogImpl::changeNameFilters()
{
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
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
    QCOMPARE(nameFiltersChangedSpy.count(), 1);
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
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(fileDialogListView,
        { tempSubDir.path(), tempFile1->fileName(), tempFile2->fileName() }, failureMessage), qPrintable(failureMessage));

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
    QTRY_VERIFY2(verifyFileDialogDelegates(fileDialogListView, { tempSubDir.path() }, failureMessage), qPrintable(failureMessage));

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
    QTRY_VERIFY2(verifyFileDialogDelegates(fileDialogListView,
        { tempSubDir.path(), tempFile1->fileName(), tempFile2->fileName() }, failureMessage), qPrintable(failureMessage));
}

void tst_QQuickFileDialogImpl::changeNameFiltersAfterChangingFolder()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "bindAllTxtHtmlNameFilters.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Go into the "sub-dir" folder.
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QString failureMessage;
    QTRY_VERIFY2(verifyFileDialogDelegates(fileDialogListView,
        { tempSubDir.path(), tempFile1->fileName(), tempFile2->fileName() }, failureMessage), qPrintable(failureMessage));
    QQuickFileDialogDelegate *subDirDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, subDirDelegate));
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
    QTRY_VERIFY2(verifyFileDialogDelegates(fileDialogListView, { tempSubSubDir.path() }, failureMessage), qPrintable(failureMessage));
}

void tst_QQuickFileDialogImpl::tabFocusNavigation()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "bindTxtHtmlNameFilters.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    QList<QQuickItem*> expectedFocusItems;

    // The initial focus should be on the first delegate.
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *firstDelegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, firstDelegate));
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
    for (int i = 0; i < fileDialogListView->count(); ++i) {
        QQuickFileDialogDelegate *delegate = nullptr;
        QTRY_VERIFY(findViewDelegateItem(fileDialogListView, i, delegate));
        expectedFocusItems << delegate;
    }

    // Tab through each item, checking the focus after each.
    for (auto expectedFocusItem : qAsConst(expectedFocusItems)) {
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
    for (auto expectedFocusItem : qAsConst(expectedFocusItems)) {
        QTest::keyClick(dialogHelper.window(), Qt::Key_Tab, Qt::ShiftModifier);

        QCOMPARE(dialogHelper.window()->activeFocusItem(), expectedFocusItem);
    }
}

void tst_QQuickFileDialogImpl::acceptRejectLabel()
{
    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "acceptRejectLabel.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

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
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "bindTitle.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

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
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(subDir.path()) }});
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
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
    const auto editPathKeySequence = QKeySequence(Qt::CTRL | Qt::Key_L);
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

    QTest::newRow("OpenFile") << QQuickFileDialog::OpenFile;
    QTest::newRow("OpenFiles") << QQuickFileDialog::OpenFiles;
    QTest::newRow("SaveFile") << QQuickFileDialog::SaveFile;
}

void tst_QQuickFileDialogImpl::fileMode()
{
    QFETCH(QQuickFileDialog::FileMode, fileMode);

    // Open the dialog.
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
    dialogHelper.dialog->setFileMode(fileMode);
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

    // Select the first file (not a directory).
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QQuickFileDialogDelegate *tempFile1Delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 1, tempFile1Delegate));
    COMPARE_URL(tempFile1Delegate->file(), QUrl::fromLocalFile(tempFile1->fileName()));
    QVERIFY(clickButton(tempFile1Delegate));
    COMPARE_URL(dialogHelper.dialog->currentFile(), QUrl::fromLocalFile(tempFile1->fileName()));
    COMPARE_URLS(dialogHelper.dialog->currentFiles(), { QUrl::fromLocalFile(tempFile1->fileName()) });

    // All modes should support opening an existing file, so the Open button should be enabled.
    QVERIFY(dialogHelper.quickDialog->footer());
    auto dialogButtonBox = dialogHelper.quickDialog->footer()->findChild<QQuickDialogButtonBox*>();
    QVERIFY(dialogButtonBox);
    QQuickAbstractButton* openButton = findDialogButton(dialogButtonBox, "Open");
    QVERIFY(openButton);
    QCOMPARE(openButton->isEnabled(), true);

    // Only the OpenFiles mode should allow multiple files to be selected, however.
    QQuickFileDialogDelegate *tempFile2Delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 2, tempFile2Delegate));
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
    const auto editPathKeySequence = QKeySequence(Qt::CTRL | Qt::Key_L);
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
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "bindCurrentFolder.qml", {},
        {{ "initialFolder", QUrl::fromLocalFile(tempSubSubDir.path()) }});
    dialogHelper.dialog->setDefaultSuffix(defaultSuffix);
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());
    COMPARE_URL(dialogHelper.dialog->currentFolder(), QUrl::fromLocalFile(tempSubSubDir.path()));

    // There should be one extension-less file: "file1".
    auto fileDialogListView = dialogHelper.quickDialog->findChild<QQuickListView*>("fileDialogListView");
    QVERIFY(fileDialogListView);
    QString failureMessage;
    const QStringList expectedVisibleFiles = { tempFile1.fileName() };
    QTRY_VERIFY2(verifyFileDialogDelegates(fileDialogListView, expectedVisibleFiles, failureMessage), qPrintable(failureMessage));

    // Choose the delegate. The suffix should be added to the delegates.
    QQuickFileDialogDelegate *file1Delegate = nullptr;
    QTRY_VERIFY(findViewDelegateItem(fileDialogListView, 0, file1Delegate));
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
    DialogTestHelper<QQuickFileDialog, QQuickFileDialogImpl> dialogHelper(this, "fileDialog.qml");
    QVERIFY2(dialogHelper.isWindowInitialized(), dialogHelper.failureMessage());
    QVERIFY(dialogHelper.waitForWindowActive());
    QVERIFY(dialogHelper.openDialog());
    QTRY_VERIFY(dialogHelper.isQuickDialogOpen());

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

QTEST_MAIN(tst_QQuickFileDialogImpl)

#include "tst_qquickfiledialogimpl.moc"
