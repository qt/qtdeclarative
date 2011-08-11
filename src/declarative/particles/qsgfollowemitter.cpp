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

#include "qsgfollowemitter_p.h"
#include "qsgparticlepainter_p.h"//TODO: What was this for again?
#include <cmath>
QT_BEGIN_NAMESPACE

/*!
    \qmlclass FollowEmitter QSGFollowEmitter
    \inqmlmodule QtQuick.Particles 2
    \inherits QSGParticleEmitter
    \brief The FollowEmitter element allows you to emit logical particles from other logical particles.

    This element emits logical particles into the ParticleSystem, with the
    starting positions based on those of other logical particles.
*/
QSGFollowEmitter::QSGFollowEmitter(QSGItem *parent) :
    QSGParticleEmitter(parent)
  , m_particlesPerParticlePerSecond(0)
  , m_lastTimeStamp(0)
  , m_emitterXVariation(0)
  , m_emitterYVariation(0)
  , m_followCount(0)
  , m_emissionExtruder(0)
  , m_defaultEmissionExtruder(new QSGParticleExtruder(this))
{
    //TODO: If followed increased their size
    connect(this, SIGNAL(followChanged(QString)),
            this, SLOT(recalcParticlesPerSecond()));
    connect(this, SIGNAL(particleDurationChanged(int)),
            this, SLOT(recalcParticlesPerSecond()));
    connect(this, SIGNAL(particlesPerParticlePerSecondChanged(int)),
            this, SLOT(recalcParticlesPerSecond()));
}

/*!
    \qmlproperty string QtQuick.Particles2::FollowEmitter::follow

    The type of logical particle which this is emitting from.
*/

/*!
    \qmlproperty Shape QtQuick.Particles2::FollowEmitter::emitShape

    As the area of a FollowEmitter is the area it follows, a separate shape can be provided
    to be the shape it emits out of.
*/
/*!
    \qmlproperty real QtQuick.Particles2::FollowEmitter::emitWidth
*/
/*!
    \qmlproperty real QtQuick.Particles2::FollowEmitter::emitHeight
*/
/*!
    \qmlproperty real QtQuick.Particles2::FollowEmitter::emitRatePerParticle
*/


void QSGFollowEmitter::recalcParticlesPerSecond(){
    if (!m_system)
        return;
    m_followCount = m_system->m_groupData[m_system->m_groupIds[m_follow]]->size();
    if (!m_followCount){
        setParticlesPerSecond(1);//XXX: Fix this horrendous hack, needed so they aren't turned off from start (causes crashes - test that when gone you don't crash with 0 PPPS)
    }else{
        setParticlesPerSecond(m_particlesPerParticlePerSecond * m_followCount);
        m_lastEmission.resize(m_followCount);
        m_lastEmission.fill(m_lastTimeStamp);
    }
}

void QSGFollowEmitter::reset()
{
    m_followCount = 0;
}

void QSGFollowEmitter::emitWindow(int timeStamp)
{
    if (m_system == 0)
        return;
    if (!m_emitting && !m_burstLeft && m_burstQueue.isEmpty())
        return;
    if (m_followCount != m_system->m_groupData[m_system->m_groupIds[m_follow]]->size()){
        qreal oldPPS = m_particlesPerSecond;
        recalcParticlesPerSecond();
        if (m_particlesPerSecond != oldPPS)
            return;//system may need to update
    }

    if (m_burstLeft){
        m_burstLeft -= timeStamp - m_lastTimeStamp * 1000.;
        if (m_burstLeft < 0){
            timeStamp += m_burstLeft;
            m_burstLeft = 0;
        }
    }

    qreal time = timeStamp / 1000.;
    qreal particleRatio = 1. / m_particlesPerParticlePerSecond;
    qreal pt;

    //Have to map it into this system, because particlesystem automaps it back
    QPointF offset = m_system->mapFromItem(this, QPointF(0, 0));
    qreal sizeAtEnd = m_particleEndSize >= 0 ? m_particleEndSize : m_particleSize;

    int gId = m_system->m_groupIds[m_follow];
    int gId2 = m_system->m_groupIds[m_particle];
    foreach (QSGParticleData *d, m_system->m_groupData[gId]->data){
        if (!d || !d->stillAlive()){
            m_lastEmission[d->index] = time; //Should only start emitting when it returns to life
            continue;
        }
        pt = m_lastEmission[d->index];
        if (pt < d->t)
            pt = d->t;

        if ((width() || height()) && !effectiveExtruder()->contains(QRectF(offset.x(), offset.y(), width(), height()),QPointF(d->curX(), d->curY()))){
            m_lastEmission[d->index] = time;//jump over this time period without emitting, because it's outside
            continue;
        }
        while (pt < time || !m_burstQueue.isEmpty()){
            QSGParticleData* datum = m_system->newDatum(gId2);
            if (datum){//else, skip this emission
                datum->e = this;//###useful?

                // Particle timestamp
                datum->t = pt;
                datum->lifeSpan =
                        (m_particleDuration
                         + ((rand() % ((m_particleDurationVariation*2) + 1)) - m_particleDurationVariation))
                        / 1000.0;

                // Particle position
                // Note that burst location doesn't get used for follow emitter
                qreal followT =  pt - d->t;
                qreal followT2 = followT * followT * 0.5;
                qreal sizeOffset = d->size/2;//TODO: Current size? As an option
                //TODO: Set variations
                //Subtract offset, because PS expects this in emitter coordinates
                QRectF boundsRect(d->x - offset.x() + d->vx * followT + d->ax * followT2 - m_emitterXVariation/2,
                                  d->y - offset.y() + d->vy * followT + d->ay * followT2 - m_emitterYVariation/2,
                                  m_emitterXVariation,
                                  m_emitterYVariation);
    //            QRectF boundsRect(d->x + d->vx * followT + d->ax * followT2 + offset.x() - sizeOffset,
    //                              d->y + d->vy * followT + d->ay * followT2 + offset.y() - sizeOffset,
    //                              sizeOffset*2,
    //                              sizeOffset*2);

                QSGParticleExtruder* effectiveEmissionExtruder = m_emissionExtruder ? m_emissionExtruder : m_defaultEmissionExtruder;
                const QPointF &newPos = effectiveEmissionExtruder->extrude(boundsRect);
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

                float size = qMax((qreal)0.0, m_particleSize + sizeVariation);
                float endSize = qMax((qreal)0.0, sizeAtEnd + sizeVariation);

                datum->size = size * float(m_emitting);
                datum->endSize = endSize * float(m_emitting);

                m_system->emitParticle(datum);
            }
            if (!m_burstQueue.isEmpty()){
                m_burstQueue.first().first--;
                if (m_burstQueue.first().first <= 0)
                    m_burstQueue.pop_front();
            }else{
                pt += particleRatio;
            }
        }
        m_lastEmission[d->index] = pt;
    }

    m_lastTimeStamp = time;
}
QT_END_NAMESPACE
