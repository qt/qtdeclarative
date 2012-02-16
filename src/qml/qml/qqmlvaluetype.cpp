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

#include "qqmlvaluetype_p.h"

#include "qqmlmetatype_p.h"
#include <private/qfont_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

template<typename T>
int qmlRegisterValueTypeEnums(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');

    QQmlPrivate::RegisterType type = {
        0,

        qRegisterMetaType<T *>(pointerName.constData()), 0, 0, 0,

        QString(),

        uri, versionMajor, versionMinor, qmlName, &T::staticMetaObject,

        0, 0,

        0, 0, 0,

        0, 0,

        0,
        0
    };

    return QQmlPrivate::qmlregister(QQmlPrivate::TypeRegistration, &type);
}

QQmlValueTypeFactory::QQmlValueTypeFactory()
{
    // ### Optimize
    for (unsigned int ii = 0; ii < (QVariant::UserType - 1); ++ii)
        valueTypes[ii] = valueType(ii);
}

QQmlValueTypeFactory::~QQmlValueTypeFactory()
{
    for (unsigned int ii = 0; ii < (QVariant::UserType - 1); ++ii)
        delete valueTypes[ii];
}

bool QQmlValueTypeFactory::isValueType(int idx)
{
    if ((uint)idx < QVariant::UserType
            && idx != QVariant::StringList
            && idx != QMetaType::QObjectStar
            && idx != QMetaType::QWidgetStar
            && idx != QMetaType::VoidStar
            && idx != QMetaType::QVariant) {
        return true;
    }
    return false;
}

void QQmlValueTypeFactory::registerBaseTypes(const char *uri, int versionMajor, int versionMinor)
{
    qmlRegisterValueTypeEnums<QQmlEasingValueType>(uri, versionMajor, versionMinor, "Easing");
    qmlRegisterValueTypeEnums<QQmlFontValueType>(uri, versionMajor, versionMinor, "Font");
}

void QQmlValueTypeFactory::registerValueTypes()
{
    registerBaseTypes("QtQuick", 2, 0);
}

QQmlValueType *QQmlValueTypeFactory::valueType(int t)
{
    QQmlValueType *rv = 0;

    switch (t) {
    case QVariant::Point:
        rv = new QQmlPointValueType;
        break;
    case QVariant::PointF:
        rv = new QQmlPointFValueType;
        break;
    case QVariant::Size:
        rv = new QQmlSizeValueType;
        break;
    case QVariant::SizeF:
        rv = new QQmlSizeFValueType;
        break;
    case QVariant::Rect:
        rv = new QQmlRectValueType;
        break;
    case QVariant::RectF:
        rv = new QQmlRectFValueType;
        break;
    case QVariant::Vector2D:
        rv = new QQmlVector2DValueType;
        break;
    case QVariant::Vector3D:
        rv = new QQmlVector3DValueType;
        break;
    case QVariant::Vector4D:
        rv = new QQmlVector4DValueType;
        break;
    case QVariant::Quaternion:
        rv = new QQmlQuaternionValueType;
        break;
    case QVariant::Matrix4x4:
        rv = new QQmlMatrix4x4ValueType;
        break;
    case QVariant::EasingCurve:
        rv = new QQmlEasingValueType;
        break;
    case QVariant::Font:
        rv = new QQmlFontValueType;
        break;
    case QVariant::Color:
        rv = new QQmlColorValueType;
        break;
    default:
        break;
    }

    Q_ASSERT(!rv || rv->metaObject()->propertyCount() < 32);
    return rv;
}

QQmlValueType::QQmlValueType(QObject *parent)
: QObject(parent)
{
}

