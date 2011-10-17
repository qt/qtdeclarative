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
#include <qtest.h>
#include <QTextDocument>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>

#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qsgview.h>
#include <private/qsgimage_p.h>
#include <private/qsgimagebase_p.h>
#include <private/qsgloader_p.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtTest/QSignalSpy>
#include <QtGui/QPainter>
#include <QtGui/QImageReader>

#include "../shared/util.h"
#include "../../../shared/util.h"
#include "../shared/testhttpserver.h"

#define SERVER_PORT 14451
#define SERVER_ADDR "http://127.0.0.1:14451"

class tst_qsgimage : public QObject
{
    Q_OBJECT
public:
    tst_qsgimage();

private slots:
    void noSource();
    void imageSource();
    void imageSource_data();
    void clearSource();
    void resized();
    void preserveAspectRatio();
    void smooth();
    void mirror();
    void svg();
    void geometry();
    void geometry_data();
    void big();
    void tiling_QTBUG_6716();
    void tiling_QTBUG_6716_data();
    void noLoading();
    void paintedWidthHeight();
    void sourceSize_QTBUG_14303();
    void sourceSize_QTBUG_16389();
    void nullPixmapPaint();

private:
    template<typename T>
    T *findItem(QSGItem *parent, const QString &id, int index=-1);

    QDeclarativeEngine engine;
};

tst_qsgimage::tst_qsgimage()
{
}

void tst_qsgimage::noSource()
{
    QString componentStr = "import QtQuick 2.0\nImage { source: \"\" }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QSGImage *obj = qobject_cast<QSGImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->source(), QUrl());
    QVERIFY(obj->status() == QSGImage::Null);
    QCOMPARE(obj->width(), 0.);
    QCOMPARE(obj->height(), 0.);
    QCOMPARE(obj->fillMode(), QSGImage::Stretch);
    QCOMPARE(obj->progress(), 0.0);

    delete obj;
}

void tst_qsgimage::imageSource_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<double>("width");
    QTest::addColumn<double>("height");
    QTest::addColumn<bool>("remote");
    QTest::addColumn<bool>("async");
    QTest::addColumn<bool>("cache");
    QTest::addColumn<QString>("error");

    QTest::newRow("local") << QUrl::fromLocalFile(TESTDATA("colors.png")).toString() << 120.0 << 120.0 << false << false << true << "";
    QTest::newRow("local no cache") << QUrl::fromLocalFile(TESTDATA("colors.png")).toString() << 120.0 << 120.0 << false << false << false << "";
    QTest::newRow("local async") << QUrl::fromLocalFile(TESTDATA("colors1.png")).toString() << 120.0 << 120.0 << false << true << true << "";
    QTest::newRow("local not found") << QUrl::fromLocalFile(TESTDATA("no-such-file.png")).toString() << 0.0 << 0.0 << false
        << false << true << "file::2:1: QML Image: Cannot open: " + QUrl::fromLocalFile(TESTDATA("no-such-file.png")).toString();
    QTest::newRow("local async not found") << QUrl::fromLocalFile(TESTDATA("no-such-file-1.png")).toString() << 0.0 << 0.0 << false
        << true << true << "file::2:1: QML Image: Cannot open: " + QUrl::fromLocalFile(TESTDATA("no-such-file-1.png")).toString();
    QTest::newRow("remote") << SERVER_ADDR "/colors.png" << 120.0 << 120.0 << true << false << true << "";
    QTest::newRow("remote redirected") << SERVER_ADDR "/oldcolors.png" << 120.0 << 120.0 << true << false << false << "";
    if (QImageReader::supportedImageFormats().contains("svg"))
        QTest::newRow("remote svg") << SERVER_ADDR "/heart.svg" << 550.0 << 500.0 << true << false << false << "";

    QTest::newRow("remote not found") << SERVER_ADDR "/no-such-file.png" << 0.0 << 0.0 << true
        << false << true << "file::2:1: QML Image: Error downloading " SERVER_ADDR "/no-such-file.png - server replied: Not found";

}

