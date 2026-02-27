// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <private/qquickvaluetypes_p.h>

#include <qtquickglobal.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qqmlstringconverters_p.h>
#include <private/qcolorspace_p.h>
#include <private/qfont_p.h>

QT_BEGIN_NAMESPACE

QQuickColorValueType::QQuickColorValueType(const QString &string)
    : QColor(QColor::fromString(string))
{
}

QVariant QQuickColorValueType::create(const QJSValue &params)
{
    return params.isString() ? QColor::fromString(params.toString()) : QVariant();
}

QString QQuickColorValueType::toString() const
{
    return QColor::name(QColor::alpha() != 255 ? QColor::HexArgb : QColor::HexRgb);
}

QColor QQuickColorValueType::lighter(qreal factor) const
{
    return QColor::lighter(int(qRound(factor*100.)));
}

QColor QQuickColorValueType::darker(qreal factor) const
{
    return QColor::darker(int(qRound(factor*100.)));
}

QColor QQuickColorValueType::alpha(qreal value) const
{
    QColor color = *this;
    color.setAlphaF(value);
    return color;
}

QColor QQuickColorValueType::tint(const QColor &tintColor) const
{
    int tintAlpha = tintColor.alpha();
    if (tintAlpha == 0xFF)
        return tintColor;
    else if (tintAlpha == 0x00)
        return *this;

    // tint the base color and return the final color
    const QColor baseColor = QColor::toRgb();
    const qreal a = tintColor.alphaF();
    const qreal inv_a = 1.0 - a;

    const qreal r = tintColor.redF() * a + baseColor.redF() * inv_a;
    const qreal g = tintColor.greenF() * a + baseColor.greenF() * inv_a;
    const qreal b = tintColor.blueF() * a + baseColor.blueF() * inv_a;

    return QColor::fromRgbF(r, g, b, a + inv_a * baseColor.alphaF());
}

qreal QQuickColorValueType::r() const
{
    return QColor::redF();
}

qreal QQuickColorValueType::g() const
{
    return QColor::greenF();
}

qreal QQuickColorValueType::b() const
{
    return QColor::blueF();
}

qreal QQuickColorValueType::a() const
{
    return QColor::alphaF();
}

qreal QQuickColorValueType::hsvHue() const
{
    return QColor::hsvHueF();
}

qreal QQuickColorValueType::hsvSaturation() const
{
    return QColor::hsvSaturationF();
}

qreal QQuickColorValueType::hsvValue() const
{
    return QColor::valueF();
}

qreal QQuickColorValueType::hslHue() const
{
    return QColor::hslHueF();
}

qreal QQuickColorValueType::hslSaturation() const
{
    return QColor::hslSaturationF();
}

qreal QQuickColorValueType::hslLightness() const
{
    return QColor::lightnessF();
}

bool QQuickColorValueType::isValid() const
{
    return QColor::isValid();
}

void QQuickColorValueType::setR(qreal r)
{
    QColor::setRedF(r);
}

void QQuickColorValueType::setG(qreal g)
{
    QColor::setGreenF(g);
}

void QQuickColorValueType::setB(qreal b)
{
    QColor::setBlueF(b);
}

void QQuickColorValueType::setA(qreal a)
{
    QColor::setAlphaF(a);
}

void QQuickColorValueType::setHsvHue(qreal hsvHue)
{
    float hue, saturation, value, alpha;
    QColor::getHsvF(&hue, &saturation, &value, &alpha);
    QColor::setHsvF(hsvHue, saturation, value, alpha);
}

void QQuickColorValueType::setHsvSaturation(qreal hsvSaturation)
{
    float hue, saturation, value, alpha;
    QColor::getHsvF(&hue, &saturation, &value, &alpha);
    QColor::setHsvF(hue, hsvSaturation, value, alpha);
}

void QQuickColorValueType::setHsvValue(qreal hsvValue)
{
    float hue, saturation, value, alpha;
    QColor::getHsvF(&hue, &saturation, &value, &alpha);
    QColor::setHsvF(hue, saturation, hsvValue, alpha);
}

void QQuickColorValueType::setHslHue(qreal hslHue)
{
    float hue, saturation, lightness, alpha;
    QColor::getHslF(&hue, &saturation, &lightness, &alpha);
    QColor::setHslF(hslHue, saturation, lightness, alpha);
}

void QQuickColorValueType::setHslSaturation(qreal hslSaturation)
{
    float hue, saturation, lightness, alpha;
    QColor::getHslF(&hue, &saturation, &lightness, &alpha);
    QColor::setHslF(hue, hslSaturation, lightness, alpha);
}

void QQuickColorValueType::setHslLightness(qreal hslLightness)
{
    float hue, saturation, lightness, alpha;
    QColor::getHslF(&hue, &saturation, &lightness, &alpha);
    QColor::setHslF(hue, saturation, hslLightness, alpha);
}

QVariant QQuickVector2DValueType::create(const QJSValue &params)
{
    if (params.isString())
        return QQmlStringConverters::valueTypeFromNumberString<QVector2D, 2, u','>(params.toString());
    if (params.isArray())
        return QVector2D(params.property(0).toNumber(), params.property(1).toNumber());
    return QVariant();
}

