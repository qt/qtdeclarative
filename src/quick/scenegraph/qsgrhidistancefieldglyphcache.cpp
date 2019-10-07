/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qsgrhidistancefieldglyphcache_p.h"
#include "qsgcontext_p.h"
#include <QtGui/private/qdistancefield_p.h>
#include <QtCore/qelapsedtimer.h>
#include <QtQml/private/qqmlglobal_p.h>
#include <qmath.h>
#include <qendian.h>

QT_BEGIN_NAMESPACE

DEFINE_BOOL_CONFIG_OPTION(qmlUseGlyphCacheWorkaround, QML_USE_GLYPHCACHE_WORKAROUND)
DEFINE_BOOL_CONFIG_OPTION(qsgPreferFullSizeGlyphCacheTextures, QSG_PREFER_FULLSIZE_GLYPHCACHE_TEXTURES)

#if !defined(QSG_RHI_DISTANCEFIELD_GLYPH_CACHE_PADDING)
#  define QSG_RHI_DISTANCEFIELD_GLYPH_CACHE_PADDING 2
#endif

QSGRhiDistanceFieldGlyphCache::QSGRhiDistanceFieldGlyphCache(QRhi *rhi, const QRawFont &font)
    : QSGDistanceFieldGlyphCache(font)
    , m_rhi(rhi)
{
    // Load a pregenerated cache if the font contains one
    loadPregeneratedCache(font);
}

QSGRhiDistanceFieldGlyphCache::~QSGRhiDistanceFieldGlyphCache()
{
    for (int i = 0; i < m_textures.count(); ++i)
        delete m_textures[i].texture;

    delete m_areaAllocator;

    // should be empty, but just in case
    qDeleteAll(m_pendingDispose);
}

void QSGRhiDistanceFieldGlyphCache::requestGlyphs(const QSet<glyph_t> &glyphs)
{
    QList<GlyphPosition> glyphPositions;
    QVector<glyph_t> glyphsToRender;

    if (m_areaAllocator == nullptr)
        m_areaAllocator = new QSGAreaAllocator(QSize(maxTextureSize(), m_maxTextureCount * maxTextureSize()));

    for (QSet<glyph_t>::const_iterator it = glyphs.constBegin(); it != glyphs.constEnd() ; ++it) {
        glyph_t glyphIndex = *it;

        int padding = QSG_RHI_DISTANCEFIELD_GLYPH_CACHE_PADDING;
        QRectF boundingRect = glyphData(glyphIndex).boundingRect;
        int glyphWidth = qCeil(boundingRect.width()) + distanceFieldRadius() * 2;
        int glyphHeight = qCeil(boundingRect.height()) + distanceFieldRadius() * 2;
        QSize glyphSize(glyphWidth + padding * 2, glyphHeight + padding * 2);
        QRect alloc = m_areaAllocator->allocate(glyphSize);

        if (alloc.isNull()) {
            // Unallocate unused glyphs until we can allocated the new glyph
            while (alloc.isNull() && !m_unusedGlyphs.isEmpty()) {
                glyph_t unusedGlyph = *m_unusedGlyphs.constBegin();

                TexCoord unusedCoord = glyphTexCoord(unusedGlyph);
                QRectF unusedGlyphBoundingRect = glyphData(unusedGlyph).boundingRect;
                int unusedGlyphWidth = qCeil(unusedGlyphBoundingRect.width()) + distanceFieldRadius() * 2;
                int unusedGlyphHeight = qCeil(unusedGlyphBoundingRect.height())  + distanceFieldRadius() * 2;
                m_areaAllocator->deallocate(QRect(unusedCoord.x - padding,
                                                  unusedCoord.y - padding,
                                                  padding * 2 + unusedGlyphWidth,
                                                  padding * 2 + unusedGlyphHeight));

                m_unusedGlyphs.remove(unusedGlyph);
                m_glyphsTexture.remove(unusedGlyph);
                removeGlyph(unusedGlyph);

                alloc = m_areaAllocator->allocate(glyphSize);
            }

            // Not enough space left for this glyph... skip to the next one
            if (alloc.isNull())
                continue;
        }

        TextureInfo *tex = textureInfo(alloc.y() / maxTextureSize());
        alloc = QRect(alloc.x(), alloc.y() % maxTextureSize(), alloc.width(), alloc.height());

        tex->allocatedArea |= alloc;
        Q_ASSERT(tex->padding == padding || tex->padding < 0);
        tex->padding = padding;

        GlyphPosition p;
        p.glyph = glyphIndex;
        p.position = alloc.topLeft() + QPoint(padding, padding);

        glyphPositions.append(p);
        glyphsToRender.append(glyphIndex);
        m_glyphsTexture.insert(glyphIndex, tex);
    }

    setGlyphsPosition(glyphPositions);
    markGlyphsToRender(glyphsToRender);
}

