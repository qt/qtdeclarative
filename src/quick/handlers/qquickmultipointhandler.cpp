// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmultipointhandler_p.h"
#include "qquickmultipointhandler_p_p.h"
#include <private/qquickitem_p.h>
#include <QLineF>
#include <QMouseEvent>
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmltype MultiPointHandler
    \since 5.10
    \preliminary
    \instantiates QQuickMultiPointHandler
    \inherits PointerDeviceHandler
    \inqmlmodule QtQuick
    \brief Abstract handler for multi-point Pointer Events.

    An intermediate class (not registered as a QML type)
    for any type of handler which requires and acts upon a specific number
    of multiple touchpoints.
*/
QQuickMultiPointHandler::QQuickMultiPointHandler(QQuickItem *parent, int minimumPointCount, int maximumPointCount)
    : QQuickPointerDeviceHandler(*(new QQuickMultiPointHandlerPrivate(minimumPointCount, maximumPointCount)), parent)
{
}

bool QQuickMultiPointHandler::wantsPointerEvent(QPointerEvent *event)
{
    Q_D(QQuickMultiPointHandler);
    if (!QQuickPointerDeviceHandler::wantsPointerEvent(event))
        return false;

    if (event->type() == QEvent::Wheel)
        return false;

    bool ret = false;
#if QT_CONFIG(gestures)
    if (event->type() == QEvent::NativeGesture && event->point(0).state() != QEventPoint::Released)
        ret = true;
#endif

    // If points were pressed or released within parentItem, reset stored state
    // and check eligible points again. This class of handlers is intended to
    // handle a specific number of points, so a differing number of points will
    // usually result in different behavior. But otherwise if the currentPoints
    // are all still there in the event, we're good to go (do not reset
    // currentPoints, because we don't want to lose the pressPosition, and do
    // not want to reshuffle the order either).
    const auto candidatePoints = eligiblePoints(event);
    if (candidatePoints.size() != d->currentPoints.size()) {
        d->currentPoints.clear();
        if (active()) {
            setActive(false);
            d->centroid.reset();
            emit centroidChanged();
        }
    } else if (hasCurrentPoints(event)) {
        return true;
    }

    ret = ret || (candidatePoints.size() >= minimumPointCount() && candidatePoints.size() <= maximumPointCount());
    if (ret) {
        const int c = candidatePoints.size();
        d->currentPoints.resize(c);
        for (int i = 0; i < c; ++i) {
            d->currentPoints[i].reset(event, candidatePoints[i]);
            if (auto par = parentItem())
                d->currentPoints[i].localize(par);
        }
    } else {
        d->currentPoints.clear();
    }
    return ret;
}

void QQuickMultiPointHandler::handlePointerEventImpl(QPointerEvent *event)
{
    Q_D(QQuickMultiPointHandler);
    QQuickPointerHandler::handlePointerEventImpl(event);
    // event's points can be reordered since the previous event, which is why currentPoints
    // is _not_ a shallow copy of the QQuickPointerTouchEvent::m_touchPoints vector.
    // So we have to update our currentPoints instances based on the given event.
    for (QQuickHandlerPoint &p : d->currentPoints) {
        if (const QEventPoint *ep = event->pointById(p.id()))
            p.reset(event, *ep);
    }
    QPointF sceneGrabPos = d->centroid.sceneGrabPosition();
    d->centroid.reset(d->currentPoints);
    d->centroid.m_sceneGrabPosition = sceneGrabPos; // preserve as it was
    emit centroidChanged();
}

void QQuickMultiPointHandler::onActiveChanged()
{
    Q_D(QQuickMultiPointHandler);
    if (active()) {
        d->centroid.m_sceneGrabPosition = d->centroid.m_scenePosition;
    } else {
        // Don't call centroid.reset() here, because in a QML onActiveChanged
        // callback, we'd like to see what the position _was_, what the velocity _was_, etc.
        // (having them undefined is not useful)
        // But pressedButtons and pressedModifiers are meant to be more real-time than those
        // (which seems a bit inconsistent, from one side).
        d->centroid.m_pressedButtons = Qt::NoButton;
        d->centroid.m_pressedModifiers = Qt::NoModifier;
    }
}

void QQuickMultiPointHandler::onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition, QPointerEvent *event, QEventPoint &point)
{
    Q_D(QQuickMultiPointHandler);
    // If another handler or item takes over this set of points, assume it has
    // decided that it's the better fit for them. Don't immediately re-grab
    // at the next opportunity. This should help to avoid grab cycles
    // (e.g. between DragHandler and PinchHandler).
    if (transition == QPointingDevice::UngrabExclusive || transition == QPointingDevice::CancelGrabExclusive)
        d->currentPoints.clear();
    if (grabber != this)
        return;
    switch (transition) {
    case QPointingDevice::GrabExclusive:
        for (auto &pt : d->currentPoints)
            if (pt.id() == point.id()) {
                pt.m_sceneGrabPosition = point.scenePosition();
                break;
            }
        QQuickPointerHandler::onGrabChanged(grabber, transition, event, point);
        break;
    case QPointingDevice::GrabPassive:
    case QPointingDevice::UngrabPassive:
    case QPointingDevice::UngrabExclusive:
    case QPointingDevice::CancelGrabPassive:
    case QPointingDevice::CancelGrabExclusive:
        QQuickPointerHandler::onGrabChanged(grabber, transition, event, point);
        break;
    case QPointingDevice::OverrideGrabPassive:
        return; // don't emit
    }
}

