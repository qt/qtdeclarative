/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <private/qdeclarativeengine_p.h>

#include <qtest.h>
#include <QDebug>
#include <QDeclarativeEngine>
#include <QFontDatabase>
#include <QFileInfo>
#include <QDeclarativeComponent>
#include <QDesktopServices>
#include <QDir>
#include <QVector3D>
#include <QCryptographicHash>
#include <QtQuick/QQuickItem>
#include <QSignalSpy>
#include "../../shared/util.h"

class tst_qdeclarativeqt : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativeqt() {}

private slots:
    void enums();
    void rgba();
    void hsla();
    void rect();
    void point();
    void size();
    void vector();
    void vector4d();
    void lighter();
    void darker();
    void tint();
    void openUrlExternally();
    void openUrlExternally_pragmaLibrary();
    void md5();
    void createComponent();
    void createComponent_pragmaLibrary();
    void createQmlObject();
    void console();
    void dateTimeConversion();
    void dateTimeFormatting();
    void dateTimeFormatting_data();
    void dateTimeFormattingVariants();
    void dateTimeFormattingVariants_data();
    void isQtObject();
    void btoa();
    void atob();
    void fontFamilies();
    void quit();
    void resolvedUrl();

private:
    QDeclarativeEngine engine;
};

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(TESTDATA(filename));
}

void tst_qdeclarativeqt::enums()
{
    QDeclarativeComponent component(&engine, TEST_FILE("enums.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test1").toInt(), (int)Qt::Key_Escape);
    QCOMPARE(object->property("test2").toInt(), (int)Qt::DescendingOrder);
    QCOMPARE(object->property("test3").toInt(), (int)Qt::ElideMiddle);
    QCOMPARE(object->property("test4").toInt(), (int)Qt::AlignRight);

    delete object;
}

void tst_qdeclarativeqt::rgba()
{
    QDeclarativeComponent component(&engine, TEST_FILE("rgba.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Qt.rgba(): Invalid arguments";
    QString warning2 = component.url().toString() + ":7: Error: Qt.rgba(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);


    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor::fromRgbF(1, 0, 0, 0.8));
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor::fromRgbF(1, 0.5, 0.3, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test3")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor::fromRgbF(1, 1, 1, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test6")), QColor::fromRgbF(0, 0, 0, 0));

    delete object;
}

void tst_qdeclarativeqt::hsla()
{
    QDeclarativeComponent component(&engine, TEST_FILE("hsla.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Qt.hsla(): Invalid arguments";
    QString warning2 = component.url().toString() + ":7: Error: Qt.hsla(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor::fromHslF(1, 0, 0, 0.8));
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor::fromHslF(1, 0.5, 0.3, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test3")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor::fromHslF(1, 1, 1, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test6")), QColor::fromHslF(0, 0, 0, 0));

    delete object;
}

