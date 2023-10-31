// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlbuiltinfunctions_p.h"

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlfile.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlloggingcategory_p.h>
#include <private/qqmlstringconverters_p.h>
#if QT_CONFIG(qml_locale)
#include <private/qqmllocale_p.h>
#endif
#include <private/qqmldelayedcallqueue_p.h>
#include <QFileInfo>

#include <private/qqmldebugconnector_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>
#include <private/qqmlglobal_p.h>

#include <private/qqmlplatform_p.h>

#include <private/qv4engine_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4include_p.h>
#include <private/qv4context_p.h>
#include <private/qv4stringobject_p.h>
#include <private/qv4dateobject_p.h>
#include <private/qv4mm_p.h>
#include <private/qv4jsonobject_p.h>
#include <private/qv4objectproto_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4stackframe_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qcryptographichash.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>
#include <QtCore/qurl.h>
#include <QtCore/qfile.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcRootProperties, "qt.qml.rootObjectProperties");
Q_LOGGING_CATEGORY(lcQml, "qml");
Q_LOGGING_CATEGORY(lcJs, "js");

using namespace QV4;

#define THROW_TYPE_ERROR_WITH_MESSAGE(msg) \
    do { \
        return scope.engine->throwTypeError(QString::fromUtf8(msg)); \
    } while (false)

/*!
\qmltype Qt
\inqmlmodule QtQml
//! \instantiates QQmlEnginePrivate
\ingroup qml-utility-elements
\keyword QmlGlobalQtObject
\brief Provides a global object with useful enums and functions from Qt.

\c Qt is a singleton type that provides utility functions, properties, and
enums. Here is an example showing how to use this type:

\qml
import QtQuick 2.0

Text {
    color: Qt.rgba(1, 0, 0, 1)
    text: Qt.md5("hello, world")
}
\endqml


\section1 Enums

The Qt object contains the enums available in the \l [QtCore]{Qt}{Qt Namespace}. For example, you can access
the \l Qt::LeftButton and \l Qt::RightButton enumeration values as \c Qt.LeftButton and \c Qt.RightButton.


\section1 Types

The Qt object also contains helper functions for creating objects of specific
data types. This is primarily useful when setting the properties of an item
when the property has one of the following types:
\list
\li \c rect - use \l{Qt::rect()}{Qt.rect()}
\li \c point - use \l{Qt::point()}{Qt.point()}
\li \c size - use \l{Qt::size()}{Qt.size()}
\endlist

If the \c QtQuick module has been imported, the following helper functions for
creating objects of specific data types are also available for clients to use:
\list
\li \c color - use \l{Qt::rgba()}{Qt.rgba()}, \l{Qt::hsla()}{Qt.hsla()}, \l{Qt::darker()}{Qt.darker()}, \l{Qt::lighter()}{Qt.lighter()} or \l{Qt::tint()}{Qt.tint()}
\li \c font - use \l{Qt::font()}{Qt.font()}
\li \c vector2d - use \l{Qt::vector2d()}{Qt.vector2d()}
\li \c vector3d - use \l{Qt::vector3d()}{Qt.vector3d()}
\li \c vector4d - use \l{Qt::vector4d()}{Qt.vector4d()}
\li \c quaternion - use \l{Qt::quaternion()}{Qt.quaternion()}
\li \c matrix4x4 - use \l{Qt::matrix4x4()}{Qt.matrix4x4()}
\endlist

There are also string based constructors for these types. See \l{qtqml-typesystem-valuetypes.html}{QML Value Types} for more information.

\section1 Date/Time Formatters

The Qt object contains several functions for formatting QDateTime, QDate and QTime values.

\list
    \li \l{Qt::formatDateTime}{string Qt.formatDateTime(datetime date, variant format)}
    \li \l{Qt::formatDate}{string Qt.formatDate(datetime date, variant format)}
    \li \l{Qt::formatTime}{string Qt.formatTime(datetime date, variant format)}
\endlist

The format specification is described at \l{Qt::formatDateTime}{Qt.formatDateTime}.


\section1 Dynamic Object Creation
The following functions on the global object allow you to dynamically create QML
items from files or strings. See \l{Dynamic QML Object Creation from JavaScript} for an overview
of their use.

\list
    \li \l{Qt::createComponent()}{object Qt.createComponent(url)}
    \li \l{Qt::createQmlObject()}{object Qt.createQmlObject(string qml, object parent, string filepath)}
\endlist


\section1 Other Functions

The following functions are also on the Qt object.

\list
    \li \l{Qt::quit()}{Qt.quit()}
    \li \l{Qt::md5()}{Qt.md5(string)}
    \li \l{Qt::btoa()}{string Qt.btoa(string)}
    \li \l{Qt::atob()}{string Qt.atob(string)}
    \li \l{Qt::binding()}{object Qt.binding(function)}
    \li \l{Qt::locale()}{object Qt.locale()}
    \li \l{Qt::resolvedUrl()}{string Qt.resolvedUrl(string)}
    \li \l{Qt::openUrlExternally()}{Qt.openUrlExternally(string)}
    \li \l{Qt::fontFamilies()}{list<string> Qt.fontFamilies()}
\endlist
*/

/*!
    \qmlproperty object Qt::platform
    \since 5.1

    The \c platform object provides info about the underlying platform.

    Its properties are:

    \table
    \row
    \li \c platform.os
    \li

    This read-only property contains the name of the operating system.

    Possible values are:

    \list
        \li \c "android" - Android
        \li \c "ios" - iOS
        \li \c "tvos" - tvOS
        \li \c "linux" - Linux
        \li \c "osx" - \macos
        \li \c "qnx" - QNX (since Qt 5.9.3)
        \li \c "unix" - Other Unix-based OS
        \li \c "windows" - Windows
        \li \c "wasm" - WebAssembly
    \endlist

    \row
    \li \c platform.pluginName
    \li This is the name of the platform set on the QGuiApplication instance
        as returned by \l QGuiApplication::platformName()

    \endtable
*/

/*!
    \qmlproperty object Qt::application
    \since 5.1

    The \c application object provides access to global application state
    properties shared by many QML components.

    It is the same as the \l Application singleton.

    The following example uses the \c application object to indicate
    whether the application is currently active:

    \snippet qml/application.qml document

    \note When using QML without a QGuiApplication, the following properties will be undefined:
    \list
    \li application.active
    \li application.state
    \li application.layoutDirection
    \li application.font
    \endlist
*/

/*!
    \qmlproperty object Qt::inputMethod
    \since 5.0

    It is the same as the \l InputMethod singleton.

    The \c inputMethod object allows access to application's QInputMethod object
    and all its properties and slots. See the QInputMethod documentation for
    further details.
*/

/*!
    \qmlproperty object Qt::styleHints
    \since 5.5

    The \c styleHints object provides platform-specific style hints and settings.
    See the \l QStyleHints documentation for further details.

    You should access StyleHints via \l Application::styleHints instead.

    \note The \c styleHints object is only available when using the Qt Quick module.
*/

/*!
\qmlmethod object Qt::include(string url, jsobject callback)
\deprecated

This method should not be used. Use ECMAScript modules, and the native
JavaScript \c import and \c export statements instead.

Includes another JavaScript file. This method can only be used from within JavaScript files,
and not regular QML files.

This imports all functions from \a url into the current script's namespace.

Qt.include() returns an object that describes the status of the operation.  The object has
a single property, \c {status}, that is set to one of the following values:

\table
\header \li Symbol \li Value \li Description
\row \li result.OK \li 0 \li The include completed successfully.
\row \li result.LOADING \li 1 \li Data is being loaded from the network.
\row \li result.NETWORK_ERROR \li 2 \li A network error occurred while fetching the url.
\row \li result.EXCEPTION \li 3 \li A JavaScript exception occurred while executing the included code.
An additional \c exception property will be set in this case.
\endtable

The \c status property will be updated as the operation progresses.

If provided, \a callback is invoked when the operation completes.  The callback is passed
the same object as is returned from the Qt.include() call.
*/
// Qt.include() is implemented in qv4include.cpp

QtObject::QtObject(ExecutionEngine *engine)
    : m_engine(engine)
{
}

QtObject::Contexts QtObject::getContexts() const
{
    QQmlEngine *engine = qmlEngine();
    if (!engine)
        return {};

    QQmlRefPointer<QQmlContextData> context = v4Engine()->callingQmlContext();
    if (!context)
        context = QQmlContextData::get(QQmlEnginePrivate::get(engine)->rootContext);

    Q_ASSERT(context);
    QQmlRefPointer<QQmlContextData> effectiveContext
            = context->isPragmaLibraryContext() ? nullptr : context;
    return {context, effectiveContext};
}

QtObject *QtObject::create(QQmlEngine *, QJSEngine *jsEngine)
{
    QV4::ExecutionEngine *v4 = jsEngine->handle();
    QV4::Scope scope(v4);
    ScopedObject globalObject(scope, v4->globalObject);
    ScopedString qtName(scope, v4->newString(QStringLiteral("Qt")));
    QV4::ScopedValue result(scope, globalObject->get(qtName->toPropertyKey()));
    return qobject_cast<QtObject *>(result->as<QV4::QObjectWrapper>()->object());
}

