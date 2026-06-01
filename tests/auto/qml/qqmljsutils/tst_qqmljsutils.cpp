// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qqmljsutils.h"
#include <QtQmlCompiler/private/qqmljsutils_p.h>

using namespace Qt::StringLiterals;

void tst_qqmljsutils::findResourceFilesFromBuildFolders()
{
    const QString buildFolder = testFile("buildfolder");
    const QStringList resourceFiles = QQmlJSUtils::resourceFilesFromBuildFolders({ buildFolder});
    QCOMPARE(resourceFiles,
             QStringList{ testFile(u"buildfolder/.qt/rcc/appuntitled72_raw_qml_0.qrc"_s) });
};

void tst_qqmljsutils::qmlFileSourcePathFromBuildPath_data()
{
    QTest::addColumn<QString>("buildFolder");
    QTest::addColumn<QString>("buildFile");
    QTest::addColumn<QString>("expectedSourceFile");

    QTest::addRow("Main.qml") << testFile(u"mymodule-build"_s)
                              << testFile(u"mymodule-build/MyModule/Main.qml"_s)
                              << testFile(u"mymodule-source/MyModule/Main.qml"_s);
    QTest::addRow("NestedModuleWithPrefers")
            << testFile(u"mymodule-build"_s)
            << testFile(u"mymodule-build/MyModule/X/Y/Z/MyComponent.qml"_s)
            << testFile(u"mymodule-source/MyModule/X/Y/Z/MyComponent.qml"_s);
    QTest::addRow("NestedModuleWithoutPrefers")
            << testFile(u"mymodule-build-without-qmldir-prefer"_s)
            << testFile(u"mymodule-build-without-qmldir-prefer/MyModule/X/Y/Z/MyComponent.qml"_s)
            << testFile(u"mymodule-source/MyModule/X/Y/Z/MyComponent.qml"_s);
}

void tst_qqmljsutils::qmlFileSourcePathFromBuildPath()
{
    QFETCH(QString, buildFolder);
    QFETCH(QString, buildFile);
    QFETCH(QString, expectedSourceFile);

    QQmlJSResourceFileMapper mapper(
            QQmlJSUtils::resourceFilesFromBuildFolders(QStringList{ buildFolder }));

    QCOMPARE(QQmlJSUtils::qmlSourcePathFromBuildPath(&mapper, buildFile), expectedSourceFile);
    QCOMPARE(QQmlJSUtils::qmlBuildPathFromSourcePath(&mapper, expectedSourceFile), buildFile);
}

void tst_qqmljsutils::qmlBuildPathFromSourcePathWithMultipleRegistrations()
{
    // The mapper is constructed directly (rather than via resourceFilesFromBuildFolders)
    // to control the qrc file order: testdata_builtin.qrc must come first to reproduce
    // the scenario where a source file is registered in multiple qrc files with
    // different resource path prefixes.
    QQmlJSResourceFileMapper mapper({
        testFile(u"mymodule-build-builtin-testdata/.qt/rcc/testdata_builtin.qrc"_s),
        testFile(u"mymodule-build-builtin-testdata/.qt/rcc/raw_qml_0.qrc"_s),
        testFile(u"mymodule-build-builtin-testdata/.qt/rcc/qmake_app.qrc"_s),
    });

    const QString sourceFile = testFile(u"mymodule-source/MyModule/Main.qml"_s);
    const QString expectedBuildPath =
            testFile(u"mymodule-build-builtin-testdata/MyModule/Main.qml"_s);

    QCOMPARE(QQmlJSUtils::qmlBuildPathFromSourcePath(&mapper, sourceFile), expectedBuildPath);
}

QTEST_MAIN(tst_qqmljsutils)
