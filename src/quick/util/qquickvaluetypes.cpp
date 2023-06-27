// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qquickvaluetypes_p.h>

#include <qtquickglobal.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qcolorspace_p.h>
#include <private/qfont_p.h>

QT_BEGIN_NAMESPACE

QQuickColorValueType::QQuickColorValueType(const QString &string)
    : v(QColor::fromString(string))
{
}

QVariant QQuickColorValueType::create(const QJSValue &params)
{
    return params.isString() ? QColor::fromString(params.toString()) : QVariant();
}

QString QQuickColorValueType::toString() const
{
    return v.name(v.alpha() != 255 ? QColor::HexArgb : QColor::HexRgb);
}

QVariant QQuickColorValueType::lighter(qreal factor) const
{
    return QQml_colorProvider()->lighter(this->v, factor);
}

QVariant QQuickColorValueType::darker(qreal factor) const
{
    return QQml_colorProvider()->darker(this->v, factor);
}

QVariant QQuickColorValueType::alpha(qreal value) const
{
    return QQml_colorProvider()->alpha(this->v, value);
}

QVariant QQuickColorValueType::tint(QVariant tintColor) const
{
    return QQml_colorProvider()->tint(this->v, tintColor);
}

qreal QQuickColorValueType::r() const
{
    return v.redF();
}

qreal QQuickColorValueType::g() const
{
    return v.greenF();
}

qreal QQuickColorValueType::b() const
{
    return v.blueF();
}

qreal QQuickColorValueType::a() const
{
    return v.alphaF();
}

qreal QQuickColorValueType::hsvHue() const
{
    return v.hsvHueF();
}

qreal QQuickColorValueType::hsvSaturation() const
{
    return v.hsvSaturationF();
}

qreal QQuickColorValueType::hsvValue() const
{
    return v.valueF();
}

qreal QQuickColorValueType::hslHue() const
{
    return v.hslHueF();
}

qreal QQuickColorValueType::hslSaturation() const
{
    return v.hslSaturationF();
}

qreal QQuickColorValueType::hslLightness() const
{
    return v.lightnessF();
}

bool QQuickColorValueType::isValid() const
{
    return v.isValid();
}

void QQuickColorValueType::setR(qreal r)
{
    v.setRedF(r);
}

void QQuickColorValueType::setG(qreal g)
{
    v.setGreenF(g);
}

void QQuickColorValueType::setB(qreal b)
{
    v.setBlueF(b);
}

void QQuickColorValueType::setA(qreal a)
{
    v.setAlphaF(a);
}

void QQuickColorValueType::setHsvHue(qreal hsvHue)
{
    float hue, saturation, value, alpha;
    v.getHsvF(&hue, &saturation, &value, &alpha);
    v.setHsvF(hsvHue, saturation, value, alpha);
}

void QQuickColorValueType::setHsvSaturation(qreal hsvSaturation)
{
    float hue, saturation, value, alpha;
    v.getHsvF(&hue, &saturation, &value, &alpha);
    v.setHsvF(hue, hsvSaturation, value, alpha);
}

void QQuickColorValueType::setHsvValue(qreal hsvValue)
{
    float hue, saturation, value, alpha;
    v.getHsvF(&hue, &saturation, &value, &alpha);
    v.setHsvF(hue, saturation, hsvValue, alpha);
}

void QQuickColorValueType::setHslHue(qreal hslHue)
{
    float hue, saturation, lightness, alpha;
    v.getHslF(&hue, &saturation, &lightness, &alpha);
    v.setHslF(hslHue, saturation, lightness, alpha);
}

void QQuickColorValueType::setHslSaturation(qreal hslSaturation)
{
    float hue, saturation, lightness, alpha;
    v.getHslF(&hue, &saturation, &lightness, &alpha);
    v.setHslF(hue, hslSaturation, lightness, alpha);
}

void QQuickColorValueType::setHslLightness(qreal hslLightness)
{
    float hue, saturation, lightness, alpha;
    v.getHslF(&hue, &saturation, &lightness, &alpha);
    v.setHslF(hue, saturation, hslLightness, alpha);
}

