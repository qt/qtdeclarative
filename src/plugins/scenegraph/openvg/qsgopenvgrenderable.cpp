// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgopenvgrenderable.h"

QT_BEGIN_NAMESPACE

QSGOpenVGRenderable::QSGOpenVGRenderable()
    : m_opacity(1.0f)
{
    m_opacityPaint = vgCreatePaint();
}

QSGOpenVGRenderable::~QSGOpenVGRenderable()
{
    vgDestroyPaint(m_opacityPaint);
}

void QSGOpenVGRenderable::setOpacity(float opacity)
{
    if (m_opacity == opacity)
        return;

    m_opacity = opacity;
    VGfloat values[] = {
        1.0f, 1.0f, 1.0f, m_opacity
    };
    vgSetParameterfv(m_opacityPaint, VG_PAINT_COLOR, 4, values);
}

float QSGOpenVGRenderable::opacity() const
{
    return m_opacity;
}

VGPaint QSGOpenVGRenderable::opacityPaint() const
{
    return m_opacityPaint;
}

void QSGOpenVGRenderable::setTransform(const QOpenVGMatrix &transform)
{
    m_transform = transform;
}

const QOpenVGMatrix &QSGOpenVGRenderable::transform() const
{
    return m_transform;
}

QT_END_NAMESPACE
