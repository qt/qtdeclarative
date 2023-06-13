// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QString>
#include <QtTest/QtTest>

#include <QtQuick/qsgnode.h>
#include <QtQuick/private/qsgbatchrenderer_p.h>
#include <QtQuick/private/qsgnodeupdater_p.h>
#include <QtQuick/private/qsgrenderloop_p.h>
#include <QtQuick/private/qsgcontext_p.h>

#include <QtQuick/qsgsimplerectnode.h>
#include <QtQuick/qsgsimpletexturenode.h>
#include <QtQuick/private/qsgplaintexture_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qoffscreensurface.h>

#include <rhi/qrhi.h>

#if QT_CONFIG(opengl)
# include <QOpenGLContext>
# define TST_GL
#endif

#if QT_CONFIG(vulkan)
# include <QVulkanInstance>
# define TST_VK
#endif

#ifdef Q_OS_WIN
# define TST_D3D11
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
# define TST_MTL
#endif

QT_BEGIN_NAMESPACE
inline bool operator==(const QSGGeometry::TexturedPoint2D& l, const QSGGeometry::TexturedPoint2D& r)
{
    return l.x == r.x && l.y == r.y && l.tx == r.tx && l.ty == r.ty;
}
QT_END_NAMESPACE

class NodesTest : public QObject
{
    Q_OBJECT

public:
    NodesTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    // Root nodes
    void propagate_data();
    void propagate();
    void propagateWithMultipleRoots_data();
    void propagateWithMultipleRoots();

    // Opacity nodes
    void basicOpacityNode_data();
    void basicOpacityNode();
    void opacityPropagation_data();
    void opacityPropagation();

    void isBlockedCheck_data();
    void isBlockedCheck();

    void textureNodeTextureOwnership_data();
    void textureNodeTextureOwnership();
    void textureNodeRect_data();
    void textureNodeRect();

private:
    void rhiTestData();

    QSGDefaultRenderContext *renderContext = nullptr;

    struct {
        QRhiNullInitParams null;
#ifdef TST_GL
        QRhiGles2InitParams gl;
#endif
#ifdef TST_VK
        QRhiVulkanInitParams vk;
#endif
#ifdef TST_D3D11
        QRhiD3D11InitParams d3d;
#endif
#ifdef TST_MTL
        QRhiMetalInitParams mtl;
#endif
    } initParams;

#ifdef TST_VK
    QVulkanInstance vulkanInstance;
#endif
    QOffscreenSurface *fallbackSurface = nullptr;
};

void NodesTest::initTestCase()
{
#ifdef TST_GL
    fallbackSurface = QRhiGles2InitParams::newFallbackSurface();
    initParams.gl.fallbackSurface = fallbackSurface;
#endif

#ifdef TST_VK
    vulkanInstance.setLayers({ QByteArrayLiteral("VK_LAYER_LUNARG_standard_validation") });
    vulkanInstance.setExtensions({ QByteArrayLiteral("VK_KHR_get_physical_device_properties2") });
    vulkanInstance.create();
    initParams.vk.inst = &vulkanInstance;
#endif

#ifdef TST_D3D11
    initParams.d3d.enableDebugLayer = true;
#endif
}

void NodesTest::cleanupTestCase()
{
#ifdef TST_VK
    vulkanInstance.destroy();
#endif

    delete fallbackSurface;
}

class DummyRenderer : public QSGBatchRenderer::Renderer
{
public:
    DummyRenderer(QSGRootNode *root, QSGDefaultRenderContext *renderContext)
        : QSGBatchRenderer::Renderer(renderContext)
    {
        setRootNode(root);
    }

    void render() override {
        ++renderCount;
        renderingOrder = ++globalRendereringOrder;
    }

    void nodeChanged(QSGNode *node, QSGNode::DirtyState state) override {
        changedNode = node;
        changedState = state;
        QSGBatchRenderer::Renderer::nodeChanged(node, state);
    }

