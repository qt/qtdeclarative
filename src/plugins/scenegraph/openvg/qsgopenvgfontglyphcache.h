// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGFONTGLYPHCACHE_H
#define QSGOPENVGFONTGLYPHCACHE_H

#include <QtGui/QGlyphRun>
#include <QtCore/QSet>
#include <VG/openvg.h>

QT_BEGIN_NAMESPACE

class QSGOpenVGFontGlyphCache;

class QSGOpenVGFontGlyphCacheManager
{
public:
    QSGOpenVGFontGlyphCacheManager();
    ~QSGOpenVGFontGlyphCacheManager();

    QSGOpenVGFontGlyphCache *cache(const QRawFont &font);
    void insertCache(const QRawFont &font, QSGOpenVGFontGlyphCache *cache);

private:
    QHash<QRawFont, QSGOpenVGFontGlyphCache *> m_caches;
};

class QSGOpenVGFontGlyphCache
{
public:
    QSGOpenVGFontGlyphCache(QSGOpenVGFontGlyphCacheManager *manager, const QRawFont &font);
    ~QSGOpenVGFontGlyphCache();

    const QSGOpenVGFontGlyphCacheManager *manager() const { return m_manager; }
    const QRawFont &referenceFont() const { return m_referenceFont; }
    int glyphCount() const { return m_glyphCount; }

    void populate(const QVector<quint32> &glyphs);
    void release(const QVector<quint32> &glyphs);

    VGFont font() { return m_font; }

private:
    void requestGlyphs(const QSet<quint32> &glyphs);
    void referenceGlyphs(const QSet<quint32> &glyphs);
    void releaseGlyphs(const QSet<quint32> &glyphs);

    QSGOpenVGFontGlyphCacheManager *m_manager;
    QRawFont m_referenceFont;
    int m_glyphCount;

    VGFont m_font;
    QHash<quint32, int> m_glyphReferences;
};


QT_END_NAMESPACE

#endif // QSGOPENVGFONTGLYPHCACHE_H