template<typename T, int NumParams>
QVariant createValueTypeFromNumberString(const QString &s)
{
    Q_STATIC_ASSERT_X(NumParams == 2 || NumParams == 3 || NumParams == 4 || NumParams == 16,
                      "Unsupported number of params; add an additional case below if necessary.");

    if (s.count(u',') != NumParams - 1)
        return QVariant();

    QVarLengthArray<float, NumParams> parameters;
    bool ok = true;
    for (qsizetype prev = 0, next = s.indexOf(u','), length = s.size(); ok && prev < length;) {
        parameters.append(s.mid(prev, next - prev).toFloat(&ok));
        prev = next + 1;
        next = (parameters.size() == NumParams - 1) ? length : s.indexOf(u',', prev);
    }

    if (!ok)
        return QVariant();

    if constexpr (NumParams == 2) {
        return T(parameters[0], parameters[1]);
    } else if constexpr (NumParams == 3) {
        return T(parameters[0], parameters[1], parameters[2]);
    } else if constexpr (NumParams == 4) {
        return T(parameters[0], parameters[1], parameters[2], parameters[3]);
    } else if constexpr (NumParams == 16) {
        return T(parameters[0], parameters[1], parameters[2], parameters[3],
                 parameters[4], parameters[5], parameters[6], parameters[7],
                 parameters[8], parameters[9], parameters[10], parameters[11],
                 parameters[12], parameters[13], parameters[14], parameters[15]);
    } else {
        Q_UNREACHABLE();
    }

    return QVariant();
}

QVariant QQuickVector2DValueType::create(const QJSValue &params)
{
    if (params.isString())
        return createValueTypeFromNumberString<QVector2D, 2>(params.toString());
    if (params.isArray())
        return QVector2D(params.property(0).toNumber(), params.property(1).toNumber());
    return QVariant();
}

QString QQuickVector2DValueType::toString() const
{
    return QString(QLatin1String("QVector2D(%1, %2)")).arg(v.x()).arg(v.y());
}

qreal QQuickVector2DValueType::x() const
{
    return v.x();
}

qreal QQuickVector2DValueType::y() const
{
    return v.y();
}

void QQuickVector2DValueType::setX(qreal x)
{
    v.setX(x);
}

void QQuickVector2DValueType::setY(qreal y)
{
    v.setY(y);
}

qreal QQuickVector2DValueType::dotProduct(const QVector2D &vec) const
{
    return QVector2D::dotProduct(v, vec);
}

QVector2D QQuickVector2DValueType::times(const QVector2D &vec) const
{
    return v * vec;
}

QVector2D QQuickVector2DValueType::times(qreal scalar) const
{
    return v * scalar;
}

QVector2D QQuickVector2DValueType::plus(const QVector2D &vec) const
{
    return v + vec;
}

QVector2D QQuickVector2DValueType::minus(const QVector2D &vec) const
{
    return v - vec;
}

QVector2D QQuickVector2DValueType::normalized() const
{
    return v.normalized();
}

qreal QQuickVector2DValueType::length() const
{
    return v.length();
}

QVector3D QQuickVector2DValueType::toVector3d() const
{
    return v.toVector3D();
}

QVector4D QQuickVector2DValueType::toVector4d() const
{
    return v.toVector4D();
}

bool QQuickVector2DValueType::fuzzyEquals(const QVector2D &vec, qreal epsilon) const
{
    qreal absEps = qAbs(epsilon);
    if (qAbs(v.x() - vec.x()) > absEps)
        return false;
    if (qAbs(v.y() - vec.y()) > absEps)
        return false;
    return true;
}

bool QQuickVector2DValueType::fuzzyEquals(const QVector2D &vec) const
{
    return qFuzzyCompare(v, vec);
}

QVariant QQuickVector3DValueType::create(const QJSValue &params)
{
    if (params.isString())
        return createValueTypeFromNumberString<QVector3D, 3>(params.toString());

    if (params.isArray()) {
        return QVector3D(params.property(0).toNumber(), params.property(1).toNumber(),
                         params.property(2).toNumber());
    }
    return QVariant();
}

QString QQuickVector3DValueType::toString() const
{
    return QString(QLatin1String("QVector3D(%1, %2, %3)")).arg(v.x()).arg(v.y()).arg(v.z());
}

qreal QQuickVector3DValueType::x() const
{
    return v.x();
}

qreal QQuickVector3DValueType::y() const
{
    return v.y();
}

qreal QQuickVector3DValueType::z() const
{
    return v.z();
}

void QQuickVector3DValueType::setX(qreal x)
{
    v.setX(x);
}

void QQuickVector3DValueType::setY(qreal y)
{
    v.setY(y);
}

void QQuickVector3DValueType::setZ(qreal z)
{
    v.setZ(z);
}

