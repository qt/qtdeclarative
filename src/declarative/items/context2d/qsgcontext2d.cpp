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

#include "qsgcontext2d_p.h"
#include "qsgcontext2dcommandbuffer_p.h"
#include "qsgcanvasitem_p.h"
#include "qsgitem_p.h"
#include "qsgshadereffectsource_p.h"
#include <QtGui/qopenglframebufferobject.h>

#include <QtCore/qdebug.h>
#include "private/qsgcontext_p.h"
#include "private/qdeclarativesvgparser_p.h"
#include "private/qdeclarativepath_p.h"

#include "private/qsgimage_p_p.h"

#include <QtGui/qguiapplication.h>
#include <qdeclarativeinfo.h>
#include <QtCore/qmath.h>
#include "qv8engine_p.h"

#include "qdeclarativeengine.h"
QT_BEGIN_NAMESPACE
/*!
    \qmlclass Context2D QSGContext2D
    \inqmlmodule QtQuick 2
    \since QtQuick 2.0
    \brief The Context2D element allows you to draw 2d graphic shapes on Canvas item.
*/
static const double Q_PI   = 3.14159265358979323846;   // pi


static bool parsePathDataFast(const QString &dataStr, QPainterPath &path);

#define DEGREES(t) ((t) * 180.0 / Q_PI)
#define qClamp(val, min, max) qMin(qMax(val, min), max)

#define CHECK_CONTEXT(r)     if (!r || !r->context || !r->context->buffer()) \
                                V8THROW_ERROR("Not a Context2D object");

#define CHECK_CONTEXT_SETTER(r)     if (!r || !r->context || !r->context->buffer()) \
                                       V8THROW_ERROR_SETTER("Not a Context2D object");

static inline int extractInt(const char **name)
{
    int result = 0;
    bool negative = false;

    //eat leading whitespace
    while (isspace(*name[0]))
        ++*name;

    if (*name[0] == '-') {
        ++*name;
        negative = true;
    } /*else if (name[0] == '+')
        ++name;     //ignore*/

    //construct number
    while (isdigit(*name[0])) {
        result = result * 10 + (*name[0] - '0');
        ++*name;
    }
    if (negative)
        result = -result;

    //handle optional percentage
    if (*name[0] == '%')
        result *= qreal(255)/100;  //### floor or round?

    //eat trailing whitespace
    while (isspace(*name[0]))
        ++*name;

    return result;
}

static bool qt_get_rgb(const QString &string, QRgb *rgb)
{
    const char *name = string.toLatin1().constData();
    int len = qstrlen(name);

    if (len < 5)
        return false;

    bool handleAlpha = false;

    if (name[0] != 'r')
        return false;
    if (name[1] != 'g')
        return false;
    if (name[2] != 'b')
        return false;
    if (name[3] == 'a') {
        handleAlpha = true;
        if(name[3] != '(')
            return false;
    } else if (name[3] != '(')
        return false;

    name += 4;

    int r, g, b, a = 1;
    int result;

    //red
    result = extractInt(&name);
    if (name[0] == ',') {
        r = result;
        ++name;
    } else
        return false;

    //green
    result = extractInt(&name);
    if (name[0] == ',') {
        g = result;
        ++name;
    } else
        return false;

    char nextChar = handleAlpha ? ',' : ')';

    //blue
    result = extractInt(&name);
    if (name[0] == nextChar) {
        b = result;
        ++name;
    } else
        return false;

    //alpha
    if (handleAlpha) {
        result = extractInt(&name);
        if (name[0] == ')') {
            a = result * 255;   //map 0-1 to 0-255
            ++name;
        } else
            return false;
    }

    if (name[0] != '\0')
        return false;

    *rgb = qRgba(qClamp(r,0,255), qClamp(g,0,255), qClamp(b,0,255), qClamp(a,0,255));
    return true;
}

//### unify with qt_get_rgb?
static bool qt_get_hsl(const QString &string, QColor *color)
{
    const char *name = string.toLatin1().constData();
    int len = qstrlen(name);

    if (len < 5)
        return false;

    bool handleAlpha = false;

    if (name[0] != 'h')
        return false;
    if (name[1] != 's')
        return false;
    if (name[2] != 'l')
        return false;
    if (name[3] == 'a') {
        handleAlpha = true;
        if(name[3] != '(')
            return false;
    } else if (name[3] != '(')
        return false;

    name += 4;

    int h, s, l, a = 1;
    int result;

    //hue
    result = extractInt(&name);
    if (name[0] == ',') {
        h = result;
        ++name;
    } else
        return false;

    //saturation
    result = extractInt(&name);
    if (name[0] == ',') {
        s = result;
        ++name;
    } else
        return false;

    char nextChar = handleAlpha ? ',' : ')';

    //lightness
    result = extractInt(&name);
    if (name[0] == nextChar) {
        l = result;
        ++name;
    } else
        return false;

    //alpha
    if (handleAlpha) {
        result = extractInt(&name);
        if (name[0] == ')') {
            a = result * 255;   //map 0-1 to 0-255
            ++name;
        } else
            return false;
    }

    if (name[0] != '\0')
        return false;

    *color = QColor::fromHsl(qClamp(h,0,255), qClamp(s,0,255), qClamp(l,0,255), qClamp(a,0,255));
    return true;
}

//### optimize further
QColor qt_color_from_string(const QString &name)
{
    if (name.startsWith(QLatin1String("rgb"))) {
        QRgb rgb;
        if (qt_get_rgb(name, &rgb))
            return QColor(rgb);
    } else if (name.startsWith(QLatin1String("hsl"))) {
        QColor color;
        if (qt_get_hsl(name, &color))
            return color;
    }

    return QColor(name);
}

QFont qt_font_from_string(const QString& fontString) {
    QFont font;
     // ### this is simplified and incomplete
    // ### TODO:get code from Qt webkit
     QStringList tokens = fontString.split(QLatin1String(" "));
     foreach (const QString &token, tokens) {
         if (token == QLatin1String("italic"))
             font.setItalic(true);
         else if (token == QLatin1String("bold"))
             font.setBold(true);
         else if (token.endsWith(QLatin1String("px"))) {
             QString number = token;
             number.remove(QLatin1String("px"));
             //font.setPointSizeF(number.trimmed().toFloat());
             font.setPixelSize(number.trimmed().toInt());
         } else
             font.setFamily(token);
     }

     return font;
}



class QSGContext2DEngineData : public QV8Engine::Deletable
{
public:
    QSGContext2DEngineData(QV8Engine *engine);
    ~QSGContext2DEngineData();

    v8::Persistent<v8::Function> constructorContext;
    v8::Persistent<v8::Function> constructorGradient;
    v8::Persistent<v8::Function> constructorPattern;
    v8::Persistent<v8::Function> constructorPixelArray;
    v8::Persistent<v8::Function> constructorImageData;
};
V8_DEFINE_EXTENSION(QSGContext2DEngineData, engineData);



class QV8Context2DResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(Context2DType)
public:
    QV8Context2DResource(QV8Engine *e) : QV8ObjectResource(e) {}
    QSGContext2D* context;
};

class QV8Context2DStyleResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(Context2DStyleType)
public:
    QV8Context2DStyleResource(QV8Engine *e) : QV8ObjectResource(e) {}
    QBrush brush;
};

class QV8Context2DPixelArrayResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(Context2DPixelArrayType)
public:
    QV8Context2DPixelArrayResource(QV8Engine *e) : QV8ObjectResource(e) {}

    QImage image;
};

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
    if (compositeOperator == QLatin1String("source-over")) {
        return QPainter::CompositionMode_SourceOver;
    } else if (compositeOperator == QLatin1String("source-out")) {
        return QPainter::CompositionMode_SourceOut;
    } else if (compositeOperator == QLatin1String("source-in")) {
        return QPainter::CompositionMode_SourceIn;
    } else if (compositeOperator == QLatin1String("source-atop")) {
        return QPainter::CompositionMode_SourceAtop;
    } else if (compositeOperator == QLatin1String("destination-atop")) {
        return QPainter::CompositionMode_DestinationAtop;
    } else if (compositeOperator == QLatin1String("destination-in")) {
        return QPainter::CompositionMode_DestinationIn;
    } else if (compositeOperator == QLatin1String("destination-out")) {
        return QPainter::CompositionMode_DestinationOut;
    } else if (compositeOperator == QLatin1String("destination-over")) {
        return QPainter::CompositionMode_DestinationOver;
    } else if (compositeOperator == QLatin1String("lighter")) {
        return QPainter::CompositionMode_Plus;
    } else if (compositeOperator == QLatin1String("copy")) {
        return QPainter::CompositionMode_Source;
    } else if (compositeOperator == QLatin1String("xor")) {
        return QPainter::CompositionMode_Xor;
    } else if (compositeOperator == QLatin1String("qt-clear")) {
        return QPainter::CompositionMode_Clear;
    } else if (compositeOperator == QLatin1String("qt-destination")) {
        return QPainter::CompositionMode_Destination;
    } else if (compositeOperator == QLatin1String("qt-multiply")) {
        return QPainter::CompositionMode_Multiply;
    } else if (compositeOperator == QLatin1String("qt-screen")) {
        return QPainter::CompositionMode_Screen;
    } else if (compositeOperator == QLatin1String("qt-overlay")) {
        return QPainter::CompositionMode_Overlay;
    } else if (compositeOperator == QLatin1String("qt-darken")) {
        return QPainter::CompositionMode_Darken;
    } else if (compositeOperator == QLatin1String("qt-lighten")) {
        return QPainter::CompositionMode_Lighten;
    } else if (compositeOperator == QLatin1String("qt-color-dodge")) {
        return QPainter::CompositionMode_ColorDodge;
    } else if (compositeOperator == QLatin1String("qt-color-burn")) {
        return QPainter::CompositionMode_ColorBurn;
    } else if (compositeOperator == QLatin1String("qt-hard-light")) {
        return QPainter::CompositionMode_HardLight;
    } else if (compositeOperator == QLatin1String("qt-soft-light")) {
        return QPainter::CompositionMode_SoftLight;
    } else if (compositeOperator == QLatin1String("qt-difference")) {
        return QPainter::CompositionMode_Difference;
    } else if (compositeOperator == QLatin1String("qt-exclusion")) {
        return QPainter::CompositionMode_Exclusion;
    }
    return QPainter::CompositionMode_SourceOver;
}

