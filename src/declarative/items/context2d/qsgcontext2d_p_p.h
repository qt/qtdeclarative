/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QSGCONTEXT2D_P_P_H
#define QSGCONTEXT2D_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsgcontext2d_p.h"
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE
class QSGCanvasItem;
struct QSGContext2DWorkerAgent {
    QSGContext2DWorkerAgent()
        :ref(1)
        , orig(0)
    {}

    QAtomicInt ref;
    QSGContext2D *orig;
    QMutex mutex;
    QWaitCondition syncDone;
};

class QSGContext2DPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSGContext2D)

public:
    QSGContext2DPrivate()
        : agent(0)
        , agentData(0)
        , v8engine(0)
        , cachedImage(1,1, QImage::Format_ARGB32)
        , canvas(0)
        , waitingForPainting(false)
        , valid(false)
    {
    }
    ~QSGContext2DPrivate() 
    {
        qPersistentDispose(v8value);
    }

    void updateMatrix(const QMatrix& m);

    void setSize(const QSize &s)
    {
        size = s;
        cachedImage = QImage(s, QImage::Format_ARGB32);
    }
    void clear();
    void reset();

    // compositing
    void setGlobalAlpha(qreal alpha);
    void setGlobalCompositeOperation(const QString &op);
    void setStrokeStyle(const QVariant &style);
    void setFillStyle(const QVariant &style);
    void setStrokeColor(const QColor& color);
    void setFillColor(const QColor& color);

    // line caps/joins
    void setLineWidth(qreal w);
    void setLineCap(const QString &s);
    void setLineJoin(const QString &s);
    void setMiterLimit(qreal m);

    void setFont(const QString &font);
    void setTextBaseline(const QString &font);
    void setTextAlign(const QString &font);


    // shadows
    void setShadowOffsetX(qreal x);
    void setShadowOffsetY(qreal y);
    void setShadowBlur(qreal b);
    void setShadowColor(const QString &str);

    bool hasShadow() const;
    void clearShadow();
    QImage makeShadowImage(const QPixmap& pix);
    void fillRectShadow(QPainter* p, QRectF shadowRect);
    void fillShadowPath(QPainter* p, const QPainterPath& path);
    void strokeShadowPath(QPainter* p, const QPainterPath& path);
    void save();
    void restore();

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

    void drawImage(const QString& url, qreal dx, qreal dy);
    void drawImage(const QString& url, qreal dx, qreal dy, qreal dw, qreal dh);
    void drawImage(const QString& url, qreal sx, qreal sy, qreal sw, qreal sh, qreal dx, qreal dy, qreal dw, qreal dh);

    QList<int> getImageData(qreal sx, qreal sy, qreal sw, qreal sh);
    void putImageData(const QVariantList& imageData, qreal x, qreal y, qreal w, qreal h);

    int baseLineOffset(QSGContext2D::TextBaseLineType value, const QFontMetrics &metrics);
    int textAlignOffset(QSGContext2D::TextAlignType value, const QFontMetrics &metrics, const QString &string);

    void clearCommands()
    {
        //qDebug() << "painting commands:" << commands.size();
        commands.remove(0, commands.size());
        variants.remove(0, variants.size());
        pens.remove(0, pens.size());
        ints.remove(0, ints.size());
        reals.remove(0, reals.size());
        strings.remove(0, strings.size());
        colors.remove(0, colors.size());
        matrixes.remove(0, matrixes.size());
        brushes.remove(0, brushes.size());
        pathes.remove(0, pathes.size());
        fonts.remove(0, fonts.size());
        images.remove(0, images.size());
        sizes.remove(0, sizes.size());
    }

    //current context2d variables
    QPainterPath path;
    QSize size;
    QSGContext2D::State state;
    QStack<QSGContext2D::State> stateStack;

    //variables for actual painting
    QVector<QSGContext2D::PaintCommand> commands;
    QVector<QVariant> variants;
    QVector<int> ints;
    QVector<qreal> reals;
    QVector<QString> strings;
    QVector<QColor> colors;
    QVector<QMatrix> matrixes;
    QVector<QPen> pens;
    QVector<QBrush> brushes;
    QVector<QPainterPath> pathes;
    QVector<QFont> fonts;
    QVector<QImage> images;
    QVector<QSize> sizes;
    QList<int> imageData;

    //workerscript agent
    QSGContext2D* agent;
    QSGContext2DWorkerAgent* agentData;

    QV8Engine *v8engine;
    v8::Persistent<v8::Object> v8value;

    QImage cachedImage;
    QSGCanvasItem* canvas;
    bool waitingForPainting;
    bool valid;
    QRectF tileRect;
};

QT_END_NAMESPACE

#endif // QSGCONTEXT2D_P_P_H