QString QQuickVector2DValueType::toString() const
{
    return QString::fromLatin1("QVector2D(%1, %2)").arg(QVector2D::x()).arg(QVector2D::y());
}

qreal QQuickVector2DValueType::x() const
{
    return QVector2D::x();
}

qreal QQuickVector2DValueType::y() const
{
    return QVector2D::y();
}

void QQuickVector2DValueType::setX(qreal x)
{
    QVector2D::setX(x);
}

void QQuickVector2DValueType::setY(qreal y)
{
    QVector2D::setY(y);
}

qreal QQuickVector2DValueType::dotProduct(const QVector2D &vec) const
{
    return QVector2D::dotProduct(*this, vec);
}

QVector2D QQuickVector2DValueType::times(const QVector2D &vec) const
{
    return *this * vec;
}

QVector2D QQuickVector2DValueType::times(qreal scalar) const
{
    return *this * scalar;
}

QVector2D QQuickVector2DValueType::plus(const QVector2D &vec) const
{
    return *this + vec;
}

QVector2D QQuickVector2DValueType::minus(const QVector2D &vec) const
{
    return *this - vec;
}

QVector2D QQuickVector2DValueType::normalized() const
{
    return QVector2D::normalized();
}

qreal QQuickVector2DValueType::length() const
{
    return QVector2D::length();
}

QVector3D QQuickVector2DValueType::toVector3d() const
{
    return QVector2D::toVector3D();
}

QVector4D QQuickVector2DValueType::toVector4d() const
{
    return QVector2D::toVector4D();
}

bool QQuickVector2DValueType::fuzzyEquals(const QVector2D &vec, qreal epsilon) const
{
    qreal absEps = qAbs(epsilon);
    if (qAbs(QVector2D::x() - vec.x()) > absEps)
        return false;
    if (qAbs(QVector2D::y() - vec.y()) > absEps)
        return false;
    return true;
}

bool QQuickVector2DValueType::fuzzyEquals(const QVector2D &vec) const
{
    return qFuzzyCompare(*this, vec);
}

QVariant QQuickVector3DValueType::create(const QJSValue &params)
{
    if (params.isString()) {
        return QQmlStringConverters::valueTypeFromNumberString<QVector3D, 3, u',', u','>(
                params.toString());
    }

    if (params.isArray()) {
        return QVector3D(params.property(0).toNumber(), params.property(1).toNumber(),
                         params.property(2).toNumber());
    }
    return QVariant();
}

QString QQuickVector3DValueType::toString() const
{
    return QString::fromLatin1("QVector3D(%1, %2, %3)")
            .arg(QVector3D::x()).arg(QVector3D::y()).arg(QVector3D::z());
}

qreal QQuickVector3DValueType::x() const
{
    return QVector3D::x();
}

qreal QQuickVector3DValueType::y() const
{
    return QVector3D::y();
}

qreal QQuickVector3DValueType::z() const
{
    return QVector3D::z();
}

void QQuickVector3DValueType::setX(qreal x)
{
    QVector3D::setX(x);
}

void QQuickVector3DValueType::setY(qreal y)
{
    QVector3D::setY(y);
}

void QQuickVector3DValueType::setZ(qreal z)
{
    QVector3D::setZ(z);
}

QVector3D QQuickVector3DValueType::crossProduct(const QVector3D &vec) const
{
    return QVector3D::crossProduct(*this, vec);
}

qreal QQuickVector3DValueType::dotProduct(const QVector3D &vec) const
{
    return QVector3D::dotProduct(*this, vec);
}

QVector3D QQuickVector3DValueType::times(const QMatrix4x4 &m) const
{
    return (QVector4D(*this, 1) * m).toVector3DAffine();
}

QVector3D QQuickVector3DValueType::times(const QVector3D &vec) const
{
    return *this * vec;
}

QVector3D QQuickVector3DValueType::times(qreal scalar) const
{
    return *this * scalar;
}

QVector3D QQuickVector3DValueType::plus(const QVector3D &vec) const
{
    return *this + vec;
}

QVector3D QQuickVector3DValueType::minus(const QVector3D &vec) const
{
    return *this - vec;
}

QVector3D QQuickVector3DValueType::normalized() const
{
    return QVector3D::normalized();
}

qreal QQuickVector3DValueType::length() const
{
    return QVector3D::length();
}

QVector2D QQuickVector3DValueType::toVector2d() const
{
    return QVector3D::toVector2D();
}

QVector4D QQuickVector3DValueType::toVector4d() const
{
    return QVector3D::toVector4D();
}

bool QQuickVector3DValueType::fuzzyEquals(const QVector3D &vec, qreal epsilon) const
{
    qreal absEps = qAbs(epsilon);
    if (qAbs(QVector3D::x() - vec.x()) > absEps)
        return false;
    if (qAbs(QVector3D::y() - vec.y()) > absEps)
        return false;
    if (qAbs(QVector3D::z() - vec.z()) > absEps)
        return false;
    return true;
}

bool QQuickVector3DValueType::fuzzyEquals(const QVector3D &vec) const
{
    return qFuzzyCompare(*this, vec);
}

