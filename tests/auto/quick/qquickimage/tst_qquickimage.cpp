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
#include <QTextDocument>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>
#include <private/qquickimage_p.h>
#include <private/qquickimagebase_p.h>
#include <private/qquickloader_p.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlexpression.h>
#include <QtTest/QSignalSpy>
#include <QtGui/QPainter>
#include <QtGui/QImageReader>

#include "../../shared/util.h"
#include "../../shared/testhttpserver.h"
#include "../shared/visualtestutil.h"

#define SERVER_PORT 14451
#define SERVER_ADDR "http://127.0.0.1:14451"


using namespace QQuickVisualTestUtil;

Q_DECLARE_METATYPE(QQuickImageBase::Status)

class tst_qquickimage : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickimage();

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
    void imageCrash_QTBUG_22125();
    void sourceSize_data();
    void sourceSize();

private:
    QQmlEngine engine;
};

tst_qquickimage::tst_qquickimage()
{
}

void tst_qquickimage::noSource()
{
    QString componentStr = "import QtQuick 2.0\nImage { source: \"\" }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->source(), QUrl());
    QVERIFY(obj->status() == QQuickImage::Null);
    QCOMPARE(obj->width(), 0.);
    QCOMPARE(obj->height(), 0.);
    QCOMPARE(obj->fillMode(), QQuickImage::Stretch);
    QCOMPARE(obj->progress(), 0.0);

    delete obj;
}

void tst_qquickimage::imageSource_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<double>("width");
    QTest::addColumn<double>("height");
    QTest::addColumn<bool>("remote");
    QTest::addColumn<bool>("async");
    QTest::addColumn<bool>("cache");
    QTest::addColumn<QString>("error");

    QTest::newRow("local") << testFileUrl("colors.png").toString() << 120.0 << 120.0 << false << false << true << "";
    QTest::newRow("local no cache") << testFileUrl("colors.png").toString() << 120.0 << 120.0 << false << false << false << "";
    QTest::newRow("local async") << testFileUrl("colors1.png").toString() << 120.0 << 120.0 << false << true << true << "";
    QTest::newRow("local not found") << testFileUrl("no-such-file.png").toString() << 0.0 << 0.0 << false
        << false << true << "file::2:1: QML Image: Cannot open: " + testFileUrl("no-such-file.png").toString();
    QTest::newRow("local async not found") << testFileUrl("no-such-file-1.png").toString() << 0.0 << 0.0 << false
        << true << true << "file::2:1: QML Image: Cannot open: " + testFileUrl("no-such-file-1.png").toString();
    QTest::newRow("remote") << SERVER_ADDR "/colors.png" << 120.0 << 120.0 << true << false << true << "";
    QTest::newRow("remote redirected") << SERVER_ADDR "/oldcolors.png" << 120.0 << 120.0 << true << false << false << "";
    if (QImageReader::supportedImageFormats().contains("svg"))
        QTest::newRow("remote svg") << SERVER_ADDR "/heart.svg" << 550.0 << 500.0 << true << false << false << "";

    QTest::newRow("remote not found") << SERVER_ADDR "/no-such-file.png" << 0.0 << 0.0 << true
        << false << true << "file::2:1: QML Image: Error downloading " SERVER_ADDR "/no-such-file.png - server replied: Not found";

}

void tst_qquickimage::imageSource()
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
        server.serveDirectory(dataDirectory());
        server.addRedirect("oldcolors.png", SERVER_ADDR "/colors.png");
    }

    if (!error.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, error.toUtf8());

    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + source + "\"; asynchronous: "
        + (async ? QLatin1String("true") : QLatin1String("false")) + "; cache: "
        + (cache ? QLatin1String("true") : QLatin1String("false")) + " }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
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
        QTRY_VERIFY(obj->status() == QQuickImage::Loading);

    QCOMPARE(obj->source(), remote ? source : QUrl(source));

    if (error.isEmpty()) {
        QTRY_VERIFY(obj->status() == QQuickImage::Ready);
        QCOMPARE(obj->width(), qreal(width));
        QCOMPARE(obj->height(), qreal(height));
        QCOMPARE(obj->fillMode(), QQuickImage::Stretch);
        QCOMPARE(obj->progress(), 1.0);
    } else {
        QTRY_VERIFY(obj->status() == QQuickImage::Error);
    }

    delete obj;
}

