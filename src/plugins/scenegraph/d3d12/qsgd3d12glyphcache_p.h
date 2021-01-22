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

#ifndef QSGD3D12GLYPHCACHE_P_H
#define QSGD3D12GLYPHCACHE_P_H

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

QT_BEGIN_NAMESPACE

class QSGD3D12Engine;

class QSGD3D12GlyphCache : public QTextureGlyphCache
{
public:
    QSGD3D12GlyphCache(QSGD3D12Engine *engine, QFontEngine::GlyphFormat format, const QTransform &matrix);
    ~QSGD3D12GlyphCache();

    void createTextureData(int width, int height) override;
    void resizeTextureData(int width, int height) override;
    void beginFillTexture() override;
    void fillTexture(const Coord &c, glyph_t glyph, QFixed subPixelPosition) override;
    void endFillTexture() override;
    int glyphPadding() const override;
    int maxTextureWidth() const override;
    int maxTextureHeight() const override;

    void useTexture();
    QSize currentSize() const;

private:
    QSGD3D12Engine *m_engine;
    uint m_id = 0;
    QVector<QImage> m_glyphImages;
    QVector<QPoint> m_glyphPos;
    QSize m_size;
};

QT_END_NAMESPACE

#endif // QSGD3D12GLYPHCACHE_P_H
