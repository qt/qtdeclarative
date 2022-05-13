// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpointattractor_p.h"
#include <cmath>
#include <QDebug>
QT_BEGIN_NAMESPACE
/*!
    \qmltype Attractor
    \instantiates QQuickAttractorAffector
    \inqmlmodule QtQuick.Particles
    \ingroup qtquick-particles
    \inherits Affector
    \brief For attracting particles towards a specific point.

    Note that the size and position of this element affects which particles it affects.
    The size of the point attracted to is always 0x0, and the location of that point
    is specified by the pointX and pointY properties.

    Note that Attractor has the standard Item x,y,width and height properties.
    Like other affectors, these represent the affected area. They
    do not represent the 0x0 point which is the target of the attraction.
*/


/*!
    \qmlproperty real QtQuick.Particles::PointAttractor::pointX

    The x coordinate of the attracting point. This is relative
    to the x coordinate of the Attractor.
*/
/*!
    \qmlproperty real QtQuick.Particles::PointAttractor::pointY

    The y coordinate of the attracting point. This is relative
    to the y coordinate of the Attractor.
*/
/*!
    \qmlproperty real QtQuick.Particles::PointAttractor::strength

    The pull, in units per second, to be exerted on an item one pixel away.

    Depending on how the attraction is proportionalToDistance this may have to
    be very high or very low to have a reasonable effect on particles at a
    distance.
*/
/*!
    \qmlproperty AffectableParameter QtQuick.Particles::Attractor::affectedParameter

    What attribute of particles is directly affected.
    \list
    \li Attractor.Position
    \li Attractor.Velocity
    \li Attractor.Acceleration
    \endlist
*/
/*!
    \qmlproperty Proportion QtQuick.Particles::Attractor::proportionalToDistance

    How the distance from the particle to the point affects the strength of the attraction.

    \list
    \li Attractor.Constant
    \li Attractor.Linear
    \li Attractor.InverseLinear
    \li Attractor.Quadratic
    \li Attractor.InverseQuadratic
    \endlist
*/


QQuickAttractorAffector::QQuickAttractorAffector(QQuickItem *parent) :
    QQuickParticleAffector(parent), m_strength(0.0), m_x(0), m_y(0)
  , m_physics(Velocity), m_proportionalToDistance(Linear)
{
}

bool QQuickAttractorAffector::affectParticle(QQuickParticleData *d, qreal dt)
{
    if (m_strength == 0.0)
        return false;
    qreal dx = m_x+m_offset.x() - d->curX(m_system);
    qreal dy = m_y+m_offset.y() - d->curY(m_system);
    qreal r = std::sqrt((dx*dx) + (dy*dy));
    qreal theta = std::atan2(dy,dx);
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
    dx = ds * std::cos(theta);
    dy = ds * std::sin(theta);
    qreal vx,vy;
    switch (m_physics){
    case Position:
        d->x = (d->x + dx);
        d->y = (d->y + dy);
        break;
    case Acceleration:
        d->setInstantaneousAX(d->ax + dx, m_system);
        d->setInstantaneousAY(d->ay + dy, m_system);
        break;
    case Velocity: //also default
    default:
        vx = d->curVX(m_system);
        vy = d->curVY(m_system);
        d->setInstantaneousVX(vx + dx, m_system);
        d->setInstantaneousVY(vy + dy, m_system);
    }

    return true;
}
QT_END_NAMESPACE

#include "moc_qquickpointattractor_p.cpp"
