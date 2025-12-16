// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLLS_QQMLCODEMODEL_H
#define TST_QMLLS_QQMLCODEMODEL_H

#include <QtJsonRpc/private/qjsonrpcprotocol_p.h>
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qlibraryinfo.h>

#include <QtTest/qtest.h>

#include <iostream>

using namespace Qt::StringLiterals;

class tst_qmlls_qqmlcodemodel : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qmlls_qqmlcodemodel();
    QString readFile(const QString &filename) const;

private slots:
    void buildPathsForFileUrl_data();
    void buildPathsForFileUrl();
    void fileNamesToWatch();
    void resourceFiles();
    void resourceFilesFallback();
    void findFilePathsFromFileNames_data();
    void findFilePathsFromFileNames();
    void openFiles_data();
    void openFiles();
    void importPathViaSettings();
    void reloadLotsOfFiles();

    // Tests for the qqmlcodemodelmanager:
    void buildPaths_data();
    void buildPaths();
    void defaultWorkspace();
    void closeWorkspace();
    void rootUrls();
    void addingWorkspaces();
    void newWorkspace();
    void duplicateWorkspace();
    void withQmllsBuildIni();
    void withQmllsBuildIniWithoutRootUrls();
    void withQmllsBuildIniRelativeImportPath();
    void withQmllsIniRelativeImportPath();
    void shortestRootUrlForFile();
    void qprocessScheduler_data();
    void qprocessScheduler();
    void qprocessSchedulerProcess();
    void multipleQProcessScheduler_data();
    void multipleQProcessScheduler();
    void reloadQmllsBuildIniAfterBuild();
};

#endif // TST_QMLLS_QQMLCODEMODEL_H
