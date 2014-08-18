/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt SceneGraph Raster Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/
#include "imagenode.h"

#include "pixmaptexture.h"
#include "softwarelayer.h"
#include <QPainter>
#include <qmath.h>

// Helper from widgets/styles/qdrawutil.cpp

namespace SoftwareContext {

void qDrawBorderPixmap(QPainter *painter, const QRect &targetRect, const QMargins &targetMargins,
                       const QPixmap &pixmap, const QRect &sourceRect,const QMargins &sourceMargins,
                       const QTileRules &rules, QDrawBorderPixmap::DrawingHints hints)
{
    QPainter::PixmapFragment d;
    d.opacity = 1.0;
    d.rotation = 0.0;

    QPixmapFragmentsArray opaqueData;
    QPixmapFragmentsArray translucentData;

    // source center
    const int sourceCenterTop = sourceRect.top() + sourceMargins.top();
    const int sourceCenterLeft = sourceRect.left() + sourceMargins.left();
    const int sourceCenterBottom = sourceRect.bottom() - sourceMargins.bottom() + 1;
    const int sourceCenterRight = sourceRect.right() - sourceMargins.right() + 1;
    const int sourceCenterWidth = sourceCenterRight - sourceCenterLeft;
    const int sourceCenterHeight = sourceCenterBottom - sourceCenterTop;
    // target center
    const int targetCenterTop = targetRect.top() + targetMargins.top();
    const int targetCenterLeft = targetRect.left() + targetMargins.left();
    const int targetCenterBottom = targetRect.bottom() - targetMargins.bottom() + 1;
    const int targetCenterRight = targetRect.right() - targetMargins.right() + 1;
    const int targetCenterWidth = targetCenterRight - targetCenterLeft;
    const int targetCenterHeight = targetCenterBottom - targetCenterTop;

    QVarLengthArray<qreal, 16> xTarget; // x-coordinates of target rectangles
    QVarLengthArray<qreal, 16> yTarget; // y-coordinates of target rectangles

    int columns = 3;
    int rows = 3;
    if (rules.horizontal != Qt::StretchTile && sourceCenterWidth != 0)
        columns = qMax(3, 2 + qCeil(targetCenterWidth / qreal(sourceCenterWidth)));
    if (rules.vertical != Qt::StretchTile && sourceCenterHeight != 0)
        rows = qMax(3, 2 + qCeil(targetCenterHeight / qreal(sourceCenterHeight)));

    xTarget.resize(columns + 1);
    yTarget.resize(rows + 1);

    bool oldAA = painter->testRenderHint(QPainter::Antialiasing);
    if (painter->paintEngine()->type() != QPaintEngine::OpenGL
        && painter->paintEngine()->type() != QPaintEngine::OpenGL2
        && oldAA && painter->combinedTransform().type() != QTransform::TxNone) {
        painter->setRenderHint(QPainter::Antialiasing, false);
    }

    xTarget[0] = targetRect.left();
    xTarget[1] = targetCenterLeft;
    xTarget[columns - 1] = targetCenterRight;
    xTarget[columns] = targetRect.left() + targetRect.width();

    yTarget[0] = targetRect.top();
    yTarget[1] = targetCenterTop;
    yTarget[rows - 1] = targetCenterBottom;
    yTarget[rows] = targetRect.top() + targetRect.height();

    qreal dx = targetCenterWidth;
    qreal dy = targetCenterHeight;

    switch (rules.horizontal) {
    case Qt::StretchTile:
        dx = targetCenterWidth;
        break;
    case Qt::RepeatTile:
        dx = sourceCenterWidth;
        break;
    case Qt::RoundTile:
        dx = targetCenterWidth / qreal(columns - 2);
        break;
    }

    for (int i = 2; i < columns - 1; ++i)
        xTarget[i] = xTarget[i - 1] + dx;

    switch (rules.vertical) {
    case Qt::StretchTile:
        dy = targetCenterHeight;
        break;
    case Qt::RepeatTile:
        dy = sourceCenterHeight;
        break;
    case Qt::RoundTile:
        dy = targetCenterHeight / qreal(rows - 2);
        break;
    }

    for (int i = 2; i < rows - 1; ++i)
        yTarget[i] = yTarget[i - 1] + dy;

    // corners
    if (targetMargins.top() > 0 && targetMargins.left() > 0 && sourceMargins.top() > 0 && sourceMargins.left() > 0) { // top left
        d.x = (0.5 * (xTarget[1] + xTarget[0]));
        d.y = (0.5 * (yTarget[1] + yTarget[0]));
        d.sourceLeft = sourceRect.left();
        d.sourceTop = sourceRect.top();
        d.width = sourceMargins.left();
        d.height = sourceMargins.top();
        d.scaleX = qreal(xTarget[1] - xTarget[0]) / d.width;
        d.scaleY = qreal(yTarget[1] - yTarget[0]) / d.height;
        if (hints & QDrawBorderPixmap::OpaqueTopLeft)
            opaqueData.append(d);
        else
            translucentData.append(d);
    }
    if (targetMargins.top() > 0 && targetMargins.right() > 0 && sourceMargins.top() > 0 && sourceMargins.right() > 0) { // top right
        d.x = (0.5 * (xTarget[columns] + xTarget[columns - 1]));
        d.y = (0.5 * (yTarget[1] + yTarget[0]));
        d.sourceLeft = sourceCenterRight;
        d.sourceTop = sourceRect.top();
        d.width = sourceMargins.right();
        d.height = sourceMargins.top();
        d.scaleX = qreal(xTarget[columns] - xTarget[columns - 1]) / d.width;
        d.scaleY = qreal(yTarget[1] - yTarget[0]) / d.height;
        if (hints & QDrawBorderPixmap::OpaqueTopRight)
            opaqueData.append(d);
        else
            translucentData.append(d);
    }
    if (targetMargins.bottom() > 0 && targetMargins.left() > 0 && sourceMargins.bottom() > 0 && sourceMargins.left() > 0) { // bottom left
        d.x = (0.5 * (xTarget[1] + xTarget[0]));
        d.y =(0.5 * (yTarget[rows] + yTarget[rows - 1]));
        d.sourceLeft = sourceRect.left();
        d.sourceTop = sourceCenterBottom;
        d.width = sourceMargins.left();
        d.height = sourceMargins.bottom();
        d.scaleX = qreal(xTarget[1] - xTarget[0]) / d.width;
        d.scaleY = qreal(yTarget[rows] - yTarget[rows - 1]) / d.height;
        if (hints & QDrawBorderPixmap::OpaqueBottomLeft)
            opaqueData.append(d);
        else
            translucentData.append(d);
    }
    if (targetMargins.bottom() > 0 && targetMargins.right() > 0 && sourceMargins.bottom() > 0 && sourceMargins.right() > 0) { // bottom right
        d.x = (0.5 * (xTarget[columns] + xTarget[columns - 1]));
        d.y = (0.5 * (yTarget[rows] + yTarget[rows - 1]));
        d.sourceLeft = sourceCenterRight;
        d.sourceTop = sourceCenterBottom;
        d.width = sourceMargins.right();
        d.height = sourceMargins.bottom();
        d.scaleX = qreal(xTarget[columns] - xTarget[columns - 1]) / d.width;
        d.scaleY = qreal(yTarget[rows] - yTarget[rows - 1]) / d.height;
        if (hints & QDrawBorderPixmap::OpaqueBottomRight)
            opaqueData.append(d);
        else
            translucentData.append(d);
    }

    // horizontal edges
    if (targetCenterWidth > 0 && sourceCenterWidth > 0) {
        if (targetMargins.top() > 0 && sourceMargins.top() > 0) { // top
            QPixmapFragmentsArray &data = hints & QDrawBorderPixmap::OpaqueTop ? opaqueData : translucentData;
            d.sourceLeft = sourceCenterLeft;
            d.sourceTop = sourceRect.top();
            d.width = sourceCenterWidth;
            d.height = sourceMargins.top();
            d.y = (0.5 * (yTarget[1] + yTarget[0]));
            d.scaleX = dx / d.width;
            d.scaleY = qreal(yTarget[1] - yTarget[0]) / d.height;
            for (int i = 1; i < columns - 1; ++i) {
                d.x = (0.5 * (xTarget[i + 1] + xTarget[i]));
                data.append(d);
            }
            if (rules.horizontal == Qt::RepeatTile)
                data[data.size() - 1].width = ((xTarget[columns - 1] - xTarget[columns - 2]) / d.scaleX);
        }
        if (targetMargins.bottom() > 0 && sourceMargins.bottom() > 0) { // bottom
            QPixmapFragmentsArray &data = hints & QDrawBorderPixmap::OpaqueBottom ? opaqueData : translucentData;
            d.sourceLeft = sourceCenterLeft;
            d.sourceTop = sourceCenterBottom;
            d.width = sourceCenterWidth;
            d.height = sourceMargins.bottom();
            d.y = (0.5 * (yTarget[rows] + yTarget[rows - 1]));
            d.scaleX = dx / d.width;
            d.scaleY = qreal(yTarget[rows] - yTarget[rows - 1]) / d.height;
            for (int i = 1; i < columns - 1; ++i) {
                d.x = (0.5 * (xTarget[i + 1] + xTarget[i]));
                data.append(d);
            }
            if (rules.horizontal == Qt::RepeatTile)
                data[data.size() - 1].width = ((xTarget[columns - 1] - xTarget[columns - 2]) / d.scaleX);
        }
    }

    // vertical edges
    if (targetCenterHeight > 0 && sourceCenterHeight > 0) {
        if (targetMargins.left() > 0 && sourceMargins.left() > 0) { // left
            QPixmapFragmentsArray &data = hints & QDrawBorderPixmap::OpaqueLeft ? opaqueData : translucentData;
            d.sourceLeft = sourceRect.left();
            d.sourceTop = sourceCenterTop;
            d.width = sourceMargins.left();
            d.height = sourceCenterHeight;
            d.x = (0.5 * (xTarget[1] + xTarget[0]));
            d.scaleX = qreal(xTarget[1] - xTarget[0]) / d.width;
            d.scaleY = dy / d.height;
            for (int i = 1; i < rows - 1; ++i) {
                d.y = (0.5 * (yTarget[i + 1] + yTarget[i]));
                data.append(d);
            }
            if (rules.vertical == Qt::RepeatTile)
                data[data.size() - 1].height = ((yTarget[rows - 1] - yTarget[rows - 2]) / d.scaleY);
        }
        if (targetMargins.right() > 0 && sourceMargins.right() > 0) { // right
            QPixmapFragmentsArray &data = hints & QDrawBorderPixmap::OpaqueRight ? opaqueData : translucentData;
            d.sourceLeft = sourceCenterRight;
            d.sourceTop = sourceCenterTop;
            d.width = sourceMargins.right();
            d.height = sourceCenterHeight;
            d.x = (0.5 * (xTarget[columns] + xTarget[columns - 1]));
            d.scaleX = qreal(xTarget[columns] - xTarget[columns - 1]) / d.width;
            d.scaleY = dy / d.height;
            for (int i = 1; i < rows - 1; ++i) {
                d.y = (0.5 * (yTarget[i + 1] + yTarget[i]));
                data.append(d);
            }
            if (rules.vertical == Qt::RepeatTile)
                data[data.size() - 1].height = ((yTarget[rows - 1] - yTarget[rows - 2]) / d.scaleY);
        }
    }

    // center
    if (targetCenterWidth > 0 && targetCenterHeight > 0 && sourceCenterWidth > 0 && sourceCenterHeight > 0) {
        QPixmapFragmentsArray &data = hints & QDrawBorderPixmap::OpaqueCenter ? opaqueData : translucentData;
        d.sourceLeft = sourceCenterLeft;
        d.sourceTop = sourceCenterTop;
        d.width = sourceCenterWidth;
        d.height = sourceCenterHeight;
        d.scaleX = dx / d.width;
        d.scaleY = dy / d.height;

        qreal repeatWidth = (xTarget[columns - 1] - xTarget[columns - 2]) / d.scaleX;
        qreal repeatHeight = (yTarget[rows - 1] - yTarget[rows - 2]) / d.scaleY;

        for (int j = 1; j < rows - 1; ++j) {
            d.y = (0.5 * (yTarget[j + 1] + yTarget[j]));
            for (int i = 1; i < columns - 1; ++i) {
                d.x = (0.5 * (xTarget[i + 1] + xTarget[i]));
                data.append(d);
            }
            if (rules.horizontal == Qt::RepeatTile)
                data[data.size() - 1].width = repeatWidth;
        }
        if (rules.vertical == Qt::RepeatTile) {
            for (int i = 1; i < columns - 1; ++i)
                data[data.size() - i].height = repeatHeight;
        }
    }

    if (opaqueData.size())
        painter->drawPixmapFragments(opaqueData.data(), opaqueData.size(), pixmap, QPainter::OpaqueHint);
    if (translucentData.size())
        painter->drawPixmapFragments(translucentData.data(), translucentData.size(), pixmap);

    if (oldAA)
        painter->setRenderHint(QPainter::Antialiasing, true);
}

}

