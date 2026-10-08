// Copyright (C) 2022 zccrs <zccrs@live.com>, JiDe Zhang <zhangjide@uniontech.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick>
#include <QtQml>
#include <QGuiApplication>

#include <private/qquickanimator_p.h>
#include <private/qquickitem_p.h>
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
    void hiddenItem_data();
    void hiddenItem();
    void hiddenItemMultipleReasons_data();
    void hiddenItemMultipleReasons();
};

tst_SoftwareRenderer::tst_SoftwareRenderer()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_SoftwareRenderer::initTestCase()
{
    QQmlDataTest::initTestCase();
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

    rc.sync();
    rc.render();

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

    rc.sync();
    rc.render();

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

void tst_SoftwareRenderer::hiddenItem_data()
{
    QTest::addColumn<QByteArray>("hideMode");

    QTest::newRow("visible") << QByteArray("visible");
    QTest::newRow("culled") << QByteArray("culled");
    QTest::newRow("hideSource") << QByteArray("hideSource");
    QTest::newRow("opacity") << QByteArray("opacity");
}

void tst_SoftwareRenderer::hiddenItem()
{
    if (QQuickWindow::sceneGraphBackend() != "software")
        QSKIP("Skipping complex rendering tests due to not running with software");

    QFETCH(QByteArray, hideMode);

    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("hiddenItem.qml")));

    QQuickItem *target = view.rootObject()->findChild<QQuickItem *>("target");
    QVERIFY(target);
    QQuickItem *effectSource = view.rootObject()->findChild<QQuickItem *>("effectSource");
    QVERIFY(effectSource);
    QQuickAnimator *animator = view.rootObject()->findChild<QQuickAnimator *>("animator");
    QVERIFY(animator);

    const auto setHidden = [&](bool hidden) {
        if (hideMode == "visible") {
            // Without an effect referencing it, an invisible item has no node in the scene
            effectSource->setProperty("sourceItem", QVariant::fromValue(target));
            target->setVisible(!hidden);
        } else if (hideMode == "culled") {
            QQuickItemPrivate::get(target)->setCulled(hidden);
        } else if (hideMode == "hideSource") {
            effectSource->setProperty("sourceItem", QVariant::fromValue(target));
            effectSource->setProperty("hideSource", hidden);
        } else {
            target->setOpacity(hidden ? 0.0 : 1.0);
        }
    };
    const auto centerColor = [&] {
        const QPointF center = target->mapToScene(QPointF(50, 50)) * view.devicePixelRatio();
        return view.grabWindow().pixelColor(center.toPoint());
    };

    QCOMPARE(centerColor(), QColor(Qt::red));

    for (int i = 0; i < 2; ++i) {
        setHidden(true);
        QCOMPARE(centerColor(), QColor(Qt::white));
        setHidden(false);
        QCOMPARE(centerColor(), QColor(Qt::red));
    }

    if (hideMode == "opacity")
        return;

    // The animated opacity must not make the hidden item visible
    setHidden(true);
    animator->start();
    QTRY_VERIFY(animator->currentTime() > 0);
    QCOMPARE(centerColor(), QColor(Qt::white));
    animator->stop();
}

void tst_SoftwareRenderer::hiddenItemMultipleReasons_data()
{
    // Each reason is "item.reason", where reason is "visible", "culled", or
    // "hideSource" or "hideSource2" for the ShaderEffectSource named
    // "effectSource" or "effectSource2", respectively.
    QTest::addColumn<QByteArray>("first");
    QTest::addColumn<QByteArray>("second");

    const std::pair<const char *, const char *> rows[] = {
        { "target.visible", "target.culled" },
        { "target.visible", "target.hideSource" },
        { "target.culled", "target.hideSource" },
        { "target.hideSource", "target.hideSource2" },
        { "outer.visible", "target.visible" },
        { "outer.culled", "target.culled" },
        { "outer.hideSource2", "target.hideSource" },
        { "outer.visible", "target.culled" },
        { "outer.culled", "target.visible" },
    };
    for (const auto &[first, second] : rows)
        QTest::addRow("%s+%s", first, second) << QByteArray(first) << QByteArray(second);
}

void tst_SoftwareRenderer::hiddenItemMultipleReasons()
{
    if (QQuickWindow::sceneGraphBackend() != "software")
        QSKIP("Skipping complex rendering tests due to not running with software");

    QFETCH(QByteArray, first);
    QFETCH(QByteArray, second);

    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("hiddenItemScene.qml")));

    const auto setHidden = [&](const QByteArray &spec, bool hidden) {
        const QList<QByteArray> parts = spec.split('.');
        QQuickItem *item = view.rootObject()->findChild<QQuickItem *>(QString::fromLatin1(parts.first()));
        QVERIFY(item);
        const QByteArray &reason = parts.last();
        if (reason == "visible") {
            item->setVisible(!hidden);
        } else if (reason == "culled") {
            QQuickItemPrivate::get(item)->setCulled(hidden);
        } else {
            const QString name = QStringLiteral("effectSource")
                    + QString::fromLatin1(reason.sliced(qstrlen("hideSource")));
            QQuickItem *effectSource = view.rootObject()->findChild<QQuickItem *>(name);
            QVERIFY(effectSource);
            effectSource->setProperty("sourceItem", QVariant::fromValue(item));
            effectSource->setProperty("hideSource", hidden);
        }
    };
    const auto targetColor = [&] {
        const QPointF pos = QPointF(40, 40) * view.devicePixelRatio();
        return view.grabWindow().pixelColor(pos.toPoint());
    };

    QCOMPARE(targetColor(), QColor(Qt::red));

    // Hide for both reasons, then show for one reason at a time, in both orders.
    // The item stays hidden until neither reason applies.
    for (const auto &[showFirst, showLast] : { std::pair(first, second), std::pair(second, first) }) {
        setHidden(first, true);
        setHidden(second, true);
        QCOMPARE(targetColor(), QColor(Qt::white));
        setHidden(showFirst, false);
        QCOMPARE(targetColor(), QColor(Qt::white));
        setHidden(showLast, false);
        QCOMPARE(targetColor(), QColor(Qt::red));
    }
}

#include "tst_softwarerenderer.moc"

QTEST_MAIN(tst_SoftwareRenderer)