static QString qt_composite_mode_to_string(QPainter::CompositionMode op)
{
    switch (op) {
    case QPainter::CompositionMode_SourceOver:
        return QLatin1String("source-over");
    case QPainter::CompositionMode_DestinationOver:
        return QLatin1String("destination-over");
    case QPainter::CompositionMode_Clear:
        return QLatin1String("qt-clear");
    case QPainter::CompositionMode_Source:
        return QLatin1String("copy");
    case QPainter::CompositionMode_Destination:
        return QLatin1String("qt-destination");
    case QPainter::CompositionMode_SourceIn:
        return QLatin1String("source-in");
    case QPainter::CompositionMode_DestinationIn:
        return QLatin1String("destination-in");
    case QPainter::CompositionMode_SourceOut:
        return QLatin1String("source-out");
    case QPainter::CompositionMode_DestinationOut:
        return QLatin1String("destination-out");
    case QPainter::CompositionMode_SourceAtop:
        return QLatin1String("source-atop");
    case QPainter::CompositionMode_DestinationAtop:
        return QLatin1String("destination-atop");
    case QPainter::CompositionMode_Xor:
        return QLatin1String("xor");
    case QPainter::CompositionMode_Plus:
        return QLatin1String("lighter");
    case QPainter::CompositionMode_Multiply:
        return QLatin1String("qt-multiply");
    case QPainter::CompositionMode_Screen:
        return QLatin1String("qt-screen");
    case QPainter::CompositionMode_Overlay:
        return QLatin1String("qt-overlay");
    case QPainter::CompositionMode_Darken:
        return QLatin1String("qt-darken");
    case QPainter::CompositionMode_Lighten:
        return QLatin1String("qt-lighten");
    case QPainter::CompositionMode_ColorDodge:
        return QLatin1String("qt-color-dodge");
    case QPainter::CompositionMode_ColorBurn:
        return QLatin1String("qt-color-burn");
    case QPainter::CompositionMode_HardLight:
        return QLatin1String("qt-hard-light");
    case QPainter::CompositionMode_SoftLight:
        return QLatin1String("qt-soft-light");
    case QPainter::CompositionMode_Difference:
        return QLatin1String("qt-difference");
    case QPainter::CompositionMode_Exclusion:
        return QLatin1String("qt-exclusion");
    default:
        break;
    }
    return QString();
}


static v8::Local<v8::Object> qt_create_image_data(qreal w, qreal h, QV8Engine* engine, const QImage& image)
{
    QSGContext2DEngineData *ed = engineData(engine);
    v8::Local<v8::Object> imageData = ed->constructorImageData->NewInstance();
    QV8Context2DPixelArrayResource *r = new QV8Context2DPixelArrayResource(engine);
    if (image.isNull()) {
        r->image = QImage(w, h, QImage::Format_ARGB32);
        r->image.fill(Qt::transparent);
    } else {
        Q_ASSERT(image.width() == w && image.height() == h);
        r->image = image;
    }
    v8::Local<v8::Object> pixelData = ed->constructorPixelArray->NewInstance();
    pixelData->SetExternalResource(r);

    imageData->SetInternalField(0, pixelData);
    return imageData;
}

//static script functions

/*!
    \qmlproperty QtQuick2::Canvas QtQuick2::Context2D::canvas
     Holds the canvas item that the context paints on.
*/
static v8::Handle<v8::Value> ctx2d_canvas(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE_ACCESSOR();

    return engine->newQObject(r->context->canvas());
}

/*!
    \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::restore()
    Pops the top state on the stack, restoring the context to that state.
*/
static v8::Handle<v8::Value> ctx2d_restore(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)

    r->context->popState();
    return args.This();
}

/*!
    \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::reset()
    Resets the context state and properties to the default values.
*/
static v8::Handle<v8::Value> ctx2d_reset(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)

    r->context->reset();
    return args.This();
}

/*!
    \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::save()
    Pushes the current state onto the stack.
*/
static v8::Handle<v8::Value> ctx2d_save(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)

    r->context->pushState();

    return args.This();
}

// transformations
/*!
    \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::rotate(real angle)
    Changes the transformation matrix to apply a rotation transformation with the given characteristics.
    Note: The angle is in radians.
*/
static v8::Handle<v8::Value> ctx2d_rotate(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 1)  {
        qreal angle = args[0]->NumberValue();
        r->context->state.matrix.rotate(DEGREES(angle));
        r->context->buffer()->updateMatrix(r->context->state.matrix);
    }

    return args.This();
}

/*!
    \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::scale(real x, real y)
    Changes the transformation matrix to apply a scaling transformation with the given characteristics.
*/
static v8::Handle<v8::Value> ctx2d_scale(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 2) {
        qreal x, y;
        x = args[0]->NumberValue();
        y = args[1]->NumberValue();
        r->context->state.matrix.scale(x, y);
        r->context->buffer()->updateMatrix(r->context->state.matrix);
    }

    return args.This();
}

/*!
    \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::setTransform(real a, real b, real c, real d, real e, real f)
    Changes the transformation matrix to the matrix given by the arguments as described below.

    \sa QtQuick2::Context2D::transform()
*/
static v8::Handle<v8::Value> ctx2d_setTransform(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 6) {
        r->context->state.matrix = QTransform(args[0]->NumberValue(),
                                              args[1]->NumberValue(),
                                              args[2]->NumberValue(),
                                              args[3]->NumberValue(),
                                              args[4]->NumberValue(),
                                              args[5]->NumberValue());
        r->context->buffer()->updateMatrix(r->context->state.matrix);
    }

    return args.This();
}

/*!
    \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::transform(real a, real b, real c, real d, real e, real f)
    Changes the transformation matrix to apply the matrix given by the arguments as described below.

    \sa QtQuick2::Context2D::setTransform()
*/
static v8::Handle<v8::Value> ctx2d_transform(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 6) {
        r->context->state.matrix *= QTransform(args[0]->NumberValue(),
                                               args[1]->NumberValue(),
                                               args[2]->NumberValue(),
                                               args[3]->NumberValue(),
                                               args[4]->NumberValue(),
                                               args[5]->NumberValue());
        r->context->buffer()->updateMatrix(r->context->state.matrix);
    }

    return args.This();
}

/*!
    \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::translate(real x, real y)
    Changes the transformation matrix to apply a translation transformation with the given characteristics.
*/
static v8::Handle<v8::Value> ctx2d_translate(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 2) {
        r->context->state.matrix.translate(args[0]->NumberValue(),
                                           args[1]->NumberValue());
        r->context->buffer()->updateMatrix(r->context->state.matrix);
    }

    return args.This();
}


/*!
    \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::resetTransform()
    Reset the transformation matrix default value.

    \sa QtQuick2::Context2D::transform(), QtQuick2::Context2D::setTransform()
*/
static v8::Handle<v8::Value> ctx2d_resetTransform(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)

    r->context->state.matrix = QTransform();
    r->context->buffer()->updateMatrix(r->context->state.matrix);

    return args.This();
}


/*!
    \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::shear(real sh, real sv )
    Shear the transformation matrix with \a sh in horizontal direction and \a sv in vertical direction.
*/
static v8::Handle<v8::Value> ctx2d_shear(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)

    r->context->state.matrix.shear(args[0]->NumberValue(),
                                   args[1]->NumberValue());
    r->context->buffer()->updateMatrix(r->context->state.matrix);

    return args.This();
}
// compositing

/*!
    \qmlproperty real QtQuick2::Context2D::globalAlpha
     Holds the the current alpha value applied to rendering operations.
     The value must be in the range from 0.0 (fully transparent) to 1.0 (no additional transparency).
*/
static v8::Handle<v8::Value> ctx2d_globalAlpha(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)

    return v8::Number::New(r->context->state.globalAlpha);
}

static void ctx2d_globalAlpha_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    qreal globalAlpha = value->NumberValue();

    if (globalAlpha >= 0.0 && globalAlpha <= 1.0 && r->context->state.globalAlpha != globalAlpha) {
        r->context->state.globalAlpha = globalAlpha;
        r->context->buffer()->setGlobalAlpha(r->context->state.globalAlpha);
    }
}

/*!
    \qmlproperty string QtQuick2::Context2D::globalCompositeOperation
     Holds the the current the current composition operation, from the list below:
     \list
     \o source-atop      - A atop B. Display the source image wherever both images are opaque.
                           Display the destination image wherever the destination image is opaque but the source image is transparent.
                           Display transparency elsewhere.
     \o source-in        - A in B. Display the source image wherever both the source image and destination image are opaque.
                           Display transparency elsewhere.
     \o source-out       - A out B. Display the source image wherever the source image is opaque and the destination image is transparent.
                           Display transparency elsewhere.
     \o source-over      - (default) A over B. Display the source image wherever the source image is opaque.
                           Display the destination image elsewhere.
     \o destination-atop - B atop A. Same as source-atop but using the destination image instead of the source image and vice versa.
     \o destination-in   - B in A. Same as source-in but using the destination image instead of the source image and vice versa.
     \o destination-out  - B out A. Same as source-out but using the destination image instead of the source image and vice versa.
     \o destination-over - B over A. Same as source-over but using the destination image instead of the source image and vice versa.
     \o lighter          - A plus B. Display the sum of the source image and destination image, with color values approaching 255 (100%) as a limit.
     \o copy             - A (B is ignored). Display the source image instead of the destination image.
     \o xor              - A xor B. Exclusive OR of the source image and destination image.
     \endlist
*/
static v8::Handle<v8::Value> ctx2d_globalCompositeOperation(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE_ACCESSOR();

    return engine->toString(qt_composite_mode_to_string(r->context->state.globalCompositeOperation));
}

static void ctx2d_globalCompositeOperation_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    QV8Engine *engine = V8ENGINE_ACCESSOR();

    QPainter::CompositionMode cm = qt_composite_mode_from_string(engine->toString(value));
    if (cm != r->context->state.globalCompositeOperation) {
        r->context->state.globalCompositeOperation = cm;
        r->context->buffer()->setGlobalCompositeOperation(cm);
    }
}

// colors and styles
/*!
    \qmlproperty variant QtQuick2::Context2D::fillStyle
     Holds the current style used for filling shapes.
     The style can be either a string containing a CSS color, or a CanvasGradient or CanvasPattern object. Invalid values are ignored.
     \sa QtQuick2::Context2D::createLinearGradient
     \sa QtQuick2::Context2D::createRadialGradient
     \sa QtQuick2::Context2D::createPattern
     \sa QtQuick2::Context2D::strokeStyle
 */
static v8::Handle<v8::Value> ctx2d_fillStyle(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)

    return r->context->m_fillStyle;
}

static void ctx2d_fillStyle_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    QV8Engine *engine = V8ENGINE_ACCESSOR();

   r->context->m_fillStyle = value;
   if (value->IsObject()) {
       QColor color = engine->toVariant(value, qMetaTypeId<QColor>()).value<QColor>();
       if (color.isValid()) {
           r->context->state.fillStyle = color;
           r->context->buffer()->setFillStyle(color);
       } else {
           QV8Context2DStyleResource *style = v8_resource_cast<QV8Context2DStyleResource>(value->ToObject());
           if (style && style->brush != r->context->state.fillStyle) {
               r->context->state.fillStyle = style->brush;
               r->context->buffer()->setFillStyle(style->brush);
           }
       }
   } else if (value->IsString()) {
       QColor color = qt_color_from_string(engine->toString(value));
       if (color.isValid() && r->context->state.fillStyle != QBrush(color)) {
            r->context->state.fillStyle = QBrush(color);
            r->context->buffer()->setFillStyle(r->context->state.fillStyle);
       }
   }
}

