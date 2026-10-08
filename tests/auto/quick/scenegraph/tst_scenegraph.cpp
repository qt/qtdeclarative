// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>

#if QT_CONFIG(opengl)
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#endif

#include <QtQuick>
#include <QtQml>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtTest/QSignalSpy>

#if QT_CONFIG(opengl)
#include <private/qopenglcontext_p.h>
#endif

#include <QtGui/qrawfont.h>
#include <QtGui/qtextdocument.h>

#include <QtQuick/qsgtextnode.h>

#include <private/qsgcontext_p.h>
#include <private/qsgrenderloop_p.h>
#include <private/qsgrhisupport_p.h>
#include <private/qsgplaintexture_p.h>
#include <private/qsgnode_p.h>
#include <private/qquickitem_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>

using namespace Qt::StringLiterals;
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
    void textureNativeInterface();
    void distanceFieldCacheInvalidation();
    void pregeneratedDistanceFieldCache_data();
    void pregeneratedDistanceFieldCache();
    void unexposeDuringPolish();

#ifdef QT_BUILD_INTERNAL
    void mutabilityGroups();
#endif

    void sgTextNode_data();
    void sgTextNode();
    void sgInternalTextNodeRecycle_data();
    void sgInternalTextNodeRecycle();
    void sgInternalTextNodeRecycleBatchRoot_data();
    void sgInternalTextNodeRecycleBatchRoot();

    void hiddenItemBlocksNode_data();
    void hiddenItemBlocksNode();
    void changeWhileHidden_data();
    void changeWhileHidden();
    void hiddenItemMultipleReasons_data();
    void hiddenItemMultipleReasons();
    void hiddenItemWindowChange_data();
    void hiddenItemWindowChange();

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
    SKIP_IF_NO_WINDOW_GRAB;
    if (QGuiApplication::platformName() == QLatin1String("offscreen")) {
        QSKIP("Skipping due to createPlatformOpenGLContext returning nullptr with "
            "offscreen platform plugin on non-x11 platforms like QNX");
    }

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

    const QString files[] = {
        u"render_DrawSets.qml"_s,
        u"render_Overlap.qml"_s,
        u"render_MovingOverlap.qml"_s,
        u"render_BreakOpacityBatch.qml"_s,
        u"render_OutOfFloatRange.qml"_s,
        u"render_StackingOrder.qml"_s,
        u"render_ImageFiltering.qml"_s,
        u"render_bug37422.qml"_s,
        u"render_OpacityThroughBatchRoot.qml"_s,
        u"render_Mipmap.qml"_s,
        u"render_AlphaOverlapRebuild.qml"_s,
        u"render_MutabilityGroups.qml"_s,
        u"render_HideFirstInBatchVisible.qml"_s,
        u"render_HideFirstInBatchOpacity.qml"_s,
        u"render_HideFirstInBatchHideSource.qml"_s,
    };

    QRegularExpression sampleCount("#samples: *(\\d+)");
    //                          X:int   Y:int   R:float       G:float       B:float       Error:float
    QRegularExpression baseSamples("#base: *(\\d+) *(\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+)");
    QRegularExpression finalSamples("#final: *(\\d+) *(\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+) *(\\d\\.\\d+)");

    for (const QString &fileName : files) {
        QFile file(testFile(fileName));
        if (!file.open(QFile::ReadOnly)) {
            qFatal("render_data: QFile::open failed! file=%s, error=%s",
                   qPrintable(fileName), qPrintable(file.errorString()));
        }
        const QStringList contents = QString::fromLatin1(file.readAll()).split(QLatin1Char('\n'));

        int samples = -1;
        for (const QString &line : contents) {
            auto match = sampleCount.match(line);
            if (match.hasMatch()) {
                samples = match.captured(1).toInt();
                break;
            }
        }
        if (samples == -1)
            qFatal("render_data: failed to find string '#samples: [count], file=%s", qPrintable(fileName));

        QList<Sample> baseStage, finalStage;
        for (const QString &line : contents) {
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
                    result = wrapperImage.flipped();
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

void tst_SceneGraph::textureNativeInterface()
{
    if (!isRunningOnRhi())
        QSKIP("Skipping texture native interface tests due to not running with QRhi");

    // test it offscreen because we want to do QSG stuff here on the main thread
    QScopedPointer<TestOffscreenScene> scene(createOffscreenScene(testFileUrl(QLatin1String("renderControl_rect.qml"))));
    QVERIFY(scene->renderControl && scene->window && scene->rootItem);

    QImage image(512, 512, QImage::Format_RGBA8888);

    QScopedPointer<QSGTexture> texture(scene->window->createTextureFromImage(image, QQuickWindow::TextureHasAlphaChannel));
    QVERIFY(texture.data());

    commitTexture(scene->window->rhi(), texture.data());

#if QT_CONFIG(metal)
    if (scene->window->graphicsApi() == QSGRendererInterface::Metal) {
        auto texNatIf = texture->nativeInterface<QNativeInterface::QSGMetalTexture>();
        QVERIFY(texNatIf);
        QVERIFY(texNatIf->nativeTexture());
    }
#endif

#if defined(Q_OS_WIN)
    if (scene->window->graphicsApi() == QSGRendererInterface::Direct3D11) {
        auto texNatIf = texture->nativeInterface<QNativeInterface::QSGD3D11Texture>();
        QVERIFY(texNatIf);
        QVERIFY(texNatIf->nativeTexture());
    } else if (scene->window->graphicsApi() == QSGRendererInterface::Direct3D12) {
        auto texNatIf = texture->nativeInterface<QNativeInterface::QSGD3D12Texture>();
        QVERIFY(texNatIf);
        QVERIFY(texNatIf->nativeTexture());
    }
#endif

#if QT_CONFIG(opengl)
    if (scene->window->graphicsApi() == QSGRendererInterface::OpenGL) {
        auto texNatIf = texture->nativeInterface<QNativeInterface::QSGOpenGLTexture>();
        QVERIFY(texNatIf);
        QVERIFY(texNatIf->nativeTexture());
    }
#endif

#if QT_CONFIG(vulkan)
    if (scene->window->graphicsApi() == QSGRendererInterface::Vulkan) {
        auto texNatIf = texture->nativeInterface<QNativeInterface::QSGVulkanTexture>();
        QVERIFY(texNatIf);
        QVERIFY(texNatIf->nativeImage());
    }
#endif
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

void tst_SceneGraph::distanceFieldCacheInvalidation()
{
    if (!isRunningOnRhi())
        QSKIP("Skipping complex rendering tests due to not running with QRhi");

    QFont font(QStringLiteral("Arial"));
    QRawFont rawFont = QRawFont::fromFont(font);

    QQuickView view;
    view.setSource(testFileUrl(QLatin1String("distanceFieldCacheInvalidation.qml")));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    {
        QSGRenderContext *rc = QQuickItemPrivate::get(view.rootObject())->sceneGraphRenderContext();

        QSGDistanceFieldGlyphCache *cache = rc->distanceFieldGlyphCache(rawFont, 1);
        QVERIFY(cache != nullptr);

        auto glyphIndexes = rawFont.glyphIndexesForString(QStringLiteral("a b"));
        cache->populate(glyphIndexes);
        QVERIFY(cache->isActive());

        cache->release(glyphIndexes);
        QVERIFY(!cache->isActive());
    }
}

void tst_SceneGraph::pregeneratedDistanceFieldCache_data()
{
    QTest::addColumn<QString>("fontFileName");
    QTest::addColumn<bool>("expectLoaded");

    // Various tests for
    QTest::newRow("control, one 64x64 texture") << QStringLiteral("ok_single.ttf") << true;
    QTest::newRow("control, two 64x64 textures") << QStringLiteral("ok_double.ttf") << true;
    QTest::newRow("control, two 64x64 textures") << QStringLiteral("ok_double.ttf") << true;
    QTest::newRow("allocatedWidth -1, height 256 -> size -256") << QStringLiteral("neg_single.ttf") << true;
    QTest::newRow("allocatedWidth -1, height 1024 -> size -1024, pointer slides back 1 KB") << QStringLiteral("neg_double.ttf") << true;
    QTest::newRow("allocatedWidth INT_MIN -> size -2147483648") << QStringLiteral("wild.ttf") << true;

    // In the 5.12 version of the qtdf format, the texture size in the header
    // is the slice height of the stacked area allocator, and the texture
    // holding a glyph that crosses a slice boundary is taller than this.
    QTest::newRow("multiple textures, version 5.12")
            << QStringLiteral("qtdf_multitexture_5_12.ttf") << true;

    // QTBUG-149508: an allocated height with the high bit set becomes
    // negative when narrowed to int, defeating the payload bounds check.
    // Such geometry must be rejected.
    QTest::newRow("invalid geometry, version 5.12")
            << QStringLiteral("qtdf_invalidgeometry_5_12.ttf") << false;
}

void tst_SceneGraph::pregeneratedDistanceFieldCache()
{
    if (!isRunningOnRhi())
        QSKIP("Skipping complex rendering tests due to not running with QRhi");

    QFETCH(QString, fontFileName);
    QFETCH(bool, expectLoaded);

    const int fontId = QFontDatabase::addApplicationFont(testFile(fontFileName));
    QVERIFY(fontId >= 0);
    const auto cleanup = qScopeGuard([fontId] { QFontDatabase::removeApplicationFont(fontId); });

    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    QVERIFY(!families.isEmpty());

    // Verify that the font resolves and carries a pregenerated cache, so that
    // the text below cannot accidentally pass by using another font or by
    // generating distance fields dynamically.
    QCOMPARE(QFontInfo(QFont(families.first())).family(), families.first());
    {
        const QRawFont rawFont(testFile(fontFileName), 32.0);
        QVERIFY(rawFont.isValid());
        QVERIFY(!rawFont.fontTable("qtdf").isEmpty());
    }

    // The scene renders "ABC" with the given font. The pregenerated cache
    // contains the glyphs for "A" and "B" in the first texture, where "B"
    // crosses the boundary into the next slice, and "C" in the second
    // texture. The distance field data marks the glyphs' texture rects as
    // fully inside, so if (and only if) the pregenerated cache is loaded, the
    // glyphs are rendered as solid boxes.
    QQuickView view;
    view.setInitialProperties({{ QStringLiteral("fontFamily"), families.first() }});
    view.setSource(testFileUrl(QLatin1String("pregeneratedDistanceFieldCache.qml")));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const QImage content = view.grabWindow();
    QVERIFY(!content.isNull());
    if (expectLoaded)
        QVERIFY(containsSomethingOtherThanWhite(content));
    else
        QVERIFY(!containsSomethingOtherThanWhite(content));
}

class NotificationItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(
            QRectF contentsRect READ contentsRect WRITE setContentsRect NOTIFY contentsRectChanged)

public:
    explicit NotificationItem(QQuickItem *parent = nullptr) : QQuickItem(parent) { }

    QRectF contentsRect() const { return m_contentsRect; }

    void setContentsRect(const QRectF &contentsRect)
    {
        if (m_contentsRect != contentsRect) {
            m_contentsRect = contentsRect;
            polish();
            Q_EMIT contentsRectChanged();
        }
    }

Q_SIGNALS:
    void contentsRectChanged();

protected:
    void updatePolish() override
    {
        if (m_contentsRect.isEmpty()) {
            window()->setVisible(false);
        }
    }

private:
    QRectF m_contentsRect = QRectF(0, 0, 100, 100);
};

static bool waitForWindowUnexposed(QWindow *window, int timeout = 5000)
{
    return QTest::qWaitFor([window]() { return !window->isExposed(); }, timeout);
}

void tst_SceneGraph::unexposeDuringPolish()
{
    qmlRegisterType<NotificationItem>("SceneGraphTest", 1, 0, "Notification");

    // This test verifies that the window will not be painted if it's unexposed while polishing
    // items. It's relevant with QPAs such as QtWayland.

    QQuickView view;
    view.setSource(testFileUrl(QLatin1String("unexposeDuringPolish.qml")));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    connect(&view, &QQuickWindow::afterAnimating, this, [&view]() {
        QVERIFY(view.isExposed());
    });
    connect(&view, &QQuickWindow::beforeSynchronizing, this, [&view]() {
        QVERIFY(view.isExposed());
    });

    QQuickItem *rootItem = view.rootObject();
    rootItem->setProperty("contentsRect", QRectF());

    QVERIFY(waitForWindowUnexposed(&view));
    QVERIFY(!view.isExposed());
    QVERIFY(!view.isVisible());
}

#ifdef QT_BUILD_INTERNAL
void tst_SceneGraph::mutabilityGroups()
{
    QQuickView view;
    view.setSource(testFileUrl(QLatin1String("simple.qml")));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *rootItem = view.rootObject();
    QVERIFY(rootItem);

    // Default mutability group
    QQuickRectangle *rect1 = new QQuickRectangle(rootItem);
    rect1->setWidth(100);
    rect1->setHeight(100);
    rect1->setColor(Qt::red);
    QCOMPARE(rect1->mutabilityGroup(), int(QQuickItem::AutoMutabilityGroup));

    view.grabWindow(); // Force a frame to create nodes

    QQuickItemPrivate *rect1Private = QQuickItemPrivate::get(rect1);

    {
        QSGNode *node1 = rect1Private->paintNode;
        QVERIFY(node1);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(node1), quint8(QQuickItem::AutoMutabilityGroup));
    }

    // Change mutability group
    QSignalSpy spy(rect1, &QQuickItem::mutabilityGroupChanged);
    rect1->setMutabilityGroup(QQuickItem::StaticMutabilityGroup);
    QCOMPARE(rect1->mutabilityGroup(), int(QQuickItem::StaticMutabilityGroup));
    QCOMPARE(spy.count(), 1);

    view.grabWindow();

    {
        QSGNode *node1 = rect1Private->paintNode;
        QVERIFY(node1);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(node1), quint8(QQuickItem::StaticMutabilityGroup));
    }

    // Set mutability group to Dynamic updates
    rect1->setMutabilityGroup(QQuickItem::DynamicMutabilityGroup);
    QCOMPARE(rect1->mutabilityGroup(), int(QQuickItem::DynamicMutabilityGroup));
    QCOMPARE(spy.count(), 2);

    view.grabWindow();
    {
        QSGNode *node1 = rect1Private->paintNode;
        QVERIFY(node1);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(node1), quint8(QQuickItem::DynamicMutabilityGroup));
    }

    // Setting identical value for group does not re-emit
    rect1->setMutabilityGroup(QQuickItem::DynamicMutabilityGroup);
    QCOMPARE(spy.count(), 2);

    // Test value clamping
    rect1->setMutabilityGroup(255);
    QCOMPARE(rect1->mutabilityGroup(), quint8(15));

    // Parent-child relationships
    QQuickRectangle *parent = new QQuickRectangle(rootItem);
    parent->setWidth(200);
    parent->setHeight(200);
    parent->setMutabilityGroup(QQuickItem::StaticMutabilityGroup);

    QQuickRectangle *child1 = new QQuickRectangle(parent);
    child1->setWidth(50);
    child1->setHeight(50);
    QCOMPARE(child1->mutabilityGroup(), int(QQuickItem::AutoMutabilityGroup));

    QQuickRectangle *child2 = new QQuickRectangle(parent);
    child2->setWidth(50);
    child2->setHeight(50);
    child2->setMutabilityGroup(QQuickItem::ModerateMutabilityGroup);
    QCOMPARE(child2->mutabilityGroup(), int(QQuickItem::ModerateMutabilityGroup));

    view.grabWindow();

    QQuickItemPrivate *parentPrivate = QQuickItemPrivate::get(parent);
    QVERIFY(parentPrivate);

    QQuickItemPrivate *childPrivate1 = QQuickItemPrivate::get(child1);
    QVERIFY(childPrivate1);

    QQuickItemPrivate *childPrivate2 = QQuickItemPrivate::get(child2);
    QVERIFY(childPrivate2);

    // Parent should have its mutability group
    {
        QSGNode *parentNode = parentPrivate->paintNode;
        QVERIFY(parentNode);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(parentNode), quint8(QQuickItem::StaticMutabilityGroup));
    }

    // Child should not inherit parent's mutability group in the scene graph
    {
        QSGNode *childNode1 = childPrivate1->paintNode;
        QVERIFY(childNode1);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(childNode1), quint8(QQuickItem::AutoMutabilityGroup));
    }

    {
        QSGNode *childNode2 = childPrivate2->paintNode;
        QVERIFY(childNode2);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(childNode2), quint8(QQuickItem::ModerateMutabilityGroup));
    }

    // Changing parent's mutability group should not propagate to children
    parent->setMutabilityGroup(QQuickItem::DynamicMutabilityGroup);
    view.grabWindow();

    {
        QSGNode *parentNode = parentPrivate->paintNode;
        QVERIFY(parentNode);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(parentNode), quint8(QQuickItem::DynamicMutabilityGroup));
    }

    {
        QSGNode *childNode1 = childPrivate1->paintNode;
        QVERIFY(childNode1);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(childNode1), quint8(QQuickItem::AutoMutabilityGroup));
    }

    {
        QSGNode *childNode2 = childPrivate2->paintNode;
        QVERIFY(childNode2);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(childNode2), quint8(QQuickItem::ModerateMutabilityGroup));
    }

    // Updating child mutability group
    child1->setMutabilityGroup(QQuickItem::StaticMutabilityGroup);
    view.grabWindow();

    {
        QSGNode *parentNode = parentPrivate->paintNode;
        QVERIFY(parentNode);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(parentNode), quint8(QQuickItem::DynamicMutabilityGroup));
    }

    {
        QSGNode *childNode1 = childPrivate1->paintNode;
        QVERIFY(childNode1);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(childNode1), quint8(QQuickItem::StaticMutabilityGroup));
    }

    {
        QSGNode *childNode2 = childPrivate2->paintNode;
        QVERIFY(childNode2);
        QCOMPARE(QSGNodePrivate::mutabilityGroup(childNode2), quint8(QQuickItem::ModerateMutabilityGroup));
    }
}
#endif // QT_BUILD_INTERNAL

