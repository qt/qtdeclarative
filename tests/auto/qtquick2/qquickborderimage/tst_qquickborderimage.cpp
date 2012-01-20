/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <QTextDocument>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>
#include <QGraphicsScene>
#include <QPainter>

#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <private/qquickborderimage_p.h>
#include <private/qquickimagebase_p.h>
#include <private/qquickscalegrid_p_p.h>
#include <private/qquickloader_p.h>
#include <QtQuick/qquickview.h>
#include <QtDeclarative/qdeclarativecontext.h>

#include "../../shared/testhttpserver.h"
#include "../../shared/util.h"

#define SERVER_PORT 14446
#define SERVER_ADDR "http://127.0.0.1:14446"

class tst_qquickborderimage : public QDeclarativeDataTest

{
    Q_OBJECT
public:
    tst_qquickborderimage();

private slots:
    void noSource();
    void imageSource();
    void imageSource_data();
    void clearSource();
    void resized();
    void smooth();
    void mirror();
    void tileModes();
    void sciSource();
    void sciSource_data();
    void invalidSciFile();
    void pendingRemoteRequest();
    void pendingRemoteRequest_data();

private:
    QDeclarativeEngine engine;
};

tst_qquickborderimage::tst_qquickborderimage()
{
}

void tst_qquickborderimage::noSource()
{
    QString componentStr = "import QtQuick 2.0\nBorderImage { source: \"\" }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickBorderImage *obj = qobject_cast<QQuickBorderImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->source(), QUrl());
    QCOMPARE(obj->width(), 0.);
    QCOMPARE(obj->height(), 0.);
    QCOMPARE(obj->horizontalTileMode(), QQuickBorderImage::Stretch);
    QCOMPARE(obj->verticalTileMode(), QQuickBorderImage::Stretch);

    delete obj;
}

void tst_qquickborderimage::imageSource_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("remote");
    QTest::addColumn<QString>("error");

    QTest::newRow("local") << testFileUrl("colors.png").toString() << false << "";
    QTest::newRow("local not found") << testFileUrl("no-such-file.png").toString() << false
        << "file::2:1: QML BorderImage: Cannot open: " + testFileUrl("no-such-file.png").toString();
    QTest::newRow("remote") << SERVER_ADDR "/colors.png" << true << "";
    QTest::newRow("remote not found") << SERVER_ADDR "/no-such-file.png" << true
        << "file::2:1: QML BorderImage: Error downloading " SERVER_ADDR "/no-such-file.png - server replied: Not found";
}

void tst_qquickborderimage::imageSource()
{
    QFETCH(QString, source);
    QFETCH(bool, remote);
    QFETCH(QString, error);

    TestHTTPServer *server = 0;
    if (remote) {
        server = new TestHTTPServer(SERVER_PORT);
        QVERIFY(server->isValid());
        server->serveDirectory(dataDirectory());
    }

    if (!error.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, error.toUtf8());

    QString componentStr = "import QtQuick 2.0\nBorderImage { source: \"" + source + "\" }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickBorderImage *obj = qobject_cast<QQuickBorderImage*>(component.create());
    QVERIFY(obj != 0);

    if (remote)
        QTRY_VERIFY(obj->status() == QQuickBorderImage::Loading);

    QCOMPARE(obj->source(), remote ? source : QUrl(source));

    if (error.isEmpty()) {
        QTRY_VERIFY(obj->status() == QQuickBorderImage::Ready);
        QCOMPARE(obj->width(), 120.);
        QCOMPARE(obj->height(), 120.);
        QCOMPARE(obj->sourceSize().width(), 120);
        QCOMPARE(obj->sourceSize().height(), 120);
        QCOMPARE(obj->horizontalTileMode(), QQuickBorderImage::Stretch);
        QCOMPARE(obj->verticalTileMode(), QQuickBorderImage::Stretch);
    } else {
        QTRY_VERIFY(obj->status() == QQuickBorderImage::Error);
    }

    delete obj;
    delete server;
}

