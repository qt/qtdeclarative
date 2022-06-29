// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qsgrendererinterface.h>
#include <QtQuick/private/qquickitem_p.h>
#include <qopenglcontext.h>
#include <qopenglfunctions.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

class tst_QQuickItemLayer: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickItemLayer();

    QImage runTest(const QString &fileName)
    {
        QQuickView view;
        view.setSource(testFileUrl(fileName));

        view.showNormal();
        return QTest::qWaitForWindowExposed(&view)
            ? view.grabWindow() : QImage();
    }

private slots:
    void initTestCase() override;
    void layerEnabled();
    void layerSmooth();
#if QT_CONFIG(opengl)
    void layerMipmap();
    void layerEffect();
#endif
    void layerVisibility_data();
    void layerVisibility();

    void layerSourceRect();

    void layerZOrder_data();
    void layerZOrder();

    void layerIsTextureProvider();

    void changeZOrder_data();
    void changeZOrder();

    void toggleLayerAndEffect();
    void disableLayer();
    void changeSamplerName();
    void itemEffect();
    void rectangleEffect();

    void textureMirroring_data();
    void textureMirroring();

    void effectSourceResizeToItem();

private:
    void mirroringCheck(int mirroring, int x, bool shouldMirror, const QImage &fb);
};

bool isOffscreen()
{
    return (QGuiApplication::platformName() == QLatin1String("offscreen"))
            || (QGuiApplication::platformName() == QLatin1String("minimal"));
}
const char skipOffscreenMsg[] =
        "Skipping due to grabWindow not functional on offscreen/minimal platformsi (QTBUG-63185)";

tst_QQuickItemLayer::tst_QQuickItemLayer() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

void tst_QQuickItemLayer::initTestCase()
{
    QQmlDataTest::initTestCase();
    QQuickView view;
    view.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
}

// The test draws a red and a blue box next to each other and tests that the
// output is still red and blue on the left and right and a combination of
// the two in the middle.

void tst_QQuickItemLayer::layerSmooth()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QImage fb = runTest("Smooth.qml");
    QVERIFY(!fb.size().isEmpty());
    QCOMPARE(fb.pixel(0, 0), qRgb(0xff, 0, 0));
    QCOMPARE(fb.pixel(fb.width() - 1, 0), qRgb(0, 0, 0xff));

    uint pixel = fb.pixel(fb.width() / 2, 0);
    QVERIFY(qRed(pixel) > 0);
    QVERIFY(qBlue(pixel) > 0);
}



// The test draws a gradient at a small size into a layer and scales the
// layer. If the layer is enabled there should be very visible bands in
// the gradient.

void tst_QQuickItemLayer::layerEnabled()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QImage fb = runTest("Enabled.qml");
    QVERIFY(!fb.size().isEmpty());
    // Verify the banding
    QCOMPARE(fb.pixel(0, 0), fb.pixel(0, 1));
    // Verify the gradient
    QVERIFY(fb.pixel(0, 0) != fb.pixel(0, fb.height() - 1));
}

#if QT_CONFIG(opengl)
// The test draws a one pixel wide line and scales it down by more than a a factor 2
// If mipmpping works, the pixels should be gray, not white or black

void tst_QQuickItemLayer::layerMipmap()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QImage fb = runTest("Mipmap.qml");
    QVERIFY(fb.pixel(0, 0) != 0xff000000);
    QVERIFY(fb.pixel(0, 0) != 0xffffffff);
}



// The test implements an rgb swapping effect sourced from a blue rectangle. The
// resulting pixel should be red

void tst_QQuickItemLayer::layerEffect()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QImage fb = runTest("Effect.qml");
    QVERIFY(!fb.size().isEmpty());
    QCOMPARE(fb.pixel(0, 0), qRgb(0xff, 0, 0));
    QCOMPARE(fb.pixel(fb.width() - 1, 0), qRgb(0, 0xff, 0));
}
#endif

// The test draws a rectangle and verifies that there is padding on each side
// as the source rect spans outside the item. The padding is verified using
// a shader that pads transparent to blue. Everything else is red.
void tst_QQuickItemLayer::layerSourceRect()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QImage fb = runTest("SourceRect.qml");
    QVERIFY(!fb.size().isEmpty());

    // Check that the edges are converted to blue
    QCOMPARE(fb.pixel(0, 0), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(fb.width() - 1, 0), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(0, fb.height() - 1), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(fb.width() - 1, fb.height() - 1), qRgb(0, 0, 0xff));

    // The center pixel should be red
    QCOMPARE(fb.pixel(fb.width() / 2, fb.height() / 2), qRgb(0xff, 0, 0));
}



