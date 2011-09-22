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

#ifndef QSGCONTEXT2DCOMMANDBUFFER_P_H
#define QSGCONTEXT2DCOMMANDBUFFER_P_H

#include "qsgcontext2d_p.h"
#include "qdeclarativepixmapcache_p.h"


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGCanvasItem;
class QMutex;

class QSGContext2DCommandBuffer
{
public:
    QSGContext2DCommandBuffer();
    ~QSGContext2DCommandBuffer();
    void reset();
    void clear();
    inline int size() {return commands.size();}
    inline bool isEmpty() const {return commands.isEmpty(); }
    inline bool hasNext() const {return cmdIdx < commands.size(); }
    inline QSGContext2D::PaintCommand takeNextCommand() { return commands[cmdIdx++]; }

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
        commands << QSGContext2D::GlobalAlpha;
        reals << alpha;
    }

    inline void setGlobalCompositeOperation(QPainter::CompositionMode cm)
    {
        commands << QSGContext2D::GlobalCompositeOperation;
        ints << cm;
    }

    inline void setStrokeStyle(const QBrush &style, bool repeatX = false, bool repeatY = false)
    {
        commands << QSGContext2D::StrokeStyle;
        brushes << style;
        bools << repeatX << repeatY;
    }

    inline void drawImage(const QImage& image, qreal sx, qreal sy, qreal sw, qreal sh, qreal dx, qreal dy, qreal dw, qreal dh)
    {
        commands << QSGContext2D::DrawImage;
        images << image;
        reals << sx << sy << sw << sh << dx << dy << dw << dh;
    }

    inline qreal takeShadowOffsetX() { return takeReal(); }
    inline qreal takeShadowOffsetY() { return takeReal(); }
    inline qreal takeShadowBlur() { return takeReal(); }
    inline QColor takeShadowColor() { return takeColor(); }


    inline void updateMatrix(const QTransform& matrix)
    {
        commands << QSGContext2D::UpdateMatrix;
        matrixes << matrix;
    }

    inline void clearRect(qreal x, qreal y, qreal w, qreal h)
    {
        commands << QSGContext2D::ClearRect;
        reals << x << y << w << h;
    }

    inline void fillRect(qreal x, qreal y, qreal w, qreal h)
    {
        commands << QSGContext2D::FillRect;
        reals << x << y << w << h;
    }

    inline void strokeRect(qreal x, qreal y, qreal w, qreal h)
    {
        QPainterPath p;
        p.addRect(x, y, w, h);

        commands << QSGContext2D::Stroke;
        pathes << p;
    }


    inline void fill(const QPainterPath& path)
    {
        commands << QSGContext2D::Fill;
        pathes << path;

    }

    inline void stroke(const QPainterPath& path)
    {
        commands << QSGContext2D::Stroke;
        pathes << path;
    }

    inline void clip(const QPainterPath& path)
    {
        commands << QSGContext2D::Clip;
        pathes << path;
    }



    inline void setFillStyle(const QBrush &style, bool repeatX = false, bool repeatY = false)
    {
        commands << QSGContext2D::FillStyle;
        brushes << style;
        bools << repeatX << repeatY;
    }


    inline void setLineWidth( qreal w)
    {
        commands << QSGContext2D::LineWidth;
        reals << w;
    }

    inline void setLineCap(Qt::PenCapStyle  cap)
    {
        commands << QSGContext2D::LineCap;
        ints << cap;
    }

    inline void setLineJoin(Qt::PenJoinStyle join)
    {
        commands << QSGContext2D::LineJoin;
        ints << join;
    }

    inline void setMiterLimit( qreal limit)
    {
        commands << QSGContext2D::MiterLimit;
        reals << limit;
    }

    inline void setShadowOffsetX( qreal x)
    {
        commands << QSGContext2D::ShadowOffsetX;
        reals << x;
    }

    inline void setShadowOffsetY( qreal y)
    {
        commands << QSGContext2D::ShadowOffsetY;
        reals << y;
    }

    inline void setShadowBlur( qreal b)
    {
        commands << QSGContext2D::ShadowBlur;
        reals << b;
    }

    inline void setShadowColor(const QColor &color)
    {
        commands << QSGContext2D::ShadowColor;
        colors << color;
    }

    inline QTransform takeMatrix() { return matrixes[matrixIdx++]; }

    // rects
    inline QRectF takeRect() {
        qreal x, y, w, h;
        x = takeReal();
        y = takeReal();
        w = takeReal();
        h = takeReal();
        return QRectF(x, y, w ,h);
    }

    inline QPainterPath takePath() { return pathes[pathIdx++]; }

    inline const QImage& takeImage() { return images[imageIdx++]; }

    inline int takeInt() { return ints[intIdx++]; }
    inline bool takeBool() {return bools[boolIdx++]; }
    inline qreal takeReal() { return reals[realIdx++]; }
    inline QColor takeColor() { return colors[colorIdx++]; }
    inline QBrush takeBrush() { return brushes[brushIdx++]; }

    QSGContext2D::State replay(QPainter* painter, QSGContext2D::State state);
private:
    QPen makePen(QSGContext2D::State state);
    void setPainterState(QPainter* painter, QSGContext2D::State state, const QPen& pen);
    int cmdIdx;
    int intIdx;
    int boolIdx;
    int realIdx;
    int colorIdx;
    int matrixIdx;
    int brushIdx;
    int pathIdx;
    int imageIdx;
    QVector<QSGContext2D::PaintCommand> commands;

    QVector<int> ints;
    QVector<bool> bools;
    QVector<qreal> reals;
    QVector<QColor> colors;
    QVector<QTransform> matrixes;
    QVector<QBrush> brushes;
    QVector<QPainterPath> pathes;
    QVector<QImage> images;
};

QT_END_HEADER

QT_END_NAMESPACE

#endif // QSGCONTEXT2DCOMMANDBUFFER_P_H
