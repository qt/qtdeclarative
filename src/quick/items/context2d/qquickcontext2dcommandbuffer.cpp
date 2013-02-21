/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qquickcontext2dcommandbuffer_p.h"
#include "qquickcanvasitem_p.h"
#include <qqml.h>
#include <QtCore/QMutex>
#include <QtQuick/qsgtexture.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QPaintEngine>
#include <QtGui/private/qopenglpaintengine_p.h>

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

QPen QQuickContext2DCommandBuffer::makePen(const QQuickContext2D::State& state)
{
    QPen pen;
    pen.setWidthF(state.lineWidth);
    pen.setCapStyle(state.lineCap);
    pen.setJoinStyle(state.lineJoin);
    pen.setMiterLimit(state.miterLimit);
    pen.setBrush(state.strokeStyle);
    return pen;
}

void QQuickContext2DCommandBuffer::setPainterState(QPainter* p, const QQuickContext2D::State& state, const QPen& pen)
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

static void qt_drawImage(QPainter *p, QQuickContext2D::State& state, QImage image, const QRectF& sr, const QRectF& dr, bool shadow = false)
{
    Q_ASSERT(p);

    if (image.isNull())
        return;

    qreal sx = sr.x();
    qreal sy = sr.y();
    qreal sw = sr.width();
    qreal sh = sr.height();
    qreal dx = dr.x();
    qreal dy = dr.y();
    qreal dw = dr.width();
    qreal dh = dr.height();

    if (sw == -1 || sh == -1) {
        sw = image.width();
        sh = image.height();
    }
    if (sx != 0 || sy != 0 || sw != image.width() || sh != image.height())
        image = image.copy(sx, sy, sw, sh);

    if (sw != dw || sh != dh)
        image = image.scaled(dw, dh);

    if (shadow) {
        QImage shadow = makeShadowImage(image, state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor);
        qreal shadow_dx = dx + (state.shadowOffsetX < 0? state.shadowOffsetY:0);
        qreal shadow_dy = dy + (state.shadowOffsetX < 0? state.shadowOffsetY:0);
        p->drawImage(shadow_dx, shadow_dy, shadow);
    }
    //Strange OpenGL painting behavior here, without beginNativePainting/endNativePainting, only the first image is painted.
    p->beginNativePainting();
    p->drawImage(dx, dy, image);
    p->endNativePainting();
}

