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

#ifndef QQUICKCONTEXT2D_P_H
#define QQUICKCONTEXT2D_P_H

#include <QtQuick/qtquickglobal.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlcomponent.h>
#include <private/qquickcanvascontext_p.h>
#include <private/qquickcanvasitem_p.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtCore/qstring.h>
#include <QtCore/qstack.h>
#include <QtCore/qqueue.h>
#include <private/qv8engine_p.h>



//#define QQUICKCONTEXT2D_DEBUG //enable this for just DEBUG purpose!

#ifdef QQUICKCONTEXT2D_DEBUG
#include <QElapsedTimer>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickContext2DCommandBuffer;
class QQuickContext2DTexture;
class QQuickPixmap;
class QSGTexture;


class Q_QUICK_EXPORT QQuickContext2D : public QQuickCanvasContext
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
        State()
            : strokeStyle(QColor("#000000"))
            , fillStyle(QColor("#000000"))
            , fillPatternRepeatX(false)
            , fillPatternRepeatY(false)
            , strokePatternRepeatX(false)
            , strokePatternRepeatY(false)
            , fillRule(Qt::WindingFill)
            , globalAlpha(1.0)
            , lineWidth(1)
            , lineCap(Qt::FlatCap)
            , lineJoin(Qt::MiterJoin)
            , miterLimit(10)
            , shadowOffsetX(0)
            , shadowOffsetY(0)
            , shadowBlur(0)
            , shadowColor(qRgba(0, 0, 0, 0))
            , globalCompositeOperation(QPainter::CompositionMode_SourceOver)
            , font(QFont(QLatin1String("sans-serif"), 10))
            , textAlign(QQuickContext2D::Start)
            , textBaseline(QQuickContext2D::Alphabetic)
        {
        }

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
        QQuickContext2D::TextAlignType textAlign;
        QQuickContext2D::TextBaseLineType textBaseline;
    };

    QQuickContext2D(QObject *parent = 0);
    ~QQuickContext2D();

    QStringList contextNames() const;
    void init(QQuickCanvasItem *canvasItem, const QVariantMap &args);
    void prepare(const QSize& canvasSize, const QSize& tileSize, const QRect& canvasWindow, const QRect& dirtyRect, bool smooth);
    void flush();
    void sync();
    QSGDynamicTexture *texture() const;
    QImage toImage(const QRectF& bounds);

    v8::Handle<v8::Object> v8value() const;
    void setV8Engine(QV8Engine *eng);

    QQuickCanvasItem* canvas() const { return m_canvas; }
    QQuickContext2DCommandBuffer* buffer() const { return m_buffer; }
    QQuickContext2DCommandBuffer* nextBuffer();

    bool bufferValid() const { return m_buffer != 0; }
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
    QStack<QQuickContext2D::State> m_stateStack;
    QQuickCanvasItem* m_canvas;
    QQuickContext2DCommandBuffer* m_buffer;
    QPainterPath m_path;
    v8::Local<v8::Value> m_fillStyle;
    v8::Local<v8::Value> m_strokeStyle;
    v8::Handle<v8::Value> m_v8path;
    QV8Engine *m_v8engine;
    v8::Persistent<v8::Object> m_v8value;
    QQuickContext2DTexture *m_texture;
    QQuickCanvasItem::RenderTarget m_renderTarget;
    QQuickCanvasItem::RenderStrategy m_renderStrategy;
    QQueue<QQuickContext2DCommandBuffer*> m_bufferQueue;
    QMutex m_bufferMutex;
};


QT_END_NAMESPACE
QML_DECLARE_TYPE(QQuickContext2D)

QT_END_HEADER

#endif // QQUICKCONTEXT2D_P_H