void tst_qdeclarativeqt::rect()
{
    QDeclarativeComponent component(&engine, TEST_FILE("rect.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Qt.rect(): Invalid arguments";
    QString warning2 = component.url().toString() + ":7: Error: Qt.rect(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(qvariant_cast<QRectF>(object->property("test1")), QRectF(10, 13, 100, 109));
    QCOMPARE(qvariant_cast<QRectF>(object->property("test2")), QRectF(-10, 13, 100, 109.6));
    QCOMPARE(qvariant_cast<QRectF>(object->property("test3")), QRectF());
    QCOMPARE(qvariant_cast<QRectF>(object->property("test4")), QRectF());
    QCOMPARE(qvariant_cast<QRectF>(object->property("test5")), QRectF(10, 13, 100, -109));

    delete object;
}

void tst_qdeclarativeqt::point()
{
    QDeclarativeComponent component(&engine, TEST_FILE("point.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Qt.point(): Invalid arguments";
    QString warning2 = component.url().toString() + ":7: Error: Qt.point(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(qvariant_cast<QPointF>(object->property("test1")), QPointF(19, 34));
    QCOMPARE(qvariant_cast<QPointF>(object->property("test2")), QPointF(-3, 109.2));
    QCOMPARE(qvariant_cast<QPointF>(object->property("test3")), QPointF());
    QCOMPARE(qvariant_cast<QPointF>(object->property("test4")), QPointF());

    delete object;
}

void tst_qdeclarativeqt::size()
{
    QDeclarativeComponent component(&engine, TEST_FILE("size.qml"));

    QString warning1 = component.url().toString() + ":7: Error: Qt.size(): Invalid arguments";
    QString warning2 = component.url().toString() + ":8: Error: Qt.size(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(qvariant_cast<QSizeF>(object->property("test1")), QSizeF(19, 34));
    QCOMPARE(qvariant_cast<QSizeF>(object->property("test2")), QSizeF(3, 109.2));
    QCOMPARE(qvariant_cast<QSizeF>(object->property("test3")), QSizeF(-3, 10));
    QCOMPARE(qvariant_cast<QSizeF>(object->property("test4")), QSizeF());
    QCOMPARE(qvariant_cast<QSizeF>(object->property("test5")), QSizeF());

    delete object;
}

void tst_qdeclarativeqt::vector()
{
    QDeclarativeComponent component(&engine, TEST_FILE("vector.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Qt.vector(): Invalid arguments";
    QString warning2 = component.url().toString() + ":7: Error: Qt.vector(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(qvariant_cast<QVector3D>(object->property("test1")), QVector3D(1, 0, 0.9));
    QCOMPARE(qvariant_cast<QVector3D>(object->property("test2")), QVector3D(102, -10, -982.1));
    QCOMPARE(qvariant_cast<QVector3D>(object->property("test3")), QVector3D());
    QCOMPARE(qvariant_cast<QVector3D>(object->property("test4")), QVector3D());

    delete object;
}

void tst_qdeclarativeqt::vector4d()
{
    QDeclarativeComponent component(&engine, TEST_FILE("vector4.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Qt.vector4d(): Invalid arguments";
    QString warning2 = component.url().toString() + ":7: Error: Qt.vector4d(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(qvariant_cast<QVector4D>(object->property("test1")), QVector4D(1, 0, 0.9, 0.6));
    QCOMPARE(qvariant_cast<QVector4D>(object->property("test2")), QVector4D(102, -10, -982.1, 10));
    QCOMPARE(qvariant_cast<QVector4D>(object->property("test3")), QVector4D());
    QCOMPARE(qvariant_cast<QVector4D>(object->property("test4")), QVector4D());

    delete object;
}

void tst_qdeclarativeqt::lighter()
{
    QDeclarativeComponent component(&engine, TEST_FILE("lighter.qml"));

    QString warning1 = component.url().toString() + ":5: Error: Qt.lighter(): Invalid arguments";
    QString warning2 = component.url().toString() + ":10: Error: Qt.lighter(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor::fromRgbF(1, 0.8, 0.3).lighter());
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test3")), QColor::fromRgbF(1, 0.8, 0.3).lighter(180));
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor("red").lighter());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test6")), QColor());

    delete object;
}

void tst_qdeclarativeqt::darker()
{
    QDeclarativeComponent component(&engine, TEST_FILE("darker.qml"));

    QString warning1 = component.url().toString() + ":5: Error: Qt.darker(): Invalid arguments";
    QString warning2 = component.url().toString() + ":10: Error: Qt.darker(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor::fromRgbF(1, 0.8, 0.3).darker());
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test3")), QColor::fromRgbF(1, 0.8, 0.3).darker(280));
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor("red").darker());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test6")), QColor());

    delete object;
}

void tst_qdeclarativeqt::tint()
{
    QDeclarativeComponent component(&engine, TEST_FILE("tint.qml"));

    QString warning1 = component.url().toString() + ":7: Error: Qt.tint(): Invalid arguments";
    QString warning2 = component.url().toString() + ":8: Error: Qt.tint(): Invalid arguments";

    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor::fromRgbF(0, 0, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor::fromRgbF(1, 0, 0));
    QColor test3 = qvariant_cast<QColor>(object->property("test3"));
    QCOMPARE(test3.rgba(), 0xFF7F0080);
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor());

    delete object;
}

class MyUrlHandler : public QObject
{
    Q_OBJECT
public:
    MyUrlHandler() : called(0) { }
    int called;
    QUrl last;

public slots:
    void noteCall(const QUrl &url) { called++; last = url; }
};

void tst_qdeclarativeqt::openUrlExternally()
{
    MyUrlHandler handler;

    QDesktopServices::setUrlHandler("test", &handler, "noteCall");
    QDesktopServices::setUrlHandler("file", &handler, "noteCall");

    QDeclarativeComponent component(&engine, TEST_FILE("openUrlExternally.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(handler.called,1);
    QCOMPARE(handler.last, QUrl("test:url"));

    object->setProperty("testFile", true);

    QCOMPARE(handler.called,2);
    QCOMPARE(handler.last, TEST_FILE("test.html"));

    QDesktopServices::unsetUrlHandler("test");
    QDesktopServices::unsetUrlHandler("file");
}

void tst_qdeclarativeqt::openUrlExternally_pragmaLibrary()
{
    MyUrlHandler handler;

    QDesktopServices::setUrlHandler("test", &handler, "noteCall");
    QDesktopServices::setUrlHandler("file", &handler, "noteCall");

    QDeclarativeComponent component(&engine, TEST_FILE("openUrlExternally_lib.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(handler.called,1);
    QCOMPARE(handler.last, QUrl("test:url"));

    object->setProperty("testFile", true);

    QCOMPARE(handler.called,2);
    QCOMPARE(handler.last, TEST_FILE("test.html"));

    QDesktopServices::unsetUrlHandler("test");
    QDesktopServices::unsetUrlHandler("file");
}

void tst_qdeclarativeqt::md5()
{
    QDeclarativeComponent component(&engine, TEST_FILE("md5.qml"));

    QString warning1 = component.url().toString() + ":4: Error: Qt.md5(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test2").toString(), QLatin1String(QCryptographicHash::hash("Hello World", QCryptographicHash::Md5).toHex()));

    delete object;
}

void tst_qdeclarativeqt::createComponent()
{
    QDeclarativeComponent component(&engine, TEST_FILE("createComponent.qml"));

    QString warning1 = component.url().toString() + ":9: Error: Qt.createComponent(): Invalid arguments";
    QString warning2 = component.url().toString() + ":10: Error: Qt.createComponent(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("absoluteUrl").toString(), QString("http://www.example.com/test.qml"));
    QCOMPARE(object->property("relativeUrl").toString(), TEST_FILE("createComponentData.qml").toString());

    delete object;
}

void tst_qdeclarativeqt::createComponent_pragmaLibrary()
{
    // Currently, just loading createComponent_lib.qml causes crash on some platforms
    QDeclarativeComponent component(&engine, TEST_FILE("createComponent_lib.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(object->property("status").toInt(), int(QDeclarativeComponent::Ready));
    QCOMPARE(object->property("readValue").toInt(), int(1913));
    delete object;
}

void tst_qdeclarativeqt::createQmlObject()
{
    QDeclarativeComponent component(&engine, TEST_FILE("createQmlObject.qml"));

    QString warning1 = component.url().toString() + ":7: Error: Qt.createQmlObject(): Invalid arguments";
    QString warning2 = component.url().toString()+ ":10: Error: Qt.createQmlObject(): failed to create object: \n    " + TEST_FILE("inline").toString() + ":2:10: Blah is not a type";
    QString warning3 = component.url().toString()+ ":11: Error: Qt.createQmlObject(): failed to create object: \n    " + TEST_FILE("main.qml").toString() + ":4:1: Duplicate property name";
    QString warning4 = component.url().toString()+ ":9: Error: Qt.createQmlObject(): Missing parent object";
    QString warning5 = component.url().toString()+ ":8: Error: Qt.createQmlObject(): Invalid arguments";
    QString warning6 = "RunTimeError:  Qt.createQmlObject(): failed to create object: \n    " + TEST_FILE("inline").toString() + ":3: Cannot assign object type QObject with no default method";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning3));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning4));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning5));
    QTest::ignoreMessage(QtDebugMsg, qPrintable(warning6));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("emptyArg").toBool(), true);
    QCOMPARE(object->property("success").toBool(), true);

    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    QVERIFY(item != 0);
    QVERIFY(item->childItems().count() == 1);

    delete object;
}

void tst_qdeclarativeqt::console()
{
    QUrl testFileUrl = TEST_FILE("console.qml");
    QString testException = QString(QLatin1String("%1:%2: ReferenceError: Can't find variable: exception")).arg(testFileUrl.toString()).arg(34);
    QString testTrace = QString(QLatin1String("onCompleted (%1:%2:%3)\n")).arg(testFileUrl.toString()).arg(31).arg(17);
    QTest::ignoreMessage(QtDebugMsg, qPrintable(testTrace));
    QTest::ignoreMessage(QtDebugMsg, "Profiling started.");
    QTest::ignoreMessage(QtDebugMsg, "Profiling ended.");
    QTest::ignoreMessage(QtDebugMsg, "completed ok");
    QTest::ignoreMessage(QtDebugMsg, "completed ok");
    QTest::ignoreMessage(QtDebugMsg, "completed ok");
    QTest::ignoreMessage(QtWarningMsg, "completed ok");
    QTest::ignoreMessage(QtCriticalMsg, "completed ok");

    QTest::ignoreMessage(QtDebugMsg, "[1,2]");
    QTest::ignoreMessage(QtDebugMsg, "Object");
    QTest::ignoreMessage(QtDebugMsg, "undefined");
    QTest::ignoreMessage(QtDebugMsg, "12");
    QTest::ignoreMessage(QtDebugMsg, "function () { return 5;}");
    QTest::ignoreMessage(QtDebugMsg, "true");
    QTest::ignoreMessage(QtDebugMsg, "Object");
    QTest::ignoreMessage(QtDebugMsg, "Object");
    QTest::ignoreMessage(QtDebugMsg, "1 pong! Object");
    QTest::ignoreMessage(QtDebugMsg, "1 [ping,pong] Object 2");


    QTest::ignoreMessage(QtWarningMsg, qPrintable(testException));


    QDeclarativeComponent component(&engine, testFileUrl);
    QObject *object = component.create();
    QVERIFY(object != 0);
    delete object;
}

void tst_qdeclarativeqt::dateTimeConversion()
{
    QDate date(2008,12,24);
    QTime time(14,15,38,200);
    QDateTime dateTime(date, time);
    //Note that when converting Date to QDateTime they can argue over historical DST data when converting to local time.
    //Tests should use UTC or recent dates.
    QDateTime dateTime2(QDate(2852,12,31), QTime(23,59,59,500));
    QDateTime dateTime3(QDate(2000,1,1), QTime(0,0,0,0));
    QDateTime dateTime4(QDate(2001,2,2), QTime(0,0,0,0));
    QDateTime dateTime5(QDate(1999,1,1), QTime(2,3,4,0));
    QDateTime dateTime6(QDate(2008,2,24), QTime(14,15,38,200));

    QDeclarativeEngine eng;
    QDeclarativeComponent component(&eng, TEST_FILE("dateTimeConversion.qml"));
    QObject *obj = component.create();

    QCOMPARE(obj->property("qdate").toDate(), date);
    QCOMPARE(obj->property("qtime").toTime(), time);
    QCOMPARE(obj->property("qdatetime").toDateTime(), dateTime);
    QCOMPARE(obj->property("qdatetime2").toDateTime(), dateTime2);
    QCOMPARE(obj->property("qdatetime3").toDateTime(), dateTime3);
    QCOMPARE(obj->property("qdatetime4").toDateTime(), dateTime4);
    QCOMPARE(obj->property("qdatetime5").toDateTime(), dateTime5);
    QCOMPARE(obj->property("qdatetime6").toDateTime(), dateTime6);
}

void tst_qdeclarativeqt::dateTimeFormatting()
{
    QFETCH(QString, method);
    QFETCH(QStringList, inputProperties);
    QFETCH(QStringList, expectedResults);

    QDate date(2008,12,24);
    QTime time(14,15,38,200);
    QDateTime dateTime(date, time);

    QDeclarativeEngine eng;

    eng.rootContext()->setContextProperty("qdate", date);
    eng.rootContext()->setContextProperty("qtime", time);
    eng.rootContext()->setContextProperty("qdatetime", dateTime);

    QDeclarativeComponent component(&eng, TEST_FILE("formatting.qml"));

    QStringList warnings;
    warnings << component.url().toString() + ":37: Error: Qt.formatDate(): Invalid date format"
        << component.url().toString() + ":36: Error: Qt.formatDate(): Invalid arguments"
        << component.url().toString() + ":40: Error: Qt.formatTime(): Invalid time format"
        << component.url().toString() + ":39: Error: Qt.formatTime(): Invalid arguments"
        << component.url().toString() + ":43: Error: Qt.formatDateTime(): Invalid datetime format"
        << component.url().toString() + ":42: Error: Qt.formatDateTime(): Invalid arguments";

    foreach (const QString &warning, warnings)
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));

    QObject *object = component.create();
    QVERIFY2(component.errorString().isEmpty(), qPrintable(component.errorString()));
    QVERIFY(object != 0);

    QVERIFY(inputProperties.count() > 0);
    QVariant result;
    foreach(const QString &prop, inputProperties) {
        QVERIFY(QMetaObject::invokeMethod(object, method.toUtf8().constData(),
                Q_RETURN_ARG(QVariant, result),
                Q_ARG(QVariant, prop)));
        QStringList output = result.toStringList();
        QCOMPARE(output.size(), expectedResults.size());
        for (int i=0; i<output.count(); i++)
            QCOMPARE(output[i], expectedResults[i]);
    }

    delete object;
}

