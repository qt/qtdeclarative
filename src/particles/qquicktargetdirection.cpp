// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktargetdirection_p.h"
#include "qquickparticleemitter_p.h"
#include <cmath>
#include <QDebug>
#include <QRandomGenerator>

QT_BEGIN_NAMESPACE
/*!
    \qmltype TargetDirection
    \nativetype QQuickTargetDirection
    \inqmlmodule QtQuick.Particles
    \ingroup qtquick-particles
    \inherits Direction
    \brief For specifying a direction towards the target point.

*/
/*!
    \qmlproperty real QtQuick.Particles::TargetDirection::targetX
*/
/*!
    \qmlproperty real QtQuick.Particles::TargetDirection::targetY
*/
/*!
    \qmlproperty Item QtQuick.Particles::TargetDirection::targetItem
    If specified, this will take precedence over targetX and targetY.
    The targeted point will be the center of the specified Item
*/
/*!
    \qmlproperty real QtQuick.Particles::TargetDirection::targetVariation
*/
/*!
    \qmlproperty real QtQuick.Particles::TargetDirection::magnitude
*/
/*!
    \qmlproperty real QtQuick.Particles::TargetDirection::magnitudeVariation
*/
/*!
    \qmlproperty bool QtQuick.Particles::TargetDirection::proportionalMagnitude

    If true, then the value of magnitude and magnitudeVariation shall be interpreted as multiples
    of the distance between the source point and the target point, per second.

    If false(default), then the value of magnitude and magnitudeVariation shall be interpreted as
    pixels per second.
*/

QQuickTargetDirection::QQuickTargetDirection(QObject *parent) :
    QQuickDirection(parent)
  , m_targetX(0)
  , m_targetY(0)
  , m_targetVariation(0)
  , m_proportionalMagnitude(false)
  , m_magnitude(0)
  , m_magnitudeVariation(0)
  , m_targetItem(nullptr)
{
}

QPointF QQuickTargetDirection::sample(const QPointF &from)
{
    //###This approach loses interpolating the last position of the target (like we could with the emitter) is it worthwhile?
    QPointF ret;
    qreal targetX;
    qreal targetY;
    if (m_targetItem){
        QQuickParticleEmitter* parentEmitter = qobject_cast<QQuickParticleEmitter*>(parent());
        targetX = m_targetItem->width()/2;
        targetY = m_targetItem->height()/2;
        if (!parentEmitter){
            qWarning() << "Directed vector is not a child of the emitter. Mapping of target item coordinates may fail.";
            targetX += m_targetItem->x();
            targetY += m_targetItem->y();
        }else{
            ret = parentEmitter->mapFromItem(m_targetItem, QPointF(targetX, targetY));
            targetX = ret.x();
            targetY = ret.y();
        }
    }else{
        targetX = m_targetX;
        targetY = m_targetY;
    }
    targetX += 0 - from.x() - m_targetVariation + QRandomGenerator::global()->generateDouble() * m_targetVariation*2;
    targetY += 0 - from.y() - m_targetVariation + QRandomGenerator::global()->generateDouble() * m_targetVariation*2;
    qreal theta = std::atan2(targetY, targetX);
    qreal mag = m_magnitude + QRandomGenerator::global()->generateDouble() * m_magnitudeVariation * 2 - m_magnitudeVariation;
    if (m_proportionalMagnitude)
        mag *= qHypot(targetX, targetY);
    ret.setX(mag * std::cos(theta));
    ret.setY(mag * std::sin(theta));
    return ret;
}

QT_END_NAMESPACE

#include "moc_qquicktargetdirection_p.cpp"
