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

#include "qsgcustomemitter_p.h"
#include "qsgparticlesystem_p.h"
QT_BEGIN_NAMESPACE
/*!
    \qmlclass CustomEmitter QSGCustomEmitter
    \inqmlmodule QtQuick.Particles 2
    \since QtQuick.Particles 2.0
    \inherits Emitter
    \brief The Custom emitter allows you to modify particles as they are emitted

*/

//TODO: Document particle 'type'
/*!
    \qmlsignal CustomEmitter::emitting(particle)

    This handler is called when a particle is emitted. You can modify particle
    attributes from within the handler.
*/

QSGCustomEmitter::QSGCustomEmitter(QSGItem* parent)
    : QSGParticleEmitter(parent)
    , m_particle_count(0)
    , m_reset_last(true)
    , m_last_timestamp(0)
    , m_last_emission(0)
{
}

void QSGCustomEmitter::reset()
{
    m_reset_last = true;
}

void QSGCustomEmitter::emitWindow(int timeStamp)
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
            datum->vx = speed.x();
            datum->vy = speed.y();

            // Particle acceleration
            const QPointF &accel = m_acceleration->sample(newPos);
            datum->ax = accel.x();
            datum->ay = accel.y();

            // Particle size
            float sizeVariation = -m_particleSizeVariation
                    + rand() / float(RAND_MAX) * m_particleSizeVariation * 2;

            float size = qMax((qreal)0.0 , m_particleSize + sizeVariation);
            float endSize = qMax((qreal)0.0 , sizeAtEnd + sizeVariation);

            datum->size = size;
            datum->endSize = endSize;

            emitParticle(datum->v8Value());//A chance for arbitrary JS changes

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
