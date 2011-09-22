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

#ifndef QSGCONTEXT2D_P_H
#define QSGCONTEXT2D_P_H

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativecomponent.h>

#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtCore/qstring.h>
#include <QtCore/qstack.h>
#include <private/qv8engine_p.h>



#define QSGCONTEXT2D_DEBUG //enable this for just DEBUG purpose!

#ifdef QSGCONTEXT2D_DEBUG
#include <QElapsedTimer>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGCanvasItem;
class QSGContext2DCommandBuffer;
class QDeclarativePixmap;

class Q_DECLARATIVE_EXPORT QSGContext2D
{
public:
    enum TextBaseLineType { Alphabetic=0, Top, Middle, Bottom, Hanging};
    enum TextAlignType { Start=0, End, Left, Right, Center};
    enum PaintCommand {
        Invalid = 0,
        UpdateMatrix,
        ClearRect,
        FillRect,
        StrokeRect,
        Fill,
        Stroke,
        Clip,
        UpdateBrush,
        GlobalAlpha,
        GlobalCompositeOperation,
        StrokeStyle,
        FillStyle,
        LineWidth,
        LineCap,
        LineJoin,
        MiterLimit,
        ShadowOffsetX,
        ShadowOffsetY,
        ShadowBlur,
        ShadowColor,
        Font,
        TextBaseline,
        TextAlign,
        FillText,
        StrokeText,
        DrawImage,
        GetImageData
    };


    struct State {
        QTransform matrix;
        QPainterPath clipPath;
        QBrush strokeStyle;
        QBrush fillStyle;
        bool fillPatternRepeatX:1;
        bool fillPatternRepeatY:1;
        bool strokePatternRepeatX:1;
        bool strokePatternRepeatY:1;
        Qt::FillRule fillRule;
        qreal globalAlpha;
        qreal lineWidth;
        Qt::PenCapStyle lineCap;
        Qt::PenJoinStyle lineJoin;
        qreal miterLimit;
        qreal shadowOffsetX;
        qreal shadowOffsetY;
        qreal shadowBlur;
        QColor shadowColor;
        QPainter::CompositionMode globalCompositeOperation;
        QFont font;
        QSGContext2D::TextAlignType textAlign;
        QSGContext2D::TextBaseLineType textBaseline;
    };

    QSGContext2D(QSGCanvasItem* item);
    ~QSGContext2D();

    inline QSGCanvasItem*  canvas() const {return m_canvas;}
    inline QSGContext2DCommandBuffer* buffer() const {return m_buffer;}

    v8::Handle<v8::Object> v8value() const;
    void setV8Engine(QV8Engine *eng);
    void popState();
    void pushState();
    void reset();

    // path API
    void beginPath();
    void closePath();
    void moveTo(qreal x, qreal y);
    void lineTo(qreal x, qreal y);
    void quadraticCurveTo(qreal cpx, qreal cpy, qreal x, qreal y);
    void bezierCurveTo(qreal cp1x, qreal cp1y,
                       qreal cp2x, qreal cp2y, qreal x, qreal y);
    void arcTo(qreal x1, qreal y1, qreal x2, qreal y2, qreal radius);
    void rect(qreal x, qreal y, qreal w, qreal h);
    void roundedRect(qreal x, qreal y,qreal w, qreal h, qreal xr, qreal yr);
    void ellipse(qreal x, qreal y,qreal w, qreal h);
    void text(const QString& str, qreal x, qreal y);
    void arc(qreal x, qreal y, qreal radius,
             qreal startAngle, qreal endAngle,
             bool anticlockwise, bool transform=true);
    void addArcTo(const QPointF& p1, const QPointF& p2, float radius);

    bool isPointInPath(qreal x, qreal y) const;

    QPainterPath createTextGlyphs(qreal x, qreal y, const QString& text);
    QImage createImage(const QUrl& url);

    State state;
    QStack<QSGContext2D::State> m_stateStack;
    QSGCanvasItem* m_canvas;
    QSGContext2DCommandBuffer* m_buffer;
    QPainterPath m_path;
    v8::Local<v8::Value> m_fillStyle;
    v8::Local<v8::Value> m_strokeStyle;
    v8::Handle<v8::Value> m_v8path;
    QString m_fontString;
    QV8Engine *m_v8engine;
    v8::Persistent<v8::Object> m_v8value;
};


QT_END_NAMESPACE
QML_DECLARE_TYPE(QSGContext2D)

QT_END_HEADER

#endif // QSGCONTEXT2D_P_H
