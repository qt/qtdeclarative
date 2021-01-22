/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
