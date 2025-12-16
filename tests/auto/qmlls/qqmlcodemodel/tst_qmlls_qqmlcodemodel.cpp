// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_qqmlcodemodel.h"

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtQmlLS/private/qprocessscheduler_p.h>
#include <QtQmlLS/private/qqmlcodemodel_p.h>
#include <QtQmlLS/private/qqmlcodemodelmanager_p.h>
#include <QtQmlLS/private/qqmllsutils_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

#include <QtCore/qstringlist.h>
#include <QtTest/qsignalspy.h>

struct TestCodeModelManager final : public QmlLsp::QQmlCodeModelManager
{
    TestCodeModelManager(QQmlToolingSharedSettings *settings = nullptr)
        : QmlLsp::QQmlCodeModelManager(nullptr, settings)
    {
        disableCMakeCalls();
    }

    QmlLsp::QQmlCodeModel *findCodeModelForFile(const QByteArray &url)
    {
        return QmlLsp::QQmlCodeModelManager::findCodeModelForFile(url);
    }
    QmlLsp::QQmlCodeModel *findCodeModel(const QByteArray &url)
    {
        const auto it = QmlLsp::QQmlCodeModelManager::findWorkspace(url);
        return it != m_workspaces.end() ? it->codeModel.get() : nullptr;
    }
};

tst_qmlls_qqmlcodemodel::tst_qmlls_qqmlcodemodel() : QQmlDataTest(QT_QQMLCODEMODEL_DATADIR) { }

void tst_qmlls_qqmlcodemodel::buildPathsForFileUrl_data()
{
    QTest::addColumn<QString>("pathFromIniFile");
    QTest::addColumn<QString>("pathFromCommandLine");
    QTest::addColumn<QString>("expectedPath");

    const QString path1 = u"/Users/helloWorld/build-myProject"_s;
    const QString path2 = u"/Users/helloWorld/build-custom"_s;
    const QString path3 = u"/Users/helloWorld/build-12345678"_s;

    QTest::addRow("justCommandLine") << QString() << path1 << path1;
    QTest::addRow("all3") << path1 << path3 << path3;

    QTest::addRow("commandLineOverridesEnvironmentVariable") << QString() << path3 << path3;
    QTest::addRow("commandLineOverridesIniFile") << path2 << path3 << path3;

    QTest::addRow("iniFile") << path1 << QString() << path1;

    // bug where qmlls allocates memory in an endless loop because of a folder called "_deps"
    QTest::addRow("endlessLoop") << QString() << testFile(u"buildfolderwithdeps"_s)
                                 << testFile(u"buildfolderwithdeps"_s);
}

void tst_qmlls_qqmlcodemodel::buildPathsForFileUrl()
{
    QFETCH(QString, pathFromIniFile);
    QFETCH(QString, pathFromCommandLine);
    QFETCH(QString, expectedPath);

    QQmlToolingSharedSettings settings(u"qmlls"_s);

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    {
        QFile settingsFile(temporaryDirectory.filePath(".qmlls.ini"));
        QVERIFY(settingsFile.open(QFile::WriteOnly | QFile::Text));
        settingsFile.write("[General]\n");
        if (!pathFromIniFile.isEmpty())
            settingsFile.write("buildDir=\"%1\""_L1.arg(pathFromIniFile).toUtf8());
    }

    TestCodeModelManager model(&settings);
    model.addRootUrls({ QUrl::fromLocalFile(temporaryDirectory.path()).toEncoded() });

    if (!pathFromCommandLine.isEmpty())
        model.setBuildPathsForRootUrl(QByteArray(), QStringList{ pathFromCommandLine });

    const QByteArray nonExistentUrl =
            QUrl::fromLocalFile(temporaryDirectory.filePath(u"file.qml"_s)).toEncoded();

    QStringList result = model.buildPathsForFileUrl(nonExistentUrl);
    QCOMPARE_EQ(result, QStringList{ expectedPath });

    result = model.findCodeModelForFile(nonExistentUrl)->buildPathsForOpenedFiles();
    QCOMPARE_EQ(result, QStringList{ expectedPath });
}

void tst_qmlls_qqmlcodemodel::findFilePathsFromFileNames_data()
{
    QTest::addColumn<QStringList>("fileNames");
    QTest::addColumn<QStringList>("expectedPaths");
    QTest::addColumn<QSet<QString>>("missingFiles");
    QTest::addColumn<QSet<QString>>("alreadyWatchedFiles");

    const QString folder = testFile("sourceFolder");
    const QString subfolder = testFile("sourceFolder/subSourceFolder/subsubSourceFolder");
    const QSet<QString> noMissingFiles;
    const QSet<QString> noAlreadyWatchedFiles;

    QTest::addRow("notExistingFile")
            << QStringList{ u"notExistingFile.h"_s } << QStringList{}
            << QSet<QString>{ u"notExistingFile.h"_s } << noAlreadyWatchedFiles;

    QTest::addRow("myqmlelement") << QStringList{ u"myqmlelement.h"_s }
                                  << QStringList{ folder + u"/myqmlelement.h"_s,
                                                  subfolder + u"/myqmlelement.h"_s }
                                  << noMissingFiles << noAlreadyWatchedFiles;

    QTest::addRow("myqmlelementAlreadyWatched")
            << QStringList{ u"myqmlelement.h"_s } << QStringList{ folder + u"/myqmlelement.h"_s }
            << noMissingFiles << QSet<QString>{ subfolder + u"/myqmlelement.h"_s };

    QTest::addRow("myqmlelement2") << QStringList{ u"myqmlelement2.hpp"_s }
                                   << QStringList{ folder + u"/myqmlelement2.hpp"_s }
                                   << noMissingFiles << noAlreadyWatchedFiles;

    QTest::addRow("anotherqmlelement") << QStringList{ u"anotherqmlelement.cpp"_s }
                                       << QStringList{ subfolder + u"/anotherqmlelement.cpp"_s }
                                       << noMissingFiles << noAlreadyWatchedFiles;
}

