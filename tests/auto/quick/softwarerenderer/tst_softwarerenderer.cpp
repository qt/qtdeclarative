// Copyright (C) 2022 zccrs <zccrs@live.com>, JiDe Zhang <zhangjide@uniontech.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick>
#include <QtQml>
#include <QGuiApplication>

#include <private/qquickanimator_p.h>
#include <private/qquickitem_p.h>
#include <private/qsgrenderloop_p.h>
#include <private/qquickwindow_p.h>
#include <private/qsgsoftwarerenderer_p.h>
#include <private/qhighdpiscaling_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

#include <memory>

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

    void partialUpdates_data();
    void partialUpdates();
    void randomScenes_data();
    void randomScenes();

private:
    void runOracle(const QString &scene, qreal dpr, int seed);
};

namespace {

// Pixels a full repaint never touched keep this color.
constexpr QRgb Sentinel = 0xffff00ff;

struct OracleWindow
{
    QQuickRenderControl control;
    std::unique_ptr<QQuickWindow> window;
    std::unique_ptr<QQuickItem> root;
    QImage target;

    bool load(QQmlEngine *engine, const QUrl &url, qreal dpr, int seed, QString *error)
    {
        QQmlComponent component(engine, url);
        root.reset(qobject_cast<QQuickItem *>(component.create()));
        if (!root) {
            *error = component.errorString();
            return false;
        }
        if (root->property("seed").isValid())
            root->setProperty("seed", seed);
        window.reset(new QQuickWindow(&control));
        const QSize size(qRound(root->width()), qRound(root->height()));
        window->resize(size);
        const QVariant windowColor = root->property("windowColor");
        window->setColor(windowColor.isValid() ? windowColor.value<QColor>() : QColor(Qt::white));
        root->setParentItem(window->contentItem());

        target = QImage((QSizeF(size) * dpr).toSize(), QImage::Format_ARGB32_Premultiplied);
        target.setDevicePixelRatio(dpr);
        target.fill(Sentinel);
        QQuickRenderTarget rt = QQuickRenderTarget::fromPaintDevice(&target);
        rt.setDevicePixelRatio(dpr);
        window->setRenderTarget(rt);
        return true;
    }

    QSGSoftwareRenderer *renderer() const
    {
        return static_cast<QSGSoftwareRenderer *>(QQuickWindowPrivate::get(window.get())->renderer);
    }

    void step(int frame)
    {
        QMetaObject::invokeMethod(root.get(), "step", Q_ARG(QVariant, frame));
    }

    void render(bool full)
    {
        control.polishItems();
        control.sync();
        if (full) {
            target.fill(Sentinel);
            if (QSGSoftwareRenderer *r = renderer())
                r->markDirty();
        }
        control.render();
    }
};

int maxChannelDiff(QRgb a, QRgb b)
{
    return qMax(qMax(qAbs(qRed(a) - qRed(b)), qAbs(qGreen(a) - qGreen(b))),
                qMax(qAbs(qBlue(a) - qBlue(b)), qAbs(qAlpha(a) - qAlpha(b))));
}

QString saveFailure(const QString &name, const QImage &partial, const QImage &full,
                    const QImage &diff)
{
    const QString dir = QDir::tempPath() + QStringLiteral("/softwarerenderer-oracle");
    QDir().mkpath(dir);
    const QString base = dir + QLatin1Char('/') + name;
    partial.save(base + QStringLiteral("-partial.png"));
    full.save(base + QStringLiteral("-full.png"));
    diff.save(base + QStringLiteral("-diff.png"));
    return base;
}

} // namespace

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

