// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QQmlEngine>
#include <QQmlComponent>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtCore/qstandardpaths.h>

class tst_qqmlstandardpaths : public QQmlDataTest
{
    Q_OBJECT

public:
    explicit tst_qqmlstandardpaths() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private Q_SLOTS:
    void standardPaths();
};

void tst_qqmlstandardpaths::standardPaths()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("tst_standardpaths.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("desktop").toInt(), QStandardPaths::DesktopLocation);
    QCOMPARE(object->property("documents").toInt(), QStandardPaths::DocumentsLocation);
    QCOMPARE(object->property("fonts").toInt(), QStandardPaths::FontsLocation);
    QCOMPARE(object->property("applications").toInt(), QStandardPaths::ApplicationsLocation);
    QCOMPARE(object->property("music").toInt(), QStandardPaths::MusicLocation);
    QCOMPARE(object->property("movies").toInt(), QStandardPaths::MoviesLocation);
    QCOMPARE(object->property("pictures").toInt(), QStandardPaths::PicturesLocation);
    QCOMPARE(object->property("temp").toInt(), QStandardPaths::TempLocation);
    QCOMPARE(object->property("home").toInt(), QStandardPaths::HomeLocation);
    QCOMPARE(object->property("appLocalData").toInt(), QStandardPaths::AppLocalDataLocation);
    QCOMPARE(object->property("cache").toInt(), QStandardPaths::CacheLocation);
    QCOMPARE(object->property("genericData").toInt(), QStandardPaths::GenericDataLocation);
    QCOMPARE(object->property("runtime").toInt(), QStandardPaths::RuntimeLocation);
    QCOMPARE(object->property("config").toInt(), QStandardPaths::ConfigLocation);
    QCOMPARE(object->property("download").toInt(), QStandardPaths::DownloadLocation);
    QCOMPARE(object->property("genericCache").toInt(), QStandardPaths::GenericCacheLocation);
    QCOMPARE(object->property("genericConfig").toInt(), QStandardPaths::GenericConfigLocation);
    QCOMPARE(object->property("appData").toInt(), QStandardPaths::AppDataLocation);
    QCOMPARE(object->property("appConfig").toInt(), QStandardPaths::AppConfigLocation);
    QCOMPARE(object->property("locateFile").toInt(), QStandardPaths::LocateFile);
    QCOMPARE(object->property("locateDirectory").toInt(), QStandardPaths::LocateDirectory);
}

QTEST_MAIN(tst_qqmlstandardpaths)

#include "tst_qqmlstandardpaths.moc"
