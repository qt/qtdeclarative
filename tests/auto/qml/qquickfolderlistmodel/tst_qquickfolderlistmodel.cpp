// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qabstractitemmodel.h>
#include <QDebug>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtLabsFolderListModel/private/qquickfolderlistmodel_p.h>

#if defined (Q_OS_WIN)
#include <qt_windows.h>
#endif

// From qquickfolderlistmodel.h
const int FileNameRole = Qt::UserRole+1;
enum SortField { Unsorted, Name, Time, Size, Type };
enum Status { Null, Ready, Loading };

class tst_qquickfolderlistmodel : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickfolderlistmodel() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

public slots:
    void removed(const QModelIndex &, int start, int end) {
        removeStart = start;
        removeEnd = end;
    }

private slots:
    void initTestCase() override;
    void basicProperties();
    void status();
    void showFiles();
    void resetFiltering();
    void nameFilters();
    void refresh();
    void cdUp();
#ifdef Q_OS_WIN32
    void changeDrive();
#endif
    void showDotAndDotDot();
    void showDotAndDotDot_data();
    void showDot();
    void showDot_data();
    void showDotDot();
    void showDotDot_data();
    void sortReversed();
    void introspectQrc();
    void sortCaseSensitive_data();
    void sortCaseSensitive();
    void updateProperties();
    void importBothVersions();
    void sortField();
    void showDirs();
    void showDirsFirst();

private:
    QQmlEngine engine;

    int removeStart = 0;
    int removeEnd = 0;
};

void tst_qquickfolderlistmodel::initTestCase()
{
    // The tests rely on a fixed number of files in the directory with the qml files
    // (the data dir), so disable the disk cache to avoid creating .qmlc files and
    // confusing the test.
    qputenv("QML_DISABLE_DISK_CACHE", "1");
    QQmlDataTest::initTestCase();
}

void tst_qquickfolderlistmodel::basicProperties()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);
    QAbstractListModel *qalm = static_cast<QAbstractListModel *>(flm);
    QAbstractItemModel *qaim = static_cast<QAbstractItemModel *>(flm);
    QSignalSpy folderChangedSpy(flm, SIGNAL(folderChanged()));
    QCOMPARE(flm->property("nameFilters").toStringList(), QStringList() << "*.qml"); // from basic.qml
    QCOMPARE(flm->property("folder").toUrl(), QUrl::fromLocalFile(QDir::currentPath()));
    folderChangedSpy.wait(); // wait for the initial folder to be processed

    flm->setProperty("folder", dataDirectoryUrl());
    QVERIFY(folderChangedSpy.wait());
    QCOMPARE(flm->property("count").toInt(), 11);
    QCOMPARE(flm->property("folder").toUrl(), dataDirectoryUrl());
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_HARMONY)
    // On Android and HarmonyOS currentDir points to some dir in qrc://,
    // which is not considered to be local file, so parentFolder is always
    // default-constructed QUrl.
    QCOMPARE(flm->property("parentFolder").toUrl(),
             QUrl::fromLocalFile(QDir(directory()).canonicalPath()));
#endif
    QCOMPARE(flm->property("sortField").toInt(), int(Name));
    QCOMPARE(flm->property("nameFilters").toStringList(), QStringList() << "*.qml");
    QCOMPARE(flm->property("sortReversed").toBool(), false);
    QCOMPARE(flm->property("showFiles").toBool(), true);
    QCOMPARE(flm->property("showDirs").toBool(), true);
    QCOMPARE(flm->property("showDotAndDotDot").toBool(), false);
    QCOMPARE(flm->property("showOnlyReadable").toBool(), false);
    QCOMPARE(flm->data(flm->index(0),FileNameRole).toString(), QLatin1String("basic.qml"));
    QCOMPARE(flm->data(flm->index(1),FileNameRole).toString(), QLatin1String("dummy.qml"));
    QCOMPARE(flm->index(0), qalm->index(0));
    QCOMPARE(flm->index(1), qalm->index(1));
    QCOMPARE(flm->index(1, 1), qalm->index(1)); // so far, the column argument is unused
    QCOMPARE(flm->index(1, 0), qaim->index(1, 5));

    flm->setProperty("folder",QUrl::fromLocalFile(""));
    QCOMPARE(flm->property("folder").toUrl(), QUrl::fromLocalFile(""));
}