QJSValue QtObject::include(const QString &url, const QJSValue &callback) const
{
    return QV4Include::method_include(v4Engine(), v4Engine()->resolvedUrl(url), callback);
}


/*!
    \qmlmethod bool Qt::isQtObject(object)

    Returns \c true if \a object is a valid reference to a Qt or QML object,
    \c false otherwise.
*/
bool QtObject::isQtObject(const QJSValue &value) const
{
    return qjsvalue_cast<QObject *>(value) != nullptr;
}

/*!
    \qmlmethod color Qt::color(string name)

    Returns the color corresponding to the given \a name (i.e. red or #ff0000).
    If there is no such color, \c null is returned.
*/
QVariant QtObject::color(const QString &name) const
{
    bool ok = false;
    const QVariant v = QQmlStringConverters::colorFromString(name, &ok);
    if (ok)
        return v;

    v4Engine()->throwError(QStringLiteral("\"%1\" is not a valid color name").arg(name));
    return QVariant::fromValue(nullptr);
}

/*!
    \qmlmethod color Qt::rgba(real red, real green, real blue, real alpha)

    Returns a color with the specified \a red, \a green, \a blue, and \a alpha
    components. All components should be in the range 0-1 (inclusive).
*/
QVariant QtObject::rgba(double r, double g, double b, double a) const
{
    if (r < 0.0) r=0.0;
    if (r > 1.0) r=1.0;
    if (g < 0.0) g=0.0;
    if (g > 1.0) g=1.0;
    if (b < 0.0) b=0.0;
    if (b > 1.0) b=1.0;
    if (a < 0.0) a=0.0;
    if (a > 1.0) a=1.0;

    return QQml_colorProvider()->fromRgbF(r, g, b, a);
}

/*!
    \qmlmethod color Qt::hsla(real hue, real saturation, real lightness, real alpha)

    Returns a color with the specified \a hue, \a saturation, \a lightness, and \a alpha
    components. All components should be in the range 0-1 (inclusive).
*/
QVariant QtObject::hsla(double h, double s, double l, double a) const
{
    if (h < 0.0) h=0.0;
    if (h > 1.0) h=1.0;
    if (s < 0.0) s=0.0;
    if (s > 1.0) s=1.0;
    if (l < 0.0) l=0.0;
    if (l > 1.0) l=1.0;
    if (a < 0.0) a=0.0;
    if (a > 1.0) a=1.0;

    return QQml_colorProvider()->fromHslF(h, s, l, a);
}

/*!
    \since 5.5
    \qmlmethod color Qt::hsva(real hue, real saturation, real value, real alpha)

    Returns a color with the specified \a hue, \a saturation, \a value and \a alpha
    components. All components should be in the range 0-1 (inclusive).

*/
QVariant QtObject::hsva(double h, double s, double v, double a) const
{
    h = qBound(0.0, h, 1.0);
    s = qBound(0.0, s, 1.0);
    v = qBound(0.0, v, 1.0);
    a = qBound(0.0, a, 1.0);

    return QQml_colorProvider()->fromHsvF(h, s, v, a);
}

/*!
    \qmlmethod color Qt::colorEqual(color lhs, string rhs)

    Returns \c true if both \a lhs and \a rhs yield equal color values. Both
    arguments may be either color values or string values. If a string value
    is supplied it must be convertible to a color, as described for the
    \l{colorvaluetypedocs}{color} value type.
*/
bool QtObject::colorEqual(const QVariant &lhs, const QVariant &rhs) const
{
    bool ok = false;

    QVariant color1 = lhs;
    if (color1.userType() == QMetaType::QString) {
        color1 = QQmlStringConverters::colorFromString(color1.toString(), &ok);
        if (!ok) {
            v4Engine()->throwError(QStringLiteral("Qt.colorEqual(): Invalid color name"));
            return false;
        }
    } else if (color1.userType() != QMetaType::QColor) {
        v4Engine()->throwError(QStringLiteral("Qt.colorEqual(): Invalid arguments"));
        return false;
    }

    QVariant color2 = rhs;
    if (color2.userType() == QMetaType::QString) {
        color2 = QQmlStringConverters::colorFromString(color2.toString(), &ok);
        if (!ok) {
            v4Engine()->throwError(QStringLiteral("Qt.colorEqual(): Invalid color name"));
            return false;
        }
    } else if (color2.userType() != QMetaType::QColor) {
        v4Engine()->throwError(QStringLiteral("Qt.colorEqual(): Invalid arguments"));
        return false;
    }

    return color1 == color2;
}

/*!
    \qmlmethod rect Qt::rect(real x, real y, real width, real height)

    Returns a rect with the top-left corner at \a x, \a y and the specified \a width and \a height.
*/
QRectF QtObject::rect(double x, double y, double width, double height) const
{
    return QRectF(x, y, width, height);
}

/*!
    \qmlmethod point Qt::point(real x, real y)

    Returns a point with the specified \a x and \a y coordinates.
*/
QPointF QtObject::point(double x, double y) const
{
    return QPointF(x, y);
}

/*!
    \qmlmethod size Qt::size(real width, real height)

    Returns a size with the specified \a width and \a height.
*/
QSizeF QtObject::size(double w, double h) const
{
    return QSizeF(w, h);
}

/*!
    \qmlmethod font Qt::font(object fontSpecifier)

    Returns a font with the properties specified in the \a fontSpecifier object
    or the nearest matching font.  The \a fontSpecifier object should contain
    key-value pairs where valid keys are the \l{fontvaluetypedocs}{font} type's
    subproperty names, and the values are valid values for each subproperty.
    Invalid keys will be ignored.
*/
QVariant QtObject::font(const QJSValue &fontSpecifier) const
{
    if (!fontSpecifier.isObject()) {
        v4Engine()->throwError(QStringLiteral("Qt.font(): Invalid arguments"));
        return QVariant();
    }

    {
        const QVariant v = QQmlValueTypeProvider::createValueType(
                    fontSpecifier, QMetaType(QMetaType::QFont));
        if (v.isValid())
            return v;
    }

    v4Engine()->throwError(QStringLiteral("Qt.font(): Invalid argument: "
                                          "no valid font subproperties specified"));
    return QVariant();
}

template<typename T>
void addParameters(QJSEngine *e, QJSValue &result, int i, T parameter)
{
    result.setProperty(i, e->toScriptValue(parameter));
}

template<>
void addParameters<double>(QJSEngine *, QJSValue &result, int i, double parameter)
{
    result.setProperty(i, QJSValue(parameter));
}

template<typename T, typename ...Others>
void addParameters(QJSEngine *e, QJSValue &result, int i, T parameter, Others... others)
{
    addParameters<T>(e, result, i, parameter);
    addParameters<Others...>(e, result, ++i, others...);
}

template<typename ...T>
static QVariant constructFromJSValue(QJSEngine *e, QMetaType type, T... parameters)
{
    if (!e)
        return QVariant();
    QJSValue params = e->newArray(sizeof...(parameters));
    addParameters(e, params, 0, parameters...);
    const QVariant variant = QQmlValueTypeProvider::createValueType(params, type);
    return variant.isValid() ? variant : QVariant(type);
}

/*!
    \qmlmethod vector2d Qt::vector2d(real x, real y)

    Returns a vector2d with the specified \a x and \a y values.
*/
QVariant QtObject::vector2d(double x, double y) const
{
    return constructFromJSValue(jsEngine(), QMetaType(QMetaType::QVector2D), x, y);
}

/*!
    \qmlmethod vector3d Qt::vector3d(real x, real y, real z)

    Returns a vector3d with the specified \a x, \a y, and \a z values.
*/
QVariant QtObject::vector3d(double x, double y, double z) const
{
    return constructFromJSValue(jsEngine(), QMetaType(QMetaType::QVector3D), x, y, z);
}

/*!
    \qmlmethod vector4d Qt::vector4d(real x, real y, real z, real w)

    Returns a vector4d with the specified \a x, \a y, \a z, and \a w values.
*/
QVariant QtObject::vector4d(double x, double y, double z, double w) const
{
    return constructFromJSValue(jsEngine(), QMetaType(QMetaType::QVector4D), x, y, z, w);
}

/*!
    \qmlmethod quaternion Qt::quaternion(real scalar, real x, real y, real z)

    Returns a quaternion with the specified \a scalar, \a x, \a y, and \a z values.
*/
QVariant QtObject::quaternion(double scalar, double x, double y, double z) const
{
    return constructFromJSValue(jsEngine(), QMetaType(QMetaType::QQuaternion), scalar, x, y, z);
}

/*!
    \qmlmethod matrix4x4 Qt::matrix4x4()

    Returns an identity matrix4x4.
 */
QVariant QtObject::matrix4x4() const
{
    const QMetaType metaType(QMetaType::QMatrix4x4);
    const QVariant variant = QQmlValueTypeProvider::createValueType(QJSValue(), metaType);
    return variant.isValid() ? variant : QVariant(metaType);
}

