// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QQuickView>
#include <QSGRendererInterface>
#include <private/qsgrhisupport_p.h>
#include <private/qquickrhiitem_p.h>
#include "testrhiitem.h"

class tst_QQuickRhiItem : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickRhiItem();

private slots:
    void initTestCase() override;
    void cleanupTestCase();
    void rhiItem();
    void properties();
};

tst_QQuickRhiItem::tst_QQuickRhiItem()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickRhiItem::initTestCase()
{
    QQmlDataTest::initTestCase();
    qDebug() << "RHI backend:" << QSGRhiSupport::instance()->rhiBackendName();
}

void tst_QQuickRhiItem::cleanupTestCase()
{
}

void tst_QQuickRhiItem::rhiItem()
{
    if (QGuiApplication::platformName() == QLatin1String("offscreen")
        || QGuiApplication::platformName() == QLatin1String("minimal"))
    {
        QSKIP("Skipping on offscreen/minimal");
    }

    // Load up a scene that instantiates a TestRhiItem, which in turn
    // renders a triangle with QRhi into a QRhiTexture and then draws
    // a quad textured with it.

    QQuickView view;
    view.setSource(testFileUrl(QLatin1String("test.qml")));
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    if (!QSGRendererInterface::isApiRhiBased(view.rendererInterface()->graphicsApi()))
        QSKIP("Scenegraph does not use QRhi, test is pointless");

    QImage result = view.grabWindow();
    QVERIFY(!result.isNull());

    const int tolerance = 5;

    // The result is a red triangle in the center of the view, confirm at least one pixel.
    QRgb a = result.pixel(result.width() / 2, result.height() / 2);
    QRgb b = QColor(Qt::red).rgb();
    QVERIFY(qAbs(qRed(a) - qRed(b)) <= tolerance
            && qAbs(qGreen(a) - qGreen(b)) <= tolerance
            && qAbs(qBlue(a) - qBlue(b)) <= tolerance);
}

void tst_QQuickRhiItem::properties()
{
    if (QGuiApplication::platformName() == QLatin1String("offscreen")
        || QGuiApplication::platformName() == QLatin1String("minimal"))
    {
        QSKIP("Skipping on offscreen/minimal");
    }

    QQuickView view;
    view.setSource(testFileUrl(QLatin1String("test.qml")));
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    if (!QSGRendererInterface::isApiRhiBased(view.rendererInterface()->graphicsApi()))
        QSKIP("Scenegraph does not use QRhi, test is pointless");

    QQuickRhiItem *item = view.rootObject()->findChild<QQuickRhiItem *>("rhiitem");
    QVERIFY(item);

    view.grabWindow(); // to ensure there's a frame
    // not quite safe in theory (threads etc.) but we know it works in practice
    QQuickRhiItemPrivate *d = QQuickRhiItemPrivate::get(item);
    QVERIFY(d->node);
    TestRenderer *r = static_cast<TestRenderer *>(d->node->m_renderer.get());
    QVERIFY(r);
    QRhi *rhi = r->rhi();
    QVERIFY(rhi);

    QVERIFY(r->colorTexture());
    QVERIFY(!r->msaaColorBuffer());
    QVERIFY(r->depthStencilBuffer());
    QVERIFY(r->renderTarget());
    QCOMPARE(item->effectiveColorBufferSize(), r->colorTexture()->pixelSize());

    QCOMPARE(item->sampleCount(), 1);
    item->setSampleCount(4);

    view.grabWindow(); // just to ensure the render thread rendered with the changed properties

    if (rhi->supportedSampleCounts().contains(4)) {
        QVERIFY(!r->colorTexture());
        QVERIFY(r->msaaColorBuffer());
        QCOMPARE(r->msaaColorBuffer()->sampleCount(), 4);
        QCOMPARE(r->depthStencilBuffer()->sampleCount(), 4);
        QCOMPARE(item->effectiveColorBufferSize(), r->msaaColorBuffer()->pixelSize());
    }

    QCOMPARE(item->alphaBlending(), false);
    item->setAlphaBlending(true);

    QCOMPARE(item->isMirrorVerticallyEnabled(), false);
    item->setMirrorVertically(true);

    item->setSampleCount(1);
    item->setFixedColorBufferWidth(123);
    item->setFixedColorBufferHeight(456);
    view.grabWindow();
    QCOMPARE(r->colorTexture()->pixelSize(), QSize(123, 456));
    QCOMPARE(item->fixedColorBufferWidth(), 123);
    QCOMPARE(item->fixedColorBufferHeight(), 456);
    QCOMPARE(item->effectiveColorBufferSize(), QSize(123, 456));

    QImage result = view.grabWindow();
    QVERIFY(!result.isNull());
    const int tolerance = 5;
    QRgb a = result.pixel(result.width() / 2, result.height() / 2);
    QRgb b = QColor(Qt::red).rgb();
    QVERIFY(qAbs(qRed(a) - qRed(b)) <= tolerance
            && qAbs(qGreen(a) - qGreen(b)) <= tolerance
            && qAbs(qBlue(a) - qBlue(b)) <= tolerance);
}

#include "tst_qquickrhiitem.moc"

QTEST_MAIN(tst_QQuickRhiItem)
