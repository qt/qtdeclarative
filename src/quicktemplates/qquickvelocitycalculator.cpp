// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickvelocitycalculator_p_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

/*
    Usage:

    QQuickVelocityCalculator velocityCalculator;

    // ...

    velocityCalcular.startMeasuring(event->pos(), event->timestamp());
    velocityCalcular.stopMeasuring(event->pos(), event->timestamp());

    // ...

    if (velocityCalculator.velocity().x() > someAmount)
        doSomething();
    else if (velocityCalculator.velocity().x() < -someAmount)
        doSomethingElse();
*/

void QQuickVelocityCalculator::startMeasuring(const QPointF &point1, qint64 timestamp)
{
    m_point1 = point1;
    m_point1Timestamp = timestamp;
}

void QQuickVelocityCalculator::stopMeasuring(const QPointF &point2, qint64 timestamp)
{
    if (timestamp == 0) {
        qWarning() << "QQuickVelocityCalculator: a call to stopMeasuring() must be preceded by a call to startMeasuring()";
        return;
    }

    m_point2 = point2;
    m_point2Timestamp = timestamp;
}

void QQuickVelocityCalculator::reset()
{
    m_point1 = QPointF();
    m_point2 = QPointF();
    m_point1Timestamp = 0;
    m_point2Timestamp = 0;
}

QPointF QQuickVelocityCalculator::velocity() const
{
    if (m_point2Timestamp == 0 || m_point1Timestamp == m_point2Timestamp)
        return QPointF();

    const qreal secondsElapsed = (m_point2Timestamp - m_point1Timestamp) / 1000.0;
    const QPointF distance = m_point2 - m_point1;
    return distance / secondsElapsed;
}

QT_END_NAMESPACE