static v8::Handle<v8::Value> ctx2d_fillRule(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)
    QV8Engine *engine = V8ENGINE_ACCESSOR();

    return engine->fromVariant(r->context->state.fillRule);
}

static void ctx2d_fillRule_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    QV8Engine *engine = V8ENGINE_ACCESSOR();

    if ((value->IsString() && engine->toString(value) == "WindingFill")
      ||(value->IsNumber() && value->NumberValue() == Qt::WindingFill)) {
        r->context->state.fillRule = Qt::WindingFill;
    } else if ((value->IsString() && engine->toString(value) == "OddEvenFill")
               ||(value->IsNumber() && value->NumberValue() == Qt::OddEvenFill)) {
        r->context->state.fillRule = Qt::OddEvenFill;
    } else {
        //error
    }
    r->context->m_path.setFillRule(r->context->state.fillRule);
}
/*!
    \qmlproperty variant QtQuick2::Context2D::strokeStyle
     Holds the current color or style to use for the lines around shapes,
     The style can be either a string containing a CSS color, or a CanvasGradient or CanvasPattern object. Invalid values are ignored.
     \sa QtQuick2::Context2D::createLinearGradient
     \sa QtQuick2::Context2D::createRadialGradient
     \sa QtQuick2::Context2D::createPattern
     \sa QtQuick2::Context2D::fillStyle
 */
v8::Handle<v8::Value> ctx2d_strokeStyle(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)


    return r->context->m_strokeStyle;
}

static void ctx2d_strokeStyle_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    QV8Engine *engine = V8ENGINE_ACCESSOR();

    r->context->m_strokeStyle = value;
    if (value->IsObject()) {
        QColor color = engine->toVariant(value, qMetaTypeId<QColor>()).value<QColor>();
        if (color.isValid()) {
            r->context->state.fillStyle = color;
            r->context->buffer()->setStrokeStyle(color);
        } else {
            QV8Context2DStyleResource *style = v8_resource_cast<QV8Context2DStyleResource>(value->ToObject());
            if (style && style->brush != r->context->state.strokeStyle) {
                r->context->state.strokeStyle = style->brush;
                r->context->buffer()->setStrokeStyle(style->brush);
            }
        }
    } else if (value->IsString()) {
        QColor color = qt_color_from_string(engine->toString(value));
        if (color.isValid() && r->context->state.strokeStyle != QBrush(color)) {
             r->context->state.strokeStyle = QBrush(color);
             r->context->buffer()->setStrokeStyle(r->context->state.strokeStyle);
        }
    }
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::createLinearGradient(real x0, real y0, real x1, real y1)
   Returns a CanvasGradient object that represents a linear gradient that paints along the line given by the coordinates
   represented by the start point (\a x0, \a y0) and the end point (\a x1, \a y1).
    \sa QtQuick2::Context2D::createRadialGradient
    \sa QtQuick2::Context2D::createPattern
    \sa QtQuick2::Context2D::fillStyle
    \sa QtQuick2::Context2D::strokeStyle
  */

static v8::Handle<v8::Value> ctx2d_createLinearGradient(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE();

    if (args.Length() == 4) {
        //TODO:infinite or NaN, the method must raise a NOT_SUPPORTED_ERR
        QSGContext2DEngineData *ed = engineData(engine);
        v8::Local<v8::Object> gradient = ed->constructorGradient->NewInstance();
        QV8Context2DStyleResource *r = new QV8Context2DStyleResource(engine);
        r->brush = QLinearGradient(args[0]->NumberValue(),
                                   args[1]->NumberValue(),
                                   args[2]->NumberValue(),
                                   args[3]->NumberValue());
        gradient->SetExternalResource(r);
        return gradient;
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::createRadialGradient(real x0, real y0, real r0, real x1, real y1, real r1)
   Returns a CanvasGradient object that represents a radial gradient that paints along the cone given by the start circle with
   origin (x0, y0) and radius r0, and the end circle with origin (x1, y1) and radius r1.

    \sa QtQuick2::Context2D::createLinearGradient
    \sa QtQuick2::Context2D::createPattern
    \sa QtQuick2::Context2D::fillStyle
    \sa QtQuick2::Context2D::strokeStyle
  */

static v8::Handle<v8::Value> ctx2d_createRadialGradient(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE();

    if (args.Length() == 6) {
        QSGContext2DEngineData *ed = engineData(engine);
        v8::Local<v8::Object> gradient = ed->constructorGradient->NewInstance();
        QV8Context2DStyleResource *r = new QV8Context2DStyleResource(engine);

        qreal x0 = args[0]->NumberValue();
        qreal y0 = args[1]->NumberValue();
        qreal r0 = args[2]->NumberValue();
        qreal x1 = args[3]->NumberValue();
        qreal y1 = args[4]->NumberValue();
        qreal r1 = args[5]->NumberValue();
        //TODO:infinite or NaN, a NOT_SUPPORTED_ERR exception must be raised.
        //If either of r0 or r1 are negative, an INDEX_SIZE_ERR exception must be raised.
        r->brush = QRadialGradient(QPointF(x1, y1), r0+r1, QPointF(x0, y0));
        gradient->SetExternalResource(r);
        return gradient;
    }

    return args.This();
}

/*!
  \qmlmethod variant createPattern(Image image, string repetition)
  Returns a CanvasPattern object that uses the given image and repeats in the direction(s) given by the repetition argument.

  The \a image parameter must be a valid Image item, if there is no image data, throws an INVALID_STATE_ERR exception.

  The allowed values for \a repetition are:

  \list
  \o "repeat"    - both directions
  \o "repeat-x   - horizontal only
  \o "repeat-y"  - vertical only
  \o "no-repeat" - neither
  \endlist

  If the repetition argument is empty or null, the value "repeat" is used.

  \sa QtQuick2::Context2D::strokeStyle
  \sa QtQuick2::Context2D::fillStyle
  */
static v8::Handle<v8::Value> ctx2d_createPattern(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE();

//    if (args.Length() == 2) {
//        QSGContext2DEngineData *ed = engineData(engine);
//        v8::Local<v8::Object> pattern = ed->constructorPattern->NewInstance();
//        QV8Context2DStyleResource *r = new QV8Context2DStyleResource(engine);

//        QImage img;

//        QSGItem* item = qobject_cast<QSGItem*>(engine->toQObject(args[0]));
//        if (item) {
//            img = qt_item_to_image(item);
//            if (img.isNull()) {
//                //exception: INVALID_STATE_ERR
//            }
//        } /*else {
//            //exception: TYPE_MISMATCH_ERR
//        }*/

//        QString repetition = engine->toString(args[1]);

//        if (repetition == "repeat" || repetition.isEmpty()) {
//            //TODO
//        } else if (repetition == "repeat-x") {
//            //TODO
//        } else if (repetition == "repeat-y") {
//            //TODO
//        } else if (repetition == "no-repeat") {
//            //TODO
//        } else {
//            //TODO: exception: SYNTAX_ERR
//        }
//        r->brush = img;
//        pattern->SetExternalResource(r);
 //       return pattern;
//    }
    return v8::Null();
}

// line styles
/*!
    \qmlproperty string QtQuick2::Context2D::lineCap
     Holds the the current line cap style.
     The possible line cap styles are:
    \list
    \o butt - the end of each line has a flat edge perpendicular to the direction of the line, this is the default line cap value.
    \o round - a semi-circle with the diameter equal to the width of the line must then be added on to the end of the line.
    \o square - a rectangle with the length of the line width and the width of half the line width, placed flat against the edge perpendicular to the direction of the line.
    \endlist
    Other values are ignored.
*/
v8::Handle<v8::Value> ctx2d_lineCap(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE_ACCESSOR();
    switch (r->context->state.lineCap) {
    case Qt::RoundCap:
        return engine->toString(QLatin1String("round"));
    case Qt::FlatCap:
        return engine->toString(QLatin1String("butt"));
    case Qt::SquareCap:
        return engine->toString(QLatin1String("square"));
    default:
        break;
    }
    return engine->toString(QLatin1String("butt"));;
}

static void ctx2d_lineCap_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    QV8Engine *engine = V8ENGINE_ACCESSOR();

    QString lineCap = engine->toString(value);
    Qt::PenCapStyle cap;
    if (lineCap == QLatin1String("round"))
        cap = Qt::RoundCap;
    else if (lineCap == QLatin1String("butt"))
        cap = Qt::FlatCap;
    else if (lineCap == QLatin1String("square"))
        cap = Qt::SquareCap;

    if (cap != r->context->state.lineCap) {
        r->context->state.lineCap = cap;
        r->context->buffer()->setLineCap(cap);
    }
}

/*!
    \qmlproperty string QtQuick2::Context2D::lineJoin
     Holds the the current line join style. A join exists at any point in a subpath
     shared by two consecutive lines. When a subpath is closed, then a join also exists
     at its first point (equivalent to its last point) connecting the first and last lines in the subpath.

    The possible line join styles are:
    \list
    \o bevel - this is all that is rendered at joins.
    \o round - a filled arc connecting the two aforementioned corners of the join, abutting (and not overlapping) the aforementioned triangle, with the diameter equal to the line width and the origin at the point of the join, must be rendered at joins.
    \o miter - a second filled triangle must (if it can given the miter length) be rendered at the join, this is the default line join style.
    \endlist
    Other values are ignored.
*/
v8::Handle<v8::Value> ctx2d_lineJoin(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE_ACCESSOR();
    switch (r->context->state.lineJoin) {
    case Qt::RoundJoin:
        return engine->toString(QLatin1String("round"));
    case Qt::BevelJoin:
        return engine->toString(QLatin1String("bevel"));
    case Qt::MiterJoin:
        return engine->toString(QLatin1String("miter"));
    default:
        break;
    }
    return engine->toString(QLatin1String("miter"));
}

static void ctx2d_lineJoin_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    QV8Engine *engine = V8ENGINE_ACCESSOR();

    QString lineJoin = engine->toString(value);
    Qt::PenJoinStyle join;
    if (lineJoin == QLatin1String("round"))
        join = Qt::RoundJoin;
    else if (lineJoin == QLatin1String("bevel"))
        join = Qt::BevelJoin;
    else if (lineJoin == QLatin1String("miter"))
        join = Qt::MiterJoin;

    if (join != r->context->state.lineJoin) {
        r->context->state.lineJoin = join;
        r->context->buffer()->setLineJoin(join);
    }
}

