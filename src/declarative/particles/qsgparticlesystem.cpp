/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgparticlesystem_p.h"
#include <qsgnode.h>
#include "qsgparticleemitter_p.h"
#include "qsgparticleaffector_p.h"
#include "qsgparticlepainter_p.h"
#include <cmath>
#include <QDebug>

QT_BEGIN_NAMESPACE

QSGParticleData::QSGParticleData()
    : group(0)
    , e(0)
    , particleIndex(0)
    , systemIndex(0)
{
    pv.x = 0;
    pv.y = 0;
    pv.t = -1;
    pv.size = 0;
    pv.endSize = 0;
    pv.sx = 0;
    pv.sy = 0;
    pv.ax = 0;
    pv.ay = 0;
}

QSGParticleSystem::QSGParticleSystem(QSGItem *parent) :
    QSGItem(parent), m_particle_count(0), m_running(true) , m_startTime(0), m_overwrite(false)
{
    m_groupIds = QHash<QString, int>();
}

void QSGParticleSystem::registerParticleType(QSGParticlePainter* p)
{
    m_particles << QPointer<QSGParticlePainter>(p);//###Set or uniqueness checking?
    reset();
}

void QSGParticleSystem::registerParticleEmitter(QSGParticleEmitter* e)
{
    m_emitters << QPointer<QSGParticleEmitter>(e);//###How to get them out?
    connect(e, SIGNAL(particleCountChanged()),
            this, SLOT(countChanged()));
    connect(e, SIGNAL(particleChanged(QString)),
            this, SLOT(countChanged()));
    reset();
}

void QSGParticleSystem::registerParticleAffector(QSGParticleAffector* a)
{
    m_affectors << QPointer<QSGParticleAffector>(a);
    //reset();//TODO: Slim down the huge batch of resets at the start
}

void QSGParticleSystem::countChanged()
{
    reset();//Need to give Particles new Count
}

void QSGParticleSystem::setRunning(bool arg)
{
    if (m_running != arg) {
        m_running = arg;
        emit runningChanged(arg);
        reset();
    }
}

void QSGParticleSystem::componentComplete()
{
    QSGItem::componentComplete();
    reset();
}

void QSGParticleSystem::initializeSystem()
{
    int oldCount = m_particle_count;
    m_particle_count = 0;//TODO: Only when changed?

    //### Reset the data too?
    for(int i=0; i<oldCount; i++){
        if(m_data[i]){
            delete m_data[i];
            m_data[i] = 0;
        }
    }

    for(QHash<int, GroupData*>::iterator iter = m_groupData.begin(); iter != m_groupData.end(); iter++)
        delete (*iter);
    m_groupData.clear();
    m_groupIds.clear();

    GroupData* gd = new GroupData;//Default group
    gd->size = 0;
    gd->start = -1;
    gd->nextIdx = 0;
    m_groupData.insert(0,gd);
    m_groupIds.insert("",0);
    m_nextGroupId = 1;

    if(!m_emitters.count() || !m_particles.count())
        return;

    foreach(QSGParticleEmitter* e, m_emitters){
        if(!m_groupIds.contains(e->particle())
                || (!e->particle().isEmpty() && !m_groupIds[e->particle()])){//or it was accidentally inserted by a failed lookup earlier
            GroupData* gd = new GroupData;
            gd->size = 0;
            gd->start = -1;
            gd->nextIdx = 0;
            int id = m_nextGroupId++;
            m_groupIds.insert(e->particle(), id);
            m_groupData.insert(id, gd);
        }
        m_groupData[m_groupIds[e->particle()]]->size += e->particleCount();
    }

    for(QHash<int, GroupData*>::iterator iter = m_groupData.begin(); iter != m_groupData.end(); iter++){
        (*iter)->start = m_particle_count;
        m_particle_count += (*iter)->size;
    }
    m_data.resize(m_particle_count);
    for(int i=oldCount; i<m_particle_count; i++)
        m_data[i] = 0;//setup new ones

    if(m_particle_count > 16000)
        qWarning() << "Particle system contains a vast number of particles (>16000). Expect poor performance";

    foreach(QSGParticlePainter* particle, m_particles){
        int particleCount = 0;
        if(particle->particles().isEmpty()){//Uses default particle
            particleCount += m_groupData[0]->size;
            m_groupData[0]->types << particle;
        }else{
            foreach(const QString &group, particle->particles()){
                particleCount += m_groupData[m_groupIds[group]]->size;
                m_groupData[m_groupIds[group]]->types << particle;
            }
        }
        particle->setCount(particleCount);
        particle->m_pleaseReset = true;
    }

    m_timestamp.start();
    m_initialized = true;
    emit systemInitialized();
    qDebug() << "System Initialized. Size:" << m_particle_count;
}

