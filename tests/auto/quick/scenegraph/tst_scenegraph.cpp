// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>

#if QT_CONFIG(opengl)
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#endif

#include <QtQuick>
#include <QtQml>

#if QT_CONFIG(opengl)
#include <private/qopenglcontext_p.h>
#endif

#include <private/qsgcontext_p.h>
#include <private/qsgrenderloop_p.h>
#include <private/qsgrhisupport_p.h>
#include <private/qsgplaintexture_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

using namespace QQuickVisualTestUtils;

class PerPixelRect : public QQuickItem
{
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_OBJECT
public:
    PerPixelRect() {
        setFlag(ItemHasContents);
    }

    void setColor(const QColor &c) {
        if (c == m_color)
            return;
        m_color = c;
        emit colorChanged(c);
    }

    QColor color() const { return m_color; }

    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *) override
    {
        delete node;
        node = new QSGNode;

        const int w = int(width());
        const int h = int(height());
        QQuickWindow *win = window();
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                QSGRectangleNode *rn = win->createRectangleNode();
                rn->setRect(x, y, 1, 1);
                rn->setColor(m_color);
                node->appendChildNode(rn);
            }
        }

        return node;
    }

Q_SIGNALS:
    void colorChanged(const QColor &c);

private:
    QColor m_color;
};

class tst_SceneGraph : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_SceneGraph();

private slots:
    void initTestCase() override;

    void manyWindows_data();
    void manyWindows();

    void render_data();
    void render();
#if QT_CONFIG(opengl)
    void hideWithOtherContext();
#endif
    void createTextureFromImage_data();
    void createTextureFromImage();
    void withAdoptedRhi();
    void resizeTextureFromImage();

private:
    QQuickView *createView(const QString &file, QWindow *parent = nullptr, int x = -1, int y = -1, int w = -1, int h = -1);
    bool isRunningOnRhi();
};

template <typename T> class ScopedList : public QList<T> {
public:
    ~ScopedList() { qDeleteAll(*this); }
};

tst_SceneGraph::tst_SceneGraph()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_SceneGraph::initTestCase()
{
    qmlRegisterType<PerPixelRect>("SceneGraphTest", 1, 0, "PerPixelRect");

    QQmlDataTest::initTestCase();

    QSGRenderLoop *loop = QSGRenderLoop::instance();
    qDebug() << "RenderLoop:" << loop
             << "RHI backend:" << QSGRhiSupport::instance()->rhiBackendName();
}

QQuickView *tst_SceneGraph::createView(const QString &file, QWindow *parent, int x, int y, int w, int h)
{
    QQuickView *view = new QQuickView(parent);
    view->setSource(testFileUrl(file));
    if (x >= 0 && y >= 0) view->setPosition(x, y);
    if (w >= 0 && h >= 0) view->resize(w, h);
    view->show();
    return view;
}

// Assumes the images are opaque white...
bool containsSomethingOtherThanWhite(const QImage &image)
{
    QImage img;
    if (image.format() != QImage::Format_ARGB32_Premultiplied
             || image.format() != QImage::Format_RGB32)
        img = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    else
        img = image;

    int w = img.width();
    int h = img.height();
    for (int y=0; y<h; ++y) {
        const uint *pixels = (const uint *) img.constScanLine(y);
        for (int x=0; x<w; ++x)
            if (pixels[x] != 0xffffffff)
                return true;
    }
    return false;
}

