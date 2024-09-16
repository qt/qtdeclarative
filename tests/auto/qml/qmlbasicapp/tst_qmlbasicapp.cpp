// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QQmlEngine>
#include <QQmlComponent>
#include <QObject>
#include <qtest.h>

class tst_basicapp : public QObject
{
    Q_OBJECT
private slots:
    void loadComponent();
    void resourceFiles();
    void fileSystemFiles();
    void qmldirContents();
};

void tst_basicapp::loadComponent()
{
    QQmlEngine engine;
#ifdef Q_OS_ANDROID
    engine.addImportPath(":/");
#endif
    QQmlComponent c(&engine, QStringLiteral("qrc:/BasicApp/main.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer o(c.create());
    QVERIFY(!o.isNull());

    const QTime time = QTime::currentTime();
    const int hour = o->property("hours").toInt();
    QVERIFY(hour >= time.hour() - 1);
    QVERIFY(hour <= time.hour() + 1);

    const int minute = o->property("minutes").toInt();
    QVERIFY(minute >= time.minute() - 1);
    QVERIFY(minute <= time.minute() + 1);

    QObject *more = qvariant_cast<QObject*>(o->property("more"));
    QVERIFY(more);
    QCOMPARE(more->objectName(), QStringLiteral("ui.qml"));

    QCOMPARE(o->property("fromESModule").toString(), QStringLiteral("eee"));
    QCOMPARE(o->property("fromJSFile").toString(), QStringLiteral("bar"));
}

void tst_basicapp::resourceFiles()
{
    QVERIFY(QFile::exists(QStringLiteral(":/BasicApp/main.qml")));
    QVERIFY(QFile::exists(QStringLiteral(":/BasicApp/qmldir")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample2/Clock.qml")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample2/center.png")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample2/clock.png")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample2/hour.png")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample2/minute.png")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample2/qmldir")));

    QVERIFY(!QFile::exists(QStringLiteral(":/BasicApp/tst_qmlbasicapp.qmltypes")));
    QVERIFY(!QFile::exists(QStringLiteral(":/TimeExample2/qmlqtimeexample2.qmltypes")));
}

void tst_basicapp::fileSystemFiles()
{
#if defined(Q_OS_ANDROID) || defined(BUILTIN_TESTDATA)
    QSKIP("This test is not valid when the files can exist only as resources.");
#endif
    const QString basedir = QCoreApplication::applicationDirPath();
    QVERIFY(QFile::exists(basedir + QStringLiteral("/BasicApp/main.qml")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/BasicApp/qmldir")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/BasicApp/tst_qmlbasicapp.qmltypes")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample2/Clock.qml")));

    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample2/center.png")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample2/clock.png")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample2/hour.png")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample2/minute.png")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample2/qmldir")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample2/qmlqtimeexample2.qmltypes")));
}

void tst_basicapp::qmldirContents()
{
#ifdef Q_OS_ANDROID
    const QString basedir = QStringLiteral(":"); // Use qrc resource path on Android
#else
    const QString basedir = QCoreApplication::applicationDirPath();
#endif
    {
        QFile qmldir(basedir + "/BasicApp/qmldir");
        QVERIFY(qmldir.open(QIODevice::ReadOnly));
        const QByteArray contents = qmldir.readAll();
        QVERIFY(contents.contains("module BasicApp"));
        QVERIFY(contents.contains("typeinfo"));
        QVERIFY(contents.contains("prefer :/BasicApp/"));
        QVERIFY(!contents.contains("classname"));
        QVERIFY(!contents.contains("plugin"));

        QFile qmldirInResources(":/BasicApp/qmldir");
        QVERIFY(qmldirInResources.open(QIODevice::ReadOnly));
        QCOMPARE(qmldirInResources.readAll(), contents);
    }

    {
        QFile qmldir(basedir + "/TimeExample2/qmldir");
        QVERIFY(qmldir.open(QIODevice::ReadOnly));
        const QByteArray contents = qmldir.readAll();
        QVERIFY(contents.contains("module TimeExample2"));
        QVERIFY(contents.contains("optional plugin"));
        QVERIFY(contents.contains("classname"));
        QVERIFY(contents.contains("typeinfo"));
        QVERIFY(contents.contains("depends QtQml"));
        QVERIFY(contents.contains("prefer :/TimeExample2/"));
        QVERIFY(contents.contains("Clock 254.0 Clock.qml"));

        QFile qmldirInResources(":/TimeExample2/qmldir");
        QVERIFY(qmldirInResources.open(QIODevice::ReadOnly));
        QCOMPARE(qmldirInResources.readAll(), contents);
    }

    {
        QFile qmldir(basedir + "/BasicExtension/qmldir");
        QVERIFY(qmldir.open(QIODevice::ReadOnly));
        const QByteArray contents = qmldir.readAll();
        QVERIFY(contents.contains("More 254.0 More.ui.qml"));
        QVERIFY(contents.contains("Less.js"));
        QVERIFY(contents.contains("ESModule.mjs"));
        QVERIFY(!contents.contains("lowerCase.js"));
        QVERIFY(!contents.contains("lowerCaseModule.mjs"));
    }
}

QTEST_MAIN(tst_basicapp)
#include "tst_qmlbasicapp.moc"
