/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickmultipointhandler_p.h"
#include <private/qquickitem_p.h>
#include <QLineF>
#include <QMouseEvent>
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    An intermediate class (not registered as a QML type)
    for the type of handler which requires and acts upon a specific number
    of multiple touchpoints.
*/
QQuickMultiPointHandler::QQuickMultiPointHandler(QObject *parent, int requiredPointCount)
    : QQuickPointerDeviceHandler(parent)
    , m_requiredPointCount(requiredPointCount)
    , m_pointDistanceThreshold(0)
{
}

QQuickMultiPointHandler::~QQuickMultiPointHandler()
{
}

bool QQuickMultiPointHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    if (!QQuickPointerDeviceHandler::wantsPointerEvent(event))
        return false;

    if (sameAsCurrentPoints(event))
        return true;

    const QVector<QQuickEventPoint *> candidatePoints = eligiblePoints(event);
    const bool ret = (candidatePoints.size() == m_requiredPointCount);
    if (ret)
        m_currentPoints = candidatePoints;
    return ret;
}

QVector<QQuickEventPoint *> QQuickMultiPointHandler::eligiblePoints(QQuickPointerEvent *event)
{
    QVector<QQuickEventPoint *> ret;
    int c = event->pointCount();
    QRectF targetBounds = target()->mapRectToScene(target()->boundingRect())
            .marginsAdded(QMarginsF(m_pointDistanceThreshold, m_pointDistanceThreshold, m_pointDistanceThreshold, m_pointDistanceThreshold));
    // If one or more points are newly pressed or released, all non-released points are candidates for this handler.
    // In other cases however, do not steal the grab: that is, if a point has a grabber,
    // it's not a candidate for this handler.
    bool stealingAllowed = event->isPressEvent() || event->isReleaseEvent();
    for (int i = 0; i < c; ++i) {
        QQuickEventPoint *p = event->point(i);
        if (!stealingAllowed) {
            QObject *exclusiveGrabber = p->exclusiveGrabber();
            if (exclusiveGrabber && exclusiveGrabber != this)
                continue;
        }
        if (p->state() != QQuickEventPoint::Released && targetBounds.contains(p->scenePos()))
            ret << p;
    }
    return ret;
}

void QQuickMultiPointHandler::setRequiredPointCount(int c)
{
    if (m_requiredPointCount == c)
        return;

    m_requiredPointCount = c;
    emit requiredPointCountChanged();
}

void QQuickMultiPointHandler::setPointDistanceThreshold(qreal pointDistanceThreshold)
{
    if (m_pointDistanceThreshold == pointDistanceThreshold)
        return;

    m_pointDistanceThreshold = pointDistanceThreshold;
    emit pointDistanceThresholdChanged();
}

bool QQuickMultiPointHandler::sameAsCurrentPoints(QQuickPointerEvent *event)
{
    bool ret = true;
    int c = event->pointCount();
    if (c != m_currentPoints.size())
        return false;
    // TODO optimize: either ensure the points are sorted,
    // or use std::equal with a predicate
    for (int i = 0; ret && i < c; ++i) {
        if (event->point(i)->state() == QQuickEventPoint::Released)
            return false;
        bool found = false;
        int pointId = event->point(i)->pointId();
        for (QQuickEventPoint *o : qAsConst(m_currentPoints))
            if (o && pointId == o->pointId())
                found = true;
        if (!found)
            ret = false;
    }
    return ret;
}

// TODO make templates for these functions somehow?
QPointF QQuickMultiPointHandler::touchPointCentroid()
{
    QPointF ret;
    if (Q_UNLIKELY(m_currentPoints.size() == 0))
        return ret;
    for (QQuickEventPoint *point : qAsConst(m_currentPoints))
        ret += point->scenePos();
    return ret / m_currentPoints.size();
}

QVector2D QQuickMultiPointHandler::touchPointCentroidVelocity()
{
    QVector2D ret;
    if (Q_UNLIKELY(m_currentPoints.size() == 0))
        return ret;
    for (QQuickEventPoint *point : qAsConst(m_currentPoints))
        ret += point->velocity();
    return ret / m_currentPoints.size();
}

qreal QQuickMultiPointHandler::averageTouchPointDistance(const QPointF &ref)
{
    qreal ret = 0;
    if (Q_UNLIKELY(m_currentPoints.size() == 0))
        return ret;
    for (QQuickEventPoint *point : qAsConst(m_currentPoints))
        ret += QVector2D(point->scenePos() - ref).length();
    return ret / m_currentPoints.size();
}

qreal QQuickMultiPointHandler::averageStartingDistance(const QPointF &ref)
{
    // TODO cache it in setActive()?
    qreal ret = 0;
    if (Q_UNLIKELY(m_currentPoints.size() == 0))
        return ret;
    for (QQuickEventPoint *point : qAsConst(m_currentPoints))
        ret += QVector2D(point->sceneGrabPos() - ref).length();
    return ret / m_currentPoints.size();
}

QVector<QQuickMultiPointHandler::PointData> QQuickMultiPointHandler::angles(const QPointF &ref) const
{
    QVector<PointData> angles;
    angles.reserve(m_currentPoints.count());
    for (QQuickEventPoint *point : qAsConst(m_currentPoints)) {
        qreal angle = QLineF(ref, point->scenePos()).angle();
        angles.append(PointData(point->pointId(), -angle));     // convert to clockwise, to be consistent with QQuickItem::rotation
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

void QQuickMultiPointHandler::acceptPoints(const QVector<QQuickEventPoint *> &points)
{
    for (QQuickEventPoint* point : points)
        point->setAccepted();
}

bool QQuickMultiPointHandler::grabPoints(QVector<QQuickEventPoint *> points)
{
    bool canGrab = true;
    for (QQuickEventPoint* point : points) {
        auto grabber = point->grabberItem();
        if (grabber && (grabber->keepMouseGrab() || grabber->keepTouchGrab()))
            canGrab = false;
    }
    if (canGrab) {
        for (QQuickEventPoint* point : points)
            setExclusiveGrab(point);
    }
    return canGrab;
}

QT_END_NAMESPACE