void tst_qmlls_qqmlcodemodel::findFilePathsFromFileNames()
{
    QFETCH(QStringList, fileNames);
    QFETCH(QStringList, expectedPaths);
    QFETCH(QSet<QString>, missingFiles);
    QFETCH(QSet<QString>, alreadyWatchedFiles);

    QmlLsp::QQmlCodeModel model(testFileUrl(u"sourceFolder"_s).toEncoded());

    auto result = model.findFilePathsFromFileNames(fileNames, alreadyWatchedFiles);

    // the order only is required for the QCOMPARE
    std::sort(result.begin(), result.end());
    std::sort(expectedPaths.begin(), expectedPaths.end());

    QCOMPARE(result, expectedPaths);
    QCOMPARE(model.ignoreForWatching(), missingFiles);
}

using namespace QQmlJS::Dom;

void tst_qmlls_qqmlcodemodel::resourceFiles()
{
    QmlLsp::QQmlCodeModelManager manager;
    const QByteArray rootUrl = testFileUrl("somePath"_L1).toEncoded();

    QTemporaryDir buildDir;
    QVERIFY(buildDir.isValid());
    QDir(buildDir.path()).mkdir(".qt"_L1);
    {
        const QString qmllsBuildIni = buildDir.filePath(".qt/.qmlls.build.ini"_L1);
        QFile qmllsBuildIniFile(qmllsBuildIni);
        QVERIFY(qmllsBuildIniFile.open(QFile::WriteOnly | QFile::Text));
        qmllsBuildIniFile.write("[General]\n[%1]\nresourceFiles=\"%7\"\n"_L1
                                        .arg(testFile("somePath").replace("/"_L1, "<SLASH>"_L1),
                                             testFile("FolderWithResources/Good.qrc"))
                                        .toUtf8());
    }

    manager.addRootUrls({ rootUrl });

    QCOMPARE_EQ(manager.defaultResourceFiles(), QStringList{});
    manager.setBuildPathsForRootUrl(rootUrl, { buildDir.path() });

    QCOMPARE_EQ(manager.defaultResourceFiles(), QStringList{});
    QCOMPARE_EQ(manager.resourceFilesForFileUrl(rootUrl),
                QStringList{ testFile("FolderWithResources/Good.qrc"_L1) });

    manager.setBuildPathsForRootUrl({}, { testFile("FolderWithResources") });
    QStringList defaultResourceFiles = manager.defaultResourceFiles();
    std::sort(defaultResourceFiles.begin(), defaultResourceFiles.end());
    const QStringList expectedDefaultResourceFiles{
        testFile("FolderWithResources/Bad.qrc"_L1),
        testFile("FolderWithResources/Good.qrc"_L1),
    };
    QCOMPARE_EQ(defaultResourceFiles, expectedDefaultResourceFiles);
    QCOMPARE_EQ(manager.resourceFilesForFileUrl(rootUrl),
                QStringList{ testFile("FolderWithResources/Good.qrc"_L1) });
}

void tst_qmlls_qqmlcodemodel::resourceFilesFallback()
{
    QmlLsp::QQmlCodeModelManager manager;
    const QByteArray rootUrl = testFileUrl("somePath").toEncoded();
    manager.addRootUrls({ rootUrl });

    QCOMPARE_EQ(manager.defaultResourceFiles(), QStringList{});
    manager.setBuildPathsForRootUrl(rootUrl, { testFile("FolderWithResources") });

    QCOMPARE_EQ(manager.defaultResourceFiles(), QStringList{});

    const QStringList expectedDefaultResourceFiles{
        testFile("FolderWithResources/Bad.qrc"),
        testFile("FolderWithResources/Good.qrc"),
    };
    QStringList defaultResourceFiles = manager.resourceFilesForFileUrl(rootUrl);
    std::sort(defaultResourceFiles.begin(), defaultResourceFiles.end());
    QCOMPARE_EQ(defaultResourceFiles, expectedDefaultResourceFiles);
}

