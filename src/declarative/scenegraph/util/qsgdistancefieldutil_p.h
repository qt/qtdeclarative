/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGDISTANCEFIELDUTIL_H
#define QSGDISTANCEFIELDUTIL_H

#include <qrawfont.h>
#include <private/qfontengine_p.h>
#include <private/qsgadaptationlayer_p.h>

QT_BEGIN_NAMESPACE

#define QT_DISTANCEFIELD_DEFAULT_BASEFONTSIZE 54
#define QT_DISTANCEFIELD_DEFAULT_TILESIZE 64
#define QT_DISTANCEFIELD_DEFAULT_SCALE 16
#define QT_DISTANCEFIELD_DEFAULT_RADIUS 80
#define QT_DISTANCEFIELD_HIGHGLYPHCOUNT 2000

#define QT_DISTANCEFIELD_BASEFONTSIZE(NarrowOutlineFont) \
    (NarrowOutlineFont ? QT_DISTANCEFIELD_DEFAULT_BASEFONTSIZE * 2 : \
                         QT_DISTANCEFIELD_DEFAULT_BASEFONTSIZE)
#define QT_DISTANCEFIELD_TILESIZE(NarrowOutlineFont) \
    (NarrowOutlineFont ? QT_DISTANCEFIELD_DEFAULT_TILESIZE * 2 : \
                         QT_DISTANCEFIELD_DEFAULT_TILESIZE)
#define QT_DISTANCEFIELD_SCALE(NarrowOutlineFont) \
    (NarrowOutlineFont ? QT_DISTANCEFIELD_DEFAULT_SCALE / 2 : \
                         QT_DISTANCEFIELD_DEFAULT_SCALE)
#define QT_DISTANCEFIELD_RADIUS(NarrowOutlineFont) \
    (NarrowOutlineFont ? QT_DISTANCEFIELD_DEFAULT_RADIUS / 2 : \
                         QT_DISTANCEFIELD_DEFAULT_RADIUS)


typedef float (*ThresholdFunc)(float glyphScale);
typedef float (*AntialiasingSpreadFunc)(float glyphScale);

bool qt_fontHasNarrowOutlines(const QRawFont &f);
QImage qt_renderDistanceFieldGlyph(const QRawFont &font, glyph_t glyph, bool doubleResolution);


class QOpenGLShaderProgram;
class QSGDistanceFieldGlyphCache;
class QSGContext;

class Q_DECLARATIVE_EXPORT QSGDistanceFieldGlyphCacheManager
{
public:
    QSGDistanceFieldGlyphCacheManager(QSGContext *c);
    ~QSGDistanceFieldGlyphCacheManager();

    QSGDistanceFieldGlyphCache *cache(const QRawFont &font);

    QSGGlyphNode::AntialiasingMode defaultAntialiasingMode() const { return m_defaultAntialiasingMode; }
    void setDefaultAntialiasingMode(QSGGlyphNode::AntialiasingMode mode) { m_defaultAntialiasingMode = mode; }

    ThresholdFunc thresholdFunc() const { return m_threshold_func; }
    void setThresholdFunc(ThresholdFunc func) { m_threshold_func = func; }

    AntialiasingSpreadFunc antialiasingSpreadFunc() const { return m_antialiasingSpread_func; }
    void setAntialiasingSpreadFunc(AntialiasingSpreadFunc func) { m_antialiasingSpread_func = func; }

private:
    QHash<QFontEngine *, QSGDistanceFieldGlyphCache *> m_caches;

    QSGContext *sgCtx;

    QSGGlyphNode::AntialiasingMode m_defaultAntialiasingMode;
    ThresholdFunc m_threshold_func;
    AntialiasingSpreadFunc m_antialiasingSpread_func;
};

QT_END_NAMESPACE

#endif // QSGDISTANCEFIELDUTIL_H
