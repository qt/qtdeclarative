// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

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

    const QMetaType m = metaType();
    return m == QMetaType::fromType<QVariant>()
            ? *static_cast<const QVariant *>(m_gadgetPtr)
            : QVariant(m, m_gadgetPtr);
}

void QQmlGadgetPtrWrapper::setValue(const QVariant &value)
{
    Q_ASSERT(m_gadgetPtr);

    const QMetaType m = metaType();
    m.destruct(m_gadgetPtr);
    if (m == QMetaType::fromType<QVariant>()) {
        m.construct(m_gadgetPtr, &value);
    } else {
        Q_ASSERT(m == value.metaType());
        m.construct(m_gadgetPtr, value.constData());
    }
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

#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
const QMetaObject *QQmlValueType::toDynamicMetaObject(QObject *) const
#else
QMetaObject *QQmlValueType::toDynamicMetaObject(QObject *)
#endif
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
    return QString::asprintf("QPointF(%g, %g)", QPointF::x(), QPointF::y());
}

qreal QQmlPointFValueType::x() const
{
    return QPointF::x();
}

qreal QQmlPointFValueType::y() const
{
    return QPointF::y();
}

void QQmlPointFValueType::setX(qreal x)
{
    QPointF::setX(x);
}

void QQmlPointFValueType::setY(qreal y)
{
    QPointF::setY(y);
}


QString QQmlPointValueType::toString() const
{
    return QString::asprintf("QPoint(%d, %d)", QPoint::x(), QPoint::y());
}

int QQmlPointValueType::x() const
{
    return QPoint::x();
}

int QQmlPointValueType::y() const
{
    return QPoint::y();
}

void QQmlPointValueType::setX(int x)
{
    QPoint::setX(x);
}

void QQmlPointValueType::setY(int y)
{
    QPoint::setY(y);
}


QString QQmlSizeFValueType::toString() const
{
    return QString::asprintf("QSizeF(%g, %g)", QSizeF::width(), QSizeF::height());
}

qreal QQmlSizeFValueType::width() const
{
    return QSizeF::width();
}

qreal QQmlSizeFValueType::height() const
{
    return QSizeF::height();
}

void QQmlSizeFValueType::setWidth(qreal w)
{
    QSizeF::setWidth(w);
}

void QQmlSizeFValueType::setHeight(qreal h)
{
    QSizeF::setHeight(h);
}


QString QQmlSizeValueType::toString() const
{
    return QString::asprintf("QSize(%d, %d)", QSize::width(), QSize::height());
}

int QQmlSizeValueType::width() const
{
    return QSize::width();
}

int QQmlSizeValueType::height() const
{
    return QSize::height();
}

void QQmlSizeValueType::setWidth(int w)
{
    QSize::setWidth(w);
}

void QQmlSizeValueType::setHeight(int h)
{
    QSize::setHeight(h);
}

QString QQmlRectFValueType::toString() const
{
    return QString::asprintf(
            "QRectF(%g, %g, %g, %g)", QRectF::x(), QRectF::y(), QRectF::width(), QRectF::height());
}

qreal QQmlRectFValueType::x() const
{
    return QRectF::x();
}

qreal QQmlRectFValueType::y() const
{
    return QRectF::y();
}

void QQmlRectFValueType::setX(qreal x)
{
    QRectF::moveLeft(x);
}

void QQmlRectFValueType::setY(qreal y)
{
    QRectF::moveTop(y);
}

qreal QQmlRectFValueType::width() const
{
    return QRectF::width();
}

qreal QQmlRectFValueType::height() const
{
    return QRectF::height();
}

void QQmlRectFValueType::setWidth(qreal w)
{
    QRectF::setWidth(w);
}

void QQmlRectFValueType::setHeight(qreal h)
{
    QRectF::setHeight(h);
}

qreal QQmlRectFValueType::left() const
{
    return QRectF::left();
}

qreal QQmlRectFValueType::right() const
{
    return QRectF::right();
}

qreal QQmlRectFValueType::top() const
{
    return QRectF::top();
}

qreal QQmlRectFValueType::bottom() const
{
    return QRectF::bottom();
}


QString QQmlRectValueType::toString() const
{
    return QString::asprintf(
            "QRect(%d, %d, %d, %d)", QRect::x(), QRect::y(), QRect::width(), QRect::height());
}

int QQmlRectValueType::x() const
{
    return QRect::x();
}

int QQmlRectValueType::y() const
{
    return QRect::y();
}

void QQmlRectValueType::setX(int x)
{
    QRect::moveLeft(x);
}

void QQmlRectValueType::setY(int y)
{
    QRect::moveTop(y);
}

int QQmlRectValueType::width() const
{
    return QRect::width();
}

int QQmlRectValueType::height() const
{
    return QRect::height();
}

void QQmlRectValueType::setWidth(int w)
{
    QRect::setWidth(w);
}

void QQmlRectValueType::setHeight(int h)
{
    QRect::setHeight(h);
}

int QQmlRectValueType::left() const
{
    return QRect::left();
}

int QQmlRectValueType::right() const
{
    return QRect::right();
}