QVector3D QQuickVector3DValueType::crossProduct(const QVector3D &vec) const
{
    return QVector3D::crossProduct(v, vec);
}

qreal QQuickVector3DValueType::dotProduct(const QVector3D &vec) const
{
    return QVector3D::dotProduct(v, vec);
}

QVector3D QQuickVector3DValueType::times(const QMatrix4x4 &m) const
{
    return (QVector4D(v, 1) * m).toVector3DAffine();
}

QVector3D QQuickVector3DValueType::times(const QVector3D &vec) const
{
    return v * vec;
}

QVector3D QQuickVector3DValueType::times(qreal scalar) const
{
    return v * scalar;
}

QVector3D QQuickVector3DValueType::plus(const QVector3D &vec) const
{
    return v + vec;
}

QVector3D QQuickVector3DValueType::minus(const QVector3D &vec) const
{
    return v - vec;
}

QVector3D QQuickVector3DValueType::normalized() const
{
    return v.normalized();
}

qreal QQuickVector3DValueType::length() const
{
    return v.length();
}

QVector2D QQuickVector3DValueType::toVector2d() const
{
    return v.toVector2D();
}

QVector4D QQuickVector3DValueType::toVector4d() const
{
    return v.toVector4D();
}

bool QQuickVector3DValueType::fuzzyEquals(const QVector3D &vec, qreal epsilon) const
{
    qreal absEps = qAbs(epsilon);
    if (qAbs(v.x() - vec.x()) > absEps)
        return false;
    if (qAbs(v.y() - vec.y()) > absEps)
        return false;
    if (qAbs(v.z() - vec.z()) > absEps)
        return false;
    return true;
}

bool QQuickVector3DValueType::fuzzyEquals(const QVector3D &vec) const
{
    return qFuzzyCompare(v, vec);
}

QVariant QQuickVector4DValueType::create(const QJSValue &params)
{
    if (params.isString())
        return createValueTypeFromNumberString<QVector4D, 4>(params.toString());

    if (params.isArray()) {
        return QVector4D(params.property(0).toNumber(), params.property(1).toNumber(),
                         params.property(2).toNumber(), params.property(3).toNumber());
    }

    return QVariant();
}

QString QQuickVector4DValueType::toString() const
{
    return QString(QLatin1String("QVector4D(%1, %2, %3, %4)")).arg(v.x()).arg(v.y()).arg(v.z()).arg(v.w());
}

qreal QQuickVector4DValueType::x() const
{
    return v.x();
}

qreal QQuickVector4DValueType::y() const
{
    return v.y();
}

qreal QQuickVector4DValueType::z() const
{
    return v.z();
}

qreal QQuickVector4DValueType::w() const
{
    return v.w();
}

void QQuickVector4DValueType::setX(qreal x)
{
    v.setX(x);
}

void QQuickVector4DValueType::setY(qreal y)
{
    v.setY(y);
}

void QQuickVector4DValueType::setZ(qreal z)
{
    v.setZ(z);
}

void QQuickVector4DValueType::setW(qreal w)
{
    v.setW(w);
}

qreal QQuickVector4DValueType::dotProduct(const QVector4D &vec) const
{
    return QVector4D::dotProduct(v, vec);
}

QVector4D QQuickVector4DValueType::times(const QVector4D &vec) const
{
    return v * vec;
}

QVector4D QQuickVector4DValueType::times(const QMatrix4x4 &m) const
{
    return v * m;
}

QVector4D QQuickVector4DValueType::times(qreal scalar) const
{
    return v * scalar;
}

QVector4D QQuickVector4DValueType::plus(const QVector4D &vec) const
{
    return v + vec;
}

QVector4D QQuickVector4DValueType::minus(const QVector4D &vec) const
{
    return v - vec;
}

QVector4D QQuickVector4DValueType::normalized() const
{
    return v.normalized();
}

qreal QQuickVector4DValueType::length() const
{
    return v.length();
}

QVector2D QQuickVector4DValueType::toVector2d() const
{
    return v.toVector2D();
}

QVector3D QQuickVector4DValueType::toVector3d() const
{
    return v.toVector3D();
}

bool QQuickVector4DValueType::fuzzyEquals(const QVector4D &vec, qreal epsilon) const
{
    qreal absEps = qAbs(epsilon);
    if (qAbs(v.x() - vec.x()) > absEps)
        return false;
    if (qAbs(v.y() - vec.y()) > absEps)
        return false;
    if (qAbs(v.z() - vec.z()) > absEps)
        return false;
    if (qAbs(v.w() - vec.w()) > absEps)
        return false;
    return true;
}

