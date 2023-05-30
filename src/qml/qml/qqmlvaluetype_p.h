// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLVALUETYPE_P_H
#define QQMLVALUETYPE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQml/private/qqmlproperty_p.h>

#include <private/qqmlnullablevalue_p.h>
#include <private/qmetatype_p.h>
#include <private/qv4referenceobject_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qrect.h>
#if QT_CONFIG(easingcurve)
#include <QtCore/qeasingcurve.h>
#endif
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QQmlValueType : public QDynamicMetaObjectData
{
public:
    QQmlValueType() = default;
    QQmlValueType(QMetaType type, const QMetaObject *staticMetaObject)
        : m_metaType(type), m_staticMetaObject(staticMetaObject)
    {}
    ~QQmlValueType();

    void *create() const { return m_metaType.create(); }
    void destroy(void *gadgetPtr) const { m_metaType.destroy(gadgetPtr); }

    void construct(void *gadgetPtr, const void *copy) const { m_metaType.construct(gadgetPtr, copy); }
    void destruct(void *gadgetPtr) const { m_metaType.destruct(gadgetPtr); }

    QMetaType metaType() const { return m_metaType; }
    const QMetaObject *staticMetaObject() const { return m_staticMetaObject; }

    // ---- dynamic meta object data interface
    QMetaObject *toDynamicMetaObject(QObject *) override;
    void objectDestroyed(QObject *) override;
    int metaCall(QObject *obj, QMetaObject::Call type, int _id, void **argv) override;
    // ----

private:
    QMetaType m_metaType;
    const QMetaObject *m_staticMetaObject = nullptr;
    QMetaObject *m_dynamicMetaObject = nullptr;
};

class Q_QML_PRIVATE_EXPORT QQmlGadgetPtrWrapper : public QObject
{
    Q_OBJECT
public:
    static QQmlGadgetPtrWrapper *instance(QQmlEngine *engine, QMetaType type);

    QQmlGadgetPtrWrapper(QQmlValueType *valueType, QObject *parent = nullptr);
    ~QQmlGadgetPtrWrapper();

    void read(QObject *obj, int idx);
    void write(QObject *obj, int idx, QQmlPropertyData::WriteFlags flags,
               int internalIndex = QV4::ReferenceObject::AllProperties) const;
    QVariant value() const;
    void setValue(const QVariant &value);

    QMetaType metaType() const { return valueType()->metaType(); }
    int metaCall(QMetaObject::Call type, int id, void **argv);

    QMetaProperty property(int index) const
    {
        return valueType()->staticMetaObject()->property(index);
    }

    QVariant readOnGadget(const QMetaProperty &property) const
    {
        return property.readOnGadget(m_gadgetPtr);
    }

    void writeOnGadget(const QMetaProperty &property, const QVariant &value)
    {
        property.writeOnGadget(m_gadgetPtr, value);
    }

    void writeOnGadget(const QMetaProperty &property, QVariant &&value)
    {
        property.writeOnGadget(m_gadgetPtr, std::move(value));
    }

private:
    const QQmlValueType *valueType() const;
    void *m_gadgetPtr = nullptr;
};

struct Q_QML_PRIVATE_EXPORT QQmlPointFValueType
{
    QPointF v;
    Q_PROPERTY(qreal x READ x WRITE setX FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY FINAL)
    Q_GADGET
    QML_VALUE_TYPE(point)
    QML_FOREIGN(QPointF)
    QML_ADDED_IN_VERSION(2, 0)
    QML_EXTENDED(QQmlPointFValueType)
    QML_STRUCTURED_VALUE

public:
    QQmlPointFValueType() = default;
    Q_INVOKABLE QQmlPointFValueType(const QPoint &point) : v(point) {}
    Q_INVOKABLE QString toString() const;
    qreal x() const;
    qreal y() const;
    void setX(qreal);
    void setY(qreal);

    operator QPointF() const { return v; }
};

struct Q_QML_PRIVATE_EXPORT QQmlPointValueType
{
    QPoint v;
    Q_PROPERTY(int x READ x WRITE setX FINAL)
    Q_PROPERTY(int y READ y WRITE setY FINAL)
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QPoint)
    QML_ADDED_IN_VERSION(2, 0)
    QML_EXTENDED(QQmlPointValueType)
    QML_STRUCTURED_VALUE