QVector<QEventPoint> QQuickMultiPointHandler::eligiblePoints(QPointerEvent *event)
{
    QVector<QEventPoint> ret;
    // If one or more points are newly pressed or released, all non-released points are candidates for this handler.
    // In other cases however, check whether it would be OK to steal the grab if the handler chooses to do that.
    bool stealingAllowed = event->isBeginEvent() || event->isEndEvent();
    for (int i = 0; i < event->pointCount(); ++i) {
        auto &p = event->point(i);
        if (QQuickDeliveryAgentPrivate::isMouseEvent(event)) {
            if (static_cast<QMouseEvent *>(event)->buttons() == Qt::NoButton)
                continue;
        }
        if (!stealingAllowed) {
            QObject *exclusiveGrabber = event->exclusiveGrabber(p);
            if (exclusiveGrabber && exclusiveGrabber != this && !canGrab(event, p))
                continue;
        }
        if (p.state() != QEventPoint::Released && wantsEventPoint(event, p))
            ret << p;
    }
    return ret;
}

/*!
     \qmlproperty int MultiPointHandler::minimumPointCount

     The minimum number of touchpoints required to activate this handler.

     If a smaller number of touchpoints are in contact with the
     \l {PointerHandler::parent}{parent}, they will be ignored.

     Any ignored points are eligible to activate other Input Handlers that
     have different constraints, on the same Item or on other Items.

     The default value is 2.
*/
int QQuickMultiPointHandler::minimumPointCount() const
{
    Q_D(const QQuickMultiPointHandler);
    return d->minimumPointCount;
}

void QQuickMultiPointHandler::setMinimumPointCount(int c)
{
    Q_D(QQuickMultiPointHandler);
    if (d->minimumPointCount == c)
        return;

    d->minimumPointCount = c;
    emit minimumPointCountChanged();
    if (d->maximumPointCount < 0)
        emit maximumPointCountChanged();
}

/*!
     \qmlproperty int MultiPointHandler::maximumPointCount

     The maximum number of touchpoints this handler can utilize.

     If a larger number of touchpoints are in contact with the
     \l {PointerHandler::parent}{parent}, the required number of points will be
     chosen in the order that they are pressed, and the remaining points will
     be ignored.

     Any ignored points are eligible to activate other Input Handlers that
     have different constraints, on the same Item or on other Items.

     The default value is the same as \l minimumPointCount.
*/
int QQuickMultiPointHandler::maximumPointCount() const
{
    Q_D(const QQuickMultiPointHandler);
    return d->maximumPointCount >= 0 ? d->maximumPointCount : d->minimumPointCount;
}

void QQuickMultiPointHandler::setMaximumPointCount(int maximumPointCount)
{
    Q_D(QQuickMultiPointHandler);
    if (d->maximumPointCount == maximumPointCount)
        return;

    d->maximumPointCount = maximumPointCount;
    emit maximumPointCountChanged();
}

/*!
    \readonly
    \qmlproperty QtQuick::handlerPoint QtQuick::MultiPointHandler::centroid

    A point exactly in the middle of the currently-pressed touch points.
    If only one point is pressed, it's the same as that point.
    A handler that has a \l target will normally transform it relative to this point.
*/
const QQuickHandlerPoint &QQuickMultiPointHandler::centroid() const
{
    Q_D(const QQuickMultiPointHandler);
    return d->centroid;
}

/*!
    Returns a modifiable reference to the point that will be returned by the
    \l centroid property. If you modify it, you are responsible to emit
    \l centroidChanged.
*/
QQuickHandlerPoint &QQuickMultiPointHandler::mutableCentroid()
{
    Q_D(QQuickMultiPointHandler);
    return d->centroid;
}

QVector<QQuickHandlerPoint> &QQuickMultiPointHandler::currentPoints()
{
    Q_D(QQuickMultiPointHandler);
    return d->currentPoints;
}

bool QQuickMultiPointHandler::hasCurrentPoints(QPointerEvent *event)
{
    Q_D(const QQuickMultiPointHandler);
    if (event->pointCount() < d->currentPoints.size() || d->currentPoints.size() == 0)
        return false;
    // TODO optimize: either ensure the points are sorted,
    // or use std::equal with a predicate
    for (const QQuickHandlerPoint &p : std::as_const(d->currentPoints)) {
        const QEventPoint *ep = event->pointById(p.id());
        if (!ep)
            return false;
        if (ep->state() == QEventPoint::Released)
            return false;
    }
    return true;
}