class CustomTextItem : public QQuickItem {
public:
    CustomTextItem(const QUrl &imgUrl)
        : m_imgUrl(imgUrl)
    {
        setFlag(ItemHasContents);
    }

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
    {
        delete oldNode;

        QSGTextNode *node = window()->createTextNode();

        if (!m_imgUrl.isEmpty()) {
            QTextDocument textDocument;
            textDocument.setHtml(QStringLiteral("<html><body>Text هناك<u>Underline</u><img src=\"%1\" width=20 height=20 />").arg(m_imgUrl.toString()));

            node->addTextDocument(QPointF(0, 0), &textDocument, 0, 3);
        } else {
            QTextLayout textLayout;
            textLayout.setText(QStringLiteral("Text هناك"));
            textLayout.beginLayout();
            textLayout.createLine();
            textLayout.endLayout();

            node->addTextLayout(QPointF(0, 0), &textLayout, 0, 3);
        }

        return node;
    }

private:
    Q_DECLARE_PRIVATE(QQuickItem)

    QUrl m_imgUrl;
};

class ChildNodeRegister : public QSGNodeVisitorEx
{
public:
    ChildNodeRegister()
    {
    }

    bool visit(QSGTransformNode *) override
    {
        Q_UNREACHABLE();
        return false;
    }