#define QML_VALUETYPE_READWRITE(name, cpptype, var) \
    QQml ## name ## ValueType::QQml ## name ## ValueType(QObject *parent) \
    : QQmlValueType(parent) \
    { \
    } \
    void QQml ## name ## ValueType::read(QObject *obj, int idx) \
    { \
        void *a[] = { &var, 0 }; \
        QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a); \
        onLoad(); \
    } \
    void QQml ## name ## ValueType::write(QObject *obj, int idx, \
                                                  QQmlPropertyPrivate::WriteFlags flags) \
    { \
        int status = -1; \
        void *a[] = { &var, 0, &status, &flags }; \
        QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a); \
    } \
    bool QQml ## name ## ValueType::isEqual(const QVariant &value) const \
    { \
        return QVariant(var) == value; \
    } \
    QVariant QQml ## name ## ValueType::value() \
    { \
        return QVariant(var); \
    } \
    void QQml ## name ## ValueType::setValue(const QVariant &value) \
    { \
        var = qvariant_cast<cpptype>(value); \
        onLoad(); \
    }

QML_VALUETYPE_READWRITE(PointF, QPointF, point);
QML_VALUETYPE_READWRITE(Point, QPoint, point);
QML_VALUETYPE_READWRITE(SizeF, QSizeF, size);
QML_VALUETYPE_READWRITE(Size, QSize, size);
QML_VALUETYPE_READWRITE(RectF, QRectF, rect);
QML_VALUETYPE_READWRITE(Rect, QRect, rect);
QML_VALUETYPE_READWRITE(Vector2D, QVector2D, vector);
QML_VALUETYPE_READWRITE(Vector3D, QVector3D, vector);
QML_VALUETYPE_READWRITE(Vector4D, QVector4D, vector);
QML_VALUETYPE_READWRITE(Quaternion, QQuaternion, quaternion);
QML_VALUETYPE_READWRITE(Matrix4x4, QMatrix4x4, matrix);
QML_VALUETYPE_READWRITE(Easing, QEasingCurve, easing);
QML_VALUETYPE_READWRITE(Font, QFont, font);
QML_VALUETYPE_READWRITE(Color, QColor, color);

QString QQmlPointFValueType::toString() const
{
    return QString(QLatin1String("QPointF(%1, %2)")).arg(point.x()).arg(point.y());
}

qreal QQmlPointFValueType::x() const
{
    return point.x();
}

qreal QQmlPointFValueType::y() const
{
    return point.y();
}

void QQmlPointFValueType::setX(qreal x)
{
    point.setX(x);
}

void QQmlPointFValueType::setY(qreal y)
{
    point.setY(y);
}

QString QQmlPointValueType::toString() const
{
    return QString(QLatin1String("QPoint(%1, %2)")).arg(point.x()).arg(point.y());
}

int QQmlPointValueType::x() const
{
    return point.x();
}

int QQmlPointValueType::y() const
{
    return point.y();
}

void QQmlPointValueType::setX(int x)
{
    point.setX(x);
}

void QQmlPointValueType::setY(int y)
{
    point.setY(y);
}

QString QQmlSizeFValueType::toString() const
{
    return QString(QLatin1String("QSizeF(%1, %2)")).arg(size.width()).arg(size.height());
}

qreal QQmlSizeFValueType::width() const
{
    return size.width();
}

qreal QQmlSizeFValueType::height() const
{
    return size.height();
}

void QQmlSizeFValueType::setWidth(qreal w)
{
    size.setWidth(w);
}

void QQmlSizeFValueType::setHeight(qreal h)
{
    size.setHeight(h);
}

QString QQmlSizeValueType::toString() const
{
    return QString(QLatin1String("QSize(%1, %2)")).arg(size.width()).arg(size.height());
}

int QQmlSizeValueType::width() const
{
    return size.width();
}

int QQmlSizeValueType::height() const
{
    return size.height();
}

void QQmlSizeValueType::setWidth(int w)
{
    size.setWidth(w);
}

void QQmlSizeValueType::setHeight(int h)
{
    size.setHeight(h);
}

QString QQmlRectFValueType::toString() const
{
    return QString(QLatin1String("QRectF(%1, %2, %3, %4)")).arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
}

qreal QQmlRectFValueType::x() const
{
    return rect.x();
}

qreal QQmlRectFValueType::y() const
{
    return rect.y();
}

void QQmlRectFValueType::setX(qreal x)
{
    rect.moveLeft(x);
}