/*!
    \qmlproperty real QtQuick2::Context2D::lineWidth
     Holds the the current line width. Values that are not finite values greater than zero are ignored.
 */
v8::Handle<v8::Value> ctx2d_lineWidth(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)


    return v8::Number::New(r->context->state.lineWidth);
}

static void ctx2d_lineWidth_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    qreal w = value->NumberValue();

    if (w > 0 && w != r->context->state.lineWidth) {
        r->context->state.lineWidth = w;
        r->context->buffer()->setLineWidth(w);
    }
}

/*!
    \qmlproperty real QtQuick2::Context2D::miterLimit
     Holds the current miter limit ratio.
     The default miter limit value is 10.0.
 */
v8::Handle<v8::Value> ctx2d_miterLimit(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)


    return v8::Number::New(r->context->state.miterLimit);
}

static void ctx2d_miterLimit_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    qreal ml = value->NumberValue();

    if (ml > 0 && ml != r->context->state.miterLimit) {
        r->context->state.miterLimit = ml;
        r->context->buffer()->setMiterLimit(ml);
    }
}

// shadows
/*!
    \qmlproperty real QtQuick2::Context2D::shadowBlur
     Holds the current level of blur applied to shadows
 */
v8::Handle<v8::Value> ctx2d_shadowBlur(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)


    return v8::Number::New(r->context->state.shadowBlur);
}

static void ctx2d_shadowBlur_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)
    qreal blur = value->NumberValue();

    if (blur > 0 && blur != r->context->state.shadowBlur) {
        r->context->state.shadowBlur = blur;
        r->context->buffer()->setShadowBlur(blur);
    }
}

/*!
    \qmlproperty string QtQuick2::Context2D::shadowColor
     Holds the current shadow color.
 */
v8::Handle<v8::Value> ctx2d_shadowColor(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE_ACCESSOR();

    return engine->toString(r->context->state.shadowColor.name());
}

static void ctx2d_shadowColor_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    QV8Engine *engine = V8ENGINE_ACCESSOR();

    QColor color = qt_color_from_string(engine->toString(value));

    if (color.isValid() && color != r->context->state.shadowColor) {
        r->context->state.shadowColor = color;
        r->context->buffer()->setShadowColor(color);
    }
}


/*!
    \qmlproperty qreal QtQuick2::Context2D::shadowOffsetX
     Holds the current shadow offset in the positive horizontal distance.

     \sa QtQuick2::Context2D::shadowOffsetY
 */
v8::Handle<v8::Value> ctx2d_shadowOffsetX(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)


    return v8::Number::New(r->context->state.shadowOffsetX);
}

static void ctx2d_shadowOffsetX_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    //TODO: check value:infinite or NaN
    qreal offsetX = value->NumberValue();
    if (offsetX != r->context->state.shadowOffsetX) {
        r->context->state.shadowOffsetX = offsetX;
        r->context->buffer()->setShadowOffsetX(offsetX);
    }
}
/*!
    \qmlproperty qreal QtQuick2::Context2D::shadowOffsetY
     Holds the current shadow offset in the positive vertical distance.

     \sa QtQuick2::Context2D::shadowOffsetX
 */
v8::Handle<v8::Value> ctx2d_shadowOffsetY(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)


    return v8::Number::New(r->context->state.shadowOffsetY);
}

static void ctx2d_shadowOffsetY_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)
    //TODO: check value:infinite or NaN
    qreal offsetY = value->NumberValue();
    if (offsetY != r->context->state.shadowOffsetY) {
        r->context->state.shadowOffsetY = offsetY;
        r->context->buffer()->setShadowOffsetY(offsetY);
    }
}

v8::Handle<v8::Value> ctx2d_path(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)
    return r->context->m_v8path;
}

static void ctx2d_path_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)
    QV8Engine *engine = V8ENGINE_ACCESSOR();

    r->context->beginPath();
    if (value->IsObject()) {
        QDeclarativePath* path = qobject_cast<QDeclarativePath*>(engine->toQObject(value));
        if (path)
            r->context->m_path = path->path();
    } else {
        QString path = engine->toString(value->ToString());
        QDeclarativeSvgParser::parsePathDataFast(path, r->context->m_path);
    }
    r->context->m_v8path = value;
}

//rects
/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::clearRect(real x, real y, real w, real h)
  Clears all pixels on the canvas in the given rectangle to transparent black.
  */
static v8::Handle<v8::Value> ctx2d_clearRect(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 4) {
        r->context->buffer()->clearRect(args[0]->NumberValue(),
                                        args[1]->NumberValue(),
                                        args[2]->NumberValue(),
                                        args[3]->NumberValue());
    }

    return args.This();
}
/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::fillRect(real x, real y, real w, real h)
   Paint the specified rectangular area using the fillStyle.

   \sa QtQuick2::Context2D::fillStyle
  */
static v8::Handle<v8::Value> ctx2d_fillRect(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 4) {
        r->context->buffer()->fillRect(args[0]->NumberValue(),
                             args[1]->NumberValue(),
                             args[2]->NumberValue(),
                             args[3]->NumberValue());
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::fillRect(real x, real y, real w, real h)
   Stroke the specified rectangle's path using the strokeStyle, lineWidth, lineJoin,
   and (if appropriate) miterLimit attributes.

   \sa QtQuick2::Context2D::strokeStyle
   \sa QtQuick2::Context2D::lineWidth
   \sa QtQuick2::Context2D::lineJoin
   \sa QtQuick2::Context2D::miterLimit
  */
static v8::Handle<v8::Value> ctx2d_strokeRect(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 4) {
        r->context->buffer()->strokeRect(args[0]->NumberValue(),
                                         args[1]->NumberValue(),
                                         args[2]->NumberValue(),
                                         args[3]->NumberValue());
    }
    
    return args.This();
}

// Complex shapes (paths) API
/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::arc(real x, real y, real radius, real startAngle, real endAngle, bool anticlockwise)
   Adds points to the subpath such that the arc described by the circumference of
   the circle described by the arguments.

   See http://www.w3.org/TR/2dcontext/#dom-context-2d-arc for details.
  */