void tst_qsgimage::imageSource()
{
    QFETCH(QString, source);
    QFETCH(double, width);
    QFETCH(double, height);
    QFETCH(bool, remote);
    QFETCH(bool, async);
    QFETCH(bool, cache);
    QFETCH(QString, error);

    TestHTTPServer server(SERVER_PORT);
    if (remote) {
        QVERIFY(server.isValid());
        server.serveDirectory(TESTDATA(""));
        server.addRedirect("oldcolors.png", SERVER_ADDR "/colors.png");
    }

    if (!error.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, error.toUtf8());

    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + source + "\"; asynchronous: "
        + (async ? QLatin1String("true") : QLatin1String("false")) + "; cache: "
        + (cache ? QLatin1String("true") : QLatin1String("false")) + " }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QSGImage *obj = qobject_cast<QSGImage*>(component.create());
    QVERIFY(obj != 0);

    if (async)
        QVERIFY(obj->asynchronous() == true);
    else
        QVERIFY(obj->asynchronous() == false);

    if (cache)
        QVERIFY(obj->cache() == true);
    else
        QVERIFY(obj->cache() == false);

    if (remote || async)
        QTRY_VERIFY(obj->status() == QSGImage::Loading);

    QCOMPARE(obj->source(), remote ? source : QUrl(source));

    if (error.isEmpty()) {
        QTRY_VERIFY(obj->status() == QSGImage::Ready);
        QCOMPARE(obj->width(), qreal(width));
        QCOMPARE(obj->height(), qreal(height));
        QCOMPARE(obj->fillMode(), QSGImage::Stretch);
        QCOMPARE(obj->progress(), 1.0);
    } else {
        QTRY_VERIFY(obj->status() == QSGImage::Error);
    }

    delete obj;
}

void tst_qsgimage::clearSource()
{
    QString componentStr = "import QtQuick 2.0\nImage { source: srcImage }";
    QDeclarativeContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("srcImage", QUrl::fromLocalFile(TESTDATA("colors.png")));
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QSGImage *obj = qobject_cast<QSGImage*>(component.create());
    QVERIFY(obj != 0);
    QVERIFY(obj->status() == QSGImage::Ready);
    QCOMPARE(obj->width(), 120.);
    QCOMPARE(obj->height(), 120.);
    QCOMPARE(obj->progress(), 1.0);

    ctxt->setContextProperty("srcImage", "");
    QVERIFY(obj->source().isEmpty());
    QVERIFY(obj->status() == QSGImage::Null);
    QCOMPARE(obj->width(), 0.);
    QCOMPARE(obj->height(), 0.);
    QCOMPARE(obj->progress(), 0.0);

    delete obj;
}

void tst_qsgimage::resized()
{
    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + TESTDATA("colors.png") + "\"; width: 300; height: 300 }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QSGImage *obj = qobject_cast<QSGImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->width(), 300.);
    QCOMPARE(obj->height(), 300.);
    QCOMPARE(obj->fillMode(), QSGImage::Stretch);
    delete obj;
}


void tst_qsgimage::preserveAspectRatio()
{
    QSGView *canvas = new QSGView(0);
    canvas->show();

    canvas->setSource(QUrl::fromLocalFile(TESTDATA("aspectratio.qml")));
    QSGImage *image = qobject_cast<QSGImage*>(canvas->rootObject());
    QVERIFY(image != 0);
    image->setWidth(80.0);
    QCOMPARE(image->width(), 80.);
    QCOMPARE(image->height(), 80.);

    canvas->setSource(QUrl::fromLocalFile(TESTDATA("aspectratio.qml")));
    image = qobject_cast<QSGImage*>(canvas->rootObject());
    image->setHeight(60.0);
    QVERIFY(image != 0);
    QCOMPARE(image->height(), 60.);
    QCOMPARE(image->width(), 60.);
    delete canvas;
}

void tst_qsgimage::smooth()
{
    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + TESTDATA("colors.png") + "\"; smooth: true; width: 300; height: 300 }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QSGImage *obj = qobject_cast<QSGImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->width(), 300.);
    QCOMPARE(obj->height(), 300.);
    QCOMPARE(obj->smooth(), true);
    QCOMPARE(obj->fillMode(), QSGImage::Stretch);

    delete obj;
}