void tst_SceneGraph::manyWindows_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<bool>("toplevel");
    QTest::addColumn<bool>("shared");

    QTest::newRow("image,toplevel") << QStringLiteral("manyWindows_image.qml") << true << false;
    QTest::newRow("image,subwindow") << QStringLiteral("manyWindows_image.qml") << false << false;
    QTest::newRow("dftext,toplevel") << QStringLiteral("manyWindows_dftext.qml") << true << false;
    QTest::newRow("dftext,subwindow") << QStringLiteral("manyWindows_dftext.qml") << false << false;
    QTest::newRow("ntext,toplevel") << QStringLiteral("manyWindows_ntext.qml") << true << false;
    QTest::newRow("ntext,subwindow") << QStringLiteral("manyWindows_ntext.qml") << false << false;
    QTest::newRow("rects,toplevel") << QStringLiteral("manyWindows_rects.qml") << true << false;
    QTest::newRow("rects,subwindow") << QStringLiteral("manyWindows_rects.qml") << false << false;

    QTest::newRow("image,toplevel,sharing") << QStringLiteral("manyWindows_image.qml") << true << true;
    QTest::newRow("image,subwindow,sharing") << QStringLiteral("manyWindows_image.qml") << false << true;
    QTest::newRow("dftext,toplevel,sharing") << QStringLiteral("manyWindows_dftext.qml") << true << true;
    QTest::newRow("dftext,subwindow,sharing") << QStringLiteral("manyWindows_dftext.qml") << false << true;
    QTest::newRow("ntext,toplevel,sharing") << QStringLiteral("manyWindows_ntext.qml") << true << true;
    QTest::newRow("ntext,subwindow,sharing") << QStringLiteral("manyWindows_ntext.qml") << false << true;
    QTest::newRow("rects,toplevel,sharing") << QStringLiteral("manyWindows_rects.qml") << true << true;
    QTest::newRow("rects,subwindow,sharing") << QStringLiteral("manyWindows_rects.qml") << false << true;
}

#if QT_CONFIG(opengl)
struct ShareContextResetter {
public:
    ~ShareContextResetter() { qt_gl_set_global_share_context(nullptr); }
};
#endif

void tst_SceneGraph::manyWindows()
{
    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");

    QFETCH(QString, file);
    QFETCH(bool, toplevel);
    QFETCH(bool, shared);
#if QT_CONFIG(opengl)
    QOpenGLContext sharedGLContext;
    ShareContextResetter cleanup; // To avoid dangling pointer in case of test-failure.
    if (shared) {
        QVERIFY(sharedGLContext.create());
        qt_gl_set_global_share_context(&sharedGLContext);
    }
#endif
    QScopedPointer<QWindow> parent;
    if (!toplevel) {
        parent.reset(new QWindow());
        parent->resize(200, 200);
        parent->show();
    }

    ScopedList <QQuickView *> views;

    const int COUNT = 4;

    QImage baseLine;
    QString errorMessage;
    for (int i=0; i<COUNT; ++i) {
        views << createView(file, parent.data(), (i % 2) * 100, (i / 2) * 100, 100, 100);
    }
    for (int i=0; i<COUNT; ++i) {
        QQuickView *view = views.at(i);
        QVERIFY(QTest::qWaitForWindowExposed(view));
        QImage content = view->grabWindow();
        if (i == 0) {
            baseLine = content;
            QVERIFY(containsSomethingOtherThanWhite(baseLine));
        } else {
            QVERIFY2(compareImages(content, baseLine, &errorMessage),
                     qPrintable(errorMessage));
        }
    }

    // Wipe and recreate one (scope pointer delets it...)
    delete views.takeLast();
    QQuickView *last = createView(file, parent.data(), 100, 100, 100, 100);
    QVERIFY(QTest::qWaitForWindowExposed(last));
    views << last;
    QVERIFY2(compareImages(baseLine, last->grabWindow(), &errorMessage),
             qPrintable(errorMessage));

    // Wipe and recreate all
    qDeleteAll(views);
    views.clear();

    for (int i=0; i<COUNT; ++i) {
        views << createView(file, parent.data(), (i % 2) * 100, (i / 2) * 100, 100, 100);
    }
    for (int i=0; i<COUNT; ++i) {
        QQuickView *view = views.at(i);
        QVERIFY(QTest::qWaitForWindowExposed(view));
        QImage content = view->grabWindow();
        QVERIFY2(compareImages(content, baseLine, &errorMessage),
                 qPrintable(errorMessage));
    }
}

struct Sample {
    constexpr Sample(int xx, int yy, qreal rr, qreal gg, qreal bb, qreal errorMargin = 0.05)
        : x(xx)
        , y(yy)
        , r(rr)
        , g(gg)
        , b(bb)
        , tolerance(errorMargin)
    {
    }
    constexpr Sample(const Sample &o) : x(o.x), y(o.y), r(o.r), g(o.g), b(o.b), tolerance(o.tolerance) { }
    constexpr Sample() : x(0), y(0), r(0), g(0), b(0), tolerance(0) { }
    constexpr Sample &operator=(const Sample &o)
    {
        x = o.x;
        y = o.y;
        r = o.r;
        g = o.g;
        b = o.b;
        tolerance = o.tolerance;
        return *this;
    }