bool QQuickVector4DValueType::fuzzyEquals(const QVector4D &vec) const
{
    return qFuzzyCompare(v, vec);
}

QVariant QQuickQuaternionValueType::create(const QJSValue &params)
{
    if (params.isString())
        return createValueTypeFromNumberString<QQuaternion, 4>(params.toString());

    if (params.isArray()) {
        return QQuaternion(params.property(0).toNumber(), params.property(1).toNumber(),
                           params.property(2).toNumber(), params.property(3).toNumber());
    }

    return QVariant();
}

QString QQuickQuaternionValueType::toString() const
{
    return QString(QLatin1String("QQuaternion(%1, %2, %3, %4)")).arg(v.scalar()).arg(v.x()).arg(v.y()).arg(v.z());
}

qreal QQuickQuaternionValueType::scalar() const
{
    return v.scalar();
}

qreal QQuickQuaternionValueType::x() const
{
    return v.x();
}

qreal QQuickQuaternionValueType::y() const
{
    return v.y();
}

qreal QQuickQuaternionValueType::z() const
{
    return v.z();
}

void QQuickQuaternionValueType::setScalar(qreal scalar)
{
    v.setScalar(scalar);
}

void QQuickQuaternionValueType::setX(qreal x)
{
    v.setX(x);
}

void QQuickQuaternionValueType::setY(qreal y)
{
    v.setY(y);
}

void QQuickQuaternionValueType::setZ(qreal z)
{
    v.setZ(z);
}

qreal QQuickQuaternionValueType::dotProduct(const QQuaternion &q) const
{
    return QQuaternion::dotProduct(v, q);
}

QQuaternion QQuickQuaternionValueType::times(const QQuaternion &q) const
{
    return v * q;
}

QVector3D QQuickQuaternionValueType::times(const QVector3D &vec) const
{
    return v * vec;
}

QQuaternion QQuickQuaternionValueType::times(qreal factor) const
{
    return v * factor;
}

QQuaternion QQuickQuaternionValueType::plus(const QQuaternion &q) const
{
    return v + q;
}

QQuaternion QQuickQuaternionValueType::minus(const QQuaternion &q) const
{
    return v - q;
}

QQuaternion QQuickQuaternionValueType::normalized() const
{
    return v.normalized();
}

QQuaternion QQuickQuaternionValueType::inverted() const
{
    return v.inverted();
}

QQuaternion QQuickQuaternionValueType::conjugated() const
{
    return v.conjugated();
}

qreal QQuickQuaternionValueType::length() const
{
    return v.length();
}

QVector3D QQuickQuaternionValueType::toEulerAngles() const
{
    return v.toEulerAngles();
}

QVector4D QQuickQuaternionValueType::toVector4d() const
{
    return v.toVector4D();
}

bool QQuickQuaternionValueType::fuzzyEquals(const QQuaternion &q, qreal epsilon) const
{
    qreal absEps = qAbs(epsilon);
    if (qAbs(v.scalar() - q.scalar()) > absEps)
        return false;
    if (qAbs(v.x() - q.x()) > absEps)
        return false;
    if (qAbs(v.y() - q.y()) > absEps)
        return false;
    if (qAbs(v.z() - q.z()) > absEps)
        return false;
    return true;
}

bool QQuickQuaternionValueType::fuzzyEquals(const QQuaternion &q) const
{
    return qFuzzyCompare(v, q);
}

QVariant QQuickMatrix4x4ValueType::create(const QJSValue &params)
{
    if (params.isNull() || params.isUndefined())
        return QMatrix4x4();

    if (params.isString())
        return createValueTypeFromNumberString<QMatrix4x4, 16>(params.toString());

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
    return v * m;
}

QVector4D QQuickMatrix4x4ValueType::times(const QVector4D &vec) const
{
    return v * vec;
}

QVector3D QQuickMatrix4x4ValueType::times(const QVector3D &vec) const
{
    return v.map(vec);
}

QMatrix4x4 QQuickMatrix4x4ValueType::times(qreal factor) const
{
    return v * factor;
}

QMatrix4x4 QQuickMatrix4x4ValueType::plus(const QMatrix4x4 &m) const
{
    return v + m;
}

QMatrix4x4 QQuickMatrix4x4ValueType::minus(const QMatrix4x4 &m) const
{
    return v - m;
}

