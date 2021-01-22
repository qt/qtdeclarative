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

#ifndef QSGRHITEXTUREGLYPHCACHE_P_H
#define QSGRHITEXTUREGLYPHCACHE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtextureglyphcache_p.h>
#include <QtGui/private/qrhi_p.h>

QT_BEGIN_NAMESPACE

class QSGRhiTextureGlyphCache : public QImageTextureGlyphCache
{
public:
    QSGRhiTextureGlyphCache(QRhi *rhi, QFontEngine::GlyphFormat format, const QTransform &matrix,
                            const QColor &color = QColor());
    ~QSGRhiTextureGlyphCache();

    void createTextureData(int width, int height) override;
    void resizeTextureData(int width, int height) override;
    void beginFillTexture() override;
    void fillTexture(const Coord &c, glyph_t glyph, QFixed subPixelPosition) override;
    void endFillTexture() override;
    int glyphPadding() const override;
    int maxTextureWidth() const override;
    int maxTextureHeight() const override;

    QRhiTexture *texture() const { return m_texture; }
    void commitResourceUpdates(QRhiResourceUpdateBatch *mergeInto);

    // Clamp the default -1 width and height to 0 for compatibility with
    // QOpenGLTextureGlyphCache.
    int width() const { return qMax(0, m_size.width()); }
    int height() const { return qMax(0, m_size.height()); }

    bool eightBitFormatIsAlphaSwizzled() const;

private:
    void prepareGlyphImage(QImage *img);
    QRhiTexture *createEmptyTexture(QRhiTexture::Format format);

    QRhi *m_rhi;
    bool m_resizeWithTextureCopy;
    QRhiResourceUpdateBatch *m_resourceUpdates = nullptr;
    QRhiTexture *m_texture = nullptr;
    QSize m_size;
    bool m_bgra = false;
    QVarLengthArray<QRhiTextureUploadEntry, 16> m_uploads;
    QSet<QRhiTexture *> m_pendingDispose;
};

QT_END_NAMESPACE

#endif // QSGRHITEXTUREGLYPHCACHE_P_H
