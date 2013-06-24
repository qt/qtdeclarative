/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickanimatedsprite_p.h"
#include "qquicksprite_p.h"
#include "qquickspriteengine_p.h"
#include <QtQuick/private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>
#include <private/qqmlglobal_p.h>
#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgtexturematerial.h>
#include <QtQuick/qsgtexture.h>
#include <QtQuick/qquickwindow.h>
#include <QtQml/qqmlinfo.h>
#include <QFile>
#include <cmath>
#include <qmath.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

static const char vertexShaderCode[] =
    "attribute highp vec2 vPos;\n"
    "attribute highp vec2 vTex;\n"
    "uniform highp vec3 animData;// w,h(premultiplied of anim), interpolation progress\n"
    "uniform highp vec4 animPos;//x,y, x,y (two frames for interpolation)\n"
    "\n"
    "uniform highp mat4 qt_Matrix;\n"
    "\n"
    "varying highp vec4 fTexS;\n"
    "varying lowp float progress;\n"
    "\n"
    "\n"
    "void main() {\n"
    "    progress = animData.z;\n"
    "    //Calculate frame location in texture\n"
    "    fTexS.xy = animPos.xy + vTex.xy * animData.xy;\n"
    "    //Next frame is also passed, for interpolation\n"
    "    fTexS.zw = animPos.zw + vTex.xy * animData.xy;\n"
    "\n"
    "    gl_Position = qt_Matrix * vec4(vPos.x, vPos.y, 0, 1);\n"
    "}\n";

static const char fragmentShaderCode[] =
    "uniform sampler2D texture;\n"
    "uniform lowp float qt_Opacity;\n"
    "\n"
    "varying highp vec4 fTexS;\n"
    "varying lowp float progress;\n"
    "\n"
    "void main() {\n"
    "    gl_FragColor = mix(texture2D(texture, fTexS.xy), texture2D(texture, fTexS.zw), progress) * qt_Opacity;\n"
    "}\n";

class QQuickAnimatedSpriteMaterial : public QSGMaterial
{
public:
    QQuickAnimatedSpriteMaterial();
    ~QQuickAnimatedSpriteMaterial();
    virtual QSGMaterialType *type() const { static QSGMaterialType type; return &type; }
    virtual QSGMaterialShader *createShader() const;
    virtual int compare(const QSGMaterial *other) const
    {
        return this - static_cast<const QQuickAnimatedSpriteMaterial *>(other);
    }

    QSGTexture *texture;

    float animT;
    float animX1;
    float animY1;
    float animX2;
    float animY2;
    float animW;
    float animH;
};

QQuickAnimatedSpriteMaterial::QQuickAnimatedSpriteMaterial()
    : texture(0)
    , animT(0.0f)
    , animX1(0.0f)
    , animY1(0.0f)
    , animX2(0.0f)
    , animY2(0.0f)
    , animW(1.0f)
    , animH(1.0f)
{
    setFlag(Blending, true);
}

QQuickAnimatedSpriteMaterial::~QQuickAnimatedSpriteMaterial()
{
    delete texture;
}

class AnimatedSpriteMaterialData : public QSGMaterialShader
{
public:
    AnimatedSpriteMaterialData(const char * /* vertexFile */ = 0, const char * /* fragmentFile */ = 0)
    {
    }