void tst_qquickfolderlistmodel::status()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);
    QTRY_COMPARE(flm->property("status").toInt(), int(Ready));
    flm->setProperty("folder", QUrl::fromLocalFile(""));
    QTRY_COMPARE(flm->property("status").toInt(), int(Null));
    flm->setProperty("folder", QUrl::fromLocalFile(QDir::currentPath()));
    QTRY_COMPARE(flm->property("status").toInt(), int(Ready));
}

void tst_qquickfolderlistmodel::showFiles()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);

    QSignalSpy showFilesChangedSpy(flm, &QQuickFolderListModel::showFilesChanged);
    flm->setProperty("folder", dataDirectoryUrl());
    QTRY_COMPARE(flm->count(), 11);
    QCOMPARE(flm->showFiles(), true);

    flm->setProperty("showFiles", false);
    QCOMPARE(showFilesChangedSpy.count(), 1);

    QCOMPARE(flm->showFiles(), false);
    QTRY_COMPARE(flm->count(), 3);
}

void tst_qquickfolderlistmodel::resetFiltering()
{
    // see QTBUG-17837
    QQmlComponent component(&engine, testFileUrl("resetFiltering.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);

    flm->setProperty("folder", testFileUrl("resetfiltering"));
    // _q_directoryUpdated may be triggered if model was empty before, but there won't be a rowsRemoved signal
    QTRY_COMPARE(flm->property("count").toInt(),3); // all files visible

    flm->setProperty("folder", testFileUrl("resetfiltering/innerdir"));
    // _q_directoryChanged is triggered so it's a total model refresh
    QTRY_COMPARE(flm->property("count").toInt(),1); // should just be "test2.txt" visible

    flm->setProperty("folder", testFileUrl("resetfiltering"));
    // _q_directoryChanged is triggered so it's a total model refresh
    QTRY_COMPARE(flm->property("count").toInt(),3); // all files visible
}

void tst_qquickfolderlistmodel::nameFilters()
{
    // see QTBUG-36576
    QQmlComponent component(&engine, testFileUrl("resetFiltering.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);

    connect(flm, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(removed(QModelIndex,int,int)));

#if defined(Q_OS_ANDROID) || defined(Q_OS_HARMONY)
    // The default folder is the application's "files" dir. On Android,
    // reading it requires special rights: the test works when started via
    // androidtestrunner, but fails when launching the APK directly. On
    // HarmonyOS, its content depends on how the test is launched. Set the
    // initial folder to the test data directory. It is embedded in the
    // resources and holds files, so the model is not empty.
    flm->setProperty("folder", testFileUrl(""));
#endif

    QTRY_VERIFY(flm->rowCount() > 0);
    // read an invalid directory first...
    flm->setProperty("folder", testFileUrl("nosuchdirectory"));
    QTRY_COMPARE(flm->property("count").toInt(),0);
    flm->setProperty("folder", testFileUrl("resetfiltering"));
    // so that the QTRY_COMPARE for 3 entries will process queued signals
    QTRY_COMPARE(flm->property("count").toInt(),3); // all files visible

    int count = flm->rowCount();
    QSignalSpy nameFilterChangedSpy(flm, &QQuickFolderListModel::nameFilterChanged);
    flm->setProperty("nameFilters", QStringList() << "*.txt");
    QCOMPARE(nameFilterChangedSpy.count(), 1);

    // _q_directoryUpdated triggered with range 0:1
    QTRY_COMPARE(flm->property("count").toInt(),1);
    QCOMPARE(flm->data(flm->index(0),FileNameRole), QVariant("test.txt"));
    QCOMPARE(removeStart, 0);
    QCOMPARE(removeEnd, count-1);

    flm->setProperty("nameFilters", QStringList() << "*.html");
    QTRY_COMPARE(flm->property("count").toInt(),2);
    QCOMPARE(flm->data(flm->index(0),FileNameRole), QVariant("test1.html"));
    QCOMPARE(flm->data(flm->index(1),FileNameRole), QVariant("test2.html"));

    flm->setProperty("nameFilters", QStringList());
    QTRY_COMPARE(flm->property("count").toInt(),3); // all files visible
}

void tst_qquickfolderlistmodel::refresh()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);

    flm->setProperty("folder", dataDirectoryUrl());
    QTRY_COMPARE(flm->property("count").toInt(), 11); // wait for refresh

    int count = flm->rowCount();

    connect(flm, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(removed(QModelIndex,int,int)));

    QSignalSpy sortReversedChangedSpy(flm, &QQuickFolderListModel::sortReversedChanged);
    flm->setProperty("sortReversed", true);
    QCOMPARE(sortReversedChangedSpy.count(), 1);

    QTRY_COMPARE(removeStart, 0);
    QTRY_COMPARE(removeEnd, count-1); // wait for refresh
}

void tst_qquickfolderlistmodel::cdUp()
{
    enum { maxIterations = 50 };
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);
    const QUrl startFolder = flm->property("folder").toUrl();
    QVERIFY(startFolder.isValid());

    // QTBUG-32139: Ensure navigating upwards terminates cleanly and does not
    // return invalid Urls like "file:".
    for (int i = 0; ; ++i) {
        const QVariant folderV = flm->property("parentFolder");
        const QUrl folder = folderV.toUrl();
        if (!folder.isValid())
            break;
        QVERIFY(folder.toString() != QLatin1String("file:"));
        QVERIFY2(i < maxIterations,
                 qPrintable(QString::fromLatin1("Unable to reach root after %1 iterations starting from %2, stuck at %3")
                            .arg(maxIterations).arg(QDir::toNativeSeparators(startFolder.toLocalFile()),
                                                    QDir::toNativeSeparators(folder.toLocalFile()))));
        flm->setProperty("folder", folderV);
    }
}

#ifdef Q_OS_WIN32
void tst_qquickfolderlistmodel::changeDrive()
{
    QSKIP("QTBUG-26728");
    class DriveMapper
    {
    public:
        DriveMapper(const QString &dataDir)
        {
            size_t stringLen = dataDir.length();
            targetPath = new wchar_t[stringLen+1];
            dataDir.toWCharArray(targetPath);
            targetPath[stringLen] = 0;

            DefineDosDevice(DDD_NO_BROADCAST_SYSTEM, L"X:", targetPath);
        }

        ~DriveMapper()
        {
            DefineDosDevice(DDD_EXACT_MATCH_ON_REMOVE | DDD_NO_BROADCAST_SYSTEM | DDD_REMOVE_DEFINITION, L"X:", targetPath);
            delete [] targetPath;
        }

    private:
        wchar_t *targetPath;
    };

    QString dataDir = testFile(0);
    DriveMapper dm(dataDir);
    QQmlComponent component(&engine, testFileUrl("basic.qml"));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != 0);

    QSignalSpy folderChangeSpy(flm, SIGNAL(folderChanged()));

    flm->setProperty("folder",QUrl::fromLocalFile(dataDir));
    QCOMPARE(flm->property("folder").toUrl(), QUrl::fromLocalFile(dataDir));
    QTRY_COMPARE(folderChangeSpy.count(), 1);

    flm->setProperty("folder",QUrl::fromLocalFile("X:/resetfiltering/"));
    QCOMPARE(flm->property("folder").toUrl(), QUrl::fromLocalFile("X:/resetfiltering/"));
    QTRY_COMPARE(folderChangeSpy.count(), 2);
}
#endif