void tst_qquickimage::clearSource()
{
    QString componentStr = "import QtQuick 2.0\nImage { source: srcImage }";
    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("srcImage", testFileUrl("colors.png"));
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != 0);
    QVERIFY(obj->status() == QQuickImage::Ready);
    QCOMPARE(obj->width(), 120.);
    QCOMPARE(obj->height(), 120.);
    QCOMPARE(obj->progress(), 1.0);

    ctxt->setContextProperty("srcImage", "");
    QVERIFY(obj->source().isEmpty());
    QVERIFY(obj->status() == QQuickImage::Null);
    QCOMPARE(obj->width(), 0.);
    QCOMPARE(obj->height(), 0.);
    QCOMPARE(obj->progress(), 0.0);

    delete obj;
}

void tst_qquickimage::resized()
{
    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + testFile("colors.png") + "\"; width: 300; height: 300 }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->width(), 300.);
    QCOMPARE(obj->height(), 300.);
    QCOMPARE(obj->fillMode(), QQuickImage::Stretch);
    delete obj;
}


void tst_qquickimage::preserveAspectRatio()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->show();

    canvas->setSource(testFileUrl("aspectratio.qml"));
    QQuickImage *image = qobject_cast<QQuickImage*>(canvas->rootObject());
    QVERIFY(image != 0);
    image->setWidth(80.0);
    QCOMPARE(image->width(), 80.);
    QCOMPARE(image->height(), 80.);

    canvas->setSource(testFileUrl("aspectratio.qml"));
    image = qobject_cast<QQuickImage*>(canvas->rootObject());
    image->setHeight(60.0);
    QVERIFY(image != 0);
    QCOMPARE(image->height(), 60.);
    QCOMPARE(image->width(), 60.);
    delete canvas;
}

void tst_qquickimage::smooth()
{
    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + testFile("colors.png") + "\"; smooth: true; width: 300; height: 300 }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->width(), 300.);
    QCOMPARE(obj->height(), 300.);
    QCOMPARE(obj->smooth(), true);
    QCOMPARE(obj->fillMode(), QQuickImage::Stretch);

    delete obj;
}

void tst_qquickimage::mirror()
{
    QSKIP("Test is broken on multiple levels, will need incremental fixes");

    QMap<QQuickImage::FillMode, QImage> screenshots;
    QList<QQuickImage::FillMode> fillModes;
    fillModes << QQuickImage::Stretch << QQuickImage::PreserveAspectFit << QQuickImage::PreserveAspectCrop
              << QQuickImage::Tile << QQuickImage::TileVertically << QQuickImage::TileHorizontally;

    qreal width = 300;
    qreal height = 250;

    foreach (QQuickImage::FillMode fillMode, fillModes) {
        QQuickView *canvas = new QQuickView;
        canvas->setSource(testFileUrl("mirror.qml"));

        QQuickImage *obj = canvas->rootObject()->findChild<QQuickImage*>("image");
        QVERIFY(obj != 0);

        obj->setFillMode(fillMode);
        obj->setProperty("mirror", true);
        canvas->show();

        QImage screenshot = canvas->grabFrameBuffer();
        screenshots[fillMode] = screenshot;
        delete canvas;
    }

    foreach (QQuickImage::FillMode fillMode, fillModes) {
        QPixmap srcPixmap;
        QVERIFY(srcPixmap.load(testFile("pattern.png")));

        QPixmap expected(width, height);
        expected.fill();
        QPainter p_e(&expected);
        QTransform transform;
        transform.translate(width, 0).scale(-1, 1.0);
        p_e.setTransform(transform);

        switch (fillMode) {
        case QQuickImage::Stretch:
            p_e.drawPixmap(QRect(0, 0, width, height), srcPixmap, QRect(0, 0, srcPixmap.width(), srcPixmap.height()));
            break;
        case QQuickImage::PreserveAspectFit:
            p_e.drawPixmap(QRect(25, 0, height, height), srcPixmap, QRect(0, 0, srcPixmap.width(), srcPixmap.height()));
            break;
        case QQuickImage::PreserveAspectCrop:
        {
            qreal ratio = width/srcPixmap.width(); // width is the longer side
            QRect rect(0, 0, srcPixmap.width()*ratio, srcPixmap.height()*ratio);
            rect.moveCenter(QRect(0, 0, width, height).center());
            p_e.drawPixmap(rect, srcPixmap, QRect(0, 0, srcPixmap.width(), srcPixmap.height()));
            break;
        }
        case QQuickImage::Tile:
            p_e.drawTiledPixmap(QRect(0, 0, width, height), srcPixmap);
            break;
        case QQuickImage::TileVertically:
            transform.scale(width / srcPixmap.width(), 1.0);
            p_e.setTransform(transform);
            p_e.drawTiledPixmap(QRect(0, 0, width, height), srcPixmap);
            break;
        case QQuickImage::TileHorizontally:
            transform.scale(1.0, height / srcPixmap.height());
            p_e.setTransform(transform);
            p_e.drawTiledPixmap(QRect(0, 0, width, height), srcPixmap);
            break;
        case QQuickImage::Pad:
            break;
        }

        QImage img = expected.toImage();
        QEXPECT_FAIL("", "QTBUG-21005 fails", Continue);
        QCOMPARE(screenshots[fillMode], img);
    }
}