    QString toString(const QImage &image) const {
        QColor color(image.pixel(x,y));
        return QString::fromLatin1("pixel(%1,%2), rgb(%3,%4,%5), tolerance=%6 -- image(%7,%8,%9)")
                .arg(x).arg(y)
                .arg(r).arg(g).arg(b)
                .arg(tolerance)
                .arg(color.redF()).arg(color.greenF()).arg(color.blueF());
    }

    bool check(const QImage &image, qreal scale) {
        const int scaledX = qRound(x * scale);
        const int scaledY = qRound(y * scale);
        const QColor color(image.pixel(scaledX, scaledY));
        return qAbs(color.redF() - r) <= tolerance
                && qAbs(color.greenF() - g) <= tolerance
                && qAbs(color.blueF() - b) <= tolerance;
    }


    int x, y;
    qreal r, g, b;
    qreal tolerance;
};

static Sample sample_from_regexp(QRegularExpressionMatch *match) {
    return Sample(match->captured(1).toInt(),
                  match->captured(2).toInt(),
                  match->captured(3).toFloat(),
                  match->captured(4).toFloat(),
                  match->captured(5).toFloat(),
                  match->captured(6).toFloat()
                  );
}

Q_DECLARE_METATYPE(Sample);

/*
  The render() test implements a small test framework for itself with
  the purpose of testing odds and ends of the scene graph
  rendering. Each .qml file can consist of one or two stages. The
  first stage is when the file is first displayed. The content is
  grabbed and matched against 'base' samples defined in the .qml file
  itself.  The samples contain a pixel position, and RGB value and a
  margin of error. The samples are defined in the .qml file so it is
  easy to make the connection between colors and positions on the screen.

  If the base stage samples all succeed, the test emits
  'enterFinalStage' on the root item and waits for the .qml file to
  update the value of 'finalStageComplete' The test can set this
  directly or run an animation and set it later. Once the
  'finalStageComplete' variable is true, we grab and match against the
  second set of samples 'final'

  The samples in the .qml file are defined in comments on the format:
      #base: x y r g b error-tolerance
      #final: x y r g b error-tolerance
      - x and y are integers
      - r g b are floats in the range of 0.0-1.0
      - error-tolerance is a float in the range of 0.0-1.0

  We also include a
      #samples: count
  to sanity check that all base/final samples were matched correctly
  as the matching regexp is a bit crude.

  To add new tests, add them to the 'files' list and put #base,
  #final, #samples tags into the .qml file
*/

void tst_SceneGraph::render_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QList<Sample> >("baseStage");
    QTest::addColumn<QList<Sample> >("finalStage");

    QList<QString> files;
    files << "render_DrawSets.qml"
          << "render_Overlap.qml"
          << "render_MovingOverlap.qml"
          << "render_BreakOpacityBatch.qml"
          << "render_OutOfFloatRange.qml"
          << "render_StackingOrder.qml"
          << "render_ImageFiltering.qml"
          << "render_bug37422.qml"
          << "render_OpacityThroughBatchRoot.qml"
          << "render_Mipmap.qml"
          << "render_AlphaOverlapRebuild.qml";

    QRegularExpression sampleCount("#samples: *(\\d+)");
    //                          X:int   Y:int   R:float       G:float       B:float       Error:float
    QRegularExpression baseSamples("#base: *(\\d+) *(\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+)");
    QRegularExpression finalSamples("#final: *(\\d+) *(\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+)");

    foreach (QString fileName, files) {
        QFile file(testFile(fileName));
        if (!file.open(QFile::ReadOnly)) {
            qFatal("render_data: QFile::open failed! file=%s, error=%s",
                   qPrintable(fileName), qPrintable(file.errorString()));
        }
        QStringList contents = QString::fromLatin1(file.readAll()).split(QLatin1Char('\n'));

        int samples = -1;
        foreach (QString line, contents) {
            auto match = sampleCount.match(line);
            if (match.hasMatch()) {
                samples = match.captured(1).toInt();
                break;
            }
        }
        if (samples == -1)
            qFatal("render_data: failed to find string '#samples: [count], file=%s", qPrintable(fileName));

        QList<Sample> baseStage, finalStage;
        foreach (QString line, contents) {
            auto match = baseSamples.match(line);
            if (match.hasMatch())
                baseStage << sample_from_regexp(&match);
            else if ((match = finalSamples.match(line)).hasMatch())
                finalStage << sample_from_regexp(&match);
        }

        if (baseStage.size() + finalStage.size() != samples)
            qFatal("render_data: #samples does not add up to number of counted samples, file=%s", qPrintable(fileName));

        QTest::newRow(qPrintable(fileName)) << fileName << baseStage << finalStage;
    }
}

