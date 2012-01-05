/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qdeclarativevaluetype_p.h"

#include "qdeclarativemetatype_p.h"
#include <private/qfont_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

template<typename T>
int qmlRegisterValueTypeEnums(const char *uri, int versionMajor, int versionMinor, const char *qmlName)
{
    QByteArray name(T::staticMetaObject.className());

    QByteArray pointerName(name + '*');

    QDeclarativePrivate::RegisterType type = {
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

    return QDeclarativePrivate::qmlregister(QDeclarativePrivate::TypeRegistration, &type);
}

QDeclarativeValueTypeFactory::QDeclarativeValueTypeFactory()
{
    // ### Optimize
    for (unsigned int ii = 0; ii < (QVariant::UserType - 1); ++ii)
        valueTypes[ii] = valueType(ii);
}

QDeclarativeValueTypeFactory::~QDeclarativeValueTypeFactory()
{
    for (unsigned int ii = 0; ii < (QVariant::UserType - 1); ++ii)
        delete valueTypes[ii];
}

bool QDeclarativeValueTypeFactory::isValueType(int idx)
{
    if ((uint)idx < QVariant::UserType && (uint)idx != QVariant::StringList)
        return true;
    return false;
}

void QDeclarativeValueTypeFactory::registerBaseTypes(const char *uri, int versionMajor, int versionMinor)
{
    qmlRegisterValueTypeEnums<QDeclarativeEasingValueType>(uri, versionMajor, versionMinor, "Easing");
    qmlRegisterValueTypeEnums<QDeclarativeFontValueType>(uri, versionMajor, versionMinor, "Font");
}

void QDeclarativeValueTypeFactory::registerValueTypes()
{
    registerBaseTypes("QtQuick", 2, 0);
}

QDeclarativeValueType *QDeclarativeValueTypeFactory::valueType(int t)
{
    QDeclarativeValueType *rv = 0;

    switch (t) {
    case QVariant::Point:
        rv = new QDeclarativePointValueType;
        break;
    case QVariant::PointF:
        rv = new QDeclarativePointFValueType;
        break;
    case QVariant::Size:
        rv = new QDeclarativeSizeValueType;
        break;
    case QVariant::SizeF:
        rv = new QDeclarativeSizeFValueType;
        break;
    case QVariant::Rect:
        rv = new QDeclarativeRectValueType;
        break;
    case QVariant::RectF:
        rv = new QDeclarativeRectFValueType;
        break;
    case QVariant::Vector2D:
        rv = new QDeclarativeVector2DValueType;
        break;
    case QVariant::Vector3D:
        rv = new QDeclarativeVector3DValueType;
        break;
    case QVariant::Vector4D:
        rv = new QDeclarativeVector4DValueType;
        break;
    case QVariant::Quaternion:
        rv = new QDeclarativeQuaternionValueType;
        break;
    case QVariant::Matrix4x4:
        rv = new QDeclarativeMatrix4x4ValueType;
        break;
    case QVariant::EasingCurve:
        rv = new QDeclarativeEasingValueType;
        break;
    case QVariant::Font:
        rv = new QDeclarativeFontValueType;
        break;
    case QVariant::Color:
        rv = new QDeclarativeColorValueType;
        break;
    default:
        break;
    }

    Q_ASSERT(!rv || rv->metaObject()->propertyCount() < 32);
    return rv;
}

QDeclarativeValueType::QDeclarativeValueType(QObject *parent)
: QObject(parent)
{
}

#define QML_VALUETYPE_READWRITE(name, cpptype, var) \
    QDeclarative ## name ## ValueType::QDeclarative ## name ## ValueType(QObject *parent) \
    : QDeclarativeValueType(parent) \
    { \
    } \
    void QDeclarative ## name ## ValueType::read(QObject *obj, int idx) \
    { \
        void *a[] = { &var, 0 }; \
        QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a); \
        onLoad(); \
    } \
    void QDeclarative ## name ## ValueType::write(QObject *obj, int idx, \
                                                  QDeclarativePropertyPrivate::WriteFlags flags) \
    { \
        int status = -1; \
        void *a[] = { &var, 0, &status, &flags }; \
        QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a); \
    } \
    bool QDeclarative ## name ## ValueType::isEqual(const QVariant &value) const \
    { \
        return QVariant(var) == value; \
    } \
    QVariant QDeclarative ## name ## ValueType::value() \
    { \
        return QVariant(var); \
    } \
    void QDeclarative ## name ## ValueType::setValue(const QVariant &value) \
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