void tst_qquickimage::svg()
{
    if (!QImageReader::supportedImageFormats().contains("svg"))
        QSKIP("svg support not available");

    QString src = testFileUrl("heart.svg").toString();
    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + src + "\"; sourceSize.width: 300; sourceSize.height: 300 }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->width(), 300.0);
    QCOMPARE(obj->height(), 300.0);
    obj->setSourceSize(QSize(200,200));

    QCOMPARE(obj->width(), 200.0);
    QCOMPARE(obj->height(), 200.0);
    delete obj;
}

void tst_qquickimage::geometry_data()
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

void tst_qquickimage::geometry()
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

    QString src = testFileUrl("rect.png").toString();
    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + src + "\"; fillMode: Image." + fillMode + "; ";

    if (explicitWidth)
        componentStr.append("width: 300; ");
    if (explicitHeight)
        componentStr.append("height: 400; ");
    componentStr.append("}");
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != 0);

    QCOMPARE(obj->width(), itemWidth);
    QCOMPARE(obj->paintedWidth(), paintedWidth);
    QCOMPARE(obj->boundingRect().width(), boundingWidth);

    QCOMPARE(obj->height(), itemHeight);
    QCOMPARE(obj->paintedHeight(), paintedHeight);
    QCOMPARE(obj->boundingRect().height(), boundingHeight);
    delete obj;
}

void tst_qquickimage::big()
{
    // If the JPEG loader does not implement scaling efficiently, it would
    // have to build a 400 MB image. That would be a bug in the JPEG loader.

    QString src = testFileUrl("big.jpeg").toString();
    QString componentStr = "import QtQuick 2.0\nImage { source: \"" + src + "\"; width: 100; sourceSize.height: 256 }";

    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != 0);
    QCOMPARE(obj->width(), 100.0);
    QCOMPARE(obj->height(), 256.0);

    delete obj;
}

// As tiling_QTBUG_6716 doesn't complete, it doesn't delete the
// canvas which causes leak warnings.  Use this delete on stack
// destruction pattern to work around this.
template<typename T>
struct AutoDelete {
    AutoDelete(T *t) : t(t) {}
    ~AutoDelete() { delete t; }
private:
    T *t;
};

void tst_qquickimage::tiling_QTBUG_6716()
{
    QSKIP("Test is broken on multiple levels, will need incremental fixes");

    QFETCH(QString, source);

    QQuickView *canvas = new QQuickView(0);
    AutoDelete<QQuickView> del(canvas);

    canvas->setSource(testFileUrl(source));
    canvas->show();
    qApp->processEvents();

    QQuickImage *tiling = findItem<QQuickImage>(canvas->rootObject(), "tiling");

    QVERIFY(tiling != 0);
    QImage img = canvas->grabFrameBuffer();
    for (int x = 0; x < tiling->width(); ++x) {
        for (int y = 0; y < tiling->height(); ++y) {
            QVERIFY(img.pixel(x, y) == qRgb(0, 255, 0));
        }
    }

    delete canvas;
}

void tst_qquickimage::tiling_QTBUG_6716_data()
{
    QTest::addColumn<QString>("source");
    QTest::newRow("vertical_tiling") << "vtiling.qml";
    QTest::newRow("horizontal_tiling") << "htiling.qml";
}

