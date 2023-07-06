// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qscopeguard.h>
#include <QtTest/QtTest>
#include <QQmlApplicationEngine>
#include <QQmlAbstractUrlInterceptor>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <private/qqmlimport_p.h>
#include <private/qqmlengine_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QQmlComponent>

class tst_QQmlImport : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQmlImport();

private slots:
    void importPathOrder();
    void testDesignerSupported();
    void uiFormatLoading();
    void completeQmldirPaths_data();
    void completeQmldirPaths();
    void interceptQmldir();
    void singletonVersionResolution();
    void removeDynamicPlugin();
    void partialImportVersions_data();
    void partialImportVersions();
    void registerModuleImport();
    void importDependenciesPrecedence();
    void cleanup();
    void envResourceImportPath();
    void preferResourcePath();
    void invalidFileImport_data();
    void invalidFileImport();
    void implicitWithDependencies();
    void qualifiedScriptImport();
    void invalidImportUrl();
};

void tst_QQmlImport::cleanup()
{
    QQmlImports::setDesignerSupportRequired(false);
}

void tst_QQmlImport::envResourceImportPath()
{
    const bool hadEnv = qEnvironmentVariableIsSet("QML_IMPORT_PATH");
    const QByteArray oldEnv = hadEnv ? qgetenv("QML_IMPORT_PATH") : QByteArray();
    auto guard = qScopeGuard([&] {
        if (hadEnv)
            qputenv("QML_IMPORT_PATH", oldEnv);
        else
            qunsetenv("QML_IMPORT_PATH");
    });

    const QStringList envPaths({
        QLatin1String(":/some/resource"),
        dataDirectory(),
        QLatin1String(":/some/other/resource"),
        directory()
    });

    qputenv("QML_IMPORT_PATH", envPaths.join(QDir::listSeparator()).toUtf8());

    QQmlImportDatabase importDb(nullptr);
    const QStringList importPaths = importDb.importPathList();

    for (const QString &path : envPaths)
        QVERIFY((importPaths.contains(path.startsWith(u':') ? QLatin1String("qrc") + path : path)));
}

void tst_QQmlImport::preferResourcePath()
{
    QQmlEngine engine;
    engine.addImportPath(dataDirectory());

    QQmlComponent component(&engine, testFileUrl("prefer.qml"));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> o(component.create());
    QCOMPARE(o->objectName(), "right");
}

void tst_QQmlImport::invalidFileImport_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("import");
    QTest::addRow("file absolute")
            << QStringLiteral("absoluteImport.qml")
            << QStringLiteral("/foo/bar/baz");
    QTest::addRow("resource absolute")
            << QStringLiteral("absoluteResourceImport.qml")
            << QStringLiteral(":/absolute/resource/path");
    QTest::addRow("resource relative")
            << QStringLiteral("relativeResourceImport.qml")
            << QStringLiteral(":relative/resource/path");
}

void tst_QQmlImport::invalidFileImport()
{
    QFETCH(QString, file);
    QFETCH(QString, import);

    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl(file));
    QVERIFY(component.isError());
    QVERIFY(component.errorString().contains(
                QStringLiteral("\"%1\" is not a valid import URL. "
                               "You can pass relative paths or URLs with schema, "
                               "but not absolute paths or resource paths.").arg(import)));
}

void tst_QQmlImport::implicitWithDependencies()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("implicitWithDependencies/A.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->objectName(), QStringLiteral("notARectangle"));
}

void tst_QQmlImport::qualifiedScriptImport()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("qualifiedScriptImport.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("a"), QVariant::fromValue<double>(12));
    QCOMPARE(o->property("b"), QVariant::fromValue<int>(3));
    QCOMPARE(o->property("c"), QVariant());
}

void tst_QQmlImport::invalidImportUrl()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("fileDotSlashImport.qml");
    QQmlComponent component(&engine, url);
    QVERIFY(component.isError());
    QCOMPARE(
            component.errorString(),
            url.toString() + QLatin1String(
                    ":2 Cannot resolve URL for import \"file://./MyModuleName\"\n"));
}