    void endVisit(QSGTransformNode *) override
    {
    }

    bool visit(QSGClipNode *) override
    {
        return false;
    }

    void endVisit(QSGClipNode *) override
    {
    }

    bool visit(QSGGeometryNode *) override
    {
        return false;
    }

    void endVisit(QSGGeometryNode *) override
    {
    }

    bool visit(QSGOpacityNode *) override
    {
        return false;
    }

    void endVisit(QSGOpacityNode *) override
    {
    }

    bool visit(QSGInternalImageNode *) override
    {
        hasImageNode = true;
        return false;
    }

    void endVisit(QSGInternalImageNode *) override
    {
    }

    bool visit(QSGPainterNode *) override
    {
        return false;
    }

    void endVisit(QSGPainterNode *) override
    {
    }

    bool visit(QSGInternalRectangleNode *) override
    {
        return false;
    }

    void endVisit(QSGInternalRectangleNode *) override
    {
    }

    bool visit(QSGGlyphNode *) override
    {
        hasGlyphNode = true;
        return false;
    }

    void endVisit(QSGGlyphNode *) override
    {
    }

    bool visit(QSGRootNode *) override
    {
        return false;
    }

    void endVisit(QSGRootNode *) override
    {
    }

#if QT_CONFIG(quick_sprite)
    bool visit(QSGSpriteNode *) override
    {
        return false;
    }