void tst_qquickimage::noLoading()
{
    qRegisterMetaType<QQuickImageBase::Status>();

    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(dataDirectory());
    server.addRedirect("oldcolors.png", SERVER_ADDR "/colors.png");

    QString componentStr = "import QtQuick 2.0\nImage { source: srcImage; cache: true }";
    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("srcImage", testFileUrl("heart.png"));
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
    QVERIFY(obj != 0);
    QVERIFY(obj->status() == QQuickImage::Ready);

    QSignalSpy sourceSpy(obj, SIGNAL(sourceChanged(const QUrl &)));
    QSignalSpy progressSpy(obj, SIGNAL(progressChanged(qreal)));
    QSignalSpy statusSpy(obj, SIGNAL(statusChanged(QQuickImageBase::Status)));

    // Loading local file
    ctxt->setContextProperty("srcImage", testFileUrl("green.png"));
    QTRY_VERIFY(obj->status() == QQuickImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);
    QTRY_COMPARE(sourceSpy.count(), 1);
    QTRY_COMPARE(progressSpy.count(), 0);
    QTRY_COMPARE(statusSpy.count(), 0);

    // Loading remote file
    ctxt->setContextProperty("srcImage", QString(SERVER_ADDR) + "/rect.png");
    QTRY_VERIFY(obj->status() == QQuickImage::Loading);
    QTRY_VERIFY(obj->progress() == 0.0);
    QTRY_VERIFY(obj->status() == QQuickImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);
    QTRY_COMPARE(sourceSpy.count(), 2);
    QTRY_COMPARE(progressSpy.count(), 2);
    QTRY_COMPARE(statusSpy.count(), 2);

    // Loading remote file again - should not go through 'Loading' state.
    ctxt->setContextProperty("srcImage", testFileUrl("green.png"));
    ctxt->setContextProperty("srcImage", QString(SERVER_ADDR) + "/rect.png");
    QTRY_VERIFY(obj->status() == QQuickImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);
    QTRY_COMPARE(sourceSpy.count(), 4);
    QTRY_COMPARE(progressSpy.count(), 2);
    QTRY_COMPARE(statusSpy.count(), 2);

    delete obj;
}

void tst_qquickimage::paintedWidthHeight()
{
    {
        QString src = testFileUrl("heart.png").toString();
        QString componentStr = "import QtQuick 2.0\nImage { source: \"" + src + "\"; width: 200; height: 25; fillMode: Image.PreserveAspectFit }";

        QQmlComponent component(&engine);
        component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
        QVERIFY(obj != 0);
        QCOMPARE(obj->width(), 200.0);
        QCOMPARE(obj->height(), 25.0);
        QCOMPARE(obj->paintedWidth(), 25.0);
        QCOMPARE(obj->paintedHeight(), 25.0);

        delete obj;
    }

    {
        QString src = testFileUrl("heart.png").toString();
        QString componentStr = "import QtQuick 2.0\nImage { source: \"" + src + "\"; width: 26; height: 175; fillMode: Image.PreserveAspectFit }";
        QQmlComponent component(&engine);
        component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());
        QVERIFY(obj != 0);
        QCOMPARE(obj->width(), 26.0);
        QCOMPARE(obj->height(), 175.0);
        QCOMPARE(obj->paintedWidth(), 26.0);
        QCOMPARE(obj->paintedHeight(), 26.0);

        delete obj;
    }
}

void tst_qquickimage::sourceSize_QTBUG_14303()
{
    QString componentStr = "import QtQuick 2.0\nImage { source: srcImage }";
    QQmlContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("srcImage", testFileUrl("heart200.png"));
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickImage *obj = qobject_cast<QQuickImage*>(component.create());

    QSignalSpy sourceSizeSpy(obj, SIGNAL(sourceSizeChanged()));

    QTRY_VERIFY(obj != 0);
    QTRY_VERIFY(obj->status() == QQuickImage::Ready);

    QTRY_COMPARE(obj->sourceSize().width(), 200);
    QTRY_COMPARE(obj->sourceSize().height(), 200);
    QTRY_COMPARE(sourceSizeSpy.count(), 0);

    ctxt->setContextProperty("srcImage", testFileUrl("colors.png"));
    QTRY_COMPARE(obj->sourceSize().width(), 120);
    QTRY_COMPARE(obj->sourceSize().height(), 120);
    QTRY_COMPARE(sourceSizeSpy.count(), 1);

    ctxt->setContextProperty("srcImage", testFileUrl("heart200.png"));
    QTRY_COMPARE(obj->sourceSize().width(), 200);
    QTRY_COMPARE(obj->sourceSize().height(), 200);
    QTRY_COMPARE(sourceSizeSpy.count(), 2);

    delete obj;
}