void tst_qmlls_qqmlcodemodel::fileNamesToWatch()
{
    DomItem qmlFile;

    auto envPtr = DomEnvironment::create(QStringList(),
                                         DomEnvironment::Option::SingleThreaded
                                                 | DomEnvironment::Option::NoDependencies,
                                         Extended);

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

void tst_qmlls_qqmlcodemodel::openFiles_data()
{
    QTest::addColumn<bool>("cmakeEnabled");

    QTest::addRow("withCMake") << true;
    QTest::addRow("withoutCMake") << false;
}

void tst_qmlls_qqmlcodemodel::openFiles()
{
    QFETCH(bool, cmakeEnabled);

    QmlLsp::QQmlCodeModel model;
    model.setImportPaths(QLibraryInfo::paths(QLibraryInfo::QmlImportsPath));

    // disabling CMake should not make the test fail!
    if (!cmakeEnabled)
        model.disableCMakeCalls();

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

    QSignalSpy spy(&model, &QmlLsp::QQmlCodeModel::openUpdateThreadFinished);
    model.newOpenFile(fileAUrl, 1, readFile(u"FileA2.qml"_s));
    // wait for QQmlCodeModel to finish loading
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 3000);

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

void tst_qmlls_qqmlcodemodel::importPathViaSettings()
{
    // prepare the qmlls.ini file
    QFile settingsTemplate(testFile(u"importPathFromSettings/.qmlls.ini.template"_s));
    QVERIFY(settingsTemplate.open(QFile::ReadOnly | QFile::Text));
    const QString data = QString::fromUtf8(settingsTemplate.readAll())
                                 .arg(QStringList{
                                         QDir::cleanPath(testFile(u"."_s)),
                                         testFile(u"SomeFolder"_s),
                                         QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
                                 }
                                              .join(QDir::listSeparator()));

    QFile settingsFile(testFile(u"importPathFromSettings/.qmlls.ini"_s));
    auto guard = qScopeGuard([&settingsFile]() { settingsFile.remove(); });

    QVERIFY(settingsFile.open(QFile::WriteOnly | QFile::Truncate | QFile::Text));
    settingsFile.write(data.toUtf8());
    settingsFile.flush();

    // actually test the qqmlcodemodel
    QQmlToolingSharedSettings settings(u"qmlls"_s);
    settings.addOption(u"importPaths"_s);
    QmlLsp::QQmlCodeModel model(QByteArray(), nullptr, &settings);

    const QString someFile = u"importPathFromSettings/SomeFile.qml"_s;
    const QByteArray fileUrl = testFileUrl(someFile).toEncoded();
    const QString filePath = testFile(someFile);

    model.newOpenFile(fileUrl, 0, readFile(someFile));

    QTRY_VERIFY_WITH_TIMEOUT(model.validEnv().field(Fields::qmlFileWithPath).key(filePath), 3000);

    {
        const DomItem fileAComponents = model.validEnv()
                                                .field(Fields::qmlFileWithPath)
                                                .key(filePath)
                                                .field(Fields::currentItem)
                                                .field(Fields::components);
        // if there is no component then the import path was not used by qqmlcodemodel ?
        QCOMPARE(fileAComponents.size(), 1);
    }
}

static void reloadLotsOfFileMethod()
{
    QmlLsp::QQmlCodeModel model;
    model.setImportPaths(QLibraryInfo::paths(QLibraryInfo::QmlImportsPath));

    QTemporaryDir folder;
    QVERIFY(folder.isValid());

    const QByteArray content = "import QtQuick\n\nItem {}";
    QStringList fileNames;
    for (int i = 0; i < 5; ++i) {
        const QString currentFileName = folder.filePath(QString::number(i).append(u".qml"));
        fileNames.append(currentFileName);

        QFile file(currentFileName);
        QVERIFY(file.open(QFile::WriteOnly));
        file.write(content);
    }

    // open all files
    for (const QString &fileName : fileNames)
        model.newOpenFile(QUrl::fromLocalFile(fileName).toEncoded(), 0, content);

    // wait for them to load
    QTRY_COMPARE_WITH_TIMEOUT(model.validEnv().field(Fields::qmlFileWithPath).keys().size(),
                              fileNames.size(), 3000);

    // populate all files
    for (const QString &key : model.validEnv().field(Fields::qmlFileWithPath).keys()) {
        QCOMPARE(model.validEnv()
                         .field(Fields::qmlFileWithPath)
                         .key(key)
                         .field(Fields::currentItem)
                         .field(Fields::components)
                         .size(),
                 1);
    }

    // modify all files on disk
    for (const QString &fileName : fileNames) {
        QFile file(fileName);
        QVERIFY(file.open(QFile::WriteOnly | QFile::Append));
        file.write("\n\n");
    }

    QSignalSpy spy(&model, &QmlLsp::QQmlCodeModel::openUpdateThreadFinished);
    // update one file
    model.newOpenFile(QUrl::fromLocalFile(fileNames.front()).toEncoded(), 1, content + "\n\n");

    // wait for QQmlCodeModel to finish loading before leaving the scope
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 3000);
}

void tst_qmlls_qqmlcodemodel::reloadLotsOfFiles()
{
    QScopedPointer<QThread> thread(QThread::create([]() { reloadLotsOfFileMethod(); }));

    // should not stack-overflow despite the small stack size to make sure QML files are loaded
    // correctly and not recursively
    thread->setStackSize(1 << 20);
    thread->start();
    thread->wait();
}

void tst_qmlls_qqmlcodemodel::buildPaths_data()
{
    QTest::addColumn<QList<QByteArray>>("roots");
    QTest::addColumn<QByteArray>("root");
    QTest::addColumn<QString>("buildPath");
    QTest::addColumn<QByteArray>("file");

    const QList<QByteArray> roots = {
        testFileUrl("twoWorkspaces/WorkSpaceA"_L1).toEncoded(),
        testFileUrl("twoWorkspaces/WorkSpaceB"_L1).toEncoded(),
    };

    QTest::addRow("WorkspaceA")
            << roots << testFileUrl("twoWorkspaces/WorkSpaceA"_L1).toEncoded()
            << testFile("twoWorkspaces/ImportPathA"_L1)
            << testFileUrl("twoWorkspaces/WorkSpaceA/UseImportPathA.qml"_L1).toEncoded();
    QTest::addRow("WorkspaceB")
            << roots << testFileUrl("twoWorkspaces/WorkSpaceB"_L1).toEncoded()
            << testFile("twoWorkspaces/ImportPathB"_L1)
            << testFileUrl("twoWorkspaces/WorkSpaceB/UseImportPathB.qml"_L1).toEncoded();
}