QVariant QQuickVector4DValueType::create(const QJSValue &params)
{
    if (params.isString()) {
        return QQmlStringConverters::valueTypeFromNumberString<QVector4D, 4, u',', u',', u','>(
                params.toString());
    }

    if (params.isArray()) {
        return QVector4D(params.property(0).toNumber(), params.property(1).toNumber(),
                         params.property(2).toNumber(), params.property(3).toNumber());
    }

    return QVariant();
}

QString QQuickVector4DValueType::toString() const
{
    return QString::fromLatin1("QVector4D(%1, %2, %3, %4)")
            .arg(QVector4D::x()).arg(QVector4D::y()).arg(QVector4D::z()).arg(QVector4D::w());
}

qreal QQuickVector4DValueType::x() const
{
    return QVector4D::x();
}

qreal QQuickVector4DValueType::y() const
{
    return QVector4D::y();
}

qreal QQuickVector4DValueType::z() const
{
    return QVector4D::z();
}

qreal QQuickVector4DValueType::w() const
{
    return QVector4D::w();
}

void QQuickVector4DValueType::setX(qreal x)
{
    QVector4D::setX(x);
}

void QQuickVector4DValueType::setY(qreal y)
{
    QVector4D::setY(y);
}

void QQuickVector4DValueType::setZ(qreal z)
{
    QVector4D::setZ(z);
}

void QQuickVector4DValueType::setW(qreal w)
{
    QVector4D::setW(w);
}

qreal QQuickVector4DValueType::dotProduct(const QVector4D &vec) const
{
    return QVector4D::dotProduct(*this, vec);
}

QVector4D QQuickVector4DValueType::times(const QVector4D &vec) const
{
    return *this * vec;
}

QVector4D QQuickVector4DValueType::times(const QMatrix4x4 &m) const
{
    return *this * m;
}

QVector4D QQuickVector4DValueType::times(qreal scalar) const
{
    return *this * scalar;
}

QVector4D QQuickVector4DValueType::plus(const QVector4D &vec) const
{
    return *this + vec;
}

QVector4D QQuickVector4DValueType::minus(const QVector4D &vec) const
{
    return *this - vec;
}

QVector4D QQuickVector4DValueType::normalized() const
{
    return QVector4D::normalized();
}

qreal QQuickVector4DValueType::length() const
{
    return QVector4D::length();
}

QVector2D QQuickVector4DValueType::toVector2d() const
{
    return QVector4D::toVector2D();
}

QVector3D QQuickVector4DValueType::toVector3d() const
{
    return QVector4D::toVector3D();
}

bool QQuickVector4DValueType::fuzzyEquals(const QVector4D &vec, qreal epsilon) const
{
    qreal absEps = qAbs(epsilon);
    if (qAbs(QVector4D::x() - vec.x()) > absEps)
        return false;
    if (qAbs(QVector4D::y() - vec.y()) > absEps)
        return false;
    if (qAbs(QVector4D::z() - vec.z()) > absEps)
        return false;
    if (qAbs(QVector4D::w() - vec.w()) > absEps)
        return false;
    return true;
}

bool QQuickVector4DValueType::fuzzyEquals(const QVector4D &vec) const
{
    return qFuzzyCompare(*this, vec);
}

QVariant QQuickQuaternionValueType::create(const QJSValue &params)
{
    if (params.isString()) {
        return QQmlStringConverters::valueTypeFromNumberString<QQuaternion, 4, u',', u',', u','>(
                params.toString());
    }

    if (params.isArray()) {
        return QQuaternion(params.property(0).toNumber(), params.property(1).toNumber(),
                           params.property(2).toNumber(), params.property(3).toNumber());
    }

    return QVariant();
}

QString QQuickQuaternionValueType::toString() const
{
    return QString::fromLatin1("QQuaternion(%1, %2, %3, %4)")
            .arg(QQuaternion::scalar())
            .arg(QQuaternion::x())
            .arg(QQuaternion::y())
            .arg(QQuaternion::z());
}

qreal QQuickQuaternionValueType::scalar() const
{
    return QQuaternion::scalar();
}

qreal QQuickQuaternionValueType::x() const
{
    return QQuaternion::x();
}

qreal QQuickQuaternionValueType::y() const
{
    return QQuaternion::y();
}

qreal QQuickQuaternionValueType::z() const
{
    return QQuaternion::z();
}

void QQuickQuaternionValueType::setScalar(qreal scalar)
{
    QQuaternion::setScalar(scalar);
}

void QQuickQuaternionValueType::setX(qreal x)
{
    QQuaternion::setX(x);
}

void QQuickQuaternionValueType::setY(qreal y)
{
    QQuaternion::setY(y);
}

void QQuickQuaternionValueType::setZ(qreal z)
{
    QQuaternion::setZ(z);
}

qreal QQuickQuaternionValueType::dotProduct(const QQuaternion &q) const
{
    return QQuaternion::dotProduct(*this, q);
}

QQuaternion QQuickQuaternionValueType::times(const QQuaternion &q) const
{
    return *this * q;
}

QVector3D QQuickQuaternionValueType::times(const QVector3D &vec) const
{
    return *this * vec;
}

QQuaternion QQuickQuaternionValueType::times(qreal factor) const
{
    return *this * factor;
}

