/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGDEFAULTDISTANCEFIELDGLYPHCACHE_H
#define QSGDEFAULTDISTANCEFIELDGLYPHCACHE_H

#include "qsgadaptationlayer_p.h"
#include <QtGui/qopenglfunctions.h>
#include <qopenglshaderprogram.h>
#include <QtGui/private/qopenglengineshadersource_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QSGDefaultDistanceFieldGlyphCache : public QSGDistanceFieldGlyphCache
{
public:
    QSGDefaultDistanceFieldGlyphCache(QSGDistanceFieldGlyphCacheManager *man, QOpenGLContext *c, const QRawFont &font);
    virtual ~QSGDefaultDistanceFieldGlyphCache();

    void requestGlyphs(const QSet<glyph_t> &glyphs);
    void storeGlyphs(const QHash<glyph_t, QImage> &glyphs);
    void referenceGlyphs(const QSet<glyph_t> &glyphs);
    void releaseGlyphs(const QSet<glyph_t> &glyphs);

    bool cacheIsFull() const {
        return m_textures.count() == m_maxTextureCount
                && textureIsFull(m_currentTexture);
    }
    bool useWorkaroundBrokenFBOReadback() const;
    int maxTextureSize() const;

    void setMaxTextureCount(int max) { m_maxTextureCount = max; }
    int maxTextureCount() const { return m_maxTextureCount; }

private:
    struct TextureInfo {
        GLuint texture;
        QSize size;
        int currX;
        int currY;
        QImage image;

        TextureInfo() : texture(0), currX(0), currY(0)
        { }
    };

    void createTexture(TextureInfo * texInfo, int width, int height);
    void resizeTexture(TextureInfo * texInfo, int width, int height);
    bool textureIsFull (const TextureInfo *tex) const { return tex->currY >= maxTextureSize(); }

    TextureInfo *createTextureInfo()
    {
        m_textures.append(TextureInfo());
        return &m_textures.last();
    }

    void createBlitProgram()
    {
        m_blitProgram = new QOpenGLShaderProgram;
        {
            QString source;
            source.append(QLatin1String(qopenglslMainWithTexCoordsVertexShader));
            source.append(QLatin1String(qopenglslUntransformedPositionVertexShader));

            QOpenGLShader *vertexShader = new QOpenGLShader(QOpenGLShader::Vertex, m_blitProgram);
            vertexShader->compileSourceCode(source);

            m_blitProgram->addShader(vertexShader);
        }
        {
            QString source;
            source.append(QLatin1String(qopenglslMainFragmentShader));
            source.append(QLatin1String(qopenglslImageSrcFragmentShader));

            QOpenGLShader *fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, m_blitProgram);
            fragmentShader->compileSourceCode(source);

            m_blitProgram->addShader(fragmentShader);
        }
        m_blitProgram->bindAttributeLocation("vertexCoordsArray", QT_VERTEX_COORDS_ATTR);
        m_blitProgram->bindAttributeLocation("textureCoordArray", QT_TEXTURE_COORDS_ATTR);
        m_blitProgram->link();
    }

    mutable int m_maxTextureSize;
    int m_maxTextureCount;

    TextureInfo *m_currentTexture;
    QList<TextureInfo> m_textures;
    QHash<glyph_t, TextureInfo *> m_glyphsTexture;
    GLuint m_fbo;
    QSet<glyph_t> m_unusedGlyphs;

    QOpenGLShaderProgram *m_blitProgram;
    GLfloat m_blitVertexCoordinateArray[8];
    GLfloat m_blitTextureCoordinateArray[8];
};

QT_END_NAMESPACE

#endif // QSGDEFAULTDISTANCEFIELDGLYPHCACHE_H