ImageNode::ImageNode()
    : m_innerSourceRect(0, 0, 1, 1)
    , m_subSourceRect(0, 0, 1, 1)
    , m_texture(0)
    , m_mirror(false)
    , m_smooth(true)
    , m_tileHorizontal(false)
    , m_tileVertical(false)
{
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}


void ImageNode::setTargetRect(const QRectF &rect)
{
    m_targetRect = rect;
}

void ImageNode::setInnerTargetRect(const QRectF &rect)
{
    m_innerTargetRect = rect;
}

void ImageNode::setInnerSourceRect(const QRectF &rect)
{
    m_innerSourceRect = rect;
}

void ImageNode::setSubSourceRect(const QRectF &rect)
{
    m_subSourceRect = rect;
}

void ImageNode::setTexture(QSGTexture *texture)
{
    m_texture = texture;
}

void ImageNode::setMirror(bool mirror)
{
    // ### implement support for mirrored pixmaps
    m_mirror = mirror;
}

void ImageNode::setMipmapFiltering(QSGTexture::Filtering /*filtering*/)
{
}

void ImageNode::setFiltering(QSGTexture::Filtering filtering)
{
    m_smooth = (filtering == QSGTexture::Nearest);
}

void ImageNode::setHorizontalWrapMode(QSGTexture::WrapMode wrapMode)
{
    m_tileHorizontal = (wrapMode == QSGTexture::Repeat);
}

