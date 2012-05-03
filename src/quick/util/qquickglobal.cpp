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
#include <private/qquickapplication_p.h>
#include <private/qqmlglobal_p.h>

#include <QtGui/QGuiApplication>
#include <QtGui/qdesktopservices.h>
#include <QtGui/qfontdatabase.h>


QT_BEGIN_NAMESPACE

class QQuickColorProvider : public QQmlColorProvider
{
public:
    static inline QColor QColorFromString(const QString &s)
    {
        // Should we also handle #rrggbb here?
        if (s.length() == 9 && s.startsWith(QLatin1Char('#'))) {
            uchar a = fromHex(s, 1);
            uchar r = fromHex(s, 3);
            uchar g = fromHex(s, 5);
            uchar b = fromHex(s, 7);
            return QColor(r, g, b, a);
        }

        return QColor(s);
    }

    QVariant colorFromString(const QString &s, bool *ok)
    {
        QColor c(QColorFromString(s));
        if (c.isValid()) {
            if (ok) *ok = true;
            return QVariant(c);
        }

        if (ok) *ok = false;
        return QVariant();
    }

    unsigned rgbaFromString(const QString &s, bool *ok)
    {
        QColor c(QColorFromString(s));
        if (c.isValid()) {
            if (ok) *ok = true;
            return c.rgba();
        }

        if (ok) *ok = false;
        return 0;
    }

    QString stringFromRgba(unsigned rgba)
    {
        QColor c(QColor::fromRgba(rgba));
        if (c.isValid()) {
            return QVariant(c).toString();
        }

        return QString();
    }

    QVariant fromRgbF(double r, double g, double b, double a)
    {
        return QVariant(QColor::fromRgbF(r, g, b, a));
    }

    QVariant fromHslF(double h, double s, double l, double a)
    {
        return QVariant(QColor::fromHslF(h, s, l, a));
    }

    QVariant lighter(const QVariant &var, qreal factor)
    {
        QColor color = var.value<QColor>();
        color = color.lighter(int(qRound(factor*100.)));
        return QVariant::fromValue(color);
    }

    QVariant darker(const QVariant &var, qreal factor)
    {
        QColor color = var.value<QColor>();
        color = color.darker(int(qRound(factor*100.)));
        return QVariant::fromValue(color);
    }

    QVariant tint(const QVariant &baseVar, const QVariant &tintVar)
    {
        QColor tintColor = tintVar.value<QColor>();

        int tintAlpha = tintColor.alpha();
        if (tintAlpha == 0xFF) {
            return tintVar;
        } else if (tintAlpha == 0x00) {
            return baseVar;
        }

        // tint the base color and return the final color
        QColor baseColor = baseVar.value<QColor>();
        qreal a = tintColor.alphaF();
        qreal inv_a = 1.0 - a;

        qreal r = tintColor.redF() * a + baseColor.redF() * inv_a;
        qreal g = tintColor.greenF() * a + baseColor.greenF() * inv_a;
        qreal b = tintColor.blueF() * a + baseColor.blueF() * inv_a;

        return QVariant::fromValue(QColor::fromRgbF(r, g, b, a + inv_a * baseColor.alphaF()));
    }

private:
    static uchar fromHex(const uchar c, const uchar c2)
    {
        uchar rv = 0;
        if (c >= '0' && c <= '9')
            rv += (c - '0') * 16;
        else if (c >= 'A' && c <= 'F')
            rv += (c - 'A' + 10) * 16;
        else if (c >= 'a' && c <= 'f')
            rv += (c - 'a' + 10) * 16;

        if (c2 >= '0' && c2 <= '9')
            rv += (c2 - '0');
        else if (c2 >= 'A' && c2 <= 'F')
            rv += (c2 - 'A' + 10);
        else if (c2 >= 'a' && c2 <= 'f')
            rv += (c2 - 'a' + 10);

        return rv;
    }

    static inline uchar fromHex(const QString &s, int idx)
    {
        uchar c = s.at(idx).toLatin1();
        uchar c2 = s.at(idx + 1).toLatin1();
        return fromHex(c, c2);
    }
};


// Note: The functions in this class provide handling only for the types
// that the QML engine will currently actually call them for, so many
// appear incompletely implemented.  For some functions, the implementation
// would be obvious, but for others (particularly create and createFromString)
// the exact semantics are unknown.  For this reason unused functionality
// has been omitted.

class QQuickValueTypeProvider : public QQmlValueTypeProvider
{
public:
    static QVector3D vector3DFromString(const QString &s, bool *ok)
    {
        if (s.count(QLatin1Char(',')) == 2) {
            int index = s.indexOf(QLatin1Char(','));
            int index2 = s.indexOf(QLatin1Char(','), index+1);

            bool xGood, yGood, zGood;
            qreal xCoord = s.left(index).toDouble(&xGood);
            qreal yCoord = s.mid(index+1, index2-index-1).toDouble(&yGood);
            qreal zCoord = s.mid(index2+1).toDouble(&zGood);

            if (xGood && yGood && zGood) {
                if (ok) *ok = true;
                return QVector3D(xCoord, yCoord, zCoord);
            }
        }

        if (ok) *ok = false;
        return QVector3D();
    }