void tst_qsgimage::mirror()
{
    QMap<QSGImage::FillMode, QImage> screenshots;
    QList<QSGImage::FillMode> fillModes;
    fillModes << QSGImage::Stretch << QSGImage::PreserveAspectFit << QSGImage::PreserveAspectCrop
              << QSGImage::Tile << QSGImage::TileVertically << QSGImage::TileHorizontally;

    qreal width = 300;
    qreal height = 250;

    foreach (QSGImage::FillMode fillMode, fillModes) {
        QSGView *canvas = new QSGView;
        canvas->setSource(QUrl::fromLocalFile(TESTDATA("mirror.qml")));

        QSGImage *obj = canvas->rootObject()->findChild<QSGImage*>("image");
        QVERIFY(obj != 0);

        obj->setFillMode(fillMode);
        obj->setProperty("mirror", true);
        canvas->show();

        QImage screenshot = canvas->grabFrameBuffer();
        screenshots[fillMode] = screenshot;
        delete canvas;
    }

    foreach (QSGImage::FillMode fillMode, fillModes) {
        QPixmap srcPixmap;
        QVERIFY(srcPixmap.load(TESTDATA("pattern.png")));

        QPixmap expected(width, height);
        expected.fill();
        QPainter p_e(&expected);
        QTransform transform;
        transform.translate(width, 0).scale(-1, 1.0);
        p_e.setTransform(transform);

        switch (fillMode) {
        case QSGImage::Stretch:
            p_e.drawPixmap(QRect(0, 0, width, height), srcPixmap, QRect(0, 0, srcPixmap.width(), srcPixmap.height()));
            break;
        case QSGImage::PreserveAspectFit:
            p_e.drawPixmap(QRect(25, 0, height, height), srcPixmap, QRect(0, 0, srcPixmap.width(), srcPixmap.height()));
            break;
        case QSGImage::PreserveAspectCrop:
        {
            qreal ratio = width/srcPixmap.width(); // width is the longer side
            QRect rect(0, 0, srcPixmap.width()*ratio, srcPixmap.height()*ratio);
            rect.moveCenter(QRect(0, 0, width, height).center());
            p_e.drawPixmap(rect, srcPixmap, QRect(0, 0, srcPixmap.width(), srcPixmap.height()));
            break;
        }
        case QSGImage::Tile:
            p_e.drawTiledPixmap(QRect(0, 0, width, height), srcPixmap);
            break;
        case QSGImage::TileVertically:
            transform.scale(width / srcPixmap.width(), 1.0);
            p_e.setTransform(transform);
            p_e.drawTiledPixmap(QRect(0, 0, width, height), srcPixmap);
            break;
        case QSGImage::TileHorizontally:
            transform.scale(1.0, height / srcPixmap.height());
            p_e.setTransform(transform);
            p_e.drawTiledPixmap(QRect(0, 0, width, height), srcPixmap);
            break;
        }

        QImage img = expected.toImage();
#ifdef Q_WS_QPA
        QEXPECT_FAIL("", "QTBUG-21005 fails", Continue);
#endif
        QCOMPARE(screenshots[fillMode], img);
    }
}

void tst_qsgimage::svg()
{
    if (!QImageReader::supportedImageFormats().contains("svg"))
        QSKIP("svg support not available", SkipAll);

    QString src = QUrl::fromLocalFile(TESTDATA("heart.svg")).toString();
    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + src + "\"; sourceSize.width: 300; sourceSize.height: 300 }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QSGImage *obj = qobject_cast<QSGImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->width(), 300.0);
    QCOMPARE(obj->height(), 300.0);
    obj->setSourceSize(QSize(200,200));

    QCOMPARE(obj->width(), 200.0);
    QCOMPARE(obj->height(), 200.0);
    delete obj;
}