void QSGParticleSystem::reset()
{
    //Clear guarded pointers which have been deleted
    int cleared = 0;
    cleared += m_emitters.removeAll(0);
    cleared += m_particles.removeAll(0);
    cleared += m_affectors.removeAll(0);
    //qDebug() << "Reset" << m_emitters.count() << m_particles.count() << "Cleared" << cleared;
    foreach(QSGParticlePainter* p, m_particles)
        p->reset();
    foreach(QSGParticleEmitter* e, m_emitters)
        e->reset();
    if(!m_running)
        return;
    initializeSystem();
    foreach(QSGParticlePainter* p, m_particles)
        p->update();
    foreach(QSGParticleEmitter* e, m_emitters)
        e->emitWindow(0);//Start, so that starttime factors appropriately
}

QSGParticleData* QSGParticleSystem::newDatum(int groupId)
{
    Q_ASSERT(groupId < m_groupData.count());//XXX shouldn't really be an assert
    Q_ASSERT(m_groupData[groupId]->size);
    int nextIdx = m_groupData[groupId]->start + m_groupData[groupId]->nextIdx++;
    if( m_groupData[groupId]->nextIdx >= m_groupData[groupId]->size)
        m_groupData[groupId]->nextIdx = 0;

    Q_ASSERT(nextIdx < m_data.size());
    QSGParticleData* ret;
    if(m_data[nextIdx]){//Recycle, it's faster.
        ret = m_data[nextIdx];
        if(!m_overwrite && ret->stillAlive()){
            return 0;//Artificial longevity (or too fast emission) means this guy hasn't died. To maintain count, don't emit a new one
        }//###Reset?
    }else{
        ret = new QSGParticleData;
        m_data[nextIdx] = ret;
    }

    ret->system = this;
    ret->systemIndex = nextIdx;
    ret->particleIndex = nextIdx - m_groupData[groupId]->start;
    ret->group = groupId;
    return ret;
}

void QSGParticleSystem::emitParticle(QSGParticleData* pd)
{// called from prepareNextFrame()->emitWindow - enforce?
    //Account for relative emitter position
    QPointF offset = this->mapFromItem(pd->e, QPointF(0, 0));
    if(!offset.isNull()){
        pd->pv.x += offset.x();
        pd->pv.y += offset.y();
    }

    foreach(QSGParticleAffector *a, m_affectors)
        if(a && a->m_needsReset)
            a->reset(pd->systemIndex);
    foreach(QSGParticlePainter* p, m_groupData[pd->group]->types)
        if(p)
            p->load(pd);
}



qint64 QSGParticleSystem::systemSync(QSGParticlePainter* p)
{
    if (!m_running)
        return 0;
    if (!m_initialized)
        return 0;//error in initialization

    if(m_syncList.isEmpty() || m_syncList.contains(p)){//Need to advance the simulation
        m_syncList.clear();

        //### Elapsed time never shrinks - may cause problems if left emitting for weeks at a time.
        qreal dt = m_timeInt / 1000.;
        m_timeInt = m_timestamp.elapsed() + m_startTime;
        qreal time =  m_timeInt / 1000.;
        dt = time - dt;
        m_needsReset.clear();
        foreach(QSGParticleEmitter* emitter, m_emitters)
            if(emitter)
                emitter->emitWindow(m_timeInt);
        foreach(QSGParticleAffector* a, m_affectors)
            if(a)
                a->affectSystem(dt);
        foreach(QSGParticleData* d, m_needsReset)
            foreach(QSGParticlePainter* p, m_groupData[d->group]->types)
                if(p && d)
                    p->reload(d);
    }
    m_syncList << p;
    return m_timeInt;
}

