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

#include "qsgd3d12glyphcache_p.h"
#include "qsgd3d12engine_p.h"

QT_BEGIN_NAMESPACE

// Keep it simple: allocate a large texture and never resize.
static const int TEXTURE_WIDTH = 2048;
static const int TEXTURE_HEIGHT = 2048;

QSGD3D12GlyphCache::QSGD3D12GlyphCache(QSGD3D12Engine *engine, QFontEngine::GlyphFormat format, const QTransform &matrix)
    : QTextureGlyphCache(format, matrix),
      m_engine(engine)
{
}

QSGD3D12GlyphCache::~QSGD3D12GlyphCache()
{
    if (m_id)
        m_engine->releaseTexture(m_id);
}

void QSGD3D12GlyphCache::createTextureData(int, int)
{
    qDebug("create");
    m_id = m_engine->genTexture();
    Q_ASSERT(m_id);
    m_engine->createTexture(m_id, QSize(TEXTURE_WIDTH, TEXTURE_HEIGHT),
                            QImage::Format_ARGB32_Premultiplied, QSGD3D12Engine::CreateWithAlpha);
}

void QSGD3D12GlyphCache::resizeTextureData(int, int)
{
    // nothing to do here
}

void QSGD3D12GlyphCache::beginFillTexture()
{
    qDebug("begin");
}

void QSGD3D12GlyphCache::fillTexture(const Coord &c, glyph_t glyph, QFixed subPixelPosition)
{
    qDebug("fill %x", glyph);
}

void QSGD3D12GlyphCache::endFillTexture()
{
    qDebug("end");
}

int QSGD3D12GlyphCache::glyphPadding() const
{
    return 1;
}

int QSGD3D12GlyphCache::maxTextureWidth() const
{
    return TEXTURE_WIDTH;
}

int QSGD3D12GlyphCache::maxTextureHeight() const
{
    return TEXTURE_HEIGHT;
}

void QSGD3D12GlyphCache::activateTexture()
{
    if (m_id)
        m_engine->activateTexture(m_id);
}

QT_END_NAMESPACE