void QSGRhiDistanceFieldGlyphCache::storeGlyphs(const QList<QDistanceField> &glyphs)
{
    typedef QHash<TextureInfo *, QVector<glyph_t> > GlyphTextureHash;
    typedef GlyphTextureHash::const_iterator GlyphTextureHashConstIt;

    GlyphTextureHash glyphTextures;

    QVarLengthArray<QRhiTextureUploadEntry, 32> uploads;
    for (int i = 0; i < glyphs.size(); ++i) {
        QDistanceField glyph = glyphs.at(i);
        glyph_t glyphIndex = glyph.glyph();
        TexCoord c = glyphTexCoord(glyphIndex);
        TextureInfo *texInfo = m_glyphsTexture.value(glyphIndex);

        resizeTexture(texInfo, texInfo->allocatedArea.width(), texInfo->allocatedArea.height());

        glyphTextures[texInfo].append(glyphIndex);

        int padding = texInfo->padding;
        int expectedWidth = qCeil(c.width + c.xMargin * 2);
        glyph = glyph.copy(-padding, -padding,
                           expectedWidth + padding  * 2, glyph.height() + padding * 2);

        if (useTextureResizeWorkaround()) {
            uchar *inBits = glyph.scanLine(0);
            uchar *outBits = texInfo->image.scanLine(int(c.y) - padding) + int(c.x) - padding;
            for (int y = 0; y < glyph.height(); ++y) {
                memcpy(outBits, inBits, glyph.width());
                inBits += glyph.width();
                outBits += texInfo->image.width();
            }
        }

        QRhiTextureSubresourceUploadDescription subresDesc(glyph.constBits(), glyph.width() * glyph.height());
        subresDesc.setSourceSize(QSize(glyph.width(), glyph.height()));
        subresDesc.setDestinationTopLeft(QPoint(c.x - padding, c.y - padding));
        texInfo->uploads.append(QRhiTextureUploadEntry(0, 0, subresDesc));
    }

    if (!m_resourceUpdates)
        m_resourceUpdates = m_rhi->nextResourceUpdateBatch();

    for (int i = 0; i < glyphs.size(); ++i) {
        TextureInfo *texInfo = m_glyphsTexture.value(glyphs.at(i).glyph());
        if (!texInfo->uploads.isEmpty()) {
            QRhiTextureUploadDescription desc;
            desc.setEntries(texInfo->uploads.cbegin(), texInfo->uploads.cend());
            m_resourceUpdates->uploadTexture(texInfo->texture, desc);
            texInfo->uploads.clear();
        }
    }

    for (GlyphTextureHashConstIt i = glyphTextures.constBegin(), cend = glyphTextures.constEnd(); i != cend; ++i) {
        Texture t;
        t.texture = i.key()->texture;
        t.size = i.key()->size;
        t.rhiBased = true;
        setGlyphsTexture(i.value(), t);
    }
}

void QSGRhiDistanceFieldGlyphCache::referenceGlyphs(const QSet<glyph_t> &glyphs)
{
    m_unusedGlyphs -= glyphs;
}

void QSGRhiDistanceFieldGlyphCache::releaseGlyphs(const QSet<glyph_t> &glyphs)
{
    m_unusedGlyphs += glyphs;
}

void QSGRhiDistanceFieldGlyphCache::createTexture(TextureInfo *texInfo,
                                                      int width,
                                                      int height)
{
    QByteArray zeroBuf(width * height, 0);
    createTexture(texInfo, width, height, zeroBuf.constData());
}

