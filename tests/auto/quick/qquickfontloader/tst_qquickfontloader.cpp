// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/private/qquickfontloader_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/testhttpserver_p.h>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>

class tst_qquickfontloader : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickfontloader();

private slots:
    void initTestCase() override;
    void noFont();
    void localFont();
    void failLocalFont();
    void webFont();
    void redirWebFont();
    void failWebFont();
    void changeFont();
    void changeFontSourceViaState();

private:
    QQmlEngine engine;
    TestHTTPServer server;
};

tst_qquickfontloader::tst_qquickfontloader()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickfontloader::initTestCase()
{
    QQmlDataTest::initTestCase();
    server.serveDirectory(dataDirectory());
    QVERIFY2(server.listen(), qPrintable(server.errorString()));
}

void tst_qquickfontloader::noFont()
{
    QString componentStr = "import QtQuick 2.0\nFontLoader { }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != nullptr);
    QCOMPARE(fontObject->name(), QString(""));
    QCOMPARE(fontObject->source(), QUrl(""));
    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Null);

    delete fontObject;
}

void tst_qquickfontloader::localFont()
{
    QString componentStr = "import QtQuick 2.0\nFontLoader { source: \"" + testFileUrl("tarzeau_ocr_a.ttf").toString() + "\" }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != nullptr);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));
    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Ready);
}

void tst_qquickfontloader::failLocalFont()
{
    QString componentStr = "import QtQuick 2.0\nFontLoader { source: \"" + testFileUrl("dummy.ttf").toString() + "\" }";
    QTest::ignoreMessage(QtWarningMsg, QString("<Unknown File>:2:1: QML FontLoader: Cannot load font: \"" + testFileUrl("dummy.ttf").toString() + QLatin1Char('"')).toUtf8().constData());
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != nullptr);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString(""));
    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Error);
}

void tst_qquickfontloader::webFont()
{
    QString componentStr = "import QtQuick 2.0\nFontLoader { source: \"" + server.baseUrl().toString() + "/tarzeau_ocr_a.ttf\" }";
    QQmlComponent component(&engine);

    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != nullptr);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));
    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Ready);
}

void tst_qquickfontloader::redirWebFont()
{
    server.addRedirect("olddir/oldname.ttf","../tarzeau_ocr_a.ttf");

    QString componentStr = "import QtQuick 2.0\nFontLoader { source: \"" + server.baseUrl().toString() + "/olddir/oldname.ttf\" }";
    QQmlComponent component(&engine);

    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != nullptr);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));
    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Ready);
}

void tst_qquickfontloader::failWebFont()
{
    QString componentStr = "import QtQuick 2.0\nFontLoader { source: \"" + server.baseUrl().toString() + "/nonexist.ttf\" }";
    const QString expectedError = "<Unknown File>:2:1: QML FontLoader: Cannot load font: \"" + server.baseUrl().toString() + "/nonexist.ttf\"";
    QTest::ignoreMessage(QtWarningMsg, expectedError.toUtf8());
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != nullptr);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString(""));
    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Error);
}

void tst_qquickfontloader::changeFont()
{
    QString componentStr = "import QtQuick 2.0\nFontLoader { source: fnt }";
    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("fnt", testFileUrl("tarzeau_ocr_a.ttf"));
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != nullptr);

    QSignalSpy nameSpy(fontObject, SIGNAL(nameChanged()));
    QSignalSpy statusSpy(fontObject, SIGNAL(statusChanged()));

    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Ready);
    QCOMPARE(nameSpy.size(), 0);
    QCOMPARE(statusSpy.size(), 0);
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));

    ctxt->setContextProperty("fnt", server.urlString("/daniel.ttf"));
    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Loading);
    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Ready);
    QCOMPARE(nameSpy.size(), 1);
    QCOMPARE(statusSpy.size(), 2);
    QTRY_COMPARE(fontObject->name(), QString("Daniel"));

    ctxt->setContextProperty("fnt", testFileUrl("tarzeau_ocr_a.ttf"));
    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Ready);
    QCOMPARE(nameSpy.size(), 2);
    QCOMPARE(statusSpy.size(), 2);
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));

    ctxt->setContextProperty("fnt", server.urlString("/daniel.ttf"));
    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Ready);
    QCOMPARE(nameSpy.size(), 3);
    QCOMPARE(statusSpy.size(), 2);
    QTRY_COMPARE(fontObject->name(), QString("Daniel"));
}

void tst_qquickfontloader::changeFontSourceViaState()
{
    QQuickView window(testFileUrl("qtbug-20268.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(qvariant_cast<QObject *>(window.rootObject()->property("fontloader")));
    QVERIFY(fontObject != nullptr);
    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Ready);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));

    window.rootObject()->setProperty("useotherfont", true);

    QTRY_COMPARE(fontObject->status(), QQuickFontLoader::Ready);
    QCOMPARE(window.rootObject()->property("name").toString(), QString("Daniel"));
}

QTEST_MAIN(tst_qquickfontloader)

#include "tst_qquickfontloader.moc"