// Same as the effect test up above, but this time use the item
// directly in a stand alone ShaderEffect
void tst_QQuickItemLayer::layerIsTextureProvider()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QImage fb = runTest("TextureProvider.qml");
    QVERIFY(!fb.size().isEmpty());
    QCOMPARE(fb.pixel(0, 0), qRgb(0xff, 0, 0));
    QCOMPARE(fb.pixel(fb.width() - 1, 0), qRgb(0, 0xff, 0));
}


void tst_QQuickItemLayer::layerVisibility_data()
{
    QTest::addColumn<bool>("visible");
    QTest::addColumn<bool>("effect");
    QTest::addColumn<qreal>("opacity");

    QTest::newRow("!effect, !visible, a=1") << false << false << 1.;
    QTest::newRow("!effect, visible, a=1") << false << true << 1.;
    QTest::newRow("effect, !visible, a=1") << true << false << 1.;
    QTest::newRow("effect, visible, a=1") << true << true << 1.;

    QTest::newRow("!effect, !visible, a=.5") << false << false << .5;
    QTest::newRow("!effect, visible, a=.5") << false << true << .5;
    QTest::newRow("effect, !visible, a=.5") << true << false << .5;
    QTest::newRow("effect, visible, a=.5") << true << true << .5;

    QTest::newRow("!effect, !visible, a=0") << false << false << 0.;
    QTest::newRow("!effect, visible, a=0") << false << true << 0.;
    QTest::newRow("effect, !visible, a=0") << true << false << 0.;
    QTest::newRow("effect, visible, a=0") << true << true << 0.;
}

void tst_QQuickItemLayer::layerVisibility()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QFETCH(bool, visible);
    QFETCH(bool, effect);
    QFETCH(qreal, opacity);

    QQuickView view;
    view.setSource(testFileUrl("Visible.qml"));

    QQuickItem *child = view.contentItem()->childItems().at(0);
    child->setProperty("layerVisible", visible);
    child->setProperty("layerEffect", effect);
    child->setProperty("layerOpacity", opacity);

    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QImage fb = view.grabWindow();
    uint pixel = fb.pixel(0, 0);

    if (!visible || opacity == 0) {
        QCOMPARE(pixel, qRgb(0xff, 0xff, 0xff));
    } else if (effect) {
        QCOMPARE(qRed(pixel), 0xff);
        QVERIFY(qGreen(pixel) < 0xff);
        QVERIFY(qBlue(pixel) < 0xff);
    } else { // no effect
        QCOMPARE(qBlue(pixel), 0xff);
        QVERIFY(qGreen(pixel) < 0xff);
        QVERIFY(qRed(pixel) < 0xff);
    }
}




void tst_QQuickItemLayer::layerZOrder_data()
{
    QTest::addColumn<bool>("effect");

    QTest::newRow("!effect") << false;
    QTest::newRow("effect") << true;
}

void tst_QQuickItemLayer::layerZOrder()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QFETCH(bool, effect);

    QQuickView view;
    view.setSource(testFileUrl("ZOrder.qml"));

    QQuickItem *child = view.contentItem()->childItems().at(0);
    child->setProperty("layerEffect", effect);

    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QImage fb = view.grabWindow();

    QCOMPARE(fb.pixel(50, 50), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(150, 150), qRgb(0, 0xff, 00));

}

void tst_QQuickItemLayer::changeZOrder_data()
{
    QTest::addColumn<bool>("layered");
    QTest::addColumn<bool>("effect");

    QTest::newRow("layered, effect") << true << true;
    QTest::newRow("layered, !effect") << true << false;
    QTest::newRow("!layered") << false << false;
}