// Compares every partially updated frame with a full repaint, and checks
// that the flush region covers every changed pixel.
void tst_SoftwareRenderer::runOracle(const QString &scene, qreal dpr, int seed)
{
    if (QQuickWindow::sceneGraphBackend() != "software")
        QSKIP("Requires the software backend");

    QQmlEngine engine;
    OracleWindow partial;
    OracleWindow full;
    QString error;
    const QUrl url = testFileUrl(scene);
    QVERIFY2(partial.load(&engine, url, dpr, seed, &error), qPrintable(error));
    QVERIFY2(full.load(&engine, url, dpr, seed, &error), qPrintable(error));

    const int frames = partial.root->property("frames").toInt();
    const int tolerance = partial.root->property("tolerance").toInt();
    const QString name = QStringLiteral("%1-dpr%2-seed%3")
            .arg(QFileInfo(scene).baseName()).arg(dpr).arg(seed);

    QImage previous;
    QStringList failures;
    int failedFrames = 0;
    qint64 totalMismatches = 0, totalUnpainted = 0, totalUnflushed = 0;
    for (int frame = 0; frame < frames; ++frame) {
        partial.step(frame);
        full.step(frame);
        partial.render(false);
        full.render(true);

        const QImage &p = partial.target;
        const QImage &f = full.target;
        QCOMPARE(p.size(), f.size());

        const QRegion flushed = QHighDpi::scale(partial.renderer()->flushRegion(), dpr);
        QImage diff(p.size(), QImage::Format_ARGB32);
        diff.fill(Qt::transparent);
        int mismatches = 0;
        int unpainted = 0;
        int unflushed = 0;
        QRect bounds;
        for (int y = 0; y < p.height(); ++y) {
            const QRgb *pl = reinterpret_cast<const QRgb *>(p.constScanLine(y));
            const QRgb *fl = reinterpret_cast<const QRgb *>(f.constScanLine(y));
            const QRgb *prevl = previous.isNull()
                    ? nullptr : reinterpret_cast<const QRgb *>(previous.constScanLine(y));
            QRgb *dl = reinterpret_cast<QRgb *>(diff.scanLine(y));
            for (int x = 0; x < p.width(); ++x) {
                if (fl[x] == Sentinel) {
                    ++unpainted;
                    dl[x] = 0xff00ff00;
                    bounds |= QRect(x, y, 1, 1);
                } else if (maxChannelDiff(pl[x], fl[x]) > tolerance) {
                    ++mismatches;
                    dl[x] = 0xffff0000;
                    bounds |= QRect(x, y, 1, 1);
                }
                if (prevl && prevl[x] != pl[x] && !flushed.contains(QPoint(x, y))) {
                    ++unflushed;
                    dl[x] = 0xff0000ff;
                }
            }
        }
        if (mismatches || unpainted || unflushed) {
            if (failures.size() < 4) {
                const QString base = saveFailure(QStringLiteral("%1-f%2").arg(name).arg(frame),
                                                 p, f, diff);
                failures << QStringLiteral(
                        "frame %1: %2 differ, %3 unpainted, %4 unflushed; bounds %5,%6 %7x%8; %9")
                        .arg(frame).arg(mismatches).arg(unpainted).arg(unflushed)
                        .arg(bounds.x()).arg(bounds.y()).arg(bounds.width())
                        .arg(bounds.height()).arg(base);
            }
            failedFrames++;
            totalMismatches += mismatches;
            totalUnpainted += unpainted;
            totalUnflushed += unflushed;
        }
        previous = p.copy();
    }
    if (failedFrames) {
        QFAIL(qPrintable(QStringLiteral(
                "%1 of %2 frames failed (pixels: %3 differ from a full repaint, %4 never "
                "painted by a full repaint, %5 changed outside the flush region)\n    %6")
                .arg(failedFrames).arg(frames).arg(totalMismatches).arg(totalUnpainted)
                .arg(totalUnflushed).arg(failures.join(QStringLiteral("\n    ")))));
    }
}

static const qreal oracleRatios[] = { 1.0, 1.1, 1.25, 1.5, 1.75, 2.0, 3.0 };

void tst_SoftwareRenderer::partialUpdates_data()
{
    QTest::addColumn<QString>("scene");
    QTest::addColumn<qreal>("dpr");

    const char *scenes[] = {
        "fractionalMove.qml", "moveUnderOpaque.qml", "thinLines.qml", "roundedCorners.qml",
        "layerInvisible.qml", "textStyles.qml", "listScroll.qml", "listLines.qml",
        "listLinesWindowColor.qml", "churn.qml"
    };
    for (const char *scene : scenes) {
        for (qreal dpr : oracleRatios)
            QTest::addRow("%s@%g", scene, dpr) << QString::fromLatin1(scene) << dpr;
    }
}

void tst_SoftwareRenderer::partialUpdates()
{
    QFETCH(QString, scene);
    QFETCH(qreal, dpr);
    runOracle(scene, dpr, 0);
}

void tst_SoftwareRenderer::randomScenes_data()
{
    QTest::addColumn<int>("seed");
    QTest::addColumn<qreal>("dpr");

    // For longer local runs
    bool ok = false;
    int seeds = qEnvironmentVariableIntValue("QT_SOFTWARERENDERER_ORACLE_SEEDS", &ok);
    if (!ok || seeds <= 0)
        seeds = 4;
    for (int seed = 1; seed <= seeds; ++seed) {
        for (qreal dpr : oracleRatios)
            QTest::addRow("seed%d@%g", seed, dpr) << seed << dpr;
    }
}

void tst_SoftwareRenderer::randomScenes()
{
    QFETCH(int, seed);
    QFETCH(qreal, dpr);
    runOracle(QStringLiteral("randomScene.qml"), dpr, seed);
}

#include "tst_softwarerenderer.moc"

QTEST_MAIN(tst_SoftwareRenderer)