void tst_SceneGraph::render()
{
    if (!isRunningOnRhi())
        QSKIP("Skipping complex rendering tests due to not running with QRhi");

    QFETCH(QString, file);
    QFETCH(QList<Sample>, baseStage);
    QFETCH(QList<Sample>, finalStage);

    QObject suite;
    suite.setObjectName("The Suite");

    QQuickView view;
    view.rootContext()->setContextProperty("suite", &suite);
    view.setSource(testFileUrl(file));
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // We're checking actual pixels, so scale up the sample point to the top-left of the
    // 2x2 pixel block and hope that this is good enough. Ideally, view and content
    // would be in identical coordinate space, meaning pixels, but we're not in an
    // ideal world.
    // Just keep this in mind when writing tests.
    qreal scale = view.devicePixelRatio();
    const bool isIntegerScale = qFuzzyIsNull(qreal(qFloor(scale)) - scale);

    if (file == "render_OutOfFloatRange.qml" && !isIntegerScale)
        QSKIP("render_OutOfFloatRange doesn't work with non-integer scaling factors");

    // Grab the window and check all our base stage samples
    QImage content = view.grabWindow();
    for (int i=0; i<baseStage.size(); ++i) {
        Sample sample = baseStage.at(i);
        QVERIFY2(sample.check(content, scale), qPrintable(sample.toString(content)));
    }

    // Put the qml file into the final stage and wait for it to
    // complete it.
    QQuickItem *rootItem = view.rootObject();
    QMetaObject::invokeMethod(rootItem, "enterFinalStage");
    QTRY_VERIFY(rootItem->property("finalStageComplete").toBool());

    // The grab the results and verify the samples in the end state.
    content = view.grabWindow();
    for (int i=0; i<finalStage.size(); ++i) {
        Sample sample = finalStage.at(i);
        QVERIFY2(sample.check(content, scale), qPrintable(sample.toString(content)));
    }
}

#if QT_CONFIG(opengl)
// Testcase for QTBUG-34898. We make another context current on another surface
// in the GUI thread and hide the QQuickWindow while the other context is
// current on the other window.
void tst_SceneGraph::hideWithOtherContext()
{
    if (!isRunningOnRhi())
        QSKIP("Skipping OpenGL context test due to not running with QRhi");

    QWindow window;
    window.setSurfaceType(QWindow::OpenGLSurface);
    window.resize(100, 100);
    window.create();
    QOpenGLContext context;
    QVERIFY(context.create());
    bool renderingOnMainThread = false;

    {
        QQuickView view;
        view.setSource(testFileUrl("simple.qml"));
        view.setResizeMode(QQuickView::SizeViewToRootObject);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        if (view.rendererInterface()->graphicsApi() != QSGRendererInterface::OpenGLRhi)
            QSKIP("Skipping OpenGL context test due to not using OpenGL");

        QOpenGLContext *ctx = static_cast<QOpenGLContext *>(view.rendererInterface()->getResource(
                                                                &view, QSGRendererInterface::OpenGLContextResource));
        renderingOnMainThread = ctx->thread() == QGuiApplication::instance()->thread();

        // Make the local context current on the local window...
        context.makeCurrent(&window);
    }

    // The local context should no longer be the current one. It is not
    // rebound because all well behaving Qt/OpenGL applications are
    // required to makeCurrent their context again before making any
    // GL calls to a new frame (see QOpenGLContext docs).
    QVERIFY(!renderingOnMainThread || QOpenGLContext::currentContext() != &context);
}
#endif

