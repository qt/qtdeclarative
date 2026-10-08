// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Measures what it costs the scene graph to hide and show items, e.g. when
// views cull delegates, when a ShaderEffectSource hides its source, or when
// an item is made invisible.
//
// A large static scene is rendered together with a panel of items. Every
// frame, half of the panel's items are hidden and the other half are shown,
// using the mechanism selected with --mode. The CPU time spent in the scene
// graph sync (QQuickWindowPrivate::updateDirtyNode() and friends) and in the
// renderer (from beforeRendering to afterRendering) is reported at the end.
//
// Run the same command line against two builds of Qt Quick to compare them.
// With --rebuild-stats the batch renderer's QSG_RENDERER_DEBUG=render output
// is used to count how often batches were fully rebuilt. That output makes
// the renderer slower, so do not compare timings from such runs.
//
// With --verify, a fixed set of items is hidden, an OpacityAnimator is started
// on one of the hidden items, and the window contents are checked to contain
// only the items that are supposed to be visible.

#include <QtCore/QCommandLineParser>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QtGui/QGuiApplication>
#include <QtGui/QSurfaceFormat>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickView>
#include <QtQuick/QSGRendererInterface>
#include <QtQuick/private/qquickitem_p.h>

#include <algorithm>
#include <atomic>
#include <cstdio>

enum class Mode { Cull, HideSource, Visible, Opacity };

enum RebuildType { RebuildNone, RebuildBatches, RebuildPartial, RebuildRenderLists, RebuildFull,
                   RebuildTypeCount };
static std::atomic<int> rebuildCounts[RebuildTypeCount];
static std::atomic<bool> measuring = false;
static QtMessageHandler previousMessageHandler = nullptr;

static void countRebuilds(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (type != QtDebugMsg) {
        previousMessageHandler(type, context, msg);
        return;
    }
    // Everything else is the renderer's debug output, which is just noise here.
    if (!measuring || !msg.startsWith(QLatin1String("Renderer::render()")))
        return;
    if (msg.contains(QLatin1String("rebuild: full")))
        ++rebuildCounts[RebuildFull];
    else if (msg.contains(QLatin1String("renderlists")))
        ++rebuildCounts[RebuildRenderLists];
    else if (msg.contains(QLatin1String("partial")))
        ++rebuildCounts[RebuildPartial];
    else if (msg.contains(QLatin1String("batches")))
        ++rebuildCounts[RebuildBatches];
    else
        ++rebuildCounts[RebuildNone];
}

static const char *graphicsApiName(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
    case QSGRendererInterface::Software: return "software";
    case QSGRendererInterface::OpenGL: return "opengl";
    case QSGRendererInterface::Direct3D11: return "d3d11";
    case QSGRendererInterface::Direct3D12: return "d3d12";
    case QSGRendererInterface::Vulkan: return "vulkan";
    case QSGRendererInterface::Metal: return "metal";
    default: return "other";
    }
}

class Benchmark : public QObject
{
    Q_OBJECT

public:
    struct Options {
        Mode mode = Mode::Cull;
        int warmupFrames = 100;
        int measuredFrames = 1000;
        bool verify = false;
        bool rebuildStats = false;
        QString description;
    };

    Benchmark(QQuickView *view, const Options &options);

    Q_INVOKABLE void addToggledItem(QQuickItem *item, QQuickItem *effectSource, QObject *animator);

    int result() const { return m_result; }

private:
    void step();
    struct FrameTime {
        qint64 syncNs;
        qint64 renderNs;
    };

    void setHidden(qsizetype index, bool hidden);
    void report();
    bool verifyWindowContents();

    QQuickView *m_view;
    Options m_options;
    QList<QPointer<QQuickItem>> m_items;
    QList<QPointer<QQuickItem>> m_effectSources;
    QPointer<QObject> m_animator;
    int m_frame = 0;
    int m_result = 0;
    bool m_phase = false;

    // Accessed on the render thread only
    QElapsedTimer m_syncTimer;
    QElapsedTimer m_renderTimer;
    qint64 m_syncNs = 0;

    QMutex m_frameTimesMutex;
    QList<FrameTime> m_frameTimes;
};

Benchmark::Benchmark(QQuickView *view, const Options &options)
    : m_view(view),
      m_options(options)
{
    m_frameTimes.reserve(options.measuredFrames + 16);

    // Exactly one step per frame, right before the frame is synchronized.
    // An animation timer would not guarantee that when frames are slow.
    connect(view, &QQuickWindow::afterAnimating, this, &Benchmark::step);

    connect(view, &QQuickWindow::beforeSynchronizing, this, [this] {
        m_syncTimer.start();
    }, Qt::DirectConnection);
    connect(view, &QQuickWindow::afterSynchronizing, this, [this] {
        m_syncNs = m_syncTimer.nsecsElapsed();
    }, Qt::DirectConnection);
    connect(view, &QQuickWindow::beforeRendering, this, [this] {
        m_renderTimer.start();
    }, Qt::DirectConnection);
    connect(view, &QQuickWindow::afterRendering, this, [this] {
        const qint64 renderNs = m_renderTimer.nsecsElapsed();
        if (measuring) {
            QMutexLocker locker(&m_frameTimesMutex);
            m_frameTimes.append({ m_syncNs, renderNs });
        }
    }, Qt::DirectConnection);
}

