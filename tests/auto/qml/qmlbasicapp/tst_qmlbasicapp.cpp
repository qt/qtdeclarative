/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
}

void tst_basicapp::resourceFiles()
{
    QVERIFY(QFile::exists(QStringLiteral(":/BasicApp/main.qml")));
    QVERIFY(QFile::exists(QStringLiteral(":/BasicApp/qmldir")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample/Clock.qml")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample/center.png")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample/clock.png")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample/hour.png")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample/minute.png")));
    QVERIFY(QFile::exists(QStringLiteral(":/TimeExample/qmldir")));

    QVERIFY(!QFile::exists(QStringLiteral(":/BasicApp/tst_qmlbasicapp.qmltypes")));
    QVERIFY(!QFile::exists(QStringLiteral(":/TimeExample/qmlqtimeexample.qmltypes")));
}

void tst_basicapp::fileSystemFiles()
{
    const QString basedir = QCoreApplication::applicationDirPath();
    QVERIFY(QFile::exists(basedir + QStringLiteral("/BasicApp/main.qml")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/BasicApp/qmldir")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/BasicApp/tst_qmlbasicapp.qmltypes")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample/Clock.qml")));

    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample/center.png")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample/clock.png")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample/hour.png")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample/minute.png")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample/qmldir")));
    QVERIFY(QFile::exists(basedir + QStringLiteral("/TimeExample/qmlqtimeexample.qmltypes")));
}

void tst_basicapp::qmldirContents()
{
    {
        QFile qmldir(QCoreApplication::applicationDirPath() + "/BasicApp/qmldir");
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
        QFile qmldir(QCoreApplication::applicationDirPath() + "/TimeExample/qmldir");
        QVERIFY(qmldir.open(QIODevice::ReadOnly));
        const QByteArray contents = qmldir.readAll();
        QVERIFY(contents.contains("module TimeExample"));
        QVERIFY(contents.contains("optional plugin"));
        QVERIFY(contents.contains("classname"));
        QVERIFY(contents.contains("typeinfo"));
        QVERIFY(contents.contains("depends QtQml"));
        QVERIFY(contents.contains("prefer :/TimeExample/"));
        QVERIFY(contents.contains("Clock 1.0 Clock.qml"));

        QFile qmldirInResources(":/TimeExample/qmldir");
        QVERIFY(qmldirInResources.open(QIODevice::ReadOnly));
        QCOMPARE(qmldirInResources.readAll(), contents);
    }

    {
        QFile qmldir(QCoreApplication::applicationDirPath() + "/BasicExtension/qmldir");
        QVERIFY(qmldir.open(QIODevice::ReadOnly));
        const QByteArray contents = qmldir.readAll();
        QVERIFY(contents.contains("More 1.0 More.ui.qml"));
        QVERIFY(!contents.contains("Less.js"));
    }
}

QTEST_MAIN(tst_basicapp)
#include "tst_qmlbasicapp.moc"
