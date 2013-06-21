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

#include "qquickcontext2d_p.h"
#include "qquickcontext2dcommandbuffer_p.h"
#include "qquickcanvasitem_p.h"
#include <private/qquickcontext2dtexture_p.h>
#include <private/qquickitem_p.h>
#include <QtQuick/private/qquickshadereffectsource_p.h>
#include <QtGui/qopenglframebufferobject.h>

#include <QtQuick/private/qsgcontext_p.h>
#include <private/qquicksvgparser_p.h>
#include <private/qquickpath_p.h>

#include <private/qquickimage_p_p.h>

#include <QtGui/qguiapplication.h>
#include <qqmlinfo.h>
#include <QtCore/qmath.h>
#include <private/qv8engine_p.h>

#include <qqmlengine.h>
#include <private/qv4domerrors_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4object_p.h>
#include <QtCore/qnumeric.h>
#include <private/qquickwindow_p.h>

#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4objectproto_p.h>

#if defined(Q_OS_QNX) || defined(Q_OS_ANDROID)
#include <ctype.h>
#endif

QT_BEGIN_NAMESPACE
/*!
    \qmltype Context2D
    \instantiates QQuickContext2D
    \inqmlmodule QtQuick 2
    \ingroup qtquick-canvas
    \since QtQuick 2.0
    \brief Provides 2D context for shapes on a Canvas item

    The Context2D object can be created by \c Canvas item's \c getContext()
    method:
    \code
    Canvas {
      id:canvas
      onPaint:{
         var ctx = canvas.getContext('2d');
         //...
      }
    }
    \endcode
    The Context2D API implements the same \l
    {http://www.w3.org/TR/2dcontext}{W3C Canvas 2D Context API standard} with
    some enhanced features.

    The Context2D API provides the rendering \b{context} which defines the
    methods and attributes needed to draw on the \c Canvas item. The following
    assigns the canvas rendering context to a \c{context} variable:
    \code
    var context = mycanvas.getContext("2d")
    \endcode

    The Context2D API renders the canvas as a coordinate system whose origin
    (0,0) is at the top left corner, as shown in the figure below. Coordinates
    increase along the \c{x} axis from left to right and along the \c{y} axis
    from top to bottom of the canvas.
    \image qml-item-canvas-context.gif
*/



Q_CORE_EXPORT double qstrtod(const char *s00, char const **se, bool *ok);

static const double Q_PI   = 3.14159265358979323846;   // pi

#define DEGREES(t) ((t) * 180.0 / Q_PI)

#define CHECK_CONTEXT(r)     if (!r || !r->context || !r->context->bufferValid()) \
                                V4THROW_ERROR("Not a Context2D object");

#define CHECK_CONTEXT_SETTER(r)     if (!r || !r->context || !r->context->bufferValid()) \
                                       V4THROW_ERROR("Not a Context2D object");
#define qClamp(val, min, max) qMin(qMax(val, min), max)
#define CHECK_RGBA(c) (c == '-' || c == '.' || (c >=0 && c <= 9))
QColor qt_color_from_string(const QV4::Value &name)
{
    QByteArray str = name.toQString().toUtf8();

    char *p = str.data();
    int len = str.length();
    //rgb/hsl color string has at least 7 characters
    if (!p || len > 255 || len <= 7)
        return QColor(p);
    else {
        bool isRgb(false), isHsl(false), hasAlpha(false);
        Q_UNUSED(isHsl)

        while (isspace(*p)) p++;
        if (strncmp(p, "rgb", 3) == 0)
            isRgb = true;
        else if (strncmp(p, "hsl", 3) == 0)
            isHsl = true;
        else
            return QColor(p);

        p+=3; //skip "rgb" or "hsl"
        hasAlpha = (*p == 'a') ? true : false;

        ++p; //skip "("

        if (hasAlpha) ++p; //skip "a"

        int rh, gs, bl, alpha = 255;

        //red
        while (isspace(*p)) p++;
        rh = strtol(p, &p, 10);
        if (*p == '%') {
            rh = qRound(rh/100.0 * 255);
            ++p;
        }
        if (*p++ != ',') return QColor();

        //green
        while (isspace(*p)) p++;
        gs = strtol(p, &p, 10);
        if (*p == '%') {
            gs = qRound(gs/100.0 * 255);
            ++p;
        }
        if (*p++ != ',') return QColor();

        //blue
        while (isspace(*p)) p++;
        bl = strtol(p, &p, 10);
        if (*p == '%') {
            bl = qRound(bl/100.0 * 255);
            ++p;
        }

        if (hasAlpha) {
            if (*p++!= ',') return QColor();
            while (isspace(*p)) p++;
            bool ok = false;
            alpha = qRound(qstrtod(p, const_cast<const char **>(&p), &ok) * 255);
        }

        if (*p != ')') return QColor();
        if (isRgb)
            return QColor::fromRgba(qRgba(qClamp(rh, 0, 255), qClamp(gs, 0, 255), qClamp(bl, 0, 255), qClamp(alpha, 0, 255)));
        else if (isHsl)
            return QColor::fromHsl(qClamp(rh, 0, 255), qClamp(gs, 0, 255), qClamp(bl, 0, 255), qClamp(alpha, 0, 255));
    }
    return QColor();
}

QFont qt_font_from_string(const QString& fontString) {
    QFont font;
     // ### this is simplified and incomplete
    // ### TODO:get code from Qt webkit
     const QStringList tokens = fontString.split(QLatin1Char(' '));
     foreach (const QString &token, tokens) {
         if (token == QStringLiteral("italic"))
             font.setItalic(true);
         else if (token == QStringLiteral("bold"))
             font.setBold(true);
         else if (token.endsWith(QStringLiteral("px"))) {
             QString number = token;
             number.remove(QStringLiteral("px"));
             //font.setPointSizeF(number.trimmed().toFloat());
             font.setPixelSize(number.trimmed().toInt());
         } else
             font.setFamily(token);
     }

     return font;
}



class QQuickContext2DEngineData : public QV8Engine::Deletable
{
public:
    QQuickContext2DEngineData(QV8Engine *engine);
    ~QQuickContext2DEngineData();

    QV4::PersistentValue contextPrototype;
    QV4::PersistentValue gradientProto;
    QV4::PersistentValue pixelArrayProto;
};

V8_DEFINE_EXTENSION(QQuickContext2DEngineData, engineData)


class QQuickJSContext2D : public QV4::Object
{
    Q_MANAGED
public:
    QQuickJSContext2D(QV4::ExecutionEngine *engine)
        : QV4::Object(engine)
    {
        vtbl = &static_vtbl;
    }
    QQuickContext2D* context;

