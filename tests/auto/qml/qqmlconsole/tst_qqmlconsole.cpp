// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QDebug>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QLoggingCategory>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qqmlconsole : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlconsole() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void logging();
    void categorized_logging();
    void tracing();
    void profiling();
    void testAssert();
    void exception();

private:
    QQmlEngine engine;
};

struct CustomObject {};

QDebug operator<<(QDebug dbg, const CustomObject &)
{
    return dbg << "MY OBJECT";
}

Q_DECLARE_METATYPE(CustomObject)

void tst_qqmlconsole::logging()
{
    QUrl testUrl = testFileUrl("logging.qml");

    QLoggingCategory loggingCategory("qml");
    QVERIFY(loggingCategory.isDebugEnabled());
    QVERIFY(loggingCategory.isWarningEnabled());
    QVERIFY(loggingCategory.isCriticalEnabled());

    QTest::ignoreMessage(QtDebugMsg, "console.debug");
    QTest::ignoreMessage(QtDebugMsg, "console.log");
    QTest::ignoreMessage(QtInfoMsg, "console.info");
    QTest::ignoreMessage(QtWarningMsg, "console.warn");
    QTest::ignoreMessage(QtCriticalMsg, "console.error");

    QTest::ignoreMessage(QtDebugMsg, "console.count: 1");
    QTest::ignoreMessage(QtDebugMsg, ": 1");
    QTest::ignoreMessage(QtDebugMsg, "console.count: 2");
    QTest::ignoreMessage(QtDebugMsg, ": 2");

    QTest::ignoreMessage(QtDebugMsg, "[1,2]");
    QTest::ignoreMessage(QtDebugMsg, "{\"a\":\"hello\",\"d\":1}");
    QTest::ignoreMessage(QtDebugMsg, "undefined");
    QTest::ignoreMessage(QtDebugMsg, "12");
    QTest::ignoreMessage(QtDebugMsg, "function e() { [native code] }");
    QTest::ignoreMessage(QtDebugMsg, "true");
    // Printing QML object prints out the class/type of QML object with the memory address
//    QTest::ignoreMessage(QtDebugMsg, "QtObject_QML_0(0xABCD..)");
//    QTest::ignoreMessage(QtDebugMsg, "[object Object]");
    QTest::ignoreMessage(QtDebugMsg, "1 pong! [object Object]");
    QTest::ignoreMessage(QtDebugMsg, "1 [ping,pong] [object Object] 2");
    QTest::ignoreMessage(QtDebugMsg, "[Hello,World]");
    QTest::ignoreMessage(QtDebugMsg, "QVariant(CustomObject, MY OBJECT)");
    QTest::ignoreMessage(QtDebugMsg, "[[1,2,3,[2,2,2,2],4],[5,6,7,8]]");

    QStringList stringList; stringList << QStringLiteral("Hello") << QStringLiteral("World");

    CustomObject customObject;

    QQmlComponent component(&engine, testUrl);
    QScopedPointer<QObject> object(component.createWithInitialProperties({
            {"customObject", QVariant::fromValue(customObject)},
            {"stringListProperty", stringList}
    }));
    QVERIFY(object != nullptr);
}