void tst_qdeclarativeqt::dateTimeFormatting_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<QStringList>("inputProperties");
    QTest::addColumn<QStringList>("expectedResults");

    QDate date(2008,12,24);
    QTime time(14,15,38,200);
    QDateTime dateTime(date, time);

    QTest::newRow("formatDate")
        << "formatDate"
        << (QStringList() << "dateFromString" << "jsdate" << "qdate" << "qdatetime")
        << (QStringList() << date.toString(Qt::DefaultLocaleShortDate)
                          << date.toString(Qt::DefaultLocaleLongDate)
                          << date.toString("ddd MMMM d yy"));

    QTest::newRow("formatTime")
        << "formatTime"
        << (QStringList() << "jsdate" << "qtime" << "qdatetime")
        << (QStringList() << time.toString(Qt::DefaultLocaleShortDate)
                          << time.toString(Qt::DefaultLocaleLongDate)
                          << time.toString("H:m:s a")
                          << time.toString("hh:mm:ss.zzz"));

    QTest::newRow("formatDateTime")
        << "formatDateTime"
        << (QStringList() << "jsdate" << "qdatetime")
        << (QStringList() << dateTime.toString(Qt::DefaultLocaleShortDate)
                          << dateTime.toString(Qt::DefaultLocaleLongDate)
                          << dateTime.toString("M/d/yy H:m:s a"));
}

