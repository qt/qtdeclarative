// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qabstractitemmodel.h>
#include <QDebug>
#include <QtQuickTestUtils/private/qmlutils_p.h>

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
    void sortReversed();
    void introspectQrc();
    void sortCaseSensitive_data();
    void sortCaseSensitive();
    void updateProperties();
    void importBothVersions();
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

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != nullptr);
    QSignalSpy folderChangedSpy(flm, SIGNAL(folderChanged()));
    QCOMPARE(flm->property("nameFilters").toStringList(), QStringList() << "*.qml"); // from basic.qml
    QCOMPARE(flm->property("folder").toUrl(), QUrl::fromLocalFile(QDir::currentPath()));
    folderChangedSpy.wait(); // wait for the initial folder to be processed

    flm->setProperty("folder", dataDirectoryUrl());
    QVERIFY(folderChangedSpy.wait());
    QCOMPARE(flm->property("count").toInt(), 9);
    QCOMPARE(flm->property("folder").toUrl(), dataDirectoryUrl());
#ifndef Q_OS_ANDROID
    // On Android currentDir points to some dir in qrc://, which is not
    // considered to be local file, so parentFolder is always
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

    flm->setProperty("folder",QUrl::fromLocalFile(""));
    QCOMPARE(flm->property("folder").toUrl(), QUrl::fromLocalFile(""));
}

void tst_qquickfolderlistmodel::status()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
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

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != nullptr);

    flm->setProperty("folder", dataDirectoryUrl());
    QTRY_COMPARE(flm->property("count").toInt(), 9); // wait for refresh
    QCOMPARE(flm->property("showFiles").toBool(), true);

    flm->setProperty("showFiles", false);
    QCOMPARE(flm->property("showFiles").toBool(), false);
    QTRY_COMPARE(flm->property("count").toInt(), 3); // wait for refresh
}

void tst_qquickfolderlistmodel::resetFiltering()
{
    // see QTBUG-17837
    QQmlComponent component(&engine, testFileUrl("resetFiltering.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
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

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != nullptr);

    connect(flm, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(removed(QModelIndex,int,int)));

#ifdef Q_OS_ANDROID
    // On Android the default folder is application's "files" dir, which
    // requires special rights for reading. The test works when started via
    // androidtestrunner, but fails when launching the APK directly. Set the
    // initial folder to resources root dir, because it is always readable.
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
    flm->setProperty("nameFilters", QStringList() << "*.txt");
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

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != nullptr);

    flm->setProperty("folder", dataDirectoryUrl());
    QTRY_COMPARE(flm->property("count").toInt(), 9); // wait for refresh

    int count = flm->rowCount();

    connect(flm, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(removed(QModelIndex,int,int)));

    flm->setProperty("sortReversed", true);

    QTRY_COMPARE(removeStart, 0);
    QTRY_COMPARE(removeEnd, count-1); // wait for refresh
}