QString QDeclarativePointFValueType::toString() const
{
    return QString(QLatin1String("QPointF(%1, %2)")).arg(point.x()).arg(point.y());
}

qreal QDeclarativePointFValueType::x() const
{
    return point.x();
}

qreal QDeclarativePointFValueType::y() const
{
    return point.y();
}

void QDeclarativePointFValueType::setX(qreal x)
{
    point.setX(x);
}

void QDeclarativePointFValueType::setY(qreal y)
{
    point.setY(y);
}

QString QDeclarativePointValueType::toString() const
{
    return QString(QLatin1String("QPoint(%1, %2)")).arg(point.x()).arg(point.y());
}

int QDeclarativePointValueType::x() const
{
    return point.x();
}

int QDeclarativePointValueType::y() const
{
    return point.y();
}

void QDeclarativePointValueType::setX(int x)
{
    point.setX(x);
}

void QDeclarativePointValueType::setY(int y)
{
    point.setY(y);
}

QString QDeclarativeSizeFValueType::toString() const
{
    return QString(QLatin1String("QSizeF(%1, %2)")).arg(size.width()).arg(size.height());
}

qreal QDeclarativeSizeFValueType::width() const
{
    return size.width();
}

qreal QDeclarativeSizeFValueType::height() const
{
    return size.height();
}

void QDeclarativeSizeFValueType::setWidth(qreal w)
{
    size.setWidth(w);
}

void QDeclarativeSizeFValueType::setHeight(qreal h)
{
    size.setHeight(h);
}

QString QDeclarativeSizeValueType::toString() const
{
    return QString(QLatin1String("QSize(%1, %2)")).arg(size.width()).arg(size.height());
}

int QDeclarativeSizeValueType::width() const
{
    return size.width();
}

int QDeclarativeSizeValueType::height() const
{
    return size.height();
}

void QDeclarativeSizeValueType::setWidth(int w)
{
    size.setWidth(w);
}

void QDeclarativeSizeValueType::setHeight(int h)
{
    size.setHeight(h);
}

QString QDeclarativeRectFValueType::toString() const
{
    return QString(QLatin1String("QRectF(%1, %2, %3, %4)")).arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
}

qreal QDeclarativeRectFValueType::x() const
{
    return rect.x();
}

qreal QDeclarativeRectFValueType::y() const
{
    return rect.y();
}

void QDeclarativeRectFValueType::setX(qreal x)
{
    rect.moveLeft(x);
}

void QDeclarativeRectFValueType::setY(qreal y)
{
    rect.moveTop(y);
}

qreal QDeclarativeRectFValueType::width() const
{
    return rect.width();
}

qreal QDeclarativeRectFValueType::height() const
{
    return rect.height();
}

void QDeclarativeRectFValueType::setWidth(qreal w)
{
    rect.setWidth(w);
}

void QDeclarativeRectFValueType::setHeight(qreal h)
{
    rect.setHeight(h);
}

QString QDeclarativeRectValueType::toString() const
{
    return QString(QLatin1String("QRect(%1, %2, %3, %4)")).arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height());
}

int QDeclarativeRectValueType::x() const
{
    return rect.x();
}

int QDeclarativeRectValueType::y() const
{
    return rect.y();
}

void QDeclarativeRectValueType::setX(int x)
{
    rect.moveLeft(x);
}

void QDeclarativeRectValueType::setY(int y)
{
    rect.moveTop(y);
}

int QDeclarativeRectValueType::width() const
{
    return rect.width();
}

int QDeclarativeRectValueType::height() const
{
    return rect.height();
}

void QDeclarativeRectValueType::setWidth(int w)
{
    rect.setWidth(w);
}

void QDeclarativeRectValueType::setHeight(int h)
{
    rect.setHeight(h);
}

QString QDeclarativeVector2DValueType::toString() const
{
    return QString(QLatin1String("QVector2D(%1, %2)")).arg(vector.x()).arg(vector.y());
}

qreal QDeclarativeVector2DValueType::x() const
{
    return vector.x();
}

qreal QDeclarativeVector2DValueType::y() const
{
    return vector.y();
}

void QDeclarativeVector2DValueType::setX(qreal x)
{
    vector.setX(x);
}

void QDeclarativeVector2DValueType::setY(qreal y)
{
    vector.setY(y);
}

QString QDeclarativeVector3DValueType::toString() const
{
    return QString(QLatin1String("QVector3D(%1, %2, %3)")).arg(vector.x()).arg(vector.y()).arg(vector.z());
}

qreal QDeclarativeVector3DValueType::x() const
{
    return vector.x();
}

