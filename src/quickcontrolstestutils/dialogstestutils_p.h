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

#ifndef DIALOGSTESTUTILS_H
#define DIALOGSTESTUTILS_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

// We need these for Windows, because FolderListModel returns a lowercase drive letter; e.g.:
// "file:///c:/blah.txt", whereas other API returns "file:///C:/blah.txt".
#define COMPARE_URL(url1, url2) \
    QCOMPARE(QFileInfo(url1.toLocalFile()).absoluteFilePath(), QFileInfo(url2.toLocalFile()).absoluteFilePath());

// Store a copy of the arguments in case { ... } list initializer syntax is used as an argument,
// which could result in two different lists being created and passed to std::transform()
// (and would also require it to be enclosed in parentheses everywhere it's used).
#define COMPARE_URLS(actualUrls, expectedUrls) \
{ \
    const QList<QUrl> actualUrlsCopy = actualUrls; \
    QList<QString> actualPaths; \
    std::transform(actualUrlsCopy.begin(), actualUrlsCopy.end(), std::back_insert_iterator(actualPaths), \
        [](const QUrl &url) { return QFileInfo(url.toLocalFile()).absoluteFilePath(); }); \
    const QList<QUrl> expectedUrlsCopy = expectedUrls; \
    QList<QString> expectedPaths; \
    std::transform(expectedUrlsCopy.begin(), expectedUrlsCopy.end(), std::back_insert_iterator(expectedPaths), \
        [](const QUrl &url) { return QFileInfo(url.toLocalFile()).absoluteFilePath(); }); \
    QCOMPARE(actualPaths, expectedPaths); \
}

namespace QQuickDialogTestUtils
{

// Saves duplicating a bunch of code in every test.
template<typename DialogType, typename QuickDialogType>
class DialogTestHelper
{
public:
    DialogTestHelper(QQmlDataTest *testCase, const QString &testFilePath,
            const QStringList &qmlImportPaths = {}, const QVariantMap &initialProperties = {}) :
        appHelper(testCase, testFilePath, qmlImportPaths, initialProperties)
    {
        if (!appHelper.ready)
            return;

        dialog = appHelper.window->property("dialog").value<DialogType*>();
        if (!dialog) {
            appHelper.errorMessage = "\"dialog\" property is not valid";
            return;
        }

        appHelper.window->show();
        appHelper.window->requestActivate();
    }

    Q_REQUIRED_RESULT bool isWindowInitialized() const
    {
        return appHelper.ready;
    }

    Q_REQUIRED_RESULT bool waitForWindowActive()
    {
        return QTest::qWaitForWindowActive(appHelper.window);
    }

    bool openDialog()
    {
        dialog->open();
        if (!dialog->isVisible()) {
            appHelper.errorMessage = "Dialog is not visible";
            return false;
        }

        // We might want to call this function more than once,
        // and we only need to get these members the first time.
        if (!quickDialog) {
            quickDialog = appHelper.window->findChild<QuickDialogType*>();
            if (!quickDialog) {
                appHelper.errorMessage = "Can't find Qt Quick-based dialog";
                return false;
            }
        }

        return true;
    }

    bool isQuickDialogOpen() const
    {
        return quickDialog->isOpened();
    }

    QQuickWindow *window() const
    {
        return appHelper.window;
    }

    const char *failureMessage() const
    {
        return appHelper.errorMessage.constData();
    }

    QQuickVisualTestUtils::QQuickApplicationHelper appHelper;
    DialogType *dialog = nullptr;
    QuickDialogType *quickDialog = nullptr;
};

}

#endif // DIALOGSTESTUTILS_H
