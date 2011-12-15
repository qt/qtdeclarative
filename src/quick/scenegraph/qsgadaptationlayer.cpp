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

#include "qsgadaptationlayer_p.h"

#include <qmath.h>
#include <QtQuick/private/qsgdistancefieldutil_p.h>
#include <QtQuick/private/qsgdistancefieldglyphnode_p.h>
#include <private/qrawfont_p.h>
#include <QtGui/qguiapplication.h>
#include <qdir.h>

QT_BEGIN_NAMESPACE


QHash<QString, QOpenGLMultiGroupSharedResource> QSGDistanceFieldGlyphCache::m_caches_data;

QSGDistanceFieldGlyphCache::QSGDistanceFieldGlyphCache(QSGDistanceFieldGlyphCacheManager *man, QOpenGLContext *c, const QRawFont &font)
    : ctx(c)
    , m_manager(man)
{
    Q_ASSERT(font.isValid());
    m_font = font;

    m_cacheData = cacheData();

    QRawFontPrivate *fontD = QRawFontPrivate::get(m_font);
    m_glyphCount = fontD->fontEngine->glyphCount();

    m_cacheData->doubleGlyphResolution = qt_fontHasNarrowOutlines(font) && m_glyphCount < QT_DISTANCEFIELD_HIGHGLYPHCOUNT;

    m_referenceFont = m_font;
    m_referenceFont.setPixelSize(QT_DISTANCEFIELD_BASEFONTSIZE(m_cacheData->doubleGlyphResolution));
    Q_ASSERT(m_referenceFont.isValid());
}

QSGDistanceFieldGlyphCache::~QSGDistanceFieldGlyphCache()
{
}

QSGDistanceFieldGlyphCache::GlyphCacheData *QSGDistanceFieldGlyphCache::cacheData()
{
    QString key = QString::fromLatin1("%1_%2_%3_%4")
            .arg(m_font.familyName())
            .arg(m_font.styleName())
            .arg(m_font.weight())
            .arg(m_font.style());
    return m_caches_data[key].value<QSGDistanceFieldGlyphCache::GlyphCacheData>(ctx);
}

qreal QSGDistanceFieldGlyphCache::fontScale() const
{
    return qreal(m_font.pixelSize()) / QT_DISTANCEFIELD_BASEFONTSIZE(m_cacheData->doubleGlyphResolution);
}

int QSGDistanceFieldGlyphCache::distanceFieldRadius() const
{
    return QT_DISTANCEFIELD_DEFAULT_RADIUS / QT_DISTANCEFIELD_SCALE(m_cacheData->doubleGlyphResolution);
}

QSGDistanceFieldGlyphCache::Metrics QSGDistanceFieldGlyphCache::glyphMetrics(glyph_t glyph)
{
    QHash<glyph_t, Metrics>::iterator metric = m_metrics.find(glyph);
    if (metric == m_metrics.end()) {
        QPainterPath path = m_font.pathForGlyph(glyph);
        QRectF br = path.boundingRect();

        Metrics m;
        m.width = br.width();
        m.height = br.height();
        m.baselineX = br.x();
        m.baselineY = -br.y();

        metric = m_metrics.insert(glyph, m);
    }

    return metric.value();
}

QSGDistanceFieldGlyphCache::TexCoord QSGDistanceFieldGlyphCache::glyphTexCoord(glyph_t glyph) const
{
    return m_cacheData->texCoords.value(glyph);
}

static QSGDistanceFieldGlyphCache::Texture g_emptyTexture;

const QSGDistanceFieldGlyphCache::Texture *QSGDistanceFieldGlyphCache::glyphTexture(glyph_t glyph) const
{
    QHash<glyph_t, Texture*>::const_iterator it = m_cacheData->glyphTextures.find(glyph);
    if (it == m_cacheData->glyphTextures.constEnd())
        return &g_emptyTexture;
    return it.value();
}