void tst_qquickfolderlistmodel::cdUp()
{
    enum { maxIterations = 50 };
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
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

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
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

    QQmlComponent component(&engine, testFileUrl("showDotAndDotDot.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != nullptr);

    flm->setProperty("folder", folder);
    flm->setProperty("rootFolder", rootFolder);
    flm->setProperty("showDotAndDotDot", showDotAndDotDot);

    int count = 10;
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
#ifdef Q_OS_ANDROID
    QSKIP("Resource file system does not list '.' and '..' due to QDir::entryList() behavior");
#endif
    QTest::addColumn<QUrl>("folder");
    QTest::addColumn<QUrl>("rootFolder");
    QTest::addColumn<bool>("showDotAndDotDot");
    QTest::addColumn<bool>("showDot");
    QTest::addColumn<bool>("showDotDot");

    QTest::newRow("false") << dataDirectoryUrl() << QUrl() << false << false << false;
    QTest::newRow("true") << dataDirectoryUrl() << QUrl() << true << true << true;
    QTest::newRow("true but root") << dataDirectoryUrl() << dataDirectoryUrl() << true << true << false;
}

void tst_qquickfolderlistmodel::sortReversed()
{
    QQmlComponent component(&engine, testFileUrl("sortReversed.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));
    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != nullptr);
    flm->setProperty("folder", dataDirectoryUrl());
    QTRY_COMPARE(flm->property("count").toInt(), 10); // wait for refresh
    QCOMPARE(flm->data(flm->index(0),FileNameRole).toString(), QLatin1String("txtdir"));
}

void tst_qquickfolderlistmodel::introspectQrc()
{
    QQmlComponent component(&engine, testFileUrl("qrc.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));
    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != nullptr);
    QTRY_COMPARE(flm->property("count").toInt(), 1); // wait for refresh
    QCOMPARE(flm->data(flm->index(0),FileNameRole).toString(), QLatin1String("hello.txt"));
}

void tst_qquickfolderlistmodel::sortCaseSensitive_data()
{
    QTest::addColumn<bool>("sortCaseSensitive");
    QTest::addColumn<QStringList>("expectedOrder");

    const QString upperFile = QLatin1String("Uppercase.txt");
    const QString lowerFile = QLatin1String("lowercase.txt");

    QTest::newRow("caseSensitive") << true << (QStringList() << upperFile << lowerFile);
    QTest::newRow("caseInsensitive") << false << (QStringList() << lowerFile << upperFile);
}

void tst_qquickfolderlistmodel::sortCaseSensitive()
{
    QFETCH(bool, sortCaseSensitive);
    QFETCH(QStringList, expectedOrder);
    QQmlComponent component(&engine);
    component.setData("import Qt.labs.folderlistmodel 1.0\n"
                      "FolderListModel { }", QUrl());
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QAbstractListModel *flm = qobject_cast<QAbstractListModel*>(component.create());
    QVERIFY(flm != 0);
    flm->setProperty("folder", testFileUrl("sortdir"));
    flm->setProperty("sortCaseSensitive", sortCaseSensitive);
    QTRY_COMPARE(flm->property("count").toInt(), 2); // wait for refresh
    for (int i = 0; i < 2; ++i)
        QTRY_COMPARE(flm->data(flm->index(i),FileNameRole).toString(), expectedOrder.at(i));
}

void tst_qquickfolderlistmodel::updateProperties()
{
    QQmlComponent component(&engine, testFileUrl("basic.qml"));
    QTRY_VERIFY2(component.isReady(), qPrintable(component.errorString()));

    QObject *folderListModel = component.create();
    QVERIFY(folderListModel);

    QVariant caseSensitive = folderListModel->property("caseSensitive");
    QVERIFY(caseSensitive.isValid());
    QCOMPARE(caseSensitive.toBool(), true);
    folderListModel->setProperty("caseSensitive", false);
    caseSensitive = folderListModel->property("caseSensitive");
    QVERIFY(caseSensitive.isValid());
    QCOMPARE(caseSensitive.toBool(), false);

    QVariant showOnlyReadable = folderListModel->property("showOnlyReadable");
    QVERIFY(showOnlyReadable.isValid());
    QCOMPARE(showOnlyReadable.toBool(), false);
    folderListModel->setProperty("showOnlyReadable", true);
    showOnlyReadable = folderListModel->property("showOnlyReadable");
    QVERIFY(showOnlyReadable.isValid());
    QCOMPARE(showOnlyReadable.toBool(), true);

    QVariant showDotAndDotDot = folderListModel->property("showDotAndDotDot");
    QVERIFY(showDotAndDotDot.isValid());
    QCOMPARE(showDotAndDotDot.toBool(), false);
    folderListModel->setProperty("showDotAndDotDot", true);
    showDotAndDotDot = folderListModel->property("showDotAndDotDot");
    QVERIFY(showDotAndDotDot.isValid());
    QCOMPARE(showDotAndDotDot.toBool(), true);

    QVariant showHidden = folderListModel->property("showHidden");
    QVERIFY(showHidden.isValid());
    QCOMPARE(showHidden.toBool(), false);
    folderListModel->setProperty("showHidden", true);
    showHidden = folderListModel->property("showHidden");
    QVERIFY(showHidden.isValid());
    QCOMPARE(showHidden.toBool(), true);
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