    static QVector4D vector4DFromString(const QString &s, bool *ok)
    {
        if (s.count(QLatin1Char(',')) == 3) {
            int index = s.indexOf(QLatin1Char(','));
            int index2 = s.indexOf(QLatin1Char(','), index+1);
            int index3 = s.indexOf(QLatin1Char(','), index2+1);

            bool xGood, yGood, zGood, wGood;
            qreal xCoord = s.left(index).toDouble(&xGood);
            qreal yCoord = s.mid(index+1, index2-index-1).toDouble(&yGood);
            qreal zCoord = s.mid(index2+1, index3-index2-1).toDouble(&zGood);
            qreal wCoord = s.mid(index3+1).toDouble(&wGood);

            if (xGood && yGood && zGood && wGood) {
                if (ok) *ok = true;
                return QVector4D(xCoord, yCoord, zCoord, wCoord);
            }
        }

        if (ok) *ok = false;
        return QVector4D();
    }

    bool create(int type, QQmlValueType *&v)
    {
        switch (type) {
        case QMetaType::QColor:
            v = new QQuickColorValueType;
            return true;
        case QMetaType::QVector2D:
            v = new QQuickVector2DValueType;
            return true;
        case QMetaType::QVector3D:
            v = new QQuickVector3DValueType;
            return true;
        case QMetaType::QVector4D:
            v = new QQuickVector4DValueType;
            return true;
        case QMetaType::QQuaternion:
            v = new QQuickQuaternionValueType;
            return true;
        case QMetaType::QMatrix4x4:
            v = new QQuickMatrix4x4ValueType;
            return true;
        case QMetaType::QFont:
            v = new QQuickFontValueType;
            return true;
        default:
            return false;
        }
    }

    bool init(int type, void *data, size_t n)
    {
        if (type == QMetaType::QColor) {
            Q_ASSERT(n >= sizeof(QColor));
            QColor *color = reinterpret_cast<QColor *>(data);
            new (color) QColor();
            return true;
        }

        return false;
    }

    bool destroy(int type, void *data, size_t n)
    {
        if (type == QMetaType::QColor) {
            Q_ASSERT(n >= sizeof(QColor));
            QColor *color = reinterpret_cast<QColor *>(data);
            color->~QColor();
            return true;
        }

        return false;
    }

    bool copy(int type, const void *src, void *dst, size_t n)
    {
        if (type == QMetaType::QColor) {
            Q_ASSERT(n >= sizeof(QColor));
            const QColor *srcColor = reinterpret_cast<const QColor *>(src);
            QColor *dstColor = reinterpret_cast<QColor *>(dst);
            new (dstColor) QColor(*srcColor);
            return true;
        }

        return false;
    }

    bool create(int type, int argc, const void *argv[], QVariant *v)
    {
        switch (type) {
        case QMetaType::QVector3D:
            if (argc == 1) {
                const float *xyz = reinterpret_cast<const float*>(argv[0]);
                QVector3D v3(xyz[0], xyz[1], xyz[2]);
                *v = QVariant(v3);
                return true;
            }
            break;
        case QMetaType::QVector4D:
            if (argc == 1) {
                const float *xyzw = reinterpret_cast<const float*>(argv[0]);
                QVector4D v4(xyzw[0], xyzw[1], xyzw[2], xyzw[3]);
                *v = QVariant(v4);
                return true;
            }
            break;
        }

        return false;
    }

    bool createFromString(int type, const QString &s, void *data, size_t n)
    {
        bool ok = false;

        switch (type) {
        case QMetaType::QColor:
            {
            Q_ASSERT(n >= sizeof(QColor));
            QColor *color = reinterpret_cast<QColor *>(data);
            new (color) QColor(QQuickColorProvider::QColorFromString(s));
            return true;
            }
        case QMetaType::QVector3D:
            {
            Q_ASSERT(n >= sizeof(QVector3D));
            QVector3D *v3 = reinterpret_cast<QVector3D *>(data);
            new (v3) QVector3D(vector3DFromString(s, &ok));
            return true;
            }
        case QMetaType::QVector4D:
            {
            Q_ASSERT(n >= sizeof(QVector4D));
            QVector4D *v4 = reinterpret_cast<QVector4D *>(data);
            new (v4) QVector4D(vector4DFromString(s, &ok));
            return true;
            }
        }

        return false;
    }

    bool createStringFrom(int type, const void *data, QString *s)
    {
        if (type == QMetaType::QColor) {
            const QColor *color = reinterpret_cast<const QColor *>(data);
            new (s) QString(QVariant(*color).toString());
            return true;
        }

        return false;
    }