void tst_qmlls_qqmlcodemodel::buildPaths()
{
    QFETCH(QList<QByteArray>, roots);
    QFETCH(QByteArray, root);
    QFETCH(QString, buildPath);
    QFETCH(QByteArray, file);

    QmlLsp::QQmlCodeModelManager manager;
    manager.addRootUrls(roots);
    QVERIFY(!manager.buildPathsForFileUrl(root).contains(buildPath));
    manager.setBuildPathsForRootUrl(root, { buildPath });
    QVERIFY(manager.buildPathsForFileUrl(root).contains(buildPath));
    QVERIFY(manager.buildPathsForFileUrl(file).contains(buildPath));
}

void tst_qmlls_qqmlcodemodel::defaultWorkspace()
{
    const QByteArray fileAUrl = testFileUrl(u"FileA.qml"_s).toEncoded();
    const QByteArray unrelatedRoot = testFileUrl("twoWorkspaces/WorkSpaceA"_L1).toEncoded();

    TestCodeModelManager manager;
    manager.setImportPaths(QLibraryInfo::paths(QLibraryInfo::QmlImportsPath));
    manager.addRootUrls({ unrelatedRoot });
    manager.newOpenFile(fileAUrl, 0, readFile(u"FileA.qml"_s));
    QTRY_VERIFY_WITH_TIMEOUT(manager.snapshotByUrl(fileAUrl).validDoc, 3000);

    // make sure that fileA was not saved in the unrelated root
    QCOMPARE_NE(manager.findCodeModelForFile(fileAUrl),
                manager.findCodeModelForFile(unrelatedRoot));
    const QByteArray defaultWS;
    QCOMPARE(manager.findCodeModelForFile(fileAUrl), manager.findCodeModelForFile(defaultWS));
}

void tst_qmlls_qqmlcodemodel::closeWorkspace()
{
    const QByteArray defaultRoot;
    const QByteArray root = testFileUrl("twoWorkspaces/WorkSpaceA"_L1).toEncoded();
    const QByteArray file1 =
            testFileUrl("twoWorkspaces/WorkSpaceA/UseImportPathA.qml"_L1).toEncoded();
    const QByteArray file2 =
            testFileUrl("twoWorkspaces/WorkSpaceA/UseImportPathA2.qml"_L1).toEncoded();
    const QString fileContent = readFile("twoWorkspaces/WorkSpaceA/UseImportPathA.qml"_L1);

    TestCodeModelManager manager;
    manager.setImportPaths({ testFile("twoWorkspaces/ImportPathA"_L1),
                             QLibraryInfo::path(QLibraryInfo::QmlImportsPath) });
    manager.addRootUrls({ root });
    manager.newOpenFile(file1, 0, fileContent);
    QTRY_VERIFY_WITH_TIMEOUT(manager.snapshotByUrl(file1).validDoc, 3000);

    QCOMPARE(manager.findCodeModelForFile(file1), manager.findCodeModel(root));

    manager.removeRootUrls({ root });
    QCOMPARE(manager.findCodeModelForFile(file1), manager.findCodeModel(root));
    QCOMPARE_NE(manager.findCodeModelForFile(file1), manager.findCodeModel(defaultRoot));
    QVERIFY(manager.snapshotByUrl(file1).validDoc);

    manager.newOpenFile(file2, 0, fileContent);
    QTRY_VERIFY_WITH_TIMEOUT(manager.snapshotByUrl(file2).validDoc, 3000);
    // new files should not open in closed workspaces
    QCOMPARE(manager.findCodeModelForFile(file2), manager.findCodeModel(defaultRoot));

    manager.closeOpenFile(file1);
    QCOMPARE(manager.findCodeModelForFile(file1), manager.findCodeModel(defaultRoot));
    QVERIFY(!manager.snapshotByUrl(file1).validDoc);
}

void tst_qmlls_qqmlcodemodel::rootUrls()
{
    const QByteArray rootA = testFileUrl("twoWorkspaces/WorkSpaceA"_L1).toEncoded();
    const QByteArray rootB = testFileUrl("twoWorkspaces/WorkSpaceB"_L1).toEncoded();
    const QByteArray defaultWS;

    TestCodeModelManager manager;
    QCOMPARE(manager.rootUrls().size(), 1);
    manager.addRootUrls({ rootA, rootB });
    QCOMPARE(manager.rootUrls().size(), 3);

    QCOMPARE_NE(manager.findCodeModelForFile(rootA), defaultWS);
    QCOMPARE_NE(manager.findCodeModelForFile(rootB), defaultWS);
    QCOMPARE_NE(manager.findCodeModelForFile(rootA), manager.findCodeModelForFile(rootB));
}