void tst_QQuickItemLayer::changeZOrder()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QFETCH(bool, layered);
    QFETCH(bool, effect);

    QQuickView view;
    view.setSource(testFileUrl("ZOrderChange.qml"));

    QQuickItem *child = view.contentItem()->childItems().at(0);
    child->setProperty("layerEnabled", layered);
    child->setProperty("layerEffect", effect);
    child->setProperty("layerZ", 1);

    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QImage fb = view.grabWindow();

    QRgb topLeft = fb.pixel(50, 50);
    QRgb topRight = fb.pixel(150, 50);
    QRgb bottomLeft = fb.pixel(50, 150);
    QRgb bottomRight = fb.pixel(150, 150);

    QCOMPARE(bottomLeft, qRgb(0, 0, 0xff));

    if (layered) {
        QCOMPARE(topLeft, qRgb(0, 0xff, 0xff));
    } else {
        QCOMPARE(qGreen(topLeft), 0xff);
        QVERIFY(qAbs(qRed(topLeft) - 0x3f) < 4);
        QVERIFY(qAbs(qBlue(topLeft) - 0xbf) < 4);
    }

    if (layered && effect) {
        QCOMPARE(qRed(topRight), 0xff);
        QCOMPARE(qGreen(topRight), 0x00);
        QVERIFY(qAbs(qBlue(topRight) - 0x7f) < 4);

        QVERIFY(qAbs(qRed(bottomRight) - 0x7f) < 4);
        QCOMPARE(qBlue(bottomRight), 0xff);
        QVERIFY(qAbs(qGreen(bottomRight) - 0x7f) < 4);
    } else {
        QCOMPARE(qRed(topRight), 0xff);
        QCOMPARE(qBlue(topRight), 0x00);
        QVERIFY(qAbs(qGreen(topRight) - 0x7f) < 4);

        QVERIFY(qAbs(qRed(bottomRight) - 0x7f) < 4);
        QCOMPARE(qGreen(bottomRight), 0xff);
        QVERIFY(qAbs(qBlue(bottomRight) - 0x7f) < 4);
    }
}

void tst_QQuickItemLayer::toggleLayerAndEffect()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    // This test passes if it doesn't crash.
    runTest("ToggleLayerAndEffect.qml");
}

void tst_QQuickItemLayer::disableLayer()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    // This test passes if it doesn't crash.
    runTest("DisableLayer.qml");
}

void tst_QQuickItemLayer::changeSamplerName()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QImage fb = runTest("SamplerNameChange.qml");
    QVERIFY(!fb.size().isEmpty());
    QCOMPARE(fb.pixel(0, 0), qRgb(0, 0, 0xff));
}

void tst_QQuickItemLayer::itemEffect()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QImage fb = runTest("ItemEffect.qml");
    QVERIFY(!fb.size().isEmpty());
    QCOMPARE(fb.pixel(0, 0), qRgb(0xff, 0, 0));
    QCOMPARE(fb.pixel(199, 0), qRgb(0xff, 0, 0));
    QCOMPARE(fb.pixel(0, 199), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(199, 199), qRgb(0, 0, 0xff));
}

void tst_QQuickItemLayer::rectangleEffect()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QImage fb = runTest("RectangleEffect.qml");
    QVERIFY(!fb.size().isEmpty());
    QCOMPARE(fb.pixel(0, 0), qRgb(0, 0xff, 0));
    QCOMPARE(fb.pixel(199, 0), qRgb(0, 0xff, 0));
    QCOMPARE(fb.pixel(0, 199), qRgb(0, 0xff, 0));
    QCOMPARE(fb.pixel(199, 199), qRgb(0, 0xff, 0));

    QCOMPARE(fb.pixel(100, 0), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(199, 100), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(100, 199), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(0, 100), qRgb(0, 0, 0xff));
}

void tst_QQuickItemLayer::textureMirroring_data()
{
    QTest::addColumn<int>("mirroring");

    QTest::newRow("no mirroring") << 0;
    QTest::newRow("horizontal") << 1;
    QTest::newRow("vertical") << 2;
    QTest::newRow("horizontal | vertical") << 3;
}

void tst_QQuickItemLayer::textureMirroring()
{
    if (isOffscreen())
        QSKIP(skipOffscreenMsg);

    QFETCH(int, mirroring);

    QQuickView view;
    view.setSource(testFileUrl("TextureMirroring.qml"));

    QQuickItem *child = view.contentItem()->childItems().at(0);
    child->setProperty("mirroring", mirroring);

    view.show();

    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QImage fb = view.grabWindow();

    // Mirroring should have no visual effect on layered item without shader effect
    mirroringCheck(mirroring, 0, false, fb);

    // Mirroring should have visual effect on layered item with shader effect
    mirroringCheck(mirroring, 50, true, fb);

    // Mirroring should have no visual effect on source item for ShaderEffectSource
    mirroringCheck(mirroring, 100, false, fb);

    // Mirroring should have no visual effect on ShaderEffectSource item
    mirroringCheck(mirroring, 150, false, fb);

    // Mirroring should have visual effect on ShaderEffect item itself
    mirroringCheck(mirroring, 200, true, fb);
}