void tst_qquickimage::sourceSize_QTBUG_16389()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setSource(testFileUrl("qtbug_16389.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickImage *image = findItem<QQuickImage>(canvas->rootObject(), "iconImage");
    QQuickItem *handle = findItem<QQuickItem>(canvas->rootObject(), "blueHandle");

    QCOMPARE(image->sourceSize().width(), 200);
    QCOMPARE(image->sourceSize().height(), 200);
    QCOMPARE(image->paintedWidth(), 0.0);
    QCOMPARE(image->paintedHeight(), 0.0);

    handle->setY(20);

    QCOMPARE(image->sourceSize().width(), 200);
    QCOMPARE(image->sourceSize().height(), 200);
    QCOMPARE(image->paintedWidth(), 20.0);
    QCOMPARE(image->paintedHeight(), 20.0);

    delete canvas;
}

static int numberOfWarnings = 0;
static void checkWarnings(QtMsgType, const char *msg)
{
    if (!QString(msg).contains("QGLContext::makeCurrent(): Failed."))
        numberOfWarnings++;
}

// QTBUG-15690
void tst_qquickimage::nullPixmapPaint()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setSource(testFileUrl("nullpixmap.qml"));
    canvas->show();

    QQuickImage *image = qobject_cast<QQuickImage*>(canvas->rootObject());
    QTRY_VERIFY(image != 0);
    image->setSource(SERVER_ADDR + QString("/no-such-file.png"));

    QtMsgHandler previousMsgHandler = qInstallMsgHandler(checkWarnings);

    // used to print "QTransform::translate with NaN called"
    QPixmap pm = QPixmap::fromImage(canvas->grabFrameBuffer());
    qInstallMsgHandler(previousMsgHandler);
    QVERIFY(numberOfWarnings == 0);
    delete image;

    delete canvas;
}

void tst_qquickimage::imageCrash_QTBUG_22125()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(dataDirectory(), TestHTTPServer::Delay);

    {
        QQuickView view(testFileUrl("qtbug_22125.qml"));
        view.show();
        qApp->processEvents();
        qApp->processEvents();
        // shouldn't crash when the view drops out of scope due to
        // QQuickPixmapData attempting to dereference a pointer to
        // the destroyed reader.
    }

    // shouldn't crash when deleting cancelled QQmlPixmapReplys.
    QTest::qWait(520); // Delay mode delays for 500 ms.
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

void tst_qquickimage::sourceSize_data()
{
    QTest::addColumn<int>("sourceWidth");
    QTest::addColumn<int>("sourceHeight");
    QTest::addColumn<qreal>("implicitWidth");
    QTest::addColumn<qreal>("implicitHeight");

    QTest::newRow("unscaled") << 0 << 0 << 300.0 << 300.0;
    QTest::newRow("scale width") << 100 << 0 << 100.0 << 100.0;
    QTest::newRow("scale height") << 0 << 150 << 150.0 << 150.0;
    QTest::newRow("larger sourceSize") << 400 << 400 << 300.0 << 300.0;
}

void tst_qquickimage::sourceSize()
{
    QFETCH(int, sourceWidth);
    QFETCH(int, sourceHeight);
    QFETCH(qreal, implicitWidth);
    QFETCH(qreal, implicitHeight);

    QQuickView *canvas = new QQuickView(0);
    QQmlContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("srcWidth", sourceWidth);
    ctxt->setContextProperty("srcHeight", sourceHeight);

    canvas->setSource(testFileUrl("sourceSize.qml"));
    canvas->show();
    qApp->processEvents();

    QQuickImage *image = qobject_cast<QQuickImage*>(canvas->rootObject());
    QVERIFY(image);

    QCOMPARE(image->sourceSize().width(), sourceWidth);
    QCOMPARE(image->sourceSize().height(), sourceHeight);
    QCOMPARE(image->implicitWidth(), implicitWidth);
    QCOMPARE(image->implicitHeight(), implicitHeight);

    delete canvas;
}

QTEST_MAIN(tst_qquickimage)

#include "tst_qquickimage.moc"