    void deactivate() {
        QSGMaterialShader::deactivate();

        for (int i=0; i<8; ++i) {
            program()->setAttributeArray(i, GL_FLOAT, chunkOfBytes, 1, 0);
        }
    }

    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *)
    {
        QQuickAnimatedSpriteMaterial *m = static_cast<QQuickAnimatedSpriteMaterial *>(newEffect);
        m->texture->bind();

        program()->setUniformValue(m_opacity_id, state.opacity());
        program()->setUniformValue(m_animData_id, m->animW, m->animH, m->animT);
        program()->setUniformValue(m_animPos_id, m->animX1, m->animY1, m->animX2, m->animY2);

        if (state.isMatrixDirty())
            program()->setUniformValue(m_matrix_id, state.combinedMatrix());
    }

    virtual void initialize() {
        m_matrix_id = program()->uniformLocation("qt_Matrix");
        m_opacity_id = program()->uniformLocation("qt_Opacity");
        m_animData_id = program()->uniformLocation("animData");
        m_animPos_id = program()->uniformLocation("animPos");
    }

    virtual const char *vertexShader() const { return vertexShaderCode; }
    virtual const char *fragmentShader() const { return fragmentShaderCode; }

    virtual char const *const *attributeNames() const {
        static const char *attr[] = {
           "vPos",
           "vTex",
            0
        };
        return attr;
    }

    int m_matrix_id;
    int m_opacity_id;
    int m_animData_id;
    int m_animPos_id;

    static float chunkOfBytes[1024];
};

float AnimatedSpriteMaterialData::chunkOfBytes[1024];

QSGMaterialShader *QQuickAnimatedSpriteMaterial::createShader() const
{
    return new AnimatedSpriteMaterialData;
}

struct AnimatedSpriteVertex {
    float x;
    float y;
    float tx;
    float ty;
};

struct AnimatedSpriteVertices {
    AnimatedSpriteVertex v1;
    AnimatedSpriteVertex v2;
    AnimatedSpriteVertex v3;
    AnimatedSpriteVertex v4;
};

/*!
    \qmltype AnimatedSprite
    \instantiates QQuickAnimatedSprite
    \inqmlmodule QtQuick 2
    \inherits Item
    \ingroup qtquick-visual
    \brief Draws a sprite animation

    AnimatedSprite provides rendering and control over animations which are provided
    as multiple frames in the same image file. You can play it at a fixed speed, at the
    frame rate of your display, or manually advance and control the progress.

    For details of how a sprite animation is defined see the \l{Sprite Animation} overview.
    Note that the AnimatedSprite type does not use Sprite types to define multiple animations,
    but instead encapsulates a single animation itself.
*/

/*!
    \qmlproperty bool QtQuick2::AnimatedSprite::running

    Whether the sprite is animating or not.

    Default is true
*/

/*!
    \qmlproperty bool QtQuick2::AnimatedSprite::interpolate

    If true, interpolation will occur between sprite frames to make the
    animation appear smoother.

    Default is true.
*/

/*!
    \qmlproperty qreal QtQuick2::AnimatedSprite::frameRate

    Frames per second to show in the animation. Values equal to or below 0 are invalid.

    If frameRate is valid  then it will be used to calculate the duration of the frames.
    If not, and frameDuration is valid , then frameDuration will be used.

    Changing this parameter will restart the animation.
*/

/*!
    \qmlproperty int QtQuick2::AnimatedSprite::frameDuration

    Duration of each frame of the animation. Values equal to or below 0 are invalid.

    If frameRate is valid then it will be used to calculate the duration of the frames.
    If not, and frameDuration is valid, then frameDuration will be used.

    Changing this parameter will restart the animation.
*/

/*!
    \qmlproperty int QtQuick2::AnimatedSprite::frameCount

    Number of frames in this AnimatedSprite.
*/
/*!
    \qmlproperty int QtQuick2::AnimatedSprite::frameHeight

    Height of a single frame in this AnimatedSprite.

    May be omitted if it is the only sprite in the file.
*/
/*!
    \qmlproperty int QtQuick2::AnimatedSprite::frameWidth

    Width of a single frame in this AnimatedSprite.

    May be omitted if it is the only sprite in the file.
*/
/*!
    \qmlproperty int QtQuick2::AnimatedSprite::frameX

    The X coordinate in the image file of the first frame of the AnimatedSprite.

    May be omitted if the first frame starts in the upper left corner of the file.
*/
/*!
    \qmlproperty int QtQuick2::AnimatedSprite::frameY

    The Y coordinate in the image file of the first frame of the AnimatedSprite.

    May be omitted if the first frame starts in the upper left corner of the file.
*/
/*!
    \qmlproperty url QtQuick2::AnimatedSprite::source

    The image source for the animation.

    If frameHeight and frameWidth are not specified, it is assumed to be a single long row of square frames.
    Otherwise, it can be multiple contiguous rows or rectangluar frames, when one row runs out the next will be used.

    If frameX and frameY are specified, the row of frames will be taken with that x/y coordinate as the upper left corner.
*/