public:
    QQmlPointValueType() = default;
    Q_INVOKABLE QQmlPointValueType(const QPointF &point) : v(point.toPoint()) {}
    Q_INVOKABLE QString toString() const;
    int x() const;
    int y() const;
    void setX(int);
    void setY(int);

    operator QPoint() const { return v; }
};

struct Q_QML_PRIVATE_EXPORT QQmlSizeFValueType
{
    QSizeF v;
    Q_PROPERTY(qreal width READ width WRITE setWidth FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight FINAL)
    Q_GADGET
    QML_VALUE_TYPE(size)
    QML_FOREIGN(QSizeF)
    QML_ADDED_IN_VERSION(2, 0)
    QML_EXTENDED(QQmlSizeFValueType)
    QML_STRUCTURED_VALUE

public:
    QQmlSizeFValueType() = default;
    Q_INVOKABLE QQmlSizeFValueType(const QSize &size) : v(size) {}
    Q_INVOKABLE QString toString() const;
    qreal width() const;
    qreal height() const;
    void setWidth(qreal);
    void setHeight(qreal);

    operator QSizeF() const { return v; }
};

struct Q_QML_PRIVATE_EXPORT QQmlSizeValueType
{
    QSize v;
    Q_PROPERTY(int width READ width WRITE setWidth FINAL)
    Q_PROPERTY(int height READ height WRITE setHeight FINAL)
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QSize)
    QML_ADDED_IN_VERSION(2, 0)
    QML_EXTENDED(QQmlSizeValueType)
    QML_STRUCTURED_VALUE

public:
    QQmlSizeValueType() = default;
    Q_INVOKABLE QQmlSizeValueType(const QSizeF &size) : v(size.toSize()) {}
    Q_INVOKABLE QString toString() const;
    int width() const;
    int height() const;
    void setWidth(int);
    void setHeight(int);

    operator QSize() const { return v; }
};

struct Q_QML_PRIVATE_EXPORT QQmlRectFValueType
{
    QRectF v;
    Q_PROPERTY(qreal x READ x WRITE setX FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY FINAL)
    Q_PROPERTY(qreal width READ width WRITE setWidth FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight FINAL)
    Q_PROPERTY(qreal left READ left DESIGNABLE false FINAL)
    Q_PROPERTY(qreal right READ right DESIGNABLE false FINAL)
    Q_PROPERTY(qreal top READ top DESIGNABLE false FINAL)
    Q_PROPERTY(qreal bottom READ bottom DESIGNABLE false FINAL)
    Q_GADGET
    QML_VALUE_TYPE(rect)
    QML_FOREIGN(QRectF)
    QML_ADDED_IN_VERSION(2, 0)
    QML_EXTENDED(QQmlRectFValueType)
    QML_STRUCTURED_VALUE

public:
    QQmlRectFValueType() = default;
    Q_INVOKABLE QQmlRectFValueType(const QRect &rect) : v(rect) {}
    Q_INVOKABLE QString toString() const;
    qreal x() const;
    qreal y() const;
    void setX(qreal);
    void setY(qreal);

    qreal width() const;
    qreal height() const;
    void setWidth(qreal);
    void setHeight(qreal);

    qreal left() const;
    qreal right() const;
    qreal top() const;
    qreal bottom() const;

    operator QRectF() const { return v; }
};

struct Q_QML_PRIVATE_EXPORT QQmlRectValueType
{
    QRect v;
    Q_PROPERTY(int x READ x WRITE setX FINAL)
    Q_PROPERTY(int y READ y WRITE setY FINAL)
    Q_PROPERTY(int width READ width WRITE setWidth FINAL)
    Q_PROPERTY(int height READ height WRITE setHeight FINAL)
    Q_PROPERTY(int left READ left DESIGNABLE false FINAL)
    Q_PROPERTY(int right READ right DESIGNABLE false FINAL)
    Q_PROPERTY(int top READ top DESIGNABLE false FINAL)
    Q_PROPERTY(int bottom READ bottom DESIGNABLE false FINAL)
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QRect)
    QML_ADDED_IN_VERSION(2, 0)
    QML_EXTENDED(QQmlRectValueType)
    QML_STRUCTURED_VALUE

public:
    QQmlRectValueType() = default;
    Q_INVOKABLE QQmlRectValueType(const QRectF &rect) : v(rect.toRect()) {}
    Q_INVOKABLE QString toString() const;
    int x() const;
    int y() const;
    void setX(int);
    void setY(int);

    int width() const;
    int height() const;
    void setWidth(int);
    void setHeight(int);

    int left() const;
    int right() const;
    int top() const;
    int bottom() const;

    operator QRect() const { return v; }
};

