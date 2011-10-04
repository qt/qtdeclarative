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

#include "qsgcontext2dcommandbuffer_p.h"
#include "qsgcanvasitem_p.h"
#include "qdeclarative.h"
#include <QtCore/QMutex>

#define HAS_SHADOW(offsetX, offsetY, blur, color) (color.isValid() && color.alpha() && (blur || offsetX || offsetY))

QT_BEGIN_NAMESPACE

void qt_image_boxblur(QImage& image, int radius, bool quality);

static QImage makeShadowImage(const QImage& image, qreal offsetX, qreal offsetY, qreal blur, const QColor& color)
{
    QImage shadowImg(image.width() + blur + qAbs(offsetX),
                     image.height() + blur + qAbs(offsetY),
                     QImage::Format_ARGB32_Premultiplied);
    shadowImg.fill(0);
    QPainter tmpPainter(&shadowImg);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    qreal shadowX = offsetX > 0? offsetX : 0;
    qreal shadowY = offsetY > 0? offsetY : 0;

    tmpPainter.drawImage(shadowX, shadowY, image);
    tmpPainter.end();

    if (blur > 0)
        qt_image_boxblur(shadowImg, blur/2, true);

    // blacken the image with shadow color...
    tmpPainter.begin(&shadowImg);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(shadowImg.rect(), color);
    tmpPainter.end();
    return shadowImg;
}

static void fillRectShadow(QPainter* p, QRectF shadowRect, qreal offsetX, qreal offsetY, qreal blur, const QColor& color)
{
    QRectF r = shadowRect;
    r.moveTo(0, 0);

    QImage shadowImage(r.size().width() + 1, r.size().height() + 1, QImage::Format_ARGB32_Premultiplied);
    QPainter tp;
    tp.begin(&shadowImage);
    tp.fillRect(r, p->brush());
    tp.end();
    shadowImage = makeShadowImage(shadowImage, offsetX, offsetY, blur, color);

    qreal dx = shadowRect.left() + (offsetX < 0? offsetX:0);
    qreal dy = shadowRect.top() + (offsetY < 0? offsetY:0);

    p->drawImage(dx, dy, shadowImage);
    p->fillRect(shadowRect, p->brush());
}

static void fillShadowPath(QPainter* p, const QPainterPath& path, qreal offsetX, qreal offsetY, qreal blur, const QColor& color)
{
    QRectF r = path.boundingRect();
    QImage img(r.size().width() + r.left() + 1,
               r.size().height() + r.top() + 1,
               QImage::Format_ARGB32_Premultiplied);
    img.fill(0);
    QPainter tp(&img);
    tp.fillPath(path.translated(0, 0), p->brush());
    tp.end();

    QImage shadowImage = makeShadowImage(img, offsetX, offsetY, blur, color);
    qreal dx = r.left() + (offsetX < 0? offsetX:0);
    qreal dy = r.top() + (offsetY < 0? offsetY:0);

    p->drawImage(dx, dy, shadowImage);
    p->fillPath(path, p->brush());
}