void Benchmark::addToggledItem(QQuickItem *item, QQuickItem *effectSource, QObject *animator)
{
    m_items.append(item);
    m_effectSources.append(effectSource);
    if (!m_animator)
        m_animator = animator;
}

void Benchmark::setHidden(qsizetype index, bool hidden)
{
    QQuickItem *item = m_items.at(index);
    if (!item)
        return;
    switch (m_options.mode) {
    case Mode::Cull:
        QQuickItemPrivate::get(item)->setCulled(hidden);
        break;
    case Mode::HideSource:
        if (QQuickItem *effectSource = m_effectSources.at(index))
            effectSource->setProperty("hideSource", hidden);
        break;
    case Mode::Visible:
        item->setVisible(!hidden);
        break;
    case Mode::Opacity:
        item->setOpacity(hidden ? 0.0 : 1.0);
        break;
    }
}

void Benchmark::step()
{
    ++m_frame;
    m_view->update();

    if (m_options.verify) {
        if (m_frame == m_options.warmupFrames) {
            // Every even item hidden, every odd item shown. Animating the
            // opacity of a hidden item must not make it visible.
            for (qsizetype i = 0; i < m_items.size(); ++i)
                setHidden(i, i % 2 == 0);
            if (m_animator && m_options.mode != Mode::Opacity)
                QMetaObject::invokeMethod(m_animator, "start");
        } else if (m_frame == m_options.warmupFrames + 10) {
            m_result = verifyWindowContents() ? 0 : 1;
            QCoreApplication::exit(m_result);
        } else if (m_frame < m_options.warmupFrames) {
            m_phase = !m_phase;
            for (qsizetype i = 0; i < m_items.size(); ++i)
                setHidden(i, (i % 2 == 0) == m_phase);
        }
        return;
    }

    if (m_frame == m_options.warmupFrames) {
        measuring = true;
    } else if (m_frame == m_options.warmupFrames + m_options.measuredFrames) {
        measuring = false;
        report();
        QCoreApplication::exit(0);
        return;
    }

    m_phase = !m_phase;
    for (qsizetype i = 0; i < m_items.size(); ++i)
        setHidden(i, (i % 2 == 0) == m_phase);
}

void Benchmark::report()
{
    QList<FrameTime> frameTimes;
    {
        QMutexLocker locker(&m_frameTimesMutex);
        frameTimes = m_frameTimes;
    }
    if (frameTimes.isEmpty()) {
        std::fprintf(stdout, "No frames were measured\n");
        return;
    }

    const auto printStats = [&frameTimes](const char *name, auto value) {
        QList<double> us;
        us.reserve(frameTimes.size());
        double sum = 0;
        for (const FrameTime &t : std::as_const(frameTimes)) {
            us.append(value(t) / 1000.0);
            sum += us.last();
        }
        std::sort(us.begin(), us.end());
        std::fprintf(stdout, "  %-7s mean %9.1f us   median %9.1f us   p95 %9.1f us\n", name,
                     sum / us.size(), us.at(us.size() / 2), us.at(us.size() * 95 / 100));
    };

    std::fprintf(stdout, "%s, backend %s, %lld frames\n", qPrintable(m_options.description),
                 graphicsApiName(m_view->rendererInterface()->graphicsApi()),
                 qint64(frameTimes.size()));
    printStats("sync", [](const FrameTime &t) { return t.syncNs; });
    printStats("render", [](const FrameTime &t) { return t.renderNs; });
    printStats("total", [](const FrameTime &t) { return t.syncNs + t.renderNs; });
    if (m_options.rebuildStats) {
        std::fprintf(stdout, "  rebuilds: full %d, renderlists %d, partial %d, batches %d, none %d"
                             " (timings above are distorted by the renderer's debug output)\n",
                     rebuildCounts[RebuildFull].load(), rebuildCounts[RebuildRenderLists].load(),
                     rebuildCounts[RebuildPartial].load(), rebuildCounts[RebuildBatches].load(),
                     rebuildCounts[RebuildNone].load());
    }
    std::fflush(stdout);
}