void tst_qquickborderimage::clearSource()
{
    QString componentStr = "import QtQuick 2.0\nBorderImage { source: srcImage }";
    QDeclarativeContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("srcImage", testFileUrl("colors.png"));
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickBorderImage *obj = qobject_cast<QQuickBorderImage*>(component.create());
    QVERIFY(obj != 0);
    QVERIFY(obj->status() == QQuickBorderImage::Ready);
    QCOMPARE(obj->width(), 120.);
    QCOMPARE(obj->height(), 120.);

    ctxt->setContextProperty("srcImage", "");
    QVERIFY(obj->source().isEmpty());
    QVERIFY(obj->status() == QQuickBorderImage::Null);
    QCOMPARE(obj->width(), 0.);
    QCOMPARE(obj->height(), 0.);
}

void tst_qquickborderimage::resized()
{
    QString componentStr = "import QtQuick 2.0\nBorderImage { source: \"" + testFileUrl("colors.png").toString() + "\"; width: 300; height: 300 }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickBorderImage *obj = qobject_cast<QQuickBorderImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->width(), 300.);
    QCOMPARE(obj->height(), 300.);
    QCOMPARE(obj->sourceSize().width(), 120);
    QCOMPARE(obj->sourceSize().height(), 120);
    QCOMPARE(obj->horizontalTileMode(), QQuickBorderImage::Stretch);
    QCOMPARE(obj->verticalTileMode(), QQuickBorderImage::Stretch);

    delete obj;
}

void tst_qquickborderimage::smooth()
{
    QString componentStr = "import QtQuick 2.0\nBorderImage { source: \"" + testFile("colors.png") + "\"; smooth: true; width: 300; height: 300 }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickBorderImage *obj = qobject_cast<QQuickBorderImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->width(), 300.);
    QCOMPARE(obj->height(), 300.);
    QCOMPARE(obj->smooth(), true);
    QCOMPARE(obj->horizontalTileMode(), QQuickBorderImage::Stretch);
    QCOMPARE(obj->verticalTileMode(), QQuickBorderImage::Stretch);

    delete obj;
}

void tst_qquickborderimage::mirror()
{
    QQuickView *canvas = new QQuickView;
    canvas->setSource(testFileUrl("mirror.qml"));
    QQuickBorderImage *image = qobject_cast<QQuickBorderImage*>(canvas->rootObject());
    QVERIFY(image != 0);

    QImage screenshot = canvas->grabFrameBuffer();

    QImage srcPixmap(screenshot);
    QTransform transform;
    transform.translate(image->width(), 0).scale(-1, 1.0);
    srcPixmap = srcPixmap.transformed(transform);

    image->setProperty("mirror", true);
    screenshot = canvas->grabFrameBuffer();
    QCOMPARE(screenshot, srcPixmap);

    delete canvas;
}

void tst_qquickborderimage::tileModes()
{
    {
        QString componentStr = "import QtQuick 2.0\nBorderImage { source: \"" + testFile("colors.png") + "\"; width: 100; height: 300; horizontalTileMode: BorderImage.Repeat; verticalTileMode: BorderImage.Repeat }";
        QDeclarativeComponent component(&engine);
        component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickBorderImage *obj = qobject_cast<QQuickBorderImage*>(component.create());
        QVERIFY(obj != 0);
        QCOMPARE(obj->width(), 100.);
        QCOMPARE(obj->height(), 300.);
        QCOMPARE(obj->horizontalTileMode(), QQuickBorderImage::Repeat);
        QCOMPARE(obj->verticalTileMode(), QQuickBorderImage::Repeat);

        delete obj;
    }
    {
        QString componentStr = "import QtQuick 2.0\nBorderImage { source: \"" + testFile("colors.png") + "\"; width: 300; height: 150; horizontalTileMode: BorderImage.Round; verticalTileMode: BorderImage.Round }";
        QDeclarativeComponent component(&engine);
        component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickBorderImage *obj = qobject_cast<QQuickBorderImage*>(component.create());
        QVERIFY(obj != 0);
        QCOMPARE(obj->width(), 300.);
        QCOMPARE(obj->height(), 150.);
        QCOMPARE(obj->horizontalTileMode(), QQuickBorderImage::Round);
        QCOMPARE(obj->verticalTileMode(), QQuickBorderImage::Round);

        delete obj;
    }
}