void QQmlRectFValueType::setY(qreal y)
{
    rect.moveTop(y);
}

qreal QQmlRectFValueType::width() const
{
    return rect.width();
}

qreal QQmlRectFValueType::height() const
{
    return rect.height();
}

void QQmlRectFValueType::setWidth(qreal w)
{
    rect.setWidth(w);
}

void QQmlRectFValueType::setHeight(qreal h)
{
    rect.setHeight(h);
}

QString QQmlRectValueType::toString() const
{
    return QString(QLatin1String("QRect(%1, %2, %3, %4)")).arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
}

int QQmlRectValueType::x() const
{
    return rect.x();
}

int QQmlRectValueType::y() const
{
    return rect.y();
}

void QQmlRectValueType::setX(int x)
{
    rect.moveLeft(x);
}

void QQmlRectValueType::setY(int y)
{
    rect.moveTop(y);
}

int QQmlRectValueType::width() const
{
    return rect.width();
}

int QQmlRectValueType::height() const
{
    return rect.height();
}

void QQmlRectValueType::setWidth(int w)
{
    rect.setWidth(w);
}

void QQmlRectValueType::setHeight(int h)
{
    rect.setHeight(h);
}

QString QQmlVector2DValueType::toString() const
{
    return QString(QLatin1String("QVector2D(%1, %2)")).arg(vector.x()).arg(vector.y());
}

qreal QQmlVector2DValueType::x() const
{
    return vector.x();
}

qreal QQmlVector2DValueType::y() const
{
    return vector.y();
}

void QQmlVector2DValueType::setX(qreal x)
{
    vector.setX(x);
}

void QQmlVector2DValueType::setY(qreal y)
{
    vector.setY(y);
}

QString QQmlVector3DValueType::toString() const
{
    return QString(QLatin1String("QVector3D(%1, %2, %3)")).arg(vector.x()).arg(vector.y()).arg(vector.z());
}

qreal QQmlVector3DValueType::x() const
{
    return vector.x();
}

qreal QQmlVector3DValueType::y() const
{
    return vector.y();
}

qreal QQmlVector3DValueType::z() const
{
    return vector.z();
}

void QQmlVector3DValueType::setX(qreal x)
{
    vector.setX(x);
}

void QQmlVector3DValueType::setY(qreal y)
{
    vector.setY(y);
}

void QQmlVector3DValueType::setZ(qreal z)
{
    vector.setZ(z);
}

QString QQmlVector4DValueType::toString() const
{
    return QString(QLatin1String("QVector4D(%1, %2, %3, %4)")).arg(vector.x()).arg(vector.y()).arg(vector.z()).arg(vector.w());
}

qreal QQmlVector4DValueType::x() const
{
    return vector.x();
}

qreal QQmlVector4DValueType::y() const
{
    return vector.y();
}

qreal QQmlVector4DValueType::z() const
{
    return vector.z();
}

qreal QQmlVector4DValueType::w() const
{
    return vector.w();
}

void QQmlVector4DValueType::setX(qreal x)
{
    vector.setX(x);
}

void QQmlVector4DValueType::setY(qreal y)
{
    vector.setY(y);
}

void QQmlVector4DValueType::setZ(qreal z)
{
    vector.setZ(z);
}

void QQmlVector4DValueType::setW(qreal w)
{
    vector.setW(w);
}

QString QQmlQuaternionValueType::toString() const
{
    return QString(QLatin1String("QQuaternion(%1, %2, %3, %4)")).arg(quaternion.scalar()).arg(quaternion.x()).arg(quaternion.y()).arg(quaternion.z());
}

qreal QQmlQuaternionValueType::scalar() const
{
    return quaternion.scalar();
}

qreal QQmlQuaternionValueType::x() const
{
    return quaternion.x();
}

qreal QQmlQuaternionValueType::y() const
{
    return quaternion.y();
}

qreal QQmlQuaternionValueType::z() const
{
    return quaternion.z();
}

void QQmlQuaternionValueType::setScalar(qreal scalar)
{
    quaternion.setScalar(scalar);
}