static v8::Handle<v8::Value> ctx2d_arc(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)

    if (args.Length() >= 5) {
        bool antiClockwise = false;

        if (args.Length() == 6)
            antiClockwise = args[5]->BooleanValue();

        qreal radius = args[2]->NumberValue();
        //Throws an INDEX_SIZE_ERR exception if the given radius is negative.
        r->context->arc(args[0]->NumberValue(),
                        args[1]->NumberValue(),
                        radius,
                        args[3]->NumberValue(),
                        args[4]->NumberValue(),
                        antiClockwise);
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::arcTo(real x1, real y1, real x2, real y2, real radius)

   Adds an arc with the given control points and radius to the current subpath, connected to the previous point by a straight line.
   See http://www.w3.org/TR/2dcontext/#dom-context-2d-arcto for details.
  */
static v8::Handle<v8::Value> ctx2d_arcTo(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 5) {
        r->context->arcTo(args[0]->NumberValue(),
                          args[1]->NumberValue(),
                          args[2]->NumberValue(),
                          args[3]->NumberValue(),
                          args[4]->NumberValue());
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::beginPath()

   Resets the current path.
  */
static v8::Handle<v8::Value> ctx2d_beginPath(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    r->context->beginPath();

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::bezierCurveTo(real cp1x, real cp1y, real cp2x, real cp2y, real x, real y)

   Adds the given point to the current subpath, connected to the previous one by a cubic Bézier curve with the given control points.

   See http://www.w3.org/TR/2dcontext/#dom-context-2d-beziercurveto for details.
  */
static v8::Handle<v8::Value> ctx2d_bezierCurveTo(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 6) {
        r->context->bezierCurveTo(args[0]->NumberValue(),
                                  args[1]->NumberValue(),
                                  args[2]->NumberValue(),
                                  args[3]->NumberValue(),
                                  args[4]->NumberValue(),
                                  args[5]->NumberValue());
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::clip()

   constrains the clipping region to the given path.

   See http://www.w3.org/TR/2dcontext/#dom-context-2d-clip for details.
  */
static v8::Handle<v8::Value> ctx2d_clip(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)

    r->context->state.clipPath = r->context->m_path;
    r->context->buffer()->clip(r->context->state.clipPath);
    
    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::closePath()

   Marks the current subpath as closed, and starts a new subpath with a point the same as the start and end of the newly closed subpath.

   See http://www.w3.org/TR/2dcontext/#dom-context-2d-closepath for details.
  */
static v8::Handle<v8::Value> ctx2d_closePath(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    r->context->closePath();

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::fill()

   Fills the subpaths with the current fill style.

   See http://www.w3.org/TR/2dcontext/#dom-context-2d-fill for details.

   \sa QtQuick2::Context2D::fillStyle
  */
static v8::Handle<v8::Value> ctx2d_fill(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r);

    r->context->buffer()->fill(r->context->m_path);

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::lineTo(real x, real y)

   Adds the given point to the current subpath, connected to the previous one by a straight line.
 */
static v8::Handle<v8::Value> ctx2d_lineTo(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 2) {
        r->context->lineTo(args[0]->NumberValue(),
                           args[1]->NumberValue());
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::moveTo(real x, real y)

   Creates a new subpath with the given point.
 */
static v8::Handle<v8::Value> ctx2d_moveTo(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 2) {
        r->context->moveTo(args[0]->NumberValue(),
                           args[1]->NumberValue());
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::quadraticCurveTo(real cpx, real cpy, real x, real y)

   Adds the given point to the current subpath, connected to the previous one by a quadratic Bézier curve with the given control point.

   See http://www.w3.org/TR/2dcontext/#dom-context-2d-quadraticcurveto for details.
 */
static v8::Handle<v8::Value> ctx2d_quadraticCurveTo(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 4) {
        r->context->quadraticCurveTo(args[0]->NumberValue(),
                                     args[1]->NumberValue(),
                                     args[2]->NumberValue(),
                                     args[3]->NumberValue());
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::rect(real x, real y, real w, real h)

   Adds a new closed subpath to the path, representing the given rectangle.

   See http://www.w3.org/TR/2dcontext/#dom-context-2d-rect for details.
 */
static v8::Handle<v8::Value> ctx2d_rect(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 4) {
        r->context->rect(args[0]->NumberValue(),
                         args[1]->NumberValue(),
                         args[2]->NumberValue(),
                         args[3]->NumberValue());
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::roundedRect(real x, real y, real w, real h,  real xRadius, real yRadius)

   Adds the given rectangle rect with rounded corners to the path. The xRadius and yRadius arguments specify the radii of the
   ellipses defining the corners of the rounded rectangle.
 */
static v8::Handle<v8::Value> ctx2d_roundedRect(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 6) {
        r->context->roundedRect(args[0]->NumberValue(),
                                args[1]->NumberValue(),
                                args[2]->NumberValue(),
                                args[3]->NumberValue(),
                                args[4]->NumberValue(),
                                args[5]->NumberValue());
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::ellipse(real x, real y, real w, real h)

  Creates an ellipse within the bounding rectangle defined by its top-left corner at (\a x, \ y), width \a w and height \a h,
  and adds it to the path as a closed subpath.

  The ellipse is composed of a clockwise curve, starting and finishing at zero degrees (the 3 o'clock position).
 */
static v8::Handle<v8::Value> ctx2d_ellipse(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    if (args.Length() == 4) {
        r->context->ellipse(args[0]->NumberValue(),
                            args[1]->NumberValue(),
                            args[2]->NumberValue(),
                            args[3]->NumberValue());
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::text(string text, real x, real y)

  Adds the given \a text to the path as a set of closed subpaths created from the current context font supplied.
  The subpaths are positioned so that the left end of the text's baseline lies at the point specified by (x, y).
 */
static v8::Handle<v8::Value> ctx2d_text(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)

    QV8Engine *engine = V8ENGINE();
    if (args.Length() == 3) {
        r->context->text(engine->toString(args[0]),
                         args[1]->NumberValue(),
                         args[2]->NumberValue());
    }

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::stroke()

   Strokes the subpaths with the current stroke style.

   See http://www.w3.org/TR/2dcontext/#dom-context-2d-stroke for details.

   \sa QtQuick2::Context2D::strokeStyle
  */
static v8::Handle<v8::Value> ctx2d_stroke(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    r->context->buffer()->stroke(r->context->m_path);

    return args.This();
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::isPointInPath(real x, real y)

   Returns true if the given point is in the current path.

   See http://www.w3.org/TR/2dcontext/#dom-context-2d-ispointinpath for details.
  */
static v8::Handle<v8::Value> ctx2d_isPointInPath(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    bool pointInPath = false;
    if (args.Length() == 2) {
        pointInPath = r->context->isPointInPath(args[0]->NumberValue(),
                                                args[1]->NumberValue());
    }

    return v8::Boolean::New(pointInPath);
}

static v8::Handle<v8::Value> ctx2d_drawFocusRing(const v8::Arguments &args)
{
    V8THROW_ERROR("Context2D::drawFocusRing is not supported")
    return args.This();
}

static v8::Handle<v8::Value> ctx2d_setCaretSelectionRect(const v8::Arguments &args)
{
    V8THROW_ERROR("Context2D::setCaretSelectionRect is not supported")
    return args.This();
}

static v8::Handle<v8::Value> ctx2d_caretBlinkRate(const v8::Arguments &args)
{
    V8THROW_ERROR("Context2D::caretBlinkRate is not supported")

    return args.This();
}
// text
/*!
  \qmlproperty string QtQuick2::Context2D::font
  Holds the current font settings, default value is "10px sans-serif".

  See http://www.w3.org/TR/2dcontext/#dom-context-2d-font for details.
  */
v8::Handle<v8::Value> ctx2d_font(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)

    QV8Engine *engine = V8ENGINE_ACCESSOR();

    return engine->toString(r->context->m_fontString);
}

static void ctx2d_font_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)

    QV8Engine *engine = V8ENGINE_ACCESSOR();
    QString fs = engine->toString(value);
    if (fs != r->context->m_fontString) {
        r->context->m_fontString = fs;
        QFont font = qt_font_from_string(fs);
        r->context->state.font = font;
    }
}

/*!
  \qmlproperty string QtQuick2::Context2D::textAlign

  Holds the current text alignment settings.
  The possible values are:
  \list
    \o start
    \o end
    \o left
    \o right
    \o center
  \endlist
  Other values are ignored. The default is start.
  */
v8::Handle<v8::Value> ctx2d_textAlign(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)
    QV8Engine *engine = V8ENGINE_ACCESSOR();
    switch (r->context->state.textAlign) {
    case QSGContext2D::Start:
        return engine->toString(QLatin1String("start"));
   case QSGContext2D::End:
        return engine->toString(QLatin1String("end"));
   case QSGContext2D::Left:
        return engine->toString(QLatin1String("left"));
   case QSGContext2D::Right:
        return engine->toString(QLatin1String("right"));
   case QSGContext2D::Center:
        return engine->toString(QLatin1String("center"));
    default:
        break;
    }
    return engine->toString(QLatin1String("start"));
}

static void ctx2d_textAlign_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)
    QV8Engine *engine = V8ENGINE_ACCESSOR();

    QString textAlign = engine->toString(value);

    QSGContext2D::TextAlignType ta;
    if (textAlign == QLatin1String("start"))
        ta = QSGContext2D::Start;
    else if (textAlign == QLatin1String("end"))
        ta = QSGContext2D::End;
    else if (textAlign == QLatin1String("left"))
        ta = QSGContext2D::Left;
    else if (textAlign == QLatin1String("right"))
        ta = QSGContext2D::Right;
    else if (textAlign == QLatin1String("center"))
        ta = QSGContext2D::Center;

    if (ta != r->context->state.textAlign) {
        r->context->state.textAlign = ta;
    }
}

/*!
  \qmlproperty string QtQuick2::Context2D::textBaseline

  Holds the current baseline alignment settings.
  The possible values are:
  \list
    \o top
    \o hanging
    \o middle
    \o alphabetic
    \o ideographic
    \o bottom
  \endlist
  Other values are ignored. The default value is "alphabetic".
  See http://www.w3.org/TR/2dcontext/#dom-context-2d-textbaseline for details.
  */
v8::Handle<v8::Value> ctx2d_textBaseline(v8::Local<v8::String>, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT(r)

    QV8Engine *engine = V8ENGINE_ACCESSOR();
    switch (r->context->state.textBaseline) {
    case QSGContext2D::Alphabetic:
        return engine->toString(QLatin1String("alphabetic"));
    case QSGContext2D::Hanging:
        return engine->toString(QLatin1String("hanging"));
    case QSGContext2D::Top:
        return engine->toString(QLatin1String("top"));
    case QSGContext2D::Bottom:
        return engine->toString(QLatin1String("bottom"));
    case QSGContext2D::Middle:
        return engine->toString(QLatin1String("middle"));
    default:
        break;
    }
    return engine->toString(QLatin1String("alphabetic"));
}

static void ctx2d_textBaseline_set(v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(info.This());
    CHECK_CONTEXT_SETTER(r)
    QV8Engine *engine = V8ENGINE_ACCESSOR();
    QString textBaseline = engine->toString(value);

    QSGContext2D::TextBaseLineType tb;
    if (textBaseline == QLatin1String("alphabetic"))
        tb = QSGContext2D::Alphabetic;
    else if (textBaseline == QLatin1String("hanging"))
        tb = QSGContext2D::Hanging;
    else if (textBaseline == QLatin1String("top"))
        tb = QSGContext2D::Top;
    else if (textBaseline == QLatin1String("bottom"))
        tb = QSGContext2D::Bottom;
    else if (textBaseline == QLatin1String("middle"))
        tb = QSGContext2D::Middle;

    if (tb != r->context->state.textBaseline) {
        r->context->state.textBaseline = tb;
    }
}

/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::fillText(text, x, y)
  Fills the given text at the given position.
  See http://www.w3.org/TR/2dcontext/#dom-context-2d-filltext for details.
  */
static v8::Handle<v8::Value> ctx2d_fillText(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE();

    if (args.Length() == 3) {
        QPainterPath textPath = r->context->createTextGlyphs(args[1]->NumberValue(),
                                                             args[2]->NumberValue(),
                                                             engine->toString(args[0]));
        r->context->buffer()->fill(textPath);
    }

    return args.This();
}
/*!
  \qmlmethod QtQuick2::Context2D QtQuick2::Context2D::strokeText(text, x, y)
  Strokes the given text at the given position.
  See http://www.w3.org/TR/2dcontext/#dom-context-2d-stroketext for details.
  */
static v8::Handle<v8::Value> ctx2d_strokeText(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE();

    if (args.Length() == 3) {
        QPainterPath textPath = r->context->createTextGlyphs(args[1]->NumberValue(),
                                                             args[2]->NumberValue(),
                                                             engine->toString(args[0]));
        r->context->buffer()->stroke(textPath);
    }

    return args.This();
}
/*!
  \qmlclass QtQuick2::TextMetrics
    \inqmlmodule QtQuick 2
    \since QtQuick 2.0
    \brief The Context2D TextMetrics interface.
    The TextMetrics object can be created by QtQuick2::Context2D::measureText method.
    See http://www.w3.org/TR/2dcontext/#textmetrics for more details.

    \sa QtQuick2::Context2D::measureText
    \sa QtQuick2::TextMetrics::width
  */

/*!
  \qmlproperty int QtQuick2::TextMetrics::width
  Holds the advance width of the text that was passed to the QtQuick2::Context2D::measureText() method.
  This property is read only.
  See http://www.w3.org/TR/2dcontext/#dom-textmetrics-width for more details.
  */

/*!
  \qmlmethod variant QtQuick2::Context2D::measureText(text)
  Returns a TextMetrics object with the metrics of the given text in the current font.
  See http://www.w3.org/TR/2dcontext/#dom-context-2d-measuretext for details.
  */
static v8::Handle<v8::Value> ctx2d_measureText(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE();

    if (args.Length() == 1) {
        QFontMetrics fm(r->context->state.font);
        uint width = fm.width(engine->toString(args[0]));
        v8::Local<v8::Object> tm = v8::Object::New();
        tm->Set(v8::String::New("width"), v8::Number::New(width));
        return tm;
    }

    return v8::Undefined();
}

// drawing images
/*!
  \qmlmethod QtQuick2::Context2D::drawImage(variant image, real dx, real dy)
  Draws the given \a image on the canvas at position (\a dx, \a dy).
  Note:
     The \a image type can be an Image item or a image url. When given as Image
  type, if the image isn't fully loaded, will draw nothing. When given as url string,
  the context loads the image asynchorously and redraw the canvas when the image is loaded.

  See http://www.w3.org/TR/2dcontext/#dom-context-2d-drawimage for more details.
  */
/*!
  \qmlmethod QtQuick2::Context2D::drawImage(variant image, real dx, real dy, real dw, real dh)
  Draws the given item as \a image onto the canvas at point (\a dx, \a dy) and with width \a dw,
  height \a dh.
  See http://www.w3.org/TR/2dcontext/#dom-context-2d-drawimage for more details.
  */
/*!
  \qmlmethod QtQuick2::Context2D::drawImage(variant image, real sx, real sy, real sw, sh, real dx, real dy, real dw, dh)
  Draws the given item as \a image from source point (\a sx, \a sy) and source width \sw, source height \sh
  onto the canvas at point (\a dx, \a dy) and with width \a dw, height \a dh.
  See http://www.w3.org/TR/2dcontext/#dom-context-2d-drawimage for more details.
  */
static v8::Handle<v8::Value> ctx2d_drawImage(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE();

    //TODO: handle exceptions

    qreal sx, sy, sw, sh, dx, dy, dw, dh;

    if (args.Length() != 3 && args.Length() != 5 && args.Length() != 9) {
        //parameter error
        return args.This();
    }


    QImage image;
    if (args[0]->IsString()) {
        image = r->context->createImage(QUrl(engine->toString(args[0]->ToString())));
    } /*else if (args[0]->IsObject()) {
        QSGImage* imageItem = qobject_cast<QSGImage*>(engine->toQObject(args[0]->ToObject()));
        if (imageItem) {
            image = imageItem->pixmap().toImage();
        } else {
            //wrong image type
            return args.This();
        }
    }*/
    if (args.Length() == 3) {
        dx = args[1]->NumberValue();
        dy = args[2]->NumberValue();
        sx = 0;
        sy = 0;
        sw = image.isNull()? -1 : image.width();
        sh = image.isNull()? -1 : image.height();
        dw = sw;
        dh = sh;
    } else if (args.Length() == 5) {
        sx = 0;
        sy = 0;
        sw = image.isNull()? -1 : image.width();
        sh = image.isNull()? -1 : image.height();
        dx = args[1]->NumberValue();
        dy = args[2]->NumberValue();
        dw = args[3]->NumberValue();
        dh = args[4]->NumberValue();
    } else if (args.Length() == 9) {
        sx = args[1]->NumberValue();
        sy = args[2]->NumberValue();
        sw = args[3]->NumberValue();
        sh = args[4]->NumberValue();
        dx = args[5]->NumberValue();
        dy = args[6]->NumberValue();
        dw = args[7]->NumberValue();
        dh = args[8]->NumberValue();
    } else {
        //error
        return args.This();
    }

    r->context->buffer()->drawImage(image,sx, sy, sw, sh, dx, dy, dw, dh);

    return args.This();
}

// pixel manipulation
/*!
  \qmlclass QtQuick2::CanvasImageData

  */
/*!
  \qmlproperty QtQuick2::CanvasImageData::width
  Holds the actual width dimension of the data in the ImageData object, in device pixels.
 */
v8::Handle<v8::Value> ctx2d_imageData_width(v8::Local<v8::String>, const v8::AccessorInfo &args)
{
    QV8Context2DPixelArrayResource *r = v8_resource_cast<QV8Context2DPixelArrayResource>(args.This()->GetInternalField(0)->ToObject());
    if (!r)
        return v8::Integer::New(0);
    return v8::Integer::New(r->image.width());
}

/*!
  \qmlproperty QtQuick2::CanvasImageData::height
  Holds the actual height dimension of the data in the ImageData object, in device pixels.
  */
v8::Handle<v8::Value> ctx2d_imageData_height(v8::Local<v8::String>, const v8::AccessorInfo &args)
{
    QV8Context2DPixelArrayResource *r = v8_resource_cast<QV8Context2DPixelArrayResource>(args.This()->GetInternalField(0)->ToObject());
    if (!r)
        return v8::Integer::New(0);

    return v8::Integer::New(r->image.height());
}

/*!
  \qmlproperty QtQuick2::CanvasImageData::data
  Holds the one-dimensional array containing the data in RGBA order, as integers in the range 0 to 255.
 */
v8::Handle<v8::Value> ctx2d_imageData_data(v8::Local<v8::String>, const v8::AccessorInfo &args)
{
    return args.This()->GetInternalField(0);
}

static v8::Handle<v8::Value> ctx2d_imageData_mirror(const v8::Arguments &args)
{
    bool horizontal = false, vertical = true;
    QV8Context2DPixelArrayResource *r = v8_resource_cast<QV8Context2DPixelArrayResource>(args.This()->GetInternalField(0)->ToObject());

    if (!r) {
        //error
        return v8::Undefined();
    }

    if (args.Length() > 2) {
      //error
      return v8::Undefined();
    }

    if (args.Length() == 1) {
        horizontal = args[0]->BooleanValue();
    } else if (args.Length() == 2) {
        horizontal = args[0]->BooleanValue();
        vertical = args[1]->BooleanValue();
    }
    r->image = r->image.mirrored(horizontal, vertical);
    return args.This();
}


static v8::Handle<v8::Value> ctx2d_imageData_filter(const v8::Arguments &args)
{
    QV8Context2DPixelArrayResource *r = v8_resource_cast<QV8Context2DPixelArrayResource>(args.This()->GetInternalField(0)->ToObject());

    if (!r) {
        //error
        return v8::Undefined();
    }

    if (args.Length() >= 1) {
        int filterFlag = args[0]->IntegerValue();
        switch(filterFlag) {
        case QSGCanvasItem::GrayScale :
        {
            for (int y = 0; y < r->image.height(); ++y) {
              QRgb *row = (QRgb*)r->image.scanLine(y);
              for (int x = 0; x < r->image.width(); ++x) {
                  unsigned char* rgb = ((unsigned char*)&row[x]);
                  rgb[0] = rgb[1] = rgb[2] = qGray(rgb[0], rgb[1], rgb[2]);
              }
            }
        }
            break;
        case QSGCanvasItem::Threshold :
        {
            int threshold = 127;
            if (args.Length() > 1)
                threshold = args[1]->IntegerValue();

            for (int y = 0; y < r->image.height(); ++y) {
              QRgb *row = (QRgb*)r->image.scanLine(y);
              for (int x = 0; x < r->image.width(); ++x) {
                  unsigned char* rgb = ((unsigned char*)&row[x]);
                  unsigned char v = qGray(rgb[0], rgb[1], rgb[2]) >= threshold ? 255 : 0;
                  rgb[0] = rgb[1] = rgb[2] = v;
              }
            }
        }
            break;
        case QSGCanvasItem::Brightness :
        {
            int adjustment = 1;
            if (args.Length() > 1)
                adjustment = args[1]->IntegerValue();

            for (int y = 0; y < r->image.height(); ++y) {
              QRgb *row = (QRgb*)r->image.scanLine(y);
              for (int x = 0; x < r->image.width(); ++x) {
                ((unsigned char*)&row[x])[0] += adjustment;
                ((unsigned char*)&row[x])[1] += adjustment;
                ((unsigned char*)&row[x])[2] += adjustment;
              }
            }
        }
            break;
        case QSGCanvasItem::Invert :
        {
            r->image.invertPixels();
        }
            break;
        case QSGCanvasItem::Blur :
        {
            int radius = 3;
            bool quality = false;

            if (args.Length() > 1)
                radius = args[1]->IntegerValue() / 2;
            if (args.Length() > 2)
                quality = args[2]->BooleanValue();

            qt_image_boxblur(r->image, radius, quality);
        }
            break;
        case QSGCanvasItem::Opaque :
        {
            for (int y = 0; y < r->image.height(); ++y) {
              QRgb *row = (QRgb*)r->image.scanLine(y);
              for (int x = 0; x < r->image.width(); ++x) {
                ((unsigned char*)&row[x])[3] = 255;
              }
            }
        }
            break;
        case QSGCanvasItem::Convolute :
        {
            if (args.Length() > 1 && args[1]->IsArray()) {
                v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(args[1]);
                QVector<qreal> weights;
                for (uint32_t i = 0; i < array->Length(); ++i)
                    weights.append(array->Get(i)->NumberValue());
                r->image = qt_image_convolute_filter(r->image, weights);
            } else {
                //error
            }
        }
            break;
        default:
            break;
        }
    }

    return args.This();
}
/*!
  \qmlclass QtQuick2::CanvasPixelArray
  The CanvasPixelArray object provides ordered, indexed access to the color components of each pixel of the image data.
  See http://www.w3.org/TR/2dcontext/#canvaspixelarray for more details.
  */

/*!
  \qmlproperty QtQuick2::CanvasPixelArray::length
  The CanvasPixelArray object represents h×w×4 integers which w and h comes from CanvasImageData.
  The length attribute of a CanvasPixelArray object must return this h×w×4 number value.
 */
v8::Handle<v8::Value> ctx2d_pixelArray_length(v8::Local<v8::String>, const v8::AccessorInfo &args)
{
    QV8Context2DPixelArrayResource *r = v8_resource_cast<QV8Context2DPixelArrayResource>(args.This());
    if (!r || r->image.isNull()) return v8::Undefined();

    return v8::Integer::New(r->image.width() * r->image.height() * 4);
}

v8::Handle<v8::Value> ctx2d_pixelArray_indexed(uint32_t index, const v8::AccessorInfo& args)
{
    QV8Context2DPixelArrayResource *r = v8_resource_cast<QV8Context2DPixelArrayResource>(args.This());

    if (r && index && index < r->image.width() * r->image.height() * 4) {
        const int w = r->image.width();
        const int h = r->image.height();
        const int row = (index / 4) / w;
        const int col = (index / 4) % w;
        const QRgb* pixel = reinterpret_cast<const QRgb*>(r->image.constScanLine(row));
        pixel += col;
        switch (index % 4) {
        case 0:
            return v8::Integer::New(qRed(*pixel));
        case 1:
            return v8::Integer::New(qGreen(*pixel));
        case 2:
            return v8::Integer::New(qBlue(*pixel));
        case 3:
            return v8::Integer::New(qAlpha(*pixel));
        }
    }
    return v8::Undefined();
}

v8::Handle<v8::Value> ctx2d_pixelArray_indexed_set(uint32_t index, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    QV8Context2DPixelArrayResource *r = v8_resource_cast<QV8Context2DPixelArrayResource>(info.This());

    const int v = value->Uint32Value();
    if (r && index > 0 && index < r->image.width() * r->image.height() * 4 && v > 0 && v <= 255) {
        const int w = r->image.width();
        const int h = r->image.height();
        const int row = (index / 4) / w;
        const int col = (index / 4) % w;

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
    return v8::Undefined();
}
/*!
  \qmlmethod QtQuick2::CanvasImageData createImageData(real sw, real sh)
   Creates a CanvasImageData object with the given dimensions(\a sw, \a sh).
  */
/*!
  \qmlmethod QtQuick2::CanvasImageData createImageData(QtQuick2::CanvasImageData imageData)
   Creates a CanvasImageData object with the same dimensions as the argument.
  */
/*!
  \qmlmethod QtQuick2::CanvasImageData createImageData(Url imageUrl)
   Creates a CanvasImageData object with the given image loaded from \a imageUrl.
   Note:The \a imageUrl must be already loaded before this function call, if not, an empty
   CanvasImageData obect will be returned.

   \sa QtQuick2::Canvas::loadImage, QtQuick2::Canvas::unloadImage, QtQuick2::Canvas::isImageLoaded
  */
static v8::Handle<v8::Value> ctx2d_createImageData(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)


    QV8Engine *engine = V8ENGINE();

    if (args.Length() == 1) {
        if (args[0]->IsObject()) {
            v8::Local<v8::Object> imgData = args[0]->ToObject();
            QV8Context2DPixelArrayResource *pa = v8_resource_cast<QV8Context2DPixelArrayResource>(imgData->GetInternalField(0)->ToObject());
            if (pa) {
                qreal w = imgData->Get(v8::String::New("width"))->NumberValue();
                qreal h = imgData->Get(v8::String::New("height"))->NumberValue();
                return qt_create_image_data(w, h, engine, QImage());
            }
        } else if (args[0]->IsString()) {
            QImage image = r->context->createImage(QUrl(engine->toString(args[0]->ToString())));
            return qt_create_image_data(image.width(), image.height(), engine, image);
        }
    } else if (args.Length() == 2) {
        qreal w = args[0]->NumberValue();
        qreal h = args[1]->NumberValue();
        if (w > 0 && h > 0)
            return qt_create_image_data(w, h, engine, QImage());
    }
    return v8::Undefined();
}

/*!
  \qmlmethod QtQuick2::CanvasImageData getImageData(real sx, real sy, real sw, real sh)
  Returns an CanvasImageData object containing the image data for the given rectangle of the canvas.
  */
static v8::Handle<v8::Value> ctx2d_getImageData(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)

    QV8Engine *engine = V8ENGINE();
    if (args.Length() == 4) {
        qreal x = args[0]->NumberValue();
        qreal y = args[1]->NumberValue();
        qreal w = args[2]->NumberValue();
        qreal h = args[3]->NumberValue();
        QImage image = r->context->canvas()->toImage(QRectF(x, y, w, h));
        if (image.format() != QImage::Format_ARGB32)
            image = image.convertToFormat(QImage::Format_ARGB32);
        v8::Local<v8::Object> imageData = qt_create_image_data(w, h, engine, image);

        return imageData;
    }
    return v8::Null();
}

/*!
  \qmlmethod QtQuick2::Context2D putImageData(QtQuick2::CanvasImageData imageData, real dx, real dy, real dirtyX, real dirtyY, real dirtyWidth, real dirtyHeight)
  Paints the data from the given ImageData object onto the canvas. If a dirty rectangle (\a dirtyX, \a dirtyY, \a dirtyWidth, \a dirtyHeight) is provided, only the pixels from that rectangle are painted.
  */
static v8::Handle<v8::Value> ctx2d_putImageData(const v8::Arguments &args)
{
    QV8Context2DResource *r = v8_resource_cast<QV8Context2DResource>(args.This());
    CHECK_CONTEXT(r)
    if (args.Length() != 3 && args.Length() != 7)
        return v8::Undefined();

    if (args[0]->IsNull() || !args[0]->IsObject()) {
        V8THROW_ERROR("Context2D::putImageData, the image data type mismatch");
    }
    qreal dx = args[1]->NumberValue();
    qreal dy = args[2]->NumberValue();
    qreal w, h, dirtyX, dirtyY, dirtyWidth, dirtyHeight;

    v8::Local<v8::Object> imageData = args[0]->ToObject();
    QV8Context2DPixelArrayResource *pixelArray = v8_resource_cast<QV8Context2DPixelArrayResource>(imageData->Get(v8::String::New("data"))->ToObject());
    if (pixelArray) {
        w = imageData->Get(v8::String::New("width"))->NumberValue();
        h = imageData->Get(v8::String::New("height"))->NumberValue();

        if (args.Length() == 7) {
            dirtyX = args[3]->NumberValue();
            dirtyY = args[4]->NumberValue();
            dirtyWidth = args[5]->NumberValue();
            dirtyHeight = args[6]->NumberValue();
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
                return args.This();
        } else {
            dirtyX = 0;
            dirtyY = 0;
            dirtyWidth = w;
            dirtyHeight = h;
        }

        QImage image = pixelArray->image.copy(dirtyX, dirtyY, dirtyWidth, dirtyHeight);
        r->context->buffer()->drawImage(image, dirtyX, dirtyY, dirtyWidth, dirtyHeight, dx, dy, dirtyWidth, dirtyHeight);
    }
    return args.This();
}

/*!
  \qmlclass QtQuick2::CanvasGradient
    \inqmlmodule QtQuick 2
    \since QtQuick 2.0
    \brief The Context2D opaque CanvasGradient interface.
  */

/*!
  \qmlmethod QtQuick2::CanvasGradient QtQuick2::CanvasGradient::addColorStop(real offsetof, string color)
  Adds a color stop with the given color to the gradient at the given offset.
  0.0 is the offset at one end of the gradient, 1.0 is the offset at the other end.
  */
static v8::Handle<v8::Value> ctx2d_gradient_addColorStop(const v8::Arguments &args)
{
    QV8Context2DStyleResource *style = v8_resource_cast<QV8Context2DStyleResource>(args.This());
    if (!style)
        V8THROW_ERROR("Not a CanvasGradient object");

    QV8Engine *engine = V8ENGINE();

    if (args.Length() == 2) {

        if (!style->brush.gradient())
            V8THROW_ERROR("Not a valid CanvasGradient object, can't get the gradient information");
        QGradient gradient = *(style->brush.gradient());
        qreal pos = args[0]->NumberValue();
        QColor color = qt_color_from_string(engine->toString(args[1]));
        if (pos < 0.0 || pos > 1.0) {
            //Throws an INDEX_SIZE_ERR exception
            V8THROW_ERROR("CanvasGradient: parameter offset out of range");
        }

        if (color.isValid()) {
            gradient.setColorAt(pos, color);
        } else {
            //Throws a SYNTAX_ERR exception
            V8THROW_ERROR("CanvasGradient: parameter color is not a valid color string");
        }
        style->brush = gradient;
    }

    return args.This();
}


void QSGContext2D::beginPath()
{
    m_path = QPainterPath();
    m_path.setFillRule(state.fillRule);
}

void QSGContext2D::closePath()
{
    if (m_path.isEmpty())
        return;

    QRectF boundRect = m_path.boundingRect();
    if (boundRect.width() || boundRect.height())
        m_path.closeSubpath();
}

void QSGContext2D::moveTo( qreal x, qreal y)
{
    m_path.moveTo(state.matrix.map(QPointF(x, y)));
}

void QSGContext2D::lineTo( qreal x, qreal y)
{
    m_path.lineTo(state.matrix.map(QPointF(x, y)));
}

void QSGContext2D::quadraticCurveTo(qreal cpx, qreal cpy,
                                           qreal x, qreal y)
{
    m_path.quadTo(state.matrix.map(QPointF(cpx, cpy)),
                      state.matrix.map(QPointF(x, y)));
}

void QSGContext2D::bezierCurveTo(qreal cp1x, qreal cp1y,
                                        qreal cp2x, qreal cp2y,
                                        qreal x, qreal y)
{
    m_path.cubicTo(state.matrix.map(QPointF(cp1x, cp1y)),
                       state.matrix.map(QPointF(cp2x, cp2y)),
                       state.matrix.map(QPointF(x, y)));
}

void QSGContext2D::addArcTo(const QPointF& p1, const QPointF& p2, float radius)
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

    arc(p.x(), p.y(), radius, sa, ea, anticlockwise, false);
}

void QSGContext2D::arcTo(qreal x1, qreal y1,
                                qreal x2, qreal y2,
                                qreal radius)
{
    QPointF st  = state.matrix.map(QPointF(x1, y1));
    QPointF end = state.matrix.map(QPointF(x2, y2));

    if (!m_path.elementCount()) {
        m_path.moveTo(st);
    } else if (st == m_path.currentPosition() || st == end || !radius) {
        m_path.lineTo(st);
    } else {
        addArcTo(st, end, radius);
    }
}

void QSGContext2D::rect(qreal x, qreal y,
                               qreal w, qreal h)
{
    m_path.addPolygon(state.matrix.map(QRectF(x, y, w, h)));
}

void QSGContext2D::roundedRect(qreal x, qreal y,
                               qreal w, qreal h,
                               qreal xr, qreal yr)
{
    QPainterPath path;
    path.addRoundedRect(QRectF(x, y, w, h), xr, yr, Qt::AbsoluteSize);
    m_path.addPath(state.matrix.map(path));
}

void QSGContext2D::ellipse(qreal x, qreal y,
                           qreal w, qreal h)
{
    QPainterPath path;
    path.addEllipse(x, y, w, h);
    m_path.addPath(state.matrix.map(path));
}

void QSGContext2D::text(const QString& str, qreal x, qreal y)
{
    QPainterPath path;
    path.addText(x, y, state.font, str);
    m_path.addPath(state.matrix.map(path));
}

void QSGContext2D::arc(qreal xc,
                       qreal yc,
                       qreal radius,
                       qreal sar,
                       qreal ear,
                       bool antiClockWise,
                       bool transform)
{

    if (transform) {
        QPointF point = state.matrix.map(QPointF(xc, yc));
        xc = point.x();
        yc = point.y();
    }
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
        if (!m_path.elementCount())
            m_path.moveTo(xs, ys);
    }


    if (transform) {
        QPointF currentPos = m_path.currentPosition();
        QPointF startPos = QPointF(xc + radius  * qCos(sar),
                                   yc - radius  * qSin(sar));
        if (currentPos != startPos)
            m_path.lineTo(startPos);
    }

    m_path.arcTo(xs, ys, width, height, sa, span);
}

int baseLineOffset(QSGContext2D::TextBaseLineType value, const QFontMetrics &metrics)
{
    int offset = 0;
    switch (value) {
    case QSGContext2D::QSGContext2D::Top:
        break;
    case QSGContext2D::QSGContext2D::Alphabetic:
    case QSGContext2D::QSGContext2D::Middle:
    case QSGContext2D::QSGContext2D::Hanging:
        offset = metrics.ascent();
        break;
    case QSGContext2D::QSGContext2D::Bottom:
        offset = metrics.height();
       break;
    }
    return offset;
}

static int textAlignOffset(QSGContext2D::TextAlignType value, const QFontMetrics &metrics, const QString &text)
{
    int offset = 0;
    if (value == QSGContext2D::Start)
        value = QGuiApplication::layoutDirection() == Qt::LeftToRight ? QSGContext2D::Left : QSGContext2D::Right;
    else if (value == QSGContext2D::End)
        value = QGuiApplication::layoutDirection() == Qt::LeftToRight ? QSGContext2D::Right: QSGContext2D::Left;
    switch (value) {
    case QSGContext2D::QSGContext2D::Center:
        offset = metrics.width(text)/2;
        break;
    case QSGContext2D::QSGContext2D::Right:
        offset = metrics.width(text);
    case QSGContext2D::QSGContext2D::Left:
    default:
        break;
    }
    return offset;
}


QImage QSGContext2D::createImage(const QUrl& url)
{
    return m_canvas->loadedImage(url);
}

QPainterPath QSGContext2D::createTextGlyphs(qreal x, qreal y, const QString& text)
{
    const QFontMetrics metrics(state.font);
    int yoffset = baseLineOffset(static_cast<QSGContext2D::TextBaseLineType>(state.textBaseline), metrics);
    int xoffset = textAlignOffset(static_cast<QSGContext2D::TextAlignType>(state.textAlign), metrics, text);

    QPainterPath textPath;

    textPath.addText(x - xoffset, y - yoffset+metrics.ascent(), state.font, text);
    textPath = state.matrix.map(textPath);
    return textPath;
}


bool QSGContext2D::isPointInPath(qreal x, qreal y) const
{
    return m_path.contains(QPointF(x, y));
}

QSGContext2D::QSGContext2D(QSGCanvasItem* item)
    : m_canvas(item)
    , m_buffer(new QSGContext2DCommandBuffer)
    , m_v8engine(0)
{
    reset();
}

QSGContext2D::~QSGContext2D()
{
}

v8::Handle<v8::Object> QSGContext2D::v8value() const
{
    return m_v8value;
}

QSGContext2DEngineData::QSGContext2DEngineData(QV8Engine *engine)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope scope(engine->context());

    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->PrototypeTemplate()->SetAccessor(v8::String::New("canvas"), ctx2d_canvas, 0, v8::External::Wrap(engine));
    ft->PrototypeTemplate()->Set(v8::String::New("restore"), V8FUNCTION(ctx2d_restore, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("reset"), V8FUNCTION(ctx2d_reset, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("save"), V8FUNCTION(ctx2d_save, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("rotate"), V8FUNCTION(ctx2d_rotate, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("scale"), V8FUNCTION(ctx2d_scale, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("resetTransform"), V8FUNCTION(ctx2d_resetTransform, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("setTransform"), V8FUNCTION(ctx2d_setTransform, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("transform"), V8FUNCTION(ctx2d_transform, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("translate"), V8FUNCTION(ctx2d_translate, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("shear"), V8FUNCTION(ctx2d_shear, engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("globalAlpha"), ctx2d_globalAlpha, ctx2d_globalAlpha_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("globalCompositeOperation"), ctx2d_globalCompositeOperation, ctx2d_globalCompositeOperation_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("fillRule"), ctx2d_fillRule, ctx2d_fillRule_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("fillStyle"), ctx2d_fillStyle, ctx2d_fillStyle_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("strokeStyle"), ctx2d_strokeStyle, ctx2d_strokeStyle_set, v8::External::Wrap(engine));
    ft->PrototypeTemplate()->Set(v8::String::New("createLinearGradient"), V8FUNCTION(ctx2d_createLinearGradient, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("createRadialGradient"), V8FUNCTION(ctx2d_createRadialGradient, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("createPattern"), V8FUNCTION(ctx2d_createPattern, engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("lineCap"), ctx2d_lineCap, ctx2d_lineCap_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("lineJoin"), ctx2d_lineJoin, ctx2d_lineJoin_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("lineWidth"), ctx2d_lineWidth, ctx2d_lineWidth_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("miterLimit"), ctx2d_miterLimit, ctx2d_miterLimit_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("shadowBlur"), ctx2d_shadowBlur, ctx2d_shadowBlur_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("shadowColor"), ctx2d_shadowColor, ctx2d_shadowColor_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("shadowOffsetX"), ctx2d_shadowOffsetX, ctx2d_shadowOffsetX_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("shadowOffsetY"), ctx2d_shadowOffsetY, ctx2d_shadowOffsetY_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("path"), ctx2d_path, ctx2d_path_set, v8::External::Wrap(engine));
    ft->PrototypeTemplate()->Set(v8::String::New("clearRect"), V8FUNCTION(ctx2d_clearRect, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("fillRect"), V8FUNCTION(ctx2d_fillRect, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("strokeRect"), V8FUNCTION(ctx2d_strokeRect, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("arc"), V8FUNCTION(ctx2d_arc, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("arcTo"), V8FUNCTION(ctx2d_arcTo, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("beginPath"), V8FUNCTION(ctx2d_beginPath, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("bezierCurveTo"), V8FUNCTION(ctx2d_bezierCurveTo, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("clip"), V8FUNCTION(ctx2d_clip, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("closePath"), V8FUNCTION(ctx2d_closePath, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("fill"), V8FUNCTION(ctx2d_fill, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("lineTo"), V8FUNCTION(ctx2d_lineTo, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("moveTo"), V8FUNCTION(ctx2d_moveTo, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("quadraticCurveTo"), V8FUNCTION(ctx2d_quadraticCurveTo, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("rect"), V8FUNCTION(ctx2d_rect, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("roundedRect"), V8FUNCTION(ctx2d_roundedRect, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("text"), V8FUNCTION(ctx2d_text, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("ellipse"), V8FUNCTION(ctx2d_ellipse, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("stroke"), V8FUNCTION(ctx2d_stroke, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("isPointInPath"), V8FUNCTION(ctx2d_isPointInPath, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("drawFocusRing"), V8FUNCTION(ctx2d_drawFocusRing, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("caretBlinkRate"), V8FUNCTION(ctx2d_caretBlinkRate, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("setCaretSelectionRect"), V8FUNCTION(ctx2d_setCaretSelectionRect, engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("font"), ctx2d_font, ctx2d_font_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("textAlign"), ctx2d_textAlign, ctx2d_textAlign_set, v8::External::Wrap(engine));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("textBaseline"), ctx2d_textBaseline, ctx2d_textBaseline_set, v8::External::Wrap(engine));
    ft->PrototypeTemplate()->Set(v8::String::New("fillText"), V8FUNCTION(ctx2d_fillText, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("measureText"), V8FUNCTION(ctx2d_measureText, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("strokeText"), V8FUNCTION(ctx2d_strokeText, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("drawImage"), V8FUNCTION(ctx2d_drawImage, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("createImageData"), V8FUNCTION(ctx2d_createImageData, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("getImageData"), V8FUNCTION(ctx2d_getImageData, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("putImageData"), V8FUNCTION(ctx2d_putImageData, engine));

    constructorContext = qPersistentNew(ft->GetFunction());

    v8::Local<v8::FunctionTemplate> ftGradient = v8::FunctionTemplate::New();
    ftGradient->InstanceTemplate()->SetHasExternalResource(true);
    ftGradient->PrototypeTemplate()->Set(v8::String::New("addColorStop"), V8FUNCTION(ctx2d_gradient_addColorStop, engine));
    constructorGradient = qPersistentNew(ftGradient->GetFunction());

    v8::Local<v8::FunctionTemplate> ftPattern = v8::FunctionTemplate::New();
    ftPattern->InstanceTemplate()->SetHasExternalResource(true);
    constructorPattern = qPersistentNew(ftPattern->GetFunction());

    v8::Local<v8::FunctionTemplate> ftPixelArray = v8::FunctionTemplate::New();
    ftPixelArray->InstanceTemplate()->SetHasExternalResource(true);
    ftPixelArray->InstanceTemplate()->SetAccessor(v8::String::New("length"), ctx2d_pixelArray_length, 0, v8::External::Wrap(engine));
    ftPixelArray->InstanceTemplate()->SetIndexedPropertyHandler(ctx2d_pixelArray_indexed, ctx2d_pixelArray_indexed_set, 0, 0, 0, v8::External::Wrap(engine));
    constructorPixelArray = qPersistentNew(ftPixelArray->GetFunction());

    v8::Local<v8::FunctionTemplate> ftImageData = v8::FunctionTemplate::New();
    ftImageData->InstanceTemplate()->SetAccessor(v8::String::New("width"), ctx2d_imageData_width, 0, v8::External::Wrap(engine));
    ftImageData->InstanceTemplate()->SetAccessor(v8::String::New("height"), ctx2d_imageData_height, 0, v8::External::Wrap(engine));
    ftImageData->InstanceTemplate()->SetAccessor(v8::String::New("data"), ctx2d_imageData_data, 0, v8::External::Wrap(engine));
    ftImageData->PrototypeTemplate()->Set(v8::String::New("mirror"), V8FUNCTION(ctx2d_imageData_mirror, engine));
    ftImageData->PrototypeTemplate()->Set(v8::String::New("filter"), V8FUNCTION(ctx2d_imageData_filter, engine));
    ftImageData->InstanceTemplate()->SetInternalFieldCount(1);
    constructorImageData = qPersistentNew(ftImageData->GetFunction());
}

QSGContext2DEngineData::~QSGContext2DEngineData()
{
    qPersistentDispose(constructorContext);
    qPersistentDispose(constructorGradient);
    qPersistentDispose(constructorPattern);
    qPersistentDispose(constructorImageData);
    qPersistentDispose(constructorPixelArray);
}

void QSGContext2D::popState()
{
    if (m_stateStack.isEmpty())
        return;

    QSGContext2D::State newState = m_stateStack.pop();

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

    if (newState.clipPath != state.clipPath)
        buffer()->clip(newState.clipPath);

    if (newState.shadowBlur != state.shadowBlur)
        buffer()->setShadowBlur(newState.shadowBlur);

    if (newState.shadowColor != state.shadowColor)
        buffer()->setShadowColor(newState.shadowColor);

    if (newState.shadowOffsetX != state.shadowOffsetX)
        buffer()->setShadowOffsetX(newState.shadowOffsetX);

    if (newState.shadowOffsetY != state.shadowOffsetY)
        buffer()->setShadowOffsetY(newState.shadowOffsetY);

    state = newState;
}
void QSGContext2D::pushState()
{
    m_stateStack.push(state);
}

void QSGContext2D::reset()
{
    QSGContext2D::State newState;
    newState.matrix = QTransform();

    QPainterPath defaultClipPath;
    defaultClipPath.addRect(0, 0, m_canvas->canvasSize().width(), m_canvas->canvasSize().height());
    newState.clipPath = defaultClipPath;
    newState.clipPath.setFillRule(Qt::WindingFill);

    newState.strokeStyle = Qt::black;
    newState.fillStyle = Qt::black;
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
    newState.font = QFont(QLatin1String("sans-serif"), 10);
    newState.textAlign = QSGContext2D::Start;
    newState.textBaseline = QSGContext2D::Alphabetic;

    m_fontString = "";
    m_stateStack.clear();
    m_stateStack.push(newState);
    popState();
}

void QSGContext2D::setV8Engine(QV8Engine *engine)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope scope(engine->context());

    if (m_v8engine != engine) {
        m_v8engine = engine;

        qPersistentDispose(m_v8value);

        if (m_v8engine == 0)
            return;

        QSGContext2DEngineData *ed = engineData(engine);
        m_v8value = qPersistentNew(ed->constructorContext->NewInstance());
        QV8Context2DResource *r = new QV8Context2DResource(engine);
        r->context = this;
        m_v8value->SetExternalResource(r);
    }
}

QT_END_NAMESPACE