/*!
    \qmlmethod matrix4x4 Qt::matrix4x4(var values)

    Returns a matrix4x4 with the specified \a values. \a values is expected to
    be a JavaScript array with 16 entries.

    The array indices correspond to positions in the matrix as follows:

    \table
    \row \li 0  \li 1  \li 2  \li 3
    \row \li 4  \li 5  \li 6  \li 7
    \row \li 8  \li 9  \li 10 \li 11
    \row \li 12 \li 13 \li 14 \li 15
    \endtable
*/
QVariant QtObject::matrix4x4(const QJSValue &value) const
{
    if (value.isObject()) {
        QVariant v = QQmlValueTypeProvider::createValueType(
                    value, QMetaType(QMetaType::QMatrix4x4));
        if (v.isValid())
            return v;
    }

    v4Engine()->throwError(QStringLiteral("Qt.matrix4x4(): Invalid argument: "
                                          "not a valid matrix4x4 values array"));
    return QVariant();
}

/*!
    \qmlmethod matrix4x4 Qt::matrix4x4(real m11, real m12, real m13, real m14, real m21, real m22, real m23, real m24, real m31, real m32, real m33, real m34, real m41, real m42, real m43, real m44)

    Returns a matrix4x4 with the specified values.

    The arguments correspond to their positions in the matrix:

    \table
    \row \li \a m11 \li \a m12 \li \a m13 \li \a m14
    \row \li \a m21 \li \a m22 \li \a m23 \li \a m24
    \row \li \a m31 \li \a m32 \li \a m33 \li \a m34
    \row \li \a m41 \li \a m42 \li \a m43 \li \a m44
    \endtable
*/
QVariant QtObject::matrix4x4(double m11, double m12, double m13, double m14,
                             double m21, double m22, double m23, double m24,
                             double m31, double m32, double m33, double m34,
                             double m41, double m42, double m43, double m44) const
{
    return constructFromJSValue(jsEngine(), QMetaType(QMetaType::QMatrix4x4),
                           m11, m12, m13, m14, m21, m22, m23, m24,
                           m31, m32, m33, m34, m41, m42, m43, m44);
}

static QVariant colorVariantFromJSValue(const QJSValue &color, bool *ok)
{
    QVariant v;
    if (color.isString()) {
        v = QQmlStringConverters::colorFromString(color.toString(), ok);
        if (!(*ok))
            return QVariant::fromValue(nullptr);
    } else {
        v = color.toVariant();
        if (v.userType() != QMetaType::QColor) {
            *ok = false;
            return QVariant::fromValue(nullptr);
        }
    }

    *ok = true;
    return v;
}

/*!
    \qmlmethod color Qt::lighter(color baseColor, real factor)

    Returns a color lighter than \a baseColor by the \a factor provided.

    If the factor is greater than 1.0, this functions returns a lighter color.
    Setting factor to 1.5 returns a color that is 50% brighter. If the factor is less than 1.0,
    the return color is darker, but we recommend using the Qt.darker() function for this purpose.
    If the factor is 0 or negative, the return value is unspecified.

    The function converts the current RGB color to HSV, multiplies the value (V) component
    by factor and converts the color back to RGB.

    If \a factor is not supplied, returns a color that is 50% lighter than \a baseColor (factor 1.5).
*/
QVariant QtObject::lighter(const QJSValue &color, double factor) const
{
    bool ok;
    const QVariant v = colorVariantFromJSValue(color, &ok);
    return ok ? QQml_colorProvider()->lighter(v, factor) : v;
}

/*!
    \qmlmethod color Qt::darker(color baseColor, real factor)

    Returns a color darker than \a baseColor by the \a factor provided.

    If the factor is greater than 1.0, this function returns a darker color.
    Setting factor to 3.0 returns a color that has one-third the brightness.
    If the factor is less than 1.0, the return color is lighter, but we recommend using
    the Qt.lighter() function for this purpose. If the factor is 0 or negative, the return
    value is unspecified.

    The function converts the current RGB color to HSV, divides the value (V) component
    by factor and converts the color back to RGB.

    If \a factor is not supplied, returns a color that is 50% darker than \a baseColor (factor 2.0).
*/
QVariant QtObject::darker(const QJSValue &color, double factor) const
{
    bool ok;
    const QVariant v = colorVariantFromJSValue(color, &ok);
    return ok ? QQml_colorProvider()->darker(v, factor) : v;
}

/*!
    \qmlmethod color Qt::alpha(color baseColor, real value)

    Returns \a baseColor with an alpha value of \a value.

    \a value is a real ranging from 0 (completely transparent) to 1 (completely opaque).
*/
QVariant QtObject::alpha(const QJSValue &baseColor, double value) const
{
    bool ok;
    const QVariant v = colorVariantFromJSValue(baseColor, &ok);
    return ok ? QQml_colorProvider()->alpha(v, value) : v;
}

/*!
    \qmlmethod color Qt::tint(color baseColor, color tintColor)

    This function allows tinting one color (\a baseColor) with another (\a tintColor).

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

    Tint is most useful when a subtle change is intended to be conveyed due to some event;
    you can then use tinting to more effectively tune the visible color.
*/
QVariant QtObject::tint(const QJSValue &baseColor, const QJSValue &tintColor) const
{
    bool ok;

    // base color
    const QVariant v1 = colorVariantFromJSValue(baseColor, &ok);
    if (!ok)
        return v1;

    // tint color
    const QVariant v2 = colorVariantFromJSValue(tintColor, &ok);

    return ok ? QQml_colorProvider()->tint(v1, v2) : v2;
}

namespace {
template <typename T>
QString formatDateTimeObjectUsingDateFormat(T formatThis, Qt::DateFormat format) {
    switch (format) {
    case Qt::TextDate:
    case Qt::ISODate:
    case Qt::RFC2822Date:
    case Qt::ISODateWithMs:
        return formatThis.toString(format);
    default: // ### Qt 6: remove once qtbase has removed the rest of the enum !
        break;
    }
    // Q_UNREACHABLE(); // ### Qt 6: restore once the default is gone
    return QString();
}
}

static QTime dateTimeToTime(const QDateTime &dateTime)
{
    return dateTime.toLocalTime().time();
}

/*!
\qmlmethod string Qt::formatDate(datetime date, variant format, variant localeFormatOption)

Returns a string representation of \a date, optionally formatted using \a format.

The \a date parameter may be a JavaScript \c Date object, a \l{date}{date}
property, a QDate, or QDateTime value. The \a format and \a localeFormatOption
parameter may be any of the possible format values as described for
\l{QtQml::Qt::formatDateTime()}{Qt.formatDateTime()}.

If \a format is not specified, \a date is formatted using
\l {QLocale::FormatType}{Locale.ShortFormat} using the
default locale.

\sa Locale
*/
static std::optional<QDate> dateFromString(const QString &string, QV4::ExecutionEngine *engine)
{
    {
        const QDate date = QDate::fromString(string, Qt::ISODate);
        if (date.isValid())
            return date;
    }

    {
        // For historical reasons, the string argument is parsed as datetime, not as only date
        const QDateTime dateTime = QDateTime::fromString(string, Qt::ISODate);
        if (dateTime.isValid()) {
            qCWarning(lcRootProperties())
                    << string << "is a date/time string being passed to formatDate()."
                    << "You should only pass date strings to formatDate().";
            return dateTime.date();
        }
    }

    {
        // Since we can coerce QDate to QString, allow the resulting string format here.
        const QDateTime dateTime = DateObject::stringToDateTime(string, engine);
        if (dateTime.isValid())
            return DateObject::dateTimeToDate(dateTime);
    }

    engine->throwError(QStringLiteral("Invalid argument passed to formatDate(): %1").arg(string));
    return std::nullopt;
}

QString QtObject::formatDate(const QDate &date, const QString &format) const
{
    return date.toString(format);
}

QString QtObject::formatDate(const QDate &date, Qt::DateFormat format) const
{
    return formatDateTimeObjectUsingDateFormat(date, format);
}

QString QtObject::formatDate(const QDateTime &dateTime, const QString &format) const
{
    return DateObject::dateTimeToDate(dateTime).toString(format);
}

QString QtObject::formatDate(const QString &string, const QString &format) const
{
    if (const auto qDate = dateFromString(string, v4Engine()))
        return formatDate(qDate.value(), format);

    return QString();
}

QString QtObject::formatDate(const QDateTime &dateTime, Qt::DateFormat format) const
{
    return formatDateTimeObjectUsingDateFormat(DateObject::dateTimeToDate(dateTime), format);
}

QString QtObject::formatDate(const QString &string, Qt::DateFormat format) const
{
    if (const auto qDate = dateFromString(string, v4Engine()))
        return formatDate(qDate.value(), format);

    return QString();
}

#if QT_CONFIG(qml_locale)
QString QtObject::formatDate(const QDate &date, const QLocale &locale,
                             QLocale::FormatType formatType) const
{
    return locale.toString(date, formatType);
}

QString QtObject::formatDate(const QDateTime &dateTime, const QLocale &locale,
                             QLocale::FormatType formatType) const
{
    return locale.toString(DateObject::dateTimeToDate(dateTime), formatType);
}

QString QtObject::formatDate(const QString &string, const QLocale &locale,
                             QLocale::FormatType formatType) const
{
    if (const auto qDate = dateFromString(string, v4Engine()))
        return locale.toString(qDate.value(), formatType);

    return QString();
}
#endif