    static QV4::Value method_get_globalAlpha(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_globalAlpha(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_globalCompositeOperation(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_globalCompositeOperation(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_fillStyle(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_fillStyle(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_fillRule(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_fillRule(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_strokeStyle(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_strokeStyle(QV4::SimpleCallContext *ctx);

    static QV4::Value method_get_lineCap(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_lineCap(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_lineJoin(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_lineJoin(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_lineWidth(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_lineWidth(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_miterLimit(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_miterLimit(QV4::SimpleCallContext *ctx);

    static QV4::Value method_get_shadowBlur(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_shadowBlur(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_shadowColor(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_shadowColor(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_shadowOffsetX(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_shadowOffsetX(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_shadowOffsetY(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_shadowOffsetY(QV4::SimpleCallContext *ctx);

    // should these two be on the proto?
    static QV4::Value method_get_path(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_path(QV4::SimpleCallContext *ctx);

    static QV4::Value method_get_font(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_font(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_textAlign(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_textAlign(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_textBaseline(QV4::SimpleCallContext *ctx);
    static QV4::Value method_set_textBaseline(QV4::SimpleCallContext *ctx);

protected:
    static void destroy(Managed *that)
    {
        static_cast<QQuickJSContext2D *>(that)->~QQuickJSContext2D();
    }
};

DEFINE_MANAGED_VTABLE(QQuickJSContext2D);


struct QQuickJSContext2DPrototype : public QV4::Object
{
    Q_MANAGED
public:
    QQuickJSContext2DPrototype(QV4::ExecutionEngine *engine)
        : QV4::Object(engine)
    {
        prototype = engine->objectPrototype;
        defineDefaultProperty(engine, QStringLiteral("quadraticCurveTo"), method_quadraticCurveTo, 0);
        defineDefaultProperty(engine, QStringLiteral("restore"), method_restore, 0);
        defineDefaultProperty(engine, QStringLiteral("moveTo"), method_moveTo, 0);
        defineDefaultProperty(engine, QStringLiteral("lineTo"), method_lineTo, 0);
        defineDefaultProperty(engine, QStringLiteral("caretBlinkRate"), method_caretBlinkRate, 0);
        defineDefaultProperty(engine, QStringLiteral("clip"), method_clip, 0);
        defineDefaultProperty(engine, QStringLiteral("setTransform"), method_setTransform, 0);
        defineDefaultProperty(engine, QStringLiteral("text"), method_text, 0);
        defineDefaultProperty(engine, QStringLiteral("roundedRect"), method_roundedRect, 0);
        defineDefaultProperty(engine, QStringLiteral("createPattern"), method_createPattern, 0);
        defineDefaultProperty(engine, QStringLiteral("stroke"), method_stroke, 0);
        defineDefaultProperty(engine, QStringLiteral("arc"), method_arc, 0);
        defineDefaultProperty(engine, QStringLiteral("createImageData"), method_createImageData, 0);
        defineDefaultProperty(engine, QStringLiteral("measureText"), method_measureText, 0);
        defineDefaultProperty(engine, QStringLiteral("ellipse"), method_ellipse, 0);
        defineDefaultProperty(engine, QStringLiteral("fill"), method_fill, 0);
        defineDefaultProperty(engine, QStringLiteral("save"), method_save, 0);
        defineDefaultProperty(engine, QStringLiteral("scale"), method_scale, 0);
        defineDefaultProperty(engine, QStringLiteral("drawImage"), method_drawImage, 0);
        defineDefaultProperty(engine, QStringLiteral("transform"), method_transform, 0);
        defineDefaultProperty(engine, QStringLiteral("fillText"), method_fillText, 0);
        defineDefaultProperty(engine, QStringLiteral("strokeText"), method_strokeText, 0);
        defineDefaultProperty(engine, QStringLiteral("translate"), method_translate, 0);
        defineDefaultProperty(engine, QStringLiteral("createRadialGradient"), method_createRadialGradient, 0);
        defineDefaultProperty(engine, QStringLiteral("shear"), method_shear, 0);
        defineDefaultProperty(engine, QStringLiteral("isPointInPath"), method_isPointInPath, 0);
        defineDefaultProperty(engine, QStringLiteral("bezierCurveTo"), method_bezierCurveTo, 0);
        defineDefaultProperty(engine, QStringLiteral("resetTransform"), method_resetTransform, 0);
        defineDefaultProperty(engine, QStringLiteral("arcTo"), method_arcTo, 0);
        defineDefaultProperty(engine, QStringLiteral("fillRect"), method_fillRect, 0);
        defineDefaultProperty(engine, QStringLiteral("createConicalGradient"), method_createConicalGradient, 0);
        defineDefaultProperty(engine, QStringLiteral("drawFocusRing"), method_drawFocusRing, 0);
        defineDefaultProperty(engine, QStringLiteral("beginPath"), method_beginPath, 0);
        defineDefaultProperty(engine, QStringLiteral("clearRect"), method_clearRect, 0);
        defineDefaultProperty(engine, QStringLiteral("rect"), method_rect, 0);
        defineDefaultProperty(engine, QStringLiteral("reset"), method_reset, 0);
        defineDefaultProperty(engine, QStringLiteral("rotate"), method_rotate, 0);
        defineDefaultProperty(engine, QStringLiteral("setCaretSelectionRect"), method_setCaretSelectionRect, 0);
        defineDefaultProperty(engine, QStringLiteral("putImageData"), method_putImageData, 0);
        defineDefaultProperty(engine, QStringLiteral("getImageData"), method_getImageData, 0);
        defineDefaultProperty(engine, QStringLiteral("createLinearGradient"), method_createLinearGradient, 0);
        defineDefaultProperty(engine, QStringLiteral("strokeRect"), method_strokeRect, 0);
        defineDefaultProperty(engine, QStringLiteral("closePath"), method_closePath, 0);
        defineAccessorProperty(engine, QStringLiteral("canvas"), QQuickJSContext2DPrototype::method_get_canvas, 0);
    }

    static QV4::Value method_get_canvas(QV4::SimpleCallContext *ctx);
    static QV4::Value method_restore(QV4::SimpleCallContext *ctx);
    static QV4::Value method_reset(QV4::SimpleCallContext *ctx);
    static QV4::Value method_save(QV4::SimpleCallContext *ctx);
    static QV4::Value method_rotate(QV4::SimpleCallContext *ctx);
    static QV4::Value method_scale(QV4::SimpleCallContext *ctx);
    static QV4::Value method_translate(QV4::SimpleCallContext *ctx);
    static QV4::Value method_setTransform(QV4::SimpleCallContext *ctx);
    static QV4::Value method_transform(QV4::SimpleCallContext *ctx);
    static QV4::Value method_resetTransform(QV4::SimpleCallContext *ctx);
    static QV4::Value method_shear(QV4::SimpleCallContext *ctx);
    static QV4::Value method_createLinearGradient(QV4::SimpleCallContext *ctx);
    static QV4::Value method_createRadialGradient(QV4::SimpleCallContext *ctx);
    static QV4::Value method_createConicalGradient(QV4::SimpleCallContext *ctx);
    static QV4::Value method_createPattern(QV4::SimpleCallContext *ctx);
    static QV4::Value method_clearRect(QV4::SimpleCallContext *ctx);
    static QV4::Value method_fillRect(QV4::SimpleCallContext *ctx);
    static QV4::Value method_strokeRect(QV4::SimpleCallContext *ctx);
    static QV4::Value method_arc(QV4::SimpleCallContext *ctx);
    static QV4::Value method_arcTo(QV4::SimpleCallContext *ctx);
    static QV4::Value method_beginPath(QV4::SimpleCallContext *ctx);
    static QV4::Value method_bezierCurveTo(QV4::SimpleCallContext *ctx);
    static QV4::Value method_clip(QV4::SimpleCallContext *ctx);
    static QV4::Value method_closePath(QV4::SimpleCallContext *ctx);
    static QV4::Value method_fill(QV4::SimpleCallContext *ctx);
    static QV4::Value method_lineTo(QV4::SimpleCallContext *ctx);
    static QV4::Value method_moveTo(QV4::SimpleCallContext *ctx);
    static QV4::Value method_quadraticCurveTo(QV4::SimpleCallContext *ctx);
    static QV4::Value method_rect(QV4::SimpleCallContext *ctx);
    static QV4::Value method_roundedRect(QV4::SimpleCallContext *ctx);
    static QV4::Value method_ellipse(QV4::SimpleCallContext *ctx);
    static QV4::Value method_text(QV4::SimpleCallContext *ctx);
    static QV4::Value method_stroke(QV4::SimpleCallContext *ctx);
    static QV4::Value method_isPointInPath(QV4::SimpleCallContext *ctx);
    static QV4::Value method_drawFocusRing(QV4::SimpleCallContext *ctx);
    static QV4::Value method_setCaretSelectionRect(QV4::SimpleCallContext *ctx);
    static QV4::Value method_caretBlinkRate(QV4::SimpleCallContext *ctx);
    static QV4::Value method_fillText(QV4::SimpleCallContext *ctx);
    static QV4::Value method_strokeText(QV4::SimpleCallContext *ctx);
    static QV4::Value method_measureText(QV4::SimpleCallContext *ctx);
    static QV4::Value method_drawImage(QV4::SimpleCallContext *ctx);
    static QV4::Value method_createImageData(QV4::SimpleCallContext *ctx);
    static QV4::Value method_getImageData(QV4::SimpleCallContext *ctx);
    static QV4::Value method_putImageData(QV4::SimpleCallContext *ctx);

};

DEFINE_MANAGED_VTABLE(QQuickJSContext2DPrototype);


class QQuickContext2DStyle : public QV4::Object
{
    Q_MANAGED
public:
    QQuickContext2DStyle(QV4::ExecutionEngine *e)
      : QV4::Object(e)
      , patternRepeatX(false)
      , patternRepeatY(false)
    {}
    QBrush brush;
    bool patternRepeatX:1;
    bool patternRepeatY:1;

    static QV4::Value gradient_proto_addColorStop(QV4::SimpleCallContext *ctx);
protected:
    static void destroy(Managed *that)
    {
        static_cast<QQuickContext2DStyle *>(that)->~QQuickContext2DStyle();
    }
};

DEFINE_MANAGED_VTABLE(QQuickContext2DStyle);

QImage qt_image_convolute_filter(const QImage& src, const QVector<qreal>& weights, int radius = 0)
{
    int sides = radius ? radius : qRound(qSqrt(weights.size()));
    int half = qFloor(sides/2);

    QImage dst = QImage(src.size(), src.format());
    int w = src.width();
    int h = src.height();
    for (int y = 0; y < dst.height(); ++y) {
      QRgb *dr = (QRgb*)dst.scanLine(y);
      for (int x = 0; x < dst.width(); ++x) {
          unsigned char* dRgb = ((unsigned char*)&dr[x]);
          unsigned char red=0, green=0, blue=0, alpha=0;
          int sy = y;
          int sx = x;

          for (int cy=0; cy<sides; cy++) {
             for (int cx=0; cx<sides; cx++) {
               int scy = sy + cy - half;
               int scx = sx + cx - half;
               if (scy >= 0 && scy < w && scx >= 0 && scx < h) {
                  const QRgb *sr = (const QRgb*)(src.constScanLine(scy));
                  const unsigned char* sRgb = ((const unsigned char*)&sr[scx]);
                  qreal wt = radius ? weights[0] : weights[cy*sides+cx];
                  red += sRgb[0] * wt;
                  green += sRgb[1] * wt;
                  blue += sRgb[2] * wt;
                  alpha += sRgb[3] * wt;
               }
             }
          }
          dRgb[0] = red;
          dRgb[1] = green;
          dRgb[2] = blue;
          dRgb[3] = alpha;
      }
    }
    return dst;
}

void qt_image_boxblur(QImage& image, int radius, bool quality)
{
    int passes = quality? 3: 1;
    for (int i=0; i < passes; i++) {
        image = qt_image_convolute_filter(image, QVector<qreal>() << 1.0/(radius * radius * 1.0), radius);
    }
}

static QPainter::CompositionMode qt_composite_mode_from_string(const QString &compositeOperator)
{
    if (compositeOperator == QStringLiteral("source-over")) {
        return QPainter::CompositionMode_SourceOver;
    } else if (compositeOperator == QStringLiteral("source-out")) {
        return QPainter::CompositionMode_SourceOut;
    } else if (compositeOperator == QStringLiteral("source-in")) {
        return QPainter::CompositionMode_SourceIn;
    } else if (compositeOperator == QStringLiteral("source-atop")) {
        return QPainter::CompositionMode_SourceAtop;
    } else if (compositeOperator == QStringLiteral("destination-atop")) {
        return QPainter::CompositionMode_DestinationAtop;
    } else if (compositeOperator == QStringLiteral("destination-in")) {
        return QPainter::CompositionMode_DestinationIn;
    } else if (compositeOperator == QStringLiteral("destination-out")) {
        return QPainter::CompositionMode_DestinationOut;
    } else if (compositeOperator == QStringLiteral("destination-over")) {
        return QPainter::CompositionMode_DestinationOver;
    } else if (compositeOperator == QStringLiteral("lighter")) {
        return QPainter::CompositionMode_Lighten;
    } else if (compositeOperator == QStringLiteral("copy")) {
        return QPainter::CompositionMode_Source;
    } else if (compositeOperator == QStringLiteral("xor")) {
        return QPainter::CompositionMode_Xor;
    } else if (compositeOperator == QStringLiteral("qt-clear")) {
        return QPainter::CompositionMode_Clear;
    } else if (compositeOperator == QStringLiteral("qt-destination")) {
        return QPainter::CompositionMode_Destination;
    } else if (compositeOperator == QStringLiteral("qt-multiply")) {
        return QPainter::CompositionMode_Multiply;
    } else if (compositeOperator == QStringLiteral("qt-screen")) {
        return QPainter::CompositionMode_Screen;
    } else if (compositeOperator == QStringLiteral("qt-overlay")) {
        return QPainter::CompositionMode_Overlay;
    } else if (compositeOperator == QStringLiteral("qt-darken")) {
        return QPainter::CompositionMode_Darken;
    } else if (compositeOperator == QStringLiteral("qt-lighten")) {
        return QPainter::CompositionMode_Lighten;
    } else if (compositeOperator == QStringLiteral("qt-color-dodge")) {
        return QPainter::CompositionMode_ColorDodge;
    } else if (compositeOperator == QStringLiteral("qt-color-burn")) {
        return QPainter::CompositionMode_ColorBurn;
    } else if (compositeOperator == QStringLiteral("qt-hard-light")) {
        return QPainter::CompositionMode_HardLight;
    } else if (compositeOperator == QStringLiteral("qt-soft-light")) {
        return QPainter::CompositionMode_SoftLight;
    } else if (compositeOperator == QStringLiteral("qt-difference")) {
        return QPainter::CompositionMode_Difference;
    } else if (compositeOperator == QStringLiteral("qt-exclusion")) {
        return QPainter::CompositionMode_Exclusion;
    }
    return QPainter::CompositionMode_SourceOver;
}

static QString qt_composite_mode_to_string(QPainter::CompositionMode op)
{
    switch (op) {
    case QPainter::CompositionMode_SourceOver:
        return QStringLiteral("source-over");
    case QPainter::CompositionMode_DestinationOver:
        return QStringLiteral("destination-over");
    case QPainter::CompositionMode_Clear:
        return QStringLiteral("qt-clear");
    case QPainter::CompositionMode_Source:
        return QStringLiteral("copy");
    case QPainter::CompositionMode_Destination:
        return QStringLiteral("qt-destination");
    case QPainter::CompositionMode_SourceIn:
        return QStringLiteral("source-in");
    case QPainter::CompositionMode_DestinationIn:
        return QStringLiteral("destination-in");
    case QPainter::CompositionMode_SourceOut:
        return QStringLiteral("source-out");
    case QPainter::CompositionMode_DestinationOut:
        return QStringLiteral("destination-out");
    case QPainter::CompositionMode_SourceAtop:
        return QStringLiteral("source-atop");
    case QPainter::CompositionMode_DestinationAtop:
        return QStringLiteral("destination-atop");
    case QPainter::CompositionMode_Xor:
        return QStringLiteral("xor");
    case QPainter::CompositionMode_Plus:
        return QStringLiteral("plus");
    case QPainter::CompositionMode_Multiply:
        return QStringLiteral("qt-multiply");
    case QPainter::CompositionMode_Screen:
        return QStringLiteral("qt-screen");
    case QPainter::CompositionMode_Overlay:
        return QStringLiteral("qt-overlay");
    case QPainter::CompositionMode_Darken:
        return QStringLiteral("qt-darken");
    case QPainter::CompositionMode_Lighten:
        return QStringLiteral("lighter");
    case QPainter::CompositionMode_ColorDodge:
        return QStringLiteral("qt-color-dodge");
    case QPainter::CompositionMode_ColorBurn:
        return QStringLiteral("qt-color-burn");
    case QPainter::CompositionMode_HardLight:
        return QStringLiteral("qt-hard-light");
    case QPainter::CompositionMode_SoftLight:
        return QStringLiteral("qt-soft-light");
    case QPainter::CompositionMode_Difference:
        return QStringLiteral("qt-difference");
    case QPainter::CompositionMode_Exclusion:
        return QStringLiteral("qt-exclusion");
    default:
        break;
    }
    return QString();
}

struct QQuickJSContext2DPixelData : public QV4::Object
{
    Q_MANAGED
    QQuickJSContext2DPixelData(QV4::ExecutionEngine *engine)
        : QV4::Object(engine)
    {
        vtbl = &static_vtbl;
    }

    static void destroy(QV4::Managed *that) {
        static_cast<QQuickJSContext2DPixelData *>(that)->~QQuickJSContext2DPixelData();
    }
    static QV4::Value getIndexed(QV4::Managed *m, uint index, bool *hasProperty);
    static void putIndexed(QV4::Managed *m, QV4::ExecutionContext *ctx, uint index, const QV4::Value &value);

    static QV4::Value proto_get_length(QV4::SimpleCallContext *ctx);

    QImage image;
};

DEFINE_MANAGED_VTABLE(QQuickJSContext2DPixelData);

struct QQuickJSContext2DImageData : public QV4::Object
{
    Q_MANAGED
    QQuickJSContext2DImageData(QV4::ExecutionEngine *engine)
        : QV4::Object(engine)
    {
        vtbl = &static_vtbl;

        defineAccessorProperty(engine, QStringLiteral("width"), method_get_width, 0);
        defineAccessorProperty(engine, QStringLiteral("height"), method_get_height, 0);
        defineAccessorProperty(engine, QStringLiteral("data"), method_get_data, 0);
    }

    static QV4::Value method_get_width(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_height(QV4::SimpleCallContext *ctx);
    static QV4::Value method_get_data(QV4::SimpleCallContext *ctx);

    static void markObjects(Managed *that) {
        static_cast<QQuickJSContext2DImageData *>(that)->pixelData.mark();
    }



    QV4::Value pixelData;
};

DEFINE_MANAGED_VTABLE(QQuickJSContext2DImageData);

static QV4::Value qt_create_image_data(qreal w, qreal h, QV8Engine* engine, const QImage& image)
{
    QQuickContext2DEngineData *ed = engineData(engine);
    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
    QQuickJSContext2DPixelData *pixelData = new (v4->memoryManager) QQuickJSContext2DPixelData(v4);
    pixelData->prototype = ed->pixelArrayProto.value().asObject();

    if (image.isNull()) {
        pixelData->image = QImage(w, h, QImage::Format_ARGB32);
        pixelData->image.fill(0x00000000);
    } else {
        Q_ASSERT(image.width() == w && image.height() == h);
        pixelData->image = image.format() == QImage::Format_ARGB32 ? image : image.convertToFormat(QImage::Format_ARGB32);
    }

    QQuickJSContext2DImageData *imageData = new (v4->memoryManager) QQuickJSContext2DImageData(v4);
    imageData->pixelData = QV4::Value::fromObject(pixelData);
    return QV4::Value::fromObject(imageData);
}

//static script functions

/*!
    \qmlproperty QtQuick2::Canvas QtQuick2::Context2D::canvas
     Holds the canvas item that the context paints on.

     This property is read only.
*/
QV4::Value QQuickJSContext2DPrototype::method_get_canvas(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    return QV4::QObjectWrapper::wrap(ctx->engine, r->context->canvas());
}

/*!
    \qmlmethod object QtQuick2::Context2D::restore()
    Pops the top state on the stack, restoring the context to that state.

    \sa save()
*/
QV4::Value QQuickJSContext2DPrototype::method_restore(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    r->context->popState();
    return ctx->thisObject;
}

/*!
    \qmlmethod object QtQuick2::Context2D::reset()
    Resets the context state and properties to the default values.
*/
QV4::Value QQuickJSContext2DPrototype::method_reset(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    r->context->reset();

    return ctx->thisObject;
}

/*!
    \qmlmethod object QtQuick2::Context2D::save()
    Pushes the current state onto the state stack.

    Before changing any state attributes, you should save the current state
    for future reference. The context maintains a stack of drawing states.
    Each state consists of the current transformation matrix, clipping region,
    and values of the following attributes:
    \list
    \li strokeStyle
    \li fillStyle
    \li fillRule
    \li globalAlpha
    \li lineWidth
    \li lineCap
    \li lineJoin
    \li miterLimit
    \li shadowOffsetX
    \li shadowOffsetY
    \li shadowBlur
    \li shadowColor
    \li globalCompositeOperation
    \li \l font
    \li textAlign
    \li textBaseline
    \endlist

    The current path is NOT part of the drawing state. The path can be reset by
    invoking the beginPath() method.
*/
QV4::Value QQuickJSContext2DPrototype::method_save(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    r->context->pushState();

    return ctx->thisObject;
}

// transformations
/*!
    \qmlmethod object QtQuick2::Context2D::rotate(real angle)
    Rotate the canvas around the current origin by \c angle in radians and clockwise direction.
    \code
    ctx.rotate(Math.PI/2);
    \endcode
    \image qml-item-canvas-rotate.png

    The rotation transformation matrix is as follows:

    \image qml-item-canvas-math-rotate.png

    where the \c angle of rotation is in radians.

*/
QV4::Value QQuickJSContext2DPrototype::method_rotate(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 1)
        r->context->rotate(ctx->arguments[0].toNumber());
    return ctx->thisObject;
}

/*!
    \qmlmethod object QtQuick2::Context2D::scale(real x, real y)
    Increases or decreases the size of each unit in the canvas grid by multiplying the scale factors
    to the current tranform matrix.
    Where \c x is the scale factor in the horizontal direction and \c y is the scale factor in the
    vertical direction.
    The following code doubles the horizontal size of an object drawn on the canvas and half its
    vertical size:
    \code
    ctx.scale(2.0, 0.5);
    \endcode
    \image qml-item-canvas-scale.png

*/
QV4::Value QQuickJSContext2DPrototype::method_scale(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    if (ctx->argumentCount == 2)
        r->context->scale(ctx->arguments[0].toNumber(), ctx->arguments[1].toNumber());
    return ctx->thisObject;
}

/*!
    \qmlmethod object QtQuick2::Context2D::setTransform(real a, real b, real c, real d, real e, real f)
    Changes the transformation matrix to the matrix given by the arguments as described below.

    Modifying the transformation matrix directly enables you to perform scaling,
    rotating, and translating transformations in a single step.

    Each point on the canvas is multiplied by the matrix before anything is
    drawn. The \l{HTML5 Canvas API} defines the transformation matrix as:

    \image qml-item-canvas-math.png
    where:
    \list
    \li \c{a} is the scale factor in the horizontal (x) direction
    \image qml-item-canvas-scalex.png
    \li \c{c} is the skew factor in the x direction
    \image qml-item-canvas-canvas-skewx.png
    \li \c{e} is the translation in the x direction
    \image qml-item-canvas-canvas-translate.png
    \li \c{b} is the skew factor in the y (vertical) direction
    \image qml-item-canvas-canvas-skewy.png
    \li \c{d} is the scale factor in the y direction
    \image qml-item-canvas-canvas-scaley.png
    \li \c{f} is the translation in the y direction
    \image qml-item-canvas-canvas-translatey.png
    \li the last row remains constant
    \endlist
    The scale factors and skew factors are multiples; \c{e} and \c{f} are
    coordinate space units, just like the units in the translate(x,y)
    method.

    \sa transform()
*/
QV4::Value QQuickJSContext2DPrototype::method_setTransform(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    if (ctx->argumentCount == 6)
        r->context->setTransform( ctx->arguments[0].toNumber()
                                                        , ctx->arguments[1].toNumber()
                                                        , ctx->arguments[2].toNumber()
                                                        , ctx->arguments[3].toNumber()
                                                        , ctx->arguments[4].toNumber()
                                                        , ctx->arguments[5].toNumber());

    return ctx->thisObject;
}

/*!
    \qmlmethod object QtQuick2::Context2D::transform(real a, real b, real c, real d, real e, real f)
    This method is very similar to setTransform(), but instead of replacing the old
    tranform matrix, this method applies the given tranform matrix to the current matrix by mulitplying to it.

    The setTransform(a, b, c, d, e, f) method actually resets the current transform to the identity matrix,
    and then invokes the transform(a, b, c, d, e, f) method with the same arguments.

    \sa setTransform()
*/
QV4::Value QQuickJSContext2DPrototype::method_transform(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    if (ctx->argumentCount == 6)
        r->context->transform( ctx->arguments[0].toNumber()
                                                  , ctx->arguments[1].toNumber()
                                                  , ctx->arguments[2].toNumber()
                                                  , ctx->arguments[3].toNumber()
                                                  , ctx->arguments[4].toNumber()
                                                  , ctx->arguments[5].toNumber());

    return ctx->thisObject;
}

/*!
    \qmlmethod object QtQuick2::Context2D::translate(real x, real y)
    Translates the origin of the canvas to point (\c x, \c y).

    \c x is the horizontal distance that the origin is translated, in coordinate space units,
    \c y is the vertical distance that the origin is translated, in coordinate space units.
    Translating the origin enables you to draw patterns of different objects on the canvas
    without having to measure the coordinates manually for each shape.
*/
QV4::Value QQuickJSContext2DPrototype::method_translate(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    if (ctx->argumentCount == 2)
            r->context->translate(ctx->arguments[0].toNumber(), ctx->arguments[1].toNumber());
    return ctx->thisObject;
}


/*!
    \qmlmethod object QtQuick2::Context2D::resetTransform()
    Reset the transformation matrix to default value.

    \sa transform(), setTransform(), reset()
*/
QV4::Value QQuickJSContext2DPrototype::method_resetTransform(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    r->context->setTransform(1, 0, 0, 1, 0, 0);

    return ctx->thisObject;
}


/*!
    \qmlmethod object QtQuick2::Context2D::shear(real sh, real sv )
    Shear the transformation matrix with \a sh in horizontal direction and \a sv in vertical direction.
*/
QV4::Value QQuickJSContext2DPrototype::method_shear(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 2)
            r->context->shear(ctx->arguments[0].toNumber(), ctx->arguments[1].toNumber());

    return ctx->thisObject;
}
// compositing

/*!
    \qmlproperty real QtQuick2::Context2D::globalAlpha
     Holds the current alpha value applied to rendering operations.
     The value must be in the range from 0.0 (fully transparent) to 1.0 (fully opque).
     The default value is 1.0.
*/
QV4::Value QQuickJSContext2D::method_get_globalAlpha(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    return QV4::Value::fromDouble(r->context->state.globalAlpha);
}

QV4::Value QQuickJSContext2D::method_set_globalAlpha(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    double globalAlpha = ctx->argument(0).toNumber();

    if (!qIsFinite(globalAlpha))
        return QV4::Value::undefinedValue();

    if (globalAlpha >= 0.0 && globalAlpha <= 1.0 && r->context->state.globalAlpha != globalAlpha) {
        r->context->state.globalAlpha = globalAlpha;
        r->context->buffer()->setGlobalAlpha(r->context->state.globalAlpha);
    }
    return QV4::Value::undefinedValue();
}

/*!
    \qmlproperty string QtQuick2::Context2D::globalCompositeOperation
     Holds the current the current composition operation, from the list below:
     \list
     \li source-atop      - A atop B. Display the source image wherever both images are opaque.
                           Display the destination image wherever the destination image is opaque but the source image is transparent.
                           Display transparency elsewhere.
     \li source-in        - A in B. Display the source image wherever both the source image and destination image are opaque.
                           Display transparency elsewhere.
     \li source-out       - A out B. Display the source image wherever the source image is opaque and the destination image is transparent.
                           Display transparency elsewhere.
     \li source-over      - (default) A over B. Display the source image wherever the source image is opaque.
                           Display the destination image elsewhere.
     \li destination-atop - B atop A. Same as source-atop but using the destination image instead of the source image and vice versa.
     \li destination-in   - B in A. Same as source-in but using the destination image instead of the source image and vice versa.
     \li destination-out  - B out A. Same as source-out but using the destination image instead of the source image and vice versa.
     \li destination-over - B over A. Same as source-over but using the destination image instead of the source image and vice versa.
     \li lighter          - A plus B. Display the sum of the source image and destination image, with color values approaching 255 (100%) as a limit.
     \li copy             - A (B is ignored). Display the source image instead of the destination image.
     \li xor              - A xor B. Exclusive OR of the source image and destination image.
     \endlist

     Additionally, this property also accepts the compositon modes listed in QPainter::CompositionMode. According to the W3C standard, these
     extension composition modes are provided as "vendorName-operationName" syntax, for example: QPainter::CompositionMode_Exclusion is provided as
     "qt-exclusion".
*/
QV4::Value QQuickJSContext2D::method_get_globalCompositeOperation(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    return QV4::Value::fromString(ctx->engine->newString(qt_composite_mode_to_string(r->context->state.globalCompositeOperation)));
}

QV4::Value QQuickJSContext2D::method_set_globalCompositeOperation(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    QString mode = ctx->argument(0).toQString();
    QPainter::CompositionMode cm = qt_composite_mode_from_string(mode);
    if (cm == QPainter::CompositionMode_SourceOver && mode != QStringLiteral("source-over"))
        return QV4::Value::undefinedValue();

    if (cm != r->context->state.globalCompositeOperation) {
        r->context->state.globalCompositeOperation = cm;
        r->context->buffer()->setGlobalCompositeOperation(cm);
    }
    return QV4::Value::undefinedValue();
}

// colors and styles
/*!
    \qmlproperty variant QtQuick2::Context2D::fillStyle
     Holds the current style used for filling shapes.
     The style can be either a string containing a CSS color, a CanvasGradient or CanvasPattern object. Invalid values are ignored.
     This property accepts several color syntaxes:
     \list
     \li 'rgb(red, green, blue)' - for example: 'rgb(255, 100, 55)' or 'rgb(100%, 70%, 30%)'
     \li 'rgba(red, green, blue, alpha)' - for example: 'rgb(255, 100, 55, 1.0)' or 'rgb(100%, 70%, 30%, 0.5)'
     \li 'hsl(hue, saturation, lightness)'
     \li 'hsla(hue, saturation, lightness, alpha)'
     \li '#RRGGBB' - for example: '#00FFCC'
     \li Qt.rgba(red, green, blue, alpha) - for example: Qt.rgba(0.3, 0.7, 1, 1.0)
     \endlist
     If the \c fillStyle or \l strokeStyle is assigned many times in a loop, the last Qt.rgba() syntax should be chosen, as it has the
     best performance, because it's already a valid QColor value, does not need to be parsed everytime.

     The default value is  '#000000'.
     \sa createLinearGradient()
     \sa createRadialGradient()
     \sa createPattern()
     \sa strokeStyle
 */
QV4::Value QQuickJSContext2D::method_get_fillStyle(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    QColor color = r->context->state.fillStyle.color();
    if (color.isValid()) {
        if (color.alpha() == 255)
            return QV4::Value::fromString(ctx->engine->newString(color.name()));
        QString alphaString = QString::number(color.alphaF(), 'f');
        while (alphaString.endsWith(QLatin1Char('0')))
            alphaString.chop(1);
        if (alphaString.endsWith(QLatin1Char('.')))
            alphaString += QLatin1Char('0');
        QString str = QString::fromLatin1("rgba(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(alphaString);
        return QV4::Value::fromString(ctx->engine->newString(str));
    }
    return r->context->m_fillStyle.value();
}

QV4::Value QQuickJSContext2D::method_set_fillStyle(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    QV4::Value value = ctx->argument(0);
    QV8Engine *engine = ctx->engine->v8Engine;

   if (value.asObject()) {
       QColor color = engine->toVariant(value, qMetaTypeId<QColor>()).value<QColor>();
       if (color.isValid()) {
           r->context->state.fillStyle = color;
           r->context->buffer()->setFillStyle(color);
           r->context->m_fillStyle = value;
       } else {
           QQuickContext2DStyle *style = value.as<QQuickContext2DStyle>();
           if (style && style->brush != r->context->state.fillStyle) {
               r->context->state.fillStyle = style->brush;
               r->context->buffer()->setFillStyle(style->brush, style->patternRepeatX, style->patternRepeatY);
               r->context->m_fillStyle = value;
               r->context->state.fillPatternRepeatX = style->patternRepeatX;
               r->context->state.fillPatternRepeatY = style->patternRepeatY;
           }
       }
   } else if (value.isString()) {
       QColor color = qt_color_from_string(value);
       if (color.isValid() && r->context->state.fillStyle != QBrush(color)) {
            r->context->state.fillStyle = QBrush(color);
            r->context->buffer()->setFillStyle(r->context->state.fillStyle);
            r->context->m_fillStyle = value;
       }
   }
   return QV4::Value::undefinedValue();
}
/*!
    \qmlproperty enumeration QtQuick2::Context2D::fillRule
     Holds the current fill rule used for filling shapes. The following fill rules supported:
     \list
     \li Qt.OddEvenFill
     \li Qt.WindingFill
     \endlist
     Note: Unlike the QPainterPath, the Canvas API uses the winding fill as the default fill rule.
     The fillRule property is part of the context rendering state.

     \sa fillStyle
 */
QV4::Value QQuickJSContext2D::method_get_fillRule(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    QV8Engine *engine = ctx->engine->v8Engine;
    return engine->fromVariant(r->context->state.fillRule);
}

QV4::Value QQuickJSContext2D::method_set_fillRule(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    QV4::Value value = ctx->argument(0);

    if ((value.isString() && value.toQString() == QStringLiteral("WindingFill"))
        || (value.isInt32() && value.integerValue() == Qt::WindingFill)) {
        r->context->state.fillRule = Qt::WindingFill;
    } else if ((value.isString() && value.toQString() == QStringLiteral("OddEvenFill"))
               || (value.isInt32() && value.integerValue() == Qt::OddEvenFill)) {
        r->context->state.fillRule = Qt::OddEvenFill;
    } else {
        //error
    }
    r->context->m_path.setFillRule(r->context->state.fillRule);
    return QV4::Value::undefinedValue();
}
/*!
    \qmlproperty variant QtQuick2::Context2D::strokeStyle
     Holds the current color or style to use for the lines around shapes,
     The style can be either a string containing a CSS color, a CanvasGradient or CanvasPattern object.
     Invalid values are ignored.

     The default value is  '#000000'.

     \sa createLinearGradient()
     \sa createRadialGradient()
     \sa createPattern()
     \sa fillStyle
 */
QV4::Value QQuickJSContext2D::method_get_strokeStyle(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    QColor color = r->context->state.strokeStyle.color();
    if (color.isValid()) {
        if (color.alpha() == 255)
            return QV4::Value::fromString(ctx->engine->newString(color.name()));
        QString alphaString = QString::number(color.alphaF(), 'f');
        while (alphaString.endsWith(QLatin1Char('0')))
            alphaString.chop(1);
        if (alphaString.endsWith(QLatin1Char('.')))
            alphaString += QLatin1Char('0');
        QString str = QString::fromLatin1("rgba(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(alphaString);
        return QV4::Value::fromString(ctx->engine->newString(str));
    }
    return r->context->m_strokeStyle.value();
}

QV4::Value QQuickJSContext2D::method_set_strokeStyle(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    QV8Engine *engine = ctx->engine->v8Engine;
    QV4::Value value = ctx->argument(0);

    if (value.asObject()) {
        QColor color = engine->toVariant(value, qMetaTypeId<QColor>()).value<QColor>();
        if (color.isValid()) {
            r->context->state.fillStyle = color;
            r->context->buffer()->setStrokeStyle(color);
            r->context->m_strokeStyle = value;
        } else {
            QQuickContext2DStyle *style = value.as<QQuickContext2DStyle>();
            if (style && style->brush != r->context->state.strokeStyle) {
                r->context->state.strokeStyle = style->brush;
                r->context->buffer()->setStrokeStyle(style->brush, style->patternRepeatX, style->patternRepeatY);
                r->context->m_strokeStyle = value;
                r->context->state.strokePatternRepeatX = style->patternRepeatX;
                r->context->state.strokePatternRepeatY = style->patternRepeatY;

            }
        }
    } else if (value.isString()) {
        QColor color = qt_color_from_string(value);
        if (color.isValid() && r->context->state.strokeStyle != QBrush(color)) {
             r->context->state.strokeStyle = QBrush(color);
             r->context->buffer()->setStrokeStyle(r->context->state.strokeStyle);
             r->context->m_strokeStyle = value;
        }
    }
    return QV4::Value::undefinedValue();
}

/*!
  \qmlmethod object QtQuick2::Context2D::createLinearGradient(real x0, real y0, real x1, real y1)
   Returns a CanvasGradient object that represents a linear gradient that transitions the color along a line between
   the start point (\a x0, \a y0) and the end point (\a x1, \a y1).

   A gradient is a smooth transition between colors. There are two types of gradients: linear and radial.
   Gradients must have two or more color stops, representing color shifts positioned from 0 to 1 between
   to the gradient's starting and end points or circles.

    \sa CanvasGradient::addColorStop()
    \sa createRadialGradient()
    \sa createConicalGradient()
    \sa createPattern()
    \sa fillStyle
    \sa strokeStyle
  */

QV4::Value QQuickJSContext2DPrototype::method_createLinearGradient(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    QV8Engine *engine = ctx->engine->v8Engine;
    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);

    if (ctx->argumentCount == 4) {
        qreal x0 = ctx->arguments[0].toNumber();
        qreal y0 = ctx->arguments[1].toNumber();
        qreal x1 = ctx->arguments[2].toNumber();
        qreal y1 = ctx->arguments[3].toNumber();

        if (!qIsFinite(x0)
         || !qIsFinite(y0)
         || !qIsFinite(x1)
         || !qIsFinite(y1)) {
            V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "createLinearGradient(): Incorrect arguments")
        }
        QQuickContext2DEngineData *ed = engineData(engine);

        QQuickContext2DStyle *gradient = new (v4->memoryManager) QQuickContext2DStyle(v4);
        gradient->prototype = ed->gradientProto.value().asObject();
        gradient->brush = QLinearGradient(x0, y0, x1, y1);
        return QV4::Value::fromObject(gradient);
    }

    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::createRadialGradient(real x0, real y0, real r0, real x1, real y1, real r1)
   Returns a CanvasGradient object that represents a radial gradient that paints along the cone given by the start circle with
   origin (x0, y0) and radius r0, and the end circle with origin (x1, y1) and radius r1.

    \sa CanvasGradient::addColorStop()
    \sa createLinearGradient()
    \sa createConicalGradient()
    \sa createPattern()
    \sa fillStyle
    \sa strokeStyle
  */

QV4::Value QQuickJSContext2DPrototype::method_createRadialGradient(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    QV8Engine *engine = ctx->engine->v8Engine;
    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);

    if (ctx->argumentCount == 6) {
        qreal x0 = ctx->arguments[0].toNumber();
        qreal y0 = ctx->arguments[1].toNumber();
        qreal r0 = ctx->arguments[2].toNumber();
        qreal x1 = ctx->arguments[3].toNumber();
        qreal y1 = ctx->arguments[4].toNumber();
        qreal r1 = ctx->arguments[5].toNumber();

        if (!qIsFinite(x0)
         || !qIsFinite(y0)
         || !qIsFinite(x1)
         || !qIsFinite(r0)
         || !qIsFinite(r1)
         || !qIsFinite(y1)) {
            V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "createRadialGradient(): Incorrect arguments")
        }

        if (r0 < 0 || r1 < 0)
            V4THROW_DOM(DOMEXCEPTION_INDEX_SIZE_ERR, "createRadialGradient(): Incorrect arguments")

        QQuickContext2DEngineData *ed = engineData(engine);

        QQuickContext2DStyle *gradient = new (v4->memoryManager) QQuickContext2DStyle(v4);
        gradient->prototype = ed->gradientProto.value().asObject();
        gradient->brush = QRadialGradient(QPointF(x1, y1), r0+r1, QPointF(x0, y0));
        return QV4::Value::fromObject(gradient);
    }

    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::createConicalGradient(real x, real y, real angle)
   Returns a CanvasGradient object that represents a conical gradient that interpolate colors counter-clockwise around a center point (\c x, \c y)
   with start angle \c angle in units of radians.

    \sa CanvasGradient::addColorStop()
    \sa createLinearGradient()
    \sa createRadialGradient()
    \sa createPattern()
    \sa fillStyle
    \sa strokeStyle
  */

QV4::Value QQuickJSContext2DPrototype::method_createConicalGradient(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    QV8Engine *engine = ctx->engine->v8Engine;
    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);

    if (ctx->argumentCount == 6) {
        qreal x = ctx->arguments[0].toNumber();
        qreal y = ctx->arguments[1].toNumber();
        qreal angle = DEGREES(ctx->arguments[2].toNumber());
        if (!qIsFinite(x) || !qIsFinite(y)) {
            V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "createConicalGradient(): Incorrect arguments");
        }

        if (!qIsFinite(angle)) {
            V4THROW_DOM(DOMEXCEPTION_INDEX_SIZE_ERR, "createConicalGradient(): Incorrect arguments");
        }

        QQuickContext2DEngineData *ed = engineData(engine);

        QQuickContext2DStyle *gradient = new (v4->memoryManager) QQuickContext2DStyle(v4);
        gradient->prototype = ed->gradientProto.value().asObject();
        gradient->brush = QConicalGradient(x, y, angle);
        return QV4::Value::fromObject(gradient);
    }

    return ctx->thisObject;
}
/*!
  \qmlmethod variant QtQuick2::Context2D::createPattern(color color, enumeration patternMode)
  This is a overload function.
  Returns a CanvasPattern object that uses the given \a color and \a patternMode.
  The valid pattern modes are:
    \list
    \li Qt.SolidPattern
    \li Qt.Dense1Pattern
    \li Qt.Dense2Pattern
    \li Qt.Dense3Pattern
    \li Qt.Dense4Pattern
    \li Qt.Dense5Pattern
    \li Qt.Dense6Pattern
    \li Qt.Dense7Pattern
    \li Qt.HorPattern
    \li Qt.VerPattern
    \li Qt.CrossPattern
    \li Qt.BDiagPattern
    \li Qt.FDiagPattern
    \li Qt.DiagCrossPattern
\endlist
    \sa Qt::BrushStyle
 */
/*!
  \qmlmethod variant QtQuick2::Context2D::createPattern(Image image, string repetition)
  Returns a CanvasPattern object that uses the given image and repeats in the direction(s) given by the repetition argument.

  The \a image parameter must be a valid Image item, a valid CanvasImageData object or loaded image url, if there is no image data, throws an INVALID_STATE_ERR exception.

  The allowed values for \a repetition are:

  \list
  \li "repeat"    - both directions
  \li "repeat-x   - horizontal only
  \li "repeat-y"  - vertical only
  \li "no-repeat" - neither
  \endlist

  If the repetition argument is empty or null, the value "repeat" is used.

  \sa strokeStyle
  \sa fillStyle
  */
QV4::Value QQuickJSContext2DPrototype::method_createPattern(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    QV8Engine *engine = ctx->engine->v8Engine;
    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);

    if (ctx->argumentCount == 2) {
        QQuickContext2DStyle *pattern = new (v4->memoryManager) QQuickContext2DStyle(v4);

        QColor color = engine->toVariant(ctx->arguments[0], qMetaTypeId<QColor>()).value<QColor>();
        if (color.isValid()) {
            int patternMode = ctx->arguments[1].toInt32();
            Qt::BrushStyle style = Qt::SolidPattern;
            if (patternMode >= 0 && patternMode < Qt::LinearGradientPattern) {
                style = static_cast<Qt::BrushStyle>(patternMode);
            }
            pattern->brush = QBrush(color, style);
        } else {
            QImage patternTexture;

            if (QV4::Object *o = ctx->arguments[0].asObject()) {
                QQuickJSContext2DPixelData *pixelData = o->get(ctx->engine->newString(QStringLiteral("data"))).as<QQuickJSContext2DPixelData>();
                if (pixelData) {
                    patternTexture = pixelData->image;
                }
            } else {
                patternTexture = r->context->createPixmap(QUrl(ctx->arguments[0].toQString()))->image();
            }

            if (!patternTexture.isNull()) {
                pattern->brush.setTextureImage(patternTexture);

                QString repetition = ctx->arguments[1].toQString();
                if (repetition == QStringLiteral("repeat") || repetition.isEmpty()) {
                    pattern->patternRepeatX = true;
                    pattern->patternRepeatY = true;
                } else if (repetition == QStringLiteral("repeat-x")) {
                    pattern->patternRepeatX = true;
                } else if (repetition == QStringLiteral("repeat-y")) {
                    pattern->patternRepeatY = true;
                } else if (repetition == QStringLiteral("no-repeat")) {
                    pattern->patternRepeatY = false;
                    pattern->patternRepeatY = false;
                } else {
                    //TODO: exception: SYNTAX_ERR
                }

            }
        }

        return QV4::Value::fromObject(pattern);

    }
    return QV4::Value::undefinedValue();
}

// line styles
/*!
    \qmlproperty string QtQuick2::Context2D::lineCap
     Holds the current line cap style.
     The possible line cap styles are:
    \list
    \li butt - the end of each line has a flat edge perpendicular to the direction of the line, this is the default line cap value.
    \li round - a semi-circle with the diameter equal to the width of the line must then be added on to the end of the line.
    \li square - a rectangle with the length of the line width and the width of half the line width, placed flat against the edge perpendicular to the direction of the line.
    \endlist
    Other values are ignored.
*/
QV4::Value QQuickJSContext2D::method_get_lineCap(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    switch (r->context->state.lineCap) {
    case Qt::RoundCap:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("round")));
    case Qt::SquareCap:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("square")));
    case Qt::FlatCap:
    default:
        break;
    }
    return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("butt")));
}

QV4::Value QQuickJSContext2D::method_set_lineCap(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    QString lineCap = ctx->argument(0).toQString();
    Qt::PenCapStyle cap;
    if (lineCap == QStringLiteral("round"))
        cap = Qt::RoundCap;
    else if (lineCap == QStringLiteral("butt"))
        cap = Qt::FlatCap;
    else if (lineCap == QStringLiteral("square"))
        cap = Qt::SquareCap;
    else
        return QV4::Value::undefinedValue();

    if (cap != r->context->state.lineCap) {
        r->context->state.lineCap = cap;
        r->context->buffer()->setLineCap(cap);
    }
    return QV4::Value::undefinedValue();
}

/*!
    \qmlproperty string QtQuick2::Context2D::lineJoin
     Holds the current line join style. A join exists at any point in a subpath
     shared by two consecutive lines. When a subpath is closed, then a join also exists
     at its first point (equivalent to its last point) connecting the first and last lines in the subpath.

    The possible line join styles are:
    \list
    \li bevel - this is all that is rendered at joins.
    \li round - a filled arc connecting the two aforementioned corners of the join, abutting (and not overlapping) the aforementioned triangle, with the diameter equal to the line width and the origin at the point of the join, must be rendered at joins.
    \li miter - a second filled triangle must (if it can given the miter length) be rendered at the join, this is the default line join style.
    \endlist
    Other values are ignored.
*/
QV4::Value QQuickJSContext2D::method_get_lineJoin(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    switch (r->context->state.lineJoin) {
    case Qt::RoundJoin:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("round")));
    case Qt::BevelJoin:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("bevel")));
    case Qt::MiterJoin:
    default:
        break;
    }
    return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("miter")));
}

QV4::Value QQuickJSContext2D::method_set_lineJoin(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    QString lineJoin = ctx->argument(0).toQString();
    Qt::PenJoinStyle join;
    if (lineJoin == QStringLiteral("round"))
        join = Qt::RoundJoin;
    else if (lineJoin == QStringLiteral("bevel"))
        join = Qt::BevelJoin;
    else if (lineJoin == QStringLiteral("miter"))
        join = Qt::SvgMiterJoin;
    else
        return QV4::Value::undefinedValue();

    if (join != r->context->state.lineJoin) {
        r->context->state.lineJoin = join;
        r->context->buffer()->setLineJoin(join);
    }
    return QV4::Value::undefinedValue();
}

/*!
    \qmlproperty real QtQuick2::Context2D::lineWidth
     Holds the current line width. Values that are not finite values greater than zero are ignored.
 */
QV4::Value QQuickJSContext2D::method_get_lineWidth(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    return QV4::Value::fromDouble(r->context->state.lineWidth);
}

QV4::Value QQuickJSContext2D::method_set_lineWidth(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    qreal w = ctx->argument(0).toNumber();

    if (w > 0 && qIsFinite(w) && w != r->context->state.lineWidth) {
        r->context->state.lineWidth = w;
        r->context->buffer()->setLineWidth(w);
    }
    return QV4::Value::undefinedValue();
}

/*!
    \qmlproperty real QtQuick2::Context2D::miterLimit
     Holds the current miter limit ratio.
     The default miter limit value is 10.0.
 */
QV4::Value QQuickJSContext2D::method_get_miterLimit(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    return QV4::Value::fromDouble(r->context->state.miterLimit);
}

QV4::Value QQuickJSContext2D::method_set_miterLimit(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    qreal ml = ctx->argument(0).toNumber();

    if (ml > 0 && qIsFinite(ml) && ml != r->context->state.miterLimit) {
        r->context->state.miterLimit = ml;
        r->context->buffer()->setMiterLimit(ml);
    }
    return QV4::Value::undefinedValue();
}

// shadows
/*!
    \qmlproperty real QtQuick2::Context2D::shadowBlur
     Holds the current level of blur applied to shadows
 */
QV4::Value QQuickJSContext2D::method_get_shadowBlur(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    return QV4::Value::fromDouble(r->context->state.shadowBlur);
}

QV4::Value QQuickJSContext2D::method_set_shadowBlur(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    qreal blur = ctx->argument(0).toNumber();

    if (blur > 0 && qIsFinite(blur) && blur != r->context->state.shadowBlur) {
        r->context->state.shadowBlur = blur;
        r->context->buffer()->setShadowBlur(blur);
    }
    return QV4::Value::undefinedValue();
}

/*!
    \qmlproperty string QtQuick2::Context2D::shadowColor
     Holds the current shadow color.
 */
QV4::Value QQuickJSContext2D::method_get_shadowColor(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    return QV4::Value::fromString(ctx->engine->newString(r->context->state.shadowColor.name()));
}

QV4::Value QQuickJSContext2D::method_set_shadowColor(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    QColor color = qt_color_from_string(ctx->argument(0));

    if (color.isValid() && color != r->context->state.shadowColor) {
        r->context->state.shadowColor = color;
        r->context->buffer()->setShadowColor(color);
    }
    return QV4::Value::undefinedValue();
}


/*!
    \qmlproperty qreal QtQuick2::Context2D::shadowOffsetX
     Holds the current shadow offset in the positive horizontal distance.

     \sa shadowOffsetY
 */
QV4::Value QQuickJSContext2D::method_get_shadowOffsetX(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    return QV4::Value::fromDouble(r->context->state.shadowOffsetX);
}

QV4::Value QQuickJSContext2D::method_set_shadowOffsetX(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    qreal offsetX = ctx->argument(0).toNumber();
    if (qIsFinite(offsetX) && offsetX != r->context->state.shadowOffsetX) {
        r->context->state.shadowOffsetX = offsetX;
        r->context->buffer()->setShadowOffsetX(offsetX);
    }
    return QV4::Value::undefinedValue();
}
/*!
    \qmlproperty qreal QtQuick2::Context2D::shadowOffsetY
     Holds the current shadow offset in the positive vertical distance.

     \sa shadowOffsetX
 */
QV4::Value QQuickJSContext2D::method_get_shadowOffsetY(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    return QV4::Value::fromDouble(r->context->state.shadowOffsetY);
}

QV4::Value QQuickJSContext2D::method_set_shadowOffsetY(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    qreal offsetY = ctx->argument(0).toNumber();
    if (qIsFinite(offsetY) && offsetY != r->context->state.shadowOffsetY) {
        r->context->state.shadowOffsetY = offsetY;
        r->context->buffer()->setShadowOffsetY(offsetY);
    }
    return QV4::Value::undefinedValue();
}

QV4::Value QQuickJSContext2D::method_get_path(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    return r->context->m_v4path.value();
}

QV4::Value QQuickJSContext2D::method_set_path(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    QV4::Value value = ctx->argument(0);
    r->context->beginPath();
    if (QV4::QObjectWrapper *qobjectWrapper = value.as<QV4::QObjectWrapper>()) {
        if (QQuickPath *path = qobject_cast<QQuickPath*>(qobjectWrapper->object()))
            r->context->m_path = path->path();
    } else {
        QString path = value.toQString();
        QQuickSvgParser::parsePathDataFast(path, r->context->m_path);
    }
    r->context->m_v4path = value;
    return QV4::Value::undefinedValue();
}

//rects
/*!
  \qmlmethod object QtQuick2::Context2D::clearRect(real x, real y, real w, real h)
  Clears all pixels on the canvas in the given rectangle to transparent black.
  */
QV4::Value QQuickJSContext2DPrototype::method_clearRect(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    if (ctx->argumentCount == 4)
        r->context->clearRect(ctx->arguments[0].toNumber(),
                              ctx->arguments[1].toNumber(),
                              ctx->arguments[2].toNumber(),
                              ctx->arguments[3].toNumber());

    return ctx->thisObject;
}
/*!
  \qmlmethod object QtQuick2::Context2D::fillRect(real x, real y, real w, real h)
   Paint the specified rectangular area using the fillStyle.

   \sa fillStyle
  */
QV4::Value QQuickJSContext2DPrototype::method_fillRect(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 4)
        r->context->fillRect(ctx->arguments[0].toNumber(), ctx->arguments[1].toNumber(), ctx->arguments[2].toNumber(), ctx->arguments[3].toNumber());
    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::fillRect(real x, real y, real w, real h)
   Stroke the specified rectangle's path using the strokeStyle, lineWidth, lineJoin,
   and (if appropriate) miterLimit attributes.

   \sa strokeStyle
   \sa lineWidth
   \sa lineJoin
   \sa miterLimit
  */
QV4::Value QQuickJSContext2DPrototype::method_strokeRect(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 4)
        r->context->strokeRect(ctx->arguments[0].toNumber(), ctx->arguments[1].toNumber(), ctx->arguments[2].toNumber(), ctx->arguments[3].toNumber());

    return ctx->thisObject;
}

// Complex shapes (paths) API
/*!
  \qmlmethod object QtQuick2::Context2D::arc(real x, real y, real radius, real startAngle, real endAngle, bool anticlockwise)
  Adds an arc to the current subpath that lies on the circumference of the circle whose center is at the point (\c x,\cy) and whose radius is \c radius.
  \image qml-item-canvas-arcTo2.png
  \sa arcTo,
      {http://www.w3.org/TR/2dcontext/#dom-context-2d-arc}{W3C 2d context standard for arc}
  */
QV4::Value QQuickJSContext2DPrototype::method_arc(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount >= 5) {
        bool antiClockwise = false;

        if (ctx->argumentCount == 6)
            antiClockwise = ctx->arguments[5].toBoolean();

        qreal radius = ctx->arguments[2].toNumber();

        if (qIsFinite(radius) && radius < 0)
           V4THROW_DOM(DOMEXCEPTION_INDEX_SIZE_ERR, "Incorrect argument radius");

        r->context->arc(ctx->arguments[0].toNumber(),
                        ctx->arguments[1].toNumber(),
                        radius,
                        ctx->arguments[3].toNumber(),
                        ctx->arguments[4].toNumber(),
                        antiClockwise);
    }

    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::arcTo(real x1, real y1, real x2, real y2, real radius)

   Adds an arc with the given control points and radius to the current subpath, connected to the previous point by a straight line.
   To draw an arc, you begin with the same steps your followed to create a line:
    \list
    \li Call the context.beginPath() method to set a new path.
    \li Call the context.moveTo(\c x, \c y) method to set your starting position on the canvas at the point (\c x,\c y).
    \li To draw an arc or circle, call the context.arcTo(\c x1, \c y1, \c x2, \c y2,\c radius) method.
       This adds an arc with starting point (\c x1,\c y1), ending point (\c x2, \c y2), and radius \c radius to the current subpath and connects
       it to the previous subpath by a straight line.
    \endlist
    \image qml-item-canvas-arcTo.png
    Both startAngle and endAngle are measured from the x axis in units of radians.

    \image qml-item-canvas-startAngle.png
    The anticlockwise has the value TRUE for each arc in the figure above because they are all drawn in the counterclockwise direction.
  \sa arc, {http://www.w3.org/TR/2dcontext/#dom-context-2d-arcto}{W3C 2d
      context standard for arcTo}
  */
QV4::Value QQuickJSContext2DPrototype::method_arcTo(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 5) {
        qreal radius = ctx->arguments[4].toNumber();

        if (qIsFinite(radius) && radius < 0)
           V4THROW_DOM(DOMEXCEPTION_INDEX_SIZE_ERR, "Incorrect argument radius");

        r->context->arcTo(ctx->arguments[0].toNumber(),
                          ctx->arguments[1].toNumber(),
                          ctx->arguments[2].toNumber(),
                          ctx->arguments[3].toNumber(),
                          radius);
    }

    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::beginPath()

   Resets the current path to a new path.
  */
QV4::Value QQuickJSContext2DPrototype::method_beginPath(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    r->context->beginPath();

    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::bezierCurveTo(real cp1x, real cp1y, real cp2x, real cp2y, real x, real y)

  Adds a cubic bezier curve between the current position and the given endPoint using the control points specified by (\c cp1x, cp1y),
  and (\c cp2x, \c cp2y).
  After the curve is added, the current position is updated to be at the end point (\c x, \c y) of the curve.
  The following code produces the path shown below:
  \code
  ctx.strokeStyle = Qt.rgba(0, 0, 0, 1);
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(20, 0);//start point
  ctx.bezierCurveTo(-10, 90, 210, 90, 180, 0);
  ctx.stroke();
  \endcode
   \image qml-item-canvas-bezierCurveTo.png
  \sa {http://www.w3.org/TR/2dcontext/#dom-context-2d-beziercurveto}{W3C 2d context standard for bezierCurveTo}
  \sa {http://www.openrise.com/lab/FlowerPower/}{The beautiful flower demo by using bezierCurveTo}
  */
QV4::Value QQuickJSContext2DPrototype::method_bezierCurveTo(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    if (ctx->argumentCount == 6) {
        qreal cp1x = ctx->arguments[0].toNumber();
        qreal cp1y = ctx->arguments[1].toNumber();
        qreal cp2x = ctx->arguments[2].toNumber();
        qreal cp2y = ctx->arguments[3].toNumber();
        qreal x = ctx->arguments[4].toNumber();
        qreal y = ctx->arguments[5].toNumber();

        if (!qIsFinite(cp1x) || !qIsFinite(cp1y) || !qIsFinite(cp2x) || !qIsFinite(cp2y) || !qIsFinite(x) || !qIsFinite(y))
            return ctx->thisObject;

        r->context->bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);
    }

    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::clip()

   Creates the clipping region from the current path.
   Any parts of the shape outside the clipping path are not displayed.
   To create a complex shape using the \c clip() method:

    \list 1
    \li Call the \c{context.beginPath()} method to set the clipping path.
    \li Define the clipping path by calling any combination of the \c{lineTo},
    \c{arcTo}, \c{arc}, \c{moveTo}, etc and \c{closePath} methods.
    \li Call the \c{context.clip()} method.
    \endlist

    The new shape displays.  The following shows how a clipping path can
    modify how an image displays:

    \image qml-canvas-clip-complex.png
    \sa beginPath()
    \sa closePath()
    \sa stroke()
    \sa fill()
   \sa {http://www.w3.org/TR/2dcontext/#dom-context-2d-clip}{W3C 2d context standard for clip}
  */
QV4::Value QQuickJSContext2DPrototype::method_clip(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    r->context->clip();
    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::closePath()
   Closes the current subpath by drawing a line to the beginning of the subpath, automatically starting a new path.
   The current point of the new path is the previous subpath's first point.

   \sa {http://www.w3.org/TR/2dcontext/#dom-context-2d-closepath}{W3C 2d context standard for closePath}
  */
QV4::Value QQuickJSContext2DPrototype::method_closePath(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    r->context->closePath();

    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::fill()

   Fills the subpaths with the current fill style.

   \sa {http://www.w3.org/TR/2dcontext/#dom-context-2d-fill}{W3C 2d context standard for fill}

   \sa fillStyle
  */
QV4::Value QQuickJSContext2DPrototype::method_fill(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r);
    r->context->fill();
    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::lineTo(real x, real y)

   Draws a line from the current position to the point (x, y).
 */
QV4::Value QQuickJSContext2DPrototype::method_lineTo(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    if (ctx->argumentCount == 2) {
        qreal x = ctx->arguments[0].toNumber();
        qreal y = ctx->arguments[1].toNumber();

        if (!qIsFinite(x) || !qIsFinite(y))
            return ctx->thisObject;

        r->context->lineTo(x, y);
    }

    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::moveTo(real x, real y)

   Creates a new subpath with the given point.
 */
QV4::Value QQuickJSContext2DPrototype::method_moveTo(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 2) {
        qreal x = ctx->arguments[0].toNumber();
        qreal y = ctx->arguments[1].toNumber();

        if (!qIsFinite(x) || !qIsFinite(y))
            return ctx->thisObject;
        r->context->moveTo(x, y);
    }
    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::quadraticCurveTo(real cpx, real cpy, real x, real y)

   Adds a quadratic bezier curve between the current point and the endpoint (\c x, \c y) with the control point specified by (\c cpx, \c cpy).

   See \l{http://www.w3.org/TR/2dcontext/#dom-context-2d-quadraticcurveto}{W3C 2d context standard for  for quadraticCurveTo}
 */
QV4::Value QQuickJSContext2DPrototype::method_quadraticCurveTo(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 4) {
        qreal cpx = ctx->arguments[0].toNumber();
        qreal cpy = ctx->arguments[1].toNumber();
        qreal x = ctx->arguments[2].toNumber();
        qreal y = ctx->arguments[3].toNumber();

        if (!qIsFinite(cpx) || !qIsFinite(cpy) || !qIsFinite(x) || !qIsFinite(y))
            return ctx->thisObject;

        r->context->quadraticCurveTo(cpx, cpy, x, y);
    }

    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::rect(real x, real y, real w, real h)

   Adds a rectangle at position (\c x, \c y), with the given width \c w and height \c h, as a closed subpath.
 */
QV4::Value QQuickJSContext2DPrototype::method_rect(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 4)
        r->context->rect(ctx->arguments[0].toNumber(), ctx->arguments[1].toNumber(), ctx->arguments[2].toNumber(), ctx->arguments[3].toNumber());
    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::roundedRect(real x, real y, real w, real h,  real xRadius, real yRadius)

   Adds the given rectangle rect with rounded corners to the path. The \c xRadius and \c yRadius arguments specify the radius of the
   ellipses defining the corners of the rounded rectangle.
 */
QV4::Value QQuickJSContext2DPrototype::method_roundedRect(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 6)
        r->context->roundedRect(ctx->arguments[0].toNumber()
                              , ctx->arguments[1].toNumber()
                              , ctx->arguments[2].toNumber()
                              , ctx->arguments[3].toNumber()
                              , ctx->arguments[4].toNumber()
                              , ctx->arguments[5].toNumber());
    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::ellipse(real x, real y, real w, real h)

  Creates an ellipse within the bounding rectangle defined by its top-left corner at (\a x, \ y), width \a w and height \a h,
  and adds it to the path as a closed subpath.

  The ellipse is composed of a clockwise curve, starting and finishing at zero degrees (the 3 o'clock position).
 */
QV4::Value QQuickJSContext2DPrototype::method_ellipse(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)


    if (ctx->argumentCount == 4)
        r->context->ellipse(ctx->arguments[0].toNumber(), ctx->arguments[1].toNumber(), ctx->arguments[2].toNumber(), ctx->arguments[3].toNumber());

    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::text(string text, real x, real y)

  Adds the given \c text to the path as a set of closed subpaths created from the current context font supplied.
  The subpaths are positioned so that the left end of the text's baseline lies at the point specified by (\c x, \c y).
 */
QV4::Value QQuickJSContext2DPrototype::method_text(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 3) {
        qreal x = ctx->arguments[1].toNumber();
        qreal y = ctx->arguments[2].toNumber();

        if (!qIsFinite(x) || !qIsFinite(y))
            return ctx->thisObject;
        r->context->text(ctx->arguments[0].toQString(), x, y);
    }
    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::stroke()

   Strokes the subpaths with the current stroke style.

   See \l{http://www.w3.org/TR/2dcontext/#dom-context-2d-stroke}{W3C 2d context standard for stroke}

   \sa strokeStyle
  */
QV4::Value QQuickJSContext2DPrototype::method_stroke(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    r->context->stroke();
    return ctx->thisObject;
}

/*!
  \qmlmethod object QtQuick2::Context2D::isPointInPath(real x, real y)

   Returns true if the given point is in the current path.

   \sa {http://www.w3.org/TR/2dcontext/#dom-context-2d-ispointinpath}{W3C 2d context standard for isPointInPath}
  */
QV4::Value QQuickJSContext2DPrototype::method_isPointInPath(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    bool pointInPath = false;
    if (ctx->argumentCount == 2)
        pointInPath = r->context->isPointInPath(ctx->arguments[0].toNumber(), ctx->arguments[1].toNumber());
    return QV4::Value::fromBoolean(pointInPath);
}

QV4::Value QQuickJSContext2DPrototype::method_drawFocusRing(QV4::SimpleCallContext *ctx)
{
    Q_UNUSED(ctx);

    V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "Context2D::drawFocusRing is not supported");
}

QV4::Value QQuickJSContext2DPrototype::method_setCaretSelectionRect(QV4::SimpleCallContext *ctx)
{
    Q_UNUSED(ctx);

    V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "Context2D::setCaretSelectionRect is not supported");
}

QV4::Value QQuickJSContext2DPrototype::method_caretBlinkRate(QV4::SimpleCallContext *ctx)
{
    Q_UNUSED(ctx);

    V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "Context2D::caretBlinkRate is not supported");
}
// text
/*!
  \qmlproperty string QtQuick2::Context2D::font
  Holds the current font settings.

  The default font value is "10px sans-serif".
  See \l {http://www.w3.org/TR/2dcontext/#dom-context-2d-font}{w3C 2d context standard for font}
  */
QV4::Value QQuickJSContext2D::method_get_font(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    return QV4::Value::fromString(ctx->engine->newString(r->context->state.font.toString()));
}

QV4::Value QQuickJSContext2D::method_set_font(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    QString fs = ctx->argument(0).toQString();
    QFont font = qt_font_from_string(fs);
    if (font != r->context->state.font) {
        r->context->state.font = font;
    }
    return QV4::Value::undefinedValue();
}

/*!
  \qmlproperty string QtQuick2::Context2D::textAlign

  Holds the current text alignment settings.
  The possible values are:
  \list
    \li start
    \li end
    \li left
    \li right
    \li center
  \endlist
  Other values are ignored. The default value is "start".
  */
QV4::Value QQuickJSContext2D::method_get_textAlign(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    switch (r->context->state.textAlign) {
    case QQuickContext2D::End:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("end")));
    case QQuickContext2D::Left:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("left")));
    case QQuickContext2D::Right:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("right")));
    case QQuickContext2D::Center:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("center")));
    case QQuickContext2D::Start:
    default:
        break;
    }
    return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("start")));
}

QV4::Value QQuickJSContext2D::method_set_textAlign(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)

    QString textAlign = ctx->argument(0).toQString();

    QQuickContext2D::TextAlignType ta;
    if (textAlign == QStringLiteral("start"))
        ta = QQuickContext2D::Start;
    else if (textAlign == QStringLiteral("end"))
        ta = QQuickContext2D::End;
    else if (textAlign == QStringLiteral("left"))
        ta = QQuickContext2D::Left;
    else if (textAlign == QStringLiteral("right"))
        ta = QQuickContext2D::Right;
    else if (textAlign == QStringLiteral("center"))
        ta = QQuickContext2D::Center;
    else
        return QV4::Value::undefinedValue();

    if (ta != r->context->state.textAlign)
        r->context->state.textAlign = ta;

    return QV4::Value::undefinedValue();
}

/*!
  \qmlproperty string QtQuick2::Context2D::textBaseline

  Holds the current baseline alignment settings.
  The possible values are:
  \list
    \li top
    \li hanging
    \li middle
    \li alphabetic
    \li ideographic
    \li bottom
  \endlist
  Other values are ignored. The default value is "alphabetic".
  */
QV4::Value QQuickJSContext2D::method_get_textBaseline(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    switch (r->context->state.textBaseline) {
    case QQuickContext2D::Hanging:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("hanging")));
    case QQuickContext2D::Top:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("top")));
    case QQuickContext2D::Bottom:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("bottom")));
    case QQuickContext2D::Middle:
        return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("middle")));
    case QQuickContext2D::Alphabetic:
    default:
        break;
    }
    return QV4::Value::fromString(ctx->engine->newString(QStringLiteral("alphabetic")));
}

QV4::Value QQuickJSContext2D::method_set_textBaseline(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT_SETTER(r)
    QString textBaseline = ctx->argument(0).toQString();

    QQuickContext2D::TextBaseLineType tb;
    if (textBaseline == QStringLiteral("alphabetic"))
        tb = QQuickContext2D::Alphabetic;
    else if (textBaseline == QStringLiteral("hanging"))
        tb = QQuickContext2D::Hanging;
    else if (textBaseline == QStringLiteral("top"))
        tb = QQuickContext2D::Top;
    else if (textBaseline == QStringLiteral("bottom"))
        tb = QQuickContext2D::Bottom;
    else if (textBaseline == QStringLiteral("middle"))
        tb = QQuickContext2D::Middle;
    else
        return QV4::Value::undefinedValue();

    if (tb != r->context->state.textBaseline)
        r->context->state.textBaseline = tb;

    return QV4::Value::undefinedValue();
}

/*!
  \qmlmethod object QtQuick2::Context2D::fillText(text, x, y)
  Fills the given text at the given position.
  \sa font
  \sa textAlign
  \sa textBaseline
  \sa strokeText
  */
QV4::Value QQuickJSContext2DPrototype::method_fillText(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 3) {
        qreal x = ctx->arguments[1].toNumber();
        qreal y = ctx->arguments[2].toNumber();
        if (!qIsFinite(x) || !qIsFinite(y))
            return ctx->thisObject;
        QPainterPath textPath = r->context->createTextGlyphs(x, y, ctx->arguments[0].toQString());
        r->context->buffer()->fill(textPath);
    }
    return ctx->thisObject;
}
/*!
  \qmlmethod object QtQuick2::Context2D::strokeText(text, x, y)
  Strokes the given text at the given position.
  \sa font
  \sa textAlign
  \sa textBaseline
  \sa fillText
  */
