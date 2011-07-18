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

#include "qsgtexturematerial.h"

#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/qpixmap.h>
#include <QtCore/qstring.h>
#include <QtCore/qstack.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qvariant.h>
#include <QtScript/qscriptvalue.h>
#include <private/qv8engine_p.h>
#include <QMutex>
#include <QWaitCondition>
#include "qsgimage_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

QColor colorFromString(const QString &name);

class QSGCanvasGradient : public QObject
{
    Q_OBJECT
public:
    QSGCanvasGradient(const QGradient &gradient) : m_gradient(gradient) {}

public slots:
    QGradient value() { return m_gradient; }
    void addColorStop(float pos, const QString &color) { m_gradient.setColorAt(pos, colorFromString(color));}

public:
    QGradient m_gradient;
};

Q_DECLARE_METATYPE(QSGCanvasGradient*)


class QSGCanvasPath : QObject
{
    Q_OBJECT
public:
    QSGCanvasPath(const QPainterPath& path, QObject* parent = 0) :  QObject(parent), m_path(path) {}

    QPainterPath m_path;
};
Q_DECLARE_METATYPE(QSGCanvasPath*)

class QSGContext2DWorkerAgent;
class QSGContext2DPrivate;
class QSGCanvasItem;
class QSGContext2D : public QObject
{
    Q_OBJECT
    // compositing
    Q_PROPERTY(qreal globalAlpha READ globalAlpha WRITE setGlobalAlpha)
    Q_PROPERTY(QString globalCompositeOperation READ globalCompositeOperation WRITE setGlobalCompositeOperation)
    Q_PROPERTY(QVariant strokeStyle READ strokeStyle WRITE setStrokeStyle)
    Q_PROPERTY(QVariant fillStyle READ fillStyle WRITE setFillStyle)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor)
    // line caps/joins
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(QString lineCap READ lineCap WRITE setLineCap)
    Q_PROPERTY(QString lineJoin READ lineJoin WRITE setLineJoin)
    Q_PROPERTY(qreal miterLimit READ miterLimit WRITE setMiterLimit)
    // shadows
    Q_PROPERTY(qreal shadowOffsetX READ shadowOffsetX WRITE setShadowOffsetX)
    Q_PROPERTY(qreal shadowOffsetY READ shadowOffsetY WRITE setShadowOffsetY)
    Q_PROPERTY(qreal shadowBlur READ shadowBlur WRITE setShadowBlur)
    Q_PROPERTY(QString shadowColor READ shadowColor WRITE setShadowColor)
    // fonts
    Q_PROPERTY(QString font READ font WRITE setFont)
    Q_PROPERTY(QString textBaseline READ textBaseline WRITE setTextBaseline)
    Q_PROPERTY(QString textAlign READ textAlign WRITE setTextAlign)

    Q_PROPERTY(QSGCanvasPath* path READ path WRITE setPath)
    Q_ENUMS(PaintCommand)
public:
    enum TextBaseLineType { Alphabetic=0, Top, Middle, Bottom, Hanging};
    enum TextAlignType { Start=0, End, Left, Right, Center};
    enum PaintCommand {
        Invalid = 0,
        Save,
        Restore,
        //matrix operations
        UpdateMatrix,
        Scale,
        Rotate,
        Translate,
        Transform,
        SetTransform,

        ClearRect,
        FillRect,

        //path operations
        UpdatePath,
        BeginPath,
        ClosePath,
        MoveTo,
        LineTo,
        QuadraticCurveTo,
        BezierCurveTo,
        ArcTo,
        Rect,
        Arc,
        Fill,
        Stroke,
        Clip,
        StrokeRect,

        //brushes and pens
        UpdateBrush,
        UpdatePen,
        GlobalAlpha,
        GlobalCompositeOperation,
        StrokeStyle,
        FillStyle,
        StrokeColor,
        FillColor,
        LineWidth,
        LineCap,
        LineJoin,
        MiterLimit,

        //shadows
        UpdateShadow,
        ShadowOffsetX,
        ShadowOffsetY,
        ShadowBlur,
        ShadowColor,

        //font&text
        Font,
        TextBaseline,
        TextAlign,
        FillText,
        StrokeText,

        //image
        DrawImage1,
        DrawImage2,
        DrawImage3,
        GetImageData,
        PutImageData
    };

    QSGContext2D(QObject *parent = 0);
    QSGContext2D(QSGContext2D *ctx2d, QSGContext2DWorkerAgent* agentData);
    ~QSGContext2D();

    QSGCanvasItem*  canvas() const;

    void setSize(int width, int height);
    void setSize(const QSize &size);
    QSize size() const;

    void clear();
    void reset();
    QPaintDevice* paintDevice();
    const QImage& toImage() const;
    bool requireCachedImage() const;
    void setCachedImage(const QImage& image);
    // compositing
    qreal globalAlpha() const; // (default 1.0)
    QString globalCompositeOperation() const; // (default over)
    QVariant strokeStyle() const; // (default black)
    QVariant fillStyle() const; // (default black)
    QColor strokeColor() const; // (default black)
    QColor fillColor() const; // (default black)

    void setGlobalAlpha(qreal alpha);
    void setGlobalCompositeOperation(const QString &op);
    void setStrokeStyle(const QVariant &style);
    void setFillStyle(const QVariant &style);
    void setStrokeColor(const QColor& color);
    void setFillColor(const QColor& color);

    // line caps/joins
    qreal lineWidth() const; // (default 1)
    QString lineCap() const; // "butt", "round", "square" (default "butt")
    QString lineJoin() const; // "round", "bevel", "miter" (default "miter")
    qreal miterLimit() const; // (default 10)

    void setLineWidth(qreal w);
    void setLineCap(const QString &s);
    void setLineJoin(const QString &s);
    void setMiterLimit(qreal m);

    void setFont(const QString &font);
    QString font() const;
    void setTextBaseline(const QString &font);
    QString textBaseline() const;
    void setTextAlign(const QString &font);
    QString textAlign() const;


    // shadows
    qreal shadowOffsetX() const; // (default 0)
    qreal shadowOffsetY() const; // (default 0)
    qreal shadowBlur() const; // (default 0)
    QString shadowColor() const; // (default black)

    void setShadowOffsetX(qreal x);
    void setShadowOffsetY(qreal y);
    void setShadowBlur(qreal b);
    void setShadowColor(const QString &str);

    QSGCanvasPath* path();
    void setPath(QSGCanvasPath* path);
