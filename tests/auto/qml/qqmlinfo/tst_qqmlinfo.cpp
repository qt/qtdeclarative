// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtTest/qsignalspy.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QTimer>
#include <QQmlContext>
#include <qqmlinfo.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include "attached.h"

class tst_qqmlinfo : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlinfo() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void qmlObject();
    void nestedQmlObject();
    void nestedComponent();
    void nonQmlObject();
    void nullObject();
    void nonQmlContextedObject();
    void types();
    void chaining();
    void messageTypes();
    void component();
    void attachedObject();

private:
    QQmlEngine engine;
};

void tst_qqmlinfo::qmlObject()
{
    QQmlComponent component(&engine, testFileUrl("qmlObject.qml"));

    QObject *object = component.create();
    QVERIFY(object != nullptr);

    QString message = component.url().toString() + ":3:1: QML QtObject: Test Message";
    QTest::ignoreMessage(QtInfoMsg, qPrintable(message));
    qmlInfo(object) << "Test Message";

    QObject *nested = qvariant_cast<QObject *>(object->property("nested"));
    QVERIFY(nested != nullptr);

    message = component.url().toString() + ":6:13: QML QtObject: Second Test Message";
    QTest::ignoreMessage(QtInfoMsg, qPrintable(message));
    qmlInfo(nested) << "Second Test Message";
}

