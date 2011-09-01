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

#include "qsgpointattractor_p.h"
#include <cmath>
#include <QDebug>
QT_BEGIN_NAMESPACE
/*!
    \qmlclass PointAttractor QSGPointAttractorAffector
    \inqmlmodule QtQuick.Particles 2
    \inherits Affector
    \brief The PointAttractor allows you to attract particles towards a specific point.

    Note that the size and position of this element affects which particles it affects.
    The size of the point attracted to is always 0x0, and the location of that point
    is specified by the pointX and pointY properties.

    Note that PointAttractor has the standard Item x,y,width and height properties.
    Like other affectors, these represent the affected area. They
    do not represent the 0x0 point which is the target of the attraction.

*/


/*!
    \qmlproperty real QtQuick.Particles2::PointAttractor::pointX
*/
/*!
    \qmlproperty real QtQuick.Particles2::PointAttractor::pointY
*/
/*!
    \qmlproperty real QtQuick.Particles2::PointAttractor::strength
*/
/*!
    \qmlproperty PhysicsAffects QtQuick.Particles2::PointAttractor::physics

    What attribute of particles is directly affected.
    \list
    \o PointAttractor.Position
    \o PointAttractor.Velocity
    \o PointAttractor.Acceleration
    \endlist
*/
/*!
    \qmlproperty Proportion QtQuick.Particles2::PointAttractor::proportionalToDistance

    How the distance from the particle to the point affects the strength of the attraction.

    \list
    \o PointAttractor.Constant
    \o PointAttractor.Linear
    \o PointAttractor.InverseLinear
    \o PointAttractor.Quadratic
    \o PointAttractor.InverseQuadratic
    \endlist
*/


QSGPointAttractorAffector::QSGPointAttractorAffector(QSGItem *parent) :
    QSGParticleAffector(parent), m_strength(0.0), m_x(0), m_y(0)
  , m_physics(Velocity), m_proportionalToDistance(Linear)
{
}

bool QSGPointAttractorAffector::affectParticle(QSGParticleData *d, qreal dt)
{
    if (m_strength == 0.0)
        return false;
    qreal dx = m_x - d->curX();
    qreal dy = m_y - d->curY();
    qreal r = sqrt((dx*dx) + (dy*dy));
    qreal theta = atan2(dy,dx);
    qreal ds = 0;
    switch (m_proportionalToDistance){
    case InverseQuadratic:
        ds = (m_strength / qMax<qreal>(1.,r*r));
        break;
    case InverseLinear:
        ds = (m_strength / qMax<qreal>(1.,r));
        break;
    case Quadratic:
        ds = (m_strength * qMax<qreal>(1.,r*r));
        break;
    case Linear:
        ds = (m_strength * qMax<qreal>(1.,r));
        break;
    default: //also Constant
        ds = m_strength;
    }
    ds *= dt;
    dx = ds * cos(theta);
    dy = ds * sin(theta);
    qreal vx,vy;
    switch (m_physics){
    case Position:
        d->x = (d->x + dx);
        d->y = (d->y + dy);
        break;
    case Acceleration:
        d->setInstantaneousAX(d->ax + dx);
        d->setInstantaneousAY(d->ay + dy);
        break;
    case Velocity: //also default
    default:
        vx = d->curVX();
        vy = d->curVY();
        d->setInstantaneousVX(vx + dx);
        d->setInstantaneousVY(vy + dy);
    }

    return true;
}
QT_END_NAMESPACE