    void endVisit(QSGSpriteNode *) override
    {
    }
#endif
    bool visit(QSGRenderNode *) override
    {
        return false;
    }

    void endVisit(QSGRenderNode *) override
    {
    }

    bool hasImageNode = false;
    bool hasGlyphNode = false;
};


void tst_SceneGraph::sgTextNode_data()
{
    QTest::addColumn<QUrl>("imgUrl");

    QTest::newRow("text layout") << QUrl{};
    QTest::newRow("text document") << testFileUrl(QLatin1String("blacknwhite.png"));
}

void tst_SceneGraph::sgTextNode()
{
    QFETCH(QUrl, imgUrl);

    QQuickView view;
    view.setSource(testFileUrl("simple.qml"));
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    CustomTextItem *item = new CustomTextItem(imgUrl);
    item->setParentItem(view.rootObject());
    item->setParent(view.rootObject());

    view.grabWindow(); // Create nodes

    QQuickItemPrivate *d = QQuickItemPrivate::get(item);
    QVERIFY(d);
    QVERIFY(d->paintNode);
    QVERIFY(d->paintNode->childCount());

    ChildNodeRegister reg;
    QSGNode *node = d->paintNode->firstChild();
    while (node != nullptr) {
        if (node->flags().testFlag(QSGNode::IsVisitableNode)) {
            QSGVisitableNode *visitableNode = static_cast<QSGVisitableNode *>(node);
            visitableNode->accept(&reg);
        }

        node = node->nextSibling();
    }

    QVERIFY(reg.hasGlyphNode);
    QCOMPARE(reg.hasImageNode, !imgUrl.isEmpty());
}