void QQmlQuaternionValueType::setX(qreal x)
{
    quaternion.setX(x);
}

void QQmlQuaternionValueType::setY(qreal y)
{
    quaternion.setY(y);
}

void QQmlQuaternionValueType::setZ(qreal z)
{
    quaternion.setZ(z);
}

QString QQmlMatrix4x4ValueType::toString() const
{
    return QString(QLatin1String("QMatrix4x4(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16)"))
            .arg(matrix(0, 0)).arg(matrix(0, 1)).arg(matrix(0, 2)).arg(matrix(0, 3))
            .arg(matrix(1, 0)).arg(matrix(1, 1)).arg(matrix(1, 2)).arg(matrix(1, 3))
            .arg(matrix(2, 0)).arg(matrix(2, 1)).arg(matrix(2, 2)).arg(matrix(2, 3))
            .arg(matrix(3, 0)).arg(matrix(3, 1)).arg(matrix(3, 2)).arg(matrix(3, 3));
}

QString QQmlEasingValueType::toString() const
{
    return QString(QLatin1String("QEasingCurve(%1, %2, %3, %4)")).arg(easing.type()).arg(easing.amplitude()).arg(easing.overshoot()).arg(easing.period());
}

QQmlEasingValueType::Type QQmlEasingValueType::type() const
{
    return (QQmlEasingValueType::Type)easing.type();
}

qreal QQmlEasingValueType::amplitude() const
{
    return easing.amplitude();
}

qreal QQmlEasingValueType::overshoot() const
{
    return easing.overshoot();
}

qreal QQmlEasingValueType::period() const
{
    return easing.period();
}

void QQmlEasingValueType::setType(QQmlEasingValueType::Type type)
{
    easing.setType((QEasingCurve::Type)type);
}

void QQmlEasingValueType::setAmplitude(qreal amplitude)
{
    easing.setAmplitude(amplitude);
}

void QQmlEasingValueType::setOvershoot(qreal overshoot)
{
    easing.setOvershoot(overshoot);
}

void QQmlEasingValueType::setPeriod(qreal period)
{
    easing.setPeriod(period);
}

void QQmlEasingValueType::setBezierCurve(const QVariantList &customCurveVariant)
{
    if (customCurveVariant.isEmpty())
        return;

    QVariantList variantList = customCurveVariant;
    if ((variantList.count() % 6) == 0) {
        bool allRealsOk = true;
        QList<qreal> reals;
        for (int i = 0; i < variantList.count(); i++) {
            bool ok;
            const qreal real = variantList.at(i).toReal(&ok);
            reals.append(real);
            if (!ok)
                allRealsOk = false;
        }
        if (allRealsOk) {
            QEasingCurve newEasingCurve(QEasingCurve::BezierSpline);
            for (int i = 0; i < reals.count() / 6; i++) {
                const qreal c1x = reals.at(i * 6);
                const qreal c1y = reals.at(i * 6 + 1);
                const qreal c2x = reals.at(i * 6 + 2);
                const qreal c2y = reals.at(i * 6 + 3);
                const qreal c3x = reals.at(i * 6 + 4);
                const qreal c3y = reals.at(i * 6 + 5);

                const QPointF c1(c1x, c1y);
                const QPointF c2(c2x, c2y);
                const QPointF c3(c3x, c3y);

                newEasingCurve.addCubicBezierSegment(c1, c2, c3);
                easing = newEasingCurve;
            }
        }
    }
}

QVariantList QQmlEasingValueType::bezierCurve() const
{
    QVariantList rv;
    QList<QPointF> points = easing.cubicBezierSpline();
    for (int ii = 0; ii < points.count(); ++ii)
        rv << QVariant(points.at(ii).x()) << QVariant(points.at(ii).y());
    return rv;
}

void QQmlFontValueType::onLoad()
{
    pixelSizeSet = false;
    pointSizeSet = false;
}

QString QQmlFontValueType::toString() const
{
    return QString(QLatin1String("QFont(%1)")).arg(font.toString());
}

QString QQmlFontValueType::family() const
{
    return font.family();
}