void tst_qsgimage::geometry_data()
{
    QTest::addColumn<QString>("fillMode");
    QTest::addColumn<bool>("explicitWidth");
    QTest::addColumn<bool>("explicitHeight");
    QTest::addColumn<double>("itemWidth");
    QTest::addColumn<double>("paintedWidth");
    QTest::addColumn<double>("boundingWidth");
    QTest::addColumn<double>("itemHeight");
    QTest::addColumn<double>("paintedHeight");
    QTest::addColumn<double>("boundingHeight");

    // tested image has width 200, height 100

    // bounding rect and item rect are equal with fillMode PreserveAspectFit, painted rect may be smaller if the aspect ratio doesn't match
    QTest::newRow("PreserveAspectFit") << "PreserveAspectFit" << false << false << 200.0 << 200.0 << 200.0 << 100.0 << 100.0 << 100.0;
    QTest::newRow("PreserveAspectFit explicit width 300") << "PreserveAspectFit" << true << false << 300.0 << 200.0 << 300.0 << 100.0 << 100.0 << 100.0;
    QTest::newRow("PreserveAspectFit explicit height 400") << "PreserveAspectFit" << false << true << 200.0 << 200.0 << 200.0 << 400.0 << 100.0 << 400.0;
    QTest::newRow("PreserveAspectFit explicit width 300, height 400") << "PreserveAspectFit" << true << true << 300.0 << 300.0 << 300.0 << 400.0 << 150.0 << 400.0;

    // bounding rect and painted rect are equal with fillMode PreserveAspectCrop, item rect may be smaller if the aspect ratio doesn't match
    QTest::newRow("PreserveAspectCrop") << "PreserveAspectCrop" << false << false << 200.0 << 200.0 << 200.0 << 100.0 << 100.0 << 100.0;
    QTest::newRow("PreserveAspectCrop explicit width 300") << "PreserveAspectCrop" << true << false << 300.0 << 300.0 << 300.0 << 100.0 << 150.0 << 150.0;
    QTest::newRow("PreserveAspectCrop explicit height 400") << "PreserveAspectCrop" << false << true << 200.0 << 800.0 << 800.0 << 400.0 << 400.0 << 400.0;
    QTest::newRow("PreserveAspectCrop explicit width 300, height 400") << "PreserveAspectCrop" << true << true << 300.0 << 800.0 << 800.0 << 400.0 << 400.0 << 400.0;

    // bounding rect, painted rect and item rect are equal in stretching and tiling images
    QStringList fillModes;
    fillModes << "Stretch" << "Tile" << "TileVertically" << "TileHorizontally";
    foreach (QString fillMode, fillModes) {
        QTest::newRow(fillMode.toLatin1()) << fillMode << false << false << 200.0 << 200.0 << 200.0 << 100.0 << 100.0 << 100.0;
        QTest::newRow(QString(fillMode + " explicit width 300").toLatin1()) << fillMode << true << false << 300.0 << 300.0 << 300.0 << 100.0 << 100.0 << 100.0;
        QTest::newRow(QString(fillMode + " explicit height 400").toLatin1()) << fillMode << false << true << 200.0 << 200.0 << 200.0 << 400.0 << 400.0 << 400.0;
        QTest::newRow(QString(fillMode + " explicit width 300, height 400").toLatin1()) << fillMode << true << true << 300.0 << 300.0 << 300.0 << 400.0 << 400.0 << 400.0;
    }
}

void tst_qsgimage::geometry()
{
    QFETCH(QString, fillMode);
    QFETCH(bool, explicitWidth);
    QFETCH(bool, explicitHeight);
    QFETCH(double, itemWidth);
    QFETCH(double, itemHeight);
    QFETCH(double, paintedWidth);
    QFETCH(double, paintedHeight);
    QFETCH(double, boundingWidth);
    QFETCH(double, boundingHeight);

    QString src = QUrl::fromLocalFile(TESTDATA("rect.png")).toString();
    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + src + "\"; fillMode: Image." + fillMode + "; ";

    if (explicitWidth)
        componentStr.append("width: 300; ");
    if (explicitHeight)
        componentStr.append("height: 400; ");
    componentStr.append("}");
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QSGImage *obj = qobject_cast<QSGImage*>(component.create());
    QVERIFY(obj != 0);

    QCOMPARE(obj->width(), itemWidth);
    QCOMPARE(obj->paintedWidth(), paintedWidth);
    QCOMPARE(obj->boundingRect().width(), boundingWidth);

    QCOMPARE(obj->height(), itemHeight);
    QCOMPARE(obj->paintedHeight(), paintedHeight);
    QCOMPARE(obj->boundingRect().height(), boundingHeight);
    delete obj;
}

void tst_qsgimage::big()
{
    // If the JPEG loader does not implement scaling efficiently, it would
    // have to build a 400 MB image. That would be a bug in the JPEG loader.

    QString src = QUrl::fromLocalFile(TESTDATA("big.jpeg")).toString();
    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + src + "\"; width: 100; sourceSize.height: 256 }";

    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QSGImage *obj = qobject_cast<QSGImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->width(), 100.0);
    QCOMPARE(obj->height(), 256.0);

    delete obj;
}

void tst_qsgimage::tiling_QTBUG_6716()
{
    QFETCH(QString, source);

    QSGView *canvas = new QSGView(0);
    canvas->setSource(QUrl::fromLocalFile(TESTDATA(source)));
    canvas->show();
    qApp->processEvents();

    QSGImage *tiling = findItem<QSGImage>(canvas->rootObject(), "tiling");

    QVERIFY(tiling != 0);
    QImage img = canvas->grabFrameBuffer();
    for (int x = 0; x < tiling->width(); ++x) {
        for (int y = 0; y < tiling->height(); ++y) {
#ifdef Q_WS_QPA
            QEXPECT_FAIL("", "QTBUG-21005 fails", Abort);
#endif
            QVERIFY(img.pixel(x, y) == qRgb(0, 255, 0));
        }
    }
    delete canvas;
}