QQuaternion QQuickQuaternionValueType::plus(const QQuaternion &q) const
{
    return *this + q;
}

QQuaternion QQuickQuaternionValueType::minus(const QQuaternion &q) const
{
    return *this - q;
}

QQuaternion QQuickQuaternionValueType::normalized() const
{
    return QQuaternion::normalized();
}

QQuaternion QQuickQuaternionValueType::inverted() const
{
    return QQuaternion::inverted();
}

QQuaternion QQuickQuaternionValueType::conjugated() const
{
    return QQuaternion::conjugated();
}

qreal QQuickQuaternionValueType::length() const
{
    return QQuaternion::length();
}

QVector3D QQuickQuaternionValueType::toEulerAngles() const
{
    return QQuaternion::toEulerAngles();
}

QVector4D QQuickQuaternionValueType::toVector4d() const
{
    return QQuaternion::toVector4D();
}

bool QQuickQuaternionValueType::fuzzyEquals(const QQuaternion &q, qreal epsilon) const
{
    qreal absEps = qAbs(epsilon);
    if (qAbs(QQuaternion::scalar() - q.scalar()) > absEps)
        return false;
    if (qAbs(QQuaternion::x() - q.x()) > absEps)
        return false;
    if (qAbs(QQuaternion::y() - q.y()) > absEps)
        return false;
    if (qAbs(QQuaternion::z() - q.z()) > absEps)
        return false;
    return true;
}

bool QQuickQuaternionValueType::fuzzyEquals(const QQuaternion &q) const
{
    return qFuzzyCompare(*this, q);
}

QVariant QQuickMatrix4x4ValueType::create(const QJSValue &params)
{
    if (params.isNull() || params.isUndefined())
        return QMatrix4x4();

    if (params.isString()) {
        return QQmlStringConverters::valueTypeFromNumberString<QMatrix4x4, 16, u',', u',', u',',
                                                               u',', u',', u',', u',', u',', u',',
                                                               u',', u',', u',', u',', u',', u','>(
                params.toString());
    }

    if (params.isArray() && params.property(QStringLiteral("length")).toInt() == 16) {
        return QMatrix4x4(params.property(0).toNumber(),
                          params.property(1).toNumber(),
                          params.property(2).toNumber(),
                          params.property(3).toNumber(),
                          params.property(4).toNumber(),
                          params.property(5).toNumber(),
                          params.property(6).toNumber(),
                          params.property(7).toNumber(),
                          params.property(8).toNumber(),
                          params.property(9).toNumber(),
                          params.property(10).toNumber(),
                          params.property(11).toNumber(),
                          params.property(12).toNumber(),
                          params.property(13).toNumber(),
                          params.property(14).toNumber(),
                          params.property(15).toNumber());
    }

    return QVariant();
}

QMatrix4x4 QQuickMatrix4x4ValueType::times(const QMatrix4x4 &m) const
{
    return *this * m;
}

QVector4D QQuickMatrix4x4ValueType::times(const QVector4D &vec) const
{
    return *this * vec;
}

QVector3D QQuickMatrix4x4ValueType::times(const QVector3D &vec) const
{
    return QMatrix4x4::map(vec);
}

QMatrix4x4 QQuickMatrix4x4ValueType::times(qreal factor) const
{
    return *this * factor;
}

QMatrix4x4 QQuickMatrix4x4ValueType::plus(const QMatrix4x4 &m) const
{
    return *this + m;
}

QMatrix4x4 QQuickMatrix4x4ValueType::minus(const QMatrix4x4 &m) const
{
    return *this - m;
}

QVector4D QQuickMatrix4x4ValueType::row(int n) const
{
    return QMatrix4x4::row(n);
}

QVector4D QQuickMatrix4x4ValueType::column(int m) const
{
    return QMatrix4x4::column(m);
}

qreal QQuickMatrix4x4ValueType::determinant() const
{
    return QMatrix4x4::determinant();
}

QMatrix4x4 QQuickMatrix4x4ValueType::inverted() const
{
    return QMatrix4x4::inverted();
}

QMatrix4x4 QQuickMatrix4x4ValueType::transposed() const
{
    return QMatrix4x4::transposed();
}

QPointF QQuickMatrix4x4ValueType::map(const QPointF p) const
{
    return QMatrix4x4::map(p);
}

QRectF QQuickMatrix4x4ValueType::mapRect(const QRectF r) const
{
    return QMatrix4x4::mapRect(r);
}

bool QQuickMatrix4x4ValueType::fuzzyEquals(const QMatrix4x4 &m, qreal epsilon) const
{
    qreal absEps = qAbs(epsilon);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (qAbs((*this)(i,j) - m(i,j)) > absEps) {
                return false;
            }
        }
    }
    return true;
}

bool QQuickMatrix4x4ValueType::fuzzyEquals(const QMatrix4x4 &m) const
{
    return qFuzzyCompare(*this, m);
}

/*!
    \qmltype PlanarTransform
    \inqmlmodule QtQuick
    \since 6.8

    \brief Provides utility functions for matrix4x4 when used for 2D transforms.

    The \c PlanarTransform is a global object with utility functions.

    It is not instantiable; to use it, call the members of the global \c PlanarTransform object
    directly. For example:

    \qml
    Item {
        transform: Matrix4x4 { matrix: PlanarTransform.fromAffineMatrix(1, 0, 0.36, 1, -36, 0) }
    }
    \endqml
*/