void tst_QQmlImport::testDesignerSupported()
{
    QQuickView *window = new QQuickView();
    window->engine()->addImportPath(directory());

    window->setSource(testFileUrl("testfile_supported.qml"));
    QVERIFY(window->errors().isEmpty());

    window->setSource(testFileUrl("testfile_unsupported.qml"));
    QVERIFY(window->errors().isEmpty());

    QQmlImports::setDesignerSupportRequired(true);

    //imports are cached so we create a new window
    delete window;
    window = new QQuickView();

    window->engine()->addImportPath(directory());
    window->engine()->clearComponentCache();

    window->setSource(testFileUrl("testfile_supported.qml"));
    QVERIFY(window->errors().isEmpty());

    QString warningString("%1:5:1: module does not support the designer \"MyPluginUnsupported\" \n     import MyPluginUnsupported 1.0\r \n     ^ ");
#if !defined(Q_OS_WIN)
    warningString.remove('\r');
#endif
    warningString = warningString.arg(testFileUrl("testfile_unsupported.qml").toString());
    QTest::ignoreMessage(QtWarningMsg, warningString.toLocal8Bit());
    window->setSource(testFileUrl("testfile_unsupported.qml"));
    QVERIFY(!window->errors().isEmpty());

    delete window;
}

void tst_QQmlImport::uiFormatLoading()
{
    int size = 0;

    QQmlApplicationEngine *test = new QQmlApplicationEngine(testFileUrl("TestForm.ui.qml"));
    test->addImportPath(directory());
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    QSignalSpy objectCreated(test, SIGNAL(objectCreated(QObject*,QUrl)));
    test->load(testFileUrl("TestForm.ui.qml"));
    QCOMPARE(objectCreated.size(), size);//one less than rootObjects().size() because we missed the first one
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    QByteArray testQml("import QtQml 2.0; QtObject{property bool success: true; property TestForm t: TestForm{}}");
    test->loadData(testQml, testFileUrl("dynamicTestForm.ui.qml"));
    QCOMPARE(objectCreated.size(), size);
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    test->load(testFileUrl("openTestFormFromDir.qml"));
    QCOMPARE(objectCreated.size(), size);
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    test->load(testFileUrl("openTestFormFromQmlDir.qml"));
    QCOMPARE(objectCreated.size(), size);
    QCOMPARE(test->rootObjects().size(), ++size);
    QVERIFY(test->rootObjects()[size -1]);
    QVERIFY(test->rootObjects()[size -1]->property("success").toBool());

    delete test;
}

tst_QQmlImport::tst_QQmlImport()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQmlImport::importPathOrder()
{
#ifdef Q_OS_ANDROID
    QSKIP("QLibraryInfo::path(QLibraryInfo::QmlImportsPath) returns bogus path on Android, but its nevertheless unusable.");
#endif
    QStringList expectedImportPaths;
    QString appDirPath = QCoreApplication::applicationDirPath();
    QString qml2Imports = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
#ifdef Q_OS_WIN
    // The drive letter has a different case as QQmlImport will
    // cause it to be converted after passing through QUrl
    appDirPath[0] = appDirPath[0].toUpper();
    qml2Imports[0] = qml2Imports[0].toUpper();
#endif
    expectedImportPaths << appDirPath
                        << QLatin1String("qrc:/qt-project.org/imports")
                        << QLatin1String("qrc:/qt/qml")
                        << qml2Imports;
    QQmlEngine engine;
    QCOMPARE(engine.importPathList(), expectedImportPaths);

    // Add an import path
    engine.addImportPath(QT_QMLTEST_DATADIR);
    QFileInfo fi(QT_QMLTEST_DATADIR);
    expectedImportPaths.prepend(fi.absoluteFilePath());
    QCOMPARE(engine.importPathList(), expectedImportPaths);

    // Add qml2Imports again to make it the first of the list
    engine.addImportPath(qml2Imports);
    expectedImportPaths.move(expectedImportPaths.indexOf(qml2Imports), 0);
    QCOMPARE(engine.importPathList(), expectedImportPaths);

    // Verify if the type in the module comes first in the import path list
    // takes the precedence. In the case below, the width of both items
    // should be the same to that of the type defined in "path2".
    engine.addImportPath(testFile("importPathOrder/path1"));
    engine.addImportPath(testFile("importPathOrder/path2"));
    QQmlComponent component(&engine, testFile("importPathOrder/MyModuleTest.qml"));
    QScopedPointer<QObject> rootItem(component.create());
    QVERIFY(component.errorString().isEmpty());
    QVERIFY(!rootItem.isNull());
    QQuickItem *item1 = rootItem->findChild<QQuickItem*>("myItem1");
    QQuickItem *item2 = rootItem->findChild<QQuickItem*>("myItem2");
    QVERIFY(item1 != nullptr);
    QVERIFY(item2 != nullptr);
    QCOMPARE(item1->width(), 200);
    QCOMPARE(item2->width(), 200);
}

