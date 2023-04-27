// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <rhi/qrhi.h>

#include <QQuickView>
#include <QSGRendererInterface>
#include <private/qsgrhisupport_p.h>

class tst_QQuickItemRhiIntegration : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickItemRhiIntegration();

private slots:
    void initTestCase() override;
    void cleanupTestCase();
    void rhiItem();
};

tst_QQuickItemRhiIntegration::tst_QQuickItemRhiIntegration()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickItemRhiIntegration::initTestCase()
{
    QQmlDataTest::initTestCase();
    qDebug() << "RHI backend:" << QSGRhiSupport::instance()->rhiBackendName();
}

void tst_QQuickItemRhiIntegration::cleanupTestCase()
{
}

void tst_QQuickItemRhiIntegration::rhiItem()
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

#include "tst_qquickitemrhiintegration.moc"

QTEST_MAIN(tst_QQuickItemRhiIntegration)