#if QT_CONFIG(easingcurve)
namespace QQmlEasingEnums
{
Q_NAMESPACE_EXPORT(Q_QML_PRIVATE_EXPORT)
QML_NAMED_ELEMENT(Easing)
QML_ADDED_IN_VERSION(2, 0)

enum Type {
    Linear = QEasingCurve::Linear,
    InQuad = QEasingCurve::InQuad, OutQuad = QEasingCurve::OutQuad,
    InOutQuad = QEasingCurve::InOutQuad, OutInQuad = QEasingCurve::OutInQuad,
    InCubic = QEasingCurve::InCubic, OutCubic = QEasingCurve::OutCubic,
    InOutCubic = QEasingCurve::InOutCubic, OutInCubic = QEasingCurve::OutInCubic,
    InQuart = QEasingCurve::InQuart, OutQuart = QEasingCurve::OutQuart,
    InOutQuart = QEasingCurve::InOutQuart, OutInQuart = QEasingCurve::OutInQuart,
    InQuint = QEasingCurve::InQuint, OutQuint = QEasingCurve::OutQuint,
    InOutQuint = QEasingCurve::InOutQuint, OutInQuint = QEasingCurve::OutInQuint,
    InSine = QEasingCurve::InSine, OutSine = QEasingCurve::OutSine,
    InOutSine = QEasingCurve::InOutSine, OutInSine = QEasingCurve::OutInSine,
    InExpo = QEasingCurve::InExpo, OutExpo = QEasingCurve::OutExpo,
    InOutExpo = QEasingCurve::InOutExpo, OutInExpo = QEasingCurve::OutInExpo,
    InCirc = QEasingCurve::InCirc, OutCirc = QEasingCurve::OutCirc,
    InOutCirc = QEasingCurve::InOutCirc, OutInCirc = QEasingCurve::OutInCirc,
    InElastic = QEasingCurve::InElastic, OutElastic = QEasingCurve::OutElastic,
    InOutElastic = QEasingCurve::InOutElastic, OutInElastic = QEasingCurve::OutInElastic,
    InBack = QEasingCurve::InBack, OutBack = QEasingCurve::OutBack,
    InOutBack = QEasingCurve::InOutBack, OutInBack = QEasingCurve::OutInBack,
    InBounce = QEasingCurve::InBounce, OutBounce = QEasingCurve::OutBounce,
    InOutBounce = QEasingCurve::InOutBounce, OutInBounce = QEasingCurve::OutInBounce,
    InCurve = QEasingCurve::InCurve, OutCurve = QEasingCurve::OutCurve,
    SineCurve = QEasingCurve::SineCurve, CosineCurve = QEasingCurve::CosineCurve,
    BezierSpline = QEasingCurve::BezierSpline,

    Bezier = BezierSpline // Evil! Don't use this!
};
Q_ENUM_NS(Type)
};

struct Q_QML_PRIVATE_EXPORT QQmlEasingValueType
{
    QEasingCurve v;
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QEasingCurve)
    QML_ADDED_IN_VERSION(2, 0)
    QML_EXTENDED(QQmlEasingValueType)
    QML_STRUCTURED_VALUE

    Q_PROPERTY(QQmlEasingEnums::Type type READ type WRITE setType FINAL)
    Q_PROPERTY(qreal amplitude READ amplitude WRITE setAmplitude FINAL)
    Q_PROPERTY(qreal overshoot READ overshoot WRITE setOvershoot FINAL)
    Q_PROPERTY(qreal period READ period WRITE setPeriod FINAL)
    Q_PROPERTY(QVariantList bezierCurve READ bezierCurve WRITE setBezierCurve FINAL)

public:
    QQmlEasingEnums::Type type() const;
    qreal amplitude() const;
    qreal overshoot() const;
    qreal period() const;
    void setType(QQmlEasingEnums::Type);
    void setAmplitude(qreal);
    void setOvershoot(qreal);
    void setPeriod(qreal);
    void setBezierCurve(const QVariantList &);
    QVariantList bezierCurve() const;

    operator QEasingCurve() const { return v; }
};
#endif

QT_END_NAMESPACE

#endif  // QQMLVALUETYPE_P_H
