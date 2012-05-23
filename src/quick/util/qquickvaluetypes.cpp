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

#include <private/qquickvaluetypes_p.h>

#include <qtquickglobal.h>
#include <private/qqmlvaluetype_p.h>
#include <private/qfont_p.h>


QT_BEGIN_NAMESPACE

namespace QQuickValueTypes {
    void registerValueTypes()
    {
        QQmlValueTypeFactory::registerValueTypes("QtQuick", 2, 0);
        qmlRegisterValueTypeEnums<QQuickFontValueType>("QtQuick", 2, 0, "Font");
    }
}

QQuickColorValueType::QQuickColorValueType(QObject *parent)
    : QQmlValueTypeBase<QColor>(parent)
{
}

QString QQuickColorValueType::toString() const
{
    // to maintain behaviour with QtQuick 1.0, we just output normal toString() value.
    return QVariant(v).toString();
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


QQuickVector2DValueType::QQuickVector2DValueType(QObject *parent)
    : QQmlValueTypeBase<QVector2D>(parent)
{
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


QQuickVector3DValueType::QQuickVector3DValueType(QObject *parent)
    : QQmlValueTypeBase<QVector3D>(parent)
{
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


QQuickVector4DValueType::QQuickVector4DValueType(QObject *parent)
    : QQmlValueTypeBase<QVector4D>(parent)
{
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


QQuickQuaternionValueType::QQuickQuaternionValueType(QObject *parent)
    : QQmlValueTypeBase<QQuaternion>(parent)
{
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


QQuickMatrix4x4ValueType::QQuickMatrix4x4ValueType(QObject *parent)
    : QQmlValueTypeBase<QMatrix4x4>(parent)
{
}

QString QQuickMatrix4x4ValueType::toString() const
{
    return QString(QLatin1String("QMatrix4x4(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16)"))
            .arg(v(0, 0)).arg(v(0, 1)).arg(v(0, 2)).arg(v(0, 3))
            .arg(v(1, 0)).arg(v(1, 1)).arg(v(1, 2)).arg(v(1, 3))
            .arg(v(2, 0)).arg(v(2, 1)).arg(v(2, 2)).arg(v(2, 3))
            .arg(v(3, 0)).arg(v(3, 1)).arg(v(3, 2)).arg(v(3, 3));
}


QQuickFontValueType::QQuickFontValueType(QObject *parent)
    : QQmlValueTypeBase<QFont>(parent),
      pixelSizeSet(false),
      pointSizeSet(false)
{
}

void QQuickFontValueType::onLoad()
{
    pixelSizeSet = false;
    pointSizeSet = false;
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

bool QQuickFontValueType::bold() const
{
    return v.bold();
}

void QQuickFontValueType::setBold(bool b)
{
    v.setBold(b);
}

QQuickFontValueType::FontWeight QQuickFontValueType::weight() const
{
    return (QQuickFontValueType::FontWeight)v.weight();
}

void QQuickFontValueType::setWeight(QQuickFontValueType::FontWeight w)
{
    v.setWeight((QFont::Weight)w);
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
        if (dpi.isNull)
            dpi = qt_defaultDpi();
        return v.pixelSize() * qreal(72.) / qreal(dpi);
    }
    return v.pointSizeF();
}

void QQuickFontValueType::setPointSize(qreal size)
{
    if (pixelSizeSet) {
        qWarning() << "Both point size and pixel size set. Using pixel size.";
        return;
    }

    if (size >= 0.0) {
        pointSizeSet = true;
        v.setPointSizeF(size);
    } else {
        pointSizeSet = false;
    }
}

int QQuickFontValueType::pixelSize() const
{
    if (v.pixelSize() == -1) {
        if (dpi.isNull)
            dpi = qt_defaultDpi();
        return (v.pointSizeF() * dpi) / qreal(72.);
    }
    return v.pixelSize();
}

void QQuickFontValueType::setPixelSize(int size)
{
    if (size >0) {
        if (pointSizeSet)
            qWarning() << "Both point size and pixel size set. Using pixel size.";
        v.setPixelSize(size);
        pixelSizeSet = true;
    } else {
        pixelSizeSet = false;
    }
}

QQuickFontValueType::Capitalization QQuickFontValueType::capitalization() const
{
    return (QQuickFontValueType::Capitalization)v.capitalization();
}

void QQuickFontValueType::setCapitalization(QQuickFontValueType::Capitalization c)
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

QT_END_NAMESPACE