void tst_qdeclarativeqt::dateTimeFormattingVariants()
{
    QFETCH(QString, method);
    QFETCH(QVariant, variant);
    QFETCH(QStringList, expectedResults);

    QDeclarativeEngine eng;
    eng.rootContext()->setContextProperty("qvariant", variant);
    QDeclarativeComponent component(&eng, TEST_FILE("formatting.qml"));

    QStringList warnings;
    warnings << component.url().toString() + ":37: Error: Qt.formatDate(): Invalid date format"
        << component.url().toString() + ":36: Error: Qt.formatDate(): Invalid arguments"
        << component.url().toString() + ":40: Error: Qt.formatTime(): Invalid time format"
        << component.url().toString() + ":39: Error: Qt.formatTime(): Invalid arguments"
        << component.url().toString() + ":43: Error: Qt.formatDateTime(): Invalid datetime format"
        << component.url().toString() + ":42: Error: Qt.formatDateTime(): Invalid arguments";

    foreach (const QString &warning, warnings)
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));

    QObject *object = component.create();
    QVERIFY2(component.errorString().isEmpty(), qPrintable(component.errorString()));
    QVERIFY(object != 0);

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(object, method.toUtf8().constData(),
            Q_RETURN_ARG(QVariant, result),
            Q_ARG(QVariant, QString(QLatin1String("qvariant")))));
    QStringList output = result.toStringList();
    QCOMPARE(output, expectedResults);

    delete object;
}

