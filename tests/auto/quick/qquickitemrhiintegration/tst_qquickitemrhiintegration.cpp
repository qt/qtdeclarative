/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtGui/private/qrhi_p.h>

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