QQuickPlanarTransform::QQuickPlanarTransform(QObject *parent)
    : QObject(parent)
{
}

/*!
    \qmlmethod matrix4x4 PlanarTransform::identity()

    Returns a matrix4x4 for the identity transform.

    This is equivalent to \l Qt::matrix4x4().
*/

QMatrix4x4 QQuickPlanarTransform::identity()
{
    return QMatrix4x4();
}

/*!
    \qmlmethod matrix4x4 PlanarTransform::fromAffineMatrix(real scaleX, real shearY,
                                                           real shearX, real scaleY,
                                                           real translateX, real translateY)

    Returns a matrix4x4 for an affine (non-projecting) 2D transform with the specified values.

    This method and its argument order correspond to SVG's \c matrix() function and the
    six-argument QTransform constructor. The result is this 4x4 matrix:

    \table
    \row \li \a scaleX \li \a shearX \li 0 \li \a translateX
    \row \li \a shearY \li \a scaleY \li 0 \li \a translateY
    \row \li 0 \li 0 \li 1 \li 0
    \row \li 0 \li 0 \li 0 \li 1
    \endtable
*/

QMatrix4x4 QQuickPlanarTransform::fromAffineMatrix(float scaleX, float shearY,
                                                   float shearX, float scaleY,
                                                   float translateX, float translateY)
{
    return QMatrix4x4(scaleX, shearX, 0, translateX,
                      shearY, scaleY, 0, translateY,
                      0, 0, 1, 0,
                      0, 0, 0, 1);
}

/*!
    \qmlmethod matrix4x4 PlanarTransform::fromTranslate(real translateX, real translateY)

    Returns a matrix4x4 for a 2D transform that translates by \a translateX horizontally and
    \a translateY vertically.
*/
QMatrix4x4 QQuickPlanarTransform::fromTranslate(float translateX, float translateY)
{
    QMatrix4x4 xf;
    xf.translate(translateX, translateY);
    return xf;
}

/*!
    \qmlmethod matrix4x4 PlanarTransform::fromScale(real scaleX, real scaleY, real originX, real originY)

    Returns a matrix4x4 for a 2D transform that scales by \a scaleX horizontally and \a scaleY
    vertically, centered at the point (\a originX, \a originY).

    \a originX and \a originY are optional and default to (0, 0).
*/
QMatrix4x4 QQuickPlanarTransform::fromScale(float scaleX, float scaleY, float originX, float originY)
{
    QMatrix4x4 xf;
    xf.translate(originX, originY);
    xf.scale(scaleX, scaleY);
    xf.translate(-originX, -originY);
    return xf;
}

/*!
    \qmlmethod matrix4x4 PlanarTransform::fromRotate(real angle, real originX, real originY)

    Returns a matrix4x4 for a 2D transform that rotates by \a angle degrees around the point (\a
    originX, \a originY).

    \a originX and \a originY are optional and default to (0, 0).
*/
QMatrix4x4 QQuickPlanarTransform::fromRotate(float angle, float originX, float originY)
{
    QMatrix4x4 xf;
    xf.translate(originX, originY);
    xf.rotate(angle, 0, 0, 1);
    xf.translate(-originX, -originY);
    return xf;
}

/*!
    \qmlmethod matrix4x4 PlanarTransform::fromShear(float shearX, float shearY, float originX, float originY)

    Returns a matrix4x4 for a 2D transform that shears by \a shearX horizontally and \a shearY
    vertically, centered at the point (\a originX, \a originY).

    \a originX and \a originY are optional and default to (0, 0).
*/
QMatrix4x4 QQuickPlanarTransform::fromShear(float shearX, float shearY, float originX, float originY)
{
    QMatrix4x4 xf;
    xf.translate(originX, originY);
    xf *= QMatrix4x4(1, shearX, 0, 0, shearY, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    xf.translate(-originX, -originY);
    return xf;
}

template<typename T>
void setFontProperty(QFont &font, void (QFont::*setter)(T value), QString name,
                     const QJSValue &params, bool *ok)
{
    const QJSValue value = params.property(name);

    if constexpr (std::is_same_v<T, bool>) {
        if (value.isBool()) {
            (font.*setter)(value.toBool());
            *ok = true;
        }
    } else if constexpr (std::is_same_v<
            typename std::remove_cv<typename std::remove_reference<T>::type>::type,
            QString>) {
        if (value.isString()) {
            (font.*setter)(value.toString());
            *ok = true;
        }
    } else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
        if (value.isNumber()) {
            (font.*setter)(T(value.toInt()));
            *ok = true;
        }
    } else if constexpr (std::is_floating_point_v<T>) {
        if (value.isNumber()) {
            (font.*setter)(value.toNumber());
            *ok = true;
        }
    }
}

