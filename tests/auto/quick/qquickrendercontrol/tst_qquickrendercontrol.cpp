/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>

#if QT_CONFIG(opengl)
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#endif
#include <QAnimationDriver>

#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>

#include "../../shared/util.h"

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

class tst_RenderControl : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void initTestCase();
    void renderAndReadBack();
};

void tst_RenderControl::initTestCase()
{
    QQmlDataTest::initTestCase();
}

class AnimationDriver : public QAnimationDriver
{
public:
    AnimationDriver(int msPerStep) : m_step(msPerStep) { }

    void advance() override
    {
        m_elapsed += m_step;
        advanceAnimation();
    }

    qint64 elapsed() const override
    {
        return m_elapsed;
    }

private:
    int m_step;
    qint64 m_elapsed = 0;
};


void tst_RenderControl::renderAndReadBack()
{
    static const int ANIM_ADVANCE_PER_FRAME = 16; // milliseconds
    QScopedPointer<AnimationDriver> animDriver(new AnimationDriver(ANIM_ADVANCE_PER_FRAME));
    animDriver->install();

    // ### Qt 6: migrate this to OpenGL-on-RHI
#if QT_CONFIG(opengl)
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL))
        QSKIP("Skipping due to platform not supporting OpenGL at run time");

    QScopedPointer<QOpenGLContext> context(new QOpenGLContext);
    QVERIFY(context->create());
    QScopedPointer<QOffscreenSurface> offscreenSurface(new QOffscreenSurface);
    offscreenSurface->setFormat(context->format());
    offscreenSurface->create();
    QVERIFY(context->makeCurrent(offscreenSurface.data()));

    QScopedPointer<QQuickRenderControl> renderControl(new QQuickRenderControl);
    QScopedPointer<QQuickWindow> quickWindow(new QQuickWindow(renderControl.data()));
    QScopedPointer<QQmlEngine> qmlEngine(new QQmlEngine);

    QScopedPointer<QQmlComponent> qmlComponent(new QQmlComponent(qmlEngine.data(), testFileUrl(QLatin1String("rect.qml"))));
    QVERIFY(!qmlComponent->isLoading());
    if (qmlComponent->isError()) {
        for (const QQmlError &error : qmlComponent->errors())
            qWarning() << error.url() << error.line() << error;
    }
    QVERIFY(!qmlComponent->isError());

    QObject *rootObject = qmlComponent->create();
    if (qmlComponent->isError()) {
        for (const QQmlError &error : qmlComponent->errors())
            qWarning() << error.url() << error.line() << error;
    }
    QVERIFY(!qmlComponent->isError());

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(rootObject);
    QVERIFY(rootItem);
    QCOMPARE(rootItem->size(), QSize(200, 200));

    quickWindow->contentItem()->setSize(rootItem->size());
    quickWindow->setGeometry(0, 0, rootItem->width(), rootItem->height());

    rootItem->setParentItem(quickWindow->contentItem());

    renderControl->initialize(context.data());

    // cannot do this test with the 'software' backend of Qt Quick
    QSGRendererInterface::GraphicsApi api = quickWindow->rendererInterface()->graphicsApi();
    if (api != QSGRendererInterface::OpenGL)
        QSKIP("Skipping due to Qt Quick not using the direct OpenGL rendering path");

    QScopedPointer<QOpenGLFramebufferObject> fbo(new QOpenGLFramebufferObject(rootItem->size().toSize(),
        QOpenGLFramebufferObject::CombinedDepthStencil));

    quickWindow->setRenderTarget(fbo.data());

    for (int frame = 0; frame < 100; ++frame) {
        // have to process events, e.g. to get queued metacalls delivered
        QCoreApplication::processEvents();

        if (frame > 0) {
            // Quick animations will now think that ANIM_ADVANCE_PER_FRAME milliseconds have passed,
            // even though in reality we have a tight loop that generates frames unthrottled.
            animDriver->advance();
        }

        renderControl->polishItems();
        renderControl->sync();
        renderControl->render();

        context->functions()->glFlush();

        QImage img = fbo->toImage();
        QVERIFY(!img.isNull());
        QCOMPARE(img.size(), rootItem->size());

        const int maxFuzz = 2;

        // The scene is: background, rectangle, text
        // where rectangle rotates

        QRgb background = img.pixel(5, 5);
        QVERIFY(qAbs(qRed(background) - 70) < maxFuzz);
        QVERIFY(qAbs(qGreen(background) - 130) < maxFuzz);
        QVERIFY(qAbs(qBlue(background) - 180) < maxFuzz);

        background = img.pixel(195, 195);
        QVERIFY(qAbs(qRed(background) - 70) < maxFuzz);
        QVERIFY(qAbs(qGreen(background) - 130) < maxFuzz);
        QVERIFY(qAbs(qBlue(background) - 180) < maxFuzz);

        // after about 1.25 seconds (animation time, one iteration is 16 ms
        // thanks to our custom animation driver) the rectangle reaches a 90
        // degree rotation, that should be frame 76
        if (frame <= 2 || (frame >= 76 && frame <= 80)) {
            QRgb c = img.pixel(28, 28); // rectangle
            QVERIFY(qAbs(qRed(c) - 152) < maxFuzz);
            QVERIFY(qAbs(qGreen(c) - 251) < maxFuzz);
            QVERIFY(qAbs(qBlue(c) - 152) < maxFuzz);
        } else {
            QRgb c = img.pixel(28, 28); // background because rectangle got rotated so this pixel is not covered by it
            QVERIFY(qAbs(qRed(c) - 70) < maxFuzz);
            QVERIFY(qAbs(qGreen(c) - 130) < maxFuzz);
            QVERIFY(qAbs(qBlue(c) - 180) < maxFuzz);
        }
    }

#else
    QSKIP("No OpenGL, skipping rendercontrol test");
#endif
}

#include "tst_qquickrendercontrol.moc"

QTEST_MAIN(tst_RenderControl)
