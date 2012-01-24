/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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

class Q_QUICK_EXPORT QSGDefaultDistanceFieldGlyphCache : public QSGDistanceFieldGlyphCache
{
public:
    QSGDefaultDistanceFieldGlyphCache(QSGDistanceFieldGlyphCacheManager *man, QOpenGLContext *c, const QRawFont &font);

    void requestGlyphs(const QSet<glyph_t> &glyphs);
    void storeGlyphs(const QHash<glyph_t, QImage> &glyphs);
    void referenceGlyphs(const QSet<glyph_t> &glyphs);
    void releaseGlyphs(const QSet<glyph_t> &glyphs);

    bool cacheIsFull() const { return m_textureData->currY >= maxTextureSize(); }
    bool useWorkaroundBrokenFBOReadback() const;
    int maxTextureSize() const;

private:
    void createTexture(int width, int height);
    void resizeTexture(int width, int height);

    mutable int m_maxTextureSize;

    struct DistanceFieldTextureData : public QOpenGLSharedResource {
        GLuint texture;
        GLuint fbo;
        QSize size;
        QSet<glyph_t> unusedGlyphs;
        int currX;
        int currY;
        QImage image;

        QOpenGLShaderProgram *blitProgram;
        GLfloat blitVertexCoordinateArray[8];
        GLfloat blitTextureCoordinateArray[8];

        DistanceFieldTextureData(QOpenGLContext *ctx)
            : QOpenGLSharedResource(ctx->shareGroup())
            , texture(0)
            , fbo(0)
            , currX(0)
            , currY(0)
            , blitProgram(0)
        {
            blitVertexCoordinateArray[0] = -1.0f;
            blitVertexCoordinateArray[1] = -1.0f;
            blitVertexCoordinateArray[2] =  1.0f;
            blitVertexCoordinateArray[3] = -1.0f;
            blitVertexCoordinateArray[4] =  1.0f;
            blitVertexCoordinateArray[5] =  1.0f;
            blitVertexCoordinateArray[6] = -1.0f;
            blitVertexCoordinateArray[7] =  1.0f;

            blitTextureCoordinateArray[0] = 0.0f;
            blitTextureCoordinateArray[1] = 0.0f;
            blitTextureCoordinateArray[2] = 1.0f;
            blitTextureCoordinateArray[3] = 0.0f;
            blitTextureCoordinateArray[4] = 1.0f;
            blitTextureCoordinateArray[5] = 1.0f;
            blitTextureCoordinateArray[6] = 0.0f;
            blitTextureCoordinateArray[7] = 1.0f;
        }

        void invalidateResource()
        {
            texture = 0;
            fbo = 0;
            size = QSize();
            delete blitProgram;
            blitProgram = 0;
        }

        void freeResource(QOpenGLContext *ctx)
        {
            glDeleteTextures(1, &texture);
            ctx->functions()->glDeleteFramebuffers(1, &fbo);
            delete blitProgram;
            blitProgram = 0;
        }

        void createBlitProgram()
        {
            blitProgram = new QOpenGLShaderProgram;
            {
                QString source;
                source.append(QLatin1String(qopenglslMainWithTexCoordsVertexShader));
                source.append(QLatin1String(qopenglslUntransformedPositionVertexShader));

                QOpenGLShader *vertexShader = new QOpenGLShader(QOpenGLShader::Vertex, blitProgram);
                vertexShader->compileSourceCode(source);

                blitProgram->addShader(vertexShader);
            }
            {
                QString source;
                source.append(QLatin1String(qopenglslMainFragmentShader));
                source.append(QLatin1String(qopenglslImageSrcFragmentShader));

                QOpenGLShader *fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, blitProgram);
                fragmentShader->compileSourceCode(source);

                blitProgram->addShader(fragmentShader);
            }
            blitProgram->bindAttributeLocation("vertexCoordsArray", QT_VERTEX_COORDS_ATTR);
            blitProgram->bindAttributeLocation("textureCoordArray", QT_TEXTURE_COORDS_ATTR);
            blitProgram->link();
        }
    };

    DistanceFieldTextureData *textureData(QOpenGLContext *c);
    DistanceFieldTextureData *m_textureData;
    static QHash<QString, QOpenGLMultiGroupSharedResource> m_textures_data;
};

QT_END_NAMESPACE

#endif // QSGDEFAULTDISTANCEFIELDGLYPHCACHE_H