void QSGRhiDistanceFieldGlyphCache::createTexture(TextureInfo *texInfo,
                                                  int width,
                                                  int height,
                                                  const void *pixels)
{
    if (useTextureResizeWorkaround() && texInfo->image.isNull()) {
        texInfo->image = QDistanceField(width, height);
        memcpy(texInfo->image.bits(), pixels, width * height);
    }

    texInfo->texture = m_rhi->newTexture(QRhiTexture::RED_OR_ALPHA8, QSize(width, height), 1, QRhiTexture::UsedAsTransferSource);
    if (texInfo->texture->build()) {
        if (!m_resourceUpdates)
            m_resourceUpdates = m_rhi->nextResourceUpdateBatch();

        QRhiTextureSubresourceUploadDescription subresDesc(pixels, width * height);
        subresDesc.setSourceSize(QSize(width, height));
        m_resourceUpdates->uploadTexture(texInfo->texture, QRhiTextureUploadEntry(0, 0, subresDesc));
    } else {
        qWarning("Failed to create distance field glyph cache");
    }

    texInfo->size = QSize(width, height);
}

void QSGRhiDistanceFieldGlyphCache::resizeTexture(TextureInfo *texInfo, int width, int height)
{
    int oldWidth = texInfo->size.width();
    int oldHeight = texInfo->size.height();
    if (width == oldWidth && height == oldHeight)
        return;

    QRhiTexture *oldTexture = texInfo->texture;
    createTexture(texInfo, width, height);

    if (!oldTexture)
        return;

    updateRhiTexture(oldTexture, texInfo->texture, texInfo->size);

    if (!m_resourceUpdates)
        m_resourceUpdates = m_rhi->nextResourceUpdateBatch();

    if (useTextureResizeWorkaround()) {
        QRhiTextureSubresourceUploadDescription subresDesc(texInfo->image.constBits(),
                                                           oldWidth * oldHeight);
        subresDesc.setSourceSize(QSize(oldWidth, oldHeight));
        m_resourceUpdates->uploadTexture(texInfo->texture, QRhiTextureUploadEntry(0, 0, subresDesc));
        texInfo->image = texInfo->image.copy(0, 0, width, height);
    } else {
        m_resourceUpdates->copyTexture(texInfo->texture, oldTexture);
    }

    m_pendingDispose.insert(oldTexture);
}

bool QSGRhiDistanceFieldGlyphCache::useTextureResizeWorkaround() const
{
    static bool set = false;
    static bool useWorkaround = false;
    if (!set) {
        useWorkaround = m_rhi->backend() == QRhi::OpenGLES2 || qmlUseGlyphCacheWorkaround();
        set = true;
    }
    return useWorkaround;
}

bool QSGRhiDistanceFieldGlyphCache::createFullSizeTextures() const
{
    return qsgPreferFullSizeGlyphCacheTextures() && glyphCount() > QT_DISTANCEFIELD_HIGHGLYPHCOUNT();
}

int QSGRhiDistanceFieldGlyphCache::maxTextureSize() const
{
    if (!m_maxTextureSize)
        m_maxTextureSize = m_rhi->resourceLimit(QRhi::TextureSizeMax);
    return m_maxTextureSize;
}

namespace {
    struct Qtdf {
        // We need these structs to be tightly packed, but some compilers we use do not
        // support #pragma pack(1), so we need to hardcode the offsets/sizes in the
        // file format
        enum TableSize {
            HeaderSize = 14,
            GlyphRecordSize = 46,
            TextureRecordSize = 17
        };

        enum Offset {
            // Header
            majorVersion        = 0,
            minorVersion        = 1,
            pixelSize           = 2,
            textureSize         = 4,
            flags               = 8,
            headerPadding       = 9,
            numGlyphs           = 10,

            // Glyph record
            glyphIndex          = 0,
            textureOffsetX      = 4,
            textureOffsetY      = 8,
            textureWidth        = 12,
            textureHeight       = 16,
            xMargin             = 20,
            yMargin             = 24,
            boundingRectX       = 28,
            boundingRectY       = 32,
            boundingRectWidth   = 36,
            boundingRectHeight  = 40,
            textureIndex        = 44,

            // Texture record
            allocatedX          = 0,
            allocatedY          = 4,
            allocatedWidth      = 8,
            allocatedHeight     = 12,
            texturePadding      = 16

        };

        template <typename T>
        static inline T fetch(const char *data, Offset offset)
        {
            return qFromBigEndian<T>(data + int(offset));
        }
    };
}