/*!
    \qmlproperty bool QtQuick2::AnimatedSprite::reverse

    If true, then the animation will be played in reverse.

    Default is false.
*/

/*!
    \qmlproperty bool QtQuick2::AnimatedSprite::frameSync

    If true, then the animation will have no duration. Instead, the animation will advance
    one frame each time a frame is rendered to the screen. This synchronizes it with the painting
    rate as opposed to elapsed time.

    If frameSync is set to true, it overrides both frameRate and frameDuration.

    Default is false.

    Changing this parameter will restart the animation.
*/

/*!
    \qmlproperty int QtQuick2::AnimatedSprite::loops

    After playing the animation this many times, the animation will automatically stop. Negative values are invalid.

    If this is set to AnimatedSprite.Infinite the animation will not stop playing on its own.

    Default is AnimatedSprite.Infinite
*/

/*!
    \qmlproperty bool QtQuick2::AnimatedSprite::paused

    When paused, the current frame can be advanced manually.

    Default is false.
*/

/*!
    \qmlproperty int QtQuick2::AnimatedSprite::currentFrame

    When paused, the current frame can be advanced manually by setting this property or calling advance().

*/

/*!
    \qmlmethod int QtQuick2::AnimatedSprite::restart()

    Stops, then starts the sprite animation.
*/

//TODO: Implicitly size element to size of sprite
QQuickAnimatedSprite::QQuickAnimatedSprite(QQuickItem *parent) :
    QQuickItem(parent)
    , m_node(0)
    , m_material(0)
    , m_sprite(new QQuickSprite(this))
    , m_spriteEngine(0)
    , m_curFrame(0)
    , m_pleaseReset(false)
    , m_running(true)
    , m_paused(false)
    , m_interpolate(true)
    , m_loops(-1)
    , m_curLoop(0)
    , m_pauseOffset(0)
{
    setFlag(ItemHasContents);
    connect(this, SIGNAL(widthChanged()),
            this, SLOT(sizeVertices()));
    connect(this, SIGNAL(heightChanged()),
            this, SLOT(sizeVertices()));
}

bool QQuickAnimatedSprite::isCurrentFrameChangedConnected()
{
    IS_SIGNAL_CONNECTED(this, QQuickAnimatedSprite, currentFrameChanged, (int));
}

void QQuickAnimatedSprite::reloadImage()
{
    if (!isComponentComplete())
        return;
    createEngine();//### It's not as inefficient as it sounds, but it still sucks having to recreate the engine
}

void QQuickAnimatedSprite::componentComplete()
{
    createEngine();
    QQuickItem::componentComplete();
    if (m_running)
        start();
}

void QQuickAnimatedSprite::start()
{
    m_running = true;
    if (!isComponentComplete())
        return;
    m_curLoop = 0;
    m_timestamp.start();
    if (m_spriteEngine) {
        m_spriteEngine->stop(0);
        m_spriteEngine->updateSprites(0);
        m_spriteEngine->start(0);
    }
    emit currentFrameChanged(0);
    emit runningChanged(true);
    update();
}

void QQuickAnimatedSprite::stop()
{
    m_running = false;
    if (!isComponentComplete())
        return;
    m_pauseOffset = 0;
    emit runningChanged(false);
}

/*!
    \qmlmethod int QtQuick2::AnimatedSprite::advance()

    Advances the sprite animation by one frame.
*/
void QQuickAnimatedSprite::advance(int frames)
{
    if (!frames)
        return;
    //TODO-C: May not work when running - only when paused
    m_curFrame += frames;
    while (m_curFrame < 0)
        m_curFrame += m_sprite->frames();
    m_curFrame = m_curFrame % m_sprite->frames();
    emit currentFrameChanged(m_curFrame);
}