void tst_qmlls_qqmlcodemodel::addingWorkspaces()
{
    const QByteArray outerWorkspace = testFileUrl("twoWorkspaces/"_L1).toEncoded();
    const QByteArray innerWorkspace = testFileUrl("twoWorkspaces/WorkSpaceA/"_L1).toEncoded();

    const QByteArray file1 =
            testFileUrl("twoWorkspaces/WorkSpaceA/UseImportPathA.qml"_L1).toEncoded();
    const QByteArray file2 =
            testFileUrl("twoWorkspaces/WorkSpaceA/UseImportPathA2.qml"_L1).toEncoded();

    const QString fileContent = readFile("twoWorkspaces/WorkSpaceA/UseImportPathA.qml"_L1);

    TestCodeModelManager manager;
    manager.setImportPaths({ testFile("twoWorkspaces/ImportPathA"_L1),
                             QLibraryInfo::path(QLibraryInfo::QmlImportsPath) });
    manager.addRootUrls({ outerWorkspace });

    manager.newOpenFile(file1, 0, fileContent);
    QTRY_VERIFY_WITH_TIMEOUT(manager.snapshotByUrl(file1).validDoc, 3000);

    QCOMPARE(manager.findCodeModelForFile(file1), manager.findCodeModelForFile(outerWorkspace));

    manager.addRootUrls({ innerWorkspace });
    // fileA should not use the new root because its open!
    QCOMPARE(manager.findCodeModelForFile(file1), manager.findCodeModelForFile(outerWorkspace));
    // new files like fileB should use the new WS
    QCOMPARE(manager.findCodeModelForFile(file2), manager.findCodeModelForFile(innerWorkspace));

    manager.newOpenFile(file2, 0, fileContent);
    QTRY_VERIFY_WITH_TIMEOUT(manager.snapshotByUrl(file2).validDoc, 3000);
    QCOMPARE(manager.findCodeModelForFile(file2), manager.findCodeModelForFile(innerWorkspace));

    manager.closeOpenFile(file1);
    // fileA was closed, so it can now be reopened in the new outerRoot workspace
    QCOMPARE(manager.findCodeModelForFile(file1), manager.findCodeModelForFile(innerWorkspace));
    manager.newOpenFile(file1, 0, fileContent);
    QTRY_VERIFY_WITH_TIMEOUT(manager.snapshotByUrl(file1).validDoc, 3000);
    QCOMPARE(manager.findCodeModelForFile(file1), manager.findCodeModelForFile(innerWorkspace));

    // closing outerRoot should not affect opened files
    manager.removeRootUrls({ innerWorkspace });
    QCOMPARE(manager.findCodeModelForFile(file1), manager.findCodeModel(innerWorkspace));
    QCOMPARE(manager.findCodeModelForFile(file2), manager.findCodeModel(innerWorkspace));
}

void tst_qmlls_qqmlcodemodel::newWorkspace()
{
    const QByteArray rootA = testFileUrl("twoWorkspaces/WorkSpaceA"_L1).toEncoded();
    const QString buildPathA = testFile("twoWorkspaces/ImportPathA"_L1);
    const QString docPathA = testFile("twoWorkspaces/dummydocpaththatdoesnotexist"_L1);

    TestCodeModelManager manager;

    // set properties before the WS is created (qmlls reads those values via commandline or
    // environment variable, so they should be valid for all code models)
    const QStringList importPaths{ buildPathA, QLibraryInfo::path(QLibraryInfo::QmlImportsPath) };
    manager.setImportPaths(importPaths);
    manager.setDocumentationRootPath(docPathA);

    manager.addRootUrls({ rootA });

    // make sure that the new WS contains the properties set before its existence
    auto *codeModel = manager.findCodeModelForFile(rootA);
    QCOMPARE(codeModel->importPaths(), importPaths);
    QCOMPARE(codeModel->documentationRootPath(), { docPathA });
}

void tst_qmlls_qqmlcodemodel::duplicateWorkspace()
{
    const QByteArray rootA = testFileUrl("twoWorkspaces/WorkSpaceA"_L1).toEncoded();

    TestCodeModelManager manager;
    manager.addRootUrls({ rootA });
    const QList<QByteArray> expectedRoots{ {}, rootA };
    QCOMPARE(manager.rootUrls(), expectedRoots);
    auto *codeModel = manager.findCodeModelForFile(rootA);

    // don't create or recreate ws that already exists
    manager.addRootUrls({ rootA, rootA, rootA });
    QCOMPARE(manager.rootUrls(), expectedRoots);
    QCOMPARE(manager.findCodeModelForFile(rootA), codeModel);
}

