// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickage_p.h"
#include "qquickparticleemitter_p.h"
QT_BEGIN_NAMESPACE
/*!
    \qmltype Age
    \instantiates QQuickAgeAffector
    \inqmlmodule QtQuick.Particles
    \inherits Affector
    \brief For altering particle ages.
    \ingroup qtquick-particles

    The Age affector allows you to alter where the particle is in its lifecycle. Common uses
    are to expire particles prematurely, possibly giving them time to animate out.

    The Age affector is also sometimes known as a 'Kill' affector, because with the default
    parameters it will immediately expire all particles which it affects.

    The Age affector only applies to particles which are still alive.
*/
/*!
    \qmlproperty int QtQuick.Particles::Age::lifeLeft

    The amount of life to set the particle to have. Affected particles
    will advance to a point in their life where they will have this many
    milliseconds left to live.
*/

/*!
    \qmlproperty bool QtQuick.Particles::Age::advancePosition

    advancePosition determines whether position, veclocity and acceleration are included in
    the simulated aging done by the affector. If advancePosition is false,
    then the position, velocity and acceleration will remain the same and only
    other attributes (such as opacity) will advance in the simulation to where
    it would normally be for that point in the particle's life. With advancePosition set to
    true the position, velocity and acceleration will also advance to where it would
    normally be by that point in the particle's life, making it advance its position
    on screen.

    Default value is true.
*/

QQuickAgeAffector::QQuickAgeAffector(QQuickItem *parent) :
    QQuickParticleAffector(parent), m_lifeLeft(0), m_advancePosition(true)
{
}


bool QQuickAgeAffector::affectParticle(QQuickParticleData *d, qreal dt)
{
    Q_UNUSED(dt);
    if (d->stillAlive(m_system)){
        float curT = m_system->timeInt / 1000.0f;
        float ttl = m_lifeLeft / 1000.0f;
        if (!m_advancePosition && ttl > 0){
            float x = d->curX(m_system);
            float vx = d->curVX(m_system);
            float ax = d->curAX();
            float y = d->curY(m_system);
            float vy = d->curVY(m_system);
            float ay = d->curAY();
            d->t = curT - (d->lifeSpan - ttl);
            d->setInstantaneousX(x, m_system);
            d->setInstantaneousVX(vx, m_system);
            d->setInstantaneousAX(ax, m_system);
            d->setInstantaneousY(y, m_system);
            d->setInstantaneousVY(vy, m_system);
            d->setInstantaneousAY(ay, m_system);
        } else {
            d->t = curT - (d->lifeSpan - ttl);
        }
        return true;
    }
    return false;
}
QT_END_NAMESPACE

#include "moc_qquickage_p.cpp"
