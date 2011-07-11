/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgtargeteddirection_p.h"
#include "qsgparticleemitter_p.h"
#include <cmath>
#include <QDebug>

QT_BEGIN_NAMESPACE
/*!
    \qmlclass TargetedDirection QSGTargetedDirection
    \inqmlmodule QtQuick.Particles 2
    \since QtQuick.Particles 2.0
    \inherits StochasticDirection
    \brief The TargetedDirection element allows you to specify a direction towards the target point

*/
/*!
    \qmlproperty real QtQuick.Particles2::TargetedDirection::targetX
*/
/*!
    \qmlproperty real QtQuick.Particles2::TargetedDirection::targetY
*/
/*!
    \qmlproperty Item QtQuick.Particles2::TargetedDirection::targetItem
    If specified, this will take precedence over targetX and targetY.
    The targeted point will be the center of the specified Item
*/
/*!
    \qmlproperty real QtQuick.Particles2::TargetedDirection::targetVariation
*/
/*!
    \qmlproperty real QtQuick.Particles2::TargetedDirection::magnitude
*/
/*!
    \qmlproperty real QtQuick.Particles2::TargetedDirection::magnitudeVariation
*/
/*!
    \qmlproperty bool QtQuick.Particles2::TargetedDirection::proportionalMagnitude

    If true, then the value of magnitude and magnitudeVariation shall be interpreted as multiples
    of the distance between the source point and the target point, per second.

    If false(default), then the value of magnitude and magnitudeVariation shall be interpreted as
    pixels per second.
*/

QSGTargetedDirection::QSGTargetedDirection(QObject *parent) :
    QSGStochasticDirection(parent)
  , m_targetX(0)
  , m_targetY(0)
  , m_targetVariation(0)
  , m_proportionalMagnitude(false)
  , m_magnitude(0)
  , m_magnitudeVariation(0)
  , m_targetItem(0)
{
}

const QPointF &QSGTargetedDirection::sample(const QPointF &from)
{
    //###This approach loses interpolating the last position of the target (like we could with the emitter) is it worthwhile?
    qreal targetX;
    qreal targetY;
    if (m_targetItem){
        QSGParticleEmitter* parentEmitter = qobject_cast<QSGParticleEmitter*>(parent());
        targetX = m_targetItem->width()/2;
        targetY = m_targetItem->height()/2;
        if (!parentEmitter){
            qWarning() << "Directed vector is not a child of the emitter. Mapping of target item coordinates may fail.";
            targetX += m_targetItem->x();
            targetY += m_targetItem->y();
        }else{
            m_ret = parentEmitter->mapFromItem(m_targetItem, QPointF(targetX, targetY));
            targetX = m_ret.x();
            targetY = m_ret.y();
        }
    }else{
        targetX = m_targetX;
        targetY = m_targetY;
    }
    targetX += 0 - from.x() - m_targetVariation + rand()/(float)RAND_MAX * m_targetVariation*2;
    targetY += 0 - from.y() - m_targetVariation + rand()/(float)RAND_MAX * m_targetVariation*2;
    qreal theta = atan2(targetY, targetX);
    qreal mag = m_magnitude + rand()/(float)RAND_MAX * m_magnitudeVariation * 2 - m_magnitudeVariation;
    if (m_proportionalMagnitude)
        mag *= sqrt(targetX * targetX + targetY * targetY);
    m_ret.setX(mag * cos(theta));
    m_ret.setY(mag * sin(theta));
    return m_ret;
}

QT_END_NAMESPACE
