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

#include "qquickmultipointerhandler_p.h"
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
QQuickMultiPointerHandler::QQuickMultiPointerHandler(QObject *parent, int requiredPointCount)
    : QQuickPointerDeviceHandler(parent)
    , m_requiredPointCount(requiredPointCount)
    , m_pointDistanceThreshold(0)
{
}

QQuickMultiPointerHandler::~QQuickMultiPointerHandler()
{
}

bool QQuickMultiPointerHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    if (!QQuickPointerDeviceHandler::wantsPointerEvent(event))
        return false;
    if (event->pointCount() < m_requiredPointCount)
        return false;
    if (sameAsCurrentPoints(event))
        return true;
    m_currentPoints = pointsInsideOrNearTarget(event);
    return (m_currentPoints.size() == m_requiredPointCount);
}

QVector<QQuickEventPoint *> QQuickMultiPointerHandler::pointsInsideOrNearTarget(QQuickPointerEvent *event)
{
    QVector<QQuickEventPoint *> ret;
    int c = event->pointCount();
    QRectF targetBounds = target()->mapRectToScene(target()->boundingRect())
            .marginsAdded(QMarginsF(m_pointDistanceThreshold, m_pointDistanceThreshold, m_pointDistanceThreshold, m_pointDistanceThreshold));
    for (int i = 0; i < c; ++i) {
        QQuickEventPoint *p = event->point(i);
        if (targetBounds.contains(p->scenePos()))
            ret << p;
    }
    return ret;
}

void QQuickMultiPointerHandler::setRequiredPointCount(int c)
{
    if (m_requiredPointCount == c)
        return;

    m_requiredPointCount = c;
    emit requiredPointCountChanged();
}

void QQuickMultiPointerHandler::setPointDistanceThreshold(qreal pointDistanceThreshold)
{
    if (m_pointDistanceThreshold == pointDistanceThreshold)
        return;

    m_pointDistanceThreshold = pointDistanceThreshold;
    emit pointDistanceThresholdChanged();
}

bool QQuickMultiPointerHandler::sameAsCurrentPoints(QQuickPointerEvent *event)
{
    bool ret = true;
    int c = event->pointCount();
    if (c != m_currentPoints.size())
        return false;
    // TODO optimize: either ensure the points are sorted,
    // or use std::equal with a predicate
    for (int i = 0; ret && i < c; ++i) {
        bool found = false;
        quint64 pointId = event->point(i)->pointId();
        for (QQuickEventPoint *o : qAsConst(m_currentPoints))
            if (o && pointId == o->pointId())
                found = true;
        if (!found)
            ret = false;
    }
    return ret;
}

// TODO make templates for these functions somehow?
QPointF QQuickMultiPointerHandler::touchPointCentroid()
{
    QPointF ret;
    if (Q_UNLIKELY(m_currentPoints.size() == 0))
        return ret;
    for (QQuickEventPoint *point : qAsConst(m_currentPoints))
        ret += point->scenePos();
    return ret / m_currentPoints.size();
}

QPointF QQuickMultiPointerHandler::startingCentroid()
{
    // TODO cache it in setActive()?
    QPointF ret;
    if (Q_UNLIKELY(m_currentPoints.size() == 0))
        return ret;
    for (QQuickEventPoint *point : qAsConst(m_currentPoints))
        ret += point->sceneGrabPos();
    return ret / m_currentPoints.size();
}

qreal QQuickMultiPointerHandler::averageTouchPointDistance(const QPointF &ref)
{
    qreal ret = 0;
    if (Q_UNLIKELY(m_currentPoints.size() == 0))
        return ret;
    for (QQuickEventPoint *point : qAsConst(m_currentPoints))
        ret += QVector2D(point->scenePos() - ref).length();
    return ret / m_currentPoints.size();
}

qreal QQuickMultiPointerHandler::averageStartingDistance(const QPointF &ref)
{
    // TODO cache it in setActive()?
    qreal ret = 0;
    if (Q_UNLIKELY(m_currentPoints.size() == 0))
        return ret;
    for (QQuickEventPoint *point : qAsConst(m_currentPoints))
        ret += QVector2D(point->sceneGrabPos() - ref).length();
    return ret / m_currentPoints.size();
}

qreal QQuickMultiPointerHandler::averageTouchPointAngle(const QPointF &ref)
{
    qreal ret = 0;
    if (Q_UNLIKELY(m_currentPoints.size() == 0))
        return ret;
    for (QQuickEventPoint *point : qAsConst(m_currentPoints))
        ret += QLineF(ref, point->scenePos()).angle();
    return ret / m_currentPoints.size();
}

qreal QQuickMultiPointerHandler::averageStartingAngle(const QPointF &ref)
{
    // TODO cache it in setActive()?
    qreal ret = 0;
    if (Q_UNLIKELY(m_currentPoints.size() == 0))
        return ret;
    for (QQuickEventPoint *point : qAsConst(m_currentPoints))
        ret += QLineF(ref, point->sceneGrabPos()).angle();
    return ret / m_currentPoints.size();
}

void QQuickMultiPointerHandler::grabPoints(QVector<QQuickEventPoint *> points)
{
    for (QQuickEventPoint* point : points)
        point->setPointerHandlerGrabber(this);
}

QT_END_NAMESPACE
