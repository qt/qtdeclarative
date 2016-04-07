/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

class QQuickAnimatedSpriteMaterial : public QSGMaterial
{
public:
    QQuickAnimatedSpriteMaterial();
    ~QQuickAnimatedSpriteMaterial();
    QSGMaterialType *type() const Q_DECL_OVERRIDE { static QSGMaterialType type; return &type; }
    QSGMaterialShader *createShader() const Q_DECL_OVERRIDE;
    int compare(const QSGMaterial *other) const Q_DECL_OVERRIDE
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
    AnimatedSpriteMaterialData()
        : QSGMaterialShader()
    {
        setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/items/shaders/sprite.vert"));
        setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/items/shaders/sprite.frag"));
    }

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *) Q_DECL_OVERRIDE
    {
        QQuickAnimatedSpriteMaterial *m = static_cast<QQuickAnimatedSpriteMaterial *>(newEffect);
        m->texture->bind();

        program()->setUniformValue(m_opacity_id, state.opacity());
        program()->setUniformValue(m_animData_id, m->animW, m->animH, m->animT);
        program()->setUniformValue(m_animPos_id, m->animX1, m->animY1, m->animX2, m->animY2);

        if (state.isMatrixDirty())
            program()->setUniformValue(m_matrix_id, state.combinedMatrix());
    }

    void initialize() Q_DECL_OVERRIDE {
        m_matrix_id = program()->uniformLocation("qt_Matrix");
        m_opacity_id = program()->uniformLocation("qt_Opacity");
        m_animData_id = program()->uniformLocation("animData");
        m_animPos_id = program()->uniformLocation("animPos");
    }

    char const *const *attributeNames() const Q_DECL_OVERRIDE {
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
};

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
    \inqmlmodule QtQuick
    \inherits Item
    \ingroup qtquick-visual
    \brief Draws a sprite animation

    AnimatedSprite provides rendering and control over animations which are provided
    as multiple frames in the same image file. You can play it at a fixed speed, at the
    frame rate of your display, or manually advance and control the progress.

    For details of how a sprite animation is defined see the \l{Sprite Animations} overview.
    Note that the AnimatedSprite type does not use Sprite types to define multiple animations,
    but instead encapsulates a single animation itself.
*/

/*!
    \qmlproperty bool QtQuick::AnimatedSprite::running

    Whether the sprite is animating or not.

    Default is true
*/

/*!
    \qmlproperty bool QtQuick::AnimatedSprite::interpolate

    If true, interpolation will occur between sprite frames to make the
    animation appear smoother.

    Default is true.
*/

/*!
    \qmlproperty qreal QtQuick::AnimatedSprite::frameRate

    Frames per second to show in the animation. Values equal to or below 0 are invalid.

    If frameRate is valid  then it will be used to calculate the duration of the frames.
    If not, and frameDuration is valid , then frameDuration will be used.

    Changing this parameter will restart the animation.
*/

/*!
    \qmlproperty int QtQuick::AnimatedSprite::frameDuration

    Duration of each frame of the animation. Values equal to or below 0 are invalid.

    If frameRate is valid then it will be used to calculate the duration of the frames.
    If not, and frameDuration is valid, then frameDuration will be used.

    Changing this parameter will restart the animation.
*/

/*!
    \qmlproperty int QtQuick::AnimatedSprite::frameCount

    Number of frames in this AnimatedSprite.
*/
/*!
    \qmlproperty int QtQuick::AnimatedSprite::frameHeight

    Height of a single frame in this AnimatedSprite.

    May be omitted if it is the only sprite in the file.
*/
/*!
    \qmlproperty int QtQuick::AnimatedSprite::frameWidth

    Width of a single frame in this AnimatedSprite.

    May be omitted if it is the only sprite in the file.
*/
/*!
    \qmlproperty int QtQuick::AnimatedSprite::frameX

    The X coordinate in the image file of the first frame of the AnimatedSprite.

    May be omitted if the first frame starts in the upper left corner of the file.
*/
/*!
    \qmlproperty int QtQuick::AnimatedSprite::frameY

    The Y coordinate in the image file of the first frame of the AnimatedSprite.

    May be omitted if the first frame starts in the upper left corner of the file.
*/
/*!
    \qmlproperty url QtQuick::AnimatedSprite::source

    The image source for the animation.

    If frameHeight and frameWidth are not specified, it is assumed to be a single long row of square frames.
    Otherwise, it can be multiple contiguous rows or rectangluar frames, when one row runs out the next will be used.

    If frameX and frameY are specified, the row of frames will be taken with that x/y coordinate as the upper left corner.
*/

/*!
    \qmlproperty bool QtQuick::AnimatedSprite::reverse

    If true, then the animation will be played in reverse.

    Default is false.
*/