void tst_SceneGraph::sgInternalTextNodeRecycle_data()
{
    QTest::addColumn<QUrl>("imgUrl");
    QTest::addColumn<QQuickText::RenderType>("renderType");

    QTest::newRow("Text layout, Qt Rendering") << QUrl{} << QQuickText::QtRendering;
    QTest::newRow("Text document, Qt Rendering") << testFileUrl(QLatin1String("blacknwhite.png")) << QQuickText::QtRendering;

    QTest::newRow("Text layout, Native Rendering") << QUrl{} << QQuickText::NativeRendering;
    QTest::newRow("Text document, Native Rendering") << testFileUrl(QLatin1String("blacknwhite.png")) << QQuickText::NativeRendering;

    QTest::newRow("Text layout, Curve Rendering") << QUrl{} << QQuickText::CurveRendering;
    QTest::newRow("Text document, Curve Rendering") << testFileUrl(QLatin1String("blacknwhite.png")) << QQuickText::CurveRendering;
}

void tst_SceneGraph::sgInternalTextNodeRecycle()
{
    QFETCH(QUrl, imgUrl);
    QFETCH(QQuickText::RenderType, renderType);

    QQuickView view;
    view.setSource(testFileUrl("simple.qml"));
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickText *textItem = view.rootObject()->findChild<QQuickText *>();
    QVERIFY(textItem);
    textItem->setRenderType(renderType);

    view.grabWindow(); // Create nodes

    QQuickItemPrivate *d = QQuickItemPrivate::get(textItem);
    QVERIFY(d);

    {
        QVERIFY(d->paintNode);
    }

    if (imgUrl.isEmpty()) {
        textItem->setText(QStringLiteral("<u>Tex</u>t هناك"));
    } else {
        textItem->setTextFormat(QQuickText::RichText);
        textItem->setText(QStringLiteral("<html><body>Text هناك<u>Underline</u><img src=\"%1\" width=20 height=20 />").arg(imgUrl.toString()));
    }

    view.grabWindow();

    // Did we successfully make some nodes?
    {
        ChildNodeRegister reg;
        QSGNode *node = d->paintNode->firstChild();
        while (node != nullptr) {
            if (node->flags().testFlag(QSGNode::IsVisitableNode)) {
                QSGVisitableNode *visitableNode = static_cast<QSGVisitableNode *>(node);
                visitableNode->accept(&reg);
            }

            node = node->nextSibling();
        }

        QVERIFY(reg.hasGlyphNode);
        QCOMPARE(reg.hasImageNode, !imgUrl.isEmpty());
    }

    // Replace with a single glyph and check that everything else was cleared out
    textItem->setText(QStringLiteral("a"));
    view.grabWindow();

    QSGNode *childNode = nullptr;
    {
        QVERIFY(d->paintNode);
        QCOMPARE(d->paintNode->childCount(), 1);
        childNode = d->paintNode->childAtIndex(0);
    }

    // Replace single glyph with single glyph and verify that glyph node is reused
    // Skip this test for CurveRendering, since we don't recycle curve-rendered nodes currently.
    if (renderType != QQuickText::CurveRendering) {
        textItem->setText(QStringLiteral("b"));
        view.grabWindow();

        {
            QVERIFY(d->paintNode);
            QCOMPARE(d->paintNode->childCount(), 1);
            QCOMPARE(d->paintNode->childAtIndex(0), childNode);
        }
    }

    textItem->setText(QString{});
    view.grabWindow();

    {
        QVERIFY(!d->paintNode);
    }

}