public slots:
    void save(); // push state on state stack
    void restore(); // pop state stack and restore state

    //    QTextMetrics measureText(const QString& text);

    void fillText(const QString &text, qreal x, qreal y);
    void strokeText(const QString &text, qreal x, qreal y);

    void scale(qreal x, qreal y);
    void rotate(qreal angle);
    void translate(qreal x, qreal y);
    void transform(qreal m11, qreal m12, qreal m21, qreal m22,
                   qreal dx, qreal dy);
    void setTransform(qreal m11, qreal m12, qreal m21, qreal m22,
                      qreal dx, qreal dy);

    QSGCanvasGradient *createLinearGradient(qreal x0, qreal y0,
                                         qreal x1, qreal y1);
    QSGCanvasGradient *createRadialGradient(qreal x0, qreal y0,
                                         qreal r0, qreal x1,
                                         qreal y1, qreal r1);

    // rects
    void clearRect(qreal x, qreal y, qreal w, qreal h);
    void fillRect(qreal x, qreal y, qreal w, qreal h);
    void strokeRect(qreal x, qreal y, qreal w, qreal h);

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
    void arc(qreal x, qreal y, qreal radius,
             qreal startAngle, qreal endAngle,
             bool anticlockwise);
    void fill();
    void stroke();
    void clip();
    bool isPointInPath(qreal x, qreal y) const;

    //path string parser
    //implement the W3C SVG path spec:
    //http://www.w3.org/TR/SVG/paths.html
    void setPathString(const QString& path);
    QSGCanvasPath* createPath(const QString& pathString);

    QSGImage *createImage(const QString &url);

    void drawImage(const QString& imgUrl, qreal dx, qreal dy);
    void drawImage(const QString& imgUrl, qreal dx, qreal dy, qreal dw, qreal dh);
    void drawImage(const QString& imgUrl, qreal sx, qreal sy, qreal sw, qreal sh, qreal dx, qreal dy, qreal dw, qreal dh);

    // pixel manipulation
    QList<int> getImageData(qreal sx, qreal sy, qreal sw, qreal sh);
    void putImageData(const QVariant& imageData, qreal x, qreal y, qreal w, qreal h);

    void paint(QPainter* painter);
    void sync();
    void processCommands(const QScriptValue& commands);
signals:
    void changed();
    void painted();
public:
    bool isDirty() const;
    v8::Handle<v8::Object> v8value() const;
    QV8Engine* v8Engine() const;
    void setV8Engine(QV8Engine *eng);

    bool valid() const;
    void setValid(bool valid);
    void setTileRect(const QRectF& region);
    void addref();
    void release();

    struct VariantRef
    {
        VariantRef() : a(0) {}
        VariantRef(const VariantRef &r) : a(r.a) { if (a) a->addref(); }
        VariantRef(QSGContext2D *_a) : a(_a) { if (a) a->addref(); }
        ~VariantRef() { if (a) a->release(); }

        VariantRef &operator=(const VariantRef &o) {
            if (o.a) o.a->addref();
            if (a) a->release(); a = o.a;
            return *this;
        }

        QSGContext2D *a;
    };
    struct Sync : public QEvent {
        Sync() : QEvent(QEvent::User) {}
        QSGContext2DWorkerAgent *data;
    };
    struct State {
        QMatrix matrix;
        QPainterPath clipPath;
        QBrush strokeStyle;
        QBrush fillStyle;
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
        QPen pen;
    };

    QMatrix worldMatrix() const;

protected:
    virtual bool event(QEvent *);

private:
    void processCommand(const QScriptValue& command);

    Q_DECLARE_PRIVATE(QSGContext2D)
};


QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSGContext2D::VariantRef)
QML_DECLARE_TYPE(QSGContext2D)

QT_END_HEADER

#endif // QSGCONTEXT2D_P_H