void tst_qdeclarativeqt::dateTimeFormattingVariants_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<QStringList>("expectedResults");

    QDateTime temporary;

    QTime time(11, 16, 39, 755);
    temporary = QDateTime(QDate(1970,1,1), time);
    QTest::newRow("formatDate, qtime") << "formatDate" << QVariant::fromValue(time) << (QStringList() << temporary.date().toString(Qt::DefaultLocaleShortDate) << temporary.date().toString(Qt::DefaultLocaleLongDate) << temporary.date().toString("ddd MMMM d yy"));
    QTest::newRow("formatDateTime, qtime") << "formatDateTime" << QVariant::fromValue(time) << (QStringList() << temporary.toString(Qt::DefaultLocaleShortDate) << temporary.toString(Qt::DefaultLocaleLongDate) << temporary.toString("M/d/yy H:m:s a"));
    QTest::newRow("formatTime, qtime") << "formatTime" << QVariant::fromValue(time) << (QStringList() << temporary.time().toString(Qt::DefaultLocaleShortDate) << temporary.time().toString(Qt::DefaultLocaleLongDate) << temporary.time().toString("H:m:s a") << temporary.time().toString("hh:mm:ss.zzz"));

    QDate date(2011,5,31);
    temporary = QDateTime(date);
    QTest::newRow("formatDate, qdate") << "formatDate" << QVariant::fromValue(date) << (QStringList() << temporary.date().toString(Qt::DefaultLocaleShortDate) << temporary.date().toString(Qt::DefaultLocaleLongDate) << temporary.date().toString("ddd MMMM d yy"));
    QTest::newRow("formatDateTime, qdate") << "formatDateTime" << QVariant::fromValue(date) << (QStringList() << temporary.toString(Qt::DefaultLocaleShortDate) << temporary.toString(Qt::DefaultLocaleLongDate) << temporary.toString("M/d/yy H:m:s a"));
    QTest::newRow("formatTime, qdate") << "formatTime" << QVariant::fromValue(date) << (QStringList() << temporary.time().toString(Qt::DefaultLocaleShortDate) << temporary.time().toString(Qt::DefaultLocaleLongDate) << temporary.time().toString("H:m:s a") << temporary.time().toString("hh:mm:ss.zzz"));

    QDateTime dateTime(date, time);
    temporary = dateTime;
    QTest::newRow("formatDate, qdatetime") << "formatDate" << QVariant::fromValue(dateTime) << (QStringList() << temporary.date().toString(Qt::DefaultLocaleShortDate) << temporary.date().toString(Qt::DefaultLocaleLongDate) << temporary.date().toString("ddd MMMM d yy"));
    QTest::newRow("formatDateTime, qdatetime") << "formatDateTime" << QVariant::fromValue(dateTime) << (QStringList() << temporary.toString(Qt::DefaultLocaleShortDate) << temporary.toString(Qt::DefaultLocaleLongDate) << temporary.toString("M/d/yy H:m:s a"));
    QTest::newRow("formatTime, qdatetime") << "formatTime" << QVariant::fromValue(dateTime) << (QStringList() << temporary.time().toString(Qt::DefaultLocaleShortDate) << temporary.time().toString(Qt::DefaultLocaleLongDate) << temporary.time().toString("H:m:s a") << temporary.time().toString("hh:mm:ss.zzz"));

    QString string(QLatin1String("2011/05/31 11:16:39.755"));
    temporary = QDateTime::fromString(string, "yyyy/MM/dd HH:mm:ss.zzz");
    QTest::newRow("formatDate, qstring") << "formatDate" << QVariant::fromValue(string) << (QStringList() << temporary.date().toString(Qt::DefaultLocaleShortDate) << temporary.date().toString(Qt::DefaultLocaleLongDate) << temporary.date().toString("ddd MMMM d yy"));
    QTest::newRow("formatDateTime, qstring") << "formatDateTime" << QVariant::fromValue(string) << (QStringList() << temporary.toString(Qt::DefaultLocaleShortDate) << temporary.toString(Qt::DefaultLocaleLongDate) << temporary.toString("M/d/yy H:m:s a"));
    QTest::newRow("formatTime, qstring") << "formatTime" << QVariant::fromValue(string) << (QStringList() << temporary.time().toString(Qt::DefaultLocaleShortDate) << temporary.time().toString(Qt::DefaultLocaleLongDate) << temporary.time().toString("H:m:s a") << temporary.time().toString("hh:mm:ss.zzz"));

    QColor color(Qt::red);
    temporary = QVariant::fromValue(color).toDateTime();
    QTest::newRow("formatDate, qcolor") << "formatDate" << QVariant::fromValue(color) << (QStringList() << temporary.date().toString(Qt::DefaultLocaleShortDate) << temporary.date().toString(Qt::DefaultLocaleLongDate) << temporary.date().toString("ddd MMMM d yy"));
    QTest::newRow("formatDateTime, qcolor") << "formatDateTime" << QVariant::fromValue(color) << (QStringList() << temporary.toString(Qt::DefaultLocaleShortDate) << temporary.toString(Qt::DefaultLocaleLongDate) << temporary.toString("M/d/yy H:m:s a"));
    QTest::newRow("formatTime, qcolor") << "formatTime" << QVariant::fromValue(color) << (QStringList() << temporary.time().toString(Qt::DefaultLocaleShortDate) << temporary.time().toString(Qt::DefaultLocaleLongDate) << temporary.time().toString("H:m:s a") << temporary.time().toString("hh:mm:ss.zzz"));

    int integer(4);
    temporary = QVariant::fromValue(integer).toDateTime();
    QTest::newRow("formatDate, int") << "formatDate" << QVariant::fromValue(integer) << (QStringList() << temporary.date().toString(Qt::DefaultLocaleShortDate) << temporary.date().toString(Qt::DefaultLocaleLongDate) << temporary.date().toString("ddd MMMM d yy"));
    QTest::newRow("formatDateTime, int") << "formatDateTime" << QVariant::fromValue(integer) << (QStringList() << temporary.toString(Qt::DefaultLocaleShortDate) << temporary.toString(Qt::DefaultLocaleLongDate) << temporary.toString("M/d/yy H:m:s a"));
    QTest::newRow("formatTime, int") << "formatTime" << QVariant::fromValue(integer) << (QStringList() << temporary.time().toString(Qt::DefaultLocaleShortDate) << temporary.time().toString(Qt::DefaultLocaleLongDate) << temporary.time().toString("H:m:s a") << temporary.time().toString("hh:mm:ss.zzz"));
}

