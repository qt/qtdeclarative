// Copyright (C) 2022 zccrs <zccrs@live.com>, JiDe Zhang <zhangjide@uniontech.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtQuick>
#include <QtQml>
#include <QGuiApplication>

#include <private/qsgrenderloop_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

class tst_SoftwareRenderer : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_SoftwareRenderer();

private slots:
    void initTestCase() override;

    void renderTarget();
};

tst_SoftwareRenderer::tst_SoftwareRenderer()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_SoftwareRenderer::initTestCase()
{
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
    QSGRenderLoop *loop = QSGRenderLoop::instance();
    qDebug() << "RenderLoop:" << loop
             << "Graph backend:" << QQuickWindow::sceneGraphBackend();
}

void tst_SoftwareRenderer::renderTarget()
{
    if (QQuickWindow::sceneGraphBackend() != "software")
        QSKIP("Skipping complex rendering tests due to not running with software");

    QQuickRenderControl rc;
    QScopedPointer<QQuickWindow> window(new QQuickWindow(&rc));
    window->setWidth(10);
    window->setHeight(10);

    QImage renderTarget1(window->size(), QImage::Format_ARGB32_Premultiplied);
    renderTarget1.fill(Qt::red);
    auto rt1 = QQuickRenderTarget::fromPaintDevice(&renderTarget1);
    rt1.setDevicePixelRatio(renderTarget1.devicePixelRatio());
    window->setRenderTarget(rt1);
    window->setColor(Qt::blue);

    rc.polishItems();

    rc.beginFrame();
    rc.sync();
    rc.render();
    rc.endFrame();

    QImage content = window->grabWindow();
    QString errorMessage;
    QVERIFY2(QQuickVisualTestUtils::compareImages(content, renderTarget1, &errorMessage),
             qPrintable(errorMessage));

    QImage renderTarget2(window->size(), QImage::Format_ARGB32_Premultiplied);
    renderTarget2.fill(Qt::green);
    auto rt2 = QQuickRenderTarget::fromPaintDevice(&renderTarget2);
    rt2.setDevicePixelRatio(renderTarget2.devicePixelRatio());
    window->setRenderTarget(rt2);

    rc.polishItems();

    rc.beginFrame();
    rc.sync();
    rc.render();
    rc.endFrame();

    content = window->grabWindow();
    QVERIFY2(QQuickVisualTestUtils::compareImages(content, renderTarget2, &errorMessage),
             qPrintable(errorMessage));
    QVERIFY2(QQuickVisualTestUtils::compareImages(renderTarget2, renderTarget1, &errorMessage),
             qPrintable(errorMessage));

    // Clear render target
    window->setRenderTarget(QQuickRenderTarget());
    QImage content2 = window->grabWindow();
    content2 = content2.scaled(content.size());
    QVERIFY2(QQuickVisualTestUtils::compareImages(content, content2, &errorMessage),
             qPrintable(errorMessage));
}

#include "tst_softwarerenderer.moc"

QTEST_MAIN(tst_SoftwareRenderer)