void tst_qquickfolderlistmodel::showDotAndDotDot()
{
    QFETCH(QUrl, folder);
    QFETCH(QUrl, rootFolder);
    QFETCH(bool, showDotAndDotDot);
    QFETCH(bool, showDot);
    QFETCH(bool, showDotDot);
    QFETCH(int, expectedrootFolderChangedSignalCount);
    QFETCH(int, expectedShowDotAndDotDotChangedSignalCount);


    QQmlComponent component(&engine, testFileUrl("showDotAndDotDot.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);

    flm->setProperty("folder", folder);
    QSignalSpy rootFolderChangedspy(flm, &QQuickFolderListModel::rootFolderChanged);
    flm->setProperty("rootFolder", rootFolder);
    QCOMPARE(rootFolderChangedspy.count(),expectedrootFolderChangedSignalCount);


    QSignalSpy showDotAndDotDotChangedSpy(flm, &QQuickFolderListModel::showDotAndDotDotChanged);
    flm->setProperty("showDotAndDotDot", showDotAndDotDot);
    QCOMPARE(showDotAndDotDotChangedSpy.count(),expectedShowDotAndDotDotChangedSignalCount);

    int count = 12;
    if (showDot) count++;
    if (showDotDot) count++;
    QTRY_COMPARE(flm->property("count").toInt(), count); // wait for refresh

    if (showDot)
        QCOMPARE(flm->data(flm->index(0),FileNameRole).toString(), QLatin1String("."));
    if (showDotDot)
        QCOMPARE(flm->data(flm->index(1),FileNameRole).toString(), QLatin1String(".."));
}

void tst_qquickfolderlistmodel::showDotAndDotDot_data()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_HARMONY)
    QSKIP("Resource file system does not list '.' and '..' due to QDir::entryList() behavior");
#endif
    QTest::addColumn<QUrl>("folder");
    QTest::addColumn<QUrl>("rootFolder");
    QTest::addColumn<bool>("showDotAndDotDot");
    QTest::addColumn<bool>("showDot");
    QTest::addColumn<bool>("showDotDot");
    QTest::addColumn<int>("expectedrootFolderChangedSignalCount");
    QTest::addColumn<int>("expectedShowDotAndDotDotChangedSignalCount");

    QTest::newRow("false") << dataDirectoryUrl() << QUrl() << false << false << false << 0 << 0;
    QTest::newRow("true") << dataDirectoryUrl() << QUrl() << true << true << true << 0 << 1;
    QTest::newRow("true but root") << dataDirectoryUrl() << dataDirectoryUrl() << true << true << false << 1 << 1;

}

