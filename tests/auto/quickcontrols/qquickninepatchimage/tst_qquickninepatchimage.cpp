// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtCore/qmath.h>
#include <QtCore/qsize.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitemgrabresult.h>
#include <QtQuick/private/qquickimage_p.h>
#include <QtQuick/private/qquickimage_p_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtGui/private/qrhi_p.h>

using namespace QQuickVisualTestUtils;

class tst_qquickninepatchimage : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qquickninepatchimage();

private slots:
    void ninePatch_data();
    void ninePatch();
    void padding_data();
    void padding();
    void inset_data();
    void inset();
    void implicitSize_data();
    void implicitSize();
    void hwCompressedImages_data();
    void hwCompressedImages();
};

static QImage grabItemToImage(QQuickItem *item)
{
    QSharedPointer<QQuickItemGrabResult> result = item->grabToImage();
    QSignalSpy spy(result.data(), SIGNAL(ready()));
    spy.wait();
    return result->image();
}

tst_qquickninepatchimage::tst_qquickninepatchimage()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickninepatchimage::ninePatch_data()
{
    QTest::addColumn<int>("dpr");
    QTest::addColumn<QSize>("size");

    // original size, downsized, stretched
    const QList<QSize> sizes = QList<QSize>()
            << QSize(40, 40) // original
            << QSize(10, 40) // downsized (h)
            << QSize(40, 10) // downsized (v)
            << QSize(10, 10) // downsized (h & v)
            << QSize(80, 40) // stretched (h)
            << QSize(40, 80) // stretched (v)
            << QSize(80, 80) // stretched (h & v)
            << QSize(8, 8);  // minimal (borders only)

    for (const QSize &sz : sizes) {
        for (int dpr = 1; dpr <= 4; ++dpr)
            QTest::newRow(qPrintable(QString::fromLatin1("DPR=%1, %2x%3").arg(dpr).arg(sz.width()).arg(sz.height()))) << dpr << sz;
    }
}