QV4::Value QQuickJSContext2DPrototype::method_strokeText(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 3)
        r->context->drawText(ctx->arguments[0].toQString(), ctx->arguments[1].toNumber(), ctx->arguments[2].toNumber(), false);
    return ctx->thisObject;
}

/*!
  \qmltype TextMetrics
    \inqmlmodule QtQuick 2
    \since QtQuick 2.0
    \ingroup qtquick-canvas
    \brief Provides a Context2D TextMetrics interface

    The TextMetrics object can be created by QtQuick2::Context2D::measureText method.
    See \l{http://www.w3.org/TR/2dcontext/#textmetrics}{W3C 2d context TexMetrics} for more details.

    \sa Context2D::measureText
    \sa width
  */

/*!
  \qmlproperty int QtQuick2::TextMetrics::width
  Holds the advance width of the text that was passed to the QtQuick2::Context2D::measureText() method.
  This property is read only.
  */

/*!
  \qmlmethod variant QtQuick2::Context2D::measureText(text)
  Returns a TextMetrics object with the metrics of the given text in the current font.
  */
QV4::Value QQuickJSContext2DPrototype::method_measureText(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    if (ctx->argumentCount == 1) {
        QFontMetrics fm(r->context->state.font);
        uint width = fm.width(ctx->arguments[0].toQString());
        QV4::Object *tm = ctx->engine->newObject();
        tm->put(ctx->engine->current, ctx->engine->newIdentifier(QStringLiteral("width")), QV4::Value::fromDouble(width));
        return QV4::Value::fromObject(tm);
    }
    return QV4::Value::undefinedValue();
}

