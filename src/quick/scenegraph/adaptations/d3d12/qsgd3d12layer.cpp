/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgd3d12layer_p.h"

QT_BEGIN_NAMESPACE

QSGD3D12Layer::QSGD3D12Layer(QSGD3D12RenderContext *rc)
    : m_rc(rc)
{
}

QSGD3D12Layer::~QSGD3D12Layer()
{
    cleanup();
}

int QSGD3D12Layer::textureId() const
{
    return 0;
}

QSize QSGD3D12Layer::textureSize() const
{
    return QSize();
}

bool QSGD3D12Layer::hasAlphaChannel() const
{
    return true;
}

bool QSGD3D12Layer::hasMipmaps() const
{
    // ###
    return false;
}

QRectF QSGD3D12Layer::normalizedTextureSubRect() const
{
    return QRectF(0, 0, 1, 1); // ### mirror h/v
}

void QSGD3D12Layer::bind()
{

}

bool QSGD3D12Layer::updateTexture()
{
    return false;
}

void QSGD3D12Layer::setItem(QSGNode *item)
{

}

void QSGD3D12Layer::setRect(const QRectF &rect)
{

}

void QSGD3D12Layer::setSize(const QSize &size)
{

}

void QSGD3D12Layer::scheduleUpdate()
{

}

QImage QSGD3D12Layer::toImage() const
{
    // ### figure out something for item grab support
    return QImage();
}

void QSGD3D12Layer::setLive(bool live)
{

}

void QSGD3D12Layer::setRecursive(bool recursive)
{

}

void QSGD3D12Layer::setFormat(uint format)
{

}

void QSGD3D12Layer::setHasMipmaps(bool mipmap)
{

}

void QSGD3D12Layer::setDevicePixelRatio(qreal ratio)
{

}

void QSGD3D12Layer::setMirrorHorizontal(bool mirror)
{

}

void QSGD3D12Layer::setMirrorVertical(bool mirror)
{

}

void QSGD3D12Layer::markDirtyTexture()
{

}

void QSGD3D12Layer::invalidated()
{
    cleanup();
}

void QSGD3D12Layer::cleanup()
{
}

QT_END_NAMESPACE