//sets the x accleration without affecting the instantaneous x velocity or position
void QSGParticleData::setInstantaneousAX(qreal ax)
{
    qreal t = (system->m_timeInt / 1000.0) - pv.t;
    qreal sx = (pv.sx + t*pv.ax) - t*ax;
    qreal ex = pv.x + pv.sx * t + 0.5 * pv.ax * t * t;
    qreal x = ex - t*sx - 0.5 * t*t*ax;

    pv.ax = ax;
    pv.sx = sx;
    pv.x = x;
}

//sets the x velocity without affecting the instantaneous x postion
void QSGParticleData::setInstantaneousSX(qreal vx)
{
    qreal t = (system->m_timeInt / 1000.0) - pv.t;
    qreal sx = vx - t*pv.ax;
    qreal ex = pv.x + pv.sx * t + 0.5 * pv.ax * t * t;
    qreal x = ex - t*sx - 0.5 * t*t*pv.ax;

    pv.sx = sx;
    pv.x = x;
}

//sets the instantaneous x postion
void QSGParticleData::setInstantaneousX(qreal x)
{
    qreal t = (system->m_timeInt / 1000.0) - pv.t;
    pv.x = x - t*pv.sx - 0.5 * t*t*pv.ax;
}

//sets the y accleration without affecting the instantaneous y velocity or position
void QSGParticleData::setInstantaneousAY(qreal ay)
{
    qreal t = (system->m_timeInt / 1000.0) - pv.t;
    qreal sy = (pv.sy + t*pv.ay) - t*ay;
    qreal ey = pv.y + pv.sy * t + 0.5 * pv.ay * t * t;
    qreal y = ey - t*sy - 0.5 * t*t*ay;

    pv.ay = ay;
    pv.sy = sy;
    pv.y = y;
}

//sets the y velocity without affecting the instantaneous y position
void QSGParticleData::setInstantaneousSY(qreal vy)
{
    qreal t = (system->m_timeInt / 1000.0) - pv.t;
    //qDebug() << t << (system->m_timeInt/1000.0) << pv.x << pv.sx << pv.ax << pv.x + pv.sx * t + 0.5 * pv.ax * t * t;
    qreal sy = vy - t*pv.ay;
    qreal ey = pv.y + pv.sy * t + 0.5 * pv.ay * t * t;
    qreal y = ey - t*sy - 0.5 * t*t*pv.ay;

    pv.sy = sy;
    pv.y = y;
}

//sets the instantaneous Y position
void QSGParticleData::setInstantaneousY(qreal y)
{
    qreal t = (system->m_timeInt / 1000.0) - pv.t;
    pv.y = y - t*pv.sy - 0.5 * t*t*pv.ay;
}

qreal QSGParticleData::curX() const
{
    qreal t = (system->m_timeInt / 1000.0) - pv.t;
    return pv.x + pv.sx * t + 0.5 * pv.ax * t * t;
}

qreal QSGParticleData::curSX() const
{
    qreal t = (system->m_timeInt / 1000.0) - pv.t;
    return pv.sx + t*pv.ax;
}

qreal QSGParticleData::curY() const
{
    qreal t = (system->m_timeInt / 1000.0) - pv.t;
    return pv.y + pv.sy * t + 0.5 * pv.ay * t * t;
}

qreal QSGParticleData::curSY() const
{
    qreal t = (system->m_timeInt / 1000.0) - pv.t;
    return pv.sy + t*pv.ay;
}

void QSGParticleData::debugDump()
{
    qDebug() << "Particle" << group
             << "Pos: " << pv.x << "," << pv.y
             << "Vel: " << pv.sx << "," << pv.sy
             << "Acc: " << pv.ax << "," << pv.ay
             << "Size: " << pv.size << "," << pv.endSize
             << "Time: " << pv.t << "," <<pv.lifeSpan;
}

bool QSGParticleData::stillAlive()
{
    if(!system)
        return false;
    return (pv.t + pv.lifeSpan) > (system->m_timeInt/1000.0);
}

QT_END_NAMESPACE