void tst_qquickborderimage::sciSource()
{
    QFETCH(QString, source);
    QFETCH(bool, valid);

    bool remote = source.startsWith("http");
    TestHTTPServer *server = 0;
    if (remote) {
        server = new TestHTTPServer(SERVER_PORT);
        QVERIFY(server->isValid());
        server->serveDirectory(dataDirectory());
    }

    QString componentStr = "import QtQuick 2.0\nBorderImage { source: \"" + source + "\"; width: 300; height: 300 }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickBorderImage *obj = qobject_cast<QQuickBorderImage*>(component.create());
    QVERIFY(obj != 0);

    if (remote)
        QTRY_VERIFY(obj->status() == QQuickBorderImage::Loading);

    QCOMPARE(obj->source(), remote ? source : QUrl(source));
    QCOMPARE(obj->width(), 300.);
    QCOMPARE(obj->height(), 300.);

    if (valid) {
        QTRY_VERIFY(obj->status() == QQuickBorderImage::Ready);
        QCOMPARE(obj->border()->left(), 10);
        QCOMPARE(obj->border()->top(), 20);
        QCOMPARE(obj->border()->right(), 30);
        QCOMPARE(obj->border()->bottom(), 40);
        QCOMPARE(obj->horizontalTileMode(), QQuickBorderImage::Round);
        QCOMPARE(obj->verticalTileMode(), QQuickBorderImage::Repeat);
    } else {
        QTRY_VERIFY(obj->status() == QQuickBorderImage::Error);
    }

    delete obj;
    delete server;
}

void tst_qquickborderimage::sciSource_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("valid");

    QTest::newRow("local") << testFileUrl("colors-round.sci").toString() << true;
    QTest::newRow("local quoted filename") << testFileUrl("colors-round-quotes.sci").toString() << true;
    QTest::newRow("local not found") << testFileUrl("no-such-file.sci").toString() << false;
    QTest::newRow("remote") << SERVER_ADDR "/colors-round.sci" << true;
    QTest::newRow("remote filename quoted") << SERVER_ADDR "/colors-round-quotes.sci" << true;
    QTest::newRow("remote image") << SERVER_ADDR "/colors-round-remote.sci" << true;
    QTest::newRow("remote not found") << SERVER_ADDR "/no-such-file.sci" << false;
}

void tst_qquickborderimage::invalidSciFile()
{
    QTest::ignoreMessage(QtWarningMsg, "QQuickGridScaledImage: Invalid tile rule specified. Using Stretch."); // for "Roun"
    QTest::ignoreMessage(QtWarningMsg, "QQuickGridScaledImage: Invalid tile rule specified. Using Stretch."); // for "Repea"

    QString componentStr = "import QtQuick 2.0\nBorderImage { source: \"" + testFileUrl("invalid.sci").toString() +"\"; width: 300; height: 300 }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickBorderImage *obj = qobject_cast<QQuickBorderImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->width(), 300.);
    QCOMPARE(obj->height(), 300.);
    QCOMPARE(obj->status(), QQuickImageBase::Error);
    QCOMPARE(obj->horizontalTileMode(), QQuickBorderImage::Stretch);
    QCOMPARE(obj->verticalTileMode(), QQuickBorderImage::Stretch);

    delete obj;
}

void tst_qquickborderimage::pendingRemoteRequest()
{
    QFETCH(QString, source);

    QString componentStr = "import QtQuick 2.0\nBorderImage { source: \"" + source + "\" }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickBorderImage *obj = qobject_cast<QQuickBorderImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->status(), QQuickBorderImage::Loading);

    // verify no crash
    // This will cause a delayed "QThread: Destroyed while thread is still running" warning
    delete obj;
    QTest::qWait(50);
}

void tst_qquickborderimage::pendingRemoteRequest_data()
{
    QTest::addColumn<QString>("source");

    QTest::newRow("png file") << "http://localhost/none.png";
    QTest::newRow("sci file") << "http://localhost/none.sci";
}

QTEST_MAIN(tst_qquickborderimage)

#include "tst_qquickborderimage.moc"