QVariant QQuickFontValueType::create(const QJSValue &params)
{
    if (!params.isObject())
        return QVariant();

    bool ok = false;
    QFont ret;

    setFontProperty(ret, &QFont::setBold, QStringLiteral("bold"), params, &ok);
    setFontProperty(ret, &QFont::setCapitalization, QStringLiteral("capitalization"), params, &ok);
    setFontProperty(ret, &QFont::setFamily, QStringLiteral("family"), params, &ok);
    setFontProperty(ret, &QFont::setItalic, QStringLiteral("italic"), params, &ok);
    setFontProperty(ret, &QFont::setPixelSize, QStringLiteral("pixelSize"), params, &ok);
    setFontProperty(ret, &QFont::setPointSize, QStringLiteral("pointSize"), params, &ok);
    setFontProperty(ret, &QFont::setStrikeOut, QStringLiteral("strikeout"), params, &ok);
    setFontProperty(ret, &QFont::setStyleName, QStringLiteral("styleName"), params, &ok);
    setFontProperty(ret, &QFont::setUnderline, QStringLiteral("underline"), params, &ok);
    setFontProperty(ret, &QFont::setWeight, QStringLiteral("weight"), params, &ok);
    setFontProperty(ret, &QFont::setWordSpacing, QStringLiteral("wordSpacing"), params, &ok);
    setFontProperty(ret, &QFont::setHintingPreference, QStringLiteral("hintingPreference"), params, &ok);
    setFontProperty(ret, &QFont::setKerning, QStringLiteral("kerning"), params, &ok);

    {
        const QJSValue vlspac = params.property(QStringLiteral("letterSpacing"));
        if (vlspac.isNumber()) {
            ret.setLetterSpacing(QFont::AbsoluteSpacing, vlspac.toNumber());
            ok = true;
        }
    }

    {
        const QJSValue vshaping = params.property(QStringLiteral("preferShaping"));
        if (vshaping.isBool()) {
            const bool enable = vshaping.toBool();
            const QFont::StyleStrategy strategy = ret.styleStrategy();
            if (enable)
                ret.setStyleStrategy(QFont::StyleStrategy(strategy & ~QFont::PreferNoShaping));
            else
                ret.setStyleStrategy(QFont::StyleStrategy(strategy | QFont::PreferNoShaping));
            ok = true;
        }
    }

    {
        const QJSValue typoMetrics = params.property(QStringLiteral("preferTypoLineMetrics"));
        if (typoMetrics.isBool()) {
            const bool enable = typoMetrics.toBool();
            const QFont::StyleStrategy strategy = ret.styleStrategy();
            if (enable)
                ret.setStyleStrategy(QFont::StyleStrategy(strategy & ~QFont::PreferTypoLineMetrics));
            else
                ret.setStyleStrategy(QFont::StyleStrategy(strategy | QFont::PreferTypoLineMetrics));
            ok = true;
        }
    }

    {
        const QJSValue ctxFontMerging = params.property(QStringLiteral("contextFontMerging"));
        if (ctxFontMerging.isBool()) {
            const bool enable = ctxFontMerging.toBool();
            const QFont::StyleStrategy strategy = ret.styleStrategy();
            if (enable)
                ret.setStyleStrategy(QFont::StyleStrategy(strategy | QFont::ContextFontMerging));
            else
                ret.setStyleStrategy(QFont::StyleStrategy(strategy & ~QFont::ContextFontMerging));
            ok = true;
        }
    }

    {
        const QJSValue variableAxes = params.property(QStringLiteral("variableAxes"));
        if (variableAxes.isObject()) {
            QVariantMap variantMap = variableAxes.toVariant().toMap();
            for (auto [variableAxisName, variableAxisValue] : variantMap.asKeyValueRange()) {
                const auto maybeTag = QFont::Tag::fromString(variableAxisName);
                if (!maybeTag) {
                    qWarning() << "Invalid variable axis" << variableAxisName << "ignored";
                    continue;
                }

                bool valueOk;
                float value = variableAxisValue.toFloat(&valueOk);
                if (!valueOk) {
                    qWarning() << "Variable axis" << variableAxisName << "value" << variableAxisValue << "is not a floating point value.";
                    continue;
                }

                ret.setVariableAxis(*maybeTag, value);
                ok = true;
            }
        }
    }

    {
        const QJSValue features = params.property(QStringLiteral("features"));
        if (features.isObject()) {
            QVariantMap variantMap = features.toVariant().toMap();
            for (auto [featureName, featureValue] : variantMap.asKeyValueRange()) {
                const auto maybeTag = QFont::Tag::fromString(featureName);
                if (!maybeTag) {
                    qWarning() << "Invalid font feature" << featureName << "ignored";
                    continue;
                }

                bool valueOk;
                quint32 value = featureValue.toUInt(&valueOk);
                if (!valueOk) {
                    qWarning() << "Font feature" << featureName << "value" << featureValue << "is not an integer.";
                    continue;
                }

                ret.setFeature(*maybeTag, value);
                ok = true;
            }
        }
    }

    return ok ? ret : QVariant();
}

QString QQuickFontValueType::toString() const
{
    return QLatin1String("QFont(%1)").arg(QFont::toString());
}

QString QQuickFontValueType::family() const
{
    return QFont::family();
}

void QQuickFontValueType::setFamily(const QString &family)
{
    QFont::setFamily(family);
}

