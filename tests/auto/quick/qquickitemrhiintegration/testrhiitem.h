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

#ifndef TESTRHIITEM_H
#define TESTRHIITEM_H

#include "rhiitem.h"

class TestRenderer : public RhiItemRenderer
{
public:
    void initialize(QRhi *rhi, QRhiTexture *outputTexture) override;
    void synchronize(RhiItem *item) override;
    void render(QRhiCommandBuffer *cb) override;

private:
    QRhi *m_rhi = nullptr;
    QRhiTexture *m_output = nullptr;
    QScopedPointer<QRhiRenderBuffer> m_ds;
    QScopedPointer<QRhiTextureRenderTarget> m_rt;
    QScopedPointer<QRhiRenderPassDescriptor> m_rp;

    struct {
        QRhiResourceUpdateBatch *resourceUpdates = nullptr;
        QScopedPointer<QRhiBuffer> vbuf;
        QScopedPointer<QRhiBuffer> ubuf;
        QScopedPointer<QRhiShaderResourceBindings> srb;
        QScopedPointer<QRhiGraphicsPipeline> ps;
        QScopedPointer<QRhiSampler> sampler;
        QScopedPointer<QRhiTexture> tex;
        QMatrix4x4 mvp;
    } scene;

    struct {
        QColor color;
    } itemData;

    void initScene();
    void updateTexture();
};

class TestRhiItem : public RhiItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TestRhiItem)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    RhiItemRenderer *createRenderer() override { return new TestRenderer; }

    QColor color() const { return m_color; }
    void setColor(QColor c);

signals:
    void colorChanged();

private:
    QColor m_color;
};

#endif