void tst_qquickfolderlistmodel::showDot()
{
    QFETCH(QUrl, folder);
    QFETCH(bool, showDot);
    QFETCH(int, expectedShowDotChangedSignalCount);

    QQmlComponent component(&engine, testFileUrl("showDot.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);

    flm->setProperty("folder", folder);

    QSignalSpy showDotChangedSpy(flm, &QQuickFolderListModel::showDotChanged);
    flm->setProperty("showDot", showDot);
    QCOMPARE(showDotChangedSpy.count(), expectedShowDotChangedSignalCount);

    int count = 12;
    if (showDot) count++;
    QTRY_COMPARE(flm->property("count").toInt(), count); // wait for refresh

    if (showDot)
        QCOMPARE(flm->data(flm->index(0),FileNameRole).toString(), QLatin1String("."));
}

void tst_qquickfolderlistmodel::showDot_data()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_HARMONY)
    QSKIP("Resource file system does not list '.' and '..' due to QDir::entryList() behavior");
#endif
    QTest::addColumn<QUrl>("folder");
    QTest::addColumn<bool>("showDot");
    QTest::addColumn<int>("expectedShowDotChangedSignalCount");

    QTest::newRow("false") << dataDirectoryUrl() << false << 0;
    QTest::newRow("true") << dataDirectoryUrl() << true << 1;
}