    QSGNode *changedNode = nullptr;
    QSGNode::DirtyState changedState;

    int renderCount = 0;
    int renderingOrder = 0;
    static int globalRendereringOrder;
};

int DummyRenderer::globalRendereringOrder;

NodesTest::NodesTest()
{
}

void NodesTest::rhiTestData()
{
    QTest::addColumn<QRhi::Implementation>("impl");
    QTest::addColumn<QRhiInitParams *>("initParams");

    QTest::newRow("Null") << QRhi::Null << static_cast<QRhiInitParams *>(&initParams.null);
#ifdef TST_GL
    QTest::newRow("OpenGL") << QRhi::OpenGLES2 << static_cast<QRhiInitParams *>(&initParams.gl);
#endif
#ifdef TST_VK
    if (vulkanInstance.isValid())
        QTest::newRow("Vulkan") << QRhi::Vulkan << static_cast<QRhiInitParams *>(&initParams.vk);
#endif
#ifdef TST_D3D11
    QTest::newRow("Direct3D 11") << QRhi::D3D11 << static_cast<QRhiInitParams *>(&initParams.d3d);
#endif
#ifdef TST_MTL
    QTest::newRow("Metal") << QRhi::Metal << static_cast<QRhiInitParams *>(&initParams.mtl);
#endif
}

#define INIT_RHI()                      \
    QFETCH(QRhi::Implementation, impl); \
    QFETCH(QRhiInitParams *, initParams); \
    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr)); \
    if (!rhi) \
        QSKIP("Failed to create QRhi, skipping test"); \
    QSGRenderLoop *renderLoop = QSGRenderLoop::instance(); \
    auto rc = renderLoop->createRenderContext(renderLoop->sceneGraphContext()); \
    renderContext = static_cast<QSGDefaultRenderContext *>(rc); \
    QVERIFY(renderContext); \
    QSGDefaultRenderContext::InitParams rcParams; \
    rcParams.rhi = rhi.data(); \
    rcParams.initialSurfacePixelSize = QSize(512, 512); \
    renderContext->initialize(&rcParams); \
    QVERIFY(renderContext->isValid()); \
    QSGRendererInterface *rif = renderLoop->sceneGraphContext()->rendererInterface(renderContext); \
    if (!QSGRendererInterface::isApiRhiBased(rif->graphicsApi())) \
        QSKIP("Skipping due to using software backend")

void NodesTest::propagate_data()
{
    rhiTestData();
}

void NodesTest::propagate()
{
    INIT_RHI();

    QSGRootNode root;
    QSGNode child; child.setFlag(QSGNode::OwnedByParent, false);
    root.appendChildNode(&child);

    DummyRenderer renderer(&root, renderContext);

    child.markDirty(QSGNode::DirtyGeometry);

    QCOMPARE(&child, renderer.changedNode);
    QCOMPARE((int) renderer.changedState, (int) QSGNode::DirtyGeometry);

    renderContext->invalidate();
}

void NodesTest::propagateWithMultipleRoots_data()
{
    rhiTestData();
}

void NodesTest::propagateWithMultipleRoots()
{
    INIT_RHI();

    QSGRootNode root1;
    QSGNode child2; child2.setFlag(QSGNode::OwnedByParent, false);
    QSGRootNode root3; root3.setFlag(QSGNode::OwnedByParent, false);
    QSGNode child4; child4.setFlag(QSGNode::OwnedByParent, false);

    root1.appendChildNode(&child2);
    child2.appendChildNode(&root3);
    root3.appendChildNode(&child4);

    DummyRenderer ren1(&root1, renderContext);
    DummyRenderer ren2(&root3, renderContext);

    child4.markDirty(QSGNode::DirtyGeometry);

    QCOMPARE(ren1.changedNode, &child4);
    QCOMPARE(ren2.changedNode, &child4);

    QCOMPARE((int) ren1.changedState, (int) QSGNode::DirtyGeometry);
    QCOMPARE((int) ren2.changedState, (int) QSGNode::DirtyGeometry);

    renderContext->invalidate();
}

