// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qtimer.h>
#include <QtTest/qtest.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQml/qqmlcomponent.h>

#include <private/qhooks_p.h>

const char testData[] =
"import QtQuick 2.0\n"
"Item {\n"
"    id: item\n"
"    property int a: 0\n"
"    Timer {\n"
"        id: timer;  interval: 1; repeat: true; running: true\n"
"        onTriggered: {\n"
"        a++\n"
"        }\n"
"    }\n"
"}\n";

struct ResolvedHooks
{
    quintptr version;
    quintptr numEntries;
    const char **qt_qmlDebugMessageBuffer;
    int *qt_qmlDebugMessageLength;
    bool (*qt_qmlDebugSendDataToService)(const char *serviceName, const char *hexData);
    bool (*qt_qmlDebugEnableService)(const char *data);
    bool (*qt_qmlDebugDisableService)(const char *data);
    void (*qt_qmlDebugObjectAvailable)();
} *hooks;

class Application : public QGuiApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv)
        : QGuiApplication(argc, argv),
          component(&engine)
    {
        component.setData(testData, QUrl("MyStuff"));
        mainObject = component.create();
    }

private slots:
    void testEcho()
    {
        QJsonObject request;
        QJsonObject arguments;
        arguments.insert(QLatin1String("test"), QLatin1String("BUH"));
        request.insert(QLatin1String("command"), QLatin1String("echo"));
        request.insert(QLatin1String("arguments"), arguments);
        QJsonDocument doc;
        doc.setObject(request);
        QByteArray hexdata = doc.toJson(QJsonDocument::Compact).toHex();

        hooks = (ResolvedHooks *)qtHookData[QHooks::Startup];
        QCOMPARE(bool(hooks), true); // Available after connector start only.
        QCOMPARE(hooks->version, quintptr(1));
        QCOMPARE(hooks->numEntries, quintptr(6));
        QCOMPARE(bool(hooks->qt_qmlDebugSendDataToService), true);
        QCOMPARE(bool(hooks->qt_qmlDebugMessageBuffer), true);
        QCOMPARE(bool(hooks->qt_qmlDebugMessageLength), true);
        QCOMPARE(bool(hooks->qt_qmlDebugEnableService), true);

        hooks->qt_qmlDebugEnableService("NativeQmlDebugger");
        hooks->qt_qmlDebugSendDataToService("NativeQmlDebugger", hexdata);
        QByteArray response(*hooks->qt_qmlDebugMessageBuffer, *hooks->qt_qmlDebugMessageLength);

        QCOMPARE(response, QByteArray("NativeQmlDebugger 25 {\"result\":{\"test\":\"BUH\"}}"));
    }

private:
    QQmlApplicationEngine engine;
    QQmlComponent component;
    QObject *mainObject;
};

int main(int argc, char *argv[])
{
    char **argv2 = new char *[argc + 2];
    for (int i = 0; i < argc; ++i)
        argv2[i] = argv[i];
    argv2[argc] = qstrdup("-qmljsdebugger=native,services:NativeQmlDebugger");
    ++argc;
    argv2[argc] = nullptr;
    Application app(argc, argv2);
    return QTest::qExec(&app, argc, argv);
}

#include "tst_qqmlnativeconnector.moc"