/*!
    \qmlproperty bool QtQuick::AnimatedSprite::frameSync

    If true, then the animation will have no duration. Instead, the animation will advance
    one frame each time a frame is rendered to the screen. This synchronizes it with the painting
    rate as opposed to elapsed time.

    If frameSync is set to true, it overrides both frameRate and frameDuration.

    Default is false.

    Changing this parameter will restart the animation.
*/

/*!
    \qmlproperty int QtQuick::AnimatedSprite::loops

    After playing the animation this many times, the animation will automatically stop. Negative values are invalid.

    If this is set to AnimatedSprite.Infinite the animation will not stop playing on its own.

    Default is AnimatedSprite.Infinite
*/

/*!
    \qmlproperty bool QtQuick::AnimatedSprite::paused

    When paused, the current frame can be advanced manually.

    Default is false.
*/

/*!
    \qmlproperty int QtQuick::AnimatedSprite::currentFrame

    When paused, the current frame can be advanced manually by setting this property or calling advance().

*/

/*!
    \qmlmethod int QtQuick::AnimatedSprite::restart()

    Stops, then starts the sprite animation.
*/

//TODO: Implicitly size element to size of sprite
QQuickAnimatedSprite::QQuickAnimatedSprite(QQuickItem *parent) :
    QQuickItem(parent)
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
            this, SLOT(reset()));
    connect(this, SIGNAL(heightChanged()),
            this, SLOT(reset()));
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
    update();
}

/*!
    \qmlmethod int QtQuick::AnimatedSprite::advance()

    Advances the sprite animation by one frame.
*/
void QQuickAnimatedSprite::advance(int frames)
{
    if (!frames)
        return;
    //TODO-C: May not work when running - only when paused
    m_curFrame += frames;
    while (m_curFrame < 0)
        m_curFrame += m_spriteEngine->maxFrames();
    m_curFrame = m_curFrame % m_spriteEngine->maxFrames();
    emit currentFrameChanged(m_curFrame);
    update();
}