void tst_SceneGraph::sgInternalTextNodeRecycleBatchRoot_data()
{
    QTest::addColumn<QQuickText::RenderType>("renderType");

    QTest::newRow("Qt Rendering") << QQuickText::QtRendering;
    QTest::newRow("Native Rendering") << QQuickText::NativeRendering;
}

void tst_SceneGraph::sgInternalTextNodeRecycleBatchRoot()
{
    // QTBUG-151670: When the text node has more children than the renderer's batch node
    // threshold, it is turned into a batch root. Recycled glyph nodes still have to get
    // their renderer state back when the text is updated.
    QFETCH(QQuickText::RenderType, renderType);

    QQuickView view;
    view.setSource(testFileUrl("simple.qml"));
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickText *textItem = view.rootObject()->findChild<QQuickText *>();
    QVERIFY(textItem);
    textItem->setRenderType(renderType);
    textItem->setTextFormat(QQuickText::StyledText);

    // Make sure clip list and inherited opacity are restored as well
    textItem->setOpacity(0.5);
    textItem->parentItem()->setClip(true);

    // Default for QSG_RENDERER_BATCH_NODE_THRESHOLD is 64
    const int runCount = 100;
    auto makeText = [runCount](int tick) {
        QString text;
        for (int i = 0; i < runCount; ++i) {
            const QColor color = QColor::fromHsl(i * 359 / runCount, 255, 100);
            text += QStringLiteral("<font color=\"%1\">%2</font>")
                            .arg(color.name())
                            .arg((tick + i) % 10);
        }
        return text;
    };

    QQuickItemPrivate *d = QQuickItemPrivate::get(textItem);
    QVERIFY(d);

    QList<QSGNode *> previousChildren;
    for (int tick = 0; tick < 4; ++tick) {
        textItem->setText(makeText(tick));
        view.grabWindow();

        QVERIFY(d->paintNode);
        QCOMPARE(d->paintNode->childCount(), runCount);

        QList<QSGNode *> children;
        for (QSGNode *node = d->paintNode->firstChild(); node != nullptr; node = node->nextSibling()) {
            QCOMPARE(node->type(), QSGNode::GeometryNodeType);
            QSGGeometryNode *geometryNode = static_cast<QSGGeometryNode *>(node);
            if (isRunningOnRhi()) {
                QVERIFY(geometryNode->matrix() != nullptr);
                QVERIFY(geometryNode->clipList() != nullptr);
                QCOMPARE(geometryNode->inheritedOpacity(), 0.5);
            }
            children.append(node);
        }

        // Glyph nodes should be reused
        if (!previousChildren.isEmpty())
            QCOMPARE(children, previousChildren);
        previousChildren = children;
    }
}

void tst_SceneGraph::hiddenItemBlocksNode_data()
{
    QTest::addColumn<QByteArray>("hideMode");

    QTest::newRow("visible") << "visible"_ba;
    QTest::newRow("culled") << "culled"_ba;
    QTest::newRow("hideSource") << "hideSource"_ba;
}

void tst_SceneGraph::hiddenItemBlocksNode()
{
    if (!isRunningOnRhi())
        QSKIP("Skipping rendering test due to not running with QRhi");

    QFETCH(QByteArray, hideMode);

    QQuickView view;
    view.setSource(testFileUrl(u"hiddenItem.qml"_s));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *target = view.rootObject()->findChild<QQuickItem *>(u"target"_s);
    QVERIFY(target);
    QQuickItem *effectSource = view.rootObject()->findChild<QQuickItem *>(u"effectSource"_s);
    QVERIFY(effectSource);
    QQuickItemPrivate *d = QQuickItemPrivate::get(target);

    const auto setHidden = [&](bool hidden) {
        if (hideMode == "visible") {
            // Without an effect referencing it, an invisible item has no node in the scene
            effectSource->setProperty("sourceItem", QVariant::fromValue(target));
            target->setVisible(!hidden);
        } else if (hideMode == "culled") {
            d->setCulled(hidden);
        } else {
            effectSource->setProperty("sourceItem", QVariant::fromValue(target));
            effectSource->setProperty("hideSource", hidden);
        }
    };
    const auto centerColor = [&] {
        const QPointF center = target->mapToScene(QPointF(50, 50)) * view.devicePixelRatio();
        return view.grabWindow().pixelColor(center.toPoint());
    };

    QCOMPARE(centerColor(), QColor(Qt::red));
    QVERIFY(d->itemNode());
    QVERIFY(!d->itemNode()->isSubtreeBlocked());

    // Hidden items get their transform node blocked, without an opacity node
    for (int i = 0; i < 2; ++i) {
        setHidden(true);
        QCOMPARE(centerColor(), QColor(Qt::white));
        QVERIFY(d->itemNode()->isSubtreeBlocked());
        QVERIFY(!d->opacityNode());

        setHidden(false);
        QCOMPARE(centerColor(), QColor(Qt::red));
        QVERIFY(!d->itemNode()->isSubtreeBlocked());
        QVERIFY(!d->opacityNode());
    }
}