/*!
\qmlmethod string Qt::formatTime(datetime time, variant format, variant localeFormatOption)

Returns a string representation of \a time, optionally formatted using
\a format, and, if provided, \a localeFormatOption.

The \a time parameter may be a JavaScript \c Date object, a QTime, or QDateTime
value. The \a format and \a localeFormatOption parameter may be any of the
possible format values as described for
\l{QtQml::Qt::formatDateTime()}{Qt.formatDateTime()}.

If \a format is not specified, \a time is formatted using
\l {QLocale::FormatType}{Locale.ShortFormat} using the default locale.

\sa Locale
*/
static std::optional<QTime> timeFromString(const QString &string, QV4::ExecutionEngine *engine)
{
    {
        const QTime time = QTime::fromString(string, Qt::ISODate);
        if (time.isValid())
            return time;
    }

    {
        // For historical reasons, the string argument is parsed as datetime, not as only time
        const QDateTime dateTime = QDateTime::fromString(string, Qt::ISODate);
        if (dateTime.isValid()) {
            qCWarning(lcRootProperties())
                    << string << "is a date/time string being passed to formatTime()."
                    << "You should only pass time strings to formatTime().";
            return dateTime.time();
        }
    }

    {
        // Since we can coerce QTime to QString, allow the resulting string format here.
        const QDateTime dateTime = DateObject::stringToDateTime(string, engine);
        if (dateTime.isValid())
            return dateTimeToTime(dateTime);
    }

    engine->throwError(QStringLiteral("Invalid argument passed to formatTime(): %1").arg(string));
    return std::nullopt;
}

QString QtObject::formatTime(const QTime &time, const QString &format) const
{
    return time.toString(format);
}

QString QtObject::formatTime(const QDateTime &dateTime, const QString &format) const
{
    return dateTimeToTime(dateTime).toString(format);
}

QString QtObject::formatTime(const QString &time, const QString &format) const
{

    if (auto qTime = timeFromString(time, v4Engine()))
        return formatTime(qTime.value(), format);

    return QString();
}

QString QtObject::formatTime(const QTime &time, Qt::DateFormat format) const
{
    return formatDateTimeObjectUsingDateFormat(time, format);
}

QString QtObject::formatTime(const QDateTime &dateTime, Qt::DateFormat format) const
{
    return formatDateTimeObjectUsingDateFormat(dateTimeToTime(dateTime), format);
}

QString QtObject::formatTime(const QString &time, Qt::DateFormat format) const
{
    if (auto qTime = timeFromString(time, v4Engine()))
        return formatTime(qTime.value(), format);

    return QString();
}

#if QT_CONFIG(qml_locale)
QString QtObject::formatTime(const QTime &time, const QLocale &locale,
                             QLocale::FormatType formatType) const
{
    return locale.toString(time, formatType);
}

QString QtObject::formatTime(const QDateTime &dateTime, const QLocale &locale,
                             QLocale::FormatType formatType) const
{
    return locale.toString(dateTimeToTime(dateTime), formatType);
}

QString QtObject::formatTime(const QString &time, const QLocale &locale,
                             QLocale::FormatType formatType) const
{
    if (auto qTime = timeFromString(time, v4Engine()))
        return locale.toString(qTime.value(), formatType);

    return QString();
}
#endif

/*!
\qmlmethod string Qt::formatDateTime(datetime dateTime, variant format, variant localeFormatOption)

Returns a string representation of \a dateTime, optionally formatted using
\a format and \a localeFormatOption.

The \a dateTime parameter may be a JavaScript \c Date object, a \l{date}{date}
property, a QDate, QTime, or QDateTime value.

If \a format is not provided, \a dateTime is formatted using
\l {QLocale::FormatType}{Locale.ShortFormat} using the
default locale. Otherwise, \a format should be either:

\list
\li One of the Qt::DateFormat enumeration values, such as
   \c Qt.RFC2822Date or \c Qt.ISODate.
\li A string that specifies the format of the returned string, as detailed below.
\li A \c locale object.
\endlist

If \a format specifies a locale object, \dateTime is formatted
with \l{QLocale::toString}. In this case, \a localeFormatOption can hold a value
of type \l {QLocale::FormatType} to further tune the formatting. If none is
provided, \l {QLocale::FormatType}{Locale.ShortFormat} is used.

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
    \row \li t
            \li include a time-zone indicator.
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
\l {QtQml::Qt::formatDate()}{Qt.formatDate()} or \l {QtQml::Qt::formatTime()}{Qt.formatTime()}
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
static std::optional<QDateTime> dateTimeFromString(const QString &string, QV4::ExecutionEngine *engine)
{
    {
        const QDateTime dateTime = QDateTime::fromString(string, Qt::ISODate);
        if (dateTime.isValid())
            return dateTime;
    }

    {
        // Since we can coerce QDateTime to QString, allow the resulting string format here.
        const QDateTime dateTime = DateObject::stringToDateTime(string, engine);
        if (dateTime.isValid())
            return dateTime;
    }

    engine->throwError(QStringLiteral("Invalid argument passed to formatDateTime(): %1").arg(string));
    return std::nullopt;
}

QString QtObject::formatDateTime(const QDateTime &dateTime, const QString &format) const
{
    return dateTime.toString(format);
}

QString QtObject::formatDateTime(const QString &string, const QString &format) const
{

    if (const auto qDateTime = dateTimeFromString(string, v4Engine()))
        return formatDateTime(qDateTime.value(), format);

    return QString();
}

QString QtObject::formatDateTime(const QDateTime &dateTime, Qt::DateFormat format) const
{
    return formatDateTimeObjectUsingDateFormat(dateTime, format);
}

QString QtObject::formatDateTime(const QString &string, Qt::DateFormat format) const
{

    if (const auto qDateTime = dateTimeFromString(string, v4Engine()))
        return formatDateTime(qDateTime.value(), format);

    return QString();
}

#if QT_CONFIG(qml_locale)
QString QtObject::formatDateTime(const QDateTime &dateTime, const QLocale &locale,
                                 QLocale::FormatType formatType) const
{
    return locale.toString(dateTime, formatType);
}

QString QtObject::formatDateTime(const QString &string, const QLocale &locale,
                                 QLocale::FormatType formatType) const
{

    if (const auto qDateTime = dateTimeFromString(string, v4Engine()))
        return formatDateTime(qDateTime.value(), locale, formatType);

    return QString();
}
#endif

/*!
    \qmlmethod bool Qt::openUrlExternally(url target)

    Attempts to open the specified \a target url in an external application, based on the user's
    desktop preferences. Returns \c true if it succeeds, \c false otherwise.

    \warning A return value of \c true indicates that the application has successfully requested
    the operating system to open the URL in an external application. The external application may
    still fail to launch or fail to open the requested URL. This result will not be reported back
    to the application.
*/
bool QtObject::openUrlExternally(const QUrl &url) const
{
    return QQml_guiProvider()->openUrlExternally(resolvedUrl(url));
}

/*!
  \qmlmethod url Qt::url(url url)

  Returns \a url verbatim. This can be used to force a type coercion to \c url.
  In contrast to Qt.resolvedUrl() this retains any relative URLs. As strings
  are implicitly converted to urls, the function can be called with a string
  as argument, and will then return a url.

  \sa resolvedUrl()
*/
QUrl QtObject::url(const QUrl &url) const
{
    return url;
}

/*!
  \qmlmethod url Qt::resolvedUrl(url url)

  Returns \a url resolved relative to the URL of the caller.

  If there is no caller or the caller is not associated with a QML context,
  returns \a url resolved relative to the QML engine's base URL. If the QML
  engine has no base URL, just returns \a url.

  \sa url()
*/
QUrl QtObject::resolvedUrl(const QUrl &url) const
{
    if (QQmlRefPointer<QQmlContextData> ctxt = v4Engine()->callingQmlContext())
        return ctxt->resolvedUrl(url);
    if (QQmlEngine *engine = qmlEngine())
        return engine->baseUrl().resolved(url);
    return url;
}

/*!
  \qmlmethod url Qt::resolvedUrl(url url, object context)

  Returns \a url resolved relative to the URL of the QML context of
  \a context. If \a context is not associated with a QML context,
  returns \a url resolved relative to the QML engine's base URL. If
  the QML engine has no base URL, just returns \a url.

  \sa url()
*/
QUrl QtObject::resolvedUrl(const QUrl &url, QObject *context) const
{
    if (context) {
        QQmlData *data = QQmlData::get(context);
        if (data && data->outerContext)
            return data->outerContext->resolvedUrl(url);
    }

    if (QQmlEngine *engine = qmlEngine())
        return engine->baseUrl().resolved(url);
    return url;
}

/*!
\qmlmethod list<string> Qt::fontFamilies()

Returns a list of the font families available to the application.
*/
QStringList QtObject::fontFamilies() const
{
    return QQml_guiProvider()->fontFamilies();
}

/*!
\qmlmethod string Qt::md5(data)
Returns a hex string of the md5 hash of \a data.
*/
QString QtObject::md5(const QString &data) const
{
    return QLatin1String(QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Md5).toHex());
}

/*!
\qmlmethod string Qt::btoa(data)
Binary to ASCII - this function returns a base64 encoding of \a data.
*/
QString QtObject::btoa(const QString &data) const
{
    return QLatin1String(data.toUtf8().toBase64());
}