/*!
    \qmlmethod int QtQuick::AnimatedSprite::pause()

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
    update();
}

/*!
    \qmlmethod int QtQuick::AnimatedSprite::resume()

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
    update();
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
    update();
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

void QQuickAnimatedSprite::sizeVertices(QSGGeometryNode *node)
{
    AnimatedSpriteVertices *p = (AnimatedSpriteVertices *) node->geometry()->vertexData();
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

    QQuickAnimatedSpriteMaterial *material = new QQuickAnimatedSpriteMaterial();

    QImage image = m_spriteEngine->assembledImage(); //Engine prints errors if there are any
    if (image.isNull())
        return 0;
    m_sheetSize = QSizeF(image.size());
    material->texture = window()->createTextureFromImage(image);
    m_spriteEngine->start(0);
    material->animT = 0;
    material->animX1 = m_spriteEngine->spriteX() / m_sheetSize.width();
    material->animY1 = m_spriteEngine->spriteY() / m_sheetSize.height();
    material->animX2 = material->animX1;
    material->animY2 = material->animY1;
    material->animW = m_spriteEngine->spriteWidth() / m_sheetSize.width();
    material->animH = m_spriteEngine->spriteHeight() / m_sheetSize.height();

    int vCount = 4;
    int iCount = 6;
    QSGGeometry *g = new QSGGeometry(AnimatedSprite_AttributeSet, vCount, iCount);
    g->setDrawingMode(GL_TRIANGLES);

    AnimatedSpriteVertices *p = (AnimatedSpriteVertices *) g->vertexData();

    QRectF texRect = material->texture->normalizedTextureSubRect();

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


    QSGGeometryNode *node = new QSGGeometryNode();
    node->setGeometry(g);
    node->setMaterial(material);
    node->setFlag(QSGGeometryNode::OwnsMaterial);
    node->setFlag(QSGGeometryNode::OwnsGeometry);
    sizeVertices(node);
    return node;
}

void QQuickAnimatedSprite::reset()
{
    m_pleaseReset = true;
    update();
}

QSGNode *QQuickAnimatedSprite::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (m_pleaseReset) {
        delete oldNode;

        oldNode = 0;
        m_pleaseReset = false;
    }

    QSGGeometryNode *node = static_cast<QSGGeometryNode *>(oldNode);
    if (!node)
        node = buildNode();

    if (node)
        prepareNextFrame(node);

    if (m_running) {
        if (!m_paused)
            update();

        if (node) {
            node->markDirty(QSGNode::DirtyMaterial);
        }
    }

    return node;
}

void QQuickAnimatedSprite::prepareNextFrame(QSGGeometryNode *node)
{
    int timeInt = m_timestamp.elapsed() + m_pauseOffset;
    qreal time =  timeInt / 1000.;

    int frameAt;
    qreal progress = 0.0;
    int lastFrame = m_curFrame;
    if (m_running && !m_paused) {
        const int nColumns = int(m_sheetSize.width()) / m_spriteEngine->spriteWidth();
        //Advance State (keeps time for psuedostates)
        m_spriteEngine->updateSprites(timeInt);

        //Advance AnimatedSprite
        qreal animT = m_spriteEngine->spriteStart()/1000.0;
        const int frameCountInRow = m_spriteEngine->spriteFrames();
        const qreal frameDuration = m_spriteEngine->spriteDuration()/frameCountInRow;
        if (frameDuration > 0) {
            qreal frame = (time - animT)/(frameDuration / 1000.0);
            bool lastLoop = m_loops > 0 && m_curLoop == m_loops-1;
            //don't visually interpolate for the last frame of the last loop
            const int max = lastLoop ? frameCountInRow - 1 : frameCountInRow;
            frame = qBound(qreal(0.0), frame, qreal(max));
            double intpart;
            progress = std::modf(frame,&intpart);
            frameAt = (int)intpart;
            const int rowIndex = m_spriteEngine->spriteY()/frameHeight();
            const int newFrame = rowIndex * nColumns + frameAt;
            if (m_curFrame > newFrame) //went around
                m_curLoop++;
            m_curFrame = newFrame;
        } else {
            m_curFrame++;
            if (m_curFrame >= m_spriteEngine->maxFrames()) {    // maxFrames: total number of frames including all rows
                m_curFrame = 0;
                m_curLoop++;
            }
            frameAt = m_curFrame % nColumns;
            if (frameAt == 0)
                m_spriteEngine->advance();
            progress = 0;
        }
        if (m_loops > 0 && m_curLoop >= m_loops) {
            frameAt = 0;
            m_running = false;
            emit runningChanged(false);
            update();
        }
    } else {
        frameAt = m_curFrame;
    }
    if (m_curFrame != lastFrame) {
        if (isCurrentFrameChangedConnected())
            emit currentFrameChanged(m_curFrame);
        update();
    }

    qreal frameCount = m_spriteEngine->spriteFrames();
    bool reverse = m_spriteEngine->sprite()->reverse();
    if (reverse)
        frameAt = (frameCount - 1) - frameAt;

    qreal w = m_spriteEngine->spriteWidth() / m_sheetSize.width();
    qreal h = m_spriteEngine->spriteHeight() / m_sheetSize.height();
    qreal x1;
    qreal y1;
    if (m_paused) {
        int spriteY = m_spriteEngine->spriteY();
        if (reverse) {
            int rows = m_spriteEngine->maxFrames() * m_spriteEngine->spriteWidth() / m_sheetSize.width();
            spriteY -= rows * m_spriteEngine->spriteHeight();
            frameAt = (frameCount - 1) - frameAt;
        }

        int position = frameAt * m_spriteEngine->spriteWidth() + m_spriteEngine->spriteX();
        int row = position / m_sheetSize.width();

        x1 = (position - (row * m_sheetSize.width())) / m_sheetSize.width();
        y1 = (row * m_spriteEngine->spriteHeight() + spriteY) / m_sheetSize.height();
    } else {
        x1 = m_spriteEngine->spriteX() / m_sheetSize.width() + frameAt * w;
        y1 = m_spriteEngine->spriteY() / m_sheetSize.height();
    }

    //### hard-coded 0/1 work because we are the only
    // images in the sprite sheet (without this we cannot assume
    // where in the sheet we begin/end).
    qreal x2;
    qreal y2;
    if (reverse) {
        if (frameAt > 0) {
            x2 = x1 - w;
            y2 = y1;
        } else {
            x2 = 1.0 - w;
            y2 = y1 - h;
            if (y2 < 0.0) {
                //the last row may not fill the entire width
                int maxRowFrames = m_sheetSize.width() / m_spriteEngine->spriteWidth();
                if (m_spriteEngine->maxFrames() % maxRowFrames)
                    x2 = ((m_spriteEngine->maxFrames() % maxRowFrames) - 1) * w;

                y2 = 1.0 - h;
            }
        }
    } else {
        if (frameAt < (frameCount-1)) {
            x2 = x1 + w;
            y2 = y1;
        } else {
            x2 = 0.0;
            y2 = y1 + h;
            if (y2 >= 1.0)
                y2 = 0.0;
        }
    }

    QQuickAnimatedSpriteMaterial *material = static_cast<QQuickAnimatedSpriteMaterial *>(node->material());
    material->animX1 = x1;
    material->animY1 = y1;
    material->animX2 = x2;
    material->animY2 = y2;
    material->animW = w;
    material->animH = h;
    material->animT = m_interpolate ? progress : 0.0;
    material->texture->setFiltering(smooth() ? QSGTexture::Linear : QSGTexture::Nearest);
}

QT_END_NAMESPACE
