// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgopenvgfontglyphcache.h"
#include "qsgopenvghelpers.h"
#include <private/qfontengine_p.h>
#include <private/qrawfont_p.h>

QT_BEGIN_NAMESPACE

QSGOpenVGFontGlyphCacheManager::QSGOpenVGFontGlyphCacheManager()
{

}

QSGOpenVGFontGlyphCacheManager::~QSGOpenVGFontGlyphCacheManager()
{
    qDeleteAll(m_caches);
}

QSGOpenVGFontGlyphCache *QSGOpenVGFontGlyphCacheManager::cache(const QRawFont &font)
{
    return m_caches.value(font, nullptr);
}

void QSGOpenVGFontGlyphCacheManager::insertCache(const QRawFont &font, QSGOpenVGFontGlyphCache *cache)
{
    m_caches.insert(font, cache);
}

QSGOpenVGFontGlyphCache::QSGOpenVGFontGlyphCache(QSGOpenVGFontGlyphCacheManager *manager, const QRawFont &font)
    : m_manager(manager)
{
    m_referenceFont = font;
    QRawFontPrivate *fontD = QRawFontPrivate::get(font);
    m_glyphCount = fontD->fontEngine->glyphCount();
    m_font = vgCreateFont(0);
}

QSGOpenVGFontGlyphCache::~QSGOpenVGFontGlyphCache()
{
    if (m_font != VG_INVALID_HANDLE)
        vgDestroyFont(m_font);
}

void QSGOpenVGFontGlyphCache::populate(const QVector<quint32> &glyphs)
{
    QSet<quint32> referencedGlyphs;
    QSet<quint32> newGlyphs;
    int count = glyphs.count();
    for (int i = 0; i < count; ++i) {
        quint32 glyphIndex = glyphs.at(i);
        if ((int) glyphIndex >= glyphCount()) {
            qWarning("Warning: glyph is not available with index %d", glyphIndex);
            continue;
        }

        referencedGlyphs.insert(glyphIndex);


        if (!m_glyphReferences.contains(glyphIndex)) {
            newGlyphs.insert(glyphIndex);
        }
    }

    referenceGlyphs(referencedGlyphs);
    if (!newGlyphs.isEmpty())
        requestGlyphs(newGlyphs);
}

void QSGOpenVGFontGlyphCache::release(const QVector<quint32> &glyphs)
{
    QSet<quint32> unusedGlyphs;
    int count = glyphs.count();
    for (int i = 0; i < count; ++i) {
        quint32 glyphIndex = glyphs.at(i);
        unusedGlyphs.insert(glyphIndex);
    }
    releaseGlyphs(unusedGlyphs);
}

void QSGOpenVGFontGlyphCache::requestGlyphs(const QSet<quint32> &glyphs)
{
    VGfloat origin[2];
    VGfloat escapement[2];
    QRawFont rawFont = m_referenceFont;

    for (auto glyph : glyphs) {
        // Calculate the path for the glyph and cache it.
        QPainterPath path = rawFont.pathForGlyph(glyph);
        VGPath vgPath;
        if (!path.isEmpty()) {
            vgPath = QSGOpenVGHelpers::qPainterPathToVGPath(path);
        } else {
            // Probably a "space" character with no visible outline.
            vgPath = VG_INVALID_HANDLE;
        }
        origin[0] = 0;
        origin[1] = 0;
        escapement[0] = 0;
        escapement[1] = 0;
        vgSetGlyphToPath(m_font, glyph, vgPath, VG_FALSE, origin, escapement);
        vgDestroyPath(vgPath);      // Reduce reference count.
    }

}

void QSGOpenVGFontGlyphCache::referenceGlyphs(const QSet<quint32> &glyphs)
{
    for (auto glyph : glyphs) {
        if (m_glyphReferences.contains(glyph))
            m_glyphReferences[glyph] += 1;
        else
            m_glyphReferences.insert(glyph, 1);
    }
}

void QSGOpenVGFontGlyphCache::releaseGlyphs(const QSet<quint32> &glyphs)
{
    for (auto glyph : glyphs) {
        int references = m_glyphReferences[glyph] -= 1;
        if (references == 0) {
            vgClearGlyph(m_font, glyph);
            m_glyphReferences.remove(glyph);
        }
    }
}

QT_END_NAMESPACE