    bool variantFromString(const QString &s, QVariant *v)
    {
        QColor c(QQuickColorProvider::QColorFromString(s));
        if (c.isValid()) {
            *v = QVariant::fromValue(c);
            return true;
        }

        bool ok = false;

        QVector3D v3 = vector3DFromString(s, &ok);
        if (ok) {
            *v = QVariant::fromValue(v3);
            return true;
        }

        QVector4D v4 = vector4DFromString(s, &ok);
        if (ok) {
            *v = QVariant::fromValue(v4);
            return true;
        }

        return false;
    }

    bool variantFromString(int type, const QString &s, QVariant *v)
    {
        bool ok = false;

        switch (type) {
        case QMetaType::QColor:
            {
            QColor c(QQuickColorProvider::QColorFromString(s));
            *v = QVariant::fromValue(c);
            return true;
            }
        case QMetaType::QVector3D:
            *v = QVariant::fromValue(vector3DFromString(s, &ok));
            return true;
        case QMetaType::QVector4D:
            *v = QVariant::fromValue(vector4DFromString(s, &ok));
            return true;
        }

        return false;
    }

    template<typename T>
    bool typedEqual(const void *lhs, const void *rhs)
    {
        return (*(reinterpret_cast<const T *>(lhs)) == *(reinterpret_cast<const T *>(rhs)));
    }

    bool equal(int type, const void *lhs, const void *rhs)
    {
        switch (type) {
        case QMetaType::QColor:
            return typedEqual<QColor>(lhs, rhs);
        case QMetaType::QVector3D:
            return typedEqual<QVector3D>(lhs, rhs);
        case QMetaType::QVector4D:
            return typedEqual<QVector4D>(lhs, rhs);
        }

        return false;
    }

    bool store(int type, const void *src, void *dst, size_t n)
    {
        switch (type) {
        case QMetaType::QColor:
            {
            Q_ASSERT(n >= sizeof(QColor));
            const QRgb *rgb = reinterpret_cast<const QRgb *>(src);
            QColor *color = reinterpret_cast<QColor *>(dst);
            new (color) QColor(QColor::fromRgba(*rgb));
            return true;
            }
        case QMetaType::QVector3D:
            {
            Q_ASSERT(n >= sizeof(QVector3D));
            const QVector3D *srcVector = reinterpret_cast<const QVector3D *>(src);
            QVector3D *dstVector = reinterpret_cast<QVector3D *>(dst);
            new (dstVector) QVector3D(*srcVector);
            return true;
            }
        case QMetaType::QVector4D:
            {
            Q_ASSERT(n >= sizeof(QVector4D));
            const QVector4D *srcVector = reinterpret_cast<const QVector4D *>(src);
            QVector4D *dstVector = reinterpret_cast<QVector4D *>(dst);
            new (dstVector) QVector4D(*srcVector);
            return true;
            }
        }

        return false;
    }

    bool read(int srcType, const void *src, int dstType, void *dst)
    {
        if (dstType == QMetaType::QColor) {
            QColor *dstColor = reinterpret_cast<QColor *>(dst);
            if (srcType == QMetaType::QColor) {
                const QColor *srcColor = reinterpret_cast<const QColor *>(src);
                *dstColor = *srcColor;
            } else {
                *dstColor = QColor();
            }
            return true;
        }

        return false;
    }

    bool write(int type, const void *src, void *dst, size_t n)
    {
        if (type == QMetaType::QColor) {
            Q_ASSERT(n >= sizeof(QColor));
            const QColor *srcColor = reinterpret_cast<const QColor *>(src);
            QColor *dstColor = reinterpret_cast<QColor *>(dst);
            if (*dstColor != *srcColor) {
                *dstColor = *srcColor;
                return true;
            }
        }

        return false;
    }
};


class QQuickGuiProvider : public QQmlGuiProvider
{
public:
    QQuickApplication *application(QObject *parent)
    {
        return new QQuickApplication(parent);
    }

    QInputMethod *inputMethod()
    {
        return qGuiApp->inputMethod();
    }

    QStringList fontFamilies()
    {
        QFontDatabase database;
        return database.families();
    }

    bool openUrlExternally(QUrl &url)
    {
#ifndef QT_NO_DESKTOPSERVICES
        return QDesktopServices::openUrl(url);
#else
        return false;
#endif
    }
};


static QQuickValueTypeProvider *getValueTypeProvider()
{
    static QQuickValueTypeProvider valueTypeProvider;
    return &valueTypeProvider;
}

static QQuickColorProvider *getColorProvider()
{
    static QQuickColorProvider colorProvider;
    return &colorProvider;
}

static QQuickGuiProvider *getGuiProvider()
{
    static QQuickGuiProvider guiProvider;
    return &guiProvider;
}

static bool initializeProviders()
{
    QQml_addValueTypeProvider(getValueTypeProvider());
    QQml_setColorProvider(getColorProvider());
    QQml_setGuiProvider(getGuiProvider());
    return true;
}

static bool initialized = initializeProviders();

QT_END_NAMESPACE