bool QSGRhiDistanceFieldGlyphCache::loadPregeneratedCache(const QRawFont &font)
{
    // The pregenerated data must be loaded first, otherwise the area allocator
    // will be wrong
    if (m_areaAllocator != nullptr) {
        qWarning("Font cache must be loaded before cache is used");
        return false;
    }

    static QElapsedTimer timer;

    bool profile = QSG_LOG_TIME_GLYPH().isDebugEnabled();
    if (profile)
        timer.start();

    QByteArray qtdfTable = font.fontTable("qtdf");
    if (qtdfTable.isEmpty())
        return false;

    typedef QHash<TextureInfo *, QVector<glyph_t> > GlyphTextureHash;

    GlyphTextureHash glyphTextures;

    if (uint(qtdfTable.size()) < Qtdf::HeaderSize) {
        qWarning("Invalid qtdf table in font '%s'",
                 qPrintable(font.familyName()));
        return false;
    }

    const char *qtdfTableStart = qtdfTable.constData();
    const char *qtdfTableEnd = qtdfTableStart + qtdfTable.size();

    int padding = 0;
    int textureCount = 0;
    {
        quint8 majorVersion = Qtdf::fetch<quint8>(qtdfTableStart, Qtdf::majorVersion);
        quint8 minorVersion = Qtdf::fetch<quint8>(qtdfTableStart, Qtdf::minorVersion);
        if (majorVersion != 5 || minorVersion != 12) {
            qWarning("Invalid version of qtdf table %d.%d in font '%s'",
                     majorVersion,
                     minorVersion,
                     qPrintable(font.familyName()));
            return false;
        }

        qreal pixelSize = qreal(Qtdf::fetch<quint16>(qtdfTableStart, Qtdf::pixelSize));
        m_maxTextureSize = Qtdf::fetch<quint32>(qtdfTableStart, Qtdf::textureSize);
        m_doubleGlyphResolution = Qtdf::fetch<quint8>(qtdfTableStart, Qtdf::flags) == 1;
        padding = Qtdf::fetch<quint8>(qtdfTableStart, Qtdf::headerPadding);

        if (pixelSize <= 0.0) {
            qWarning("Invalid pixel size in '%s'", qPrintable(font.familyName()));
            return false;
        }

        if (m_maxTextureSize <= 0) {
            qWarning("Invalid texture size in '%s'", qPrintable(font.familyName()));
            return false;
        }

        int systemMaxTextureSize = m_rhi->resourceLimit(QRhi::TextureSizeMax);

        if (m_maxTextureSize > systemMaxTextureSize) {
            qWarning("System maximum texture size is %d. This is lower than the value in '%s', which is %d",
                     systemMaxTextureSize,
                     qPrintable(font.familyName()),
                     m_maxTextureSize);
        }

        if (padding != QSG_RHI_DISTANCEFIELD_GLYPH_CACHE_PADDING) {
            qWarning("Padding mismatch in '%s'. Font requires %d, but Qt is compiled with %d.",
                     qPrintable(font.familyName()),
                     padding,
                     QSG_RHI_DISTANCEFIELD_GLYPH_CACHE_PADDING);
        }

        m_referenceFont.setPixelSize(pixelSize);

        quint32 glyphCount = Qtdf::fetch<quint32>(qtdfTableStart, Qtdf::numGlyphs);
        m_unusedGlyphs.reserve(glyphCount);

        const char *allocatorData = qtdfTableStart + Qtdf::HeaderSize;
        {
            m_areaAllocator = new QSGAreaAllocator(QSize(0, 0));
            allocatorData = m_areaAllocator->deserialize(allocatorData, qtdfTableEnd - allocatorData);
            if (allocatorData == nullptr)
                return false;
        }

        if (m_areaAllocator->size().height() % m_maxTextureSize != 0) {
            qWarning("Area allocator size mismatch in '%s'", qPrintable(font.familyName()));
            return false;
        }

        textureCount = m_areaAllocator->size().height() / m_maxTextureSize;
        m_maxTextureCount = qMax(m_maxTextureCount, textureCount);

        const char *textureRecord = allocatorData;
        for (int i = 0; i < textureCount; ++i, textureRecord += Qtdf::TextureRecordSize) {
            if (textureRecord + Qtdf::TextureRecordSize > qtdfTableEnd) {
                qWarning("qtdf table too small in font '%s'.",
                         qPrintable(font.familyName()));
                return false;
            }

            TextureInfo *tex = textureInfo(i);
            tex->allocatedArea.setX(Qtdf::fetch<quint32>(textureRecord, Qtdf::allocatedX));
            tex->allocatedArea.setY(Qtdf::fetch<quint32>(textureRecord, Qtdf::allocatedY));
            tex->allocatedArea.setWidth(Qtdf::fetch<quint32>(textureRecord, Qtdf::allocatedWidth));
            tex->allocatedArea.setHeight(Qtdf::fetch<quint32>(textureRecord, Qtdf::allocatedHeight));
            tex->padding = Qtdf::fetch<quint8>(textureRecord, Qtdf::texturePadding);
        }

        const char *glyphRecord = textureRecord;
        for (quint32 i = 0; i < glyphCount; ++i, glyphRecord += Qtdf::GlyphRecordSize) {
            if (glyphRecord + Qtdf::GlyphRecordSize > qtdfTableEnd) {
                qWarning("qtdf table too small in font '%s'.",
                         qPrintable(font.familyName()));
                return false;
            }

            glyph_t glyph = Qtdf::fetch<quint32>(glyphRecord, Qtdf::glyphIndex);
            m_unusedGlyphs.insert(glyph);

            GlyphData &glyphData = emptyData(glyph);

#define FROM_FIXED_POINT(value) \
(((qreal)value)/(qreal)65536)

            glyphData.texCoord.x = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::textureOffsetX));
            glyphData.texCoord.y = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::textureOffsetY));
            glyphData.texCoord.width = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::textureWidth));
            glyphData.texCoord.height = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::textureHeight));
            glyphData.texCoord.xMargin = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::xMargin));
            glyphData.texCoord.yMargin = FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::yMargin));
            glyphData.boundingRect.setX(FROM_FIXED_POINT(Qtdf::fetch<qint32>(glyphRecord, Qtdf::boundingRectX)));
            glyphData.boundingRect.setY(FROM_FIXED_POINT(Qtdf::fetch<qint32>(glyphRecord, Qtdf::boundingRectY)));
            glyphData.boundingRect.setWidth(FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::boundingRectWidth)));
            glyphData.boundingRect.setHeight(FROM_FIXED_POINT(Qtdf::fetch<quint32>(glyphRecord, Qtdf::boundingRectHeight)));