void tst_qqmlconsole::categorized_logging()
{
    QUrl testUrl = testFileUrl("categorized_logging.qml");
    QQmlTestMessageHandler messageHandler;
    messageHandler.setIncludeCategoriesEnabled(true);

    QLoggingCategory testCategory("qt.test");
    testCategory.setEnabled(QtDebugMsg, true);
    QVERIFY(testCategory.isDebugEnabled());
    QVERIFY(testCategory.isWarningEnabled());
    QVERIFY(testCategory.isCriticalEnabled());

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY2(object != nullptr, component.errorString().toUtf8());

    QVERIFY(messageHandler.messages().contains("qt.test: console.info"));
    QVERIFY(messageHandler.messages().contains("qt.test: console.warn"));
    QVERIFY(messageHandler.messages().contains("qt.test: console.error"));
    QVERIFY(!messageHandler.messages().contains("qt.test.warning: console.debug"));
    QVERIFY(!messageHandler.messages().contains("qt.test.warning: console.log"));
    QVERIFY(!messageHandler.messages().contains("qt.test.warning: console.info"));
    QVERIFY(messageHandler.messages().contains("qt.test.warning: console.warn"));
    QVERIFY(messageHandler.messages().contains("qt.test.warning: console.error"));

    QString emptyCategory = "default: " + QString::fromLatin1("%1:%2:%3: ").arg(testUrl.toString()).arg(20).arg(5) +
                            "QML LoggingCategory: Declaring the name of a LoggingCategory is mandatory and cannot be changed later";
    QVERIFY(messageHandler.messages().contains(emptyCategory));


    QString notChangedCategory = "default: " + QString::fromLatin1("%1:%2:%3: ").arg(testUrl.toString()).arg(9).arg(5) +
                            "QML LoggingCategory: The name of a LoggingCategory cannot be changed after the component is completed";
    QVERIFY(!messageHandler.messages().contains(notChangedCategory));
    QString changedCategory = "default: " + QString::fromLatin1("%1:%2:%3: ").arg(testUrl.toString()).arg(14).arg(5) +
                            "QML LoggingCategory: The name of a LoggingCategory cannot be changed after the component is completed";
    QVERIFY(messageHandler.messages().contains(changedCategory));


    QString notChangedDefaultLogLevel = "default: " + QString::fromLatin1("%1:%2:%3: ").arg(testUrl.toString()).arg(9).arg(5) +
                            "QML LoggingCategory: The defaultLogLevel of a LoggingCategory cannot be changed after the component is completed";
    QVERIFY(!messageHandler.messages().contains(notChangedDefaultLogLevel));
    QString changedDefaultLogLevel = "default: " + QString::fromLatin1("%1:%2:%3: ").arg(testUrl.toString()).arg(14).arg(5) +
                            "QML LoggingCategory: The defaultLogLevel of a LoggingCategory cannot be changed after the component is completed";
    QVERIFY(messageHandler.messages().contains(changedDefaultLogLevel));


    QString useEmptyCategory = "default: " + QString::fromLatin1("%1:%2: ").arg(testUrl.toString()).arg(42) +
                            "Error: A QmlLoggingCatgory was provided without a valid name";
    QVERIFY(messageHandler.messages().contains(useEmptyCategory));

    delete object;
}

void tst_qqmlconsole::tracing()
{
    QUrl testUrl = testFileUrl("tracing.qml");

    QString traceText = QString::fromLatin1("tracing (%1:%2)\n").arg(testUrl.toString()).arg(12)
            + QString::fromLatin1("expression for onCompleted (%1:%2)")
                      .arg(testUrl.toString())
                      .arg(16);

    QTest::ignoreMessage(QtDebugMsg, qPrintable(traceText));

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    delete object;
}

void tst_qqmlconsole::profiling()
{
    QUrl testUrl = testFileUrl("profiling.qml");

    // profiling()
    QTest::ignoreMessage(QtWarningMsg, "Cannot start profiling because debug service is disabled. Start with -qmljsdebugger=port:XXXXX.");
    QTest::ignoreMessage(QtWarningMsg, "Ignoring console.profileEnd(): the debug service is disabled.");

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    delete object;
}

void tst_qqmlconsole::testAssert()
{
    QUrl testUrl = testFileUrl("assert.qml");

    // assert()
    QString assert1 = "This will fail\n"
            + QString::fromLatin1("expression for onCompleted (%1:%2)")
                      .arg(testUrl.toString())
                      .arg(17);

    QString assert2 = "This will fail too\n"
            + QString::fromLatin1("assertFail (%1:%2)\n").arg(testUrl.toString()).arg(10)
            + QString::fromLatin1("expression for onCompleted (%1:%2)")
                      .arg(testUrl.toString())
                      .arg(22);

    QTest::ignoreMessage(QtCriticalMsg, qPrintable(assert1));
    QTest::ignoreMessage(QtCriticalMsg, qPrintable(assert2));

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    delete object;
}

void tst_qqmlconsole::exception()
{
    QUrl testUrl = testFileUrl("exception.qml");

    // exception()
    QString exception1 = "Exception 1\n"
            + QString::fromLatin1("expression for onCompleted (%1:%2)")
                      .arg(testUrl.toString())
                      .arg(13);

    QString exception2 = "Exception 2\n"
            + QString::fromLatin1("exceptionFail (%1:%2)\n").arg(testUrl.toString()).arg(8)
            + QString::fromLatin1("expression for onCompleted (%1:%2)")
                      .arg(testUrl.toString())
                      .arg(18);

    QTest::ignoreMessage(QtCriticalMsg, qPrintable(exception1));
    QTest::ignoreMessage(QtCriticalMsg, qPrintable(exception2));

    QQmlComponent component(&engine, testUrl);
    QObject *object = component.create();
    QVERIFY(object != nullptr);
    delete object;
}

QTEST_MAIN(tst_qqmlconsole)

#include "tst_qqmlconsole.moc"