/*!
\qmlmethod string Qt::atob(data)
ASCII to binary - this function decodes the base64 encoded \a data string and returns it.
*/
QString QtObject::atob(const QString &data) const
{
    return QString::fromUtf8(QByteArray::fromBase64(data.toLatin1()));
}

/*!
    \qmlmethod Qt::quit()

    This function causes the QQmlEngine::quit() signal to be emitted.
    Within the \l {Prototyping with the QML Runtime Tool}{qml tool},
    this causes the launcher application to exit; to quit a C++ application
    when this method is called, connect the QQmlEngine::quit() signal to the
    QCoreApplication::quit() slot.

    \sa exit()
*/
void QtObject::quit() const
{
    if (QQmlEngine *engine = qmlEngine())
        QQmlEnginePrivate::get(engine)->sendQuit();
}

/*!
    \qmlmethod Qt::exit(int retCode)

    This function causes the QQmlEngine::exit(int) signal to be emitted.
    Within the \l {Prototyping with the QML Runtime Tool}{qml tool},
    this causes the launcher application to exit with
    the specified return code (\a retCode). To exit from the event loop with a specified
    return code when this method is called, a C++ application can connect the
    QQmlEngine::exit(int) signal to the QCoreApplication::exit(int) slot.

    \sa quit()
*/
void QtObject::exit(int retCode) const
{
    if (QQmlEngine *engine = qmlEngine())
        QQmlEnginePrivate::get(engine)->sendExit(retCode);
}

/*!
\qmlmethod object Qt::createQmlObject(string qml, object parent, string filepath)

Returns a new object created from the given \a qml string which will have the specified \a parent,
or \c null if there was an error in creating the object.

If \a filepath is specified, it will be used for error reporting for the created object.

Example (where \c parentItem is the id of an existing QML item):

\snippet qml/createQmlObject.qml 0

In the case of an error, a QQmlError object is thrown. This object has an additional property,
\c qmlErrors, which is an array of the errors encountered.
Each object in this array has the members \c lineNumber, \c columnNumber, \c fileName and \c message.
For example, if the above snippet had misspelled color as 'colro' then the array would contain an object like the following:
{ "lineNumber" : 1, "columnNumber" : 32, "fileName" : "dynamicSnippet1", "message" : "Cannot assign to non-existent property \"colro\""}.

\note This function returns immediately, and therefore may not work if
the \a qml string loads new components (that is, external QML files that have not yet been loaded).
If this is the case, consider using \l{QtQml::Qt::createComponent()}{Qt.createComponent()} instead.

\warning This function is extremely slow since it has to compile the passed QML string every time
it is invoked. Furthermore, it's very easy to produce invalid QML when programmatically constructing
QML code. It's much better to keep your QML components as separate files and add properties and
methods to customize their behavior than to produce new components by string manipulation.

See \l {Dynamic QML Object Creation from JavaScript} for more information on using this function.
*/
QObject *QtObject::createQmlObject(const QString &qml, QObject *parent, const QUrl &url) const
{
    QQmlEngine *engine = qmlEngine();
    if (!engine) {
        v4Engine()->throwError(QStringLiteral("Qt.createQmlObject(): "
                                              "Can only be called on a QML engine."));
        return nullptr;
    }

    struct Error {
        static ReturnedValue create(QV4::ExecutionEngine *v4, const QList<QQmlError> &errors) {
            Scope scope(v4);
            QString errorstr;
            // '+=' reserves extra capacity. Follow-up appending will be probably free.
            errorstr += QLatin1String("Qt.createQmlObject(): failed to create object: ");

            QV4::ScopedArrayObject qmlerrors(scope, v4->newArrayObject());
            QV4::ScopedObject qmlerror(scope);
            QV4::ScopedString s(scope);
            QV4::ScopedValue v(scope);
            for (int ii = 0; ii < errors.size(); ++ii) {
                const QQmlError &error = errors.at(ii);
                errorstr += QLatin1String("\n    ") + error.toString();
                qmlerror = v4->newObject();
                qmlerror->put((s = v4->newString(QStringLiteral("lineNumber"))), (v = QV4::Value::fromInt32(error.line())));
                qmlerror->put((s = v4->newString(QStringLiteral("columnNumber"))), (v = QV4::Value::fromInt32(error.column())));
                qmlerror->put((s = v4->newString(QStringLiteral("fileName"))), (v = v4->newString(error.url().toString())));
                qmlerror->put((s = v4->newString(QStringLiteral("message"))), (v = v4->newString(error.description())));
                qmlerrors->put(ii, qmlerror);
            }

            v = v4->newString(errorstr);
            ScopedObject errorObject(scope, v4->newErrorObject(v));
            errorObject->put((s = v4->newString(QStringLiteral("qmlErrors"))), qmlerrors);
            return errorObject.asReturnedValue();
        }
    };

    QQmlRefPointer<QQmlContextData> context = v4Engine()->callingQmlContext();
    if (!context)
        context = QQmlContextData::get(QQmlEnginePrivate::get(engine)->rootContext);

    Q_ASSERT(context);
    QQmlContext *effectiveContext = nullptr;
    if (context->isPragmaLibraryContext())
        effectiveContext = engine->rootContext();
    else
        effectiveContext = context->asQQmlContext();
    Q_ASSERT(effectiveContext);

    if (qml.isEmpty())
        return nullptr;

    QUrl resolvedUrl = url;
    if (url.isValid() && url.isRelative())
        resolvedUrl = context->resolvedUrl(url);

    if (!parent) {
        v4Engine()->throwError(QStringLiteral("Qt.createQmlObject(): Missing parent object"));
        return nullptr;
    }

    QQmlRefPointer<QQmlTypeData> typeData = QQmlEnginePrivate::get(engine)->typeLoader.getType(
                qml.toUtf8(), resolvedUrl, QQmlTypeLoader::Synchronous);
    Q_ASSERT(typeData->isCompleteOrError());
    QQmlComponent component(engine);
    QQmlComponentPrivate *componentPrivate = QQmlComponentPrivate::get(&component);
    componentPrivate->fromTypeData(typeData);
    componentPrivate->progress = 1.0;

    Scope scope(v4Engine());
    if (component.isError()) {
        ScopedValue v(scope, Error::create(scope.engine, component.errors()));
        scope.engine->throwError(v);
        return nullptr;
    }

    if (!component.isReady()) {
        v4Engine()->throwError(QStringLiteral("Qt.createQmlObject(): Component is not ready"));
        return nullptr;
    }

    if (!effectiveContext->isValid()) {
        v4Engine()->throwError(QStringLiteral("Qt.createQmlObject(): Cannot create a component "
                                              "in an invalid context"));
        return nullptr;
    }

    QObject *obj = component.beginCreate(effectiveContext);
    if (obj) {
        QQmlData::get(obj, true)->explicitIndestructibleSet = false;
        QQmlData::get(obj)->indestructible = false;

        obj->setParent(parent);

        QList<QQmlPrivate::AutoParentFunction> functions = QQmlMetaType::parentFunctions();
        for (int ii = 0; ii < functions.size(); ++ii) {
            if (QQmlPrivate::Parented == functions.at(ii)(obj, parent))
                break;
        }
    }
    component.completeCreate();

    if (component.isError()) {
        ScopedValue v(scope, Error::create(scope.engine, component.errors()));
        scope.engine->throwError(v);
        return nullptr;
    }

    Q_ASSERT(obj);
    return obj;
}

/*!
\qmlmethod Component Qt::createComponent(url url, enumeration mode, QtObject parent)

Returns a \l Component object created using the QML file at the specified \a url,
or \c null if an empty string was given.

The returned component's \l Component::status property indicates whether the
component was successfully created. If the status is \c Component.Error,
see \l Component::errorString() for an error description.

If the optional \a mode parameter is set to \c Component.Asynchronous, the
component will be loaded in a background thread.  The Component::status property
will be \c Component.Loading while it is loading.  The status will change to
\c Component.Ready if the component loads successfully, or \c Component.Error
if loading fails. This parameter defaults to \c Component.PreferSynchronous
if omitted.

If \a mode is set to \c Component.PreferSynchronous, Qt will attempt to load
the component synchronously, but may end up loading it asynchronously if
necessary. Scenarios that may cause asynchronous loading include, but are not
limited to, the following:

\list
\li The URL refers to a network resource
\li The component is being created as a result of another component that is
being loaded asynchronously
\endlist

If the optional \a parent parameter is given, it should refer to the object
that will become the parent for the created \l Component object. If no mode
was passed, this can be the second argument.

Call \l {Component::createObject()}{Component.createObject()} on the returned
component to create an object instance of the component.

For example:

\snippet qml/createComponent-simple.qml 0

See \l {Dynamic QML Object Creation from JavaScript} for more information on using this function.

To create a QML object from an arbitrary string of QML (instead of a file),
use \l{QtQml::Qt::createQmlObject()}{Qt.createQmlObject()}.
*/

/*!
\qmlmethod Component Qt::createComponent(string moduleUri, string typeName, enumeration mode, QtObject parent)
\overload
Returns a \l Component object created for the type specified by \a moduleUri and \a typeName.
\qml
import QtQuick
QtObject {
    id: root
    property Component myComponent: Qt.createComponent(Rectangle, root)
}
\endqml
This overload mostly behaves as the \c url based version, but can be used
to instantiate types which do not have an URL (e.g. C++ types registered
via \l {QML_ELEMENT}).
\note In some cases, passing \c Component.Asynchronous won't have any
effect:
\list
\li The type is implemented in C++
\li The type is an inline component.
\endlist
If the optional \a parent parameter is given, it should refer to the object
that will become the parent for the created \l Component object. If no mode
was passed, this can be the second argument.
*/

