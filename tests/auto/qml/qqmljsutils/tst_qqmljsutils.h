// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QQMLJSUTILS_P_H
#define TST_QQMLJSUTILS_P_H

#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtTest/QtTest>

class tst_qqmljsutils: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmljsutils(): QQmlDataTest(QT_QQMLJSUTILS_DATADIR) { };

private slots:
    void findResourceFilesFromBuildFolders();

    void qmlFileSourcePathFromBuildPath_data();
    void qmlFileSourcePathFromBuildPath();
};

#endif // TST_QQMLJSUTILS_P_H