using PixelSamples = QList<std::pair<QPoint, QColor>>;

static bool checkPixels(QQuickWindow *window, const PixelSamples &samples, QString *errorMessage)
{
    const QImage content = window->grabWindow();
    for (const auto &[pos, expected] : samples) {
        const QPoint pixelPos = (QPointF(pos) * window->devicePixelRatio()).toPoint();
        const QColor actual = content.pixelColor(pixelPos);
        if (actual != expected) {
            *errorMessage = u"Pixel at %1,%2 is %3, expected %4"_s
                    .arg(pos.x()).arg(pos.y()).arg(actual.name(), expected.name());
            return false;
        }
    }
    return true;
}

// Hides or shows item for one reason: "visible", "culled", or "hideSource" or
// "hideSource2" for the ShaderEffectSource named "effectSource" or
// "effectSource2" in scene, respectively.
static void setItemHidden(QObject *scene, QQuickItem *item, QByteArrayView reason, bool hidden)
{
    if (reason == "visible") {
        item->setVisible(!hidden);
    } else if (reason == "culled") {
        QQuickItemPrivate::get(item)->setCulled(hidden);
    } else {
        QVERIFY(reason.startsWith("hideSource"));
        const QString name = u"effectSource"_s
                + QString::fromLatin1(reason.sliced(qstrlen("hideSource")));
        QQuickItem *effectSource = scene->findChild<QQuickItem *>(name);
        QVERIFY(effectSource);
        effectSource->setProperty("sourceItem", QVariant::fromValue(item));
        effectSource->setProperty("hideSource", hidden);
    }
}

void tst_SceneGraph::changeWhileHidden_data()
{
    QTest::addColumn<QByteArray>("hideMode");
    QTest::addColumn<QByteArray>("change");
    QTest::addColumn<PixelSamples>("hiddenSamples");
    QTest::addColumn<PixelSamples>("shownSamples");

    // The target is red at (20, 20, 60x60), the cover blue on top of it at
    // (60, 60, 60x60), the background is white.
    const struct {
        const char *change;
        PixelSamples hiddenSamples;
        PixelSamples shownSamples;
    } changes[] = {
        { "color",
          { { { 40, 40 }, Qt::white } },
          { { { 40, 40 }, Qt::green } } },
        { "size",
          { { { 150, 40 }, Qt::white } },
          { { { 40, 40 }, Qt::red }, { { 150, 40 }, Qt::red } } },
        { "position",
          { { { 40, 160 }, Qt::white } },
          { { { 40, 40 }, Qt::white }, { { 40, 160 }, Qt::red } } },
        { "child",
          { { { 35, 35 }, Qt::white } },
          { { { 35, 35 }, Qt::green }, { { 70, 30 }, Qt::red } } },
        { "stacking",
          { { { 70, 70 }, Qt::blue } },
          { { { 40, 40 }, Qt::red }, { { 70, 70 }, Qt::red } } },
    };

    for (const char *hideMode : { "visible", "culled", "hideSource" }) {
        for (const auto &c : changes) {
            QTest::addRow("%s-%s", hideMode, c.change)
                    << QByteArray(hideMode) << QByteArray(c.change)
                    << c.hiddenSamples << c.shownSamples;
        }
    }
}

void tst_SceneGraph::changeWhileHidden()
{
    if (!isRunningOnRhi())
        QSKIP("Skipping rendering test due to not running with QRhi");

    QFETCH(QByteArray, hideMode);
    QFETCH(QByteArray, change);
    QFETCH(PixelSamples, hiddenSamples);
    QFETCH(PixelSamples, shownSamples);

    QQuickView view;
    view.setSource(testFileUrl(u"hiddenItemScene.qml"_s));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *target = view.rootObject()->findChild<QQuickItem *>(u"target"_s);
    QVERIFY(target);

    QString errorMessage;
    QVERIFY2(checkPixels(&view, { { { 40, 40 }, Qt::red }, { { 70, 70 }, Qt::blue } },
                         &errorMessage),
             qPrintable(errorMessage));

    setItemHidden(view.rootObject(), target, hideMode, true);
    QVERIFY2(checkPixels(&view, { { { 40, 40 }, Qt::white }, { { 70, 70 }, Qt::blue } },
                         &errorMessage),
             qPrintable(errorMessage));

    // The renderer has no shadow nodes for a blocked subtree, so it ignores
    // the change while the item is hidden. Showing the item again, below a
    // clip node, only partially rebuilds the batches, which must still pick
    // up the change.
    if (change == "color") {
        target->setProperty("color", QColor(Qt::green));
    } else if (change == "size") {
        target->setWidth(160);
    } else if (change == "position") {
        target->setY(130);
    } else if (change == "child") {
        QQuickRectangle *child = new QQuickRectangle(target);
        child->setPosition(QPointF(5, 5));
        child->setSize(QSizeF(20, 20));
        child->setColor(Qt::green);
    } else if (change == "stacking") {
        target->setZ(1);
    }
    QVERIFY2(checkPixels(&view, hiddenSamples, &errorMessage), qPrintable(errorMessage));

    setItemHidden(view.rootObject(), target, hideMode, false);
    QVERIFY2(checkPixels(&view, shownSamples, &errorMessage), qPrintable(errorMessage));
}