QVector4D QQuickMatrix4x4ValueType::row(int n) const
{
    return v.row(n);
}

QVector4D QQuickMatrix4x4ValueType::column(int m) const
{
    return v.column(m);
}

qreal QQuickMatrix4x4ValueType::determinant() const
{
    return v.determinant();
}

QMatrix4x4 QQuickMatrix4x4ValueType::inverted() const
{
    return v.inverted();
}

QMatrix4x4 QQuickMatrix4x4ValueType::transposed() const
{
    return v.transposed();
}

QPointF QQuickMatrix4x4ValueType::map(const QPointF p) const
{
    return v.map(p);
}

QRectF QQuickMatrix4x4ValueType::mapRect(const QRectF r) const
{
    return v.mapRect(r);
}

bool QQuickMatrix4x4ValueType::fuzzyEquals(const QMatrix4x4 &m, qreal epsilon) const
{
    qreal absEps = qAbs(epsilon);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (qAbs(v(i,j) - m(i,j)) > absEps) {
                return false;
            }
        }
    }
    return true;
}

bool QQuickMatrix4x4ValueType::fuzzyEquals(const QMatrix4x4 &m) const
{
    return qFuzzyCompare(v, m);
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
    setFontProperty(ret, &QFont::setUnderline, QStringLiteral("underline"), params, &ok);
    setFontProperty(ret, &QFont::setWeight, QStringLiteral("weight"), params, &ok);
    setFontProperty(ret, &QFont::setWordSpacing, QStringLiteral("wordSpacing"), params, &ok);
    setFontProperty(ret, &QFont::setHintingPreference, QStringLiteral("hintingPreference"), params, &ok);
    setFontProperty(ret, &QFont::setKerning, QStringLiteral("kerning"), params, &ok);

    const QJSValue vlspac = params.property(QStringLiteral("letterSpacing"));
    if (vlspac.isNumber()) {
        ret.setLetterSpacing(QFont::AbsoluteSpacing, vlspac.toNumber());
        ok = true;
    }

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

    return ok ? ret : QVariant();
}

QString QQuickFontValueType::toString() const
{
    return QString(QLatin1String("QFont(%1)")).arg(v.toString());
}

QString QQuickFontValueType::family() const
{
    return v.family();
}

void QQuickFontValueType::setFamily(const QString &family)
{
    v.setFamily(family);
}

QString QQuickFontValueType::styleName() const
{
    return v.styleName();
}

void QQuickFontValueType::setStyleName(const QString &style)
{
    v.setStyleName(style);
}

bool QQuickFontValueType::bold() const
{
    return v.bold();
}

void QQuickFontValueType::setBold(bool b)
{
    v.setBold(b);
}

int QQuickFontValueType::weight() const
{
    return v.weight();
}

void QQuickFontValueType::setWeight(int w)
{
    v.setWeight(QFont::Weight(w));
}

bool QQuickFontValueType::italic() const
{
    return v.italic();
}

void QQuickFontValueType::setItalic(bool b)
{
    v.setItalic(b);
}

bool QQuickFontValueType::underline() const
{
    return v.underline();
}

void QQuickFontValueType::setUnderline(bool b)
{
    v.setUnderline(b);
}

bool QQuickFontValueType::overline() const
{
    return v.overline();
}

void QQuickFontValueType::setOverline(bool b)
{
    v.setOverline(b);
}

bool QQuickFontValueType::strikeout() const
{
    return v.strikeOut();
}

void QQuickFontValueType::setStrikeout(bool b)
{
    v.setStrikeOut(b);
}

qreal QQuickFontValueType::pointSize() const
{
    if (v.pointSizeF() == -1) {
        return v.pixelSize() * qreal(72.) / qreal(qt_defaultDpi());
    }
    return v.pointSizeF();
}

void QQuickFontValueType::setPointSize(qreal size)
{
    if ((v.resolveMask() & QFont::SizeResolved) && v.pixelSize() != -1) {
        qWarning() << "Both point size and pixel size set. Using pixel size.";
        return;
    }

    if (size >= 0.0) {
        v.setPointSizeF(size);
    }
}

int QQuickFontValueType::pixelSize() const
{
    if (v.pixelSize() == -1) {
        return (v.pointSizeF() * qt_defaultDpi()) / qreal(72.);
    }
    return v.pixelSize();
}

void QQuickFontValueType::setPixelSize(int size)
{
    if (size >0) {
        if ((v.resolveMask() & QFont::SizeResolved) && v.pointSizeF() != -1)
            qWarning() << "Both point size and pixel size set. Using pixel size.";
        v.setPixelSize(size);
    }
}