void tst_qmlls_qqmlcodemodel::withQmllsBuildIni()
{
    const QByteArray rootAUrl = testFileUrl("twoWorkspaces/WorkSpaceA/"_L1).toEncoded();
    const QByteArray rootBUrl = testFileUrl("twoWorkspaces/WorkSpaceB/"_L1).toEncoded();
    const QString rootA = testFile("twoWorkspaces/WorkSpaceA/"_L1);
    const QString rootB = testFile("twoWorkspaces/WorkSpaceB/"_L1);
    const QString importPathA = testFile("twoWorkspaces/ImportPathA/"_L1);
    const QString importPathB = testFile("twoWorkspaces/ImportPathB/"_L1);
    const QString defaultImportPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    const QStringList expectedImportPathA{ importPathA, defaultImportPath };
    const QStringList expectedImportPathB{ importPathB, defaultImportPath };
    const QByteArray fileAUrl =
            testFileUrl("twoWorkspaces/WorkSpaceA/UseImportPathA.qml"_L1).toEncoded();
    const QByteArray fileBUrl =
            testFileUrl("twoWorkspaces/WorkSpaceB/UseImportPathB.qml"_L1).toEncoded();
    const QString fileA = testFile("twoWorkspaces/WorkSpaceA/UseImportPathA.qml"_L1);

    QTemporaryDir buildPathA;
    QVERIFY(buildPathA.isValid());

    const QString resourceFileA = buildPathA.filePath("resourceA.qrc");
    const QString resourceFileB = buildPathA.filePath("resourceB.qrc");

    QDir(buildPathA.path()).mkdir(".qt"_L1);

    {
        const QString qmllsBuildIni = buildPathA.filePath(".qt/.qmlls.build.ini"_L1);
        QFile qmllsBuildIniFile(qmllsBuildIni);
        QVERIFY(qmllsBuildIniFile.open(QFile::WriteOnly | QFile::Text));
        qmllsBuildIniFile.write(
                "[General]\n[%1]\nimportPaths=\"%2%6%3\"\nresourceFiles=\"%7\"\n[%4]\nimportPaths=\"%5%6%3\"\nresourceFiles=\"%8\""_L1
                        .arg(QString(rootA).replace("/"_L1, "<SLASH>"_L1), importPathA,
                             defaultImportPath, QString(rootB).replace("/"_L1, "<SLASH>"_L1),
                             importPathB, QDir::listSeparator(), resourceFileA, resourceFileB)
                        .toUtf8());
    }

    TestCodeModelManager manager;
    manager.addRootUrls({ rootAUrl });

    QCOMPARE_NE(manager.findCodeModelForFile(rootAUrl)->importPaths(), expectedImportPathA);
    QCOMPARE_NE(manager.findCodeModelForFile(rootBUrl)->importPaths(), expectedImportPathB);

    QCOMPARE(manager.resourceFilesForFileUrl(rootAUrl), {});
    QCOMPARE(manager.resourceFilesForFileUrl(rootBUrl), {});

    manager.setBuildPathsForRootUrl(rootAUrl, { buildPathA.path() });
    // import path was updated using .qmlls.build.ini on existing WS
    QCOMPARE(manager.findCodeModelForFile(rootAUrl)->importPaths(), expectedImportPathA);
    QCOMPARE(manager.findCodeModelForFile(rootAUrl)->importPathsForUrl(rootAUrl),
             expectedImportPathA);
    QCOMPARE(manager.resourceFilesForFileUrl(rootAUrl), { resourceFileA });

    manager.addRootUrls({ rootBUrl });
    // import path was set using .qmlls.build.ini on newly created WS
    QCOMPARE(manager.findCodeModelForFile(rootBUrl)->importPaths(), expectedImportPathB);
    QCOMPARE(manager.findCodeModelForFile(rootBUrl)->importPathsForUrl(rootBUrl),
             expectedImportPathB);
    QCOMPARE(manager.resourceFilesForFileUrl(rootBUrl), { resourceFileB });

    manager.newOpenFile(fileAUrl, 0, readFile("twoWorkspaces/WorkSpaceA/UseImportPathA.qml"_L1));
    manager.newOpenFile(fileBUrl, 0, readFile("twoWorkspaces/WorkSpaceB/UseImportPathB.qml"_L1));

    for (const auto &fileUrl : { fileAUrl, fileBUrl })
        QTRY_VERIFY_WITH_TIMEOUT(manager.snapshotByUrl(fileUrl).validDoc, 3000);

    DomItem domItemA = manager.snapshotByUrl(fileAUrl).validDoc;
    DomItem domItemB = manager.snapshotByUrl(fileBUrl).validDoc;

    // sanity check
    QCOMPARE_NE(domItemA, domItemB);

    auto loadedQmldir = [](const DomItem &item) {
        return item[Fields::components]
                .key(QString())
                .index(0)[Fields::objects]
                .index(0)[Fields::prototypes]
                .index(0)[Fields::get][Fields::uri]
                .value()
                .toString();
    };
    QCOMPARE(loadedQmldir(domItemA), "\"%1\""_L1.arg(importPathA + "MyModule/qmldir"_L1));
    QCOMPARE(loadedQmldir(domItemB), "\"%1\""_L1.arg(importPathB + "MyModule/qmldir"_L1));
}

void tst_qmlls_qqmlcodemodel::withQmllsBuildIniRelativeImportPath()
{
    const QString defaultImportPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);

    QTemporaryDir buildPathA;
    QVERIFY(buildPathA.isValid());

    QDir(buildPathA.path()).mkdir(".qt"_L1);

    {
        const QString qmllsBuildIni = buildPathA.filePath(".qt/.qmlls.build.ini"_L1);
        QFile qmllsBuildIniFile(qmllsBuildIni);
        QVERIFY(qmllsBuildIniFile.open(QFile::WriteOnly | QFile::Text));

        const QString rootA = testFile("twoWorkspaces/WorkSpaceA/"_L1);
        qmllsBuildIniFile.write("[General]\n[%1]\nimportPaths=\"%2%4%3\"\n"_L1
                                        .arg(QString(rootA).replace("/"_L1, "<SLASH>"_L1),
                                             "../ImportPathA", defaultImportPath,
                                             QDir::listSeparator())
                                        .toUtf8());
    }

    const QByteArray rootAUrl = testFileUrl("twoWorkspaces/WorkSpaceA/"_L1).toEncoded();
    TestCodeModelManager manager;
    manager.addRootUrls({ rootAUrl });
    manager.setBuildPathsForRootUrl(rootAUrl, { buildPathA.path() });

    const QString importPathA = testFile("twoWorkspaces/ImportPathA"_L1);
    const QStringList expectedImportPathA{ importPathA, defaultImportPath };
    QCOMPARE(manager.findCodeModelForFile(rootAUrl)->importPaths(), expectedImportPathA);
}