void tst_SceneGraph::createTextureFromImage_data()
{
    QImage rgba(64, 64, QImage::Format_ARGB32_Premultiplied);
    QImage rgb(64, 64, QImage::Format_RGB32);

    QTest::addColumn<QImage>("image");
    QTest::addColumn<uint>("flags");
    QTest::addColumn<bool>("expectedAlpha");

    QTest::newRow("rgb") << rgb << uint(0) << false;
    QTest::newRow("argb") << rgba << uint(0) << true;
    QTest::newRow("rgb,alpha") << rgb << uint(QQuickWindow::TextureHasAlphaChannel) << false;
    QTest::newRow("argb,alpha") << rgba << uint(QQuickWindow::TextureHasAlphaChannel) << true;
    QTest::newRow("rgb,!alpha") << rgb << uint(QQuickWindow::TextureIsOpaque) << false;
    QTest::newRow("argb,!alpha") << rgba << uint(QQuickWindow::TextureIsOpaque) << false;
}

void tst_SceneGraph::createTextureFromImage()
{
    QFETCH(QImage, image);
    QFETCH(uint, flags);
    QFETCH(bool, expectedAlpha);

    QQuickView view;
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QTRY_VERIFY(view.isSceneGraphInitialized());

    QScopedPointer<QSGTexture> texture(view.createTextureFromImage(image, (QQuickWindow::CreateTextureOptions) flags));
    QCOMPARE(texture->hasAlphaChannel(), expectedAlpha);
}

#if QT_CONFIG(vulkan)
static QVulkanInstance *TestOffscreenScene_vkinst = nullptr;
#endif

struct TestOffscreenScene
{
    QQuickRenderControl *renderControl = nullptr;
    QQuickWindow *window = nullptr;
    QQmlEngine *engine = nullptr;
    QQmlComponent *component = nullptr;
    QQuickItem *rootItem = nullptr;

    // to be called at the end of each test case once all TestOffscreenScene instances are destroyed
    static void cleanup()
    {
#if QT_CONFIG(vulkan)
        delete TestOffscreenScene_vkinst;
        TestOffscreenScene_vkinst = nullptr;
#endif
    }

    ~TestOffscreenScene()
    {
        delete component;
        delete engine;
        delete window;
        delete renderControl;
    }
};

static TestOffscreenScene *createOffscreenScene(const QUrl &url, QQuickWindow *compatibleWindow = nullptr)
{
    std::unique_ptr<TestOffscreenScene> scene(new TestOffscreenScene);
    scene->renderControl = new QQuickRenderControl;
    scene->window = new QQuickWindow(scene->renderControl);

    if (compatibleWindow) {
        scene->window->setGraphicsApi(compatibleWindow->rendererInterface()->graphicsApi());

#if QT_CONFIG(vulkan)
        if (compatibleWindow->rendererInterface()->graphicsApi() == QSGRendererInterface::VulkanRhi)
            scene->window->setVulkanInstance(compatibleWindow->vulkanInstance());
#endif

        QRhi *rhi = static_cast<QRhi *>(compatibleWindow->rendererInterface()->getResource(compatibleWindow, QSGRendererInterface::RhiResource));
        if (rhi) {
            // make it so that the rendercontrol will not create a new QRhi, but rather use what we specify here
            scene->window->setGraphicsDevice(QQuickGraphicsDevice::fromRhi(rhi));
        } else {
            qWarning("No QRhi from the specified compatibleWindow, this is unexpected");
        }
    } else {
#if QT_CONFIG(vulkan)
        if (QQuickWindow::graphicsApi() == QSGRendererInterface::Vulkan) { // honor what QSG_RHI_BACKEND says
            if (!TestOffscreenScene_vkinst) {
                TestOffscreenScene_vkinst = new QVulkanInstance;
                TestOffscreenScene_vkinst->setExtensions(QQuickGraphicsConfiguration::preferredInstanceExtensions());
                TestOffscreenScene_vkinst->create();
            }
            scene->window->setVulkanInstance(TestOffscreenScene_vkinst);
        }
#endif
    }

    scene->engine = new QQmlEngine;
    scene->component = new QQmlComponent(scene->engine, url);
    if (scene->component->isError()) {
        for (const QQmlError &error : scene->component->errors())
            qWarning() << error.url() << error.line() << error;
        return nullptr;
    }

    QObject *rootObject = scene->component->create();
    if (scene->component->isError()) {
        for (const QQmlError &error : scene->component->errors())
            qWarning() << error.url() << error.line() << error;
    }

    scene->rootItem = qobject_cast<QQuickItem *>(rootObject);
    if (!scene->rootItem) {
        qWarning("No root QQuickItem");
        return nullptr;
    }

    scene->window->contentItem()->setSize(scene->rootItem->size());
    scene->window->setGeometry(0, 0, scene->rootItem->width(), scene->rootItem->height());
    scene->rootItem->setParentItem(scene->window->contentItem());

    if (!scene->renderControl->initialize()) {
        qWarning("Failed to initialize rendercontrol");
        return nullptr;
    }

    return scene.release();
}