void QQuickContext2DCommandBuffer::replay(QPainter* p, QQuickContext2D::State& state)
{
    if (!p)
        return;

    reset();

    QTransform originMatrix = p->worldTransform();

    QPen pen = makePen(state);
    setPainterState(p, state, pen);

    while (hasNext()) {
        QQuickContext2D::PaintCommand cmd = takeNextCommand();
        switch (cmd) {
        case QQuickContext2D::UpdateMatrix:
        {
            state.matrix = takeMatrix();
            p->setWorldTransform(state.matrix * originMatrix);
            break;
        }
        case QQuickContext2D::ClearRect:
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
        case QQuickContext2D::FillRect:
        {
            QRectF r = takeRect();
            if (HAS_SHADOW(state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor))
                fillRectShadow(p, r, state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor);
            else
                p->fillRect(r, p->brush());
            break;
        }
        case QQuickContext2D::ShadowColor:
        {
            state.shadowColor = takeColor();
            break;
        }
        case QQuickContext2D::ShadowBlur:
        {
            state.shadowBlur = takeShadowBlur();
            break;
        }
        case QQuickContext2D::ShadowOffsetX:
        {
            state.shadowOffsetX = takeShadowOffsetX();
            break;
        }
        case QQuickContext2D::ShadowOffsetY:
        {
            state.shadowOffsetY = takeShadowOffsetY();
            break;
        }
        case QQuickContext2D::FillStyle:
        {
            state.fillStyle = takeFillStyle();
            state.fillPatternRepeatX = takeBool();
            state.fillPatternRepeatY = takeBool();
            p->setBrush(state.fillStyle);
            break;
        }
        case QQuickContext2D::StrokeStyle:
        {
            state.strokeStyle = takeStrokeStyle();
            state.strokePatternRepeatX = takeBool();
            state.strokePatternRepeatY = takeBool();
            QPen nPen = p->pen();
            nPen.setBrush(state.strokeStyle);
            p->setPen(nPen);
            break;
        }
        case QQuickContext2D::LineWidth:
        {
            state.lineWidth = takeLineWidth();
            QPen nPen = p->pen();

            nPen.setWidthF(state.lineWidth);
            p->setPen(nPen);
            break;
        }
        case QQuickContext2D::LineCap:
        {
            state.lineCap = takeLineCap();
            QPen nPen = p->pen();
            nPen.setCapStyle(state.lineCap);
            p->setPen(nPen);
            break;
        }
        case QQuickContext2D::LineJoin:
        {
            state.lineJoin = takeLineJoin();
            QPen nPen = p->pen();
            nPen.setJoinStyle(state.lineJoin);
            p->setPen(nPen);
            break;
        }
        case QQuickContext2D::MiterLimit:
        {
            state.miterLimit = takeMiterLimit();
            QPen nPen = p->pen();
            nPen.setMiterLimit(state.miterLimit);
            p->setPen(nPen);
            break;
        }
        case QQuickContext2D::TextAlign:
        case QQuickContext2D::TextBaseline:
            break;
        case QQuickContext2D::Fill:
        {
            QPainterPath path = takePath();
            path.closeSubpath();
            if (HAS_SHADOW(state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor))
                fillShadowPath(p,path, state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor);
            else
                p->fillPath(path, p->brush());
            break;
        }
        case QQuickContext2D::Stroke:
        {
            if (HAS_SHADOW(state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor))
                strokeShadowPath(p,takePath(), state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor);
            else
                p->strokePath(takePath(), p->pen());
            break;
        }
        case QQuickContext2D::Clip:
        {
            state.clipPath = takePath();
            p->setClipping(true);
            p->setClipPath(state.clipPath);
            break;
        }
        case QQuickContext2D::GlobalAlpha:
        {
            state.globalAlpha = takeGlobalAlpha();
            p->setOpacity(state.globalAlpha);
            break;
        }
        case QQuickContext2D::GlobalCompositeOperation:
        {
            state.globalCompositeOperation = takeGlobalCompositeOperation();
            p->setCompositionMode(state.globalCompositeOperation);
            break;
        }
        case QQuickContext2D::DrawImage:
        {
            QRectF sr = takeRect();
            QRectF dr = takeRect();
            qt_drawImage(p, state, takeImage(), sr, dr, HAS_SHADOW(state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor));
            break;
        }
        case QQuickContext2D::DrawPixmap:
        {
            QRectF sr = takeRect();
            QRectF dr = takeRect();

            QQmlRefPointer<QQuickCanvasPixmap> pix = takePixmap();
            Q_ASSERT(!pix.isNull());

            const bool hasShadow = HAS_SHADOW(state.shadowOffsetX, state.shadowOffsetY, state.shadowBlur, state.shadowColor);
            if (p->paintEngine()->type() != QPaintEngine::OpenGL2 || hasShadow){
                //TODO: generate shadow blur with shaders
                qt_drawImage(p, state, pix->image(), sr, dr, hasShadow);
            } else if (pix->texture()){
                QSGTexture *tex = pix->texture();
                QSGDynamicTexture *dynamicTexture = qobject_cast<QSGDynamicTexture *>(tex);
                if (dynamicTexture)
                    dynamicTexture->updateTexture();

                if (tex->textureId()) {

                    if (sr.width() < 0)
                        sr.setWidth(tex->textureSize().width());
                    if (sr.height() < 0)
                        sr.setHeight(tex->textureSize().height());

                    if (dr.width() < 0)
                        dr.setWidth(sr.width());
                    if (dr.height() < 0)
                        dr.setHeight(sr.height());

                    qreal srBottom = sr.bottom();
                    sr.setBottom(sr.top());
                    sr.setTop(srBottom);

                    tex->bind();
                    if (p->paintEngine()->type() == QPaintEngine::OpenGL2) {
                        QOpenGL2PaintEngineEx *engine = static_cast<QOpenGL2PaintEngineEx *>(p->paintEngine());
                        engine->drawTexture(dr, tex->textureId(), tex->textureSize(), sr);
                    }
                }
            }
            break;
        }
        case QQuickContext2D::GetImageData:
        {
            //TODO:
            break;
        }
        default:
            break;
        }
    }

    p->end();
}

QQuickContext2DCommandBuffer::QQuickContext2DCommandBuffer()
    : cmdIdx(0)
    , intIdx(0)
    , boolIdx(0)
    , realIdx(0)
    , rectIdx(0)
    , colorIdx(0)
    , matrixIdx(0)
    , brushIdx(0)
    , pathIdx(0)
    , imageIdx(0)
    , pixmapIdx(0)
{
    static bool registered = false;
    if (!registered) {
        qRegisterMetaType<QQuickContext2DCommandBuffer*>("QQuickContext2DCommandBuffer*");
        registered = true;
    }
}


QQuickContext2DCommandBuffer::~QQuickContext2DCommandBuffer()
{
}

void QQuickContext2DCommandBuffer::clear()
{
    commands.clear();
    ints.clear();
    bools.clear();
    reals.clear();
    rects.clear();
    colors.clear();
    matrixes.clear();
    brushes.clear();
    pathes.clear();
    images.clear();
    pixmaps.clear();
    reset();
}

void QQuickContext2DCommandBuffer::reset()
{
    cmdIdx = 0;
    intIdx = 0;
    boolIdx = 0;
    realIdx = 0;
    rectIdx = 0;
    colorIdx = 0;
    matrixIdx = 0;
    brushIdx = 0;
    pathIdx = 0;
    imageIdx = 0;
    pixmapIdx = 0;
}

QT_END_NAMESPACE