// drawing images
/*!
  \qmlmethod QtQuick2::Context2D::drawImage(variant image, real dx, real dy)
  Draws the given \a image on the canvas at position (\a dx, \a dy).
  Note:
  The \a image type can be an Image item, an image url or a CanvasImageData object.
  When given as Image item, if the image isn't fully loaded, this method draws nothing.
  When given as url string, the image should be loaded by calling Canvas item's Canvas::loadImage() method first.
  This image been drawing is subject to the current context clip path, even the given \c image is a CanvasImageData object.

  \sa CanvasImageData
  \sa Image
  \sa Canvas::loadImage
  \sa Canvas::isImageLoaded
  \sa Canvas::onImageLoaded

  \sa {http://www.w3.org/TR/2dcontext/#dom-context-2d-drawimage}{W3C 2d context standard for drawImage}
  */
/*!
  \qmlmethod QtQuick2::Context2D::drawImage(variant image, real dx, real dy, real dw, real dh)
  This is an overloaded function.
  Draws the given item as \a image onto the canvas at point (\a dx, \a dy) and with width \a dw,
  height \a dh.

  Note:
  The \a image type can be an Image item, an image url or a CanvasImageData object.
  When given as Image item, if the image isn't fully loaded, this method draws nothing.
  When given as url string, the image should be loaded by calling Canvas item's Canvas::loadImage() method first.
  This image been drawing is subject to the current context clip path, even the given \c image is a CanvasImageData object.

  \sa CanvasImageData
  \sa Image
  \sa Canvas::loadImage()
  \sa Canvas::isImageLoaded
  \sa Canvas::onImageLoaded

  \sa {http://www.w3.org/TR/2dcontext/#dom-context-2d-drawimage}{W3C 2d context standard for drawImage}
  */