QQmlComponent *QtObject::createComponent(const QUrl &url, QObject *parent) const
{
    return createComponent(url, QQmlComponent::PreferSynchronous, parent);
}

QQmlComponent *QtObject::createComponent(const QUrl &url, QQmlComponent::CompilationMode mode,
                                         QObject *parent) const
{
    if (mode != QQmlComponent::Asynchronous && mode != QQmlComponent::PreferSynchronous) {
        v4Engine()->throwError(QStringLiteral("Invalid compilation mode %1").arg(mode));
        return nullptr;
    }

    if (url.isEmpty())
        return nullptr;

    QQmlEngine *engine = qmlEngine();
    if (!engine)
        return nullptr;

    auto [context, effectiveContext] = getContexts();
    if (!context)
        return nullptr;

    QQmlComponent *c = new QQmlComponent(engine, context->resolvedUrl(url), mode, parent);
    QQmlComponentPrivate::get(c)->creationContext = effectiveContext;
    QQmlData::get(c, true)->explicitIndestructibleSet = false;
    QQmlData::get(c)->indestructible = false;
    return c;
}

QQmlComponent *QtObject::createComponent(const QString &moduleUri, const QString &typeName,
                                         QObject *parent) const
{
    return createComponent(moduleUri, typeName, QQmlComponent::PreferSynchronous, parent);
}

QQmlComponent *QtObject::createComponent(const QString &moduleUri, const QString &typeName, QQmlComponent::CompilationMode mode, QObject *parent) const
{
    if (mode != QQmlComponent::Asynchronous && mode != QQmlComponent::PreferSynchronous) {
        v4Engine()->throwError(QStringLiteral("Invalid compilation mode %1").arg(mode));
        return nullptr;
    }

    QQmlEngine *engine = qmlEngine();
    if (!engine)
        return nullptr;

    if (moduleUri.isEmpty() || typeName.isEmpty())
        return nullptr;

    auto [context, effectiveContext] = getContexts();
    if (!context)
        return nullptr;

    QQmlComponent *c = new QQmlComponent(engine, moduleUri, typeName, mode, parent);
    if (c->isError() && !parent && moduleUri.endsWith(u".qml")) {
        v4Engine()->throwTypeError(
                    QStringLiteral("Invalid arguments; did you swap mode and parent"));
    }
    QQmlComponentPrivate::get(c)->creationContext = effectiveContext;
    QQmlData::get(c, true)->explicitIndestructibleSet = false;
    QQmlData::get(c)->indestructible = false;
    return c;
}

#if QT_CONFIG(translation)
QString QtObject::uiLanguage() const
{
    if (const QJSEngine *e = jsEngine())
        return e->uiLanguage();
    return QString();
}

void QtObject::setUiLanguage(const QString &uiLanguage)
{
    if (QJSEngine *e = jsEngine())
        e->setUiLanguage(uiLanguage);
}

QBindable<QString> QtObject::uiLanguageBindable()
{
    if (QJSEngine *e = jsEngine())
        return QBindable<QString>(&QJSEnginePrivate::get(e)->uiLanguage);
    return QBindable<QString>();
}
#endif

#if QT_CONFIG(qml_locale)
/*!
    \qmlmethod Qt::locale(name)

    Returns a JS object representing the locale with the specified
    \a name, which has the format "language[_territory][.codeset][@modifier]"
    or "C", where:

    \list
    \li \c language is a lowercase, two-letter, ISO 639 language code,
    \li \c territory is an uppercase, two-letter, ISO 3166 country code, and
    \li \c codeset and \c modifier are ignored.
    \endlist

    If the string violates the locale format, or language is not a
    valid ISO 369 code, the "C" locale is used instead. If country
    is not present, or is not a valid ISO 3166 code, the most
    appropriate country is chosen for the specified language.

    \sa Locale
*/
QLocale QtObject::locale() const
{
    return QLocale();
}

QLocale QtObject::locale(const QString &name) const
{
    return QLocale(name);
}
#endif

void Heap::QQmlBindingFunction::init(const QV4::FunctionObject *bindingFunction)
{
    Scope scope(bindingFunction->engine());
    ScopedContext context(scope, bindingFunction->scope());
    FunctionObject::init(context, bindingFunction->function());
    this->bindingFunction.set(internalClass->engine, bindingFunction->d());
}

QQmlSourceLocation QQmlBindingFunction::currentLocation() const
{
    QV4::CppStackFrame *frame = engine()->currentStackFrame;
    if (frame->v4Function) // synchronous loading:
        return QQmlSourceLocation(frame->source(), frame->lineNumber(), 0);
    else // async loading:
        return bindingFunction()->function->sourceLocation();
}

DEFINE_OBJECT_VTABLE(QQmlBindingFunction);

/*!
    \qmlmethod Qt::binding(function)

    Returns a JavaScript object representing a \l{Property Binding}{property binding},
    with a \a function that evaluates the binding.

    There are two main use-cases for the function: firstly, to apply a
    property binding imperatively from JavaScript code:

    \snippet qml/qtBinding.1.qml 0

    and secondly, to apply a property binding when initializing property values
    of dynamically constructed objects (via \l{Component::createObject()}
    {Component.createObject()} or \l{Loader::setSource()}{Loader.setSource()}).

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

    \since 5.0
*/
QJSValue QtObject::binding(const QJSValue &function) const
{
    const QV4::FunctionObject *f = QJSValuePrivate::asManagedType<FunctionObject>(&function);
    QV4::ExecutionEngine *e = v4Engine();
    if (!f) {
        return QJSValuePrivate::fromReturnedValue(
                    e->throwError(
                        QStringLiteral(
                            "binding(): argument (binding expression) must be a function")));
    }

    return QJSValuePrivate::fromReturnedValue(
                Encode(e->memoryManager->allocate<QQmlBindingFunction>(f)));
}

void QtObject::callLater(QQmlV4Function *args)
{
    m_engine->delayedCallQueue()->addUniquelyAndExecuteLater(m_engine, args);
}


QQmlPlatform *QtObject::platform()
{
    if (!m_platform)
        m_platform = new QQmlPlatform(this);
    return m_platform;
}

QQmlApplication *QtObject::application()
{
    if (!m_application)
        // Only allocate an application object once
        m_application = QQml_guiProvider()->application(this);

    return m_application;
}

QObject *QtObject::inputMethod() const
{
    return QQml_guiProvider()->inputMethod();
}

QObject *QtObject::styleHints() const
{
    return QQml_guiProvider()->styleHints();
}

void QV4::Heap::ConsoleObject::init()
{
    Object::init();
    QV4::Scope scope(internalClass->engine);
    QV4::ScopedObject o(scope, this);

    o->defineDefaultProperty(QStringLiteral("debug"), QV4::ConsoleObject::method_log);
    o->defineDefaultProperty(QStringLiteral("log"), QV4::ConsoleObject::method_log);
    o->defineDefaultProperty(QStringLiteral("info"), QV4::ConsoleObject::method_info);
    o->defineDefaultProperty(QStringLiteral("warn"), QV4::ConsoleObject::method_warn);
    o->defineDefaultProperty(QStringLiteral("error"), QV4::ConsoleObject::method_error);
    o->defineDefaultProperty(QStringLiteral("assert"), QV4::ConsoleObject::method_assert);

    o->defineDefaultProperty(QStringLiteral("count"), QV4::ConsoleObject::method_count);
    o->defineDefaultProperty(QStringLiteral("profile"), QV4::ConsoleObject::method_profile);
    o->defineDefaultProperty(QStringLiteral("profileEnd"), QV4::ConsoleObject::method_profileEnd);
    o->defineDefaultProperty(QStringLiteral("time"), QV4::ConsoleObject::method_time);
    o->defineDefaultProperty(QStringLiteral("timeEnd"), QV4::ConsoleObject::method_timeEnd);
    o->defineDefaultProperty(QStringLiteral("trace"), QV4::ConsoleObject::method_trace);
    o->defineDefaultProperty(QStringLiteral("exception"), QV4::ConsoleObject::method_exception);
}


enum ConsoleLogTypes {
    Log,
    Info,
    Warn,
    Error
};

static QString jsStack(QV4::ExecutionEngine *engine) {
    QString stack;

    QVector<QV4::StackFrame> stackTrace = engine->stackTrace(10);

    for (int i = 0; i < stackTrace.size(); i++) {
        const QV4::StackFrame &frame = stackTrace.at(i);

        QString stackFrame;
        if (frame.column >= 0) {
            stackFrame = QStringLiteral("%1 (%2:%3:%4)").arg(
                frame.function, frame.source,
                QString::number(qAbs(frame.line)), QString::number(frame.column));
        } else {
            stackFrame = QStringLiteral("%1 (%2:%3)").arg(
                frame.function, frame.source, QString::number(qAbs(frame.line)));
        }

        if (i)
            stack += QLatin1Char('\n');
        stack += stackFrame;
    }
    return stack;
}

