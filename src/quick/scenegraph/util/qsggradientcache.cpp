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
    QSGPlainTexture *tx = m_textures[grad];
    if (!tx) {
        static const int W = 1024; // texture size is 1024x1
        QImage gradTab(W, 1, QImage::Format_RGBA8888_Premultiplied);
        if (!grad.stops.isEmpty())
            generateGradientColorTable(grad, reinterpret_cast<uint *>(gradTab.bits()), W, 1.0f);
        else
            gradTab.fill(Qt::black);
        tx = new QSGPlainTexture;
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
        m_textures[grad] = tx;
    }
    return tx;
}


QT_END_NAMESPACE