/*!
  \qmlmethod QtQuick2::Context2D::drawImage(variant image, real sx, real sy, real sw, sh, real dx, real dy, real dw, dh)
  This is an overloaded function.
  Draws the given item as \a image from source point (\a sx, \a sy) and source width \a sw, source height \a sh
  onto the canvas at point (\a dx, \a dy) and with width \a dw, height \a dh.


  Note:
  The \a image type can be an Image or Canvas item, an image url or a CanvasImageData object.
  When given as Image item, if the image isn't fully loaded, this method draws nothing.
  When given as url string, the image should be loaded by calling Canvas item's Canvas::loadImage() method first.
  This image been drawing is subject to the current context clip path, even the given \c image is a CanvasImageData object.

  \sa CanvasImageData
  \sa Image
  \sa Canvas::loadImage()
  \sa Canvas::isImageLoaded
  \sa Canvas::onImageLoaded

  \sa {http://www.w3.org/TR/2dcontext/#dom-context-2d-drawimage}{W3C 2d context standard for drawImage}
*/
QV4::Value QQuickJSContext2DPrototype::method_drawImage(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    qreal sx, sy, sw, sh, dx, dy, dw, dh;

    if (!ctx->argumentCount)
        return ctx->thisObject;

    //FIXME:This function should be moved to QQuickContext2D::drawImage(...)
    if (!r->context->state.invertibleCTM)
        return ctx->thisObject;

    QQmlRefPointer<QQuickCanvasPixmap> pixmap;

    if (ctx->arguments[0].isString()) {
        QUrl url(ctx->arguments[0].toQString());
        if (!url.isValid())
            V4THROW_DOM(DOMEXCEPTION_TYPE_MISMATCH_ERR, "drawImage(), type mismatch");

        pixmap = r->context->createPixmap(url);
    } else if (ctx->arguments[0].isObject()) {
        if (QV4::QObjectWrapper *qobjectWrapper = ctx->arguments[0].as<QV4::QObjectWrapper>()) {
            if (QQuickImage *imageItem = qobject_cast<QQuickImage*>(qobjectWrapper->object())) {
                pixmap.take(r->context->createPixmap(imageItem->source()));
            } else if (QQuickCanvasItem *canvas = qobject_cast<QQuickCanvasItem*>(qobjectWrapper->object())) {
                QImage img = canvas->toImage();
                if (!img.isNull())
                    pixmap.take(new QQuickCanvasPixmap(img, canvas->window()));
            } else {
                V4THROW_DOM(DOMEXCEPTION_TYPE_MISMATCH_ERR, "drawImage(), type mismatch");
            }
        } else if (QQuickJSContext2DImageData *imageData = ctx->arguments[0].as<QQuickJSContext2DImageData>()) {
            QQuickJSContext2DPixelData *pix = imageData->pixelData.as<QQuickJSContext2DPixelData>();
            if (pix && !pix->image.isNull()) {
                pixmap.take(new QQuickCanvasPixmap(pix->image, r->context->canvas()->window()));
            } else {
                V4THROW_DOM(DOMEXCEPTION_TYPE_MISMATCH_ERR, "drawImage(), type mismatch");
            }
        } else {
            V4THROW_DOM(DOMEXCEPTION_TYPE_MISMATCH_ERR, "drawImage(), type mismatch");
        }
    } else {
        V4THROW_DOM(DOMEXCEPTION_TYPE_MISMATCH_ERR, "drawImage(), type mismatch");
    }

    if (pixmap.isNull() || !pixmap->isValid())
        return ctx->thisObject;

    if (ctx->argumentCount == 3) {
        dx = ctx->arguments[1].toNumber();
        dy = ctx->arguments[2].toNumber();
        sx = 0;
        sy = 0;
        sw = pixmap->width();
        sh = pixmap->height();
        dw = sw;
        dh = sh;
    } else if (ctx->argumentCount == 5) {
        sx = 0;
        sy = 0;
        sw = pixmap->width();
        sh = pixmap->height();
        dx = ctx->arguments[1].toNumber();
        dy = ctx->arguments[2].toNumber();
        dw = ctx->arguments[3].toNumber();
        dh = ctx->arguments[4].toNumber();
    } else if (ctx->argumentCount == 9) {
        sx = ctx->arguments[1].toNumber();
        sy = ctx->arguments[2].toNumber();
        sw = ctx->arguments[3].toNumber();
        sh = ctx->arguments[4].toNumber();
        dx = ctx->arguments[5].toNumber();
        dy = ctx->arguments[6].toNumber();
        dw = ctx->arguments[7].toNumber();
        dh = ctx->arguments[8].toNumber();
    } else {
        return ctx->thisObject;
    }

    if (!qIsFinite(sx)
     || !qIsFinite(sy)
     || !qIsFinite(sw)
     || !qIsFinite(sh)
     || !qIsFinite(dx)
     || !qIsFinite(dy)
     || !qIsFinite(dw)
     || !qIsFinite(dh))
        return ctx->thisObject;

    if (sx < 0
    || sy < 0
    || sw == 0
    || sh == 0
    || sx + sw > pixmap->width()
    || sy + sh > pixmap->height()
    || sx + sw < 0 || sy + sh < 0) {
            V4THROW_DOM(DOMEXCEPTION_INDEX_SIZE_ERR, "drawImage(), index size error");
    }

    r->context->buffer()->drawPixmap(pixmap, QRectF(sx, sy, sw, sh), QRectF(dx, dy, dw, dh));

    return ctx->thisObject;
}