void tst_qqmlinfo::nestedQmlObject()
{
    QQmlComponent component(&engine, testFileUrl("nestedQmlObject.qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QObject *nested = qvariant_cast<QObject *>(object->property("nested"));
    QVERIFY(nested != nullptr);
    QObject *nested2 = qvariant_cast<QObject *>(object->property("nested2"));
    QVERIFY(nested2 != nullptr);

    QString message = component.url().toString() + ":5:13: QML NestedObject: Outer Object";
    QTest::ignoreMessage(QtInfoMsg, qPrintable(message));
    qmlInfo(nested) << "Outer Object";

    message = testFileUrl("NestedObject.qml").toString() + ":6:14: QML QtObject: Inner Object";
    QTest::ignoreMessage(QtInfoMsg, qPrintable(message));
    qmlInfo(nested2) << "Inner Object";
}

void tst_qqmlinfo::nestedComponent()
{
    QQmlComponent component(&engine, testFileUrl("NestedComponent.qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QObject *nested = qvariant_cast<QObject *>(object->property("nested"));
    QVERIFY(nested != nullptr);
    QObject *nested2 = qvariant_cast<QObject *>(object->property("nested2"));
    QVERIFY(nested2 != nullptr);

    QString message = component.url().toString() + ":10:9: QML NestedObject: Complex Object";
    QTest::ignoreMessage(QtInfoMsg, qPrintable(message));
    qmlInfo(nested) << "Complex Object";

    message = component.url().toString() + ":16:9: QML Image: Simple Object";
    QTest::ignoreMessage(QtInfoMsg, qPrintable(message));
    qmlInfo(nested2) << "Simple Object";
}

void tst_qqmlinfo::nonQmlObject()
{
    QObject object;
    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: QML QtObject: Test Message");
    qmlInfo(&object) << "Test Message";

    QTimer nonQmlObject;
    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: QML QTimer: Test Message");
    qmlInfo(&nonQmlObject) << "Test Message";
}

void tst_qqmlinfo::nullObject()
{
    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: Null Object Test Message");
    qmlInfo(nullptr) << "Null Object Test Message";
}

void tst_qqmlinfo::nonQmlContextedObject()
{
    QObject object;
    QQmlContext context(&engine);
    QQmlEngine::setContextForObject(&object, &context);
    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: QML QtObject: Test Message");
    qmlInfo(&object) << "Test Message";
}

void tst_qqmlinfo::types()
{
    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: false");
    qmlInfo(nullptr) << false;

    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: 1.1");
    qmlInfo(nullptr) << 1.1;

    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: 1.2");
    qmlInfo(nullptr) << 1.2f;

    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: 15");
    qmlInfo(nullptr) << 15;

    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: 'b'");
    qmlInfo(nullptr) << QChar('b');

    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: \"Qt\"");
    qmlInfo(nullptr) << QByteArray("Qt");

    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: true");
    qmlInfo(nullptr) << bool(true);

    //### do we actually want QUrl to show up in the output?
    //### why the extra space at the end?
    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: QUrl(\"http://www.qt-project.org\") ");
    qmlInfo(nullptr) << QUrl("http://www.qt-project.org");

    //### should this be quoted?
    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: hello");
    qmlInfo(nullptr) << QLatin1String("hello");

    //### should this be quoted?
    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: World");
    QString str("Hello World");
    auto ref = QStringView(str).mid(6, 5);
    qmlInfo(nullptr) << ref;

    //### should this be quoted?
    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: Quick");
    qmlInfo(nullptr) << QString ("Quick");
}

void tst_qqmlinfo::chaining()
{
    QString str("Hello World");
    auto ref = QStringView(str).mid(6, 5);
    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: false 1.1 1.2 15 hello 'b' World \"Qt\" true Quick QUrl(\"http://www.qt-project.org\") ");
    qmlInfo(nullptr) << false << ' '
               << 1.1 << ' '
               << 1.2f << ' '
               << 15 << ' '
               << QLatin1String("hello") << ' '
               << QChar('b') << ' '
               << ref << ' '
               << QByteArray("Qt") << ' '
               << bool(true) << ' '
               << QString ("Quick") << ' '
               << QUrl("http://www.qt-project.org");
}

// Ensure that messages of different types are sent with the correct QtMsgType.
void tst_qqmlinfo::messageTypes()
{
    QTest::ignoreMessage(QtDebugMsg, "<Unknown File>: debug");
    qmlDebug(nullptr) << QLatin1String("debug");

    QTest::ignoreMessage(QtInfoMsg, "<Unknown File>: info");
    qmlInfo(nullptr) << QLatin1String("info");

    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: warning");
    qmlWarning(nullptr) << QLatin1String("warning");
}

void tst_qqmlinfo::component()
{
    QQmlComponent component(&engine, testFileUrl("Component.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QQmlComponent *delegate = qobject_cast<QQmlComponent*>(object->property("delegate").value<QObject*>());
    QVERIFY(delegate);

    QString message = component.url().toString() + ":4:34: QML Component: Delegate error";
    QTest::ignoreMessage(QtInfoMsg, qPrintable(message));
    qmlInfo(delegate) << "Delegate error";
}

void tst_qqmlinfo::attachedObject()
{
    QQmlComponent component(&engine, testFileUrl("AttachedObject.qml"));

    QSignalSpy warningSpy(&engine, SIGNAL(warnings(QList<QQmlError>)));
    QVERIFY(warningSpy.isValid());

    const QString qmlBindingLoopMessage = "QML Rectangle: Binding loop detected for property \"width\"";
    const QString qmlBindingLoopMessageFull = component.url().toString() + ":8:9: " + qmlBindingLoopMessage;
    QTest::ignoreMessage(QtWarningMsg, qPrintable(qmlBindingLoopMessageFull));

    const QString cppBindingLoopMessage = "QML AttachedObject (parent or ancestor of Attached): Binding loop detected for property \"a\":\n"
        + component.url().toString() + ":5:5";
    const QString cppBindingLoopMessageFull = component.url().toString() + ":4:1: " + cppBindingLoopMessage;
    QTest::ignoreMessage(QtWarningMsg, qPrintable(cppBindingLoopMessageFull));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(object != nullptr, qPrintable(component.errorString()));
    QCOMPARE(warningSpy.size(), 2);

    // The Attached C++ type has no QML engine since it was created in C++, so we should see its parent instead.
    const auto cppWarnings = warningSpy.at(0).first().value<QList<QQmlError>>();
    QCOMPARE(cppWarnings.first().description(), cppBindingLoopMessage);

    // The QML type has a QML engine, so we should see the normal message.
    const auto qmlWarnings = warningSpy.at(1).first().value<QList<QQmlError>>();
    QCOMPARE(qmlWarnings.first().description(), qmlBindingLoopMessage);
}

QTEST_MAIN(tst_qqmlinfo)

#include "tst_qqmlinfo.moc"