static void strokeShadowPath(QPainter* p, const QPainterPath& path, qreal offsetX, qreal offsetY, qreal blur, const QColor& color)
{
    QRectF r = path.boundingRect();
    QImage img(r.size().width() + r.left() + 1,
               r.size().height() + r.top() + 1,
               QImage::Format_ARGB32_Premultiplied);
    img.fill(0);
    QPainter tp(&img);
    tp.strokePath(path, p->pen());
    tp.end();

    QImage shadowImage = makeShadowImage(img, offsetX, offsetY, blur, color);
    qreal dx = r.left() + (offsetX < 0? offsetX:0);
    qreal dy = r.top() + (offsetY < 0? offsetY:0);
    p->drawImage(dx, dy, shadowImage);
    p->strokePath(path, p->pen());
}
static inline void drawRepeatPattern(QPainter* p, const QImage& image, const QRectF& rect, const bool repeatX, const bool repeatY)
{
    // Patterns must be painted so that the top left of the first image is anchored at
    // the origin of the coordinate space
    if (!image.isNull()) {
        int w = image.width();
        int h = image.height();
        int startX, startY;
        QRect r(static_cast<int>(rect.x()), static_cast<int>(rect.y()), static_cast<int>(rect.width()), static_cast<int>(rect.height()));

        // startX, startY is the coordinate of the first image we need to put on the left-top of the rect
        if (repeatX && repeatY) {
            // repeat
            // startX, startY is at the left top side of the left-top of the rect
            startX = r.x() >=0 ? r.x() - (r.x() % w) : r.x() - (w - qAbs(r.x()) % w);
            startY = r.y() >=0 ? r.y() - (r.y() % h) : r.y() - (h - qAbs(r.y()) % h);
        } else {
           if (!repeatX && !repeatY) {
               // no-repeat
               // only draw the image once at orgin once, check if need to draw
               QRect imageRect(0, 0, w, h);
               if (imageRect.intersects(r)) {
                   startX = 0;
                   startY = 0;
               } else
                   return;
           } else if (repeatX && !repeatY) {
               // repeat-x
               // startY is fixed, but startX change based on the left-top of the rect
               QRect imageRect(r.x(), 0, r.width(), h);
               if (imageRect.intersects(r)) {
                   startX = r.x() >=0 ? r.x() - (r.x() % w) : r.x() - (w - qAbs(r.x()) % w);
                   startY = 0;
               } else
                   return;
           } else {
               // repeat-y
               // startX is fixed, but startY change based on the left-top of the rect
               QRect imageRect(0, r.y(), w, r.height());
               if (imageRect.intersects(r)) {
                   startX = 0;
                   startY = r.y() >=0 ? r.y() - (r.y() % h) : r.y() - (h - qAbs(r.y()) % h);
               } else
                   return;
           }
        }

        int x = startX;
        int y = startY;
        do {
            // repeat Y
            do {
                // repeat X
                QRect   imageRect(x, y, w, h);
                QRect   intersectRect = imageRect.intersected(r);
                QPoint  destStart(intersectRect.x(), intersectRect.y());
                QRect   sourceRect(intersectRect.x() - imageRect.x(), intersectRect.y() - imageRect.y(), intersectRect.width(), intersectRect.height());

                p->drawImage(destStart, image, sourceRect);
                x += w;
            } while (repeatX && x < r.x() + r.width());
            x = startX;
            y += h;
        } while (repeatY && y < r.y() + r.height());
    }
}

QPen QSGContext2DCommandBuffer::makePen(QSGContext2D::State state)
{
    QPen pen;
    pen.setWidthF(state.lineWidth);
    pen.setCapStyle(state.lineCap);
    pen.setJoinStyle(state.lineJoin);
    pen.setMiterLimit(state.miterLimit);
    pen.setBrush(state.strokeStyle);
    return pen;
}

void QSGContext2DCommandBuffer::setPainterState(QPainter* p, QSGContext2D::State state, const QPen& pen)
{
   p->setTransform(state.matrix * p->transform());

   if (pen != p->pen())
       p->setPen(pen);

   if (state.fillStyle != p->brush())
       p->setBrush(state.fillStyle);

   if (state.font != p->font())
       p->setFont(state.font);

   if (state.globalAlpha != p->opacity()) {
       p->setOpacity(state.globalAlpha);
   }

   if (state.globalCompositeOperation != p->compositionMode())
       p->setCompositionMode(state.globalCompositeOperation);
}