#undef FROM_FIXED_POINT

            int textureIndex = Qtdf::fetch<quint16>(glyphRecord, Qtdf::textureIndex);
            if (textureIndex < 0 || textureIndex >= textureCount) {
                qWarning("Invalid texture index %d (texture count == %d) in '%s'",
                         textureIndex,
                         textureCount,
                         qPrintable(font.familyName()));
                return false;
            }


            TextureInfo *texInfo = textureInfo(textureIndex);
            m_glyphsTexture.insert(glyph, texInfo);

            glyphTextures[texInfo].append(glyph);
        }

        const uchar *textureData = reinterpret_cast<const uchar *>(glyphRecord);
        for (int i = 0; i < textureCount; ++i) {

            TextureInfo *texInfo = textureInfo(i);

            int width = texInfo->allocatedArea.width();
            int height = texInfo->allocatedArea.height();
            qint64 size = width * height;
            if (reinterpret_cast<const char *>(textureData + size) > qtdfTableEnd) {
                qWarning("qtdf table too small in font '%s'.",
                         qPrintable(font.familyName()));
                return false;
            }

            createTexture(texInfo, width, height, textureData);

            QVector<glyph_t> glyphs = glyphTextures.value(texInfo);

            Texture t;
            t.texture = texInfo->texture;
            t.size = texInfo->size;
            t.rhiBased = true;

            setGlyphsTexture(glyphs, t);

            textureData += size;
        }
    }

    if (profile) {
        quint64 now = timer.elapsed();
        qCDebug(QSG_LOG_TIME_GLYPH,
                "distancefield: %d pre-generated glyphs loaded in %dms",
                m_unusedGlyphs.size(),
                (int) now);
    }

    return true;
}

void QSGRhiDistanceFieldGlyphCache::commitResourceUpdates(QRhiResourceUpdateBatch *mergeInto)
{
    if (m_resourceUpdates) {
        mergeInto->merge(m_resourceUpdates);
        m_resourceUpdates->release();
        m_resourceUpdates = nullptr;
    }

    // now let's assume the resource updates will be committed in this frame
    for (QRhiTexture *t : m_pendingDispose)
        t->releaseAndDestroyLater(); // will be releaseAndDestroyed after the frame is submitted -> safe

    m_pendingDispose.clear();
}

bool QSGRhiDistanceFieldGlyphCache::eightBitFormatIsAlphaSwizzled() const
{
    // return true when the shaders for 8-bit formats need .a instead of .r
    // when sampling the texture
    return !m_rhi->isFeatureSupported(QRhi::RedOrAlpha8IsRed);
}

QT_END_NAMESPACE