void tst_qquickninepatchimage::ninePatch()
{
    QFETCH(int, dpr);
    QFETCH(QSize, size);

    QHighDpiScaling::setGlobalFactor(dpr);

    QQuickView view(testFileUrl("ninepatchimage.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickImage *ninePatchImage = qobject_cast<QQuickImage *>(view.rootObject());
    QVERIFY(ninePatchImage);
    ninePatchImage->setSource(testFileUrl("foo.9.png"));
    ninePatchImage->setSize(size);

    const QImage ninePatchImageGrab = grabItemToImage(ninePatchImage).scaled(size * dpr);

    // Generate an image to compare against the actual 9-patch image.
    QImage generatedImage(size * dpr, ninePatchImageGrab.format());
    generatedImage.fill(Qt::red);

    QImage blueRect(4 * dpr, 4 * dpr, ninePatchImageGrab.format());
    blueRect.fill(Qt::blue);

    QPainter painter(&generatedImage);
    // Top-left
    painter.drawImage(0, 0, blueRect);
    // Top-right
    painter.drawImage(generatedImage.width() - blueRect.width(), 0, blueRect);
    // Bottom-right
    painter.drawImage(generatedImage.width() - blueRect.width(), generatedImage.height() - blueRect.height(), blueRect);
    // Bottom-left
    painter.drawImage(0, generatedImage.height() - blueRect.height(), blueRect);

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");

    QCOMPARE(ninePatchImageGrab, generatedImage);
}

void tst_qquickninepatchimage::padding_data()
{
    QTest::addColumn<int>("dpr");

    for (int dpr = 1; dpr <= 4; ++dpr)
        QTest::newRow(qPrintable(QString::fromLatin1("DPR=%1").arg(dpr))) << dpr;
}

void tst_qquickninepatchimage::padding()
{
    QFETCH(int, dpr);

    QHighDpiScaling::setGlobalFactor(dpr);

    QQuickView view(testFileUrl("ninepatchimage.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickImage *ninePatchImage = qobject_cast<QQuickImage *>(view.rootObject());
    QVERIFY(ninePatchImage);
    ninePatchImage->setSource(testFileUrl("padding.9.png"));

    QCOMPARE(ninePatchImage->property("topPadding").toReal(), 8);
    QCOMPARE(ninePatchImage->property("leftPadding").toReal(), 18);
    QCOMPARE(ninePatchImage->property("rightPadding").toReal(), 20);
    QCOMPARE(ninePatchImage->property("bottomPadding").toReal(), 10);
}

void tst_qquickninepatchimage::inset_data()
{
    QTest::addColumn<int>("dpr");
    QTest::addColumn<QString>("file");
    QTest::addColumn<QMarginsF>("insets");

    const QStringList files = QStringList() << "inset-all.9.png" << "inset-topleft.9.png" << "inset-bottomright.9.png";
    const QList<QMarginsF> insets = QList<QMarginsF>() << QMarginsF(2, 1, 3, 4) << QMarginsF(2, 1, 0, 0) << QMarginsF(0, 0, 3, 4);

    for (int i = 0; i < files.size(); ++i) {
        QString file = files.at(i);
        for (int dpr = 1; dpr <= 4; ++dpr)
            QTest::newRow(qPrintable(QString::fromLatin1("%1 DPR=%2").arg(file).arg(dpr))) << dpr << file << insets.at(i);
    }
}

Q_DECLARE_METATYPE(QMarginsF)

void tst_qquickninepatchimage::inset()
{
    QFETCH(int, dpr);
    QFETCH(QString, file);
    QFETCH(QMarginsF, insets);

    QHighDpiScaling::setGlobalFactor(dpr);

    QQuickView view(testFileUrl("ninepatchimage.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickImage *ninePatchImage = qobject_cast<QQuickImage *>(view.rootObject());
    QVERIFY(ninePatchImage);
    ninePatchImage->setSource(testFileUrl(file));

    QCOMPARE(ninePatchImage->property("topInset").toReal(), insets.top());
    QCOMPARE(ninePatchImage->property("leftInset").toReal(), insets.left());
    QCOMPARE(ninePatchImage->property("rightInset").toReal(), insets.right());
    QCOMPARE(ninePatchImage->property("bottomInset").toReal(), insets.bottom());
}

void tst_qquickninepatchimage::implicitSize_data()
{
    QTest::addColumn<int>("dpr");
    QTest::addColumn<QString>("file");
    QTest::addColumn<QSizeF>("implicitSize");

    const struct TestFile {
        QString name;
        QSizeF sizeHint;
    } testFiles [] = {
        { "foo.9.png", QSizeF(40, 40) },
        { "padding.9.png", QSizeF(40, 40) },
        { "inset-all.9.png", QSizeF(45, 45) },
        { "inset-topleft.9.png", QSizeF(42, 41) },
        { "inset-bottomright.9.png", QSizeF(43, 44) }
    };

    for (const TestFile &file : testFiles) {
        for (int dpr = 1; dpr <= 4; ++dpr)
            QTest::newRow(qPrintable(QString::fromLatin1("%1 DPR=%2").arg(file.name).arg(dpr))) << dpr << file.name << file.sizeHint;
    }
}

void tst_qquickninepatchimage::implicitSize()
{
    QFETCH(int, dpr);
    QFETCH(QString, file);
    QFETCH(QSizeF, implicitSize);

    QHighDpiScaling::setGlobalFactor(dpr);

    QQuickView view(testFileUrl("ninepatchimage.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickImage *ninePatchImage = qobject_cast<QQuickImage *>(view.rootObject());
    QVERIFY(ninePatchImage);
    ninePatchImage->setSource(testFileUrl(file));

    QCOMPARE(ninePatchImage->implicitWidth(), implicitSize.width());
    QCOMPARE(ninePatchImage->implicitHeight(), implicitSize.height());
}

void tst_qquickninepatchimage::hwCompressedImages_data()
{
    QTest::addColumn<int>("dpr");
    QTest::addColumn<QString>("file");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<QRhiTexture::Format>("format");

    const struct TestFile {
        QString name;
        QSize size;
        QRhiTexture::Format format;
    } testFiles [] = {
        { "o1_bc1.ktx", QSize(64, 64), QRhiTexture::BC1 },
        { "logo.pkm", QSize(256, 256), QRhiTexture::ETC2_RGB8 },
        { "qt4.astc", QSize(250, 200), QRhiTexture::ASTC_8x8 }
    };

    for (const TestFile &file : testFiles) {
        for (int dpr = 1; dpr <= 4; ++dpr)
            QTest::newRow(qPrintable(QString::fromLatin1("%1 DPR=%2").arg(file.name).arg(dpr))) << dpr << file.name << file.size << file.format;
    }
}

void tst_qquickninepatchimage::hwCompressedImages()
{
    QFETCH(int, dpr);
    QFETCH(QString, file);
    QFETCH(QSize, size);
    QFETCH(QRhiTexture::Format, format);

    QHighDpiScaling::setGlobalFactor(dpr);

    QQuickView view(testFileUrl("ninepatchimage.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    if (!QSGRendererInterface::isApiRhiBased(view.rendererInterface()->graphicsApi()))
        QSKIP("Skipping due to using software backend");

    QRhi *rhi = static_cast<QRhi *>(view.rendererInterface()->getResource(&view, QSGRendererInterface::RhiResource));
    if (!rhi->isTextureFormatSupported(format))
        QSKIP(qPrintable(QString::fromLatin1("%1 not supported, skip").arg(format)));

    QQuickImage *ninePatchImage = qobject_cast<QQuickImage *>(view.rootObject());
    QVERIFY(ninePatchImage);
    ninePatchImage->setSource(testFileUrl(file));
    ninePatchImage->setSize(size);
    QSignalSpy spy(&view, SIGNAL(afterSynchronizing()));
    QTRY_VERIFY(spy.size() >= 1);

    QQuickImagePrivate *ninePatchImagePrivate = static_cast<QQuickImagePrivate *>(QQuickItemPrivate::get(ninePatchImage));
    QVERIFY(ninePatchImagePrivate->paintNode);
}

QTEST_MAIN(tst_qquickninepatchimage)

#include "tst_qquickninepatchimage.moc"