Q_DECLARE_METATYPE(QQmlImports::ImportVersion)

void tst_QQmlImport::completeQmldirPaths_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QStringList>("basePaths");
    QTest::addColumn<QTypeRevision>("version");
    QTest::addColumn<QStringList>("expectedPaths");

    QTest::newRow("QtQml") << "QtQml" << (QStringList() << "qtbase/qml/" << "path/to/qml")
                           << QTypeRevision::fromVersion(2, 7)
                           << (QStringList() << "qtbase/qml/QtQml.2.7/qmldir" << "path/to/qml/QtQml.2.7/qmldir"
                                             << "qtbase/qml/QtQml.2/qmldir" << "path/to/qml/QtQml.2/qmldir"
                                             << "qtbase/qml/QtQml/qmldir" << "path/to/qml/QtQml/qmldir");

    QTest::newRow("QtQml.Models") << "QtQml.Models" << QStringList("qtbase/qml/")
                                  << QTypeRevision::fromVersion(2, 2)
                                  << (QStringList() << "qtbase/qml/QtQml/Models.2.2/qmldir" << "qtbase/qml/QtQml.2.2/Models/qmldir"
                                                    << "qtbase/qml/QtQml/Models.2/qmldir" << "qtbase/qml/QtQml.2/Models/qmldir"
                                                    << "qtbase/qml/QtQml/Models/qmldir");

    QTest::newRow("org.qt-project.foo.bar 0.1") << "org.qt-project.foo.bar" << QStringList("qtbase/qml/")
                                                << QTypeRevision::fromVersion(0, 1)
                                                << (QStringList()
                                                    << "qtbase/qml/org/qt-project/foo/bar.0.1/qmldir"
                                                    << "qtbase/qml/org/qt-project/foo.0.1/bar/qmldir"
                                                    << "qtbase/qml/org/qt-project.0.1/foo/bar/qmldir"
                                                    << "qtbase/qml/org.0.1/qt-project/foo/bar/qmldir"
                                                    << "qtbase/qml/org/qt-project/foo/bar.0/qmldir"
                                                    << "qtbase/qml/org/qt-project/foo.0/bar/qmldir"
                                                    << "qtbase/qml/org/qt-project.0/foo/bar/qmldir"
                                                    << "qtbase/qml/org.0/qt-project/foo/bar/qmldir"
                                                    << "qtbase/qml/org/qt-project/foo/bar/qmldir");

    QTest::newRow("org.qt-project.foo.bar 4") << "org.qt-project.foo.bar" << QStringList("qtbase/qml/")
                                              << QTypeRevision::fromMajorVersion(4)
                                              << (QStringList()
                                                  << "qtbase/qml/org/qt-project/foo/bar.4/qmldir"
                                                  << "qtbase/qml/org/qt-project/foo.4/bar/qmldir"
                                                  << "qtbase/qml/org/qt-project.4/foo/bar/qmldir"
                                                  << "qtbase/qml/org.4/qt-project/foo/bar/qmldir"
                                                  << "qtbase/qml/org/qt-project/foo/bar/qmldir");

    QTest::newRow("org.qt-project.foo.bar") << "org.qt-project.foo.bar" << QStringList("qtbase/qml/")
                                            << QTypeRevision()
                                            << (QStringList()
                                                << "qtbase/qml/org/qt-project/foo/bar/qmldir");
}

