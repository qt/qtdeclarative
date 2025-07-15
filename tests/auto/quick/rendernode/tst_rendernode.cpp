// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <private/qsgrendernode_p.h>
#include <rhi/qrhi.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

#if QT_CONFIG(opengl)
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#endif

class tst_rendernode: public QQmlDataTest
{
    Q_OBJECT

public:
    tst_rendernode();

private slots:
    void test_data();
    void test();
#if QT_CONFIG(opengl)
    void gltest_data();
    void gltest();
#endif

private:
    QQuickView *createView(const QString &file, QWindow *parent, int x, int y, int w, int h);
    bool isRunningOnRhi() const;
};

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

// *****
// *  *
// * *
// *
// assumes the top-left scenegraph coordinate system, will be scaled to the item size
static float vertexData[] = {
    0,  0,   0, 0, 1, // blue
    0,  1,   0, 0, 1,
    1,  0,   0, 0, 1
};

class SimpleNode : public QSGRenderNode
{
public:
    SimpleNode(QQuickWindow *window)
        : m_window(window)
    {
    }

    StateFlags changedStates() const override
    {
        return ViewportState; // nothing else matters in Qt 6
    }

    RenderingFlags flags() const override
    {
        // this node uses QRhi directly and is also well behaving depth-wise
        return NoExternalRendering | DepthAwareRendering;
    }

    void prepare() override
    {
        QSGRendererInterface *rif = m_window->rendererInterface();
        QRhi *rhi = static_cast<QRhi *>(rif->getResource(m_window, QSGRendererInterface::RhiResource));
        QVERIFY(rhi);

        QSGRenderNodePrivate *d = QSGRenderNodePrivate::get(this);
        QRhiRenderTarget *rt = d->m_rt.rt;
        QRhiCommandBuffer *cb = d->m_rt.cb;

        QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();

        if (!m_vbuf) {
            m_vbuf.reset(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData)));
            QVERIFY(m_vbuf->create());
            u->uploadStaticBuffer(m_vbuf.data(), vertexData);
        }

        if (!m_ubuf) {
            m_ubuf.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68));
            QVERIFY(m_ubuf->create());
        }

        if (!m_srb) {
            m_srb.reset(rhi->newShaderResourceBindings());
            m_srb->setBindings({
                                   QRhiShaderResourceBinding::uniformBuffer(0,
                                   QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                   m_ubuf.get())
                               });
            m_srb->create();
        }

        if (!m_ps) {
            m_ps.reset(rhi->newGraphicsPipeline());

            const QShader vs = getShader(QLatin1String(":/shaders/color.vert.qsb"));
            if (!vs.isValid())
                qFatal("Failed to load shader pack (vertex)");
            const QShader fs = getShader(QLatin1String(":/shaders/color.frag.qsb"));
            if (!fs.isValid())
                qFatal("Failed to load shader pack (fragment)");

            m_ps->setShaderStages({
                { QRhiShaderStage::Vertex, vs },
                { QRhiShaderStage::Fragment, fs }
            });

            m_ps->setCullMode(QRhiGraphicsPipeline::Back);
            // important to test against what's already in the depth buffer from the opaque pass
            m_ps->setDepthTest(true);
            m_ps->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
            // we are in the alpha pass always so not writing out the depth
            m_ps->setDepthWrite(false);

            QRhiVertexInputLayout inputLayout;
            inputLayout.setBindings({
                { 5 * sizeof(float) }
            });
            inputLayout.setAttributes({
                { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
                { 0, 1, QRhiVertexInputAttribute::Float3, 2 * sizeof(float) }
            });

            m_ps->setVertexInputLayout(inputLayout);
            m_ps->setShaderResourceBindings(m_srb.data());
            m_ps->setRenderPassDescriptor(rt->renderPassDescriptor());

            QVERIFY(m_ps->create());
        }

        // follow what the scenegraph tells us, hence DepthAwareRendering from
        // flags, the downside is that we are stuck with the scenegraph
        // coordinate system but that's enough for this test.
        QMatrix4x4 mvp = *projectionMatrix() * *matrix();

        mvp.scale(m_itemSize.width(), m_itemSize.height());
        u->updateDynamicBuffer(m_ubuf.data(), 0, 64, mvp.constData());

        const float opacity = inheritedOpacity();
        u->updateDynamicBuffer(m_ubuf.data(), 64, 4, &opacity);

        cb->resourceUpdate(u);
    }

    void render(const RenderState *) override
    {
        QSGRenderNodePrivate *d = QSGRenderNodePrivate::get(this);
        QRhiRenderTarget *rt = d->m_rt.rt;
        QRhiCommandBuffer *cb = d->m_rt.cb;

        cb->setGraphicsPipeline(m_ps.data());
        cb->setViewport({ 0, 0, float(rt->pixelSize().width()), float(rt->pixelSize().height()) });
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
        cb->setVertexInput(0, 1, &vbufBinding);
        cb->draw(3);
    }

    QQuickWindow *m_window;
    QScopedPointer<QRhiBuffer> m_vbuf;
    QScopedPointer<QRhiBuffer> m_ubuf;
    QScopedPointer<QRhiShaderResourceBindings> m_srb;
    QScopedPointer<QRhiGraphicsPipeline> m_ps;
    QSizeF m_itemSize;
};

