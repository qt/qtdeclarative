// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qmlls_qqmlcodemodel.h"

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtQmlLS/private/qqmlcodemodel_p.h>

void tst_qmlls_qqmlcodemodel::buildPathsForFileUrl_data()
{
    QTest::addColumn<QString>("pathFromIniFile");
    QTest::addColumn<QString>("pathFromEnvironmentVariable");
    QTest::addColumn<QString>("pathFromCommandLine");
    QTest::addColumn<QString>("expectedPath");

    const QString path1 = u"/Users/helloWorld/build-myProject"_s;
    const QString path2 = u"/Users/helloWorld/build-custom"_s;
    const QString path3 = u"/Users/helloWorld/build-12345678"_s;

    QTest::addRow("justCommandLine") << QString() << QString() << path1 << path1;
    QTest::addRow("all3") << path1 << path2 << path3 << path3;

    QTest::addRow("commandLineOverridesEnvironmentVariable")
            << QString() << path2 << path3 << path3;
    QTest::addRow("commandLineOverridesIniFile") << path2 << QString() << path3 << path3;

    QTest::addRow("EnvironmentVariableOverridesIniFile") << path1 << path2 << QString() << path2;
    QTest::addRow("iniFile") << path1 << QString() << QString() << path1;
    QTest::addRow("environmentVariable") << QString() << path3 << QString() << path3;
}

void tst_qmlls_qqmlcodemodel::buildPathsForFileUrl()
{
    QFETCH(QString, pathFromIniFile);
    QFETCH(QString, pathFromEnvironmentVariable);
    QFETCH(QString, pathFromCommandLine);
    QFETCH(QString, expectedPath);

    QQmlToolingSettings settings(u"qmlls"_s);
    if (!pathFromIniFile.isEmpty())
        settings.addOption("buildDir", pathFromIniFile);

    constexpr char environmentVariable[] = "QMLLS_BUILD_DIRS";
    qunsetenv(environmentVariable);
    if (!pathFromEnvironmentVariable.isEmpty()) {
        qputenv(environmentVariable, pathFromEnvironmentVariable.toUtf8());
    }

    QmlLsp::QQmlCodeModel model(nullptr, &settings);
    if (!pathFromCommandLine.isEmpty())
        model.setBuildPathsForRootUrl(QByteArray(), QStringList{ pathFromCommandLine });

    // use nonexistent path to avoid loading random .qmlls.ini files that might be laying around.
    // in this case, it should abort the search and the standard value we set in the settings
    const QByteArray nonExistentUrl =
            QUrl::fromLocalFile(u"./___thispathdoesnotexist123___/abcdefghijklmnop"_s).toEncoded();

    QStringList result = model.buildPathsForFileUrl(nonExistentUrl);
    QCOMPARE(result.size(), 1);
    QCOMPARE(result.front(), expectedPath);
}

QTEST_MAIN(tst_qmlls_qqmlcodemodel)