qreal QQuickMultiPointHandler::averageTouchPointDistance(const QPointF &ref)
{
    Q_D(const QQuickMultiPointHandler);
    qreal ret = 0;
    if (Q_UNLIKELY(d->currentPoints.size() == 0))
        return ret;
    for (const QQuickHandlerPoint &p : d->currentPoints)
        ret += QVector2D(p.scenePosition() - ref).length();
    return ret / d->currentPoints.size();
}

qreal QQuickMultiPointHandler::averageStartingDistance(const QPointF &ref)
{
    Q_D(const QQuickMultiPointHandler);
    // TODO cache it in setActive()?
    qreal ret = 0;
    if (Q_UNLIKELY(d->currentPoints.size() == 0))
        return ret;
    for (const QQuickHandlerPoint &p : d->currentPoints)
        ret += QVector2D(p.sceneGrabPosition() - ref).length();
    return ret / d->currentPoints.size();
}

QVector<QQuickMultiPointHandler::PointData> QQuickMultiPointHandler::angles(const QPointF &ref) const
{
    Q_D(const QQuickMultiPointHandler);
    QVector<PointData> angles;
    angles.reserve(d->currentPoints.size());
    for (const QQuickHandlerPoint &p : d->currentPoints) {
        qreal angle = QLineF(ref, p.scenePosition()).angle();
        angles.append(PointData(p.id(), -angle));     // convert to clockwise, to be consistent with QQuickItem::rotation
    }
    return angles;
}

qreal QQuickMultiPointHandler::averageAngleDelta(const QVector<PointData> &old, const QVector<PointData> &newAngles)
{
    qreal avgAngleDelta = 0;
    int numSamples = 0;

    auto oldBegin = old.constBegin();

    for (PointData newData : newAngles) {
        quint64 id = newData.id;
        auto it = std::find_if(oldBegin, old.constEnd(), [id] (PointData pd) { return pd.id == id; });
        qreal angleD = 0;
        if (it != old.constEnd()) {
            PointData oldData = *it;
            // We might rotate from 359 degrees to 1 degree. However, this
            // should be interpreted as a rotation of +2 degrees instead of
            // -358 degrees. Therefore, we call remainder() to translate the angle
            // to be in the range [-180, 180] (-350 to +10 etc)
            angleD = remainder(newData.angle - oldData.angle, qreal(360));
            // optimization: narrow down the O(n^2) search to optimally O(n)
            // if both vectors have the same points and they are in the same order
            if (it == oldBegin)
                ++oldBegin;
            numSamples++;
        }
        avgAngleDelta += angleD;
    }
    if (numSamples > 1)
        avgAngleDelta /= numSamples;

    return avgAngleDelta;
}

void QQuickMultiPointHandler::acceptPoints(const QVector<QEventPoint> &points)
{
    // "auto point" is a copy, but it's OK because
    // setAccepted() changes QEventPointPrivate::accept via the shared d-pointer
    for (auto point : points)
        point.setAccepted();
}

bool QQuickMultiPointHandler::grabPoints(QPointerEvent *event, const QVector<QEventPoint> &points)
{
    if (points.isEmpty())
        return false;
    bool allowed = true;
    for (auto &point : points) {
        if (event->exclusiveGrabber(point) != this && !canGrab(event, point)) {
            allowed = false;
            break;
        }
    }
    if (allowed) {
        for (const auto &point : std::as_const(points))
            setExclusiveGrab(event, point);
    }
    return allowed;
}

void QQuickMultiPointHandler::moveTarget(QPointF pos)
{
    Q_D(QQuickMultiPointHandler);
    if (QQuickItem *t = target()) {
        d->xMetaProperty().write(t, pos.x());
        d->yMetaProperty().write(t, pos.y());
        d->centroid.m_position = t->mapFromScene(d->centroid.m_scenePosition);
    } else {
        qWarning() << "moveTarget: target is null";
    }
}

QQuickMultiPointHandlerPrivate::QQuickMultiPointHandlerPrivate(int minPointCount, int maxPointCount)
  : QQuickPointerDeviceHandlerPrivate()
  , minimumPointCount(minPointCount)
  , maximumPointCount(maxPointCount)
{
}

QMetaProperty &QQuickMultiPointHandlerPrivate::xMetaProperty() const
{
    Q_Q(const QQuickMultiPointHandler);
    if (!xProperty.isValid() && q->target()) {
        const QMetaObject *targetMeta = q->target()->metaObject();
        xProperty = targetMeta->property(targetMeta->indexOfProperty("x"));
    }
    return xProperty;
}

QMetaProperty &QQuickMultiPointHandlerPrivate::yMetaProperty() const
{
    Q_Q(const QQuickMultiPointHandler);
    if (!yProperty.isValid() && q->target()) {
        const QMetaObject *targetMeta = q->target()->metaObject();
        yProperty = targetMeta->property(targetMeta->indexOfProperty("y"));
    }
    return yProperty;
}

QT_END_NAMESPACE

#include "moc_qquickmultipointhandler_p.cpp"