/*!
    \qmlmethod int QtQuick2::AnimatedSprite::pause()

    Pauses the sprite animation. This does nothing if
    \l paused is true.

    \sa resume()
*/
void QQuickAnimatedSprite::pause()
{
    if (m_paused)
        return;
    m_pauseOffset = m_timestamp.elapsed();
    m_paused = true;
    emit pausedChanged(true);
}

/*!
    \qmlmethod int QtQuick2::AnimatedSprite::resume()

    Resumes the sprite animation if \l paused is true;
    otherwise, this does nothing.

    \sa pause()
*/
void QQuickAnimatedSprite::resume()
{
    if (!m_paused)
        return;
    m_pauseOffset = m_pauseOffset - m_timestamp.elapsed();
    m_paused = false;
    emit pausedChanged(false);
}

void QQuickAnimatedSprite::createEngine()
{
    if (m_spriteEngine)
        delete m_spriteEngine;
    QList<QQuickSprite*> spriteList;
    spriteList << m_sprite;
    m_spriteEngine = new QQuickSpriteEngine(QList<QQuickSprite*>(spriteList), this);
    m_spriteEngine->startAssemblingImage();
    reset();
}

static QSGGeometry::Attribute AnimatedSprite_Attributes[] = {
    QSGGeometry::Attribute::create(0, 2, GL_FLOAT, true),   // pos
    QSGGeometry::Attribute::create(1, 2, GL_FLOAT),         // tex
};

static QSGGeometry::AttributeSet AnimatedSprite_AttributeSet =
{
    2, // Attribute Count
    (2+2) * sizeof(float),
    AnimatedSprite_Attributes
};

void QQuickAnimatedSprite::sizeVertices()
{
    if (!m_node)
        return;

    AnimatedSpriteVertices *p = (AnimatedSpriteVertices *) m_node->geometry()->vertexData();
    p->v1.x = 0;
    p->v1.y = 0;

    p->v2.x = width();
    p->v2.y = 0;

    p->v3.x = 0;
    p->v3.y = height();

    p->v4.x = width();
    p->v4.y = height();
}

QSGGeometryNode* QQuickAnimatedSprite::buildNode()
{
    if (!m_spriteEngine) {
        qmlInfo(this) << "No sprite engine...";
        return 0;
    } else if (m_spriteEngine->status() == QQuickPixmap::Null) {
        m_spriteEngine->startAssemblingImage();
        update();//Schedule another update, where we will check again
        return 0;
    } else if (m_spriteEngine->status() == QQuickPixmap::Loading) {
        update();//Schedule another update, where we will check again
        return 0;
    }

    m_material = new QQuickAnimatedSpriteMaterial();

    QImage image = m_spriteEngine->assembledImage(); //Engine prints errors if there are any
    if (image.isNull())
        return 0;
    m_sheetSize = QSizeF(image.size());
    m_material->texture = window()->createTextureFromImage(image);
    m_material->texture->setFiltering(QSGTexture::Linear);
    m_spriteEngine->start(0);
    m_material->animT = 0;
    m_material->animX1 = m_spriteEngine->spriteX() / m_sheetSize.width();
    m_material->animY1 = m_spriteEngine->spriteY() / m_sheetSize.height();
    m_material->animX2 = m_material->animX1;
    m_material->animY2 = m_material->animY1;
    m_material->animW = m_spriteEngine->spriteWidth() / m_sheetSize.width();
    m_material->animH = m_spriteEngine->spriteHeight() / m_sheetSize.height();

    int vCount = 4;
    int iCount = 6;
    QSGGeometry *g = new QSGGeometry(AnimatedSprite_AttributeSet, vCount, iCount);
    g->setDrawingMode(GL_TRIANGLES);

    AnimatedSpriteVertices *p = (AnimatedSpriteVertices *) g->vertexData();

    QRectF texRect = m_material->texture->normalizedTextureSubRect();

    p->v1.tx = texRect.topLeft().x();
    p->v1.ty = texRect.topLeft().y();

    p->v2.tx = texRect.topRight().x();
    p->v2.ty = texRect.topRight().y();

    p->v3.tx = texRect.bottomLeft().x();
    p->v3.ty = texRect.bottomLeft().y();

    p->v4.tx = texRect.bottomRight().x();
    p->v4.ty = texRect.bottomRight().y();

    quint16 *indices = g->indexDataAsUShort();
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 1;
    indices[4] = 3;
    indices[5] = 2;


    m_timestamp.start();
    m_node = new QSGGeometryNode();
    m_node->setGeometry(g);
    m_node->setMaterial(m_material);
    m_node->setFlag(QSGGeometryNode::OwnsMaterial);
    sizeVertices();
    return m_node;
}