static QString serializeArray(Object *array, ExecutionEngine *v4, QSet<QV4::Heap::Object *> &alreadySeen) {
    Scope scope(v4);
    ScopedValue val(scope);
    QString result;

    alreadySeen.insert(array->d());
    result += QLatin1Char('[');
    const uint length = array->getLength();
    for (uint i = 0; i < length; ++i) {
        if (i != 0)
            result += QLatin1Char(',');
        val = array->get(i);
        if (val->isManaged() && val->managed()->isArrayLike())
            if (!alreadySeen.contains(val->objectValue()->d()))
                result += serializeArray(val->objectValue(), v4, alreadySeen);
            else
                result += QLatin1String("[Circular]");
        else
            result += val->toQStringNoThrow();
    }
    result += QLatin1Char(']');
    alreadySeen.remove(array->d());
    return result;
};

static ReturnedValue writeToConsole(const FunctionObject *b, const Value *argv, int argc,
                                    ConsoleLogTypes logType, bool printStack = false)
{
    const QLoggingCategory *loggingCategory = nullptr;
    QString result;
    QV4::Scope scope(b);
    QV4::ExecutionEngine *v4 = scope.engine;

    int start = 0;
    if (argc > 0) {
        if (const QObjectWrapper* wrapper = argv[0].as<QObjectWrapper>()) {
            if (QQmlLoggingCategory* category = qobject_cast<QQmlLoggingCategory*>(wrapper->object())) {
                if (category->category())
                    loggingCategory = category->category();
                else
                    THROW_GENERIC_ERROR("A QmlLoggingCatgory was provided without a valid name");
                start = 1;
            }
        }
    }


    for (int i = start, ei = argc; i < ei; ++i) {
        if (i != start)
            result.append(QLatin1Char(' '));

        QSet<QV4::Heap::Object *> alreadySeenElements;
        if (argv[i].isManaged() && argv[i].managed()->isArrayLike())
            result.append(serializeArray(argv[i].objectValue(), v4, alreadySeenElements));
        else
            result.append(argv[i].toQStringNoThrow());
    }

    if (printStack)
        result += QLatin1Char('\n') + jsStack(v4);

    if (!loggingCategory)
        loggingCategory = v4->qmlEngine() ? &lcQml() : &lcJs();
    QV4::CppStackFrame *frame = v4->currentStackFrame;
    const QByteArray baSource = frame ? frame->source().toUtf8() : QByteArray();
    const QByteArray baFunction = frame ? frame->function().toUtf8() : QByteArray();
    QMessageLogger logger(baSource.constData(), frame ? frame->lineNumber() : 0,
                          baFunction.constData(), loggingCategory->categoryName());

    switch (logType) {
    case Log:
        if (loggingCategory->isDebugEnabled())
            logger.debug("%s", result.toUtf8().constData());
        break;
    case Info:
        if (loggingCategory->isInfoEnabled())
            logger.info("%s", result.toUtf8().constData());
        break;
    case Warn:
        if (loggingCategory->isWarningEnabled())
            logger.warning("%s", result.toUtf8().constData());
        break;
    case Error:
        if (loggingCategory->isCriticalEnabled())
            logger.critical("%s", result.toUtf8().constData());
        break;
    default:
        break;
    }

    return Encode::undefined();
}

DEFINE_OBJECT_VTABLE(ConsoleObject);

ReturnedValue ConsoleObject::method_error(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    return writeToConsole(b, argv, argc, Error);
}

ReturnedValue ConsoleObject::method_log(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    //console.log
    //console.debug
    //print
    return writeToConsole(b, argv, argc, Log);
}

ReturnedValue ConsoleObject::method_info(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    return writeToConsole(b, argv, argc, Info);
}

ReturnedValue ConsoleObject::method_profile(const FunctionObject *b, const Value *, const Value *, int)
{
    QV4::Scope scope(b);
    QV4::ExecutionEngine *v4 = scope.engine;

    QV4::CppStackFrame *frame = v4->currentStackFrame;
    const QByteArray baSource = frame->source().toUtf8();
    const QByteArray baFunction = frame->function().toUtf8();
    QMessageLogger logger(baSource.constData(), frame->lineNumber(), baFunction.constData());
    QQmlProfilerService *service = QQmlDebugConnector::service<QQmlProfilerService>();
    if (!service) {
        logger.warning("Cannot start profiling because debug service is disabled. Start with -qmljsdebugger=port:XXXXX.");
    } else {
        service->startProfiling(v4->jsEngine());
        logger.debug("Profiling started.");
    }

    return QV4::Encode::undefined();
}

ReturnedValue ConsoleObject::method_profileEnd(const FunctionObject *b, const Value *, const Value *, int)
{
    QV4::Scope scope(b);
    QV4::ExecutionEngine *v4 = scope.engine;

    QV4::CppStackFrame *frame = v4->currentStackFrame;
    const QByteArray baSource = frame->source().toUtf8();
    const QByteArray baFunction = frame->function().toUtf8();
    QMessageLogger logger(baSource.constData(), frame->lineNumber(), baFunction.constData());

    QQmlProfilerService *service = QQmlDebugConnector::service<QQmlProfilerService>();
    if (!service) {
        logger.warning("Ignoring console.profileEnd(): the debug service is disabled.");
    } else {
        service->stopProfiling(v4->jsEngine());
        logger.debug("Profiling ended.");
    }

    return QV4::Encode::undefined();
}

ReturnedValue ConsoleObject::method_time(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc != 1)
        THROW_GENERIC_ERROR("console.time(): Invalid arguments");

    QString name = argv[0].toQStringNoThrow();
    scope.engine->startTimer(name);
    return QV4::Encode::undefined();
}

ReturnedValue ConsoleObject::method_timeEnd(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc != 1)
        THROW_GENERIC_ERROR("console.timeEnd(): Invalid arguments");

    QString name = argv[0].toQStringNoThrow();
    bool wasRunning;
    qint64 elapsed = scope.engine->stopTimer(name, &wasRunning);
    if (wasRunning) {
        qDebug("%s: %llims", qPrintable(name), elapsed);
    }
    return QV4::Encode::undefined();
}

ReturnedValue ConsoleObject::method_count(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    // first argument: name to print. Ignore any additional arguments
    QString name;
    if (argc > 0)
        name = argv[0].toQStringNoThrow();

    Scope scope(b);
    QV4::ExecutionEngine *v4 = scope.engine;

    QV4::CppStackFrame *frame = v4->currentStackFrame;

    QString scriptName = frame->source();

    int value = v4->consoleCountHelper(scriptName, frame->lineNumber(), 0);
    QString message = name + QLatin1String(": ") + QString::number(value);

    QMessageLogger(qPrintable(scriptName), frame->lineNumber(),
                   qPrintable(frame->function()))
        .debug("%s", qPrintable(message));

    return QV4::Encode::undefined();
}

ReturnedValue ConsoleObject::method_trace(const FunctionObject *b, const Value *, const Value *, int argc)
{
    QV4::Scope scope(b);
    if (argc != 0)
        THROW_GENERIC_ERROR("console.trace(): Invalid arguments");

    QV4::ExecutionEngine *v4 = scope.engine;

    QString stack = jsStack(v4);

    QV4::CppStackFrame *frame = v4->currentStackFrame;
    QMessageLogger(frame->source().toUtf8().constData(), frame->lineNumber(),
                   frame->function().toUtf8().constData())
        .debug("%s", qPrintable(stack));

    return QV4::Encode::undefined();
}

ReturnedValue ConsoleObject::method_warn(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    return writeToConsole(b, argv, argc, Warn);
}

ReturnedValue ConsoleObject::method_assert(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc == 0)
        THROW_GENERIC_ERROR("console.assert(): Missing argument");

    QV4::ExecutionEngine *v4 = scope.engine;

    if (!argv[0].toBoolean()) {
        QString message;
        for (int i = 1, ei = argc; i < ei; ++i) {
            if (i != 1)
                message.append(QLatin1Char(' '));

            message.append(argv[i].toQStringNoThrow());
        }

        QString stack = jsStack(v4);

        QV4::CppStackFrame *frame = v4->currentStackFrame;
        QMessageLogger(frame->source().toUtf8().constData(), frame->lineNumber(),
                       frame->function().toUtf8().constData())
            .critical("%s\n%s",qPrintable(message), qPrintable(stack));

    }
    return QV4::Encode::undefined();
}

ReturnedValue ConsoleObject::method_exception(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc == 0)
        THROW_GENERIC_ERROR("console.exception(): Missing argument");

    return writeToConsole(b, argv, argc, Error, true);
}