void tst_qquickfolderlistmodel::showDotDot()
{
    QFETCH(QUrl, folder);
    QFETCH(QUrl, rootFolder);
    QFETCH(bool, showDotDot);
    QFETCH(int, expectedRootFolderChangedSignalCount);
    QFETCH(int, expectedShowDotDotChangedSignalCount);

    QQmlComponent component(&engine, testFileUrl("showDotDot.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);

    flm->setProperty("folder", folder);
    QSignalSpy rootFolderChangedspy(flm, &QQuickFolderListModel::rootFolderChanged);
    flm->setProperty("rootFolder", rootFolder);
    QCOMPARE(rootFolderChangedspy.count(),expectedRootFolderChangedSignalCount);

    QSignalSpy showDotDotChangedSpy(flm, &QQuickFolderListModel::showDotDotChanged);
    flm->setProperty("showDotDot", showDotDot);
    QCOMPARE(showDotDotChangedSpy.count(),expectedShowDotDotChangedSignalCount);

    int count = 12;
    if (showDotDot && folder != rootFolder) count++;
    QTRY_COMPARE(flm->property("count").toInt(), count); // wait for refresh

    if (showDotDot) {
        if (folder == rootFolder) // if we are at the root folder, .. should not be present
            QCOMPARE_NE(flm->data(flm->index(0),FileNameRole).toString(), QLatin1String(".."));
        else
            QCOMPARE(flm->data(flm->index(0),FileNameRole).toString(), QLatin1String(".."));
    }
}

void tst_qquickfolderlistmodel::showDotDot_data()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_HARMONY)
    QSKIP("Resource file system does not list '.' and '..' due to QDir::entryList() behavior");
#endif
    QTest::addColumn<QUrl>("folder");
    QTest::addColumn<QUrl>("rootFolder");
    QTest::addColumn<bool>("showDotDot");
    QTest::addColumn<int>("expectedRootFolderChangedSignalCount");
    QTest::addColumn<int>("expectedShowDotDotChangedSignalCount");

    QTest::newRow("false") << dataDirectoryUrl() << QUrl() << false << 0 << 0;
    QTest::newRow("true") << dataDirectoryUrl() << QUrl() << true << 0 << 1;
    QTest::newRow("true but root") << dataDirectoryUrl() << dataDirectoryUrl() << true << 1 << 1;

}