void tst_qsgimage::tiling_QTBUG_6716_data()
{
    QTest::addColumn<QString>("source");
#ifdef QT_BUILD_INTERNAL // QTBUG-21688 - unstable test on developer build
    QTest::newRow("vertical_tiling") << "vtiling.qml";
#endif
    QTest::newRow("horizontal_tiling") << "htiling.qml";
}

void tst_qsgimage::noLoading()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(TESTDATA(""));
    server.addRedirect("oldcolors.png", SERVER_ADDR "/colors.png");

    QString componentStr = "import QtQuick 2.0\nImage { source: srcImage; cache: true }";
    QDeclarativeContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("srcImage", QUrl::fromLocalFile(TESTDATA("heart.png")));
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QSGImage *obj = qobject_cast<QSGImage*>(component.create());
    QVERIFY(obj != 0);
    QVERIFY(obj->status() == QSGImage::Ready);

    QSignalSpy sourceSpy(obj, SIGNAL(sourceChanged(const QUrl &)));
    QSignalSpy progressSpy(obj, SIGNAL(progressChanged(qreal)));
    QSignalSpy statusSpy(obj, SIGNAL(statusChanged(QSGImageBase::Status)));

    // Loading local file
    ctxt->setContextProperty("srcImage", QUrl::fromLocalFile(TESTDATA("green.png")));
    QTRY_VERIFY(obj->status() == QSGImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);
    QTRY_COMPARE(sourceSpy.count(), 1);
    QTRY_COMPARE(progressSpy.count(), 0);
    QTRY_COMPARE(statusSpy.count(), 0);

    // Loading remote file
    ctxt->setContextProperty("srcImage", QString(SERVER_ADDR) + "/rect.png");
    QTRY_VERIFY(obj->status() == QSGImage::Loading);
    QTRY_VERIFY(obj->progress() == 0.0);
    QTRY_VERIFY(obj->status() == QSGImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);
    QTRY_COMPARE(sourceSpy.count(), 2);
    QTRY_COMPARE(progressSpy.count(), 2);
    QTRY_COMPARE(statusSpy.count(), 2);

    // Loading remote file again - should not go through 'Loading' state.
    ctxt->setContextProperty("srcImage", QUrl::fromLocalFile(TESTDATA("green.png")));
    ctxt->setContextProperty("srcImage", QString(SERVER_ADDR) + "/rect.png");
    QTRY_VERIFY(obj->status() == QSGImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);
    QTRY_COMPARE(sourceSpy.count(), 4);
    QTRY_COMPARE(progressSpy.count(), 2);
    QTRY_COMPARE(statusSpy.count(), 2);

    delete obj;
}

void tst_qsgimage::paintedWidthHeight()
{
    {
        QString src = QUrl::fromLocalFile(TESTDATA("heart.png")).toString();
        QString componentStr = "import QtQuick 2.0\nImage { source: \"" + src + "\"; width: 200; height: 25; fillMode: Image.PreserveAspectFit }";

        QDeclarativeComponent component(&engine);
        component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QSGImage *obj = qobject_cast<QSGImage*>(component.create());
        QVERIFY(obj != 0);
        QCOMPARE(obj->width(), 200.0);
        QCOMPARE(obj->height(), 25.0);
        QCOMPARE(obj->paintedWidth(), 25.0);
        QCOMPARE(obj->paintedHeight(), 25.0);

        delete obj;
    }

    {
        QString src = QUrl::fromLocalFile(TESTDATA("heart.png")).toString();
        QString componentStr = "import QtQuick 2.0\nImage { source: \"" + src + "\"; width: 26; height: 175; fillMode: Image.PreserveAspectFit }";
        QDeclarativeComponent component(&engine);
        component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QSGImage *obj = qobject_cast<QSGImage*>(component.create());
        QVERIFY(obj != 0);
        QCOMPARE(obj->width(), 26.0);
        QCOMPARE(obj->height(), 175.0);
        QCOMPARE(obj->paintedWidth(), 26.0);
        QCOMPARE(obj->paintedHeight(), 26.0);

        delete obj;
    }
}