void tst_qdeclarativeqt::isQtObject()
{
    QDeclarativeComponent component(&engine, TEST_FILE("isQtObject.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test1").toBool(), true);
    QCOMPARE(object->property("test2").toBool(), false);
    QCOMPARE(object->property("test3").toBool(), false);
    QCOMPARE(object->property("test4").toBool(), false);
    QCOMPARE(object->property("test5").toBool(), false);

    delete object;
}

void tst_qdeclarativeqt::btoa()
{
    QDeclarativeComponent component(&engine, TEST_FILE("btoa.qml"));

    QString warning1 = component.url().toString() + ":4: Error: Qt.btoa(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test2").toString(), QString("SGVsbG8gd29ybGQh"));

    delete object;
}

void tst_qdeclarativeqt::atob()
{
    QDeclarativeComponent component(&engine, TEST_FILE("atob.qml"));

    QString warning1 = component.url().toString() + ":4: Error: Qt.atob(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("test2").toString(), QString("Hello world!"));

    delete object;
}

void tst_qdeclarativeqt::fontFamilies()
{
    QDeclarativeComponent component(&engine, TEST_FILE("fontFamilies.qml"));

    QString warning1 = component.url().toString() + ":4: Error: Qt.fontFamilies(): Invalid arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QFontDatabase database;
    QCOMPARE(object->property("test2"), QVariant::fromValue(database.families()));

    delete object;
}

void tst_qdeclarativeqt::quit()
{
    QDeclarativeComponent component(&engine, TEST_FILE("quit.qml"));

    QSignalSpy spy(&engine, SIGNAL(quit()));
    QObject *object = component.create();
    QVERIFY(object != 0);
    QCOMPARE(spy.count(), 1);

    delete object;
}

void tst_qdeclarativeqt::resolvedUrl()
{
    QDeclarativeComponent component(&engine, TEST_FILE("resolvedUrl.qml"));

    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("result").toString(), component.url().toString());
    QCOMPARE(object->property("isString").toBool(), true);

    delete object;
}

QTEST_MAIN(tst_qdeclarativeqt)

#include "tst_qdeclarativeqt.moc"