void tst_QQuickItemLayer::effectSourceResizeToItem() // QTBUG-104442
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("itemImageLayer.qml")));
    window.setResizeMode(QQuickView::SizeRootObjectToView);
    QQuickItem *image = window.rootObject()->findChild<QQuickItem*>("image");
    QVERIFY(image);
    QCOMPARE(image->size(), window.rootObject()->size());
    QQuickItemLayer *layer = QQuickItemPrivate::get(image)->layer();
    QVERIFY(layer);
    auto *effectSource = layer->effectSource();
    QVERIFY(effectSource);
    QCOMPARE(effectSource->size(), image->size());

    window.resize(200, 200); // shrink it a bit
    QTRY_COMPARE(image->size().toSize(), QSize(200, 200)); // wait for the window system
    QCOMPARE(effectSource->size(), image->size());
}

void tst_QQuickItemLayer::mirroringCheck(int mirroring, int x, bool shouldMirror, const QImage &fb)
{
    int offset = 10;
    int spacing = 25;

    if (shouldMirror) {
        switch (mirroring) {
        case 0: { // No mirroring -> Visually Y gets swapped, X is default
            QCOMPARE(fb.pixel(x + offset, offset), qRgb(0, 0xff, 0));
            QCOMPARE(fb.pixel(x + offset + spacing, offset), qRgb(0, 0, 0xff));
            QCOMPARE(fb.pixel(x + offset, offset + spacing), qRgb(0, 0, 0));
            QCOMPARE(fb.pixel(x + offset + spacing, offset + spacing), qRgb(0xff, 0, 0));
            break;
        }
        case 1: { // Horizontal mirroring -> Visually both X and Y get swapped, as neither is default
            QCOMPARE(fb.pixel(x + offset, offset), qRgb(0, 0, 0xff));
            QCOMPARE(fb.pixel(x + offset + spacing, offset), qRgb(0, 0xff, 0));
            QCOMPARE(fb.pixel(x + offset, offset + spacing), qRgb(0xff, 0, 0));
            QCOMPARE(fb.pixel(x + offset + spacing, offset + spacing), qRgb(0, 0, 0));
            break;
        }
        case 2: { // Vertical mirroring -> The default case, nothing gets swapped
            QCOMPARE(fb.pixel(x + offset, offset), qRgb(0, 0, 0));
            QCOMPARE(fb.pixel(x + offset + spacing, offset), qRgb(0xff, 0, 0));
            QCOMPARE(fb.pixel(x + offset, offset + spacing), qRgb(0, 0xff, 0));
            QCOMPARE(fb.pixel(x + offset + spacing, offset + spacing), qRgb(0, 0, 0xff));
            break;
        }
        case 3: { // Both axes mirrored -> Visually X gets swapped, Y is default
            QCOMPARE(fb.pixel(x + offset, offset), qRgb(0xff, 0, 0));
            QCOMPARE(fb.pixel(x + offset + spacing, offset), qRgb(0, 0, 0));
            QCOMPARE(fb.pixel(x + offset, offset + spacing), qRgb(0, 0, 0xff));
            QCOMPARE(fb.pixel(x + offset + spacing, offset + spacing), qRgb(0, 0xff, 0));
            break;
        }
        default:
            qWarning() << "Invalid case!";
            break;
        }
    } else {
        QCOMPARE(fb.pixel(x + offset, offset), qRgb(0, 0, 0));
        QCOMPARE(fb.pixel(x + offset + spacing, offset), qRgb(0xff, 0, 0));
        QCOMPARE(fb.pixel(x + offset, offset + spacing), qRgb(0, 0xff, 0));
        QCOMPARE(fb.pixel(x + offset + spacing, offset + spacing), qRgb(0, 0, 0xff));
    }
}

QTEST_MAIN(tst_QQuickItemLayer)

#include "tst_qquickitemlayer.moc"