void ImageNode::setVerticalWrapMode(QSGTexture::WrapMode wrapMode)
{
    m_tileVertical = (wrapMode == QSGTexture::Repeat);
}

void ImageNode::update()
{
}

void ImageNode::preprocess()
{
    bool doDirty = false;
    QSGLayer *t = qobject_cast<QSGLayer *>(m_texture);
    if (t) {
        doDirty = t->updateTexture();
        markDirty(DirtyGeometry);
    }
    if (doDirty)
        markDirty(DirtyMaterial);
}

static Qt::TileRule getTileRule(qreal factor)
{
    int ifactor = qRound(factor);
    if (qFuzzyCompare(factor, ifactor )) {
        if (ifactor  == 1 || ifactor  == 0)
            return Qt::StretchTile;
        return Qt::RoundTile;
    }
    return Qt::RepeatTile;
}


void ImageNode::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::SmoothPixmapTransform, m_smooth);

    const QPixmap &pm = pixmap();

    if (m_innerTargetRect != m_targetRect) {
        // border image
        QMargins margins(m_innerTargetRect.left() - m_targetRect.left(), m_innerTargetRect.top() - m_targetRect.top(),
                         m_targetRect.right() - m_innerTargetRect.right(), m_targetRect.bottom() - m_innerTargetRect.bottom());
        QTileRules tilerules(getTileRule(m_subSourceRect.width()), getTileRule(m_subSourceRect.height()));
        SoftwareContext::qDrawBorderPixmap(painter, m_targetRect.toRect(), margins, pm, QRect(0, 0, pm.width(), pm.height()),
                                           margins, tilerules, QDrawBorderPixmap::DrawingHints(0));
        return;
    }

    if (m_tileHorizontal || m_tileVertical) {
        painter->save();
        qreal sx = m_targetRect.width()/(m_subSourceRect.width()*pm.width());
        qreal sy = m_targetRect.height()/(m_subSourceRect.height()*pm.height());
        QMatrix transform(sx, 0, 0, sy, 0, 0);
        painter->setMatrix(transform, true);
        painter->drawTiledPixmap(QRectF(m_targetRect.x()/sx, m_targetRect.y()/sy, m_targetRect.width()/sx, m_targetRect.height()/sy),
                                 pm,
                                 QPointF(m_subSourceRect.left()*pm.width(), m_subSourceRect.top()*pm.height()));
        painter->restore();
    } else {
        QRectF sr(m_subSourceRect.left()*pm.width(), m_subSourceRect.top()*pm.height(),
                  m_subSourceRect.width()*pm.width(), m_subSourceRect.height()*pm.height());
        painter->drawPixmap(m_targetRect, pm, sr);
    }
}

const QPixmap &ImageNode::pixmap() const
{
    if (PixmapTexture *pt = qobject_cast<PixmapTexture*>(m_texture)) {
        return pt->pixmap();
    } else if (SoftwareLayer *layer = qobject_cast<SoftwareLayer*>(m_texture)) {
        return layer->pixmap();
    } else {
        qFatal("Image used with invalid texture format.");
    }
}