void tst_qsgimage::sourceSize_QTBUG_14303()
{
    QString componentStr = "import QtQuick 2.0\nImage { source: srcImage }";
    QDeclarativeContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("srcImage", QUrl::fromLocalFile(TESTDATA("heart200.png")));
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QSGImage *obj = qobject_cast<QSGImage*>(component.create());

    QSignalSpy sourceSizeSpy(obj, SIGNAL(sourceSizeChanged()));

    QTRY_VERIFY(obj != 0);
    QTRY_VERIFY(obj->status() == QSGImage::Ready);

    QTRY_COMPARE(obj->sourceSize().width(), 200);
    QTRY_COMPARE(obj->sourceSize().height(), 200);
    QTRY_COMPARE(sourceSizeSpy.count(), 0);

    ctxt->setContextProperty("srcImage", QUrl::fromLocalFile(TESTDATA("colors.png")));
    QTRY_COMPARE(obj->sourceSize().width(), 120);
    QTRY_COMPARE(obj->sourceSize().height(), 120);
    QTRY_COMPARE(sourceSizeSpy.count(), 1);

    ctxt->setContextProperty("srcImage", QUrl::fromLocalFile(TESTDATA("heart200.png")));
    QTRY_COMPARE(obj->sourceSize().width(), 200);
    QTRY_COMPARE(obj->sourceSize().height(), 200);
    QTRY_COMPARE(sourceSizeSpy.count(), 2);

    delete obj;
}

void tst_qsgimage::sourceSize_QTBUG_16389()
{
    QSGView *canvas = new QSGView(0);
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("qtbug_16389.qml")));
    canvas->show();
    qApp->processEvents();

    QSGImage *image = findItem<QSGImage>(canvas->rootObject(), "iconImage");
    QSGItem *handle = findItem<QSGItem>(canvas->rootObject(), "blueHandle");

    QCOMPARE(image->sourceSize().width(), 200);
    QCOMPARE(image->sourceSize().height(), 200);
    QCOMPARE(image->paintedWidth(), 0.0);
    QCOMPARE(image->paintedHeight(), 0.0);

    handle->setY(20);

    QCOMPARE(image->sourceSize().width(), 200);
    QCOMPARE(image->sourceSize().height(), 200);
    QCOMPARE(image->paintedWidth(), 20.0);
    QCOMPARE(image->paintedHeight(), 20.0);
}

static int numberOfWarnings = 0;
static void checkWarnings(QtMsgType, const char *msg)
{
    if (!QString(msg).contains("QGLContext::makeCurrent(): Failed."))
        numberOfWarnings++;
}

// QTBUG-15690
void tst_qsgimage::nullPixmapPaint()
{
    QSGView *canvas = new QSGView(0);
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("nullpixmap.qml")));
    canvas->show();

    QSGImage *image = qobject_cast<QSGImage*>(canvas->rootObject());
    QTRY_VERIFY(image != 0);
    image->setSource(SERVER_ADDR + QString("/no-such-file.png"));

    QtMsgHandler previousMsgHandler = qInstallMsgHandler(checkWarnings);

    // used to print "QTransform::translate with NaN called"
    QPixmap pm = QPixmap::fromImage(canvas->grabFrameBuffer());
    qInstallMsgHandler(previousMsgHandler);
    QVERIFY(numberOfWarnings == 0);
    delete image;
}

/*
   Find an item with the specified objectName.  If index is supplied then the
   item must also evaluate the {index} expression equal to index
*/
template<typename T>
T *tst_qsgimage::findItem(QSGItem *parent, const QString &objectName, int index)
{
    const QMetaObject &mo = T::staticMetaObject;
    //qDebug() << parent->childItems().count() << "children";
    for (int i = 0; i < parent->childItems().count(); ++i) {
        QSGItem *item = qobject_cast<QSGItem*>(parent->childItems().at(i));
        if (!item)
            continue;
        //qDebug() << "try" << item;
        if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName)) {
            if (index != -1) {
                QDeclarativeExpression e(qmlContext(item), item, "index");
                if (e.evaluate().toInt() == index)
                    return static_cast<T*>(item);
            } else {
                return static_cast<T*>(item);
            }
        }
        item = findItem<T>(item, objectName, index);
        if (item)
            return static_cast<T*>(item);
    }

    return 0;
}

QTEST_MAIN(tst_qsgimage)

#include "tst_qsgimage.moc"