void tst_SceneGraph::withAdoptedRhi()
{
    if (!isRunningOnRhi())
        QSKIP("Skipping test due to not running with QRhi");

    // Use only QQuickRenderControl-based, single-threaded scenes for this
    // test. QQuickView would not be suitable because it it uses the threaded
    // render loop, then we end up in threading issues with graphics resources.

    TestOffscreenScene *scene1 = createOffscreenScene(testFileUrl(QLatin1String("renderControl_rect.qml")));
    QVERIFY(scene1->renderControl && scene1->window && scene1->rootItem);

    // Now another one, but this time sharing the same QRhi as the first one.
    TestOffscreenScene *scene2 = createOffscreenScene(testFileUrl(QLatin1String("renderControl_rect.qml")), scene1->window);
    QVERIFY(scene2->renderControl && scene2->window && scene2->rootItem);

    QRhi *rhi = static_cast<QRhi *>(scene1->window->rendererInterface()->getResource(scene1->window, QSGRendererInterface::RhiResource));
    QCOMPARE(rhi, static_cast<QRhi *>(scene2->window->rendererInterface()->getResource(scene2->window, QSGRendererInterface::RhiResource)));

    { // scope to get resources destroyed before the QRhi
        const QSize size = scene1->rootItem->size().toSize();
        QCOMPARE(size, scene2->rootItem->size().toSize());
        QScopedPointer<QRhiRenderBuffer> ds(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size, 1));
        QVERIFY(ds->create());

        // texture for scene1
        QScopedPointer<QRhiTexture> tex1(rhi->newTexture(QRhiTexture::RGBA8, size, 1,
                                                         QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        QVERIFY(tex1->create());
        QRhiTextureRenderTargetDescription rtDesc1(QRhiColorAttachment(tex1.data()));
        rtDesc1.setDepthStencilBuffer(ds.data());
        QScopedPointer<QRhiTextureRenderTarget> texRt1(rhi->newTextureRenderTarget(rtDesc1));
        QScopedPointer<QRhiRenderPassDescriptor> rp1(texRt1->newCompatibleRenderPassDescriptor());
        texRt1->setRenderPassDescriptor(rp1.data());
        QVERIFY(texRt1->create());
        scene1->window->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(texRt1.data()));

        // for scene2
        QScopedPointer<QRhiTexture> tex2(rhi->newTexture(QRhiTexture::RGBA8, size, 1,
                                                         QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        QVERIFY(tex2->create());
        QRhiTextureRenderTargetDescription rtDesc2(QRhiColorAttachment(tex2.data()));
        rtDesc2.setDepthStencilBuffer(ds.data());
        QScopedPointer<QRhiTextureRenderTarget> texRt2(rhi->newTextureRenderTarget(rtDesc2));
        QScopedPointer<QRhiRenderPassDescriptor> rp2(texRt2->newCompatibleRenderPassDescriptor());
        texRt2->setRenderPassDescriptor(rp2.data());
        QVERIFY(texRt2->create());
        scene2->window->setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(texRt2.data()));

        // render a frame, first with scene1, then with scene2, targeting their respective textures
        scene1->renderControl->polishItems();
        scene1->renderControl->beginFrame();
        scene1->renderControl->sync();
        scene1->renderControl->render();
        scene1->renderControl->endFrame();

        scene2->renderControl->polishItems();
        scene2->renderControl->beginFrame();
        scene2->renderControl->sync();
        scene2->renderControl->render();
        scene2->renderControl->endFrame();

        // Both tex1 and tex2 belong to the same one QRhi. Read back the
        // contents. In a real world application one could now render with the
        // QRhi to some other window, using both textures.
        for (int stage = 0; stage < 2; ++stage) {
            QRhiCommandBuffer *cb = nullptr;
            rhi->beginOffscreenFrame(&cb);
            bool readCompleted = false;
            QRhiReadbackResult readResult;
            QImage result;
            readResult.completed = [&readCompleted, &readResult, &result, &rhi] {
                readCompleted = true;
                QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                                    readResult.pixelSize.width(), readResult.pixelSize.height(),
                                    QImage::Format_RGBA8888_Premultiplied);
                if (rhi->isYUpInFramebuffer())
                    result = wrapperImage.mirrored();
                else
                    result = wrapperImage.copy();
            };
            QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
            readbackBatch->readBackTexture(stage == 0 ? tex1.data() : tex2.data(), &readResult);
            cb->resourceUpdate(readbackBatch);
            rhi->endOffscreenFrame();

            QVERIFY(readCompleted);
            QCOMPARE(result.size(), QSize(200, 200));
            const int maxFuzz = 2;
            // red
            QVERIFY(qAbs(qRed(result.pixel(100, 100)) - 255) < maxFuzz);
            QVERIFY(qAbs(qGreen(result.pixel(100, 100))) < maxFuzz);
            QVERIFY(qAbs(qBlue(result.pixel(100, 100))) < maxFuzz);
        }
    }

    delete scene2; // this does not destroy the QRhi
    // call anything on the QRhi just to test if it is still valid
    QVERIFY(!rhi->isDeviceLost());
    delete scene1; // this does

    TestOffscreenScene::cleanup();
}

