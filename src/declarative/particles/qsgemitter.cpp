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

#include "qsgemitter_p.h"
#include "qsgparticlesystem_p.h"
QT_BEGIN_NAMESPACE

/*!
    \qmlclass Emitter QSGBasicEmitter
    \inqmlmodule QtQuick.Particles 2
    \since QtQuick.Particles 2.0
    \brief The Emitter element allows you to emit logical particles.

    This element emits logical particles into the ParticleSystem, with the
    given starting attributes.

    Note that logical particles are not
    automatically rendered, you will need to have one or more
    ParticlePainter elements visualizing them.

    Note that the given starting attributes can be modified at any point
    in the particle's lifetime by any Affector element in the same
    ParticleSystem. This includes attributes like lifespan.
*/

QSGBasicEmitter::QSGBasicEmitter(QSGItem* parent)
    : QSGParticleEmitter(parent)
    , m_speed_from_movement(0)
    , m_particle_count(0)
    , m_reset_last(true)
    , m_last_timestamp(0)
    , m_last_emission(0)
{
//    setFlag(ItemHasContents);
}

/*!
    \qmlproperty ParticleSystem QtQuick.Particles2::Emitter::system

    This is the Particle system that the Emitter will emit into.
    This can be omitted if the Emitter is a direct child of the ParticleSystem
*/
/*!
    \qmlproperty string QtQuick.Particles2::Emitter::particle
*/
/*!
    \qmlproperty Shape QtQuick.Particles2::Emitter::shape
*/
/*!
    \qmlproperty bool QtQuick.Particles2::Emitter::emitting
*/
/*!
    \qmlproperty real QtQuick.Particles2::Emitter::emitRate
*/
/*!
    \qmlproperty int QtQuick.Particles2::Emitter::lifeSpan
*/
/*!
    \qmlproperty int QtQuick.Particles2::Emitter::lifeSpanVariation
*/
/*!
    \qmlproperty int QtQuick.Particles2::Emitter::emitCap
*/
/*!
    \qmlproperty real QtQuick.Particles2::Emitter::size
*/
/*!
    \qmlproperty real QtQuick.Particles2::Emitter::endSize
*/
/*!
    \qmlproperty real QtQuick.Particles2::Emitter::sizeVariation
*/
/*!
    \qmlproperty StochasticDirection QtQuick.Particles2::Emitter::speed
*/
/*!
    \qmlproperty StochasticDirection QtQuick.Particles2::Emitter::acceleration
*/
/*!
    \qmlproperty qreal QtQuick.Particles2::Emitter::speedFromMovement
*/

void QSGBasicEmitter::setSpeedFromMovement(qreal t)
{
    if (t == m_speed_from_movement)
        return;
    m_speed_from_movement = t;
    emit speedFromMovementChanged();
}

void QSGBasicEmitter::reset()
{
    m_reset_last = true;
}

void QSGBasicEmitter::emitWindow(int timeStamp)
{
    if (m_system == 0)
        return;
    if ((!m_emitting || !m_particlesPerSecond)&& !m_burstLeft && m_burstQueue.isEmpty()){
        m_reset_last = true;
        return;
    }

    if (m_reset_last) {
        m_last_emitter = m_last_last_emitter = QPointF(x(), y());
        m_last_timestamp = timeStamp/1000.;
        m_last_emission = m_last_timestamp;
        m_reset_last = false;
    }

    if (m_burstLeft){
        m_burstLeft -= timeStamp - m_last_timestamp * 1000.;
        if (m_burstLeft < 0){
            if (!m_emitting)
                timeStamp += m_burstLeft;
            m_burstLeft = 0;
        }
    }

    qreal time = timeStamp / 1000.;

    qreal particleRatio = 1. / m_particlesPerSecond;
    qreal pt = m_last_emission;

    qreal opt = pt; // original particle time
    qreal dt = time - m_last_timestamp; // timestamp delta...
    if (!dt)
        dt = 0.000001;

    // emitter difference since last...
    qreal dex = (x() - m_last_emitter.x());
    qreal dey = (y() - m_last_emitter.y());

    qreal ax = (m_last_last_emitter.x() + m_last_emitter.x()) / 2;
    qreal bx = m_last_emitter.x();
    qreal cx = (x() + m_last_emitter.x()) / 2;
    qreal ay = (m_last_last_emitter.y() + m_last_emitter.y()) / 2;
    qreal by = m_last_emitter.y();
    qreal cy = (y() + m_last_emitter.y()) / 2;

    qreal sizeAtEnd = m_particleEndSize >= 0 ? m_particleEndSize : m_particleSize;
    qreal emitter_x_offset = m_last_emitter.x() - x();
    qreal emitter_y_offset = m_last_emitter.y() - y();
    if (!m_burstQueue.isEmpty() && !m_burstLeft && !m_emitting)//'outside time' emissions only
        pt = time;
    while (pt < time || !m_burstQueue.isEmpty()) {
        //int pos = m_last_particle % m_particle_count;
        QSGParticleData* datum = m_system->newDatum(m_system->m_groupIds[m_particle]);
        if (datum){//actually emit(otherwise we've been asked to skip this one)
            datum->e = this;//###useful?
            qreal t = 1 - (pt - opt) / dt;
            qreal vx =
              - 2 * ax * (1 - t)
              + 2 * bx * (1 - 2 * t)
              + 2 * cx * t;
            qreal vy =
              - 2 * ay * (1 - t)
              + 2 * by * (1 - 2 * t)
              + 2 * cy * t;


            // Particle timestamp
            datum->t = pt;
            datum->lifeSpan = //TODO:Promote to base class?
                    (m_particleDuration
                     + ((rand() % ((m_particleDurationVariation*2) + 1)) - m_particleDurationVariation))
                    / 1000.0;

            // Particle position
            QRectF boundsRect;
            if (!m_burstQueue.isEmpty()){
                boundsRect = QRectF(m_burstQueue.first().second.x() - x(), m_burstQueue.first().second.y() - y(),
                        width(), height());
            } else {
                boundsRect = QRectF(emitter_x_offset + dex * (pt - opt) / dt, emitter_y_offset + dey * (pt - opt) / dt
                              , width(), height());
            }
            QPointF newPos = effectiveExtruder()->extrude(boundsRect);
            datum->x = newPos.x();
            datum->y = newPos.y();

            // Particle speed
            const QPointF &speed = m_speed->sample(newPos);
            datum->vx = speed.x()
                    + m_speed_from_movement * vx;
            datum->vy = speed.y()
                    + m_speed_from_movement * vy;

            // Particle acceleration
            const QPointF &accel = m_acceleration->sample(newPos);
            datum->ax = accel.x();
            datum->ay = accel.y();

            // Particle size
            float sizeVariation = -m_particleSizeVariation
                    + rand() / float(RAND_MAX) * m_particleSizeVariation * 2;

            float size = qMax((qreal)0.0 , m_particleSize + sizeVariation);
            float endSize = qMax((qreal)0.0 , sizeAtEnd + sizeVariation);

            datum->size = size;// * float(m_emitting);
            datum->endSize = endSize;// * float(m_emitting);

            m_system->emitParticle(datum);
        }
        if (m_burstQueue.isEmpty()){
            pt += particleRatio;
        }else{
            m_burstQueue.first().first--;
            if (m_burstQueue.first().first <= 0)
                m_burstQueue.pop_front();
        }
    }
    m_last_emission = pt;

    m_last_last_last_emitter = m_last_last_emitter;
    m_last_last_emitter = m_last_emitter;
    m_last_emitter = QPointF(x(), y());
    m_last_timestamp = time;
}


QT_END_NAMESPACE