bool Benchmark::verifyWindowContents()
{
    const QImage image = m_view->grabWindow();
    const qreal dpr = m_view->effectiveDevicePixelRatio();
    const QColor shownColor(QStringLiteral("steelblue"));
    const QColor hiddenColor(QStringLiteral("#c0e0ff"));
    const auto matches = [](QColor a, QColor b) {
        return qAbs(a.red() - b.red()) < 8 && qAbs(a.green() - b.green()) < 8
                && qAbs(a.blue() - b.blue()) < 8;
    };

    int failures = 0;
    for (qsizetype i = 0; i < m_items.size(); ++i) {
        QQuickItem *item = m_items.at(i);
        if (!item)
            continue;
        // Sample a corner, away from the label
        const QPoint pos = (item->mapToScene(QPointF(4, 4)) * dpr).toPoint();
        if (!image.rect().contains(pos))
            continue;
        const bool expectHidden = i % 2 == 0;
        const QColor actual = image.pixelColor(pos);
        if (!matches(actual, expectHidden ? hiddenColor : shownColor)) {
            std::fprintf(stdout, "  item %lld: expected %s, got %s%s\n", qint64(i),
                         expectHidden ? "hidden" : "shown", qPrintable(actual.name()),
                         i == 0 && expectHidden && m_options.mode != Mode::Opacity
                                 ? " (item has a running OpacityAnimator)" : "");
            ++failures;
        }
    }

    std::fprintf(stdout, "%s, backend %s: %s\n", qPrintable(m_options.description),
                 graphicsApiName(m_view->rendererInterface()->graphicsApi()),
                 failures ? "FAIL" : "PASS");
    std::fflush(stdout);
    return failures == 0;
}

int main(int argc, char *argv[])
{
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption modeOption(QStringLiteral("mode"),
        QStringLiteral("How items are hidden: cull, hidesource, visible or opacity."),
        QStringLiteral("mode"), QStringLiteral("cull"));
    QCommandLineOption cellsOption(QStringLiteral("cells"),
        QStringLiteral("Number of cells in the static part of the scene."),
        QStringLiteral("count"), QStringLiteral("3000"));
    QCommandLineOption itemsOption(QStringLiteral("items"),
        QStringLiteral("Number of items that are hidden and shown."),
        QStringLiteral("count"), QStringLiteral("40"));
    QCommandLineOption framesOption(QStringLiteral("frames"),
        QStringLiteral("Number of measured frames."),
        QStringLiteral("count"), QStringLiteral("1000"));
    QCommandLineOption noClipOption(QStringLiteral("no-clip"),
        QStringLiteral("Do not clip the panel containing the items."));
    QCommandLineOption vsyncOption(QStringLiteral("vsync"),
        QStringLiteral("Keep vsync enabled."));
    QCommandLineOption softwareOption(QStringLiteral("software"),
        QStringLiteral("Use the software backend."));
    QCommandLineOption rebuildStatsOption(QStringLiteral("rebuild-stats"),
        QStringLiteral("Count batch rebuilds. Makes timings meaningless."));
    QCommandLineOption verifyOption(QStringLiteral("verify"),
        QStringLiteral("Check that hidden items are not rendered, instead of measuring."));
    parser.addOptions({ modeOption, cellsOption, itemsOption, framesOption, noClipOption,
                        vsyncOption, softwareOption, rebuildStatsOption, verifyOption });

    QStringList arguments;
    for (int i = 0; i < argc; ++i)
        arguments.append(QString::fromLocal8Bit(argv[i]));
    parser.parse(arguments);

    // Must be done before anything in Qt Quick reads the variable
    if (parser.isSet(rebuildStatsOption))
        qputenv("QSG_RENDERER_DEBUG", "render");

    if (!parser.isSet(vsyncOption)) {
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        format.setSwapInterval(0);
        QSurfaceFormat::setDefaultFormat(format);
    }

    QGuiApplication app(argc, argv);
    parser.process(app);

    Benchmark::Options options;
    const QString mode = parser.value(modeOption);
    if (mode == QLatin1String("cull")) {
        options.mode = Mode::Cull;
    } else if (mode == QLatin1String("hidesource")) {
        options.mode = Mode::HideSource;
    } else if (mode == QLatin1String("visible")) {
        options.mode = Mode::Visible;
    } else if (mode == QLatin1String("opacity")) {
        options.mode = Mode::Opacity;
    } else {
        std::fprintf(stderr, "Unknown mode %s\n", qPrintable(mode));
        return 2;
    }
    options.measuredFrames = parser.value(framesOption).toInt();
    options.verify = parser.isSet(verifyOption);
    options.rebuildStats = parser.isSet(rebuildStatsOption);
    const int cells = parser.value(cellsOption).toInt();
    const int items = parser.value(itemsOption).toInt();
    const bool clip = !parser.isSet(noClipOption);
    options.description = QStringLiteral("mode %1, %2 items, %3 cells, clip %4")
            .arg(mode).arg(items).arg(cells).arg(clip ? QStringLiteral("on") : QStringLiteral("off"));

    if (options.rebuildStats)
        previousMessageHandler = qInstallMessageHandler(countRebuilds);

    if (parser.isSet(softwareOption))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);

    QQuickView view;
    Benchmark benchmark(&view, options);
    qmlRegisterSingletonInstance("HiddenItems", 1, 0, "Benchmark", &benchmark);

    view.setInitialProperties({
        { QStringLiteral("cellCount"), cells },
        { QStringLiteral("toggledCount"), items },
        { QStringLiteral("clipPanel"), clip },
        { QStringLiteral("useEffectSource"), options.mode == Mode::HideSource },
    });
    view.setSource(QUrl(QStringLiteral("qrc:/main.qml")));
    if (view.status() != QQuickView::Ready)
        return 2;
    view.show();

    const int exitCode = app.exec();
    return options.verify ? benchmark.result() : exitCode;
}

#include "main.moc"