void tst_qmlls_qqmlcodemodel::withQmllsIniRelativeImportPath()
{
    const QString defaultImportPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);

    QQmlToolingSharedSettings settings("qmlls");
    QmlLsp::QQmlCodeModel model(QByteArray{}, nullptr, &settings);
    const QString importPathA = testFile("twoWorkspaces"_L1);
    const QStringList expectedImportPathA = (model.importPaths() << importPathA);
    QCOMPARE(model.importPathsForUrl(testFileUrl("FolderWithQmllsIni/SomeType.qml").toEncoded()),
             expectedImportPathA);
}

void tst_qmlls_qqmlcodemodel::withQmllsBuildIniWithoutRootUrls()
{
    const QByteArray projectRootUrl = testFileUrl("twoWorkspaces/"_L1).toEncoded();
    const QByteArray rootAUrl = testFileUrl("twoWorkspaces/WorkSpaceA/"_L1).toEncoded();
    const QString importPath = testFile("twoWorkspaces/ImportPathA/"_L1);
    const QString defaultImportPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    const QByteArray fileUrl =
            testFileUrl("twoWorkspaces/WorkSpaceA/UseImportPathA.qml"_L1).toEncoded();

    QTemporaryDir buildPathA;
    QVERIFY(buildPathA.isValid());

    QDir(buildPathA.path()).mkdir(".qt"_L1);

    {
        const QString qmllsBuildIni = buildPathA.filePath(".qt/.qmlls.build.ini"_L1);
        QFile qmllsBuildIniFile(qmllsBuildIni);
        QVERIFY(qmllsBuildIniFile.open(QFile::WriteOnly));
        qmllsBuildIniFile.write(
                "[General]\n[%1]\nimportPaths=\"%2%4%3\"\n"_L1
                        .arg(testFile("twoWorkspaces/WorkSpaceA/"_L1).replace("/"_L1, "<SLASH>"_L1),
                             importPath, defaultImportPath, QDir::listSeparator())
                        .toUtf8());
    }

    TestCodeModelManager manager;
    manager.addRootUrls({ projectRootUrl });
    manager.setBuildPathsForRootUrl(projectRootUrl, { buildPathA.path() });

    QCOMPARE(manager.rootUrls(), QList<QByteArray>{} << QByteArray{} << projectRootUrl);

    // opening fileA should create the rootA workspace, even if rootA was never added as root url to
    // manager.
    manager.newOpenFile(fileUrl, 0, readFile("twoWorkspaces/WorkSpaceA/UseImportPathA.qml"_L1));
    QTRY_VERIFY_WITH_TIMEOUT(manager.snapshotByUrl(fileUrl).validDoc, 3000);
    QCOMPARE(manager.rootUrls(), QList<QByteArray>{} << QByteArray{} << projectRootUrl << rootAUrl);

    QCOMPARE_NE(manager.findCodeModelForFile(fileUrl),
                manager.findCodeModelForFile(projectRootUrl));
    const QByteArray defaultRoot;
    QCOMPARE_NE(manager.findCodeModelForFile(fileUrl), manager.findCodeModelForFile(defaultRoot));
    QCOMPARE(manager.findCodeModelForFile(rootAUrl)->importPaths(),
             QStringList{} << importPath << defaultImportPath);

    DomItem domItemA = manager.snapshotByUrl(fileUrl).validDoc;

    auto loadedQmldir = [](const DomItem &item) {
        return item[Fields::components]
                .key(QString())
                .index(0)[Fields::objects]
                .index(0)[Fields::prototypes]
                .index(0)[Fields::get][Fields::uri]
                .value()
                .toString();
    };
    QCOMPARE(loadedQmldir(domItemA),
             "\"%1\""_L1.arg(testFile("twoWorkspaces/ImportPathA/MyModule/qmldir")));
}

void tst_qmlls_qqmlcodemodel::shortestRootUrlForFile()
{
    QmlLsp::QQmlCodeModelManager manager;
    const QByteArray rootA{ testFileUrl("rootA").toEncoded() };
    const QByteArray rootB{ testFileUrl("rootA/sub/subsub/rootB").toEncoded() };
    const QByteArray rootC{ testFileUrl("rootA/sub/rootC").toEncoded() };
    const QByteArray rootD{ testFileUrl("rootD").toEncoded() };

    manager.addRootUrls({ rootA, rootB, rootC, rootD });
    QCOMPARE(manager.shortestRootUrlForFile(testFileUrl("rootA/MyFile.qml").toEncoded()), rootA);
    QCOMPARE(manager.shortestRootUrlForFile(
                     testFileUrl("rootA/sub/subsub/rootB/MyFile.qml").toEncoded()),
             rootA);
    QCOMPARE(manager.shortestRootUrlForFile(testFileUrl("rootA/sub/rootC/MyFile.qml").toEncoded()),
             rootA);
    QCOMPARE(manager.shortestRootUrlForFile(testFileUrl("rootD/sub/rootC/MyFile.qml").toEncoded()),
             rootD);

    QmlLsp::QQmlCodeModelManager empty;
    QCOMPARE(empty.shortestRootUrlForFile(testFileUrl("rootD/sub/rootC/MyFile.qml").toEncoded()),
             QByteArray{});
}

const constexpr char *filenameKey = "QT_TST_QMLLS_QQMLCODEMODEL_WRITE_FILES";

void tst_qmlls_qqmlcodemodel::qprocessSchedulerProcess()
{
    if (!qEnvironmentVariableIsSet(filenameKey))
        return;

    QFile f(qEnvironmentVariable(filenameKey));
    QVERIFY(f.open(QFile::ReadWrite | QFile::Text | QFile::Append));

    f.write("X\n");
}

