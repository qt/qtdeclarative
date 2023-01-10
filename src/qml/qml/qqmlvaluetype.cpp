// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlvaluetype_p.h"

#include <QtCore/qmutex.h>
#include <private/qqmlglobal_p.h>
#include <QtCore/qdebug.h>
#include <private/qqmlengine_p.h>
#include <private/qmetaobjectbuilder_p.h>

QT_BEGIN_NAMESPACE

QQmlValueType::~QQmlValueType()
{
    ::free(m_dynamicMetaObject);
}

QQmlGadgetPtrWrapper *QQmlGadgetPtrWrapper::instance(QQmlEngine *engine, QMetaType type)
{
    return engine ? QQmlEnginePrivate::get(engine)->valueTypeInstance(type) : nullptr;
}

QQmlGadgetPtrWrapper::QQmlGadgetPtrWrapper(QQmlValueType *valueType, QObject *parent)
    : QObject(parent), m_gadgetPtr(valueType->create())
{
    QObjectPrivate *d = QObjectPrivate::get(this);
    Q_ASSERT(!d->metaObject);
    d->metaObject = valueType;
}

QQmlGadgetPtrWrapper::~QQmlGadgetPtrWrapper()
{
    QObjectPrivate *d = QObjectPrivate::get(this);
    static_cast<const QQmlValueType *>(d->metaObject)->destroy(m_gadgetPtr);
    d->metaObject = nullptr;
}

void QQmlGadgetPtrWrapper::read(QObject *obj, int idx)
{
    Q_ASSERT(m_gadgetPtr);
    void *a[] = { m_gadgetPtr, nullptr };
    QMetaObject::metacall(obj, QMetaObject::ReadProperty, idx, a);
}

void QQmlGadgetPtrWrapper::write(
        QObject *obj, int idx, QQmlPropertyData::WriteFlags flags, int internalIndex) const
{
    Q_ASSERT(m_gadgetPtr);
    int status = -1;
    void *a[] = { m_gadgetPtr, nullptr, &status, &flags, &internalIndex };
    QMetaObject::metacall(obj, QMetaObject::WriteProperty, idx, a);
}

QVariant QQmlGadgetPtrWrapper::value() const
{
    Q_ASSERT(m_gadgetPtr);
    return QVariant(metaType(), m_gadgetPtr);
}

void QQmlGadgetPtrWrapper::setValue(const QVariant &value)
{
    Q_ASSERT(m_gadgetPtr);
    Q_ASSERT(metaType() == value.metaType());
    const QQmlValueType *type = valueType();
    type->destruct(m_gadgetPtr);
    type->construct(m_gadgetPtr, value.constData());
}

int QQmlGadgetPtrWrapper::metaCall(QMetaObject::Call type, int id, void **argv)
{
    Q_ASSERT(m_gadgetPtr);
    const QMetaObject *metaObject = valueType()->staticMetaObject();
    QQmlMetaObject::resolveGadgetMethodOrPropertyIndex(type, &metaObject, &id);
    metaObject->d.static_metacall(static_cast<QObject *>(m_gadgetPtr), type, id, argv);
    return id;
}

const QQmlValueType *QQmlGadgetPtrWrapper::valueType() const
{
    const QObjectPrivate *d = QObjectPrivate::get(this);
    return static_cast<const QQmlValueType *>(d->metaObject);
}

QMetaObject *QQmlValueType::toDynamicMetaObject(QObject *)
{
    if (!m_dynamicMetaObject) {
        QMetaObjectBuilder builder(m_staticMetaObject);

        // Do not set PropertyAccessInStaticMetaCall here. QQmlGadgetPtrWrapper likes to
        // to intercept the metacalls since it needs to use its gadgetPtr.
        // For QQmlValueType::metaObject() we use the base type that has the flag.

        m_dynamicMetaObject = builder.toMetaObject();
    }
    return m_dynamicMetaObject;
}

void QQmlValueType::objectDestroyed(QObject *)
{
}

int QQmlValueType::metaCall(QObject *object, QMetaObject::Call type, int _id, void **argv)
{
    return static_cast<QQmlGadgetPtrWrapper *>(object)->metaCall(type, _id, argv);
}

QString QQmlPointFValueType::toString() const
{
    return QString::asprintf("QPointF(%g, %g)", v.x(), v.y());
}

qreal QQmlPointFValueType::x() const
{
    return v.x();
}

qreal QQmlPointFValueType::y() const
{
    return v.y();
}

void QQmlPointFValueType::setX(qreal x)
{
    v.setX(x);
}

void QQmlPointFValueType::setY(qreal y)
{
    v.setY(y);
}


QString QQmlPointValueType::toString() const
{
    return QString::asprintf("QPoint(%d, %d)", v.x(), v.y());
}

int QQmlPointValueType::x() const
{
    return v.x();
}

int QQmlPointValueType::y() const
{
    return v.y();
}

void QQmlPointValueType::setX(int x)
{
    v.setX(x);
}

void QQmlPointValueType::setY(int y)
{
    v.setY(y);
}


QString QQmlSizeFValueType::toString() const
{
    return QString::asprintf("QSizeF(%g, %g)", v.width(), v.height());
}

qreal QQmlSizeFValueType::width() const
{
    return v.width();
}

qreal QQmlSizeFValueType::height() const
{
    return v.height();
}

void QQmlSizeFValueType::setWidth(qreal w)
{
    v.setWidth(w);
}