QString QQuickFontValueType::styleName() const
{
    return QFont::styleName();
}

void QQuickFontValueType::setStyleName(const QString &style)
{
    QFont::setStyleName(style);
}

bool QQuickFontValueType::bold() const
{
    return QFont::bold();
}

void QQuickFontValueType::setBold(bool b)
{
    QFont::setBold(b);
}

int QQuickFontValueType::weight() const
{
    return QFont::weight();
}

void QQuickFontValueType::setWeight(int w)
{
    QFont::setWeight(QFont::Weight(w));
}

bool QQuickFontValueType::italic() const
{
    return QFont::italic();
}

void QQuickFontValueType::setItalic(bool b)
{
    QFont::setItalic(b);
}

bool QQuickFontValueType::underline() const
{
    return QFont::underline();
}

void QQuickFontValueType::setUnderline(bool b)
{
    QFont::setUnderline(b);
}

bool QQuickFontValueType::overline() const
{
    return QFont::overline();
}

void QQuickFontValueType::setOverline(bool b)
{
    QFont::setOverline(b);
}

bool QQuickFontValueType::strikeout() const
{
    return QFont::strikeOut();
}

void QQuickFontValueType::setStrikeout(bool b)
{
    QFont::setStrikeOut(b);
}

qreal QQuickFontValueType::pointSize() const
{
    if (QFont::pointSizeF() == -1) {
        return QFont::pixelSize() * qreal(72.) / qreal(qt_defaultDpi());
    }
    return QFont::pointSizeF();
}

void QQuickFontValueType::setPointSize(qreal size)
{
    if ((QFont::resolveMask() & QFont::SizeResolved) && QFont::pixelSize() != -1) {
        qWarning() << "Both point size and pixel size set. Using pixel size.";
        return;
    }

    if (size >= 0.0) {
        QFont::setPointSizeF(size);
    }
}

int QQuickFontValueType::pixelSize() const
{
    if (QFont::pixelSize() == -1) {
        return (QFont::pointSizeF() * qt_defaultDpi()) / qreal(72.);
    }
    return QFont::pixelSize();
}

void QQuickFontValueType::setPixelSize(int size)
{
    if (size >0) {
        if ((QFont::resolveMask() & QFont::SizeResolved) && QFont::pointSizeF() != -1)
            qWarning() << "Both point size and pixel size set. Using pixel size.";
        QFont::setPixelSize(size);
    }
}

QQuickFontEnums::Capitalization QQuickFontValueType::capitalization() const
{
    return (QQuickFontEnums::Capitalization)QFont::capitalization();
}

void QQuickFontValueType::setCapitalization(QQuickFontEnums::Capitalization c)
{
    QFont::setCapitalization((QFont::Capitalization)c);
}

qreal QQuickFontValueType::letterSpacing() const
{
    return QFont::letterSpacing();
}

void QQuickFontValueType::setLetterSpacing(qreal size)
{
    QFont::setLetterSpacing(QFont::AbsoluteSpacing, size);
}

qreal QQuickFontValueType::wordSpacing() const
{
    return QFont::wordSpacing();
}

void QQuickFontValueType::setWordSpacing(qreal size)
{
    QFont::setWordSpacing(size);
}

QQuickFontEnums::HintingPreference QQuickFontValueType::hintingPreference() const
{
    return QQuickFontEnums::HintingPreference(QFont::hintingPreference());
}

void QQuickFontValueType::setHintingPreference(QQuickFontEnums::HintingPreference hintingPreference)
{
    QFont::setHintingPreference(QFont::HintingPreference(hintingPreference));
}

bool QQuickFontValueType::kerning() const
{
    return QFont::kerning();
}

void QQuickFontValueType::setKerning(bool b)
{
    QFont::setKerning(b);
}

bool QQuickFontValueType::preferShaping() const
{
    return (QFont::styleStrategy() & QFont::PreferNoShaping) == 0;
}

void QQuickFontValueType::setPreferShaping(bool enable)
{
    if (enable) {
        QFont::setStyleStrategy(
                static_cast<QFont::StyleStrategy>(QFont::styleStrategy() & ~QFont::PreferNoShaping));
    } else {
        QFont::setStyleStrategy(
                static_cast<QFont::StyleStrategy>(QFont::styleStrategy() | QFont::PreferNoShaping));
    }
}

void QQuickFontValueType::setVariableAxes(const QVariantMap &variableAxes)
{
    QFont::clearVariableAxes();
    for (auto [variableAxisName, variableAxisValue] : variableAxes.asKeyValueRange()) {
        const auto maybeTag = QFont::Tag::fromString(variableAxisName);
        if (!maybeTag) {
            qWarning() << "Invalid variable axis" << variableAxisName << "ignored";
            continue;
        }

        bool ok;
        float value = variableAxisValue.toFloat(&ok);
        if (!ok) {
            qWarning() << "Variable axis" << variableAxisName << "value" << variableAxisValue
                       << "is not a floating point value.";
            continue;
        }

        QFont::setVariableAxis(*maybeTag, value);
    }
}

QVariantMap QQuickFontValueType::variableAxes() const
{
    QVariantMap ret;
    for (const auto &tag : QFont::variableAxisTags())
        ret.insert(QString::fromUtf8(tag.toString()), QFont::variableAxisValue(tag));

    return ret;
}