void QSGDistanceFieldGlyphCache::populate(const QVector<glyph_t> &glyphs)
{
    QSet<glyph_t> referencedGlyphs;
    QSet<glyph_t> newGlyphs;
    int count = glyphs.count();
    for (int i = 0; i < count; ++i) {
        glyph_t glyphIndex = glyphs.at(i);
        if ((int) glyphIndex >= glyphCount()) {
            qWarning("Warning: distance-field glyph is not available with index %d", glyphIndex);
            continue;
        }

        ++m_cacheData->glyphRefCount[glyphIndex];
        referencedGlyphs.insert(glyphIndex);

        if (m_cacheData->texCoords.contains(glyphIndex) || newGlyphs.contains(glyphIndex))
            continue;

        QPainterPath path = m_referenceFont.pathForGlyph(glyphIndex);
        m_cacheData->glyphPaths.insert(glyphIndex, path);
        if (path.isEmpty()) {
            TexCoord c;
            c.width = 0;
            c.height = 0;
            m_cacheData->texCoords.insert(glyphIndex, c);
            continue;
        }

        newGlyphs.insert(glyphIndex);
    }

    if (newGlyphs.isEmpty())
        return;

    referenceGlyphs(referencedGlyphs);
    requestGlyphs(newGlyphs);
}

void QSGDistanceFieldGlyphCache::release(const QVector<glyph_t> &glyphs)
{
    QSet<glyph_t> unusedGlyphs;
    int count = glyphs.count();
    for (int i = 0; i < count; ++i) {
        glyph_t glyphIndex = glyphs.at(i);
        if (--m_cacheData->glyphRefCount[glyphIndex] == 0 && !glyphTexCoord(glyphIndex).isNull())
            unusedGlyphs.insert(glyphIndex);
    }
    releaseGlyphs(unusedGlyphs);
}

void QSGDistanceFieldGlyphCache::update()
{
    if (m_cacheData->pendingGlyphs.isEmpty())
        return;

    QHash<glyph_t, QImage> distanceFields;

    // ### Remove before final release
    static bool cacheDistanceFields = QGuiApplication::arguments().contains(QLatin1String("--cache-distance-fields"));

    QString tmpPath = QString::fromLatin1("%1/.qt/").arg(QDir::tempPath());
    QString keyBase = QString::fromLatin1("%1%2%3_%4_%5_%6.fontblob")
            .arg(tmpPath)
            .arg(m_font.familyName())
            .arg(m_font.styleName())
            .arg(m_font.weight())
            .arg(m_font.style());

    if (cacheDistanceFields && !QFile::exists(tmpPath))
        QDir(tmpPath).mkpath(tmpPath);

    for (int i = 0; i < m_cacheData->pendingGlyphs.size(); ++i) {
        glyph_t glyphIndex = m_cacheData->pendingGlyphs.at(i);

        if (cacheDistanceFields) {
            QString key = keyBase.arg(glyphIndex);
            QFile file(key);
            if (file.open(QFile::ReadOnly)) {
                int fileSize = file.size();
                int dim = sqrt(float(fileSize));
                QByteArray blob = file.readAll();
                QImage df(dim, dim, QImage::Format_Indexed8);
                memcpy(df.bits(), blob.constData(), fileSize);
                distanceFields.insert(glyphIndex, df);
                continue;
            }
        }

        QImage distanceField = qt_renderDistanceFieldGlyph(m_font, glyphIndex, m_cacheData->doubleGlyphResolution);
        distanceFields.insert(glyphIndex, distanceField);

        if (cacheDistanceFields) {
            QString key = keyBase.arg(glyphIndex);
            QFile file(key);
            file.open(QFile::WriteOnly);
            file.write((const char *) distanceField.constBits(), distanceField.width() * distanceField.height());
        }
    }

    m_cacheData->pendingGlyphs.reset();

    storeGlyphs(distanceFields);
}

