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

#include <private/qquickvaluetypes_p.h>
#include <private/qquickapplication_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qv8_p.h>
#include <private/qv8engine_p.h>

#include <QtGui/QGuiApplication>
#include <QtGui/qdesktopservices.h>
#include <QtGui/qfontdatabase.h>

#ifdef Q_CC_MSVC
// MSVC2010 warns about 'unused variable t', even if it's used in t->~T()
#  pragma warning( disable : 4189 )
#endif

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

#if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
    #define ASSERT_VALID_SIZE(size, min) Q_UNUSED(size)
#else
    #define ASSERT_VALID_SIZE(size, min) Q_ASSERT(size >= min)
#endif

    static QVector2D vector2DFromString(const QString &s, bool *ok)
    {
        if (s.count(QLatin1Char(',')) == 1) {
            int index = s.indexOf(QLatin1Char(','));

            bool xGood, yGood;
            float xCoord = s.left(index).toFloat(&xGood);
            float yCoord = s.mid(index+1).toFloat(&yGood);

            if (xGood && yGood) {
                if (ok) *ok = true;
                return QVector2D(xCoord, yCoord);
            }
        }

        if (ok) *ok = false;
        return QVector2D();
    }

    static QVector3D vector3DFromString(const QString &s, bool *ok)
    {
        if (s.count(QLatin1Char(',')) == 2) {
            int index = s.indexOf(QLatin1Char(','));
            int index2 = s.indexOf(QLatin1Char(','), index+1);

            bool xGood, yGood, zGood;
            float xCoord = s.left(index).toFloat(&xGood);
            float yCoord = s.mid(index+1, index2-index-1).toFloat(&yGood);
            float zCoord = s.mid(index2+1).toFloat(&zGood);

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
            float xCoord = s.left(index).toFloat(&xGood);
            float yCoord = s.mid(index+1, index2-index-1).toFloat(&yGood);
            float zCoord = s.mid(index2+1, index3-index2-1).toFloat(&zGood);
            float wCoord = s.mid(index3+1).toFloat(&wGood);

            if (xGood && yGood && zGood && wGood) {
                if (ok) *ok = true;
                return QVector4D(xCoord, yCoord, zCoord, wCoord);
            }
        }

        if (ok) *ok = false;
        return QVector4D();
    }

    static QQuaternion quaternionFromString(const QString &s, bool *ok)
    {
        if (s.count(QLatin1Char(',')) == 3) {
            int index = s.indexOf(QLatin1Char(','));
            int index2 = s.indexOf(QLatin1Char(','), index+1);
            int index3 = s.indexOf(QLatin1Char(','), index2+1);

            bool sGood, xGood, yGood, zGood;
            qreal sCoord = s.left(index).toDouble(&sGood);
            qreal xCoord = s.mid(index+1, index2-index-1).toDouble(&xGood);
            qreal yCoord = s.mid(index2+1, index3-index2-1).toDouble(&yGood);
            qreal zCoord = s.mid(index3+1).toDouble(&zGood);

            if (sGood && xGood && yGood && zGood) {
                if (ok) *ok = true;
                return QQuaternion(sCoord, xCoord, yCoord, zCoord);
            }
        }

        if (ok) *ok = false;
        return QQuaternion();
    }

    static QMatrix4x4 matrix4x4FromString(const QString &s, bool *ok)
    {
        if (s.count(QLatin1Char(',')) == 15) {
            float matValues[16];
            bool vOK = true;
            QString mutableStr = s;
            for (int i = 0; vOK && i < 16; ++i) {
                int cidx = mutableStr.indexOf(QLatin1Char(','));
                matValues[i] = mutableStr.left(cidx).toDouble(&vOK);
                mutableStr = mutableStr.mid(cidx + 1);
            }

            if (vOK) {
                if (ok) *ok = true;
                return QMatrix4x4(matValues);
            }
        }

        if (ok) *ok = false;
        return QMatrix4x4();
    }

    static QFont fontFromObject(QQmlV8Handle object, QV8Engine *e, bool *ok)
    {
        if (ok) *ok = false;
        QFont retn;
        v8::Handle<v8::Object> obj = object.toHandle()->ToObject();

        v8::Handle<v8::Value> vbold = obj->Get(v8::String::New("bold"));
        v8::Handle<v8::Value> vcap = obj->Get(v8::String::New("capitalization"));
        v8::Handle<v8::Value> vfam = obj->Get(v8::String::New("family"));
        v8::Handle<v8::Value> vital = obj->Get(v8::String::New("italic"));
        v8::Handle<v8::Value> vlspac = obj->Get(v8::String::New("letterSpacing"));
        v8::Handle<v8::Value> vpixsz = obj->Get(v8::String::New("pixelSize"));
        v8::Handle<v8::Value> vpntsz = obj->Get(v8::String::New("pointSize"));
        v8::Handle<v8::Value> vstrk = obj->Get(v8::String::New("strikeout"));
        v8::Handle<v8::Value> vundl = obj->Get(v8::String::New("underline"));
        v8::Handle<v8::Value> vweight = obj->Get(v8::String::New("weight"));
        v8::Handle<v8::Value> vwspac = obj->Get(v8::String::New("wordSpacing"));

        // pull out the values, set ok to true if at least one valid field is given.
        if (!vbold.IsEmpty() && !vbold->IsNull() && !vbold->IsUndefined() && vbold->IsBoolean()) {
            retn.setBold(vbold->BooleanValue());
            if (ok) *ok = true;
        }
        if (!vcap.IsEmpty() && !vcap->IsNull() && !vcap->IsUndefined() && vcap->IsInt32()) {
            retn.setCapitalization(static_cast<QFont::Capitalization>(vcap->Int32Value()));
            if (ok) *ok = true;
        }
        if (!vfam.IsEmpty() && !vfam->IsNull() && !vfam->IsUndefined() && vfam->IsString()) {
            retn.setFamily(e->toString(vfam->ToString()));
            if (ok) *ok = true;
        }
        if (!vital.IsEmpty() && !vital->IsNull() && !vital->IsUndefined() && vital->IsBoolean()) {
            retn.setItalic(vital->BooleanValue());
            if (ok) *ok = true;
        }
        if (!vlspac.IsEmpty() && !vlspac->IsNull() && !vlspac->IsUndefined() && vlspac->IsNumber()) {
            retn.setLetterSpacing(QFont::AbsoluteSpacing, vlspac->NumberValue());
            if (ok) *ok = true;
        }
        if (!vpixsz.IsEmpty() && !vpixsz->IsNull() && !vpixsz->IsUndefined() && vpixsz->IsInt32()) {
            retn.setPixelSize(vpixsz->Int32Value());
            if (ok) *ok = true;
        }
        if (!vpntsz.IsEmpty() && !vpntsz->IsNull() && !vpntsz->IsUndefined() && vpntsz->IsNumber()) {
            retn.setPointSize(vpntsz->NumberValue());
            if (ok) *ok = true;
        }
        if (!vstrk.IsEmpty() && !vstrk->IsNull() && !vstrk->IsUndefined() && vstrk->IsBoolean()) {
            retn.setStrikeOut(vstrk->BooleanValue());
            if (ok) *ok = true;
        }
        if (!vundl.IsEmpty() && !vundl->IsNull() && !vundl->IsUndefined() && vundl->IsBoolean()) {
            retn.setUnderline(vundl->BooleanValue());
            if (ok) *ok = true;
        }
        if (!vweight.IsEmpty() && !vweight->IsNull() && !vweight->IsUndefined() && vweight->IsInt32()) {
            retn.setWeight(static_cast<QFont::Weight>(vweight->Int32Value()));
            if (ok) *ok = true;
        }
        if (!vwspac.IsEmpty() && !vwspac->IsNull() && !vwspac->IsUndefined() && vwspac->IsNumber()) {
            retn.setWordSpacing(vwspac->NumberValue());
            if (ok) *ok = true;
        }

        return retn;
    }

    static QMatrix4x4 matrix4x4FromObject(QQmlV8Handle object, bool *ok)
    {
        if (ok) *ok = false;
        v8::Handle<v8::Object> obj = object.toHandle()->ToObject();
        if (!obj->IsArray())
            return QMatrix4x4();

        v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(obj);
        if (array->Length() != 16)
            return QMatrix4x4();

        float matVals[16];
        for (uint32_t i = 0; i < 16; ++i) {
            v8::Handle<v8::Value> v = array->Get(i);
            if (!v->IsNumber())
                return QMatrix4x4();
            matVals[i] = v->NumberValue();
        }

        if (ok) *ok = true;
        return QMatrix4x4(matVals);
    }

    template<typename T>
    bool typedCreate(QQmlValueType *&v)
    {
        v = new T;
        return true;
    }

    bool create(int type, QQmlValueType *&v)
    {
        switch (type) {
        case QMetaType::QColor:
            return typedCreate<QQuickColorValueType>(v);
        case QMetaType::QFont:
            return typedCreate<QQuickFontValueType>(v);
        case QMetaType::QVector2D:
            return typedCreate<QQuickVector2DValueType>(v);
        case QMetaType::QVector3D:
            return typedCreate<QQuickVector3DValueType>(v);
        case QMetaType::QVector4D:
            return typedCreate<QQuickVector4DValueType>(v);
        case QMetaType::QQuaternion:
            return typedCreate<QQuickQuaternionValueType>(v);
        case QMetaType::QMatrix4x4:
            return typedCreate<QQuickMatrix4x4ValueType>(v);
        default:
            break;
        }

        return false;
    }

    template<typename T>
    bool typedInit(void *data, size_t dataSize)
    {
        ASSERT_VALID_SIZE(dataSize, sizeof(T));
        T *t = reinterpret_cast<T *>(data);
        new (t) T();
        return true;
    }

    bool init(int type, void *data, size_t dataSize)
    {
        switch (type) {
        case QMetaType::QColor:
            return typedInit<QColor>(data, dataSize);
        case QMetaType::QFont:
            return typedInit<QFont>(data, dataSize);
        case QMetaType::QVector2D:
            return typedInit<QVector2D>(data, dataSize);
        case QMetaType::QVector3D:
            return typedInit<QVector3D>(data, dataSize);
        case QMetaType::QVector4D:
            return typedInit<QVector4D>(data, dataSize);
        case QMetaType::QQuaternion:
            return typedInit<QQuaternion>(data, dataSize);
        case QMetaType::QMatrix4x4:
            {
            if (dataSize >= sizeof(QMatrix4x4))
                return typedInit<QMatrix4x4>(data, dataSize);

            // special case: init matrix-containing qvariant.
            Q_ASSERT(dataSize >= sizeof(QVariant));
            QVariant *matvar = reinterpret_cast<QVariant *>(data);
            new (matvar) QVariant(QMatrix4x4());
            return true;
            }
        default: break;
        }

        return false;
    }

    template<typename T>
    bool typedDestroy(void *data, size_t dataSize)
    {
        ASSERT_VALID_SIZE(dataSize, sizeof(T));
        T *t = reinterpret_cast<T *>(data);
        t->~T();
        return true;
    }

    bool destroy(int type, void *data, size_t dataSize)
    {
        switch (type) {
        case QMetaType::QColor:
            return typedDestroy<QColor>(data, dataSize);
        case QMetaType::QFont:
            return typedDestroy<QFont>(data, dataSize);
        case QMetaType::QVector2D:
            return typedDestroy<QVector2D>(data, dataSize);
        case QMetaType::QVector3D:
            return typedDestroy<QVector3D>(data, dataSize);
        case QMetaType::QVector4D:
            return typedDestroy<QVector4D>(data, dataSize);
        case QMetaType::QQuaternion:
            return typedDestroy<QQuaternion>(data, dataSize);
        case QMetaType::QMatrix4x4:
            {
            if (dataSize >= sizeof(QMatrix4x4))
                return typedDestroy<QMatrix4x4>(data, dataSize);

            // special case: destroying matrix-containing qvariant.
            Q_ASSERT(dataSize >= sizeof(QVariant));
            QVariant *matvar = reinterpret_cast<QVariant *>(data);
            matvar->~QVariant();
            return true;
            }
        default: break;
        }

        return false;
    }

    template<typename T>
    bool typedCopyConstruct(const void *src, void *dst, size_t dstSize)
    {
        ASSERT_VALID_SIZE(dstSize, sizeof(T));
        const T *srcT = reinterpret_cast<const T *>(src);
        T *destT = reinterpret_cast<T *>(dst);
        new (destT) T(*srcT);
        return true;
    }

    bool copy(int type, const void *src, void *dst, size_t dstSize)
    {
        switch (type) {
        case QMetaType::QColor:
            return typedCopyConstruct<QColor>(src, dst, dstSize);
        case QMetaType::QFont:
            return typedCopyConstruct<QFont>(src, dst, dstSize);
        case QMetaType::QVector2D:
            return typedCopyConstruct<QVector2D>(src, dst, dstSize);
        case QMetaType::QVector3D:
            return typedCopyConstruct<QVector3D>(src, dst, dstSize);
        case QMetaType::QVector4D:
            return typedCopyConstruct<QVector4D>(src, dst, dstSize);
        case QMetaType::QQuaternion:
            return typedCopyConstruct<QQuaternion>(src, dst, dstSize);
        case QMetaType::QMatrix4x4:
            {
            if (dstSize >= sizeof(QMatrix4x4))
                return typedCopyConstruct<QMatrix4x4>(src, dst, dstSize);

            // special case: copying matrix into variant.
            Q_ASSERT(dstSize >= sizeof(QVariant));
            const QMatrix4x4 *srcMatrix = reinterpret_cast<const QMatrix4x4 *>(src);
            QVariant *dstMatrixVar = reinterpret_cast<QVariant *>(dst);
            new (dstMatrixVar) QVariant(*srcMatrix);
            return true;
            }
        default: break;
        }

        return false;
    }

    bool create(int type, int argc, const void *argv[], QVariant *v)
    {
        switch (type) {
        case QMetaType::QFont: // must specify via js-object.
            break;
        case QMetaType::QVector2D:
            if (argc == 1) {
                const float *xy = reinterpret_cast<const float*>(argv[0]);
                QVector2D v2(xy[0], xy[1]);
                *v = QVariant(v2);
                return true;
            }
            break;
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
        case QMetaType::QQuaternion:
            if (argc == 1) {
                const qreal *sxyz = reinterpret_cast<const qreal*>(argv[0]);
                QQuaternion q(sxyz[0], sxyz[1], sxyz[2], sxyz[3]);
                *v = QVariant(q);
                return true;
            }
            break;
        case QMetaType::QMatrix4x4:
            if (argc == 1) {
                const qreal *vals = reinterpret_cast<const qreal*>(argv[0]);
                QMatrix4x4 m(vals[0], vals[1], vals[2], vals[3],
                             vals[4], vals[5], vals[6], vals[7],
                             vals[8], vals[9], vals[10], vals[11],
                             vals[12], vals[13], vals[14], vals[15]);
                *v = QVariant(m);
                return true;
            }
            break;
        default: break;
        }

        return false;
    }

    template<typename T>
    bool createFromStringTyped(void *data, size_t dataSize, T initValue)
    {
        ASSERT_VALID_SIZE(dataSize, sizeof(T));
        T *t = reinterpret_cast<T *>(data);
        new (t) T(initValue);
        return true;
    }

    bool createFromString(int type, const QString &s, void *data, size_t dataSize)
    {
        bool ok = false;

        switch (type) {
        case QMetaType::QColor:
            return createFromStringTyped<QColor>(data, dataSize, QQuickColorProvider::QColorFromString(s));
        case QMetaType::QVector2D:
            return createFromStringTyped<QVector2D>(data, dataSize, vector2DFromString(s, &ok));
        case QMetaType::QVector3D:
            return createFromStringTyped<QVector3D>(data, dataSize, vector3DFromString(s, &ok));
        case QMetaType::QVector4D:
            return createFromStringTyped<QVector4D>(data, dataSize, vector4DFromString(s, &ok));
        case QMetaType::QQuaternion:
            return createFromStringTyped<QQuaternion>(data, dataSize, quaternionFromString(s, &ok));
        case QMetaType::QMatrix4x4:
            {
            if (dataSize >= sizeof(QMatrix4x4))
                return createFromStringTyped<QMatrix4x4>(data, dataSize, matrix4x4FromString(s, &ok));

            Q_ASSERT(dataSize >= sizeof(QVariant));
            QVariant *matVar = reinterpret_cast<QVariant *>(data);
            new (matVar) QVariant(matrix4x4FromString(s, &ok));
            return true;
            }
        default: break;
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

        QVector2D v2 = vector2DFromString(s, &ok);
        if (ok) {
            *v = QVariant::fromValue(v2);
            return true;
        }

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

        QQuaternion q = quaternionFromString(s, &ok);
        if (ok) {
            *v = QVariant::fromValue(q);
            return true;
        }

        QMatrix4x4 m = matrix4x4FromString(s, &ok);
        if (ok) {
            *v = QVariant::fromValue(m);
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
        case QMetaType::QVector2D:
            {
            *v = QVariant::fromValue(vector2DFromString(s, &ok));
            return true;
            }
        case QMetaType::QVector3D:
            {
            *v = QVariant::fromValue(vector3DFromString(s, &ok));
            return true;
            }
        case QMetaType::QVector4D:
            {
            *v = QVariant::fromValue(vector4DFromString(s, &ok));
            return true;
            }
        case QMetaType::QQuaternion:
            {
            *v = QVariant::fromValue(quaternionFromString(s, &ok));
            return true;
            }
        case QMetaType::QMatrix4x4:
            {
            *v = QVariant::fromValue(matrix4x4FromString(s, &ok));
            return true;
            }
        default:
            break;
        }

        return false;
    }

    bool variantFromJsObject(int type, QQmlV8Handle object, QV8Engine *e, QVariant *v)
    {
        // must be called with a valid v8 context.
        Q_ASSERT(object.toHandle()->IsObject());
        bool ok = false;
        switch (type) {
        case QMetaType::QFont:
            *v = QVariant::fromValue(fontFromObject(object, e, &ok));
            break;
        case QMetaType::QMatrix4x4:
            *v = QVariant::fromValue(matrix4x4FromObject(object, &ok));
        default: break;
        }

        return ok;
    }

    template<typename T>
    bool typedEqual(const void *lhs, const void *rhs)
    {
        return (*(reinterpret_cast<const T *>(lhs)) == *(reinterpret_cast<const T *>(rhs)));
    }

    bool equal(int type, const void *lhs, const void *rhs, size_t rhsSize)
    {
        switch (type) {
        case QMetaType::QColor:
            return typedEqual<QColor>(lhs, rhs);
        case QMetaType::QFont:
            return typedEqual<QFont>(lhs, rhs);
        case QMetaType::QVector2D:
            return typedEqual<QVector2D>(lhs, rhs);
        case QMetaType::QVector3D:
            return typedEqual<QVector3D>(lhs, rhs);
        case QMetaType::QVector4D:
            return typedEqual<QVector4D>(lhs, rhs);
        case QMetaType::QQuaternion:
            return typedEqual<QQuaternion>(lhs, rhs);
        case QMetaType::QMatrix4x4:
            {
            if (rhsSize >= sizeof(QMatrix4x4))
                return typedEqual<QMatrix4x4>(lhs, rhs);

            Q_ASSERT(rhsSize >= sizeof(QVariant));
            QMatrix4x4 rhsmat = reinterpret_cast<const QVariant *>(rhs)->value<QMatrix4x4>();
            return typedEqual<QMatrix4x4>(lhs, &rhsmat);
            }
        default: break;
        }

        return false;
    }

    template<typename T>
    bool typedStore(const void *src, void *dst, size_t dstSize)
    {
        ASSERT_VALID_SIZE(dstSize, sizeof(T));
        const T *srcT = reinterpret_cast<const T *>(src);
        T *dstT = reinterpret_cast<T *>(dst);
        new (dstT) T(*srcT);
        return true;
    }

    bool store(int type, const void *src, void *dst, size_t dstSize)
    {
        switch (type) {
        case QMetaType::QColor:
            {
            Q_ASSERT(dstSize >= sizeof(QColor));
            const QRgb *rgb = reinterpret_cast<const QRgb *>(src);
            QColor *color = reinterpret_cast<QColor *>(dst);
            new (color) QColor(QColor::fromRgba(*rgb));
            return true;
            }
        case QMetaType::QFont:
            return typedStore<QFont>(src, dst, dstSize);
        case QMetaType::QVector2D:
            return typedStore<QVector2D>(src, dst, dstSize);
        case QMetaType::QVector3D:
            return typedStore<QVector3D>(src, dst, dstSize);
        case QMetaType::QVector4D:
            return typedStore<QVector4D>(src, dst, dstSize);
        case QMetaType::QQuaternion:
            return typedStore<QQuaternion>(src, dst, dstSize);
        case QMetaType::QMatrix4x4:
            {
            if (dstSize >= sizeof(QMatrix4x4))
                return typedStore<QMatrix4x4>(src, dst, dstSize);

            // special case: storing matrix into variant
            // eg, QVMEMO QVMEVariant data cell is big enough to store
            // QVariant, but not large enough to store QMatrix4x4.
            Q_ASSERT(dstSize >= sizeof(QVariant));
            const QMatrix4x4 *srcMat = reinterpret_cast<const QMatrix4x4 *>(src);
            QVariant *dstMatVar = reinterpret_cast<QVariant *>(dst);
            new (dstMatVar) QVariant(*srcMat);
            return true;
            }
        default: break;
        }

        return false;
    }

    template<typename T>
    bool typedRead(int srcType, const void *src, size_t srcSize, int dstType, void *dst)
    {
        T *dstT = reinterpret_cast<T *>(dst);
        if (srcType == dstType) {
            ASSERT_VALID_SIZE(srcSize, sizeof(T));
            const T *srcT = reinterpret_cast<const T *>(src);
            *dstT = *srcT;
        } else {
            *dstT = T();
        }
        return true;
    }

    bool read(int srcType, const void *src, size_t srcSize, int dstType, void *dst)
    {
        switch (dstType) {
        case QMetaType::QColor:
            return typedRead<QColor>(srcType, src, srcSize, dstType, dst);
        case QMetaType::QFont:
            return typedRead<QFont>(srcType, src, srcSize, dstType, dst);
        case QMetaType::QVector2D:
            return typedRead<QVector2D>(srcType, src, srcSize, dstType, dst);
        case QMetaType::QVector3D:
            return typedRead<QVector3D>(srcType, src, srcSize, dstType, dst);
        case QMetaType::QVector4D:
            return typedRead<QVector4D>(srcType, src, srcSize, dstType, dst);
        case QMetaType::QQuaternion:
            return typedRead<QQuaternion>(srcType, src, srcSize, dstType, dst);
        case QMetaType::QMatrix4x4:
            {
            if (srcSize >= sizeof(QMatrix4x4))
                return typedRead<QMatrix4x4>(srcType, src, srcSize, dstType, dst);

            // the source data may be stored in a QVariant.
            QMatrix4x4 *dstMat = reinterpret_cast<QMatrix4x4 *>(dst);
            if (srcType == dstType) {
                Q_ASSERT(srcSize >= sizeof(QVariant));
                const QVariant *srcMatVar = reinterpret_cast<const QVariant *>(src);
                *dstMat = srcMatVar->value<QMatrix4x4>();
            } else {
                *dstMat = QMatrix4x4();
            }
            return true;
            }
        default: break;
        }

        return false;
    }

    template<typename T>
    bool typedWrite(const void *src, void *dst, size_t dstSize)
    {
        ASSERT_VALID_SIZE(dstSize, sizeof(T));
        const T *srcT = reinterpret_cast<const T *>(src);
        T *dstT = reinterpret_cast<T *>(dst);
        if (*dstT != *srcT) {
            *dstT = *srcT;
            return true;
        }
        return false;
    }

    bool write(int type, const void *src, void *dst, size_t dstSize)
    {
        switch (type) {
        case QMetaType::QColor:
            return typedWrite<QColor>(src, dst, dstSize);
        case QMetaType::QFont:
            return typedWrite<QFont>(src, dst, dstSize);
        case QMetaType::QVector2D:
            return typedWrite<QVector2D>(src, dst, dstSize);
        case QMetaType::QVector3D:
            return typedWrite<QVector3D>(src, dst, dstSize);
        case QMetaType::QVector4D:
            return typedWrite<QVector4D>(src, dst, dstSize);
        case QMetaType::QQuaternion:
            return typedWrite<QQuaternion>(src, dst, dstSize);
        case QMetaType::QMatrix4x4:
            {
            if (dstSize >= sizeof(QMatrix4x4))
                return typedWrite<QMatrix4x4>(src, dst, dstSize);

            // special case: storing matrix into variant
            // eg, QVMEMO QVMEVariant data cell is big enough to store
            // QVariant, but not large enough to store QMatrix4x4.
            Q_ASSERT(dstSize >= sizeof(QVariant));
            const QMatrix4x4 *srcMat = reinterpret_cast<const QMatrix4x4 *>(src);
            QVariant *dstMatVar = reinterpret_cast<QVariant *>(dst);
            QMatrix4x4 dstMatVal = dstMatVar->value<QMatrix4x4>();
            if (dstMatVal != *srcMat) {
                *dstMatVar = QVariant(*srcMat);
                return true;
            }
            return false;
            }
        default: break;
        }

        return false;
    }
#undef ASSERT_VALID_SIZE
};


class QQuickGuiProvider : public QQmlGuiProvider
{
public:
    QQuickApplication *application(QObject *parent)
    {
        return new QQuickApplication(parent);
    }

#ifndef QT_NO_IM
    QInputMethod *inputMethod()
    {
        return qGuiApp->inputMethod();
    }
#endif

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

Q_CONSTRUCTOR_FUNCTION(initializeProviders)

QT_END_NAMESPACE