QQuickFontEnums::Capitalization QQuickFontValueType::capitalization() const
{
    return (QQuickFontEnums::Capitalization)v.capitalization();
}

void QQuickFontValueType::setCapitalization(QQuickFontEnums::Capitalization c)
{
    v.setCapitalization((QFont::Capitalization)c);
}

qreal QQuickFontValueType::letterSpacing() const
{
    return v.letterSpacing();
}

void QQuickFontValueType::setLetterSpacing(qreal size)
{
    v.setLetterSpacing(QFont::AbsoluteSpacing, size);
}

qreal QQuickFontValueType::wordSpacing() const
{
    return v.wordSpacing();
}

void QQuickFontValueType::setWordSpacing(qreal size)
{
    v.setWordSpacing(size);
}

QQuickFontEnums::HintingPreference QQuickFontValueType::hintingPreference() const
{
    return QQuickFontEnums::HintingPreference(v.hintingPreference());
}

void QQuickFontValueType::setHintingPreference(QQuickFontEnums::HintingPreference hintingPreference)
{
    v.setHintingPreference(QFont::HintingPreference(hintingPreference));
}

bool QQuickFontValueType::kerning() const
{
    return v.kerning();
}

void QQuickFontValueType::setKerning(bool b)
{
    v.setKerning(b);
}

bool QQuickFontValueType::preferShaping() const
{
    return (v.styleStrategy() & QFont::PreferNoShaping) == 0;
}

void QQuickFontValueType::setPreferShaping(bool enable)
{
    if (enable)
        v.setStyleStrategy(static_cast<QFont::StyleStrategy>(v.styleStrategy() & ~QFont::PreferNoShaping));
    else
        v.setStyleStrategy(static_cast<QFont::StyleStrategy>(v.styleStrategy() | QFont::PreferNoShaping));
}

void QQuickFontValueType::setFeatures(const QVariantMap &features)
{
    v.clearFeatures();
    for (auto it = features.constBegin(); it != features.constEnd(); ++it) {
        QString featureName = it.key();
        quint32 tag = QFont::stringToTag(featureName.toUtf8());
        if (tag == 0) {
            qWarning() << "Invalid font feature" << featureName << "ignored";
            continue;
        }

        bool ok;
        quint32 value = it.value().toUInt(&ok);
        if (!ok) {
            qWarning() << "Font feature value" << it.value() << "is not an integer.";
            continue;
        }

        v.setFeature(tag, value);
    }
}

QVariantMap QQuickFontValueType::features() const
{
    QVariantMap ret;
    for (quint32 tag : v.featureTags()) {
        QString featureName = QString::fromUtf8(QFont::tagToString(tag));

        ret.insert(featureName, v.featureValue(tag));
    }

    return ret;
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
    if (const auto *p = QColorSpacePrivate::get(v))
        return (QQuickColorSpaceEnums::NamedColorSpace)p->namedColorSpace;
    return QQuickColorSpaceEnums::Unknown;
}
void QQuickColorSpaceValueType::setNamedColorSpace(QQuickColorSpaceEnums::NamedColorSpace namedColorSpace)
{
    v = { (QColorSpace::NamedColorSpace)namedColorSpace };
}

QQuickColorSpaceEnums::Primaries QQuickColorSpaceValueType::primaries() const noexcept
{
    return (QQuickColorSpaceEnums::Primaries)v.primaries();
}

void QQuickColorSpaceValueType::setPrimaries(QQuickColorSpaceEnums::Primaries primariesId)
{
    v.setPrimaries((QColorSpace::Primaries)primariesId);
}

QQuickColorSpaceEnums::TransferFunction QQuickColorSpaceValueType::transferFunction() const noexcept
{
    return (QQuickColorSpaceEnums::TransferFunction)v.transferFunction();
}

void QQuickColorSpaceValueType::setTransferFunction(QQuickColorSpaceEnums::TransferFunction transferFunction)
{
    v.setTransferFunction((QColorSpace::TransferFunction)transferFunction, v.gamma());
}

float QQuickColorSpaceValueType::gamma() const noexcept
{
    return v.gamma();
}

void QQuickColorSpaceValueType::setGamma(float gamma)
{
    v.setTransferFunction(v.transferFunction(), gamma);
}

QT_END_NAMESPACE

#include "moc_qquickvaluetypes_p.cpp"
