// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcustomaffector_p.h"

#include <private/qquickv4particledata_p.h>
#include <private/qqmlglobal_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Affector
    \nativetype QQuickCustomAffector
    \inqmlmodule QtQuick.Particles
    \brief Applies alterations to the attributes of logical particles at any
    point in their lifetime.
    \inherits ParticleAffector
    \ingroup qtquick-particles

    Custom Affector manipulates the properties of the particles directly in
    JavaScript.
*/

/*!
    \qmlsignal QtQuick.Particles::Affector::affectParticles(Array particles, real dt)

    This signal is emitted when particles are selected to be affected.
    \a particles is an array of particle objects which can be directly
    manipulated.

    \a dt is the time since the last time it was affected. Use \a dt to
    normalize trajectory manipulations to real time.

    \note JavaScript is slower to execute, so it is not recommended to use
    this in high-volume particle systems.
*/

/*!
    \qmlproperty StochasticDirection QtQuick.Particles::Affector::position

    Affected particles will have their position set to this direction,
    relative to the ParticleSystem. When interpreting directions as points,
    imagine it as an arrow with the base at the 0,0 of the ParticleSystem and the
    tip at where the specified position will be.
*/

/*!
    \qmlproperty StochasticDirection QtQuick.Particles::Affector::velocity

    Affected particles will have their velocity set to this direction.
*/


/*!
    \qmlproperty StochasticDirection QtQuick.Particles::Affector::acceleration

    Affected particles will have their acceleration set to this direction.
*/


/*!
    \qmlproperty bool QtQuick.Particles::Affector::relative

    Whether the affected particles have their existing position/velocity/acceleration added
    to the new one.

    Default is true.
*/
QQuickCustomAffector::QQuickCustomAffector(QQuickItem *parent) :
    QQuickParticleAffector(parent)
    , m_position(&m_nullVector)
    , m_velocity(&m_nullVector)
    , m_acceleration(&m_nullVector)
    , m_relative(true)
{
}

bool QQuickCustomAffector::isAffectConnected()
{
    IS_SIGNAL_CONNECTED(
            this, QQuickCustomAffector, affectParticles,
            (const QList<QQuickV4ParticleData> &, qreal));
}

void QQuickCustomAffector::affectSystem(qreal dt)
{
    //Acts a bit differently, just emits affected for everyone it might affect, when the only thing is connecting to affected(x,y)
    bool justAffected = (m_acceleration == &m_nullVector
        && m_velocity == &m_nullVector
        && m_position == &m_nullVector
        && isAffectedConnected());
    if (!isAffectConnected() && !justAffected) {
        QQuickParticleAffector::affectSystem(dt);
        return;
    }
    if (!m_enabled)
        return;
    updateOffsets();

    QList<QQuickParticleData*> toAffect;
    for (const QQuickParticleGroupData *gd : std::as_const(m_system->groupData)) {
        if (activeGroup(gd->index)) {
            for (QQuickParticleData *d : gd->data) {
                if (shouldAffect(d)) {
                    toAffect << d;
                }
            }
        }
    }

    if (toAffect.isEmpty())
        return;

    if (justAffected) {
        for (const QQuickParticleData *d : std::as_const(toAffect)) {//Not postAffect to avoid saying the particle changed
            if (m_onceOff)
                m_onceOffed << qMakePair(d->groupId, d->index);
            emit affected(d->curX(m_system), d->curY(m_system));
        }
        return;
    }

    if (m_onceOff)
        dt = 1.0;

    QList<QQuickV4ParticleData> particles;
    particles.reserve(toAffect.size());
    for (QQuickParticleData *data: std::as_const(toAffect))
        particles.push_back(data->v4Value(m_system));

    const auto doAffect = [&](qreal dt) {
        affectProperties(toAffect, dt);
        emit affectParticles(particles, dt);
    };

    if (dt >= simulationCutoff || dt <= simulationDelta) {
        doAffect(dt);
    } else {
        int realTime = m_system->timeInt;
        m_system->timeInt -= dt * 1000.0;
        while (dt > simulationDelta) {
            m_system->timeInt += simulationDelta * 1000.0;
            dt -= simulationDelta;
            doAffect(simulationDelta);
        }
        m_system->timeInt = realTime;
        if (dt > 0.0)
            doAffect(dt);
    }

    for (QQuickParticleData *d : std::as_const(toAffect))
        if (d->update == 1.0)
            postAffect(d);
}

bool QQuickCustomAffector::affectParticle(QQuickParticleData *d, qreal dt)
{
    //This does the property based affecting, called by superclass if signal isn't hooked up.
    bool changed = false;
    QPointF curPos(d->curX(m_system), d->curY(m_system));

    if (m_acceleration != &m_nullVector){
        QPointF pos = m_acceleration->sample(curPos);
        QPointF curAcc = QPointF(d->curAX(), d->curAY());
        if (m_relative) {
            pos *= dt;
            pos += curAcc;
        }
        if (pos != curAcc) {
            d->setInstantaneousAX(pos.x(), m_system);
            d->setInstantaneousAY(pos.y(), m_system);
            changed = true;
        }
    }

    if (m_velocity != &m_nullVector){
        QPointF pos = m_velocity->sample(curPos);
        QPointF curVel = QPointF(d->curVX(m_system), d->curVY(m_system));
        if (m_relative) {
            pos *= dt;
            pos += curVel;
        }
        if (pos != curVel) {
            d->setInstantaneousVX(pos.x(), m_system);
            d->setInstantaneousVY(pos.y(), m_system);
            changed = true;
        }
    }

    if (m_position != &m_nullVector){
        QPointF pos = m_position->sample(curPos);
        if (m_relative) {
            pos *= dt;
            pos += curPos;
        }
        if (pos != curPos) {
            d->setInstantaneousX(pos.x(), m_system);
            d->setInstantaneousY(pos.y(), m_system);
            changed = true;
        }
    }

    return changed;
}

void QQuickCustomAffector::affectProperties(const QList<QQuickParticleData*> &particles, qreal dt)
{
    for (QQuickParticleData *d : particles)
        if ( affectParticle(d, dt) )
            d->update = 1.0;
}

QT_END_NAMESPACE

#include "moc_qquickcustomaffector_p.cpp"