class SimpleItem : public QQuickItem
{
    Q_OBJECT
public:
    SimpleItem()
    {
        setFlag(ItemHasContents, true);
    }
protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override
    {
        if (size().isEmpty()) {
            delete oldNode;
            return nullptr;
        }

        SimpleNode *node = static_cast<SimpleNode *>(oldNode);
        if (!node)
            node = new SimpleNode(window());

        node->m_itemSize = size();

        return node;
    }
};

#if QT_CONFIG(opengl)
class GLNode : public QSGRenderNode
{
public:
    StateFlags changedStates() const override
    {
        return ViewportState; // nothing else matters in Qt 6
    }

    RenderingFlags flags() const override
    {
        return {};
    }

    void prepare() override
    {
        QVERIFY(QOpenGLContext::currentContext());
    }

    void render(const RenderState *) override
    {
        QVERIFY(QOpenGLContext::currentContext());
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        QVERIFY(f);

        GLint viewport[4];
        f->glGetIntegerv(GL_VIEWPORT, viewport);
        QCOMPARE(viewport[0], 0);
        QCOMPARE(viewport[1], 0);
        QCOMPARE(viewport[2], 320);
        QCOMPARE(viewport[3], 200);

        f->glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        f->glClear(GL_COLOR_BUFFER_BIT);
    }
};

class GLItem : public QQuickItem
{
    Q_OBJECT
public:
    GLItem()
    {
        setFlag(ItemHasContents, true);
    }
protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override
    {
        if (size().isEmpty()) {
            delete oldNode;
            return nullptr;
        }

        GLNode *node = static_cast<GLNode *>(oldNode);
        if (!node)
            node = new GLNode;

        return node;
    }
};
#endif // QT_CONFIG(opengl)

tst_rendernode::tst_rendernode()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qmlRegisterType<SimpleItem>("Test", 1, 0, "SimpleItem");
#if QT_CONFIG(opengl)
    qmlRegisterType<GLItem>("Test", 1, 0, "GLSimpleItem");
#endif
}

void tst_rendernode::test_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("simple") << QStringLiteral("simple.qml");
}