void tst_QQmlImport::completeQmldirPaths()
{
    QFETCH(QString, uri);
    QFETCH(QStringList, basePaths);
    QFETCH(QTypeRevision, version);
    QFETCH(QStringList, expectedPaths);

    QCOMPARE(QQmlImports::completeQmldirPaths(uri, basePaths, version), expectedPaths);
}

class QmldirUrlInterceptor : public QQmlAbstractUrlInterceptor {
public:
    QUrl intercept(const QUrl &url, DataType type) override
    {
        if (type != UrlString && !url.isEmpty() && url.isValid()) {
            QString str = url.toString(QUrl::None);
            return str.replace(QStringLiteral("$(INTERCEPT)"), QStringLiteral("intercepted"));
        }
        return url;
    }
};

void tst_QQmlImport::interceptQmldir()
{
    QQmlEngine engine;
    QmldirUrlInterceptor interceptor;
    engine.addUrlInterceptor(&interceptor);

    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("interceptQmldir.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
}

// QTBUG-77102
void tst_QQmlImport::singletonVersionResolution()
{
    QQmlEngine engine;
    engine.addImportPath(testFile("QTBUG-77102/imports"));
    {
        // Singleton with higher version is simply ignored when importing lower version of plugin
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl("QTBUG-77102/main.0.9.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
    }
    {
        // but the singleton is not accessible
        QQmlComponent component(&engine);
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, QRegularExpression {".*ReferenceError: MySettings is not defined$"} );
        component.loadUrl(testFileUrl("QTBUG-77102/main.0.9.fail.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
    }
    {
        // unless a version which is high enough is imported
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl("QTBUG-77102/main.1.0.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        auto item = qobject_cast<QQuickItem*>(obj.get());
        QCOMPARE(item->width(), 50);
    }
    {
        // or when there is no number because we are importing from a path
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl("QTBUG-77102/main.nonumber.qml"));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
        auto item = qobject_cast<QQuickItem*>(obj.get());
        QCOMPARE(item->width(), 50);
    }
}

void tst_QQmlImport::removeDynamicPlugin()
{
    qmlClearTypeRegistrations();
    QQmlEngine engine;
    {
        // Load something that adds a dynamic plugin
        QQmlComponent component(&engine, testFileUrl("importQtQuickTooling.qml"));
        // Make sure to use something other than QtTest here, since the !plugins.isEmpty()
        // check will fail if we do.
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    }
    QQmlImportDatabase *imports = &QQmlEnginePrivate::get(&engine)->importDatabase;
    const QStringList &plugins = imports->dynamicPlugins();
    QVERIFY(!plugins.isEmpty());
    for (const QString &plugin : plugins)
        QVERIFY(imports->removeDynamicPlugin(plugin));
    QVERIFY(imports->dynamicPlugins().isEmpty());
    qmlClearTypeRegistrations();
}

void tst_QQmlImport::partialImportVersions_data()
{
    QTest::addColumn<QString>("version");
    QTest::addColumn<bool>("valid");

    QTest::addRow("empty") << "" << true;
    QTest::addRow("2") << "2" << true;
    QTest::addRow("6") << "6" << true;
    QTest::addRow("2.0") << "2.0" << false;
    QTest::addRow("2.3") << "2.3" << true;
    QTest::addRow("2.15") << "2.15" << true;
    QTest::addRow("6.0") << "6.0" << true;
}

void tst_QQmlImport::partialImportVersions()
{
    QFETCH(QString, version);
    QFETCH(bool, valid);

    QQmlEngine engine;

    QQmlComponent component(&engine);

    component.setData("import QtQml " + version.toUtf8() + "; Connections { enabled: false }",
                      QUrl());
    QCOMPARE(component.isReady(), valid);
    if (valid) {
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(!obj.isNull());
    }
}

class NotItem : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Item)
};

void tst_QQmlImport::registerModuleImport()
{
    enum Result { IsItem, IsNotItem, IsNull, IsInvalid };
    const auto check = [&]() -> Result {
        QQmlEngine engine;
        engine.addImportPath(directory());
        QQmlComponent component(&engine);
        component.setData("import MyPluginSupported; Item {}", QUrl());
        if (!component.isReady())
            return IsInvalid;
        QScopedPointer<QObject> obj(component.create());
        if (obj.isNull())
            return IsNull;
        if (qobject_cast<NotItem *>(obj.data()))
            return IsNotItem;
        else if (qobject_cast<QQuickItem *>(obj.data()))
            return IsItem;
        return IsInvalid;
    };

    const auto isValid = [&]() {
        return check() == IsItem;
    };

    qmlRegisterTypesAndRevisions<NotItem>("ShadowQuick", 1);

    qmlRegisterModuleImport("MyPluginSupported", 2, "QtQuick");
    QVERIFY(isValid());
    qmlUnregisterModuleImport("MyPluginSupported", 2, "QtQuick");
    QVERIFY(!isValid());
    qmlRegisterModuleImport("MyPluginSupported", 3, "QtQuick"); // won't match, 3 doesn't exist
    QVERIFY(!isValid());
    qmlRegisterModuleImport("MyPluginSupported", 1, "QtQuick"); // won't match, as we import latest
    QVERIFY(!isValid());
    qmlRegisterModuleImport("MyPluginSupported", QQmlModuleImportModuleAny, "QtQuick");
    QVERIFY(isValid());
    qmlUnregisterModuleImport("MyPluginSupported", QQmlModuleImportModuleAny, "QtQuick");
    QVERIFY(!isValid());
    qmlRegisterModuleImport("MyPluginSupported", QQmlModuleImportModuleAny, "QtQuick",
                            QQmlModuleImportAuto); // matches, because both 2.0
    QVERIFY(isValid());
    qmlUnregisterModuleImport("MyPluginSupported", QQmlModuleImportModuleAny, "QtQuick",
                              QQmlModuleImportAuto);
    QVERIFY(!isValid());
    qmlRegisterModuleImport("MyPluginSupported", 2, "QtQuick", 2, 15);
    QVERIFY(isValid());
    qmlUnregisterModuleImport("MyPluginSupported", 2, "QtQuick", 2, 15);
    QVERIFY(!isValid());
    qmlUnregisterModuleImport("MyPluginSupported", 3, "QtQuick");
    QVERIFY(!isValid());
    qmlUnregisterModuleImport("MyPluginSupported", 1, "QtQuick");
    QVERIFY(!isValid());

    qmlRegisterModuleImport("MyPluginSupported", 2, "ShadowQuick", 1);
    qmlRegisterModuleImport("MyPluginSupported", 2, "QtQuick", 2);
    QCOMPARE(check(), IsItem);

    qmlUnregisterModuleImport("MyPluginSupported", 2, "ShadowQuick", 1);
    qmlUnregisterModuleImport("MyPluginSupported", 2, "QtQuick", 2);

    qmlRegisterModuleImport("MyPluginSupported", 2, "QtQuick", 2);
    qmlRegisterModuleImport("MyPluginSupported", 2, "ShadowQuick", 1);
    QCOMPARE(check(), IsNotItem);

    qmlUnregisterModuleImport("MyPluginSupported", 2, "QtQuick", 2);
    qmlUnregisterModuleImport("MyPluginSupported", 2, "ShadowQuick", 1);
}

void tst_QQmlImport::importDependenciesPrecedence()
{
    QQmlEngine engine;
    engine.addImportPath(dataDirectory());

    QQmlComponent component(&engine, testFile("dependencies.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> instance(component.create());
    QVERIFY(!instance.isNull());
    QCOMPARE(instance->property("a").toString(), QString::fromLatin1("a"));
    QCOMPARE(instance->property("b").toString(), QString::fromLatin1("b"));
}


QTEST_MAIN(tst_QQmlImport)

#include "tst_qqmlimport.moc"