void QSGDistanceFieldGlyphCache::setGlyphsPosition(const QList<GlyphPosition> &glyphs)
{
    QVector<quint32> invalidatedGlyphs;

    int count = glyphs.count();
    for (int i = 0; i < count; ++i) {
        GlyphPosition glyph = glyphs.at(i);

        QPainterPath path = m_cacheData->glyphPaths.value(glyph.glyph);
        QRectF br = path.boundingRect();
        TexCoord c;
        c.xMargin = QT_DISTANCEFIELD_RADIUS(m_cacheData->doubleGlyphResolution) / qreal(QT_DISTANCEFIELD_SCALE(m_cacheData->doubleGlyphResolution));
        c.yMargin = QT_DISTANCEFIELD_RADIUS(m_cacheData->doubleGlyphResolution) / qreal(QT_DISTANCEFIELD_SCALE(m_cacheData->doubleGlyphResolution));
        c.x = glyph.position.x();
        c.y = glyph.position.y();
        c.width = br.width();
        c.height = br.height();

        if (m_cacheData->texCoords.contains(glyph.glyph))
            invalidatedGlyphs.append(glyph.glyph);

        m_cacheData->texCoords.insert(glyph.glyph, c);
    }

    if (!invalidatedGlyphs.isEmpty()) {
        QLinkedList<QSGDistanceFieldGlyphNode *>::iterator it = m_cacheData->m_registeredNodes.begin();
        while (it != m_cacheData->m_registeredNodes.end()) {
            (*it)->invalidateGlyphs(invalidatedGlyphs);
            ++it;
        }
    }
}

void QSGDistanceFieldGlyphCache::setGlyphsTexture(const QVector<glyph_t> &glyphs, const Texture &tex)
{
    int i = m_cacheData->textures.indexOf(tex);
    if (i == -1) {
        m_cacheData->textures.append(tex);
        i = m_cacheData->textures.size() - 1;
    } else {
        m_cacheData->textures[i].size = tex.size;
    }
    Texture *texture = &(m_cacheData->textures[i]);

    QVector<quint32> invalidatedGlyphs;

    int count = glyphs.count();
    for (int j = 0; j < count; ++j) {
        glyph_t glyphIndex = glyphs.at(j);
        if (m_cacheData->glyphTextures.contains(glyphIndex))
            invalidatedGlyphs.append(glyphIndex);
        m_cacheData->glyphTextures.insert(glyphIndex, texture);
    }

    if (!invalidatedGlyphs.isEmpty()) {
        QLinkedList<QSGDistanceFieldGlyphNode *>::iterator it = m_cacheData->m_registeredNodes.begin();
        while (it != m_cacheData->m_registeredNodes.end()) {
            (*it)->invalidateGlyphs(invalidatedGlyphs);
            ++it;
        }
    }
}

void QSGDistanceFieldGlyphCache::markGlyphsToRender(const QVector<glyph_t> &glyphs)
{
    int count = glyphs.count();
    for (int i = 0; i < count; ++i)
        m_cacheData->pendingGlyphs.add(glyphs.at(i));
}

void QSGDistanceFieldGlyphCache::removeGlyph(glyph_t glyph)
{
    m_cacheData->texCoords.remove(glyph);
    m_cacheData->glyphTextures.remove(glyph);
}

void QSGDistanceFieldGlyphCache::updateTexture(GLuint oldTex, GLuint newTex, const QSize &newTexSize)
{
    int count = m_cacheData->textures.count();
    for (int i = 0; i < count; ++i) {
        Texture &tex = m_cacheData->textures[i];
        if (tex.textureId == oldTex) {
            tex.textureId = newTex;
            tex.size = newTexSize;
            return;
        }
    }
}

bool QSGDistanceFieldGlyphCache::containsGlyph(glyph_t glyph) const
{
    return m_cacheData->texCoords.contains(glyph);
}

void QSGDistanceFieldGlyphCache::registerGlyphNode(QSGDistanceFieldGlyphNode *node)
{
    m_cacheData->m_registeredNodes.append(node);
}

void QSGDistanceFieldGlyphCache::unregisterGlyphNode(QSGDistanceFieldGlyphNode *node)
{
    m_cacheData->m_registeredNodes.removeOne(node);
}


QT_END_NAMESPACE
