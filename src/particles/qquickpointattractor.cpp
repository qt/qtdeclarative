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
    \brief Attracts particles towards a specific point.

    Like other affectors, Attractor has the standard properties x, y, width,
    and height that represent the affected area. The size and position of the
    Attractor item determine the affected particles.

    The size of the attracting point is always 0x0, and its location is
    specified by \l pointX and \l pointY properties.
*/


/*!
    \qmlproperty real QtQuick.Particles::Attractor::pointX

    The x coordinate of the attracting point, relative
    to the x coordinate of the Attractor item.
*/
/*!
    \qmlproperty real QtQuick.Particles::Attractor::pointY

    The y coordinate of the attracting point, relative
    to the y coordinate of the Attractor item.
*/
/*!
    \qmlproperty real QtQuick.Particles::Attractor::strength

    The pull, in units per second, to be exerted on an item one pixel away.

    Strength, together with the value of \l proportionalToDistance property,
    determine the exact amount of pull exerted on particles at a distance.
*/
/*!
    \qmlproperty enumeration QtQuick.Particles::Attractor::affectedParameter

    The attribute of particles that is directly affected.

    \value Attractor.Position Position
    \value Attractor.Velocity Velocity
    \value Attractor.Acceleration Acceleration
*/
/*!
    \qmlproperty enumeration QtQuick.Particles::Attractor::proportionalToDistance

    The relation between the \l strength of the attraction and the distance from
    the particle to the attracting point.

    \value Attractor.Constant Constant
    \value Attractor.Linear Linear
    \value Attractor.InverseLinear Inverse linear
    \value Attractor.Quadratic Quadratic
    \value Attractor.InverseQuadratic Inverse quadratic
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