qreal QDeclarativeVector3DValueType::y() const
{
    return vector.y();
}

qreal QDeclarativeVector3DValueType::z() const
{
    return vector.z();
}

void QDeclarativeVector3DValueType::setX(qreal x)
{
    vector.setX(x);
}

void QDeclarativeVector3DValueType::setY(qreal y)
{
    vector.setY(y);
}

void QDeclarativeVector3DValueType::setZ(qreal z)
{
    vector.setZ(z);
}

QString QDeclarativeVector4DValueType::toString() const
{
    return QString(QLatin1String("QVector4D(%1, %2, %3, %4)")).arg(vector.x()).arg(vector.y()).arg(vector.z()).arg(vector.w());
}

qreal QDeclarativeVector4DValueType::x() const
{
    return vector.x();
}

qreal QDeclarativeVector4DValueType::y() const
{
    return vector.y();
}

qreal QDeclarativeVector4DValueType::z() const
{
    return vector.z();
}

qreal QDeclarativeVector4DValueType::w() const
{
    return vector.w();
}

void QDeclarativeVector4DValueType::setX(qreal x)
{
    vector.setX(x);
}

void QDeclarativeVector4DValueType::setY(qreal y)
{
    vector.setY(y);
}

void QDeclarativeVector4DValueType::setZ(qreal z)
{
    vector.setZ(z);
}

void QDeclarativeVector4DValueType::setW(qreal w)
{
    vector.setW(w);
}

QString QDeclarativeQuaternionValueType::toString() const
{
    return QString(QLatin1String("QQuaternion(%1, %2, %3, %4)")).arg(quaternion.scalar()).arg(quaternion.x()).arg(quaternion.y()).arg(quaternion.z());
}

qreal QDeclarativeQuaternionValueType::scalar() const
{
    return quaternion.scalar();
}

qreal QDeclarativeQuaternionValueType::x() const
{
    return quaternion.x();
}

qreal QDeclarativeQuaternionValueType::y() const
{
    return quaternion.y();
}

qreal QDeclarativeQuaternionValueType::z() const
{
    return quaternion.z();
}

void QDeclarativeQuaternionValueType::setScalar(qreal scalar)
{
    quaternion.setScalar(scalar);
}

void QDeclarativeQuaternionValueType::setX(qreal x)
{
    quaternion.setX(x);
}

void QDeclarativeQuaternionValueType::setY(qreal y)
{
    quaternion.setY(y);
}

void QDeclarativeQuaternionValueType::setZ(qreal z)
{
    quaternion.setZ(z);
}

QString QDeclarativeMatrix4x4ValueType::toString() const
{
    return QString(QLatin1String("QMatrix4x4(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16)"))
            .arg(matrix(0, 0)).arg(matrix(0, 1)).arg(matrix(0, 2)).arg(matrix(0, 3))
            .arg(matrix(1, 0)).arg(matrix(1, 1)).arg(matrix(1, 2)).arg(matrix(1, 3))
            .arg(matrix(2, 0)).arg(matrix(2, 1)).arg(matrix(2, 2)).arg(matrix(2, 3))
            .arg(matrix(3, 0)).arg(matrix(3, 1)).arg(matrix(3, 2)).arg(matrix(3, 3));
}

QString QDeclarativeEasingValueType::toString() const
{
    return QString(QLatin1String("QEasingCurve(%1, %2, %3, %4)")).arg(easing.type()).arg(easing.amplitude()).arg(easing.overshoot()).arg(easing.period());
}

QDeclarativeEasingValueType::Type QDeclarativeEasingValueType::type() const
{
    return (QDeclarativeEasingValueType::Type)easing.type();
}

qreal QDeclarativeEasingValueType::amplitude() const
{
    return easing.amplitude();
}

qreal QDeclarativeEasingValueType::overshoot() const
{
    return easing.overshoot();
}

qreal QDeclarativeEasingValueType::period() const
{
    return easing.period();
}

void QDeclarativeEasingValueType::setType(QDeclarativeEasingValueType::Type type)
{
    easing.setType((QEasingCurve::Type)type);
}

void QDeclarativeEasingValueType::setAmplitude(qreal amplitude)
{
    easing.setAmplitude(amplitude);
}

void QDeclarativeEasingValueType::setOvershoot(qreal overshoot)
{
    easing.setOvershoot(overshoot);
}

void QDeclarativeEasingValueType::setPeriod(qreal period)
{
    easing.setPeriod(period);
}

void QDeclarativeEasingValueType::setBezierCurve(const QVariantList &customCurveVariant)
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

