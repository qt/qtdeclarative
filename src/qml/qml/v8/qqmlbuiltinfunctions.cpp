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

#include "qqmlbuiltinfunctions_p.h"

#include <QtQml/qqmlcomponent.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlstringconverters_p.h>
#include <private/qqmllocale_p.h>
#include <private/qv8engine_p.h>
#include <private/qjsconverter_impl_p.h>

#include <private/qv8profilerservice_p.h>
#include <private/qqmlprofilerservice_p.h>
#include <private/qqmlglobal_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>
#include <QtCore/qurl.h>
#include <QtCore/qfile.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

namespace QQmlBuiltinFunctions {

enum ConsoleLogTypes {
    Log,
    Warn,
    Error
};

static void jsContext(v8::Handle<v8::Value> *file, int *line, v8::Handle<v8::Value> *function) {
    v8::Local<v8::StackTrace> stackTrace = v8::StackTrace::CurrentStackTrace(1);
    if (stackTrace->GetFrameCount()) {
        v8::Local<v8::StackFrame> frame = stackTrace->GetFrame(0);
        *file = frame->GetScriptName();
        *line = frame->GetLineNumber();
        *function = frame->GetFunctionName();
    }
}

static QString jsStack() {
    QStringList stackFrames;

    //The v8 default is currently 10 stack frames.
    v8::Handle<v8::StackTrace> stackTrace =
        v8::StackTrace::CurrentStackTrace(10, v8::StackTrace::kOverview);
    int stackCount = stackTrace->GetFrameCount();

    for (int i = 0; i < stackCount; i++) {
        v8::Local<v8::StackFrame> frame = stackTrace->GetFrame(i);
        v8::Handle<v8::String> function(frame->GetFunctionName());
        v8::Handle<v8::String> script(frame->GetScriptName());
        int lineNumber = frame->GetLineNumber();
        int columnNumber = frame->GetColumn();

        QString stackFrame =
                QString::fromLatin1("%1 (%2:%3:%4)").arg(QJSConverter::toString(function),
                                                         QJSConverter::toString(script),
                                                         QString::number(lineNumber),
                                                         QString::number(columnNumber));
        stackFrames.append(stackFrame);
    }
    return stackFrames.join(QLatin1String("\n"));
}

v8::Handle<v8::Value> console(ConsoleLogTypes logType, const v8::Arguments &args,
                              bool printStack = false)
{
    v8::HandleScope handleScope;

    QString result;
    QV8Engine *engine = V8ENGINE();
    for (int i = 0; i < args.Length(); ++i) {
        if (i != 0)
            result.append(QLatin1Char(' '));

        v8::Local<v8::Value> value = args[i];

        v8::TryCatch tryCatch;
        v8::Local<v8::String> toString = value->ToString();
        if (tryCatch.HasCaught()) {
            // toString() threw Exception
            // Is it possible for value to be anything other than Object?
            QString str;
            if (value->IsObject()) {
                str = QStringLiteral("[object Object]");
            } else {
                toString = tryCatch.Exception()->ToString();
                str = QStringLiteral("toString() threw exception: %1")
                        .arg(engine->toString(toString));
            }
            result.append(str);
            continue;
        }

        QString tmp = engine->toString(toString);
        if (value->IsArray())
            result.append(QStringLiteral("[%1]").arg(tmp));
        else
            result.append(tmp);
    }

    if (printStack) {
        result.append(QLatin1String("\n"));
        result.append(jsStack());
    }

    v8::Handle<v8::Value> fileHandle;
    v8::Handle<v8::Value> functionHandle;
    int line;

    jsContext(&fileHandle, &line, &functionHandle);

    switch (logType) {
    case Log:
        QMessageLogger(*v8::String::AsciiValue(fileHandle), line,
                       *v8::String::AsciiValue(functionHandle)).debug("%s", qPrintable(result));
        break;
    case Warn:
        QMessageLogger(*v8::String::AsciiValue(fileHandle), line,
                       *v8::String::AsciiValue(functionHandle)).warning("%s", qPrintable(result));
        break;
    case Error:
        QMessageLogger(*v8::String::AsciiValue(fileHandle), line,
                       *v8::String::AsciiValue(functionHandle)).critical("%s", qPrintable(result));
        break;
    default:
        break;
    }

    return v8::Undefined();
}

v8::Handle<v8::Value> gc(const v8::Arguments &args)
{
    Q_UNUSED(args);
    QV8Engine::gc();
    return v8::Undefined();
}

v8::Handle<v8::Value> consoleError(const v8::Arguments &args)
{
    return console(Error, args);
}

v8::Handle<v8::Value> consoleLog(const v8::Arguments &args)
{
    //console.log
    //console.debug
    //console.info
    //print
    return console(Log, args);
}

v8::Handle<v8::Value> consoleProfile(const v8::Arguments &args)
{
    //DeclarativeDebugTrace cannot handle nested profiling
    //although v8 can handle several profiling at once,
    //we do not allow that. Hence, we pass an empty(default) title
    Q_UNUSED(args);
    QString title;



    v8::Handle<v8::Value> file;
    v8::Handle<v8::Value> function;
    int line;
    jsContext(&file, &line, &function);

    if (QQmlProfilerService::startProfiling()) {
        QV8ProfilerService::instance()->startProfiling(title);

        QMessageLogger(*v8::String::AsciiValue(file), line,
                       *v8::String::AsciiValue(function)).debug("Profiling started.");
    } else {
        QMessageLogger(*v8::String::AsciiValue(file), line,
                       *v8::String::AsciiValue(function)).warning(
                    "Profiling is already in progress. First, end current profiling session.");
    }

    return v8::Undefined();
}

v8::Handle<v8::Value> consoleProfileEnd(const v8::Arguments &args)
{
    //DeclarativeDebugTrace cannot handle nested profiling
    //although v8 can handle several profiling at once,
    //we do not allow that. Hence, we pass an empty(default) title
    Q_UNUSED(args);
    QString title;

    v8::Handle<v8::Value> file;
    v8::Handle<v8::Value> function;
    int line;
    jsContext(&file, &line, &function);

    if (QQmlProfilerService::stopProfiling()) {
        QV8ProfilerService *profiler = QV8ProfilerService::instance();
        profiler->stopProfiling(title);
        QQmlProfilerService::sendProfilingData();
        profiler->sendProfilingData();

        QMessageLogger(*v8::String::AsciiValue(file), line,
                       *v8::String::AsciiValue(function)).debug("Profiling ended.");
    } else {
        QMessageLogger(*v8::String::AsciiValue(file), line,
                       *v8::String::AsciiValue(function)).warning("Profiling was not started.");
    }

    return v8::Undefined();
}

v8::Handle<v8::Value> consoleTime(const v8::Arguments &args)
{
    if (args.Length() != 1)
        V8THROW_ERROR("console.time(): Invalid arguments");
    QString name = V8ENGINE()->toString(args[0]);
    V8ENGINE()->startTimer(name);
    return v8::Undefined();
}

v8::Handle<v8::Value> consoleTimeEnd(const v8::Arguments &args)
{
    if (args.Length() != 1)
        V8THROW_ERROR("console.time(): Invalid arguments");
    QString name = V8ENGINE()->toString(args[0]);
    bool wasRunning;
    qint64 elapsed = V8ENGINE()->stopTimer(name, &wasRunning);
    if (wasRunning) {
        qDebug("%s: %llims", qPrintable(name), elapsed);
    }
    return v8::Undefined();
}

v8::Handle<v8::Value> consoleCount(const v8::Arguments &args)
{
    // first argument: name to print. Ignore any additional arguments
    QString name;
    if (args.Length() > 0)
        name = V8ENGINE()->toString(args[0]);

    v8::Handle<v8::StackTrace> stackTrace =
        v8::StackTrace::CurrentStackTrace(1, v8::StackTrace::kOverview);

    if (stackTrace->GetFrameCount()) {
        v8::Local<v8::StackFrame> frame = stackTrace->GetFrame(0);

        QString scriptName = V8ENGINE()->toString(frame->GetScriptName());
        QString functionName = V8ENGINE()->toString(frame->GetFunctionName());
        int line = frame->GetLineNumber();
        int column = frame->GetColumn();

        int value = V8ENGINE()->consoleCountHelper(scriptName, line, column);
        QString message = name + QLatin1String(": ") + QString::number(value);

        QMessageLogger(qPrintable(scriptName), line,
                       qPrintable(functionName)).debug("%s", qPrintable(message));
    }

    return v8::Undefined();
}

v8::Handle<v8::Value> consoleTrace(const v8::Arguments &args)
{
    if (args.Length() != 0)
        V8THROW_ERROR("console.trace(): Invalid arguments");

    QString stack = jsStack();

    v8::Handle<v8::Value> file;
    v8::Handle<v8::Value> function;
    int line;
    jsContext(&file, &line, &function);

    QMessageLogger(*v8::String::AsciiValue(file), line, *v8::String::AsciiValue(function)).debug(
                "%s", qPrintable(stack));
    return v8::Undefined();
}

v8::Handle<v8::Value> consoleWarn(const v8::Arguments &args)
{
    return console(Warn, args);
}

v8::Handle<v8::Value> consoleAssert(const v8::Arguments &args)
{
    if (args.Length() == 0)
        V8THROW_ERROR("console.assert(): Missing argument");

    if (!args[0]->ToBoolean()->Value()) {
        QString message;
        for (int i = 1; i < args.Length(); ++i) {
            if (i != 1)
                message.append(QLatin1Char(' '));

            v8::Local<v8::Value> value = args[i];
            message.append(V8ENGINE()->toString(value->ToString()));
        }

        QString stack = jsStack();

        v8::Handle<v8::Value> file;
        v8::Handle<v8::Value> function;
        int line;
        jsContext(&file, &line, &function);

        QMessageLogger(*v8::String::AsciiValue(file), line, *v8::String::AsciiValue(function)).critical(
                    "%s\n%s", qPrintable(message), qPrintable(stack));

    }
    return v8::Undefined();
}

v8::Handle<v8::Value> consoleException(const v8::Arguments &args)
{
    if (args.Length() == 0)
        V8THROW_ERROR("console.exception(): Missing argument");

    console(Error, args, true);

    return v8::Undefined();
}

v8::Handle<v8::Value> stringArg(const v8::Arguments &args)
{
    QString value = V8ENGINE()->toString(args.This()->ToString());
    if (args.Length() != 1)
        V8THROW_ERROR("String.arg(): Invalid arguments");

    v8::Handle<v8::Value> arg = args[0];
    if (arg->IsUint32())
        return V8ENGINE()->toString(value.arg(arg->Uint32Value()));
    else if (arg->IsInt32())
        return V8ENGINE()->toString(value.arg(arg->Int32Value()));
    else if (arg->IsNumber())
        return V8ENGINE()->toString(value.arg(arg->NumberValue()));
    else if (arg->IsBoolean())
        return V8ENGINE()->toString(value.arg(arg->BooleanValue()));

    return V8ENGINE()->toString(value.arg(V8ENGINE()->toString(arg)));
}

/*!
\qmlmethod bool Qt::isQtObject(object)
Returns true if \c object is a valid reference to a Qt or QML object, otherwise false.
*/
v8::Handle<v8::Value> isQtObject(const v8::Arguments &args)
{
    if (args.Length() == 0)
        return v8::Boolean::New(false);

    return v8::Boolean::New(0 != V8ENGINE()->toQObject(args[0]));
}

/*!
\qmlmethod color Qt::rgba(real red, real green, real blue, real alpha)

Returns a color with the specified \c red, \c green, \c blue and \c alpha components.
All components should be in the range 0-1 inclusive.
*/
v8::Handle<v8::Value> rgba(const v8::Arguments &args)
{
    int argCount = args.Length();
    if (argCount < 3 || argCount > 4)
        V8THROW_ERROR("Qt.rgba(): Invalid arguments");

    double r = args[0]->NumberValue();
    double g = args[1]->NumberValue();
    double b = args[2]->NumberValue();
    double a = (argCount == 4) ? args[3]->NumberValue() : 1;

    if (r < 0.0) r=0.0;
    if (r > 1.0) r=1.0;
    if (g < 0.0) g=0.0;
    if (g > 1.0) g=1.0;
    if (b < 0.0) b=0.0;
    if (b > 1.0) b=1.0;
    if (a < 0.0) a=0.0;
    if (a > 1.0) a=1.0;

    return V8ENGINE()->fromVariant(QQml_colorProvider()->fromRgbF(r, g, b, a));
}

/*!
\qmlmethod color Qt::hsla(real hue, real saturation, real lightness, real alpha)

Returns a color with the specified \c hue, \c saturation, \c lightness and \c alpha components.
All components should be in the range 0-1 inclusive.
*/
v8::Handle<v8::Value> hsla(const v8::Arguments &args)
{
    int argCount = args.Length();
    if (argCount < 3 || argCount > 4)
        V8THROW_ERROR("Qt.hsla(): Invalid arguments");

    double h = args[0]->NumberValue();
    double s = args[1]->NumberValue();
    double l = args[2]->NumberValue();
    double a = (argCount == 4) ? args[3]->NumberValue() : 1;

    if (h < 0.0) h=0.0;
    if (h > 1.0) h=1.0;
    if (s < 0.0) s=0.0;
    if (s > 1.0) s=1.0;
    if (l < 0.0) l=0.0;
    if (l > 1.0) l=1.0;
    if (a < 0.0) a=0.0;
    if (a > 1.0) a=1.0;

    return V8ENGINE()->fromVariant(QQml_colorProvider()->fromHslF(h, s, l, a));
}

/*!
\qmlmethod color Qt::colorEqual(color lhs, string rhs)

Returns true if both \c lhs and \c rhs yield equal color values.  Both arguments
may be either color values or string values.  If a string value is supplied it
must be convertible to a color, as described for the \l{colorbasictypedocs}{color}
basic type.
*/
v8::Handle<v8::Value> colorEqual(const v8::Arguments &args)
{
    if (args.Length() != 2)
        V8THROW_ERROR("Qt.colorEqual(): Invalid arguments");

    bool ok = false;

    QVariant lhs = V8ENGINE()->toVariant(args[0], -1);
    if (lhs.userType() == QVariant::String) {
        lhs = QQmlStringConverters::colorFromString(lhs.toString(), &ok);
        if (!ok) {
            V8THROW_ERROR("Qt.colorEqual(): Invalid color name");
        }
    } else if (lhs.userType() != QVariant::Color) {
        V8THROW_ERROR("Qt.colorEqual(): Invalid arguments");
    }

    QVariant rhs = V8ENGINE()->toVariant(args[1], -1);
    if (rhs.userType() == QVariant::String) {
        rhs = QQmlStringConverters::colorFromString(rhs.toString(), &ok);
        if (!ok) {
            V8THROW_ERROR("Qt.colorEqual(): Invalid color name");
        }
    } else if (rhs.userType() != QVariant::Color) {
        V8THROW_ERROR("Qt.colorEqual(): Invalid arguments");
    }

    bool equal = (lhs == rhs);
    return V8ENGINE()->fromVariant(equal);
}

/*!
\qmlmethod rect Qt::rect(int x, int y, int width, int height)

Returns a \c rect with the top-left corner at \c x, \c y and the specified \c width and \c height.

The returned object has \c x, \c y, \c width and \c height attributes with the given values.
*/
v8::Handle<v8::Value> rect(const v8::Arguments &args)
{
    if (args.Length() != 4)
        V8THROW_ERROR("Qt.rect(): Invalid arguments");

    double x = args[0]->NumberValue();
    double y = args[1]->NumberValue();
    double w = args[2]->NumberValue();
    double h = args[3]->NumberValue();

    return V8ENGINE()->fromVariant(QVariant::fromValue(QRectF(x, y, w, h)));
}

/*!
\qmlmethod point Qt::point(int x, int y)
Returns a Point with the specified \c x and \c y coordinates.
*/
v8::Handle<v8::Value> point(const v8::Arguments &args)
{
    if (args.Length() != 2)
        V8THROW_ERROR("Qt.point(): Invalid arguments");

    double x = args[0]->ToNumber()->Value();
    double y = args[1]->ToNumber()->Value();

    return V8ENGINE()->fromVariant(QVariant::fromValue(QPointF(x, y)));
}

/*!
\qmlmethod Qt::size(int width, int height)
Returns a Size with the specified \c width and \c height.
*/
v8::Handle<v8::Value> size(const v8::Arguments &args)
{
    if (args.Length() != 2)
        V8THROW_ERROR("Qt.size(): Invalid arguments");

    double w = args[0]->ToNumber()->Value();
    double h = args[1]->ToNumber()->Value();

    return V8ENGINE()->fromVariant(QVariant::fromValue(QSizeF(w, h)));
}

/*!
\qmlmethod Qt::vector2d(real x, real y)
Returns a Vector2D with the specified \c x and \c y.
*/
v8::Handle<v8::Value> vector2d(const v8::Arguments &args)
{
    if (args.Length() != 2)
        V8THROW_ERROR("Qt.vector2d(): Invalid arguments");

    float xy[3]; // qvector2d uses float internally
    xy[0] = args[0]->ToNumber()->Value();
    xy[1] = args[1]->ToNumber()->Value();

    const void *params[] = { xy };
    return V8ENGINE()->fromVariant(QQml_valueTypeProvider()->createValueType(QMetaType::QVector2D, 1, params));
}

/*!
\qmlmethod Qt::vector3d(real x, real y, real z)
Returns a Vector3D with the specified \c x, \c y and \c z.
*/
v8::Handle<v8::Value> vector3d(const v8::Arguments &args)
{
    if (args.Length() != 3)
        V8THROW_ERROR("Qt.vector3d(): Invalid arguments");

    float xyz[3]; // qvector3d uses float internally
    xyz[0] = args[0]->ToNumber()->Value();
    xyz[1] = args[1]->ToNumber()->Value();
    xyz[2] = args[2]->ToNumber()->Value();

    const void *params[] = { xyz };
    return V8ENGINE()->fromVariant(QQml_valueTypeProvider()->createValueType(QMetaType::QVector3D, 1, params));
}

/*!
\qmlmethod Qt::vector4d(real x, real y, real z, real w)
Returns a Vector4D with the specified \c x, \c y, \c z and \c w.
*/
v8::Handle<v8::Value> vector4d(const v8::Arguments &args)
{
    if (args.Length() != 4)
        V8THROW_ERROR("Qt.vector4d(): Invalid arguments");

    float xyzw[4]; // qvector4d uses float internally
    xyzw[0] = args[0]->ToNumber()->Value();
    xyzw[1] = args[1]->ToNumber()->Value();
    xyzw[2] = args[2]->ToNumber()->Value();
    xyzw[3] = args[3]->ToNumber()->Value();

    const void *params[] = { xyzw };
    return V8ENGINE()->fromVariant(QQml_valueTypeProvider()->createValueType(QMetaType::QVector4D, 1, params));
}

/*!
\qmlmethod Qt::quaternion(real scalar, real x, real y, real z)
Returns a Quaternion with the specified \c scalar, \c x, \c y, and \c z.
*/
v8::Handle<v8::Value> quaternion(const v8::Arguments &args)
{
    if (args.Length() != 4)
        V8THROW_ERROR("Qt.quaternion(): Invalid arguments");

    qreal sxyz[4]; // qquaternion uses qreal internally
    sxyz[0] = args[0]->ToNumber()->Value();
    sxyz[1] = args[1]->ToNumber()->Value();
    sxyz[2] = args[2]->ToNumber()->Value();
    sxyz[3] = args[3]->ToNumber()->Value();

    const void *params[] = { sxyz };
    return V8ENGINE()->fromVariant(QQml_valueTypeProvider()->createValueType(QMetaType::QQuaternion, 1, params));
}

/*!
\qmlmethod Qt::font(object fontSpecifier)
Returns a Font with the properties specified in the \c fontSpecifier object
or the nearest matching font.  The \c fontSpecifier object should contain
key-value pairs where valid keys are the \l{fontbasictypedocs}{font} type's
subproperty names, and the values are valid values for each subproperty.
Invalid keys will be ignored.
*/
v8::Handle<v8::Value> font(const v8::Arguments &args)
{
    if (args.Length() != 1 || !args[0]->IsObject())
        V8THROW_ERROR("Qt.font(): Invalid arguments");

    v8::Handle<v8::Object> obj = args[0]->ToObject();
    bool ok = false;
    QVariant v = QQml_valueTypeProvider()->createVariantFromJsObject(QMetaType::QFont, QQmlV8Handle::fromHandle(obj), V8ENGINE(), &ok);
    if (!ok)
        V8THROW_ERROR("Qt.font(): Invalid argument: no valid font subproperties specified");
    return V8ENGINE()->fromVariant(v);
}

/*!
\qmlmethod Qt::matrix4x4(real m11, real m12, real m13, real m14, real m21, real m22, real m23, real m24, real m31, real m32, real m33, real m34, real m41, real m42, real m43, real m44)
Returns a Matrix4x4 with the specified values.
Alternatively, the function may be called with a single argument
where that argument is a JavaScript array which contains the sixteen
matrix values.
*/
v8::Handle<v8::Value> matrix4x4(const v8::Arguments &args)
{
    if (args.Length() == 1 && args[0]->IsObject()) {
        v8::Handle<v8::Object> obj = args[0]->ToObject();
        bool ok = false;
        QVariant v = QQml_valueTypeProvider()->createVariantFromJsObject(QMetaType::QMatrix4x4, QQmlV8Handle::fromHandle(obj), V8ENGINE(), &ok);
        if (!ok)
            V8THROW_ERROR("Qt.matrix4x4(): Invalid argument: not a valid matrix4x4 values array");
        return V8ENGINE()->fromVariant(v);
    }

    if (args.Length() != 16)
        V8THROW_ERROR("Qt.matrix4x4(): Invalid arguments");

    qreal vals[16]; // qmatrix4x4 uses qreal internally
    vals[0] = args[0]->ToNumber()->Value();
    vals[1] = args[1]->ToNumber()->Value();
    vals[2] = args[2]->ToNumber()->Value();
    vals[3] = args[3]->ToNumber()->Value();
    vals[4] = args[4]->ToNumber()->Value();
    vals[5] = args[5]->ToNumber()->Value();
    vals[6] = args[6]->ToNumber()->Value();
    vals[7] = args[7]->ToNumber()->Value();
    vals[8] = args[8]->ToNumber()->Value();
    vals[9] = args[9]->ToNumber()->Value();
    vals[10] = args[10]->ToNumber()->Value();
    vals[11] = args[11]->ToNumber()->Value();
    vals[12] = args[12]->ToNumber()->Value();
    vals[13] = args[13]->ToNumber()->Value();
    vals[14] = args[14]->ToNumber()->Value();
    vals[15] = args[15]->ToNumber()->Value();

    const void *params[] = { vals };
    return V8ENGINE()->fromVariant(QQml_valueTypeProvider()->createValueType(QMetaType::QMatrix4x4, 1, params));
}

/*!
\qmlmethod color Qt::lighter(color baseColor, real factor)
Returns a color lighter than \c baseColor by the \c factor provided.

If the factor is greater than 1.0, this functions returns a lighter color.
Setting factor to 1.5 returns a color that is 50% brighter. If the factor is less than 1.0,
the return color is darker, but we recommend using the Qt.darker() function for this purpose.
If the factor is 0 or negative, the return value is unspecified.

The function converts the current RGB color to HSV, multiplies the value (V) component
by factor and converts the color back to RGB.

If \c factor is not supplied, returns a color 50% lighter than \c baseColor (factor 1.5).
*/
v8::Handle<v8::Value> lighter(const v8::Arguments &args)
{
    if (args.Length() != 1 && args.Length() != 2)
        V8THROW_ERROR("Qt.lighter(): Invalid arguments");

    QVariant v = V8ENGINE()->toVariant(args[0], -1);
    if (v.userType() == QVariant::String) {
        bool ok = false;
        v = QQmlStringConverters::colorFromString(v.toString(), &ok);
        if (!ok) {
            return v8::Null();
        }
    } else if (v.userType() != QVariant::Color) {
        return v8::Null();
    }

    qreal factor = 1.5;
    if (args.Length() == 2)
        factor = args[1]->ToNumber()->Value();

    return V8ENGINE()->fromVariant(QQml_colorProvider()->lighter(v, factor));
}

/*!
\qmlmethod color Qt::darker(color baseColor, real factor)
Returns a color darker than \c baseColor by the \c factor provided.

If the factor is greater than 1.0, this function returns a darker color.
Setting factor to 3.0 returns a color that has one-third the brightness.
If the factor is less than 1.0, the return color is lighter, but we recommend using
the Qt.lighter() function for this purpose. If the factor is 0 or negative, the return
value is unspecified.

The function converts the current RGB color to HSV, divides the value (V) component
by factor and converts the color back to RGB.

If \c factor is not supplied, returns a color 50% darker than \c baseColor (factor 2.0).
*/
v8::Handle<v8::Value> darker(const v8::Arguments &args)
{
    if (args.Length() != 1 && args.Length() != 2)
        V8THROW_ERROR("Qt.darker(): Invalid arguments");

    QVariant v = V8ENGINE()->toVariant(args[0], -1);
    if (v.userType() == QVariant::String) {
        bool ok = false;
        v = QQmlStringConverters::colorFromString(v.toString(), &ok);
        if (!ok) {
            return v8::Null();
        }
    } else if (v.userType() != QVariant::Color) {
        return v8::Null();
    }

    qreal factor = 2.0;
    if (args.Length() == 2)
        factor = args[1]->ToNumber()->Value();

    return V8ENGINE()->fromVariant(QQml_colorProvider()->darker(v, factor));
}

/*!
    \qmlmethod color Qt::tint(color baseColor, color tintColor)
    This function allows tinting one color with another.

    The tint color should usually be mostly transparent, or you will not be
    able to see the underlying color. The below example provides a slight red
    tint by having the tint color be pure red which is only 1/16th opaque.

    \qml
    Item {
        Rectangle {
            x: 0; width: 80; height: 80
            color: "lightsteelblue"
        }
        Rectangle {
            x: 100; width: 80; height: 80
            color: Qt.tint("lightsteelblue", "#10FF0000")
        }
    }
    \endqml
    \image declarative-rect_tint.png

    Tint is most useful when a subtle change is intended to be conveyed due to some event; you can then use tinting to more effectively tune the visible color.
*/
v8::Handle<v8::Value> tint(const v8::Arguments &args)
{
    if (args.Length() != 2)
        V8THROW_ERROR("Qt.tint(): Invalid arguments");

    // base color
    QVariant v1 = V8ENGINE()->toVariant(args[0], -1);
    if (v1.userType() == QVariant::String) {
        bool ok = false;
        v1 = QQmlStringConverters::colorFromString(v1.toString(), &ok);
        if (!ok) {
            return v8::Null();
        }
    } else if (v1.userType() != QVariant::Color) {
        return v8::Null();
    }

    // tint color
    QVariant v2 = V8ENGINE()->toVariant(args[1], -1);
    if (v2.userType() == QVariant::String) {
        bool ok = false;
        v2 = QQmlStringConverters::colorFromString(v2.toString(), &ok);
        if (!ok) {
            return v8::Null();
        }
    } else if (v2.userType() != QVariant::Color) {
        return v8::Null();
    }

    return V8ENGINE()->fromVariant(QQml_colorProvider()->tint(v1, v2));
}

/*!
\qmlmethod string Qt::formatDate(datetime date, variant format)

Returns a string representation of \c date, optionally formatted according
to \c format.

The \a date parameter may be a JavaScript \c Date object, a \l{date}{date}
property, a QDate, or QDateTime value. The \a format parameter may be any of
the possible format values as described for
\l{QtQml2::Qt::formatDateTime()}{Qt.formatDateTime()}.

If \a format is not specified, \a date is formatted using
\l {Qt::DefaultLocaleShortDate}{Qt.DefaultLocaleShortDate}.

\sa Locale
*/
v8::Handle<v8::Value> formatDate(const v8::Arguments &args)
{
    if (args.Length() < 1 || args.Length() > 2)
        V8THROW_ERROR("Qt.formatDate(): Invalid arguments");

    Qt::DateFormat enumFormat = Qt::DefaultLocaleShortDate;
    QDate date = V8ENGINE()->toVariant(args[0], -1).toDateTime().date();
    QString formattedDate;
    if (args.Length() == 2) {
        if (args[1]->IsString()) {
            QString format = V8ENGINE()->toVariant(args[1], -1).toString();
            formattedDate = date.toString(format);
        } else if (args[1]->IsNumber()) {
            quint32 intFormat = args[1]->ToNumber()->Value();
            Qt::DateFormat format = Qt::DateFormat(intFormat);
            formattedDate = date.toString(format);
        } else {
            V8THROW_ERROR("Qt.formatDate(): Invalid date format");
        }
    } else {
         formattedDate = date.toString(enumFormat);
    }

    return V8ENGINE()->fromVariant(QVariant::fromValue(formattedDate));
}

/*!
\qmlmethod string Qt::formatTime(datetime time, variant format)

Returns a string representation of \c time, optionally formatted according to
\c format.

The \a time parameter may be a JavaScript \c Date object, a QTime, or QDateTime
value. The \a format parameter may be any of the possible format values as
described for \l{QtQml2::Qt::formatDateTime()}{Qt.formatDateTime()}.

If \a format is not specified, \a time is formatted using
\l {Qt::DefaultLocaleShortDate}{Qt.DefaultLocaleShortDate}.

\sa Locale
*/
v8::Handle<v8::Value> formatTime(const v8::Arguments &args)
{
    if (args.Length() < 1 || args.Length() > 2)
        V8THROW_ERROR("Qt.formatTime(): Invalid arguments");

    QVariant argVariant = V8ENGINE()->toVariant(args[0], -1);
    QTime time;
    if (args[0]->IsDate() || (argVariant.type() == QVariant::String))
        time = argVariant.toDateTime().time();
    else // if (argVariant.type() == QVariant::Time), or invalid.
        time = argVariant.toTime();

    Qt::DateFormat enumFormat = Qt::DefaultLocaleShortDate;
    QString formattedTime;
    if (args.Length() == 2) {
        if (args[1]->IsString()) {
            QString format = V8ENGINE()->toVariant(args[1], -1).toString();
            formattedTime = time.toString(format);
        } else if (args[1]->IsNumber()) {
            quint32 intFormat = args[1]->ToNumber()->Value();
            Qt::DateFormat format = Qt::DateFormat(intFormat);
            formattedTime = time.toString(format);
        } else {
            V8THROW_ERROR("Qt.formatTime(): Invalid time format");
        }
    } else {
         formattedTime = time.toString(enumFormat);
    }

    return V8ENGINE()->fromVariant(QVariant::fromValue(formattedTime));
}

/*!
\qmlmethod string Qt::formatDateTime(datetime dateTime, variant format)

Returns a string representation of \c datetime, optionally formatted according to
\c format.

The \a date parameter may be a JavaScript \c Date object, a \l{date}{date}
property, a QDate, QTime, or QDateTime value.

If \a format is not provided, \a dateTime is formatted using
\l {Qt::DefaultLocaleShortDate}{Qt.DefaultLocaleShortDate}. Otherwise,
\a format should be either:

\list
\li One of the Qt::DateFormat enumeration values, such as
   \c Qt.DefaultLocaleShortDate or \c Qt.ISODate
\li A string that specifies the format of the returned string, as detailed below.
\endlist

If \a format specifies a format string, it should use the following expressions
to specify the date:

    \table
    \header \li Expression \li Output
    \row \li d \li the day as number without a leading zero (1 to 31)
    \row \li dd \li the day as number with a leading zero (01 to 31)
    \row \li ddd
            \li the abbreviated localized day name (e.g. 'Mon' to 'Sun').
            Uses QDate::shortDayName().
    \row \li dddd
            \li the long localized day name (e.g. 'Monday' to 'Qt::Sunday').
            Uses QDate::longDayName().
    \row \li M \li the month as number without a leading zero (1-12)
    \row \li MM \li the month as number with a leading zero (01-12)
    \row \li MMM
            \li the abbreviated localized month name (e.g. 'Jan' to 'Dec').
            Uses QDate::shortMonthName().
    \row \li MMMM
            \li the long localized month name (e.g. 'January' to 'December').
            Uses QDate::longMonthName().
    \row \li yy \li the year as two digit number (00-99)
    \row \li yyyy \li the year as four digit number
    \endtable

In addition the following expressions can be used to specify the time:

    \table
    \header \li Expression \li Output
    \row \li h
         \li the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
    \row \li hh
         \li the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)
    \row \li m \li the minute without a leading zero (0 to 59)
    \row \li mm \li the minute with a leading zero (00 to 59)
    \row \li s \li the second without a leading zero (0 to 59)
    \row \li ss \li the second with a leading zero (00 to 59)
    \row \li z \li the milliseconds without leading zeroes (0 to 999)
    \row \li zzz \li the milliseconds with leading zeroes (000 to 999)
    \row \li AP
            \li use AM/PM display. \e AP will be replaced by either "AM" or "PM".
    \row \li ap
            \li use am/pm display. \e ap will be replaced by either "am" or "pm".
    \endtable

    All other input characters will be ignored. Any sequence of characters that
    are enclosed in single quotes will be treated as text and not be used as an
    expression. Two consecutive single quotes ("''") are replaced by a single quote
    in the output.

For example, if the following date/time value was specified:

    \code
    // 21 May 2001 14:13:09
    var dateTime = new Date(2001, 5, 21, 14, 13, 09)
    \endcode

This \a dateTime value could be passed to \c Qt.formatDateTime(),
\l {QtQml2::Qt::formatDate()}{Qt.formatDate()} or \l {QtQml2::Qt::formatTime()}{Qt.formatTime()}
with the \a format values below to produce the following results:

    \table
    \header \li Format \li Result
    \row \li "dd.MM.yyyy"      \li 21.05.2001
    \row \li "ddd MMMM d yy"   \li Tue May 21 01
    \row \li "hh:mm:ss.zzz"    \li 14:13:09.042
    \row \li "h:m:s ap"        \li 2:13:9 pm
    \endtable

    \sa Locale
*/
v8::Handle<v8::Value> formatDateTime(const v8::Arguments &args)
{
    if (args.Length() < 1 || args.Length() > 2)
        V8THROW_ERROR("Qt.formatDateTime(): Invalid arguments");

    Qt::DateFormat enumFormat = Qt::DefaultLocaleShortDate;
    QDateTime dt = V8ENGINE()->toVariant(args[0], -1).toDateTime();
    QString formattedDt;
    if (args.Length() == 2) {
        if (args[1]->IsString()) {
            QString format = V8ENGINE()->toVariant(args[1], -1).toString();
            formattedDt = dt.toString(format);
        } else if (args[1]->IsNumber()) {
            quint32 intFormat = args[1]->ToNumber()->Value();
            Qt::DateFormat format = Qt::DateFormat(intFormat);
            formattedDt = dt.toString(format);
        } else {
            V8THROW_ERROR("Qt.formatDateTime(): Invalid datetime format");
        }
    } else {
         formattedDt = dt.toString(enumFormat);
    }

    return V8ENGINE()->fromVariant(QVariant::fromValue(formattedDt));
}

/*!
\qmlmethod bool Qt::openUrlExternally(url target)
Attempts to open the specified \c target url in an external application, based on the user's desktop preferences. Returns true if it succeeds, and false otherwise.
*/
v8::Handle<v8::Value> openUrlExternally(const v8::Arguments &args)
{
    if (args.Length() != 1)
        return V8ENGINE()->fromVariant(false);

    QUrl url(V8ENGINE()->toVariant(resolvedUrl(args), -1).toUrl());
    return V8ENGINE()->fromVariant(QQml_guiProvider()->openUrlExternally(url));
}

/*!
  \qmlmethod url Qt::resolvedUrl(url url)
  Returns \a url resolved relative to the URL of the caller.
*/
v8::Handle<v8::Value> resolvedUrl(const v8::Arguments &args)
{
    QUrl url = V8ENGINE()->toVariant(args[0], -1).toUrl();
    QQmlEngine *e = V8ENGINE()->engine();
    QQmlEnginePrivate *p = 0;
    if (e) p = QQmlEnginePrivate::get(e);
    if (p) {
        QQmlContextData *ctxt = V8ENGINE()->callingContext();
        if (ctxt)
            return V8ENGINE()->toString(ctxt->resolvedUrl(url).toString());
        else
            return V8ENGINE()->toString(url.toString());
    }

    return V8ENGINE()->toString(e->baseUrl().resolved(url).toString());
}

/*!
\qmlmethod list<string> Qt::fontFamilies()
Returns a list of the font families available to the application.
*/
v8::Handle<v8::Value> fontFamilies(const v8::Arguments &args)
{
    if (args.Length() != 0)
        V8THROW_ERROR("Qt.fontFamilies(): Invalid arguments");

    return V8ENGINE()->fromVariant(QVariant(QQml_guiProvider()->fontFamilies()));
}

/*!
\qmlmethod string Qt::md5(data)
Returns a hex string of the md5 hash of \c data.
*/
v8::Handle<v8::Value> md5(const v8::Arguments &args)
{
    if (args.Length() != 1)
        V8THROW_ERROR("Qt.md5(): Invalid arguments");

    QByteArray data = V8ENGINE()->toString(args[0]->ToString()).toUtf8();
    QByteArray result = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    return V8ENGINE()->toString(QLatin1String(result.toHex()));
}

/*!
\qmlmethod string Qt::btoa(data)
Binary to ASCII - this function returns a base64 encoding of \c data.
*/
v8::Handle<v8::Value> btoa(const v8::Arguments &args)
{
    if (args.Length() != 1)
        V8THROW_ERROR("Qt.btoa(): Invalid arguments");

    QByteArray data = V8ENGINE()->toString(args[0]->ToString()).toUtf8();

    return V8ENGINE()->toString(QLatin1String(data.toBase64()));
}

/*!
\qmlmethod string Qt::atob(data)
ASCII to binary - this function returns a base64 decoding of \c data.
*/
v8::Handle<v8::Value> atob(const v8::Arguments &args)
{
    if (args.Length() != 1)
        V8THROW_ERROR("Qt.atob(): Invalid arguments");

    QByteArray data = V8ENGINE()->toString(args[0]->ToString()).toUtf8();

    return V8ENGINE()->toString(QLatin1String(QByteArray::fromBase64(data)));
}

/*!
\qmlmethod Qt::quit()
This function causes the QQmlEngine::quit() signal to be emitted.
Within the \l {Prototyping with qmlscene}, this causes the launcher application to exit;
to quit a C++ application when this method is called, connect the
QQmlEngine::quit() signal to the QCoreApplication::quit() slot.
*/
v8::Handle<v8::Value> quit(const v8::Arguments &args)
{
    QQmlEnginePrivate::get(V8ENGINE()->engine())->sendQuit();
    return v8::Undefined();
}

/*!
\qmlmethod object Qt::createQmlObject(string qml, object parent, string filepath)

Returns a new object created from the given \a string of QML which will have the specified \a parent,
or \c null if there was an error in creating the object.

If \a filepath is specified, it will be used for error reporting for the created object.

Example (where \c parentItem is the id of an existing QML item):

\snippet qml/createQmlObject.qml 0

In the case of an error, a \l {Qt Script} Error object is thrown. This object has an additional property,
\c qmlErrors, which is an array of the errors encountered.
Each object in this array has the members \c lineNumber, \c columnNumber, \c fileName and \c message.
For example, if the above snippet had misspelled color as 'colro' then the array would contain an object like the following:
{ "lineNumber" : 1, "columnNumber" : 32, "fileName" : "dynamicSnippet1", "message" : "Cannot assign to non-existent property \"colro\""}.

Note that this function returns immediately, and therefore may not work if
the \a qml string loads new components (that is, external QML files that have not yet been loaded).
If this is the case, consider using \l{QtQml2::Qt::createComponent()}{Qt.createComponent()} instead.

See \l {Dynamic QML Object Creation from JavaScript} for more information on using this function.
*/
v8::Handle<v8::Value> createQmlObject(const v8::Arguments &args)
{
    if (args.Length() < 2 || args.Length() > 3)
        V8THROW_ERROR("Qt.createQmlObject(): Invalid arguments");

    struct Error {
        static v8::Local<v8::Value> create(QV8Engine *engine, const QList<QQmlError> &errors) {
            QString errorstr = QLatin1String("Qt.createQmlObject(): failed to create object: ");

            v8::Local<v8::Array> qmlerrors = v8::Array::New(errors.count());
            for (int ii = 0; ii < errors.count(); ++ii) {
                const QQmlError &error = errors.at(ii);
                errorstr += QLatin1String("\n    ") + error.toString();
                v8::Local<v8::Object> qmlerror = v8::Object::New();
                qmlerror->Set(v8::String::New("lineNumber"), v8::Integer::New(error.line()));
                qmlerror->Set(v8::String::New("columnNumber"), v8::Integer::New(error.column()));
                qmlerror->Set(v8::String::New("fileName"), engine->toString(error.url().toString()));
                qmlerror->Set(v8::String::New("message"), engine->toString(error.description()));
                qmlerrors->Set(ii, qmlerror);
            }

            v8::Local<v8::Value> error = v8::Exception::Error(engine->toString(errorstr));
            v8::Local<v8::Object> errorObject = error->ToObject();
            errorObject->Set(v8::String::New("qmlErrors"), qmlerrors);
            return error;
        }
    };

    QV8Engine *v8engine = V8ENGINE();
    QQmlEngine *engine = v8engine->engine();

    QQmlContextData *context = v8engine->callingContext();
    QQmlContext *effectiveContext = 0;
    if (context->isPragmaLibraryContext)
        effectiveContext = engine->rootContext();
    else
        effectiveContext = context->asQQmlContext();
    Q_ASSERT(context && effectiveContext);

    QString qml = v8engine->toString(args[0]->ToString());
    if (qml.isEmpty())
        return v8::Null();

    QUrl url;
    if (args.Length() > 2)
        url = QUrl(v8engine->toString(args[2]->ToString()));
    else
        url = QUrl(QLatin1String("inline"));

    if (url.isValid() && url.isRelative())
        url = context->resolvedUrl(url);

    QObject *parentArg = v8engine->toQObject(args[1]);
    if (!parentArg)
        V8THROW_ERROR("Qt.createQmlObject(): Missing parent object");

    QQmlComponent component(engine);
    component.setData(qml.toUtf8(), url);

    if (component.isError()) {
        v8::ThrowException(Error::create(v8engine, component.errors()));
        return v8::Undefined();
    }

    if (!component.isReady())
        V8THROW_ERROR("Qt.createQmlObject(): Component is not ready");

    QObject *obj = component.beginCreate(effectiveContext);
    if (obj) {
        QQmlData::get(obj, true)->explicitIndestructibleSet = false;
        QQmlData::get(obj)->indestructible = false;


        obj->setParent(parentArg);

        QList<QQmlPrivate::AutoParentFunction> functions = QQmlMetaType::parentFunctions();
        for (int ii = 0; ii < functions.count(); ++ii) {
            if (QQmlPrivate::Parented == functions.at(ii)(obj, parentArg))
                break;
        }
    }
    component.completeCreate();

    if (component.isError()) {
        v8::ThrowException(Error::create(v8engine, component.errors()));
        return v8::Undefined();
    }

    Q_ASSERT(obj);

    return v8engine->newQObject(obj);
}

/*!
\qmlmethod object Qt::createComponent(url, mode, parent)

Returns a \l Component object created using the QML file at the specified \a url,
or \c null if an empty string was given.

The returned component's \l Component::status property indicates whether the
component was successfully created. If the status is \c Component.Error,
see \l Component::errorString() for an error description.

If the optional \a mode parameter is set to \c Component.Asynchronous, the
component will be loaded in a background thread.  The Component::status property
will be \c Component.Loading while it is loading.  The status will change to
\c Component.Ready if the component loads successfully, or \c Component.Error
if loading fails.

If the optional \a parent parameter is given, it should refer to the object
that will become the parent for the created \l Component object.

Call \l {Component::createObject()}{Component.createObject()} on the returned
component to create an object instance of the component.

For example:

\snippet qml/createComponent-simple.qml 0

See \l {Dynamic QML Object Creation from JavaScript} for more information on using this function.

To create a QML object from an arbitrary string of QML (instead of a file),
use \l{QtQml2::Qt::createQmlObject()}{Qt.createQmlObject()}.
*/
v8::Handle<v8::Value> createComponent(const v8::Arguments &args)
{
    const char *invalidArgs = "Qt.createComponent(): Invalid arguments";
    const char *invalidParent = "Qt.createComponent(): Invalid parent object";
    if (args.Length() < 1 || args.Length() > 3)
        V8THROW_ERROR(invalidArgs);

    QV8Engine *v8engine = V8ENGINE();
    QQmlEngine *engine = v8engine->engine();

    QQmlContextData *context = v8engine->callingContext();
    QQmlContextData *effectiveContext = context;
    if (context->isPragmaLibraryContext)
        effectiveContext = 0;
    Q_ASSERT(context);

    QString arg = v8engine->toString(args[0]->ToString());
    if (arg.isEmpty())
        return v8::Null();

    QQmlComponent::CompilationMode compileMode = QQmlComponent::PreferSynchronous;
    QObject *parentArg = 0;

    int consumedCount = 1;
    if (args.Length() > 1) {
        const v8::Local<v8::Value> &lastArg = args[args.Length()-1];

        // The second argument could be the mode enum
        if (args[1]->IsInt32()) {
            int mode = args[1]->Int32Value();
            if (mode != int(QQmlComponent::PreferSynchronous) && mode != int(QQmlComponent::Asynchronous))
                V8THROW_ERROR(invalidArgs);
            compileMode = QQmlComponent::CompilationMode(mode);
            consumedCount += 1;
        } else {
            // The second argument could be the parent only if there are exactly two args
            if ((args.Length() != 2) || !(lastArg->IsObject() || lastArg->IsNull()))
                V8THROW_ERROR(invalidArgs);
        }

        if (consumedCount < args.Length()) {
            if (lastArg->IsObject()) {
                parentArg = v8engine->toQObject(lastArg);
                if (!parentArg)
                    V8THROW_ERROR(invalidParent);
            } else if (lastArg->IsNull()) {
                parentArg = 0;
            } else {
                V8THROW_ERROR(invalidParent);
            }
        }
    }

    QUrl url = context->resolvedUrl(QUrl(arg));
    QQmlComponent *c = new QQmlComponent(engine, url, compileMode, parentArg);
    QQmlComponentPrivate::get(c)->creationContext = effectiveContext;
    QQmlData::get(c, true)->explicitIndestructibleSet = false;
    QQmlData::get(c)->indestructible = false;

    return v8engine->newQObject(c);
}

#ifndef QT_NO_TRANSLATION
/*!
    \qmlmethod string Qt::qsTranslate(string context, string sourceText, string disambiguation, int n)

    Returns a translated version of \a sourceText within the given \a context, optionally based on a
    \a disambiguation string and value of \a n for strings containing plurals;
    otherwise returns \a sourceText itself if no appropriate translated string
    is available.

    If the same \a sourceText is used in different roles within the
    same translation \a context, an additional identifying string may be passed in
    for \a disambiguation.

    Example:
    \snippet qml/qsTranslate.qml 0

    \sa {Internationalization and Localization with Qt Quick}
*/
v8::Handle<v8::Value> qsTranslate(const v8::Arguments &args)
{
    if (args.Length() < 2)
        V8THROW_ERROR("qsTranslate() requires at least two arguments");
    if (!args[0]->IsString())
        V8THROW_ERROR("qsTranslate(): first argument (context) must be a string");
    if (!args[1]->IsString())
        V8THROW_ERROR("qsTranslate(): second argument (sourceText) must be a string");
    if ((args.Length() > 2) && !args[2]->IsString())
        V8THROW_ERROR("qsTranslate(): third argument (disambiguation) must be a string");

    QV8Engine *v8engine = V8ENGINE();
    QString context = v8engine->toString(args[0]);
    QString text = v8engine->toString(args[1]);
    QString comment;
    if (args.Length() > 2) comment = v8engine->toString(args[2]);

    int i = 3;
    if (args.Length() > i && args[i]->IsString()) {
        qWarning("qsTranslate(): specifying the encoding as fourth argument is deprecated");
        ++i;
    }

    int n = -1;
    if (args.Length() > i)
        n = args[i]->Int32Value();

    QString result = QCoreApplication::translate(context.toUtf8().constData(),
                                                 text.toUtf8().constData(),
                                                 comment.toUtf8().constData(),
                                                 n);

    return v8engine->toString(result);
}

/*!
    \qmlmethod string Qt::qsTranslateNoOp(string context, string sourceText, string disambiguation)

    Marks \a sourceText for dynamic translation in the given \a context; i.e, the stored \a sourceText
    will not be altered.

    If the same \a sourceText is used in different roles within the
    same translation context, an additional identifying string may be passed in
    for \a disambiguation.

    Returns the \a sourceText.

    QT_TRANSLATE_NOOP is used in conjunction with the dynamic translation functions
    qsTr() and qsTranslate(). It identifies a string as requiring
    translation (so it can be identified by \c lupdate), but leaves the actual
    translation to the dynamic functions.

    Example:
    \snippet qml/qtTranslateNoOp.qml 0

    \sa {Internationalization and Localization with Qt Quick}
*/
v8::Handle<v8::Value> qsTranslateNoOp(const v8::Arguments &args)
{
    if (args.Length() < 2)
        return v8::Undefined();
    return args[1];
}

/*!
    \qmlmethod string Qt::qsTr(string sourceText, string disambiguation, int n)

    Returns a translated version of \a sourceText, optionally based on a
    \a disambiguation string and value of \a n for strings containing plurals;
    otherwise returns \a sourceText itself if no appropriate translated string
    is available.

    If the same \a sourceText is used in different roles within the
    same translation context, an additional identifying string may be passed in
    for \a disambiguation.

    Example:
    \snippet qml/qsTr.qml 0

    \sa {Internationalization and Localization with Qt Quick}
*/
v8::Handle<v8::Value> qsTr(const v8::Arguments &args)
{
    if (args.Length() < 1)
        V8THROW_ERROR("qsTr() requires at least one argument");
    if (!args[0]->IsString())
        V8THROW_ERROR("qsTr(): first argument (sourceText) must be a string");
    if ((args.Length() > 1) && !args[1]->IsString())
        V8THROW_ERROR("qsTr(): second argument (disambiguation) must be a string");
    if ((args.Length() > 2) && !args[2]->IsNumber())
        V8THROW_ERROR("qsTr(): third argument (n) must be a number");

    QV8Engine *v8engine = V8ENGINE();
    QQmlContextData *ctxt = v8engine->callingContext();

    QString path = ctxt->url.toString();
    int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    QString context = (lastSlash > -1) ? path.mid(lastSlash + 1, path.length()-lastSlash-5) : QString();

    QString text = v8engine->toString(args[0]);
    QString comment;
    if (args.Length() > 1)
        comment = v8engine->toString(args[1]);
    int n = -1;
    if (args.Length() > 2)
        n = args[2]->Int32Value();

    QString result = QCoreApplication::translate(context.toUtf8().constData(), text.toUtf8().constData(),
                                                 comment.toUtf8().constData(), n);

    return v8engine->toString(result);
}

/*!
    \qmlmethod string Qt::qsTrNoOp(string sourceText, string disambiguation)

    Marks \a sourceText for dynamic translation; i.e, the stored \a sourceText
    will not be altered.

    If the same \a sourceText is used in different roles within the
    same translation context, an additional identifying string may be passed in
    for \a disambiguation.

    Returns the \a sourceText.

    QT_TR_NOOP is used in conjunction with the dynamic translation functions
    qsTr() and qsTranslate(). It identifies a string as requiring
    translation (so it can be identified by \c lupdate), but leaves the actual
    translation to the dynamic functions.

    Example:
    \snippet qml/qtTrNoOp.qml 0

    \sa {Internationalization and Localization with Qt Quick}
*/
v8::Handle<v8::Value> qsTrNoOp(const v8::Arguments &args)
{
    if (args.Length() < 1)
        return v8::Undefined();
    return args[0];
}

/*!
    \qmlmethod string Qt::qsTrId(string id, int n)

    Returns a translated string identified by \a id.
    If no matching string is found, the id itself is returned. This
    should not happen under normal conditions.

    If \a n >= 0, all occurrences of \c %n in the resulting string
    are replaced with a decimal representation of \a n. In addition,
    depending on \a n's value, the translation text may vary.

    Example:
    \snippet qml/qsTrId.qml 0

    It is possible to supply a source string template like:

    \tt{//% <string>}

    or

    \tt{\\begincomment% <string> \\endcomment}

    Example:
    \snippet qml/qsTrId.1.qml 0

    Creating binary translation (QM) files suitable for use with this function requires passing
    the \c -idbased option to the \c lrelease tool.

    \sa QT_TRID_NOOP, {Internationalization and Localization with Qt Quick}
*/
v8::Handle<v8::Value> qsTrId(const v8::Arguments &args)
{
    if (args.Length() < 1)
        V8THROW_ERROR("qsTrId() requires at least one argument");
    if (!args[0]->IsString())
        V8THROW_TYPE("qsTrId(): first argument (id) must be a string");
    if (args.Length() > 1 && !args[1]->IsNumber())
        V8THROW_TYPE("qsTrId(): second argument (n) must be a number");

    int n = -1;
    if (args.Length() > 1)
        n = args[1]->Int32Value();

    QV8Engine *v8engine = V8ENGINE();
    return v8engine->toString(qtTrId(v8engine->toString(args[0]).toUtf8().constData(), n));
}

/*!
    \qmlmethod string Qt::qsTrIdNoOp(string id)

    Marks \a id for dynamic translation.

    Returns the \a id.

    QT_TRID_NOOP is used in conjunction with the dynamic translation function
    qsTrId(). It identifies a string as requiring translation (so it can be identified
    by \c lupdate), but leaves the actual translation to qsTrId().

    Example:
    \snippet qml/qtTrIdNoOp.qml 0

    \sa qsTrId(), {Internationalization and Localization with Qt Quick}
*/
v8::Handle<v8::Value> qsTrIdNoOp(const v8::Arguments &args)
{
    if (args.Length() < 1)
        return v8::Undefined();
    return args[0];
}
#endif // QT_NO_TRANSLATION

/*!
    \qmlmethod Qt::locale(name)

    Returns a JS object representing the locale with the specified
    name, which has the format "language[_territory][.codeset][@modifier]"
    or "C", where:

    \list
    \li language is a lowercase, two-letter, ISO 639 language code,
    \li territory is an uppercase, two-letter, ISO 3166 country code,
    \li and codeset and modifier are ignored.
    \endlist

    If the string violates the locale format, or language is not a
    valid ISO 369 code, the "C" locale is used instead. If country
    is not present, or is not a valid ISO 3166 code, the most
    appropriate country is chosen for the specified language.

    \sa QtQuick2::Locale
*/
v8::Handle<v8::Value> locale(const v8::Arguments &args)
{
    QString code;
    if (args.Length() > 1)
        V8THROW_ERROR("locale() requires 0 or 1 argument");
    if (args.Length() == 1 && !args[0]->IsString())
        V8THROW_TYPE("locale(): argument (locale code) must be a string");

    QV8Engine *v8engine = V8ENGINE();
    if (args.Length() == 1)
        code = v8engine->toString(args[0]);

    return QQmlLocale::locale(v8engine, code);
}

/*!
    \qmlmethod Qt::binding(function)

    Returns a JS object representing a binding expression which may be
    assigned to any property in imperative code to cause a binding
    assignment.

    There are two main use-cases for the function: firstly, in imperative
    JavaScript code to cause a binding assignment:

    \snippet qml/qtBinding.1.qml 0

    and secondly, when defining initial property values of dynamically
    constructed objects (via Component.createObject() or
    Loader.setSource()) as being bound to the result of an expression.

    For example, assuming the existence of a DynamicText component:
    \snippet qml/DynamicText.qml 0

    the output from:
    \snippet qml/qtBinding.2.qml 0

    and from:
    \snippet qml/qtBinding.3.qml 0

    should both be:
    \code
    Root text extra text
    Modified root text extra text
    Dynamic text extra text
    Modified dynamic text extra text
    \endcode

    This function cannot be used in property binding declarations
    (see the documentation on \l{qml-javascript-assignment}{binding
    declarations and binding assignments}) except when the result is
    stored in an array bound to a var property.

    \snippet qml/qtBinding.4.qml 0

    \note In \l {Qt Quick 1}, all function assignment was treated as
    binding assignment, so the Qt.binding() function is new in
    \l {Qt Quick}{Qt Quick 2}.

    \since QtQuick 2.0
*/
v8::Handle<v8::Value> binding(const v8::Arguments &args)
{
    QString code;
    if (args.Length() != 1)
        V8THROW_ERROR("binding() requires 1 argument");
    if (!args[0]->IsFunction())
        V8THROW_TYPE("binding(): argument (binding expression) must be a function");

    v8::Handle<v8::Object> rv = args[0]->ToObject()->Clone();
    rv->SetHiddenValue(V8ENGINE()->bindingFlagKey(), v8::Boolean::New(true));
    return rv;
}

} // namespace QQmlBuiltinFunctions

QT_END_NAMESPACE