void NodesTest::basicOpacityNode_data()
{
    rhiTestData();
}

void NodesTest::basicOpacityNode()
{
    INIT_RHI();

    QSGOpacityNode n;
    QCOMPARE(n.opacity(), 1.);

    n.setOpacity(0.5);
    QCOMPARE(n.opacity(), 0.5);

    n.setOpacity(-1);
    QCOMPARE(n.opacity(), 0.);

    n.setOpacity(2);
    QCOMPARE(n.opacity(), 1.);

    renderContext->invalidate();
}

void NodesTest::opacityPropagation_data()
{
    rhiTestData();
}

void NodesTest::opacityPropagation()
{
    INIT_RHI();

    QSGRootNode root;
    QSGOpacityNode *a = new QSGOpacityNode;
    QSGOpacityNode *b = new QSGOpacityNode;
    QSGOpacityNode *c = new QSGOpacityNode;

    QSGSimpleRectNode *geometry = new QSGSimpleRectNode;
    geometry->setRect(0, 0, 100, 100);

    DummyRenderer renderer(&root, renderContext);

    root.appendChildNode(a);
    a->appendChildNode(b);
    b->appendChildNode(c);
    c->appendChildNode(geometry);

    a->setOpacity(0.9);
    b->setOpacity(0.8);
    c->setOpacity(0.7);

    // We do not need to really render, but have to do the preprocessing.
    // The expectation towards renderer is that calling renderScene()
    // without a render target set does not render anything, but performs
    // the preprocessing steps.
    renderer.renderScene();

    QCOMPARE(a->combinedOpacity(), 0.9);
    QCOMPARE(b->combinedOpacity(), 0.9 * 0.8);
    QCOMPARE(c->combinedOpacity(), 0.9 * 0.8 * 0.7);
    QCOMPARE(geometry->inheritedOpacity(), 0.9 * 0.8 * 0.7);

    b->setOpacity(0.1);
    renderer.renderScene();

    QCOMPARE(a->combinedOpacity(), 0.9);
    QCOMPARE(b->combinedOpacity(), 0.9 * 0.1);
    QCOMPARE(c->combinedOpacity(), 0.9 * 0.1 * 0.7);
    QCOMPARE(geometry->inheritedOpacity(), 0.9 * 0.1 * 0.7);

    b->setOpacity(0);
    renderer.renderScene();

    QVERIFY(b->isSubtreeBlocked());

    // Verify that geometry did not get updated as it is in a blocked
    // subtree
    QCOMPARE(c->combinedOpacity(), 0.9 * 0.1 * 0.7);
    QCOMPARE(geometry->inheritedOpacity(), 0.9 * 0.1 * 0.7);

    renderContext->invalidate();
}

void NodesTest::isBlockedCheck_data()
{
    rhiTestData();
}

void NodesTest::isBlockedCheck()
{
    INIT_RHI();

    QSGRootNode root;
    QSGOpacityNode *opacity = new QSGOpacityNode();
    QSGNode *node = new QSGNode();

    root.appendChildNode(opacity);
    opacity->appendChildNode(node);

    QSGNodeUpdater updater;

    opacity->setOpacity(0);
    QVERIFY(updater.isNodeBlocked(node, &root));

    opacity->setOpacity(1);
    QVERIFY(!updater.isNodeBlocked(node, &root));

    renderContext->invalidate();
}

void NodesTest::textureNodeTextureOwnership_data()
{
    rhiTestData();
}

