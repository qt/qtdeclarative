// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testrhiitem.h"
#include <QFile>

static const QSize TEX_SIZE(512, 512);

void TestRenderer::initialize(QRhi *rhi, QRhiTexture *outputTexture)
{
    m_rhi = rhi;
    m_output = outputTexture;

    if (!m_ds) {
        m_ds.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_output->pixelSize()));
        m_ds->create();
    } else if (m_ds->pixelSize() != m_output->pixelSize()) {
        m_ds->setPixelSize(m_output->pixelSize());
        m_ds->create();
    }

    if (!m_rt) {
        m_rt.reset(m_rhi->newTextureRenderTarget({ { m_output }, m_ds.data() }));
        m_rp.reset(m_rt->newCompatibleRenderPassDescriptor());
        m_rt->setRenderPassDescriptor(m_rp.data());
        m_rt->create();
    }

    if (!scene.vbuf)
        initScene();

    const QSize outputSize = m_output->pixelSize();
    scene.mvp = m_rhi->clipSpaceCorrMatrix();
    scene.mvp.perspective(45.0f, outputSize.width() / (float) outputSize.height(), 0.01f, 1000.0f);
    scene.mvp.translate(0, 0, -4);
    if (!scene.resourceUpdates)
        scene.resourceUpdates = m_rhi->nextResourceUpdateBatch();
    scene.resourceUpdates->updateDynamicBuffer(scene.ubuf.data(), 0, 64, scene.mvp.constData());
}

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

void TestRenderer::updateTexture()
{
    QImage img(TEX_SIZE, QImage::Format_RGBA8888);
    img.fill(itemData.color);

    if (!scene.resourceUpdates)
        scene.resourceUpdates = m_rhi->nextResourceUpdateBatch();

    scene.resourceUpdates->uploadTexture(scene.tex.data(), img);
}

void TestRenderer::initScene()
{
    static const float tri[] = {
        0.0f, 0.5f,     0.0f, 1.0f,
        -0.5f, -0.5f,   0.0f, 0.0f,
        0.5f, -0.5f,    1.0f, 0.0f
    };

    scene.vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(tri)));
    scene.vbuf->create();

    scene.resourceUpdates = m_rhi->nextResourceUpdateBatch();
    scene.resourceUpdates->uploadStaticBuffer(scene.vbuf.data(), tri);

    scene.ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68));
    scene.ubuf->create();

    const qint32 flip = m_rhi->isYUpInFramebuffer() ? 1 : 0;
    scene.resourceUpdates->updateDynamicBuffer(scene.ubuf.data(), 64, 4, &flip);

    scene.tex.reset(m_rhi->newTexture(QRhiTexture::RGBA8, TEX_SIZE));
    scene.tex->create();

    scene.sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                               QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    scene.sampler->create();

    scene.srb.reset(m_rhi->newShaderResourceBindings());
    scene.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, scene.ubuf.data()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, scene.tex.data(), scene.sampler.data())
    });
    scene.srb->create();

    scene.ps.reset(m_rhi->newGraphicsPipeline());
    scene.ps->setDepthTest(true);
    scene.ps->setDepthWrite(true);
    scene.ps->setDepthOp(QRhiGraphicsPipeline::Less);
    scene.ps->setCullMode(QRhiGraphicsPipeline::Back);
    scene.ps->setFrontFace(QRhiGraphicsPipeline::CCW);
    QShader vs = getShader(QLatin1String(":/texture.vert.qsb"));
    Q_ASSERT(vs.isValid());
    QShader fs = getShader(QLatin1String(":/texture.frag.qsb"));
    Q_ASSERT(fs.isValid());
    scene.ps->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 4 * sizeof(float) },
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
    });
    scene.ps->setVertexInputLayout(inputLayout);
    scene.ps->setShaderResourceBindings(scene.srb.data());
    scene.ps->setRenderPassDescriptor(m_rp.data());
    scene.ps->create();
}

void TestRenderer::synchronize(RhiItem *rhiItem)
{
    TestRhiItem *item = static_cast<TestRhiItem *>(rhiItem);
    if (item->color() != itemData.color) {
        itemData.color = item->color();
        updateTexture();
    }
}

void TestRenderer::render(QRhiCommandBuffer *cb)
{
    QRhiResourceUpdateBatch *rub = scene.resourceUpdates;
    if (rub)
        scene.resourceUpdates = nullptr;

    const QColor clearColor = QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f);
    cb->beginPass(m_rt.data(), clearColor, { 1.0f, 0 }, rub);

    cb->setGraphicsPipeline(scene.ps.data());
    const QSize outputSize = m_output->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { scene.vbuf.data(), 0 },
    };
    cb->setVertexInput(0, 1, vbufBindings);
    cb->draw(3);

    cb->endPass();
}

void TestRhiItem::setColor(QColor c)
{
    if (m_color == c)
        return;

    m_color = c;
    emit colorChanged();
    update();
}