static inline void commitTexture(QRhi *rhi, QSGTexture *texture)
{
    QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
    texture->commitTextureOperations(rhi, rub);
    QRhiCommandBuffer *cb = nullptr;
    rhi->beginOffscreenFrame(&cb);
    cb->resourceUpdate(rub);
    rhi->endOffscreenFrame();
}

void tst_SceneGraph::resizeTextureFromImage()
{
    if (!isRunningOnRhi())
        QSKIP("Skipping test due to not running with QRhi");

    // We will need to directly work with QSGTexture and QRhi so have
    // to be on the same thread as the scene graph. Hence using the
    // offscreen infrastructure from other tests.

    // note the lifetimes: (vulkan instance) > scenegraph(incl. QRhi) > QSGTexture
    {
        QScopedPointer<TestOffscreenScene> scene(createOffscreenScene(testFileUrl(QLatin1String("renderControl_rect.qml"))));
        QVERIFY(scene->renderControl && scene->window && scene->rootItem);

        {
            QImage image(256, 128, QImage::Format_RGBA8888);
            QScopedPointer<QSGTexture> texture(scene->window->createTextureFromImage(image, QQuickWindow::TextureHasAlphaChannel));
            QRhi *rhi = static_cast<QRhi *>(scene->window->rendererInterface()->getResource(scene->window, QSGRendererInterface::RhiResource));
            QVERIFY(rhi);
            commitTexture(rhi, texture.data());
            // neither is too big nor relies on optional features like NPoT repeat so the size should match always
            QCOMPARE(texture->textureSize(), image.size());
            QCOMPARE(texture->rhiTexture()->pixelSize(), image.size());

            QSGPlainTexture *plainTex = qobject_cast<QSGPlainTexture *>(texture.data());
            QVERIFY(plainTex);

            // QTBUG-96190 - the commitTexture call here used to crash due to not
            // updating the QRhiTexture size correctly in QSGPlainTexture.
            image = QImage(512, 256, QImage::Format_RGBA8888);
            plainTex->setImage(image);
            commitTexture(rhi, texture.data());
            QCOMPARE(texture->textureSize(), image.size());
            QCOMPARE(texture->rhiTexture()->pixelSize(), image.size());
        }
    }

    TestOffscreenScene::cleanup();
}

bool tst_SceneGraph::isRunningOnRhi()
{
    static bool retval = false;
    static bool decided = false;
    if (!decided) {
        decided = true;
        QQuickView dummy;
        dummy.show();
        if (!QTest::qWaitForWindowExposed(&dummy)) {
            [](){ QFAIL("Could not show a QQuickView"); }();
            return false;
        }
        QSGRendererInterface::GraphicsApi api = dummy.rendererInterface()->graphicsApi();
        retval = QSGRendererInterface::isApiRhiBased(api);
        dummy.hide();
    }
    return retval;
}

#include "tst_scenegraph.moc"

QTEST_MAIN(tst_SceneGraph)

