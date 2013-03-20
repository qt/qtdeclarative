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

#ifndef QQUICKCONTEXT2DCOMMANDBUFFER_P_H
#define QQUICKCONTEXT2DCOMMANDBUFFER_P_H

#include <QtCore/qmutex.h>
#include "qquickcontext2d_p.h"

QT_BEGIN_NAMESPACE

class QQuickCanvasItem;
class QMutex;

class QQuickContext2DCommandBuffer
{
public:
    QQuickContext2DCommandBuffer();
    ~QQuickContext2DCommandBuffer();
    void reset();
    void clear();

    void lockQueue() { queueLock.lock(); }
    void unlockQueue() { queueLock.unlock(); }
    inline int size() {return commands.size();}
    inline bool isEmpty() const {return commands.isEmpty(); }
    inline bool hasNext() const {return cmdIdx < commands.size(); }
    inline QQuickContext2D::PaintCommand takeNextCommand() { return commands[cmdIdx++]; }

    inline qreal takeGlobalAlpha() { return takeReal(); }
    inline QPainter::CompositionMode takeGlobalCompositeOperation(){ return static_cast<QPainter::CompositionMode>(takeInt()); }
    inline QBrush takeStrokeStyle() { return takeBrush(); }
    inline QBrush takeFillStyle() { return takeBrush(); }

    inline qreal takeLineWidth() { return takeReal(); }
    inline Qt::PenCapStyle takeLineCap() { return static_cast<Qt::PenCapStyle>(takeInt());}
    inline Qt::PenJoinStyle takeLineJoin(){ return static_cast<Qt::PenJoinStyle>(takeInt());}
    inline qreal takeMiterLimit() { return takeReal(); }

    inline void setGlobalAlpha( qreal alpha)
    {
        commands << QQuickContext2D::GlobalAlpha;
        reals << alpha;
    }

    inline void setGlobalCompositeOperation(QPainter::CompositionMode cm)
    {
        commands << QQuickContext2D::GlobalCompositeOperation;
        ints << cm;
    }

    inline void setStrokeStyle(const QBrush &style, bool repeatX = false, bool repeatY = false)
    {
        commands << QQuickContext2D::StrokeStyle;
        brushes << style;
        bools << repeatX << repeatY;
    }

    inline void drawImage(const QImage& image,  const QRectF& sr, const QRectF& dr)
    {
        commands << QQuickContext2D::DrawImage;
        images << image;
        rects << sr << dr;
    }

    inline void drawPixmap(QQmlRefPointer<QQuickCanvasPixmap> pixmap, const QRectF& sr, const QRectF& dr)
    {
        commands << QQuickContext2D::DrawPixmap;
        pixmaps << pixmap;
        rects << sr << dr;
    }

    inline qreal takeShadowOffsetX() { return takeReal(); }
    inline qreal takeShadowOffsetY() { return takeReal(); }
    inline qreal takeShadowBlur() { return takeReal(); }
    inline QColor takeShadowColor() { return takeColor(); }


    inline void updateMatrix(const QTransform& matrix)
    {
        commands << QQuickContext2D::UpdateMatrix;
        matrixes << matrix;
    }

    inline void clearRect(const QRectF& r)
    {
        commands << QQuickContext2D::ClearRect;
        rects << r;
    }

    inline void fillRect(const QRectF& r)
    {
        commands << QQuickContext2D::FillRect;
        rects << r;
    }

    inline void strokeRect(const QRectF& r)
    {
        QPainterPath p;
        p.addRect(r);

        commands << QQuickContext2D::Stroke;
        pathes << p;
    }


    inline void fill(const QPainterPath& path)
    {
        commands << QQuickContext2D::Fill;
        pathes << path;

    }

    inline void stroke(const QPainterPath& path)
    {
        commands << QQuickContext2D::Stroke;
        pathes << path;
    }

    inline void clip(const QPainterPath& path)
    {
        commands << QQuickContext2D::Clip;
        pathes << path;
    }



    inline void setFillStyle(const QBrush &style, bool repeatX = false, bool repeatY = false)
    {
        commands << QQuickContext2D::FillStyle;
        brushes << style;
        bools << repeatX << repeatY;
    }


    inline void setLineWidth( qreal w)
    {
        commands << QQuickContext2D::LineWidth;
        reals << w;
    }

    inline void setLineCap(Qt::PenCapStyle  cap)
    {
        commands << QQuickContext2D::LineCap;
        ints << cap;
    }

    inline void setLineJoin(Qt::PenJoinStyle join)
    {
        commands << QQuickContext2D::LineJoin;
        ints << join;
    }

    inline void setMiterLimit( qreal limit)
    {
        commands << QQuickContext2D::MiterLimit;
        reals << limit;
    }

    inline void setShadowOffsetX( qreal x)
    {
        commands << QQuickContext2D::ShadowOffsetX;
        reals << x;
    }

    inline void setShadowOffsetY( qreal y)
    {
        commands << QQuickContext2D::ShadowOffsetY;
        reals << y;
    }

    inline void setShadowBlur( qreal b)
    {
        commands << QQuickContext2D::ShadowBlur;
        reals << b;
    }

    inline void setShadowColor(const QColor &color)
    {
        commands << QQuickContext2D::ShadowColor;
        colors << color;
    }

    inline QTransform takeMatrix() { return matrixes[matrixIdx++]; }

    inline QRectF takeRect() { return rects[rectIdx++]; }

    inline QPainterPath takePath() { return pathes[pathIdx++]; }

    inline const QImage& takeImage() { return images[imageIdx++]; }
    inline QQmlRefPointer<QQuickCanvasPixmap> takePixmap() { return pixmaps[pixmapIdx++]; }

    inline int takeInt() { return ints[intIdx++]; }
    inline bool takeBool() {return bools[boolIdx++]; }
    inline qreal takeReal() { return reals[realIdx++]; }
    inline QColor takeColor() { return colors[colorIdx++]; }
    inline QBrush takeBrush() { return brushes[brushIdx++]; }

    void replay(QPainter* painter, QQuickContext2D::State& state);
private:
    QPen makePen(const QQuickContext2D::State& state);
    void setPainterState(QPainter* painter, const QQuickContext2D::State& state, const QPen& pen);
    int cmdIdx;
    int intIdx;
    int boolIdx;
    int realIdx;
    int rectIdx;
    int colorIdx;
    int matrixIdx;
    int brushIdx;
    int pathIdx;
    int imageIdx;
    int pixmapIdx;
    QVector<QQuickContext2D::PaintCommand> commands;

    QVector<int> ints;
    QVector<bool> bools;
    QVector<qreal> reals;
    QVector<QRectF> rects;
    QVector<QColor> colors;
    QVector<QTransform> matrixes;
    QVector<QBrush> brushes;
    QVector<QPainterPath> pathes;
    QVector<QImage> images;
    QVector<QQmlRefPointer<QQuickCanvasPixmap> > pixmaps;
    QMutex queueLock;
};

QT_END_NAMESPACE

#endif // QQUICKCONTEXT2DCOMMANDBUFFER_P_H