// pixel manipulation
/*!
    \qmltype CanvasImageData
    \inqmlmodule QtQuick 2
    \ingroup qtquick-canvas
    \brief Contains image pixel data in RGBA order

     The CanvasImageData object holds the image pixel data.

     The CanvasImageData object has the actual dimensions of the data stored in
     this object and holds the one-dimensional array containing the data in RGBA order,
     as integers in the range 0 to 255.

     \sa width
     \sa height
     \sa data
     \sa Context2D::createImageData()
     \sa Context2D::getImageData()
     \sa Context2D::putImageData()
  */
/*!
  \qmlproperty int QtQuick2::CanvasImageData::width
  Holds the actual width dimension of the data in the ImageData object, in device pixels.
 */
QV4::Value QQuickJSContext2DImageData::method_get_width(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2DImageData *imageData = ctx->thisObject.as<QQuickJSContext2DImageData>();
    if (!imageData)
        ctx->throwTypeError();
    QQuickJSContext2DPixelData *r = imageData->pixelData.as<QQuickJSContext2DPixelData>();
    if (!r)
        return QV4::Value::fromInt32(0);
    return QV4::Value::fromInt32(r->image.width());
}

/*!
  \qmlproperty int QtQuick2::CanvasImageData::height
  Holds the actual height dimension of the data in the ImageData object, in device pixels.
  */
QV4::Value QQuickJSContext2DImageData::method_get_height(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2DImageData *imageData = ctx->thisObject.as<QQuickJSContext2DImageData>();
    if (!imageData)
        ctx->throwTypeError();
    QQuickJSContext2DPixelData *r = imageData->pixelData.as<QQuickJSContext2DPixelData>();
    if (!r)
        return QV4::Value::fromInt32(0);
    return QV4::Value::fromInt32(r->image.height());
}

/*!
  \qmlproperty object QtQuick2::CanvasImageData::data
  Holds the one-dimensional array containing the data in RGBA order, as integers in the range 0 to 255.
 */
QV4::Value QQuickJSContext2DImageData::method_get_data(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2DImageData *imageData = ctx->thisObject.as<QQuickJSContext2DImageData>();
    if (!imageData)
        ctx->throwTypeError();
    return imageData->pixelData;
}

/*!
    \qmltype CanvasPixelArray
    \inqmlmodule QtQuick 2
    \ingroup qtquick-canvas
    \brief Provides ordered and indexed access to the components of each pixel in image data

  The CanvasPixelArray object provides ordered, indexed access to the color components of each pixel of the image data.
  The CanvasPixelArray can be accessed as normal Javascript array.
    \sa CanvasImageData
    \sa {http://www.w3.org/TR/2dcontext/#canvaspixelarray}{W3C 2d context standard for PixelArray}
  */

/*!
  \qmlproperty int QtQuick2::CanvasPixelArray::length
  The CanvasPixelArray object represents h×w×4 integers which w and h comes from CanvasImageData.
  The length attribute of a CanvasPixelArray object must return this h×w×4 number value.
  This property is read only.
*/
QV4::Value QQuickJSContext2DPixelData::proto_get_length(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2DPixelData *r = ctx->thisObject.as<QQuickJSContext2DPixelData>();
    if (!r || r->image.isNull())
        return QV4::Value::undefinedValue();

    return QV4::Value::fromInt32(r->image.width() * r->image.height() * 4);
}

QV4::Value QQuickJSContext2DPixelData::getIndexed(QV4::Managed *m, uint index, bool *hasProperty)
{
    QQuickJSContext2DPixelData *r = m->as<QQuickJSContext2DPixelData>();
    if (!m)
        m->engine()->current->throwTypeError();

    if (r && index < static_cast<quint32>(r->image.width() * r->image.height() * 4)) {
        if (hasProperty)
            *hasProperty = true;
        const quint32 w = r->image.width();
        const quint32 row = (index / 4) / w;
        const quint32 col = (index / 4) % w;
        const QRgb* pixel = reinterpret_cast<const QRgb*>(r->image.constScanLine(row));
        pixel += col;
        switch (index % 4) {
        case 0:
            return QV4::Value::fromInt32(qRed(*pixel));
        case 1:
            return QV4::Value::fromInt32(qGreen(*pixel));
        case 2:
            return QV4::Value::fromInt32(qBlue(*pixel));
        case 3:
            return QV4::Value::fromInt32(qAlpha(*pixel));
        }
    }
    if (hasProperty)
        *hasProperty = false;
    return QV4::Value::undefinedValue();
}

void QQuickJSContext2DPixelData::putIndexed(QV4::Managed *m, QV4::ExecutionContext *ctx, uint index, const QV4::Value &value)
{
    QQuickJSContext2DPixelData *r = m->as<QQuickJSContext2DPixelData>();
    if (!r)
        ctx->throwTypeError();

    const int v = value.toInt32();
    if (r && index < static_cast<quint32>(r->image.width() * r->image.height() * 4) && v >= 0 && v <= 255) {
        const quint32 w = r->image.width();
        const quint32 row = (index / 4) / w;
        const quint32 col = (index / 4) % w;

        QRgb* pixel = reinterpret_cast<QRgb*>(r->image.scanLine(row));
        pixel += col;
        switch (index % 4) {
        case 0:
            *pixel = qRgba(v, qGreen(*pixel), qBlue(*pixel), qAlpha(*pixel));
            break;
        case 1:
            *pixel = qRgba(qRed(*pixel), v, qBlue(*pixel), qAlpha(*pixel));
            break;
        case 2:
            *pixel = qRgba(qRed(*pixel), qGreen(*pixel), v, qAlpha(*pixel));
            break;
        case 3:
            *pixel = qRgba(qRed(*pixel), qGreen(*pixel), qBlue(*pixel), v);
            break;
        }
    }
}
/*!
  \qmlmethod CanvasImageData QtQuick2::Context2D::createImageData(real sw, real sh)
   Creates a CanvasImageData object with the given dimensions(\a sw, \a sh).
  */
/*!
  \qmlmethod CanvasImageData QtQuick2::Context2D::createImageData(CanvasImageData imageData)
   Creates a CanvasImageData object with the same dimensions as the argument.
  */
/*!
  \qmlmethod CanvasImageData QtQuick2::Context2D::createImageData(Url imageUrl)
   Creates a CanvasImageData object with the given image loaded from \a imageUrl.
   Note:The \a imageUrl must be already loaded before this function call, if not, an empty
   CanvasImageData obect will be returned.

   \sa Canvas::loadImage(), QtQuick2::Canvas::unloadImage(), QtQuick2::Canvas::isImageLoaded
  */
QV4::Value QQuickJSContext2DPrototype::method_createImageData(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    QV8Engine *engine = ctx->engine->v8Engine;

    if (ctx->argumentCount == 1) {
        if (QQuickJSContext2DImageData *imgData = ctx->arguments[0].as<QQuickJSContext2DImageData>()) {
            QQuickJSContext2DPixelData *pa = imgData->pixelData.as<QQuickJSContext2DPixelData>();
            if (pa) {
                qreal w = pa->image.width();
                qreal h = pa->image.height();
                return qt_create_image_data(w, h, engine, QImage());
            }
        } else if (ctx->arguments[0].isString()) {
            QImage image = r->context->createPixmap(QUrl(ctx->arguments[0].toQString()))->image();
            return qt_create_image_data(image.width(), image.height(), engine, image);
        }
    } else if (ctx->argumentCount == 2) {
        qreal w = ctx->arguments[0].toNumber();
        qreal h = ctx->arguments[1].toNumber();

        if (!qIsFinite(w) || !qIsFinite(h))
            V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "createImageData(): invalid arguments");

        if (w > 0 && h > 0)
            return qt_create_image_data(w, h, engine, QImage());
        else
            V4THROW_DOM(DOMEXCEPTION_INDEX_SIZE_ERR, "createImageData(): invalid arguments");
    }
    return QV4::Value::undefinedValue();
}

/*!
  \qmlmethod CanvasImageData QtQuick2::Canvas::getImageData(real sx, real sy, real sw, real sh)
  Returns an CanvasImageData object containing the image data for the given rectangle of the canvas.
  */
QV4::Value QQuickJSContext2DPrototype::method_getImageData(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)

    QV8Engine *engine = ctx->engine->v8Engine;
    if (ctx->argumentCount == 4) {
        qreal x = ctx->arguments[0].toNumber();
        qreal y = ctx->arguments[1].toNumber();
        qreal w = ctx->arguments[2].toNumber();
        qreal h = ctx->arguments[3].toNumber();
        if (!qIsFinite(x) || !qIsFinite(y) || !qIsFinite(w) || !qIsFinite(w))
            V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "getImageData(): Invalid arguments");

        if (w <= 0 || h <= 0)
            V4THROW_DOM(DOMEXCEPTION_INDEX_SIZE_ERR, "getImageData(): Invalid arguments");

        QImage image = r->context->canvas()->toImage(QRectF(x, y, w, h));
        QV4::Value imageData = qt_create_image_data(w, h, engine, image);

        return imageData;
    }
    return QV4::Value::nullValue();
}

/*!
  \qmlmethod object QtQuick2::Context2D::putImageData(CanvasImageData imageData, real dx, real dy, real dirtyX, real dirtyY, real dirtyWidth, real dirtyHeight)
  Paints the data from the given ImageData object onto the canvas. If a dirty rectangle (\a dirtyX, \a dirtyY, \a dirtyWidth, \a dirtyHeight) is provided, only the pixels from that rectangle are painted.
  */
QV4::Value QQuickJSContext2DPrototype::method_putImageData(QV4::SimpleCallContext *ctx)
{
    QQuickJSContext2D *r = ctx->thisObject.as<QQuickJSContext2D>();
    CHECK_CONTEXT(r)
    if (ctx->argumentCount != 3 && ctx->argumentCount != 7)
        return QV4::Value::undefinedValue();

    if (!ctx->arguments[0].isObject())
        V4THROW_DOM(DOMEXCEPTION_TYPE_MISMATCH_ERR, "Context2D::putImageData, the image data type mismatch");

    qreal dx = ctx->arguments[1].toNumber();
    qreal dy = ctx->arguments[2].toNumber();
    qreal w, h, dirtyX, dirtyY, dirtyWidth, dirtyHeight;

    if (!qIsFinite(dx) || !qIsFinite(dy))
        V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "putImageData() : Invalid arguments");

    QQuickJSContext2DImageData *imageData = ctx->arguments[0].as<QQuickJSContext2DImageData>();
    if (!imageData)
        return ctx->thisObject;

    QQuickJSContext2DPixelData *pixelArray = imageData->pixelData.as<QQuickJSContext2DPixelData>();
    if (pixelArray) {
        w = pixelArray->image.width();
        h = pixelArray->image.height();

        if (ctx->argumentCount == 7) {
            dirtyX = ctx->arguments[3].toNumber();
            dirtyY = ctx->arguments[4].toNumber();
            dirtyWidth = ctx->arguments[5].toNumber();
            dirtyHeight = ctx->arguments[6].toNumber();

            if (!qIsFinite(dirtyX) || !qIsFinite(dirtyY) || !qIsFinite(dirtyWidth) || !qIsFinite(dirtyHeight))
                V4THROW_DOM(DOMEXCEPTION_NOT_SUPPORTED_ERR, "putImageData() : Invalid arguments");


            if (dirtyWidth < 0) {
                dirtyX = dirtyX+dirtyWidth;
                dirtyWidth = -dirtyWidth;
            }

            if (dirtyHeight < 0) {
                dirtyY = dirtyY+dirtyHeight;
                dirtyHeight = -dirtyHeight;
            }

            if (dirtyX < 0) {
                dirtyWidth = dirtyWidth+dirtyX;
                dirtyX = 0;
            }

            if (dirtyY < 0) {
                dirtyHeight = dirtyHeight+dirtyY;
                dirtyY = 0;
            }

            if (dirtyX+dirtyWidth > w) {
                dirtyWidth = w - dirtyX;
            }

            if (dirtyY+dirtyHeight > h) {
                dirtyHeight = h - dirtyY;
            }

            if (dirtyWidth <=0 || dirtyHeight <= 0)
                return ctx->thisObject;
        } else {
            dirtyX = 0;
            dirtyY = 0;
            dirtyWidth = w;
            dirtyHeight = h;
        }

        QImage image = pixelArray->image.copy(dirtyX, dirtyY, dirtyWidth, dirtyHeight);
        r->context->buffer()->drawImage(image, QRectF(dirtyX, dirtyY, dirtyWidth, dirtyHeight), QRectF(dx, dy, dirtyWidth, dirtyHeight));
    }
    return ctx->thisObject;
}