void QQmlFontValueType::setFamily(const QString &family)
{
    font.setFamily(family);
}

bool QQmlFontValueType::bold() const
{
    return font.bold();
}

void QQmlFontValueType::setBold(bool b)
{
    font.setBold(b);
}

QQmlFontValueType::FontWeight QQmlFontValueType::weight() const
{
    return (QQmlFontValueType::FontWeight)font.weight();
}

void QQmlFontValueType::setWeight(QQmlFontValueType::FontWeight w)
{
    font.setWeight((QFont::Weight)w);
}

bool QQmlFontValueType::italic() const
{
    return font.italic();
}

void QQmlFontValueType::setItalic(bool b)
{
    font.setItalic(b);
}

bool QQmlFontValueType::underline() const
{
    return font.underline();
}

void QQmlFontValueType::setUnderline(bool b)
{
    font.setUnderline(b);
}

bool QQmlFontValueType::overline() const
{
    return font.overline();
}

void QQmlFontValueType::setOverline(bool b)
{
    font.setOverline(b);
}

bool QQmlFontValueType::strikeout() const
{
    return font.strikeOut();
}

void QQmlFontValueType::setStrikeout(bool b)
{
    font.setStrikeOut(b);
}

qreal QQmlFontValueType::pointSize() const
{
    if (font.pointSizeF() == -1) {
        if (dpi.isNull)
            dpi = qt_defaultDpi();
        return font.pixelSize() * qreal(72.) / qreal(dpi);
    }
    return font.pointSizeF();
}

void QQmlFontValueType::setPointSize(qreal size)
{
    if (pixelSizeSet) {
        qWarning() << "Both point size and pixel size set. Using pixel size.";
        return;
    }

    if (size >= 0.0) {
        pointSizeSet = true;
        font.setPointSizeF(size);
    } else {
        pointSizeSet = false;
    }
}

int QQmlFontValueType::pixelSize() const
{
    if (font.pixelSize() == -1) {
        if (dpi.isNull)
            dpi = qt_defaultDpi();
        return (font.pointSizeF() * dpi) / qreal(72.);
    }
    return font.pixelSize();
}

void QQmlFontValueType::setPixelSize(int size)
{
    if (size >0) {
        if (pointSizeSet)
            qWarning() << "Both point size and pixel size set. Using pixel size.";
        font.setPixelSize(size);
        pixelSizeSet = true;
    } else {
        pixelSizeSet = false;
    }
}

QQmlFontValueType::Capitalization QQmlFontValueType::capitalization() const
{
    return (QQmlFontValueType::Capitalization)font.capitalization();
}

void QQmlFontValueType::setCapitalization(QQmlFontValueType::Capitalization c)
{
    font.setCapitalization((QFont::Capitalization)c);
}

qreal QQmlFontValueType::letterSpacing() const
{
    return font.letterSpacing();
}

void QQmlFontValueType::setLetterSpacing(qreal size)
{
    font.setLetterSpacing(QFont::AbsoluteSpacing, size);
}

qreal QQmlFontValueType::wordSpacing() const
{
    return font.wordSpacing();
}

void QQmlFontValueType::setWordSpacing(qreal size)
{
    font.setWordSpacing(size);
}

QString QQmlColorValueType::toString() const
{
    // special case - to maintain behaviour with QtQuick 1.0, we just output normal toString() value.
    return QVariant(color).toString();
}

qreal QQmlColorValueType::r() const
{
    return color.redF();
}

qreal QQmlColorValueType::g() const
{
    return color.greenF();
}

qreal QQmlColorValueType::b() const
{
    return color.blueF();
}

qreal QQmlColorValueType::a() const
{
    return color.alphaF();
}

void QQmlColorValueType::setR(qreal r)
{
    color.setRedF(r);
}

void QQmlColorValueType::setG(qreal g)
{
    color.setGreenF(g);
}

void QQmlColorValueType::setB(qreal b)
{
    color.setBlueF(b);
}

void QQmlColorValueType::setA(qreal a)
{
    color.setAlphaF(a);
}

QT_END_NAMESPACE