void tst_qmlls_qqmlcodemodel::qprocessScheduler_data()
{
    QTest::addColumn<QStringList>("fileNames");

    QTest::addRow("empty") << QStringList{};
    QTest::addRow("two") << QStringList{ "a"_L1, "b"_L1 };
    QTest::addRow("five") << QStringList{ "a"_L1, "b"_L1, "c"_L1, "d"_L1, "e"_L1 };
}

void tst_qmlls_qqmlcodemodel::qprocessScheduler()
{
    using QmlLsp::QProcessScheduler;
    QFETCH(QStringList, fileNames);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QList<QProcessScheduler::Command> list;
    for (const QString &fileName : fileNames) {
        QProcessScheduler::Command command{
            QCoreApplication::applicationFilePath(),
            { "qprocessSchedulerProcess"_L1 },
        };
        command.customEnvironment.insert(filenameKey, dir.filePath(fileName));
        list.append(command);
    }

    QProcessScheduler scheduler;
    QSignalSpy spy(&scheduler, &QProcessScheduler::done);
    QCOMPARE(spy.count(), 0);

    scheduler.schedule(list, QByteArray());

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);

    // verify that the processes really ran and wrote something to disk:
    for (const QString &fileName : fileNames)
        QVERIFY(QFile::exists(dir.filePath(fileName)));
}

using Hash = QHash<QByteArray, QStringList>;

void tst_qmlls_qqmlcodemodel::multipleQProcessScheduler_data()
{
    QTest::addColumn<Hash>("fileNamesById");

    QTest::addRow("empty") << Hash{};
    QTest::addRow("empty2") << Hash{
        { "url", {} },
    };
    QTest::addRow("two") << Hash{
        { "url1", { "a"_L1, "b"_L1 } },
        { "url2", { "a"_L1, "b"_L1 } },
    };
    QTest::addRow("five") << Hash{
        { "url1", { "a"_L1, "b"_L1 } },
        { "url2", { "a"_L1, "b"_L1 } },
        { "url3", { "a"_L1, "b"_L1, "e"_L1 } },
        { "url4", { "a"_L1, "b"_L1 } },
        { "url5", { "e"_L1, "b"_L1, "c"_L1, "d"_L1, "a"_L1 } },
    };
    QTest::addRow("duplicate") << Hash{
        { "url1", { "a"_L1, "a"_L1, "a"_L1, "a"_L1, "a"_L1, "a"_L1, "a"_L1 } },
    };
}

void tst_qmlls_qqmlcodemodel::multipleQProcessScheduler()
{
    using QmlLsp::QProcessScheduler;
    QFETCH(Hash, fileNamesById);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QProcessScheduler scheduler;
    QSignalSpy spy(&scheduler, &QProcessScheduler::done);
    QCOMPARE(spy.count(), 0);

    for (const auto &[id, fileNames] : fileNamesById.asKeyValueRange()) {
        QList<QProcessScheduler::Command> list;
        for (const QString &fileName : fileNames) {
            QProcessScheduler::Command command{
                QCoreApplication::applicationFilePath(),
                { "qprocessSchedulerProcess"_L1 },
            };
            command.customEnvironment.insert(filenameKey, dir.filePath(fileName));
            list.append(command);
        }

        scheduler.schedule(list, id);
    }

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), fileNamesById.size(), 5000);

    for (const auto &[id, fileNames] : fileNamesById.asKeyValueRange()) {
        for (const QString &fileName : fileNames)
            QVERIFY(QFile::exists(dir.filePath(fileName)));
    }

    // ensure that duplicates were filtered away and that the file was appended to only once
    if (QTest::currentTestFunction() == "duplicate"_L1) {
        QFile file(dir.filePath("a"_L1));
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        QCOMPARE(file.readAll().count("X"_L1), 1);
    }
}

void tst_qmlls_qqmlcodemodel::reloadQmllsBuildIniAfterBuild()
{
    QmlLsp::QQmlCodeModelManager manager;
    const QByteArray rootUrl{ testFileUrl("rootA").toEncoded() };

    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    manager.addRootUrls({ rootUrl });

    manager.setBuildPathsForRootUrl(rootUrl, { temporaryDir.path() });
    QCOMPARE(manager.importPathsForUrl(rootUrl), QStringList{ temporaryDir.path() });

    QDir dir(temporaryDir.path());
    dir.mkdir(".qt"_L1);

    {
        QFile file(temporaryDir.filePath(".qt/.qmlls.build.ini"_L1));
        QVERIFY(file.open(QFile::WriteOnly | QFile::Text));
        file.write("[General]\n[%1]\nimportPaths=\"test\"\n"_L1
                           .arg(testFile("rootA").replace("/"_L1, "<SLASH>"_L1))
                           .toUtf8());
    }

    manager.onBuildFinished(rootUrl);

    QCOMPARE(manager.importPathsForUrl(rootUrl), QStringList{ testFile("test"_L1) });

    {
        QFile file(temporaryDir.filePath(".qt/.qmlls.build.ini"_L1));
        QVERIFY(file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text));
        file.write("[General]\n[%1]\nimportPaths=\"test2\"\n"_L1
                           .arg(testFile("rootA").replace("/"_L1, "<SLASH>"_L1))
                           .toUtf8());
    }

    manager.onBuildFinished(rootUrl);

    QCOMPARE(manager.importPathsForUrl(rootUrl), QStringList{ testFile("test2"_L1) });
}

QTEST_MAIN(tst_qmlls_qqmlcodemodel)
