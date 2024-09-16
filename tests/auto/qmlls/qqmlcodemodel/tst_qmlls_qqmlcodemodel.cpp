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
    QTest::addColumn<QSet<QString>>("missingFiles");

    const QString folder = testFile("sourceFolder");
    const QString subfolder = testFile("sourceFolder/subSourceFolder/subsubSourceFolder");
    const QSet<QString> noMissingFiles;

    QTest::addRow("notExistingFile") << QStringList{ u"notExistingFile.h"_s } << QStringList{}
                                     << QSet<QString>{ u"notExistingFile.h"_s };

    QTest::addRow("myqmlelement") << QStringList{ u"myqmlelement.h"_s }
                                  << QStringList{ folder + u"/myqmlelement.h"_s,
                                                  subfolder + u"/myqmlelement.h"_s }
                                  << noMissingFiles;

    QTest::addRow("myqmlelement2")
            << QStringList{ u"myqmlelement2.hpp"_s }
            << QStringList{ folder + u"/myqmlelement2.hpp"_s } << noMissingFiles;

    QTest::addRow("anotherqmlelement")
            << QStringList{ u"anotherqmlelement.cpp"_s }
            << QStringList{ subfolder + u"/anotherqmlelement.cpp"_s } << noMissingFiles;
}

void tst_qmlls_qqmlcodemodel::findFilePathsFromFileNames()
{
    QFETCH(QStringList, fileNames);
    QFETCH(QStringList, expectedPaths);
    QFETCH(QSet<QString>, missingFiles);

    QmlLsp::QQmlCodeModel model;
    model.setRootUrls({ testFileUrl(u"sourceFolder"_s).toEncoded() });

    auto result = model.findFilePathsFromFileNames(fileNames);

    // the order only is required for the QCOMPARE
    std::sort(result.begin(), result.end());
    std::sort(expectedPaths.begin(), expectedPaths.end());

    QCOMPARE(result, expectedPaths);
    QCOMPARE(model.ignoreForWatching(), missingFiles);
}

using namespace QQmlJS::Dom;

void tst_qmlls_qqmlcodemodel::fileNamesToWatch()
{
    DomItem qmlFile;
    DomCreationOptions options;
    options.setFlag(DomCreationOption::WithSemanticAnalysis);

    auto envPtr = DomEnvironment::create(QStringList(),
                                         DomEnvironment::Option::SingleThreaded
                                                 | DomEnvironment::Option::NoDependencies, options);

    envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, testFile("MyCppModule/Main.qml")),
                     [&qmlFile](Path, const DomItem &, const DomItem &newIt) {
                         qmlFile = newIt.fileObject();
                     });
    envPtr->loadPendingDependencies();

    const auto fileNames = QmlLsp::QQmlCodeModel::fileNamesToWatch(qmlFile);

    // fileNames also contains some builtins it seems, like:
    // QSet("qqmlcomponentattached_p.h", "qqmlcomponent.h", "qobject.h", "qqmllist.h",
    // "helloworld.h", "qqmlengine_p.h")
    QVERIFY(fileNames.contains(u"helloworld.h"_s));

    // test for no duplicates
    QVERIFY(std::is_sorted(fileNames.begin(), fileNames.end()));
    QVERIFY(std::adjacent_find(fileNames.begin(), fileNames.end()) == fileNames.end());

    // should not contain any empty strings
    QVERIFY(!fileNames.contains(QString()));
}

QString tst_qmlls_qqmlcodemodel::readFile(const QString &filename) const
{
    QFile f(testFile(filename));
    if (!f.open(QFile::ReadOnly)) {
        QTest::qFail("Can't read test file", __FILE__, __LINE__);
        return {};
    }
    return f.readAll();
}

void tst_qmlls_qqmlcodemodel::openFiles()
{
    QmlLsp::QQmlCodeModel model;

    const QByteArray fileAUrl = testFileUrl(u"FileA.qml"_s).toEncoded();
    const QString fileAPath = testFile(u"FileA.qml"_s);

    // open file A
    model.newOpenFile(fileAUrl, 0, readFile(u"FileA.qml"_s));

    QTRY_VERIFY_WITH_TIMEOUT(model.validEnv().field(Fields::qmlFileWithPath).key(fileAPath), 3000);

    {
        const DomItem fileAComponents = model.validEnv()
                                                .field(Fields::qmlFileWithPath)
                                                .key(fileAPath)
                                                .field(Fields::currentItem)
                                                .field(Fields::components);
        // if there is no component then the lazy qml file was not loaded correctly.
        QCOMPARE(fileAComponents.size(), 1);
    }

    model.newDocForOpenFile(fileAUrl, 1, readFile(u"FileA2.qml"_s));

    {
        const DomItem fileAComponents = model.validEnv()
                                                .field(Fields::qmlFileWithPath)
                                                .key(fileAPath)
                                                .field(Fields::currentItem)
                                                .field(Fields::components);
        // if there is no component then the lazy qml file was not loaded correctly.
        QCOMPARE(fileAComponents.size(), 1);

        // also check if the property is there
        const DomItem properties = fileAComponents.key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::propertyDefs);
        QVERIFY(properties);
        QVERIFY(properties.key(u"helloProperty"_s));
    }
}

QTEST_MAIN(tst_qmlls_qqmlcodemodel)