void QV4::GlobalExtensions::init(Object *globalObject, QJSEngine::Extensions extensions)
{
    ExecutionEngine *v4 = globalObject->engine();
    Scope scope(v4);

    if (extensions.testFlag(QJSEngine::TranslationExtension)) {
    #if QT_CONFIG(translation)
        globalObject->defineDefaultProperty(QStringLiteral("qsTranslate"), QV4::GlobalExtensions::method_qsTranslate);
        globalObject->defineDefaultProperty(QStringLiteral("QT_TRANSLATE_NOOP"), QV4::GlobalExtensions::method_qsTranslateNoOp);
        globalObject->defineDefaultProperty(QStringLiteral("qsTr"), QV4::GlobalExtensions::method_qsTr);
        globalObject->defineDefaultProperty(QStringLiteral("QT_TR_NOOP"), QV4::GlobalExtensions::method_qsTrNoOp);
        globalObject->defineDefaultProperty(QStringLiteral("qsTrId"), QV4::GlobalExtensions::method_qsTrId);
        globalObject->defineDefaultProperty(QStringLiteral("QT_TRID_NOOP"), QV4::GlobalExtensions::method_qsTrIdNoOp);

        // Initialize the Qt global object for the uiLanguage property
        ScopedString qtName(scope, v4->newString(QStringLiteral("Qt")));
        ScopedObject qt(scope, globalObject->get(qtName));
        if (!qt)
            v4->createQtObject();

        // string prototype extension
        scope.engine->stringPrototype()->defineDefaultProperty(QStringLiteral("arg"), QV4::GlobalExtensions::method_string_arg);
    #endif
    }

    if (extensions.testFlag(QJSEngine::ConsoleExtension)) {
        globalObject->defineDefaultProperty(QStringLiteral("print"), QV4::ConsoleObject::method_log);


        QV4::ScopedObject console(scope, globalObject->engine()->memoryManager->allocate<QV4::ConsoleObject>());
        globalObject->defineDefaultProperty(QStringLiteral("console"), console);
    }

    if (extensions.testFlag(QJSEngine::GarbageCollectionExtension)) {
        globalObject->defineDefaultProperty(QStringLiteral("gc"), QV4::GlobalExtensions::method_gc);
    }
}


#if QT_CONFIG(translation)
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

    \sa {Internationalization with Qt}
*/
ReturnedValue GlobalExtensions::method_qsTranslate(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc < 2)
        THROW_GENERIC_ERROR("qsTranslate() requires at least two arguments");
    if (!argv[0].isString())
        THROW_GENERIC_ERROR("qsTranslate(): first argument (context) must be a string");
    if (!argv[1].isString())
        THROW_GENERIC_ERROR("qsTranslate(): second argument (sourceText) must be a string");
    if ((argc > 2) && !argv[2].isString())
        THROW_GENERIC_ERROR("qsTranslate(): third argument (disambiguation) must be a string");

    QString context = argv[0].toQStringNoThrow();
    QString text = argv[1].toQStringNoThrow();
    QString comment;
    if (argc > 2) comment = argv[2].toQStringNoThrow();

    int i = 3;
    if (argc > i && argv[i].isString()) {
        qWarning("qsTranslate(): specifying the encoding as fourth argument is deprecated");
        ++i;
    }

    int n = -1;
    if (argc > i)
        n = argv[i].toInt32();

    if (QQmlEnginePrivate *ep = (scope.engine->qmlEngine() ? QQmlEnginePrivate::get(scope.engine->qmlEngine()) : nullptr))
        if (ep->propertyCapture)
            ep->propertyCapture->captureTranslation();

    QString result = QCoreApplication::translate(context.toUtf8().constData(),
                                                 text.toUtf8().constData(),
                                                 comment.toUtf8().constData(),
                                                 n);

    return Encode(scope.engine->newString(result));
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

    \sa {Internationalization with Qt}
*/
ReturnedValue GlobalExtensions::method_qsTranslateNoOp(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc < 2)
        return QV4::Encode::undefined();
    else
        return argv[1].asReturnedValue();
}

QString GlobalExtensions::currentTranslationContext(ExecutionEngine *engine)
{
    QString context;
    CppStackFrame *frame = engine->currentStackFrame;

    // The first non-empty source URL in the call stack determines the translation context.
    while (frame && context.isEmpty()) {
        if (CompiledData::CompilationUnitBase *baseUnit = frame->v4Function->compilationUnit) {
            const auto *unit = static_cast<const CompiledData::CompilationUnit *>(baseUnit);
            QString fileName = unit->fileName();
            QUrl url(unit->fileName());
            if (url.isValid() && url.isRelative()) {
                context = url.fileName();
            } else {
                context = QQmlFile::urlToLocalFileOrQrc(fileName);
                if (context.isEmpty() && fileName.startsWith(QLatin1String(":/")))
                    context = fileName;
            }
            context = QFileInfo(context).completeBaseName();
        }
        frame = frame->parentFrame();
    }

    if (context.isEmpty()) {
        if (QQmlRefPointer<QQmlContextData> ctxt = engine->callingQmlContext()) {
            QString path = ctxt->urlString();
            int lastSlash = path.lastIndexOf(QLatin1Char('/'));
            int lastDot = path.lastIndexOf(QLatin1Char('.'));
            int length = lastDot - (lastSlash + 1);
            context = (lastSlash > -1) ? path.mid(lastSlash + 1, (length > -1) ? length : -1) : QString();
        }
    }

    return context;
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

    \sa {Internationalization with Qt}
*/
ReturnedValue GlobalExtensions::method_qsTr(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc < 1)
        THROW_GENERIC_ERROR("qsTr() requires at least one argument");
    if (!argv[0].isString())
        THROW_GENERIC_ERROR("qsTr(): first argument (sourceText) must be a string");
    if ((argc > 1) && !argv[1].isString())
        THROW_GENERIC_ERROR("qsTr(): second argument (disambiguation) must be a string");
    if ((argc > 2) && !argv[2].isNumber())
        THROW_GENERIC_ERROR("qsTr(): third argument (n) must be a number");

    const QString context = currentTranslationContext(scope.engine);
    const QString text = argv[0].toQStringNoThrow();
    const QString comment = argc > 1 ? argv[1].toQStringNoThrow() : QString();
    const int n = argc > 2 ? argv[2].toInt32() : -1;

    if (QQmlEnginePrivate *ep = (scope.engine->qmlEngine() ? QQmlEnginePrivate::get(scope.engine->qmlEngine()) : nullptr))
        if (ep->propertyCapture)
            ep->propertyCapture->captureTranslation();

    QString result = QCoreApplication::translate(context.toUtf8().constData(), text.toUtf8().constData(),
                                                 comment.toUtf8().constData(), n);

    return Encode(scope.engine->newString(result));
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

    \sa {Internationalization with Qt}
*/
ReturnedValue GlobalExtensions::method_qsTrNoOp(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    if (argc < 1)
        return QV4::Encode::undefined();
    else
        return argv[0].asReturnedValue();
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

    \sa QT_TRID_NOOP(), {Internationalization with Qt}
*/
ReturnedValue GlobalExtensions::method_qsTrId(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc < 1)
        THROW_GENERIC_ERROR("qsTrId() requires at least one argument");
    if (!argv[0].isString())
        THROW_TYPE_ERROR_WITH_MESSAGE("qsTrId(): first argument (id) must be a string");
    if (argc > 1 && !argv[1].isNumber())
        THROW_TYPE_ERROR_WITH_MESSAGE("qsTrId(): second argument (n) must be a number");

    int n = -1;
    if (argc > 1)
        n = argv[1].toInt32();

    if (QQmlEnginePrivate *ep = (scope.engine->qmlEngine() ? QQmlEnginePrivate::get(scope.engine->qmlEngine()) : nullptr))
        if (ep->propertyCapture)
            ep->propertyCapture->captureTranslation();

    return Encode(scope.engine->newString(qtTrId(argv[0].toQStringNoThrow().toUtf8().constData(), n)));
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

    \sa qsTrId(), {Internationalization with Qt}
*/
ReturnedValue GlobalExtensions::method_qsTrIdNoOp(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    if (argc < 1)
        return QV4::Encode::undefined();
    else
        return argv[0].asReturnedValue();
}
#endif // translation


ReturnedValue GlobalExtensions::method_gc(const FunctionObject *b, const Value *, const Value *, int)
{
    b->engine()->memoryManager->runGC();

    return QV4::Encode::undefined();
}



ReturnedValue GlobalExtensions::method_string_arg(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    QV4::Scope scope(b);
    if (argc != 1)
        THROW_GENERIC_ERROR("String.arg(): Invalid arguments");

    QString value = thisObject->toQString();

    QV4::ScopedValue arg(scope, argv[0]);
    if (arg->isInteger())
        RETURN_RESULT(scope.engine->newString(value.arg(arg->integerValue())));
    else if (arg->isDouble())
        RETURN_RESULT(scope.engine->newString(value.arg(arg->doubleValue())));
    else if (arg->isBoolean())
        RETURN_RESULT(scope.engine->newString(value.arg(arg->booleanValue())));

    RETURN_RESULT(scope.engine->newString(value.arg(arg->toQString())));
}

/*!
\qmlmethod Qt::callLater(function)
\qmlmethod Qt::callLater(function, argument1, argument2, ...)
\since 5.8
Use this function to eliminate redundant calls to a function or signal.

The function passed as the first argument to Qt.callLater()
will be called later, once the QML engine returns to the event loop.

When this function is called multiple times in quick succession with the
same function as its first argument, that function will be called only once.

For example:
\snippet qml/qtLater.qml 0

Any additional arguments passed to Qt.callLater() will
be passed on to the function invoked. Note that if redundant calls
are eliminated, then only the last set of arguments will be passed to the
function.
*/

QT_END_NAMESPACE

#include "moc_qqmlbuiltinfunctions_p.cpp"