void QQuickAnimatedSprite::reset()
{
    m_pleaseReset = true;
}

QSGNode *QQuickAnimatedSprite::updatePaintNode(QSGNode *, UpdatePaintNodeData *)
{
    if (m_pleaseReset) {
        delete m_node;

        m_node = 0;
        m_material = 0;
        m_pleaseReset = false;
    }

    prepareNextFrame();

    if (m_running) {
        update();
        if (m_node)
            m_node->markDirty(QSGNode::DirtyMaterial);
    }

    return m_node;
}

void QQuickAnimatedSprite::prepareNextFrame()
{
    if (m_node == 0)
        m_node = buildNode();
    if (m_node == 0) //error creating node
        return;

    int timeInt = m_timestamp.elapsed() + m_pauseOffset;
    qreal time =  timeInt / 1000.;

    double frameAt; //double just for modf
    qreal progress = 0.0;
    int lastFrame = m_curFrame;
    if (!m_paused) {
        //Advance State (keeps time for psuedostates)
        m_spriteEngine->updateSprites(timeInt);

        //Advance AnimatedSprite
        qreal animT = m_spriteEngine->spriteStart()/1000.0;
        qreal frameCount = m_spriteEngine->spriteFrames();
        qreal frameDuration = m_spriteEngine->spriteDuration()/frameCount;
        if (frameDuration > 0) {
            qreal frame = (time - animT)/(frameDuration / 1000.0);
            frame = qBound(qreal(0.0), frame, frameCount - qreal(1.0));//Stop at count-1 frames until we have between anim interpolation
            progress = modf(frame,&frameAt);
            if (m_curFrame > frameAt) //went around
                m_curLoop++;
            m_curFrame = frameAt;
        } else {
            m_curFrame++;
            if (m_curFrame >= frameCount){
                m_curFrame = 0;
                m_curLoop++;
                m_spriteEngine->advance();
            }
            frameAt = m_curFrame;
            progress = 0;
        }
        if (m_loops > 0 && m_curLoop >= m_loops) {
            frameAt = 0;
            if (m_running) {
                m_running = false;
                emit runningChanged(false);
            }
        }
    } else {
        frameAt = m_curFrame;
    }
    if (m_curFrame != lastFrame && isCurrentFrameChangedConnected())
        emit currentFrameChanged(m_curFrame);
    if (m_spriteEngine->sprite()->reverse())
        frameAt = (m_spriteEngine->spriteFrames() - 1) - frameAt;
    qreal y = m_spriteEngine->spriteY() / m_sheetSize.height();
    qreal w = m_spriteEngine->spriteWidth() / m_sheetSize.width();
    qreal h = m_spriteEngine->spriteHeight() / m_sheetSize.height();
    qreal x1 = m_spriteEngine->spriteX() / m_sheetSize.width();
    x1 += frameAt * w;
    qreal x2 = x1;
    if (frameAt < (m_spriteEngine->spriteFrames()-1))
        x2 += w;

    m_material->animX1 = x1;
    m_material->animY1 = y;
    m_material->animX2 = x2;
    m_material->animY2 = y;
    m_material->animW = w;
    m_material->animH = h;
    m_material->animT = m_interpolate ? progress : 0.0;
}

QT_END_NAMESPACE