QSGContext2D::State QSGContext2DCommandBuffer::replay(QPainter* p, QSGContext2D::State state)
{
    if (!p)
        return state;

    reset();

    QTransform originMatrix = p->transform();

    QPen pen = makePen(state);
    setPainterState(p, state, pen);

    while (hasNext()) {
        QSGContext2D::PaintCommand cmd = takeNextCommand();
        switch (cmd) {
        case QSGContext2D::UpdateMatrix:
        {
            state.matrix = takeMatrix();
            p->setTransform(state.matrix * originMatrix);
            break;
        }
        case QSGContext2D::ClearRect:
        {
            QPainter::CompositionMode  cm = p->compositionMode();
            qreal alpha = p->opacity();
            p->setCompositionMode(QPainter::CompositionMode_Source);
            p->setOpacity(0);
            p->fillRect(takeRect(), QColor(qRgba(0, 0, 0, 0)));
            p->setCompositionMode(cm);
            p->setOpacity(alpha);
            break;
        }
        case QSGContext2D::FillRect:
        {
            QRectF r = takeRect();
            if (HAS_SHADOW(state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor))
                fillRectShadow(p, r, state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor);
            else
                p->fillRect(r, p->brush());
            break;
        }
        case QSGContext2D::ShadowColor:
        {
            state.shadowColor = takeColor();
            break;
        }
        case QSGContext2D::ShadowBlur:
        {
            state.shadowBlur = takeShadowBlur();
            break;
        }
        case QSGContext2D::ShadowOffsetX:
        {
            state.shadowOffsetX = takeShadowOffsetX();
            break;
        }
        case QSGContext2D::ShadowOffsetY:
        {
            state.shadowOffsetY = takeShadowOffsetY();
            break;
        }
        case QSGContext2D::FillStyle:
        {
            state.fillStyle = takeFillStyle();
            state.fillPatternRepeatX = takeBool();
            state.fillPatternRepeatY = takeBool();
            p->setBrush(state.fillStyle);
            break;
        }
        case QSGContext2D::StrokeStyle:
        {
            state.strokeStyle = takeStrokeStyle();
            state.strokePatternRepeatX = takeBool();
            state.strokePatternRepeatY = takeBool();
            pen.setBrush(state.strokeStyle);
            p->setPen(pen);
            break;
        }
        case QSGContext2D::LineWidth:
        {
            state.lineWidth = takeLineWidth();
            pen.setWidth(state.lineWidth);
            p->setPen(pen);
            break;
        }
        case QSGContext2D::LineCap:
        {
            state.lineCap = takeLineCap();
            pen.setCapStyle(state.lineCap);
            p->setPen(pen);
            break;
        }
        case QSGContext2D::LineJoin:
        {
            state.lineJoin = takeLineJoin();
            pen.setJoinStyle(state.lineJoin);
            p->setPen(pen);
            break;
        }
        case QSGContext2D::MiterLimit:
        {
            state.miterLimit = takeMiterLimit();
            pen.setMiterLimit(state.miterLimit);
            p->setPen(pen);
            break;
        }
        case QSGContext2D::TextAlign:
        case QSGContext2D::TextBaseline:
            break;
        case QSGContext2D::Fill:
        {
            bool hasPattern = p->brush().style() == Qt::TexturePattern;
            QPainterPath path = takePath();
            path.closeSubpath();
            if (HAS_SHADOW(state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor))
                fillShadowPath(p,path, state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor);
            else
                p->fillPath(path, p->brush());
            break;
        }
        case QSGContext2D::Stroke:
        {
            if (HAS_SHADOW(state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor))
                strokeShadowPath(p,takePath(), state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor);
            else
                p->strokePath(takePath(), p->pen());
            break;
        }
        case QSGContext2D::Clip:
        {
            state.clipPath = takePath();
            p->setClipping(true);
            p->setClipPath(state.clipPath);
            break;
        }
        case QSGContext2D::GlobalAlpha:
        {
            state.globalAlpha = takeGlobalAlpha();
            p->setOpacity(state.globalAlpha);
            break;
        }
        case QSGContext2D::GlobalCompositeOperation:
        {
            state.globalCompositeOperation = takeGlobalCompositeOperation();
            p->setCompositionMode(state.globalCompositeOperation);
            break;
        }
        case QSGContext2D::DrawImage:
        {
            qreal sx = takeReal();
            qreal sy = takeReal();
            qreal sw = takeReal();
            qreal sh = takeReal();
            qreal dx = takeReal();
            qreal dy = takeReal();
            qreal dw = takeReal();
            qreal dh = takeReal();
            QImage image = takeImage();

            if (!image.isNull()) {
                if (sw == -1 || sh == -1) {
                    sw = image.width();
                    sh = image.height();
                }
                if (sx != 0 || sy != 0 || sw != image.width() || sh != image.height())
                    image = image.copy(sx, sy, sw, sh);

                image = image.scaled(dw, dh);

                if (HAS_SHADOW(state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor)) {
                    QImage shadow = makeShadowImage(image, state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor);
                    qreal shadow_dx = dx + (state.shadowOffsetX < 0? state.shadowOffsetY:0);
                    qreal shadow_dy = dy + (state.shadowOffsetX < 0? state.shadowOffsetY:0);
                    p->drawImage(shadow_dx, shadow_dy, shadow);
                }
                p->drawImage(dx, dy, image);
            }
            break;
        }
        case QSGContext2D::GetImageData:
        {
            //TODO:
            break;
        }
        default:
            break;
        }
    }

    p->end();
    return state;
}

QSGContext2DCommandBuffer::QSGContext2DCommandBuffer()
    : cmdIdx(0)
    , intIdx(0)
    , boolIdx(0)
    , realIdx(0)
    , colorIdx(0)
    , matrixIdx(0)
    , brushIdx(0)
    , pathIdx(0)
    , imageIdx(0)
{
}


QSGContext2DCommandBuffer::~QSGContext2DCommandBuffer()
{
}

void QSGContext2DCommandBuffer::clear()
{
    commands.clear();
    ints.clear();
    bools.clear();
    reals.clear();
    colors.clear();
    matrixes.clear();
    brushes.clear();
    pathes.clear();
    images.clear();
    reset();
}

void QSGContext2DCommandBuffer::reset()
{
    cmdIdx = 0;
    intIdx = 0;
    boolIdx = 0;
    realIdx = 0;
    colorIdx = 0;
    matrixIdx = 0;
    brushIdx = 0;
    pathIdx = 0;
    imageIdx = 0;
}

QT_END_NAMESPACE