/*!
    \qmltype CanvasGradient
    \inqmlmodule QtQuick 2
    \since QtQuick 2.0
    \ingroup qtquick-canvas
    \brief Provides an opaque CanvasGradient interface
  */

/*!
  \qmlmethod CanvasGradient QtQuick2::CanvasGradient::addColorStop(real offsetof, string color)
  Adds a color stop with the given color to the gradient at the given offset.
  0.0 is the offset at one end of the gradient, 1.0 is the offset at the other end.

  For example:
  \code
  var gradient = ctx.createLinearGradient(0, 0, 100, 100);
  gradient.addColorStop(0.3, Qt.rgba(1, 0, 0, 1));
  gradient.addColorStop(0.7, 'rgba(0, 255, 255, 1');
  \endcode
  */
QV4::Value QQuickContext2DStyle::gradient_proto_addColorStop(QV4::SimpleCallContext *ctx)
{
    QQuickContext2DStyle *style = ctx->thisObject.as<QQuickContext2DStyle>();
    if (!style)
        V4THROW_ERROR("Not a CanvasGradient object");

    QV8Engine *engine = ctx->engine->v8Engine;

    if (ctx->argumentCount == 2) {

        if (!style->brush.gradient())
            V4THROW_ERROR("Not a valid CanvasGradient object, can't get the gradient information");
        QGradient gradient = *(style->brush.gradient());
        qreal pos = ctx->arguments[0].toNumber();
        QColor color;

        if (ctx->arguments[1].asObject()) {
            color = engine->toVariant(ctx->arguments[1], qMetaTypeId<QColor>()).value<QColor>();
        } else {
            color = qt_color_from_string(ctx->arguments[1]);
        }
        if (pos < 0.0 || pos > 1.0 || !qIsFinite(pos)) {
            V4THROW_DOM(DOMEXCEPTION_INDEX_SIZE_ERR, "CanvasGradient: parameter offset out of range");
        }

        if (color.isValid()) {
            gradient.setColorAt(pos, color);
        } else {
            V4THROW_DOM(DOMEXCEPTION_SYNTAX_ERR, "CanvasGradient: parameter color is not a valid color string");
        }
        style->brush = gradient;
    }

    return ctx->thisObject;
}

void QQuickContext2D::scale(qreal x,  qreal y)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(x) || !qIsFinite(y))
        return;

    QTransform newTransform = state.matrix;
    newTransform.scale(x, y);

    if (!newTransform.isInvertible()) {
        state.invertibleCTM = false;
        return;
    }

    state.matrix = newTransform;
    buffer()->updateMatrix(state.matrix);
    m_path = QTransform().scale(1.0 / x, 1.0 / y).map(m_path);
}

void QQuickContext2D::rotate(qreal angle)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(angle))
        return;

    QTransform newTransform =state.matrix;
    newTransform.rotate(DEGREES(angle));

    if (!newTransform.isInvertible()) {
        state.invertibleCTM = false;
        return;
    }

    state.matrix = newTransform;
    buffer()->updateMatrix(state.matrix);
    m_path = QTransform().rotate(-DEGREES(angle)).map(m_path);
}

void QQuickContext2D::shear(qreal h, qreal v)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(h) || !qIsFinite(v))
        return ;

    QTransform newTransform = state.matrix;
    newTransform.shear(h, v);

    if (!newTransform.isInvertible()) {
        state.invertibleCTM = false;
        return;
    }

    state.matrix = newTransform;
    buffer()->updateMatrix(state.matrix);
    m_path = QTransform().shear(-h, -v).map(m_path);
}

void QQuickContext2D::translate(qreal x, qreal y)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(x) || !qIsFinite(y))
        return ;

    QTransform newTransform = state.matrix;
    newTransform.translate(x, y);

    if (!newTransform.isInvertible()) {
        state.invertibleCTM = false;
        return;
    }

    state.matrix = newTransform;
    buffer()->updateMatrix(state.matrix);
    m_path = QTransform().translate(-x, -y).map(m_path);
}

void QQuickContext2D::transform(qreal a, qreal b, qreal c, qreal d, qreal e, qreal f)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(a) || !qIsFinite(b) || !qIsFinite(c) || !qIsFinite(d) || !qIsFinite(e) || !qIsFinite(f))
        return;

    QTransform transform(a, b, c, d, e, f);
    QTransform newTransform = state.matrix * transform;

    if (!newTransform.isInvertible()) {
        state.invertibleCTM = false;
        return;
    }
    state.matrix = newTransform;
    buffer()->updateMatrix(state.matrix);
    m_path = transform.inverted().map(m_path);
}

void QQuickContext2D::setTransform(qreal a, qreal b, qreal c, qreal d, qreal e, qreal f)
{
    if (!qIsFinite(a) || !qIsFinite(b) || !qIsFinite(c) || !qIsFinite(d) || !qIsFinite(e) || !qIsFinite(f))
        return;

    QTransform ctm = state.matrix;
    if (!ctm.isInvertible())
        return;

    state.matrix = ctm.inverted() * state.matrix;
    m_path = ctm.map(m_path);
    state.invertibleCTM = true;
    transform(a, b, c, d, e, f);
}

void QQuickContext2D::fill()
{
    if (!state.invertibleCTM)
        return;

    if (!m_path.elementCount())
        return;

    m_path.setFillRule(state.fillRule);
    buffer()->fill(m_path);
}

void QQuickContext2D::clip()
{
    if (!state.invertibleCTM)
        return;

    QPainterPath clipPath = m_path;
    clipPath.closeSubpath();
    if (!state.clipPath.isEmpty())
        state.clipPath = clipPath.intersected(state.clipPath);
    else
        state.clipPath = clipPath;
    buffer()->clip(state.clipPath);
}

void QQuickContext2D::stroke()
{
    if (!state.invertibleCTM)
        return;

    if (!m_path.elementCount())
        return;

    buffer()->stroke(m_path);
}

void QQuickContext2D::fillRect(qreal x, qreal y, qreal w, qreal h)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(x) || !qIsFinite(y) || !qIsFinite(w) || !qIsFinite(h))
        return;

    buffer()->fillRect(QRectF(x, y, w, h));
}

void QQuickContext2D::strokeRect(qreal x, qreal y, qreal w, qreal h)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(x) || !qIsFinite(y) || !qIsFinite(w) || !qIsFinite(h))
        return;

    buffer()->strokeRect(QRectF(x, y, w, h));
}

void QQuickContext2D::clearRect(qreal x, qreal y, qreal w, qreal h)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(x) || !qIsFinite(y) || !qIsFinite(w) || !qIsFinite(h))
        return;

    buffer()->clearRect(QRectF(x, y, w, h));
}

void QQuickContext2D::drawText(const QString& text, qreal x, qreal y, bool fill)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(x) || !qIsFinite(y))
        return;

    QPainterPath textPath = createTextGlyphs(x, y, text);
    if (fill)
        buffer()->fill(textPath);
    else
        buffer()->stroke(textPath);
}


void QQuickContext2D::beginPath()
{
    if (!m_path.elementCount())
        return;
    m_path = QPainterPath();
}

void QQuickContext2D::closePath()
{
    if (!m_path.elementCount())
        return;

    QRectF boundRect = m_path.boundingRect();
    if (boundRect.width() || boundRect.height())
        m_path.closeSubpath();
    //FIXME:QPainterPath set the current point to (0,0) after close subpath
    //should be the first point of the previous subpath
}

void QQuickContext2D::moveTo( qreal x, qreal y)
{
    if (!state.invertibleCTM)
        return;

    //FIXME: moveTo should not close the previous subpath
    m_path.moveTo(QPointF(x, y));
}

void QQuickContext2D::lineTo( qreal x, qreal y)
{
    if (!state.invertibleCTM)
        return;

    QPointF pt(x, y);

    if (!m_path.elementCount())
        m_path.moveTo(pt);
    else if (m_path.currentPosition() != pt)
        m_path.lineTo(pt);
}

void QQuickContext2D::quadraticCurveTo(qreal cpx, qreal cpy,
                                           qreal x, qreal y)
{
    if (!state.invertibleCTM)
        return;

    if (!m_path.elementCount())
        m_path.moveTo(QPointF(cpx, cpy));

    QPointF pt(x, y);
    if (m_path.currentPosition() != pt)
        m_path.quadTo(QPointF(cpx, cpy), pt);
}

void QQuickContext2D::bezierCurveTo(qreal cp1x, qreal cp1y,
                                        qreal cp2x, qreal cp2y,
                                        qreal x, qreal y)
{
    if (!state.invertibleCTM)
        return;

    if (!m_path.elementCount())
        m_path.moveTo(QPointF(cp1x, cp1y));

    QPointF pt(x, y);
    if (m_path.currentPosition() != pt)
        m_path.cubicTo(QPointF(cp1x, cp1y), QPointF(cp2x, cp2y),  pt);
}

void QQuickContext2D::addArcTo(const QPointF& p1, const QPointF& p2, float radius)
{
    QPointF p0(m_path.currentPosition());

    QPointF p1p0((p0.x() - p1.x()), (p0.y() - p1.y()));
    QPointF p1p2((p2.x() - p1.x()), (p2.y() - p1.y()));
    float p1p0_length = qSqrt(p1p0.x() * p1p0.x() + p1p0.y() * p1p0.y());
    float p1p2_length = qSqrt(p1p2.x() * p1p2.x() + p1p2.y() * p1p2.y());

    double cos_phi = (p1p0.x() * p1p2.x() + p1p0.y() * p1p2.y()) / (p1p0_length * p1p2_length);

    // The points p0, p1, and p2 are on the same straight line (HTML5, 4.8.11.1.8)
    // We could have used areCollinear() here, but since we're reusing
    // the variables computed above later on we keep this logic.
    if (qFuzzyCompare(qAbs(cos_phi), 1.0)) {
        m_path.lineTo(p1);
        return;
    }

    float tangent = radius / tan(acos(cos_phi) / 2);
    float factor_p1p0 = tangent / p1p0_length;
    QPointF t_p1p0((p1.x() + factor_p1p0 * p1p0.x()), (p1.y() + factor_p1p0 * p1p0.y()));

    QPointF orth_p1p0(p1p0.y(), -p1p0.x());
    float orth_p1p0_length = sqrt(orth_p1p0.x() * orth_p1p0.x() + orth_p1p0.y() * orth_p1p0.y());
    float factor_ra = radius / orth_p1p0_length;

    // angle between orth_p1p0 and p1p2 to get the right vector orthographic to p1p0
    double cos_alpha = (orth_p1p0.x() * p1p2.x() + orth_p1p0.y() * p1p2.y()) / (orth_p1p0_length * p1p2_length);
    if (cos_alpha < 0.f)
        orth_p1p0 = QPointF(-orth_p1p0.x(), -orth_p1p0.y());

    QPointF p((t_p1p0.x() + factor_ra * orth_p1p0.x()), (t_p1p0.y() + factor_ra * orth_p1p0.y()));

    // calculate angles for addArc
    orth_p1p0 = QPointF(-orth_p1p0.x(), -orth_p1p0.y());
    float sa = acos(orth_p1p0.x() / orth_p1p0_length);
    if (orth_p1p0.y() < 0.f)
        sa = 2 * Q_PI - sa;

    // anticlockwise logic
    bool anticlockwise = false;

    float factor_p1p2 = tangent / p1p2_length;
    QPointF t_p1p2((p1.x() + factor_p1p2 * p1p2.x()), (p1.y() + factor_p1p2 * p1p2.y()));
    QPointF orth_p1p2((t_p1p2.x() - p.x()), (t_p1p2.y() - p.y()));
    float orth_p1p2_length = sqrtf(orth_p1p2.x() * orth_p1p2.x() + orth_p1p2.y() * orth_p1p2.y());
    float ea = acos(orth_p1p2.x() / orth_p1p2_length);
    if (orth_p1p2.y() < 0)
        ea = 2 * Q_PI - ea;
    if ((sa > ea) && ((sa - ea) < Q_PI))
        anticlockwise = true;
    if ((sa < ea) && ((ea - sa) > Q_PI))
        anticlockwise = true;

    arc(p.x(), p.y(), radius, sa, ea, anticlockwise);
}

void QQuickContext2D::arcTo(qreal x1, qreal y1,
                                qreal x2, qreal y2,
                                qreal radius)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(x1) || !qIsFinite(y1) || !qIsFinite(x2) || !qIsFinite(y2) || !qIsFinite(radius))
        return;

    QPointF st(x1, y1);
    QPointF end(x2, y2);

    if (!m_path.elementCount())
        m_path.moveTo(st);
    else if (st == m_path.currentPosition() || st == end || !radius)
        lineTo(x1, y1);
    else
        addArcTo(st, end, radius);
 }

void QQuickContext2D::rect(qreal x, qreal y, qreal w, qreal h)
{
    if (!state.invertibleCTM)
        return;
    if (!qIsFinite(x) || !qIsFinite(y) || !qIsFinite(w) || !qIsFinite(h))
        return;

    if (!w && !h) {
        m_path.moveTo(x, y);
        return;
    }
    m_path.addRect(x, y, w, h);
}

void QQuickContext2D::roundedRect(qreal x, qreal y,
                               qreal w, qreal h,
                               qreal xr, qreal yr)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(x) || !qIsFinite(y) || !qIsFinite(w) || !qIsFinite(h) || !qIsFinite(xr) || !qIsFinite(yr))
        return;

    if (!w && !h) {
        m_path.moveTo(x, y);
        return;
    }
    m_path.addRoundedRect(QRectF(x, y, w, h), xr, yr, Qt::AbsoluteSize);
}

void QQuickContext2D::ellipse(qreal x, qreal y,
                           qreal w, qreal h)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(x) || !qIsFinite(y) || !qIsFinite(w) || !qIsFinite(h))
        return;

    if (!w && !h) {
        m_path.moveTo(x, y);
        return;
    }

    m_path.addEllipse(x, y, w, h);
}

void QQuickContext2D::text(const QString& str, qreal x, qreal y)
{
    if (!state.invertibleCTM)
        return;

    QPainterPath path;
    path.addText(x, y, state.font, str);
    m_path.addPath(path);
}

void QQuickContext2D::arc(qreal xc, qreal yc, qreal radius, qreal sar, qreal ear, bool antiClockWise)
{
    if (!state.invertibleCTM)
        return;

    if (!qIsFinite(xc) || !qIsFinite(yc) || !qIsFinite(sar) || !qIsFinite(ear) || !qIsFinite(radius))
        return;

    if (sar == ear)
        return;


    //### HACK

    // In Qt we don't switch the coordinate system for degrees
    // and still use the 0,0 as bottom left for degrees so we need
    // to switch
    sar = -sar;
    ear = -ear;
    antiClockWise = !antiClockWise;
    //end hack

    float sa = DEGREES(sar);
    float ea = DEGREES(ear);

    double span = 0;

    double xs     = xc - radius;
    double ys     = yc - radius;
    double width  = radius*2;
    double height = radius*2;
    if ((!antiClockWise && (ea - sa >= 360)) || (antiClockWise && (sa - ea >= 360)))
        // If the anticlockwise argument is false and endAngle-startAngle is equal to or greater than 2*PI, or, if the
        // anticlockwise argument is true and startAngle-endAngle is equal to or greater than 2*PI, then the arc is the whole
        // circumference of this circle.
        span = 360;
    else {
        if (!antiClockWise && (ea < sa)) {
            span += 360;
        } else if (antiClockWise && (sa < ea)) {
            span -= 360;
        }
        //### this is also due to switched coordinate system
        // we would end up with a 0 span instead of 360
        if (!(qFuzzyCompare(span + (ea - sa) + 1, 1) &&
              qFuzzyCompare(qAbs(span), 360))) {
            span   += ea - sa;
        }
    }

    // If the path is empty, move to where the arc will start to avoid painting a line from (0,0)
    if (!m_path.elementCount())
        m_path.arcMoveTo(xs, ys, width, height, sa);
    else if (!radius) {
        m_path.lineTo(xc, yc);
        return;
    }

    m_path.arcTo(xs, ys, width, height, sa, span);
}

