// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "dialogstestutils_p.h"

#include <QtTest/qsignalspy.h>
#include <QtQuick/private/qtquickglobal_p.h>
#if QT_CONFIG(quick_listview)
#include <QtQuick/private/qquicklistview_p.h>
#endif
#include <QtQuickTest/quicktest.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p.h>
#if QT_CONFIG(quick_listview) && QT_CONFIG(quick_draganddrop)
#include <QtQuickDialogs2QuickImpl/private/qquickfiledialogdelegate_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickfolderbreadcrumbbar_p.h>
#include <QtQuickDialogs2QuickImpl/private/qquickfolderbreadcrumbbar_p_p.h>
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(quick_listview) && QT_CONFIG(quick_draganddrop)
bool QQuickDialogTestUtils::verifyFileDialogDelegates(QQuickListView *fileDialogListView,
    const QStringList &expectedFiles, QString &failureMessage)
{
    if (QQuickTest::qIsPolishScheduled(fileDialogListView)) {
        if (!QQuickTest::qWaitForPolish(fileDialogListView)) {
            failureMessage = QLatin1String("Failed to polish fileDialogListView");
            return false;
        }
    }

    QStringList actualFiles;
    for (int i = 0; i < fileDialogListView->count(); ++i) {
        auto delegate = qobject_cast<QQuickFileDialogDelegate*>(
            QQuickVisualTestUtils::findViewDelegateItem(fileDialogListView, i));
        if (!delegate) {
            failureMessage = QString::fromLatin1("Delegate at index %1 is null").arg(i);
            return false;
        }

        // Need to call canonicalFilePath on Windows; see comment in dialogstestutils_p.h.
        actualFiles.append(QFileInfo(delegate->file().toLocalFile()).canonicalFilePath());
    }

    if (actualFiles != expectedFiles) {
        QString expectedFilesStr = QDebug::toString(expectedFiles);
        QString actualFilesStr = QDebug::toString(actualFiles);
        failureMessage = QString::fromLatin1("Mismatch in actual vs expected "
            "delegates in fileDialogListView:\n    expected: %1\n      actual: %2");
        if (failureMessage.size() + expectedFilesStr.size() + actualFilesStr.size() > 1024) {
            // If we've exceeded QTest's character limit for failure messages,
            // just show the number of files.
            expectedFilesStr = QString::number(expectedFiles.size());
            actualFilesStr = QString::number(actualFiles.size());
        }
        failureMessage = failureMessage.arg(expectedFilesStr, actualFilesStr);
        return false;
    }

    return true;
}

bool QQuickDialogTestUtils::verifyBreadcrumbDelegates(QQuickFolderBreadcrumbBar *breadcrumbBar,
    const QUrl &expectedFolder, QString &failureMessage)
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
        if (!QQuickTest::qWaitForPolish(breadcrumbBarListView)) {
            failureMessage = QLatin1String("Failed to polish breadcrumbBarListView");
            return false;
        }
    }

    QStringList actualCrumbs;
    for (int i = 0; i < breadcrumbBarListView->count(); ++i) {
        auto delegate = qobject_cast<QQuickAbstractButton*>(
            QQuickVisualTestUtils::findViewDelegateItem(breadcrumbBarListView, i));
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
            .arg(QDebug::toString(expectedCrumbs), QDebug::toString(actualCrumbs));
        return false;
    }

    return true;
}
#endif // QT_CONFIG(quick_listview) && QT_CONFIG(quick_draganddrop)

QQuickAbstractButton *QQuickDialogTestUtils::findDialogButton(QQuickDialogButtonBox *box, const QString &buttonText)
{
    for (int i = 0; i < box->count(); ++i) {
        auto button = qobject_cast<QQuickAbstractButton*>(box->itemAt(i));
        if (button && button->text().toUpper() == buttonText.toUpper())
            return button;
    }
    return nullptr;
}

void QQuickDialogTestUtils::enterText(QWindow *window, const QString &textToEnter)
{
    for (int i = 0; i < textToEnter.size(); ++i) {
        const QChar key = textToEnter.at(i);
        QTest::keyClick(window, key.toLatin1());
    }
}

QT_END_NAMESPACE