void tst_qquickfolderlistmodel::sortReversed()
{
    QQmlComponent component(&engine, testFileUrl("sortReversed.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));
    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);
    flm->setProperty("folder", dataDirectoryUrl());
    QTRY_COMPARE(flm->property("count").toInt(), 12); // wait for refresh
    QCOMPARE(flm->data(flm->index(0),FileNameRole).toString(), QLatin1String("txtdir"));
}

void tst_qquickfolderlistmodel::introspectQrc()
{
    QQmlComponent component(&engine, testFileUrl("qrc.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));
    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);
    QTRY_COMPARE(flm->property("count").toInt(), 1); // wait for refresh
    QCOMPARE(flm->data(flm->index(0),FileNameRole).toString(), QLatin1String("hello.txt"));
}

void tst_qquickfolderlistmodel::sortCaseSensitive_data()
{
    QTest::addColumn<bool>("sortCaseSensitive");
    QTest::addColumn<QStringList>("expectedOrder");
    QTest::addColumn<int>("expectedsortCaseSensitiveChangedSignalCount");

    const QString upperFile = QLatin1String("Uppercase.txt");
    const QString lowerFile = QLatin1String("lowercase.txt");

    QTest::newRow("caseSensitive") << true << (QStringList() << upperFile << lowerFile) << 0;
    QTest::newRow("caseInsensitive") << false << (QStringList() << lowerFile << upperFile) << 1;
}

void tst_qquickfolderlistmodel::sortCaseSensitive()
{
    QFETCH(bool, sortCaseSensitive);
    QFETCH(QStringList, expectedOrder);
    QFETCH(int,expectedsortCaseSensitiveChangedSignalCount);

    QQmlComponent component(&engine);
    component.setData("import Qt.labs.folderlistmodel 1.0\n"
                      "FolderListModel { }", QUrl());
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != 0);
    flm->setProperty("folder", testFileUrl("sortdir"));
    QSignalSpy sortCaseSensitiveChangedSpy(flm, &QQuickFolderListModel::sortCaseSensitiveChanged);
    flm->setProperty("sortCaseSensitive", sortCaseSensitive);

    QCOMPARE(sortCaseSensitiveChangedSpy.count(),expectedsortCaseSensitiveChangedSignalCount);

    QTRY_COMPARE(flm->property("count").toInt(), 2); // wait for refresh
    for (int i = 0; i < 2; ++i)
        QTRY_COMPARE(flm->data(flm->index(i),FileNameRole).toString(), expectedOrder.at(i));
}

void tst_qquickfolderlistmodel::updateProperties()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm);

    QVariant caseSensitive = flm->property("caseSensitive");
    QVERIFY(caseSensitive.isValid());
    QCOMPARE(caseSensitive.toBool(), true);

    QSignalSpy caseSensitiveChangedSpy(flm, &QQuickFolderListModel::caseSensitiveChanged);
    flm->setProperty("caseSensitive", false);
    QCOMPARE(caseSensitiveChangedSpy.count(), 1);

    caseSensitive = flm->property("caseSensitive");
    QVERIFY(caseSensitive.isValid());
    QCOMPARE(caseSensitive.toBool(), false);

    QVariant showOnlyReadable = flm->property("showOnlyReadable");
    QVERIFY(showOnlyReadable.isValid());
    QCOMPARE(showOnlyReadable.toBool(), false);

    QSignalSpy showOnlyReadableChangedSpy(flm, &QQuickFolderListModel::showOnlyReadableChanged);
    flm->setProperty("showOnlyReadable", true);
    QCOMPARE(showOnlyReadableChangedSpy.count(), 1);

    showOnlyReadable = flm->property("showOnlyReadable");
    QVERIFY(showOnlyReadable.isValid());
    QCOMPARE(showOnlyReadable.toBool(), true);

    QVariant showDotAndDotDot = flm->property("showDotAndDotDot");
    QVERIFY(showDotAndDotDot.isValid());
    QCOMPARE(showDotAndDotDot.toBool(), false);
    flm->setProperty("showDotAndDotDot", true);
    showDotAndDotDot = flm->property("showDotAndDotDot");
    QVERIFY(showDotAndDotDot.isValid());
    QCOMPARE(showDotAndDotDot.toBool(), true);

    QVariant showHidden = flm->property("showHidden");
    QVERIFY(showHidden.isValid());
    QCOMPARE(showHidden.toBool(), false);

    QSignalSpy showHiddenChangedSpy(flm, &QQuickFolderListModel::showHiddenChanged);
    flm->setProperty("showHidden", true);
    QCOMPARE(showHiddenChangedSpy.count(), 1);

    showHidden = flm->property("showHidden");
    QVERIFY(showHidden.isValid());
    QCOMPARE(showHidden.toBool(), true);
}

void tst_qquickfolderlistmodel::sortField()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));
    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm != nullptr);

    QSignalSpy sortFieldChangedSpy(flm, &QQuickFolderListModel::sortFieldChanged);
    flm->setProperty("sortField", SortField::Size);
    QCOMPARE(sortFieldChangedSpy.count(), 1);
}

void tst_qquickfolderlistmodel::showDirs()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm);

    QSignalSpy showDirsChangedSpy(flm, &QQuickFolderListModel::showDirsChanged);

    flm->setProperty("showDirs", true);
    QCOMPARE(showDirsChangedSpy.count(), 0);

    flm->setProperty("showDirs", false);
    QCOMPARE(showDirsChangedSpy.count(), 1);
}

void tst_qquickfolderlistmodel::showDirsFirst()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QQuickFolderListModel *flm = qobject_cast<QQuickFolderListModel*>(component.create());
    QVERIFY(flm);
    QSignalSpy showDirsFirstChangedSpy(flm, &QQuickFolderListModel::showDirsFirstChanged);
    flm->setProperty("showDirsFirst", true);
    QCOMPARE(showDirsFirstChangedSpy.count(), 1);
}

void tst_qquickfolderlistmodel::importBothVersions()
{
    {
        QQmlComponent component(&engine, testFileUrl("sortReversed.qml"));
        QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(obj);
    }
    {
        QQmlComponent component(&engine, testFileUrl("qrc.qml"));
        QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> obj(component.create());
        QVERIFY(obj);
    }
}

QTEST_MAIN(tst_qquickfolderlistmodel)

#include "tst_qquickfolderlistmodel.moc"