int baseLineOffset(QQuickContext2D::TextBaseLineType value, const QFontMetrics &metrics)
{
    int offset = 0;
    switch (value) {
    case QQuickContext2D::Top:
        break;
    case QQuickContext2D::Alphabetic:
    case QQuickContext2D::Middle:
    case QQuickContext2D::Hanging:
        offset = metrics.ascent();
        break;
    case QQuickContext2D::Bottom:
        offset = metrics.height();
       break;
    }
    return offset;
}

static int textAlignOffset(QQuickContext2D::TextAlignType value, const QFontMetrics &metrics, const QString &text)
{
    int offset = 0;
    if (value == QQuickContext2D::Start)
        value = QGuiApplication::layoutDirection() == Qt::LeftToRight ? QQuickContext2D::Left : QQuickContext2D::Right;
    else if (value == QQuickContext2D::End)
        value = QGuiApplication::layoutDirection() == Qt::LeftToRight ? QQuickContext2D::Right: QQuickContext2D::Left;
    switch (value) {
    case QQuickContext2D::Center:
        offset = metrics.width(text)/2;
        break;
    case QQuickContext2D::Right:
        offset = metrics.width(text);
    case QQuickContext2D::Left:
    default:
        break;
    }
    return offset;
}

void QQuickContext2D::setGrabbedImage(const QImage& grab)
{
    m_grabbedImage = grab;
    m_grabbed = true;
}

QQmlRefPointer<QQuickCanvasPixmap> QQuickContext2D::createPixmap(const QUrl& url)
{
    return m_canvas->loadedPixmap(url);
}

QPainterPath QQuickContext2D::createTextGlyphs(qreal x, qreal y, const QString& text)
{
    const QFontMetrics metrics(state.font);
    int yoffset = baseLineOffset(static_cast<QQuickContext2D::TextBaseLineType>(state.textBaseline), metrics);
    int xoffset = textAlignOffset(static_cast<QQuickContext2D::TextAlignType>(state.textAlign), metrics, text);

    QPainterPath textPath;

    textPath.addText(x - xoffset, y - yoffset+metrics.ascent(), state.font, text);
    textPath = state.matrix.map(textPath);
    return textPath;
}


static inline bool areCollinear(const QPointF& a, const QPointF& b, const QPointF& c)
{
    // Solved from comparing the slopes of a to b and b to c: (ay-by)/(ax-bx) == (cy-by)/(cx-bx)
    return qFuzzyCompare((c.y() - b.y()) * (a.x() - b.x()), (a.y() - b.y()) * (c.x() - b.x()));
}

static inline bool withinRange(qreal p, qreal a, qreal b)
{
    return (p >= a && p <= b) || (p >= b && p <= a);
}

bool QQuickContext2D::isPointInPath(qreal x, qreal y) const
{
    if (!state.invertibleCTM)
        return false;

    if (!m_path.elementCount())
        return false;

    if (!qIsFinite(x) || !qIsFinite(y))
        return false;

    QPointF point(x, y);
    QTransform ctm = state.matrix;
    QPointF p = ctm.inverted().map(point);
    if (!qIsFinite(p.x()) || !qIsFinite(p.y()))
        return false;

    const_cast<QQuickContext2D *>(this)->m_path.setFillRule(state.fillRule);

    bool contains = m_path.contains(p);

    if (!contains) {
        // check whether the point is on the border
        QPolygonF border = m_path.toFillPolygon();

        QPointF p1 = border.at(0);
        QPointF p2;

        for (int i = 1; i < border.size(); ++i) {
            p2 = border.at(i);
            if (areCollinear(p, p1, p2)
                    // Once we know that the points are collinear we
                    // only need to check one of the coordinates
                    && (qAbs(p2.x() - p1.x()) > qAbs(p2.y() - p1.y()) ?
                        withinRange(p.x(), p1.x(), p2.x()) :
                        withinRange(p.y(), p1.y(), p2.y()))) {
                return true;
            }
            p1 = p2;
        }
    }
    return contains;
}

QQuickContext2D::QQuickContext2D(QObject *parent)
    : QQuickCanvasContext(parent)
    , m_buffer(new QQuickContext2DCommandBuffer)
    , m_v8engine(0)
    , m_surface(0)
    , m_glContext(0)
    , m_thread(0)
    , m_grabbed(false)
{
}

QQuickContext2D::~QQuickContext2D()
{
    delete m_buffer;
    m_texture->deleteLater();
}

QV4::Value QQuickContext2D::v4value() const
{
    return m_v4value.value();
}

QStringList QQuickContext2D::contextNames() const
{
    return QStringList() << QStringLiteral("2d");
}

void QQuickContext2D::init(QQuickCanvasItem *canvasItem, const QVariantMap &args)
{
    Q_UNUSED(args);

    m_canvas = canvasItem;
    m_renderTarget = canvasItem->renderTarget();

    QQuickWindow *window = canvasItem->window();
    m_renderStrategy = canvasItem->renderStrategy();

    switch (m_renderTarget) {
    case QQuickCanvasItem::Image:
        m_texture = new QQuickContext2DImageTexture;
        break;
    case QQuickCanvasItem::FramebufferObject:
        m_texture = new QQuickContext2DFBOTexture;
        break;
    }

    m_texture->setItem(canvasItem);
    m_texture->setCanvasWindow(canvasItem->canvasWindow().toRect());
    m_texture->setTileSize(canvasItem->tileSize());
    m_texture->setCanvasSize(canvasItem->canvasSize().toSize());
    m_texture->setSmooth(canvasItem->smooth());
    m_texture->setAntialiasing(canvasItem->antialiasing());
    m_thread = QThread::currentThread();

    QThread *renderThread = m_thread;
    QThread *sceneGraphThread = window->openglContext() ? window->openglContext()->thread() : 0;

    if (m_renderStrategy == QQuickCanvasItem::Threaded)
        renderThread = QQuickContext2DRenderThread::instance(qmlEngine(canvasItem));
    else if (m_renderStrategy == QQuickCanvasItem::Cooperative)
        renderThread = sceneGraphThread;

    if (renderThread && renderThread != QThread::currentThread())
        m_texture->moveToThread(renderThread);

    if (m_renderTarget == QQuickCanvasItem::FramebufferObject && renderThread != sceneGraphThread) {
         QOpenGLContext *cc = QQuickWindowPrivate::get(window)->context->glContext();
         m_surface = window;
         m_glContext = new QOpenGLContext;
         m_glContext->setFormat(cc->format());
         m_glContext->setShareContext(cc);
         if (renderThread != QThread::currentThread())
             m_glContext->moveToThread(renderThread);
    }

    connect(m_texture, SIGNAL(textureChanged()), SIGNAL(textureChanged()));

    reset();
}

void QQuickContext2D::prepare(const QSize& canvasSize, const QSize& tileSize, const QRect& canvasWindow, const QRect& dirtyRect, bool smooth, bool antialiasing)
{
    QMetaObject::invokeMethod(m_texture
                                                , "canvasChanged"
                                                , Qt::AutoConnection
                                                , Q_ARG(QSize, canvasSize)
                                                , Q_ARG(QSize, tileSize)
                                                , Q_ARG(QRect, canvasWindow)
                                                , Q_ARG(QRect, dirtyRect)
                                                , Q_ARG(bool, smooth)
                                                , Q_ARG(bool, antialiasing));
}

void QQuickContext2D::flush()
{
    if (m_buffer)
        QMetaObject::invokeMethod(m_texture,
                                                                   "paint",
                                                                   Qt::AutoConnection,
                                                                   Q_ARG(QQuickContext2DCommandBuffer*, m_buffer));
    m_buffer = new QQuickContext2DCommandBuffer();
}

QSGDynamicTexture *QQuickContext2D::texture() const
{
    return m_texture;
}

QImage QQuickContext2D::toImage(const QRectF& bounds)
{
    flush();
    if (m_texture->thread() == QThread::currentThread())
        m_texture->grabImage(bounds);
    else if (m_renderStrategy == QQuickCanvasItem::Cooperative) {
        qWarning() << "Pixel read back is not support in Cooperative mode, please try Theaded or Immediate mode";
        return QImage();
    } else {
        QMetaObject::invokeMethod(m_texture,
                                  "grabImage",
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(QRectF, bounds));
    }
    QImage img = m_grabbedImage;
    m_grabbedImage = QImage();
    m_grabbed = false;
    return img;
}


QQuickContext2DEngineData::QQuickContext2DEngineData(QV8Engine *engine)
{
    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);

    QV4::Object *proto = new (v4->memoryManager) QQuickJSContext2DPrototype(v4);
    proto->defineAccessorProperty(v4, QStringLiteral("strokeStyle"), QQuickJSContext2D::method_get_strokeStyle, QQuickJSContext2D::method_set_strokeStyle);
    proto->defineAccessorProperty(v4, QStringLiteral("font"), QQuickJSContext2D::method_get_font, QQuickJSContext2D::method_set_font);
    proto->defineAccessorProperty(v4, QStringLiteral("fillRule"), QQuickJSContext2D::method_get_fillRule, QQuickJSContext2D::method_set_fillRule);
    proto->defineAccessorProperty(v4, QStringLiteral("globalAlpha"), QQuickJSContext2D::method_get_globalAlpha, QQuickJSContext2D::method_set_globalAlpha);
    proto->defineAccessorProperty(v4, QStringLiteral("lineCap"), QQuickJSContext2D::method_get_lineCap, QQuickJSContext2D::method_set_lineCap);
    proto->defineAccessorProperty(v4, QStringLiteral("shadowOffsetX"), QQuickJSContext2D::method_get_shadowOffsetX, QQuickJSContext2D::method_set_shadowOffsetX);
    proto->defineAccessorProperty(v4, QStringLiteral("shadowOffsetY"), QQuickJSContext2D::method_get_shadowOffsetY, QQuickJSContext2D::method_set_shadowOffsetY);
    proto->defineAccessorProperty(v4, QStringLiteral("globalCompositeOperation"), QQuickJSContext2D::method_get_globalCompositeOperation, QQuickJSContext2D::method_set_globalCompositeOperation);
    proto->defineAccessorProperty(v4, QStringLiteral("miterLimit"), QQuickJSContext2D::method_get_miterLimit, QQuickJSContext2D::method_set_miterLimit);
    proto->defineAccessorProperty(v4, QStringLiteral("fillStyle"), QQuickJSContext2D::method_get_fillStyle, QQuickJSContext2D::method_set_fillStyle);
    proto->defineAccessorProperty(v4, QStringLiteral("shadowColor"), QQuickJSContext2D::method_get_shadowColor, QQuickJSContext2D::method_set_shadowColor);
    proto->defineAccessorProperty(v4, QStringLiteral("textBaseline"), QQuickJSContext2D::method_get_textBaseline, QQuickJSContext2D::method_set_textBaseline);
    proto->defineAccessorProperty(v4, QStringLiteral("path"), QQuickJSContext2D::method_get_path, QQuickJSContext2D::method_set_path);
    proto->defineAccessorProperty(v4, QStringLiteral("lineJoin"), QQuickJSContext2D::method_get_lineJoin, QQuickJSContext2D::method_set_lineJoin);
    proto->defineAccessorProperty(v4, QStringLiteral("lineWidth"), QQuickJSContext2D::method_get_lineWidth, QQuickJSContext2D::method_set_lineWidth);
    proto->defineAccessorProperty(v4, QStringLiteral("textAlign"), QQuickJSContext2D::method_get_textAlign, QQuickJSContext2D::method_set_textAlign);
    proto->defineAccessorProperty(v4, QStringLiteral("shadowBlur"), QQuickJSContext2D::method_get_shadowBlur, QQuickJSContext2D::method_set_shadowBlur);
    contextPrototype = QV4::Value::fromObject(proto);

    proto = v4->newObject();
    proto->defineDefaultProperty(v4, QStringLiteral("addColorStop"), QQuickContext2DStyle::gradient_proto_addColorStop, 0);
    gradientProto = QV4::Value::fromObject(proto);

    proto = v4->newObject();
    proto->defineAccessorProperty(v4->id_length, QQuickJSContext2DPixelData::proto_get_length, 0);
    pixelArrayProto = QV4::Value::fromObject(proto);
}

QQuickContext2DEngineData::~QQuickContext2DEngineData()
{
}

void QQuickContext2D::popState()
{
    if (m_stateStack.isEmpty())
        return;

    QQuickContext2D::State newState = m_stateStack.pop();

    if (state.matrix != newState.matrix)
        buffer()->updateMatrix(newState.matrix);

    if (newState.globalAlpha != state.globalAlpha)
        buffer()->setGlobalAlpha(newState.globalAlpha);

    if (newState.globalCompositeOperation != state.globalCompositeOperation)
        buffer()->setGlobalCompositeOperation(newState.globalCompositeOperation);

    if (newState.fillStyle != state.fillStyle)
        buffer()->setFillStyle(newState.fillStyle);

    if (newState.strokeStyle != state.strokeStyle)
        buffer()->setStrokeStyle(newState.strokeStyle);

    if (newState.lineWidth != state.lineWidth)
        buffer()->setLineWidth(newState.lineWidth);

    if (newState.lineCap != state.lineCap)
        buffer()->setLineCap(newState.lineCap);

    if (newState.lineJoin != state.lineJoin)
        buffer()->setLineJoin(newState.lineJoin);

    if (newState.miterLimit != state.miterLimit)
        buffer()->setMiterLimit(newState.miterLimit);

    if (newState.clipPath != state.clipPath) {
        buffer()->clip(newState.clipPath);
    }

    if (newState.shadowBlur != state.shadowBlur)
        buffer()->setShadowBlur(newState.shadowBlur);

    if (newState.shadowColor != state.shadowColor)
        buffer()->setShadowColor(newState.shadowColor);

    if (newState.shadowOffsetX != state.shadowOffsetX)
        buffer()->setShadowOffsetX(newState.shadowOffsetX);

    if (newState.shadowOffsetY != state.shadowOffsetY)
        buffer()->setShadowOffsetY(newState.shadowOffsetY);
    m_path = state.matrix.map(m_path);
    state = newState;
    m_path = state.matrix.inverted().map(m_path);
}
void QQuickContext2D::pushState()
{
    m_stateStack.push(state);
}

void QQuickContext2D::reset()
{
    QQuickContext2D::State newState;
    newState.matrix = QTransform();

    m_path = QPainterPath();

    QPainterPath defaultClipPath;

    QRect r(0, 0, m_canvas->canvasSize().width(), m_canvas->canvasSize().height());
    r = r.united(m_canvas->canvasWindow().toRect());
    defaultClipPath.addRect(r);
    newState.clipPath = defaultClipPath;
    newState.clipPath.setFillRule(Qt::WindingFill);

    newState.strokeStyle = QColor("#000000");
    newState.fillStyle = QColor("#000000");
    newState.fillPatternRepeatX = false;
    newState.fillPatternRepeatY = false;
    newState.strokePatternRepeatX = false;
    newState.strokePatternRepeatY = false;
    newState.invertibleCTM = true;
    newState.fillRule = Qt::WindingFill;
    newState.globalAlpha = 1.0;
    newState.lineWidth = 1;
    newState.lineCap = Qt::FlatCap;
    newState.lineJoin = Qt::MiterJoin;
    newState.miterLimit = 10;
    newState.shadowOffsetX = 0;
    newState.shadowOffsetY = 0;
    newState.shadowBlur = 0;
    newState.shadowColor = qRgba(0, 0, 0, 0);
    newState.globalCompositeOperation = QPainter::CompositionMode_SourceOver;
    newState.font = QFont(QStringLiteral("sans-serif"), 10);
    newState.textAlign = QQuickContext2D::Start;
    newState.textBaseline = QQuickContext2D::Alphabetic;

    m_stateStack.clear();
    m_stateStack.push(newState);
    popState();
    m_buffer->clearRect(QRectF(0, 0, m_canvas->width(), m_canvas->height()));
}

void QQuickContext2D::setV8Engine(QV8Engine *engine)
{
    if (m_v8engine != engine) {
        m_v8engine = engine;

        if (m_v8engine == 0)
            return;

        QQuickContext2DEngineData *ed = engineData(engine);
        QV4::ExecutionEngine *v4Engine = QV8Engine::getV4(engine);
        QQuickJSContext2D *wrapper = new (v4Engine->memoryManager) QQuickJSContext2D(v4Engine);
        wrapper->prototype = ed->contextPrototype.value().asObject();
        wrapper->context = this;
        m_v4value = QV4::Value::fromObject(wrapper);
    }
}

QQuickContext2DCommandBuffer* QQuickContext2D::nextBuffer()
{
    QMutexLocker lock(&m_mutex);
    return m_bufferQueue.isEmpty() ? 0 : m_bufferQueue.dequeue();
}

QT_END_NAMESPACE