QVariantList QDeclarativeEasingValueType::bezierCurve() const
{
    QVariantList rv;
    QList<QPointF> points = easing.cubicBezierSpline();
    for (int ii = 0; ii < points.count(); ++ii)
        rv << QVariant(points.at(ii).x()) << QVariant(points.at(ii).y());
    return rv;
}

void QDeclarativeFontValueType::onLoad()
{
    pixelSizeSet = false;
    pointSizeSet = false;
}

QString QDeclarativeFontValueType::toString() const
{
    return QString(QLatin1String("QFont(%1)")).arg(font.toString());
}

QString QDeclarativeFontValueType::family() const
{
    return font.family();
}

void QDeclarativeFontValueType::setFamily(const QString &family)
{
    font.setFamily(family);
}

bool QDeclarativeFontValueType::bold() const
{
    return font.bold();
}

void QDeclarativeFontValueType::setBold(bool b)
{
    font.setBold(b);
}

QDeclarativeFontValueType::FontWeight QDeclarativeFontValueType::weight() const
{
    return (QDeclarativeFontValueType::FontWeight)font.weight();
}

void QDeclarativeFontValueType::setWeight(QDeclarativeFontValueType::FontWeight w)
{
    font.setWeight((QFont::Weight)w);
}

bool QDeclarativeFontValueType::italic() const
{
    return font.italic();
}

void QDeclarativeFontValueType::setItalic(bool b)
{
    font.setItalic(b);
}

bool QDeclarativeFontValueType::underline() const
{
    return font.underline();
}

void QDeclarativeFontValueType::setUnderline(bool b)
{
    font.setUnderline(b);
}

bool QDeclarativeFontValueType::overline() const
{
    return font.overline();
}

void QDeclarativeFontValueType::setOverline(bool b)
{
    font.setOverline(b);
}

bool QDeclarativeFontValueType::strikeout() const
{
    return font.strikeOut();
}

void QDeclarativeFontValueType::setStrikeout(bool b)
{
    font.setStrikeOut(b);
}

qreal QDeclarativeFontValueType::pointSize() const
{
    if (font.pointSizeF() == -1) {
        if (dpi.isNull)
            dpi = qt_defaultDpi();
        return font.pixelSize() * qreal(72.) / qreal(dpi);
    }
    return font.pointSizeF();
}

void QDeclarativeFontValueType::setPointSize(qreal size)
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

int QDeclarativeFontValueType::pixelSize() const
{
    if (font.pixelSize() == -1) {
        if (dpi.isNull)
            dpi = qt_defaultDpi();
        return (font.pointSizeF() * dpi) / qreal(72.);
    }
    return font.pixelSize();
}

void QDeclarativeFontValueType::setPixelSize(int size)
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

QDeclarativeFontValueType::Capitalization QDeclarativeFontValueType::capitalization() const
{
    return (QDeclarativeFontValueType::Capitalization)font.capitalization();
}

void QDeclarativeFontValueType::setCapitalization(QDeclarativeFontValueType::Capitalization c)
{
    font.setCapitalization((QFont::Capitalization)c);
}

qreal QDeclarativeFontValueType::letterSpacing() const
{
    return font.letterSpacing();
}

void QDeclarativeFontValueType::setLetterSpacing(qreal size)
{
    font.setLetterSpacing(QFont::AbsoluteSpacing, size);
}

qreal QDeclarativeFontValueType::wordSpacing() const
{
    return font.wordSpacing();
}

void QDeclarativeFontValueType::setWordSpacing(qreal size)
{
    font.setWordSpacing(size);
}

QString QDeclarativeColorValueType::toString() const
{
    // special case - to maintain behaviour with QtQuick 1.0, we just output normal toString() value.
    return QVariant(color).toString();
}

qreal QDeclarativeColorValueType::r() const
{
    return color.redF();
}

qreal QDeclarativeColorValueType::g() const
{
    return color.greenF();
}

qreal QDeclarativeColorValueType::b() const
{
    return color.blueF();
}

qreal QDeclarativeColorValueType::a() const
{
    return color.alphaF();
}

void QDeclarativeColorValueType::setR(qreal r)
{
    color.setRedF(r);
}

void QDeclarativeColorValueType::setG(qreal g)
{
    color.setGreenF(g);
}

void QDeclarativeColorValueType::setB(qreal b)
{
    color.setBlueF(b);
}

void QDeclarativeColorValueType::setA(qreal a)
{
    color.setAlphaF(a);
}

QT_END_NAMESPACE
