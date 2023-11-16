// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsggradientcache_p.h"

#include <QtGui/private/qdrawhelper_p.h>
#include <QtGui/rhi/qrhi.h>

#include <QtQuick/qsgtexture.h>
#include <QtQuick/private/qsgplaintexture_p.h>

QT_BEGIN_NAMESPACE

static void generateGradientColorTable(const QSGGradientCacheKey &gradient,
                                       uint *colorTable, int size, float opacity)
{
    int pos = 0;
    const QGradientStops &s = gradient.stops;
    Q_ASSERT(!s.isEmpty());
    const bool colorInterpolation = true;

    uint alpha = qRound(opacity * 256);
    uint current_color = ARGB_COMBINE_ALPHA(s[0].second.rgba(), alpha);
    qreal incr = 1.0 / qreal(size);
    qreal fpos = 1.5 * incr;
    colorTable[pos++] = ARGB2RGBA(qPremultiply(current_color));

    while (fpos <= s.first().first) {
        colorTable[pos] = colorTable[pos - 1];
        pos++;
        fpos += incr;
    }

    if (colorInterpolation)
        current_color = qPremultiply(current_color);

    const int sLast = s.size() - 1;
    for (int i = 0; i < sLast; ++i) {
        qreal delta = 1/(s[i+1].first - s[i].first);
        uint next_color = ARGB_COMBINE_ALPHA(s[i + 1].second.rgba(), alpha);
        if (colorInterpolation)
            next_color = qPremultiply(next_color);

        while (fpos < s[i+1].first && pos < size) {
            int dist = int(256 * ((fpos - s[i].first) * delta));
            int idist = 256 - dist;
            if (colorInterpolation)
                colorTable[pos] = ARGB2RGBA(INTERPOLATE_PIXEL_256(current_color, idist, next_color, dist));
            else
                colorTable[pos] = ARGB2RGBA(qPremultiply(INTERPOLATE_PIXEL_256(current_color, idist, next_color, dist)));
            ++pos;
            fpos += incr;
        }
        current_color = next_color;
    }

    uint last_color = ARGB2RGBA(qPremultiply(ARGB_COMBINE_ALPHA(s[sLast].second.rgba(), alpha)));
    for ( ; pos < size; ++pos)
        colorTable[pos] = last_color;

    colorTable[size-1] = last_color;
}

QSGGradientCache::QSGGradientCache()
{
    static int envLimit = qEnvironmentVariableIntValue("QT_QUICKSHAPES_MAX_GRADIENTS");
    m_cache.setMaxCost(envLimit > 0 ? envLimit - 1 : 255);
}

QSGGradientCache::~QSGGradientCache()
{
    qDeleteAll(m_textures);
}

QSGGradientCache *QSGGradientCache::cacheForRhi(QRhi *rhi)
{
    static QHash<QRhi *, QSGGradientCache *> caches;
    auto it = caches.constFind(rhi);
    if (it != caches.constEnd())
        return *it;

    QSGGradientCache *cache = new QSGGradientCache;
    rhi->addCleanupCallback([cache](QRhi *rhi) {
        caches.remove(rhi);
        delete cache;
    });
    caches.insert(rhi, cache);
    return cache;
}

QSGTexture *QSGGradientCache::get(const QSGGradientCacheKey &grad)
{
    QSGPlainTexture *tx = nullptr;

    // How the cache works:
    // m_textures is a list of texture pointers.
    // m_cache holds indexes into the m_textures list.
    // As long as m_cache is not full, we add new texture pointers to the end of the list.
    // When m_cache is full, we instead reuse one of the existing texture pointers.
    // So, the returned texture pointers are never invalidated, although their content may change.
    //
    // The trick to find which texture to reuse is the IndexHolder object. When m_cache is full,
    // each insert() will cause an object to be deleted to make room. The IndexHolder destructor
    // then stores the index of the cleaned-out object in m_freeIndex. That is then used for the
    // next insertion.

    const IndexHolder *ih = m_cache[grad];
    if (ih) {
        tx = m_textures[ih->idx];
    } else {
        qsizetype newIdx = m_freeIndex;
        if (newIdx < 0) {
            newIdx = m_textures.size();
            m_textures.append(new QSGPlainTexture);
        }
        tx = m_textures[newIdx];
        setTextureData(tx, grad);
        m_cache.insert(grad, new IndexHolder{newIdx, &m_freeIndex});
    }
    return tx;
}

void QSGGradientCache::setTextureData(QSGPlainTexture *tx, const QSGGradientCacheKey &grad)
{
    static const int W = 1024; // texture size is 1024x1
    QImage gradTab(W, 1, QImage::Format_RGBA8888_Premultiplied);
    if (!grad.stops.isEmpty())
        generateGradientColorTable(grad, reinterpret_cast<uint *>(gradTab.bits()), W, 1.0f);
    else
        gradTab.fill(Qt::black);
    tx->setImage(gradTab);
    switch (grad.spread) {
    case QGradient::PadSpread:
        tx->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        tx->setVerticalWrapMode(QSGTexture::ClampToEdge);
        break;
    case QGradient::RepeatSpread:
        tx->setHorizontalWrapMode(QSGTexture::Repeat);
        tx->setVerticalWrapMode(QSGTexture::Repeat);
        break;
    case QGradient::ReflectSpread:
        tx->setHorizontalWrapMode(QSGTexture::MirroredRepeat);
        tx->setVerticalWrapMode(QSGTexture::MirroredRepeat);
        break;
    default:
        qWarning("Unknown gradient spread mode %d", grad.spread);
        break;
    }
    tx->setFiltering(QSGTexture::Linear);
}

QT_END_NAMESPACE