void NodesTest::textureNodeTextureOwnership()
{
    INIT_RHI();

    { // Check that it is not deleted by default
        QPointer<QSGTexture> texture(new QSGPlainTexture());

        QSGSimpleTextureNode *tn = new QSGSimpleTextureNode();
        QVERIFY(!tn->ownsTexture());

        tn->setTexture(texture);
        delete tn;
        QVERIFY(!texture.isNull());
    }

    { // Check that it is deleted on destruction when we so desire
        QPointer<QSGTexture> texture(new QSGPlainTexture());

        QSGSimpleTextureNode *tn = new QSGSimpleTextureNode();
        tn->setOwnsTexture(true);
        QVERIFY(tn->ownsTexture());

        tn->setTexture(texture);
        delete tn;
        QVERIFY(texture.isNull());
    }

    { // Check that it is deleted on update when we so desire
        QPointer<QSGTexture> oldTexture(new QSGPlainTexture());
        QPointer<QSGTexture> newTexture(new QSGPlainTexture());

        QSGSimpleTextureNode *tn = new QSGSimpleTextureNode();
        tn->setOwnsTexture(true);
        QVERIFY(tn->ownsTexture());

        tn->setTexture(oldTexture);
        QVERIFY(!oldTexture.isNull());
        QVERIFY(!newTexture.isNull());

        tn->setTexture(newTexture);
        QVERIFY(oldTexture.isNull());
        QVERIFY(!newTexture.isNull());

        delete tn;
    }

    renderContext->invalidate();
}

void NodesTest::textureNodeRect_data()
{
    rhiTestData();
}

void NodesTest::textureNodeRect()
{
    INIT_RHI();

    QSGPlainTexture texture;
    texture.setTextureSize(QSize(400, 400));
    QSGSimpleTextureNode tn;
    tn.setTexture(&texture);
    QSGGeometry::TexturedPoint2D *vertices = tn.geometry()->vertexDataAsTexturedPoint2D();

    QSGGeometry::TexturedPoint2D topLeft, bottomLeft, topRight, bottomRight;
    topLeft.set(0, 0, 0, 0);
    bottomLeft.set(0, 0, 0, 1);
    topRight.set(0, 0, 1, 0);
    bottomRight.set(0, 0, 1, 1);
    QCOMPARE(vertices[0], topLeft);
    QCOMPARE(vertices[1], bottomLeft);
    QCOMPARE(vertices[2], topRight);
    QCOMPARE(vertices[3], bottomRight);

    tn.setRect(1, 2, 100, 100);
    topLeft.set(1, 2, 0, 0);
    bottomLeft.set(1, 102, 0, 1);
    topRight.set(101, 2, 1, 0);
    bottomRight.set(101, 102, 1, 1);
    QCOMPARE(vertices[0], topLeft);
    QCOMPARE(vertices[1], bottomLeft);
    QCOMPARE(vertices[2], topRight);
    QCOMPARE(vertices[3], bottomRight);

    tn.setRect(0, 0, 100, 100);
    tn.setSourceRect(100, 100, 200, 200);
    topLeft.set(0, 0, 0.25, 0.25);
    bottomLeft.set(0, 100, 0.25, 0.75);
    topRight.set(100, 0, 0.75, 0.25);
    bottomRight.set(100, 100, 0.75, 0.75);
    QCOMPARE(vertices[0], topLeft);
    QCOMPARE(vertices[1], bottomLeft);
    QCOMPARE(vertices[2], topRight);
    QCOMPARE(vertices[3], bottomRight);

    tn.setSourceRect(300, 300, -200, -200);
    topLeft.set(0, 0, 0.75, 0.75);
    bottomLeft.set(0, 100, 0.75, 0.25);
    topRight.set(100, 0, 0.25, 0.75);
    bottomRight.set(100, 100, 0.25, 0.25);
    QCOMPARE(vertices[0], topLeft);
    QCOMPARE(vertices[1], bottomLeft);
    QCOMPARE(vertices[2], topRight);
    QCOMPARE(vertices[3], bottomRight);

    renderContext->invalidate();
}

QTEST_MAIN(NodesTest);

#include "tst_nodestest.moc"
