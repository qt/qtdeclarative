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

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>

#include "../../shared/util.h"

class tst_QQuickItemLayer: public QDeclarativeDataTest
{
    Q_OBJECT
public:
    tst_QQuickItemLayer();

    QImage runTest(const QString &url)
    {
        QQuickView view;
        view.setSource(QUrl(url));

        view.show();
        QTest::qWaitForWindowShown(&view);

        return view.grabFrameBuffer();
    }

private slots:
    void layerEnabled();
    void layerSmooth();
    void layerMipmap();
    void layerEffect();

    void layerVisibility_data();
    void layerVisibility();

    void layerSourceRect();


    void layerZOrder_data();
    void layerZOrder();

    void layerIsTextureProvider();
};

tst_QQuickItemLayer::tst_QQuickItemLayer()
{
}



// The test draws a red and a blue box next to each other and tests that the
// output is still red and blue on the left and right and a combination of
// the two in the middle.

void tst_QQuickItemLayer::layerSmooth()
{
    QImage fb = runTest(testFile("Smooth.qml"));
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
    QImage fb = runTest(testFile("Enabled.qml"));
    // Verify the banding
    QCOMPARE(fb.pixel(0, 0), fb.pixel(0, 1));
    // Verify the gradient
    QVERIFY(fb.pixel(0, 0) != fb.pixel(0, fb.height() - 1));
}



// The test draws a one pixel wide line and scales it down by more than a a factor 2
// If mipmpping works, the pixels should be gray, not white or black

void tst_QQuickItemLayer::layerMipmap()
{
    QImage fb = runTest(testFile("Mipmap.qml"));
    QVERIFY(fb.pixel(0, 0) != 0xff000000);
    QVERIFY(fb.pixel(0, 0) != 0xffffffff);
}



// The test implements an rgb swapping effect sourced from a blue rectangle. The
// resulting pixel should be red

void tst_QQuickItemLayer::layerEffect()
{
    QImage fb = runTest(testFile("Effect.qml"));
    QCOMPARE(fb.pixel(0, 0), qRgb(0xff, 0, 0));
    QCOMPARE(fb.pixel(fb.width() - 1, 0), qRgb(0, 0xff, 0));
}



// The test draws a rectangle and verifies that there is padding on each side
// as the source rect spans outside the item. The padding is verified using
// a shader that pads transparent to blue. Everything else is red.
void tst_QQuickItemLayer::layerSourceRect()
{
    QImage fb = runTest(testFile("SourceRect.qml"));

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
    QImage fb = runTest(testFile("TextureProvider.qml"));
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
    QFETCH(bool, visible);
    QFETCH(bool, effect);
    QFETCH(qreal, opacity);

    QQuickView view;
    view.setSource(testFile("Visible.qml"));

    QQuickItem *child = view.rootItem()->childItems().at(0);
    child->setProperty("layerVisible", visible);
    child->setProperty("layerEffect", effect);
    child->setProperty("layerOpacity", opacity);

    view.show();

    QTest::qWaitForWindowShown(&view);

    QImage fb = view.grabFrameBuffer();
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
    QFETCH(bool, effect);

    QQuickView view;
    view.setSource(testFile("ZOrder.qml"));

    QQuickItem *child = view.rootItem()->childItems().at(0);
    child->setProperty("layerEffect", effect);

    view.show();

    QTest::qWaitForWindowShown(&view);

    QImage fb = view.grabFrameBuffer();

    QCOMPARE(fb.pixel(50, 50), qRgb(0, 0, 0xff));
    QCOMPARE(fb.pixel(150, 150), qRgb(0, 0xff, 00));

}



QTEST_MAIN(tst_QQuickItemLayer)

#include "tst_qquickitemlayer.moc"
