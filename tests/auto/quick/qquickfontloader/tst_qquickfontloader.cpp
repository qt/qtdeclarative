/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/private/qquickfontloader_p.h>
#include "../../shared/util.h"
#include "../../shared/testhttpserver.h"
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>

#define SERVER_PORT 14448

class tst_qquickfontloader : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickfontloader();

private slots:
    void initTestCase();
    void noFont();
    void namedFont();
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

tst_qquickfontloader::tst_qquickfontloader() :
    server(SERVER_PORT)
{
}

void tst_qquickfontloader::initTestCase()
{
    QQmlDataTest::initTestCase();
    server.serveDirectory(dataDirectory());
    QVERIFY(server.isValid());
}

void tst_qquickfontloader::noFont()
{
    QString componentStr = "import QtQuick 2.0\nFontLoader { }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != 0);
    QCOMPARE(fontObject->name(), QString(""));
    QCOMPARE(fontObject->source(), QUrl(""));
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Null);

    delete fontObject;
}

void tst_qquickfontloader::namedFont()
{
    QString componentStr = "import QtQuick 2.0\nFontLoader { name: \"Helvetica\" }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != 0);
    QCOMPARE(fontObject->source(), QUrl(""));
    QCOMPARE(fontObject->name(), QString("Helvetica"));
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Ready);
}

void tst_qquickfontloader::localFont()
{
#if defined(Q_OS_WIN)
    QSKIP("Windows doesn't support font loading.");
#endif
    QString componentStr = "import QtQuick 2.0\nFontLoader { source: \"" + testFileUrl("tarzeau_ocr_a.ttf").toString() + "\" }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != 0);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Ready);
}

void tst_qquickfontloader::failLocalFont()
{
#if defined(Q_OS_WIN)
    QSKIP("Windows doesn't support font loading.");
#endif
    QString componentStr = "import QtQuick 2.0\nFontLoader { source: \"" + testFileUrl("dummy.ttf").toString() + "\" }";
    QTest::ignoreMessage(QtWarningMsg, QString("file::2:1: QML FontLoader: Cannot load font: \"" + testFileUrl("dummy.ttf").toString() + "\"").toUtf8().constData());
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != 0);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString(""));
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Error);
}

void tst_qquickfontloader::webFont()
{
#if defined(Q_OS_WIN)
    QSKIP("Windows doesn't support font loading.");
#endif
    QString componentStr = "import QtQuick 2.0\nFontLoader { source: \"http://localhost:14448/tarzeau_ocr_a.ttf\" }";
    QQmlComponent component(&engine);

    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != 0);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Ready);
}

void tst_qquickfontloader::redirWebFont()
{
#if defined(Q_OS_WIN)
    QSKIP("Windows doesn't support font loading.");
#endif
    server.addRedirect("olddir/oldname.ttf","../tarzeau_ocr_a.ttf");

    QString componentStr = "import QtQuick 2.0\nFontLoader { source: \"http://localhost:14448/olddir/oldname.ttf\" }";
    QQmlComponent component(&engine);

    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != 0);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Ready);
}

void tst_qquickfontloader::failWebFont()
{
#if defined(Q_OS_WIN)
    QSKIP("Windows doesn't support font loading.");
#endif
    QString componentStr = "import QtQuick 2.0\nFontLoader { source: \"http://localhost:14448/nonexist.ttf\" }";
    QTest::ignoreMessage(QtWarningMsg, "file::2:1: QML FontLoader: Cannot load font: \"http://localhost:14448/nonexist.ttf\"");
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != 0);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString(""));
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Error);
}

void tst_qquickfontloader::changeFont()
{
#if defined(Q_OS_WIN)
    QSKIP("Windows doesn't support font loading.");
#endif
    QString componentStr = "import QtQuick 2.0\nFontLoader { source: font }";
    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("font", testFileUrl("tarzeau_ocr_a.ttf"));
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(component.create());

    QVERIFY(fontObject != 0);

    QSignalSpy nameSpy(fontObject, SIGNAL(nameChanged()));
    QSignalSpy statusSpy(fontObject, SIGNAL(statusChanged()));

    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Ready);
    QCOMPARE(nameSpy.count(), 0);
    QCOMPARE(statusSpy.count(), 0);
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));

    ctxt->setContextProperty("font", "http://localhost:14448/daniel.ttf");
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Loading);
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Ready);
    QCOMPARE(nameSpy.count(), 1);
    QCOMPARE(statusSpy.count(), 2);
    QTRY_COMPARE(fontObject->name(), QString("Daniel"));

    ctxt->setContextProperty("font", testFileUrl("tarzeau_ocr_a.ttf"));
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Ready);
    QCOMPARE(nameSpy.count(), 2);
    QCOMPARE(statusSpy.count(), 2);
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));

    ctxt->setContextProperty("font", "http://localhost:14448/daniel.ttf");
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Ready);
    QCOMPARE(nameSpy.count(), 3);
    QCOMPARE(statusSpy.count(), 2);
    QTRY_COMPARE(fontObject->name(), QString("Daniel"));
}

void tst_qquickfontloader::changeFontSourceViaState()
{
#if defined(Q_OS_WIN)
    QSKIP("Windows doesn't support font loading.");
#endif
    QQuickView canvas(testFileUrl("qtbug-20268.qml"));
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());

    QQuickFontLoader *fontObject = qobject_cast<QQuickFontLoader*>(qvariant_cast<QObject *>(canvas.rootObject()->property("fontloader")));
    QVERIFY(fontObject != 0);
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Ready);
    QVERIFY(fontObject->source() != QUrl(""));
    QTRY_COMPARE(fontObject->name(), QString("OCRA"));

    canvas.rootObject()->setProperty("usename", true);

    // This warning should probably not be printed once QTBUG-20268 is fixed
    QString warning = QString(testFileUrl("qtbug-20268.qml").toString()) +
                              QLatin1String(":13:5: QML FontLoader: Cannot load font: \"\"");
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));

    QEXPECT_FAIL("", "QTBUG-20268", Abort);
    QTRY_VERIFY(fontObject->status() == QQuickFontLoader::Ready);
    QCOMPARE(canvas.rootObject()->property("name").toString(), QString("Tahoma"));
}

QTEST_MAIN(tst_qquickfontloader)

#include "tst_qquickfontloader.moc"