int QQmlRectValueType::top() const
{
    return QRect::top();
}

int QQmlRectValueType::bottom() const
{
    return QRect::bottom();
}

QString QQmlMarginsFValueType::toString() const
{
    return QString::asprintf(
            "QMarginsF(%g, %g, %g, %g)",
            QMarginsF::left(), QMarginsF::top(), QMarginsF::right(), QMarginsF::bottom());
}

qreal QQmlMarginsFValueType::left() const
{
    return QMarginsF::left();
}

qreal QQmlMarginsFValueType::top() const
{
    return QMarginsF::top();
}

qreal QQmlMarginsFValueType::right() const
{
    return QMarginsF::right();
}

qreal QQmlMarginsFValueType::bottom() const
{
    return QMarginsF::bottom();
}

void QQmlMarginsFValueType::setLeft(qreal left)
{
    QMarginsF::setLeft(left);
}

void QQmlMarginsFValueType::setTop(qreal top)
{
    QMarginsF::setTop(top);
}

void QQmlMarginsFValueType::setRight(qreal right)
{
    QMarginsF::setRight(right);
}

void QQmlMarginsFValueType::setBottom(qreal bottom)
{
    QMarginsF::setBottom(bottom);
}

QString QQmlMarginsValueType::toString() const
{
    return QString::asprintf(
            "QMargins(%d, %d, %d, %d)",
            QMargins::left(), QMargins::top(), QMargins::right(), QMargins::bottom());
}

int QQmlMarginsValueType::left() const
{
    return QMargins::left();
}

int QQmlMarginsValueType::top() const
{
    return QMargins::top();
}

int QQmlMarginsValueType::right() const
{
    return QMargins::right();
}

int QQmlMarginsValueType::bottom() const
{
    return QMargins::bottom();
}

void QQmlMarginsValueType::setLeft(int left)
{
    QMargins::setLeft(left);
}

void QQmlMarginsValueType::setTop(int top)
{
    QMargins::setTop(top);
}

void QQmlMarginsValueType::setRight(int right)
{
    QMargins::setRight(right);
}

void QQmlMarginsValueType::setBottom(int bottom)
{
    QMargins::setBottom(bottom);
}

#if QT_CONFIG(easingcurve)
qreal QQmlEasing::valueForProgress(Type type, qreal progress) const
{
    return QEasingCurve(static_cast<QEasingCurve::Type>(type)).valueForProgress(progress);
}

QQmlEasingValueType::QQmlEasingValueType(QQmlEasing::Type type)
    : QEasingCurve(static_cast<QEasingCurve::Type>(type))
{
}

qreal QQmlEasingValueType::valueForProgress(qreal progress)
{
    return QEasingCurve::valueForProgress(progress);
}

QQmlEasing::Type QQmlEasingValueType::type() const
{
    return (QQmlEasing::Type)QEasingCurve::type();
}

qreal QQmlEasingValueType::amplitude() const
{
    return QEasingCurve::amplitude();
}

qreal QQmlEasingValueType::overshoot() const
{
    return QEasingCurve::overshoot();
}

qreal QQmlEasingValueType::period() const
{
    return QEasingCurve::period();
}

void QQmlEasingValueType::setType(QQmlEasing::Type type)
{
    QEasingCurve::setType((QEasingCurve::Type)type);
}

void QQmlEasingValueType::setAmplitude(qreal amplitude)
{
    QEasingCurve::setAmplitude(amplitude);
}

void QQmlEasingValueType::setOvershoot(qreal overshoot)
{
    QEasingCurve::setOvershoot(overshoot);
}

void QQmlEasingValueType::setPeriod(qreal period)
{
    QEasingCurve::setPeriod(period);
}

void QQmlEasingValueType::setBezierCurve(const QList<qreal> &customCurveVariant)
{
    if (customCurveVariant.isEmpty())
        return;

    if ((customCurveVariant.size() % 6) != 0)
        return;

    QEasingCurve newEasingCurve(QEasingCurve::BezierSpline);
    for (int i = 0, ei = customCurveVariant.size(); i < ei; i += 6) {
        const qreal c1x = customCurveVariant.at(i    );
        const qreal c1y = customCurveVariant.at(i + 1);
        const qreal c2x = customCurveVariant.at(i + 2);
        const qreal c2y = customCurveVariant.at(i + 3);
        const qreal c3x = customCurveVariant.at(i + 4);
        const qreal c3y = customCurveVariant.at(i + 5);

        const QPointF c1(c1x, c1y);
        const QPointF c2(c2x, c2y);
        const QPointF c3(c3x, c3y);

        newEasingCurve.addCubicBezierSegment(c1, c2, c3);
    }

    *this = newEasingCurve;
}

QList<qreal> QQmlEasingValueType::bezierCurve() const
{
    QList<qreal> rv;
    const QList<QPointF> points = QEasingCurve::toCubicSpline();
    rv.reserve(points.size() * 2);
    for (const auto &point : points)
        rv << point.x() << point.y();
    return rv;
}
#endif // easingcurve


QT_END_NAMESPACE

#include "moc_qqmlvaluetype_p.cpp"