void QQmlSizeFValueType::setHeight(qreal h)
{
    v.setHeight(h);
}


QString QQmlSizeValueType::toString() const
{
    return QString::asprintf("QSize(%d, %d)", v.width(), v.height());
}

int QQmlSizeValueType::width() const
{
    return v.width();
}

int QQmlSizeValueType::height() const
{
    return v.height();
}

void QQmlSizeValueType::setWidth(int w)
{
    v.setWidth(w);
}

void QQmlSizeValueType::setHeight(int h)
{
    v.setHeight(h);
}

QString QQmlRectFValueType::toString() const
{
    return QString::asprintf("QRectF(%g, %g, %g, %g)", v.x(), v.y(), v.width(), v.height());
}

qreal QQmlRectFValueType::x() const
{
    return v.x();
}

qreal QQmlRectFValueType::y() const
{
    return v.y();
}

void QQmlRectFValueType::setX(qreal x)
{
    v.moveLeft(x);
}

void QQmlRectFValueType::setY(qreal y)
{
    v.moveTop(y);
}

qreal QQmlRectFValueType::width() const
{
    return v.width();
}

qreal QQmlRectFValueType::height() const
{
    return v.height();
}

void QQmlRectFValueType::setWidth(qreal w)
{
    v.setWidth(w);
}

void QQmlRectFValueType::setHeight(qreal h)
{
    v.setHeight(h);
}

qreal QQmlRectFValueType::left() const
{
    return v.left();
}

qreal QQmlRectFValueType::right() const
{
    return v.right();
}

qreal QQmlRectFValueType::top() const
{
    return v.top();
}

qreal QQmlRectFValueType::bottom() const
{
    return v.bottom();
}


QString QQmlRectValueType::toString() const
{
    return QString::asprintf("QRect(%d, %d, %d, %d)", v.x(), v.y(), v.width(), v.height());
}

int QQmlRectValueType::x() const
{
    return v.x();
}

int QQmlRectValueType::y() const
{
    return v.y();
}

void QQmlRectValueType::setX(int x)
{
    v.moveLeft(x);
}

void QQmlRectValueType::setY(int y)
{
    v.moveTop(y);
}

int QQmlRectValueType::width() const
{
    return v.width();
}

int QQmlRectValueType::height() const
{
    return v.height();
}

void QQmlRectValueType::setWidth(int w)
{
    v.setWidth(w);
}

void QQmlRectValueType::setHeight(int h)
{
    v.setHeight(h);
}

int QQmlRectValueType::left() const
{
    return v.left();
}

int QQmlRectValueType::right() const
{
    return v.right();
}

int QQmlRectValueType::top() const
{
    return v.top();
}

int QQmlRectValueType::bottom() const
{
    return v.bottom();
}

#if QT_CONFIG(easingcurve)
QQmlEasingEnums::Type QQmlEasingValueType::type() const
{
    return (QQmlEasingEnums::Type)v.type();
}

qreal QQmlEasingValueType::amplitude() const
{
    return v.amplitude();
}

qreal QQmlEasingValueType::overshoot() const
{
    return v.overshoot();
}

qreal QQmlEasingValueType::period() const
{
    return v.period();
}

void QQmlEasingValueType::setType(QQmlEasingEnums::Type type)
{
    v.setType((QEasingCurve::Type)type);
}

void QQmlEasingValueType::setAmplitude(qreal amplitude)
{
    v.setAmplitude(amplitude);
}

void QQmlEasingValueType::setOvershoot(qreal overshoot)
{
    v.setOvershoot(overshoot);
}

void QQmlEasingValueType::setPeriod(qreal period)
{
    v.setPeriod(period);
}

void QQmlEasingValueType::setBezierCurve(const QVariantList &customCurveVariant)
{
    if (customCurveVariant.isEmpty())
        return;

    if ((customCurveVariant.size() % 6) != 0)
        return;

    auto convert = [](const QVariant &v, qreal &r) {
        bool ok;
        r = v.toReal(&ok);
        return ok;
    };

    QEasingCurve newEasingCurve(QEasingCurve::BezierSpline);
    for (int i = 0, ei = customCurveVariant.size(); i < ei; i += 6) {
        qreal c1x, c1y, c2x, c2y, c3x, c3y;
        if (!convert(customCurveVariant.at(i    ), c1x)) return;
        if (!convert(customCurveVariant.at(i + 1), c1y)) return;
        if (!convert(customCurveVariant.at(i + 2), c2x)) return;
        if (!convert(customCurveVariant.at(i + 3), c2y)) return;
        if (!convert(customCurveVariant.at(i + 4), c3x)) return;
        if (!convert(customCurveVariant.at(i + 5), c3y)) return;

        const QPointF c1(c1x, c1y);
        const QPointF c2(c2x, c2y);
        const QPointF c3(c3x, c3y);

        newEasingCurve.addCubicBezierSegment(c1, c2, c3);
    }

    v = newEasingCurve;
}

QVariantList QQmlEasingValueType::bezierCurve() const
{
    QVariantList rv;
    const QVector<QPointF> points = v.toCubicSpline();
    rv.reserve(points.size() * 2);
    for (const auto &point : points)
        rv << QVariant(point.x()) << QVariant(point.y());
    return rv;
}
#endif // easingcurve


QT_END_NAMESPACE

#include "moc_qqmlvaluetype_p.cpp"
