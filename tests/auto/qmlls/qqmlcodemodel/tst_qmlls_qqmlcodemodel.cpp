// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_qqmlcodemodel.h"

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtQmlLS/private/qqmlcodemodel_p.h>
#include <QtQmlLS/private/qqmllsutils_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

tst_qmlls_qqmlcodemodel::tst_qmlls_qqmlcodemodel() : QQmlDataTest(QT_QQMLCODEMODEL_DATADIR) { }

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

    // bug where qmlls allocates memory in an endless loop because of a folder called "_deps"
    QTest::addRow("endlessLoop") << QString() << QString() << testFile(u"buildfolderwithdeps"_s)
                                 << testFile(u"buildfolderwithdeps"_s);
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

void tst_qmlls_qqmlcodemodel::findFilePathsFromFileNames_data()
{
    QTest::addColumn<QStringList>("fileNames");
    QTest::addColumn<QStringList>("expectedPaths");

    const QString folder = testFile("sourceFolder");
    const QString subfolder = testFile("sourceFolder/subSourceFolder/subsubSourceFolder");

    QTest::addRow("notExistingFile") << QStringList{ u"notExistingFile.h"_s } << QStringList{};

    QTest::addRow("myqmlelement") << QStringList{ u"myqmlelement.h"_s }
                                  << QStringList{ folder + u"/myqmlelement.h"_s,
                                                  subfolder + u"/myqmlelement.h"_s };

    QTest::addRow("myqmlelement2") << QStringList{ u"myqmlelement2.hpp"_s }
                                   << QStringList{ folder + u"/myqmlelement2.hpp"_s };

    QTest::addRow("anotherqmlelement") << QStringList{ u"anotherqmlelement.cpp"_s }
                                       << QStringList{ subfolder + u"/anotherqmlelement.cpp"_s };
}

void tst_qmlls_qqmlcodemodel::findFilePathsFromFileNames()
{
    QFETCH(QStringList, fileNames);
    QFETCH(QStringList, expectedPaths);

    QmlLsp::QQmlCodeModel model;
    model.setRootUrls({ testFileUrl(u"sourceFolder"_s).toEncoded() });

    auto result = model.findFilePathsFromFileNames(fileNames);
    // the order only is required for the QCOMPARE
    std::sort(result.begin(), result.end());
    std::sort(expectedPaths.begin(), expectedPaths.end());

    QCOMPARE(result, expectedPaths);
}

using namespace QQmlJS::Dom;

void tst_qmlls_qqmlcodemodel::fileNamesToWatch()
{
    DomItem env = DomEnvironment::create(QStringList(),
                                         DomEnvironment::Option::SingleThreaded
                                                 | DomEnvironment::Option::NoDependencies);

    DomItem qmlFile;
    DomCreationOptions options;
    options.setFlag(DomCreationOption::WithSemanticAnalysis);

    env.loadFile(
            FileToLoad::fromFileSystem(env.ownerAs<DomEnvironment>(),
                                       testFile("MyCppModule/Main.qml"), options),
            [&qmlFile](Path, const DomItem &, const DomItem &newIt) {
                qmlFile = newIt.fileObject();
            },
            LoadOption::DefaultLoad);
    env.loadPendingDependencies();

    const auto fileNames = QmlLsp::QQmlCodeModel::fileNamesToWatch(qmlFile);

    // fileNames also contains some builtins it seems, like:
    // QSet("qqmlcomponentattached_p.h", "qqmlcomponent.h", "qobject.h", "qqmllist.h",
    // "helloworld.h", "qqmlengine_p.h")
    QVERIFY(fileNames.contains(u"helloworld.h"_s));
}

QTEST_MAIN(tst_qmlls_qqmlcodemodel)