void QQuickFontValueType::setFeatures(const QVariantMap &features)
{
    QFont::clearFeatures();
    for (auto [featureName, featureValue] : features.asKeyValueRange()) {
        const auto maybeTag = QFont::Tag::fromString(featureName);
        if (!maybeTag) {
            qWarning() << "Invalid font feature" << featureName << "ignored";
            continue;
        }

        bool ok;
        quint32 value = featureValue.toUInt(&ok);
        if (!ok) {
            qWarning() << "Font feature" << featureName << "value" << featureValue << "is not an integer.";
            continue;
        }

        QFont::setFeature(*maybeTag, value);
    }
}

QVariantMap QQuickFontValueType::features() const
{
    QVariantMap ret;
    for (const auto &tag : QFont::featureTags())
        ret.insert(QString::fromUtf8(tag.toString()), QFont::featureValue(tag));

    return ret;
}

bool QQuickFontValueType::contextFontMerging() const
{
    return (QFont::styleStrategy() & QFont::ContextFontMerging) != 0;
}

void QQuickFontValueType::setContextFontMerging(bool enable)
{
    if (enable) {
        QFont::setStyleStrategy(
                static_cast<QFont::StyleStrategy>(
                        QFont::styleStrategy() | QFont::ContextFontMerging));
    } else {
        QFont::setStyleStrategy(
                static_cast<QFont::StyleStrategy>(
                        QFont::styleStrategy() & ~QFont::ContextFontMerging));
    }
}

bool QQuickFontValueType::preferTypoLineMetrics() const
{
    return (QFont::styleStrategy() & QFont::PreferTypoLineMetrics) != 0;
}

void QQuickFontValueType::setPreferTypoLineMetrics(bool enable)
{
    if (enable) {
        QFont::setStyleStrategy(
                static_cast<QFont::StyleStrategy>(
                        QFont::styleStrategy() | QFont::PreferTypoLineMetrics));
    } else {
        QFont::setStyleStrategy(
                static_cast<QFont::StyleStrategy>(
                        QFont::styleStrategy() & ~QFont::PreferTypoLineMetrics));
    }
}

QVariant QQuickColorSpaceValueType::create(const QJSValue &params)
{
    if (!params.isObject())
        return QVariant();


    const QJSValue vName = params.property(QStringLiteral("namedColorSpace"));
    if (vName.isNumber())
        return QColorSpace((QColorSpace::NamedColorSpace)vName.toInt());

    const QJSValue vPri = params.property(QStringLiteral("primaries"));
    const QJSValue vTra = params.property(QStringLiteral("transferFunction"));
    if (!vPri.isNumber() || !vTra.isNumber())
        return QVariant();

    QColorSpace::Primaries pri = static_cast<QColorSpace::Primaries>(vPri.toInt());
    QColorSpace::TransferFunction tra = static_cast<QColorSpace::TransferFunction>(vTra.toInt());
    float gamma = 0.0f;
    if (tra == QColorSpace::TransferFunction::Gamma) {
        const QJSValue vGam = params.property(QStringLiteral("gamma"));
        if (!vGam.isNumber())
            return QVariant();
        gamma = vGam.toNumber();
    }

    return QColorSpace(pri, tra, gamma);
}

QQuickColorSpaceEnums::NamedColorSpace QQuickColorSpaceValueType::namedColorSpace() const noexcept
{
    if (const auto *p = QColorSpacePrivate::get(*this))
        return (QQuickColorSpaceEnums::NamedColorSpace)p->namedColorSpace;
    return QQuickColorSpaceEnums::Unknown;
}
void QQuickColorSpaceValueType::setNamedColorSpace(
        QQuickColorSpaceEnums::NamedColorSpace namedColorSpace)
{
    *static_cast<QColorSpace *>(this) = { (QColorSpace::NamedColorSpace)namedColorSpace };
}

QQuickColorSpaceEnums::Primaries QQuickColorSpaceValueType::primaries() const noexcept
{
    return (QQuickColorSpaceEnums::Primaries)QColorSpace::primaries();
}

void QQuickColorSpaceValueType::setPrimaries(QQuickColorSpaceEnums::Primaries primariesId)
{
    QColorSpace::setPrimaries((QColorSpace::Primaries)primariesId);
}

QQuickColorSpaceEnums::TransferFunction QQuickColorSpaceValueType::transferFunction() const noexcept
{
    return (QQuickColorSpaceEnums::TransferFunction)QColorSpace::transferFunction();
}

void QQuickColorSpaceValueType::setTransferFunction(
        QQuickColorSpaceEnums::TransferFunction transferFunction)
{
    QColorSpace::setTransferFunction(
            (QColorSpace::TransferFunction)transferFunction, QColorSpace::gamma());
}

float QQuickColorSpaceValueType::gamma() const noexcept
{
    return QColorSpace::gamma();
}

void QQuickColorSpaceValueType::setGamma(float gamma)
{
    QColorSpace::setTransferFunction(QColorSpace::transferFunction(), gamma);
}

QT_END_NAMESPACE

#include "moc_qquickvaluetypes_p.cpp"