void tst_SceneGraph::hiddenItemMultipleReasons_data()
{
    // Each reason is "item.reason", see setItemHidden() for the reasons
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

void tst_SceneGraph::hiddenItemMultipleReasons()
{
    if (!isRunningOnRhi())
        QSKIP("Skipping rendering test due to not running with QRhi");

    QFETCH(QByteArray, first);
    QFETCH(QByteArray, second);

    QQuickView view;
    view.setSource(testFileUrl(u"hiddenItemScene.qml"_s));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickItem *target = view.rootObject()->findChild<QQuickItem *>(u"target"_s);
    QVERIFY(target);
    QQuickItemPrivate *d = QQuickItemPrivate::get(target);

    const auto setHidden = [&](const QByteArray &spec, bool hidden) {
        const QList<QByteArray> parts = spec.split('.');
        QQuickItem *item = view.rootObject()->findChild<QQuickItem *>(QString::fromLatin1(parts.first()));
        QVERIFY(item);
        setItemHidden(view.rootObject(), item, parts.last(), hidden);
    };

    const PixelSamples hiddenSamples = { { { 40, 40 }, Qt::white } };
    const PixelSamples shownSamples = { { { 40, 40 }, Qt::red } };
    QString errorMessage;
    QVERIFY2(checkPixels(&view, shownSamples, &errorMessage), qPrintable(errorMessage));

    // Hide for both reasons, then show for one reason at a time, in both orders.
    // The item stays hidden until neither reason applies.
    for (const auto &[showFirst, showLast] : { std::pair(first, second), std::pair(second, first) }) {
        setHidden(first, true);
        setHidden(second, true);
        QVERIFY2(checkPixels(&view, hiddenSamples, &errorMessage), qPrintable(errorMessage));
        QVERIFY(d->itemNode()->isSubtreeBlocked());

        setHidden(showFirst, false);
        QVERIFY2(checkPixels(&view, hiddenSamples, &errorMessage), qPrintable(errorMessage));
        // Showing an item with an invisible parent does not mark it dirty, so
        // its node only reliably reflects reasons that still apply to itself.
        if (showLast.startsWith("target."))
            QVERIFY(d->itemNode()->isSubtreeBlocked());

        setHidden(showLast, false);
        QVERIFY2(checkPixels(&view, shownSamples, &errorMessage), qPrintable(errorMessage));
        QVERIFY(!d->itemNode()->isSubtreeBlocked());
    }
}

void tst_SceneGraph::hiddenItemWindowChange_data()
{
    QTest::addColumn<QByteArray>("hideMode");

    QTest::newRow("visible") << "visible"_ba;
    QTest::newRow("culled") << "culled"_ba;
    QTest::newRow("hideSource") << "hideSource"_ba;
}

void tst_SceneGraph::hiddenItemWindowChange()
{
    if (!isRunningOnRhi())
        QSKIP("Skipping rendering test due to not running with QRhi");

    QFETCH(QByteArray, hideMode);

    QQuickView view;
    view.setSource(testFileUrl(u"hiddenItemScene.qml"_s));
    QQuickItem *content = view.rootObject()->findChild<QQuickItem *>(u"content"_s);
    QVERIFY(content);
    QQuickItem *target = view.rootObject()->findChild<QQuickItem *>(u"target"_s);
    QVERIFY(target);
    QQuickItemPrivate *d = QQuickItemPrivate::get(target);

    const PixelSamples hiddenSamples = { { { 40, 40 }, Qt::white } };
    const PixelSamples shownSamples = { { { 40, 40 }, Qt::red } };
    QString errorMessage;

    // Hidden before the first frame, so the item's nodes are created blocked
    setItemHidden(view.rootObject(), target, hideMode, true);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY2(checkPixels(&view, hiddenSamples, &errorMessage), qPrintable(errorMessage));
    QVERIFY(d->itemNode()->isSubtreeBlocked());

    setItemHidden(view.rootObject(), target, hideMode, false);
    QVERIFY2(checkPixels(&view, shownSamples, &errorMessage), qPrintable(errorMessage));
    QVERIFY(!d->itemNode()->isSubtreeBlocked());

    // Moving to another window recreates the item's nodes, which must be
    // blocked again. Declared after view, so that it is destroyed first.
    QQuickWindow otherWindow;
    otherWindow.resize(200, 200);
    otherWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&otherWindow));

    for (QQuickWindow *window : { static_cast<QQuickWindow *>(&otherWindow),
                                  static_cast<QQuickWindow *>(&view) }) {
        setItemHidden(view.rootObject(), target, hideMode, true);
        content->setParentItem(window == &view ? view.rootObject() : window->contentItem());
        QVERIFY2(checkPixels(window, hiddenSamples, &errorMessage), qPrintable(errorMessage));
        QVERIFY(d->itemNode()->isSubtreeBlocked());

        setItemHidden(view.rootObject(), target, hideMode, false);
        QVERIFY2(checkPixels(window, shownSamples, &errorMessage), qPrintable(errorMessage));
        QVERIFY(!d->itemNode()->isSubtreeBlocked());
    }
}

#include "tst_scenegraph.moc"

QTEST_MAIN(tst_SceneGraph)