void tst_rendernode::test()
{
    SKIP_IF_NO_WINDOW_GRAB;

    if (!isRunningOnRhi())
        QSKIP("Skipping QSGRenderNode test due to not running with QRhi");

    QFETCH(QString, file);

    QScopedPointer<QQuickView> view(createView(file, nullptr, 100, 100, 320, 200));
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    QImage result = view->grabWindow();

    const int maxFuzz = 5;
    // red background
    int x = 10, y = 10;
    QVERIFY(qAbs(qRed(result.pixel(x, y)) - 255) < maxFuzz);
    QVERIFY(qAbs(qGreen(result.pixel(x, y))) < maxFuzz);
    QVERIFY(qAbs(qBlue(result.pixel(x, y))) < maxFuzz);

    // gray rectangle in the middle
    x = result.width() / 2;
    y = result.height() / 2;
    QVERIFY(qAbs(qRed(result.pixel(x, y)) - 128) < maxFuzz);
    QVERIFY(qAbs(qGreen(result.pixel(x, y)) - 128) < maxFuzz);
    QVERIFY(qAbs(qBlue(result.pixel(x, y)) - 128) < maxFuzz);

    // check a bit up and left, this catches if the triangle is not depth
    // tested correctly and so appears above the gray rect, not below as it should
    x = result.width() / 2 - 5;
    y = result.height() / 2 - 5;
    QVERIFY(qAbs(qRed(result.pixel(x, y)) - 128) < maxFuzz);
    QVERIFY(qAbs(qGreen(result.pixel(x, y)) - 128) < maxFuzz);
    QVERIFY(qAbs(qBlue(result.pixel(x, y)) - 128) < maxFuzz);

    // in search for blue pixels
    int blueCount = 0;
    for (y = 0; y < result.height(); ++y) {
        for (x = 0; x < result.width(); ++x) {
            if (qAbs(qRed(result.pixel(x, y))) < maxFuzz
                    && qAbs(qGreen(result.pixel(x, y))) < maxFuzz
                    && qAbs(qBlue(result.pixel(x, y)) - 255) < maxFuzz)
            {
                ++blueCount;
            }
        }
    }

    // if the blue triangle is rendered by SimpleNode, there should be lots of
    // blue pixels present
    QVERIFY(blueCount > 5000);
}

#if QT_CONFIG(opengl)
void tst_rendernode::gltest_data()
{
    QTest::addColumn<QString>("file");

    QTest::newRow("simple") << QStringLiteral("glsimple.qml");
    QTest::newRow("rendernode only") << QStringLiteral("glsimple_only.qml");
}

void tst_rendernode::gltest()
{
    SKIP_IF_NO_WINDOW_GRAB;

    if (!isRunningOnRhi())
        QSKIP("Skipping QSGRenderNode test due to not running with QRhi");

    if (QQuickWindow::graphicsApi() != QSGRendererInterface::OpenGL)
        QSKIP("Skipping test due to not using OpenGL");

    QFETCH(QString, file);

    QScopedPointer<QQuickView> view(createView(file, nullptr, 100, 100, 320, 200));
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    QImage result = view->grabWindow();

    const int maxFuzz = 5;

    // green
    int x = 10, y = 10;
    QVERIFY(qAbs(qRed(result.pixel(x, y))) < maxFuzz);
    QVERIFY(qAbs(qGreen(result.pixel(x, y)) - 255) < maxFuzz);
    QVERIFY(qAbs(qBlue(result.pixel(x, y))) < maxFuzz);

    // gray rectangle in the middle
    x = result.width() / 2;
    y = result.height() / 2;
    QVERIFY(qAbs(qRed(result.pixel(x, y)) - 128) < maxFuzz);
    QVERIFY(qAbs(qGreen(result.pixel(x, y)) - 128) < maxFuzz);
    QVERIFY(qAbs(qBlue(result.pixel(x, y)) - 128) < maxFuzz);
}
#endif // QT_CONFIG(opengl)

QQuickView *tst_rendernode::createView(const QString &file, QWindow *parent, int x, int y, int w, int h)
{
    QQuickView *view = new QQuickView(parent);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->setSource(testFileUrl(file));
    if (x >= 0 && y >= 0)
        view->setPosition(x, y);
    if (w >= 0 && h >= 0)
        view->resize(w, h);
    view->show();
    return view;
}

bool tst_rendernode::isRunningOnRhi() const
{
    static bool retval = false;
    static bool decided = false;
    if (!decided) {
        decided = true;
        QQuickView dummy;
        dummy.show();
        if (QTest::qWaitForWindowExposed(&dummy)) {
            QSGRendererInterface::GraphicsApi api = dummy.rendererInterface()->graphicsApi();
            retval = QSGRendererInterface::isApiRhiBased(api);
        }
        dummy.hide();
    }
    return retval;
}

QTEST_MAIN(tst_rendernode)

#include "tst_rendernode.moc"
