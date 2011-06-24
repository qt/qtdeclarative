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
    , index(0)
{
    x = 0;
    y = 0;
    t = -1;
    size = 0;
    endSize = 0;
    sx = 0;
    sy = 0;
    ax = 0;
    ay = 0;
    xx = 1;
    xy = 0;
    yx = 0;
    yy = 1;
    rotation = 0;
    rotationSpeed = 0;
    autoRotate = 0;
    animIdx = -1;
    frameDuration = 1;
    frameCount = 0;
    animT = -1;
    color.r = 255;
    color.g = 255;
    color.b = 255;
    color.a = 255;
    r = 0;
    delegate = 0;
    modelIndex = -1;
}

QSGParticleSystem::QSGParticleSystem(QSGItem *parent) :
    QSGItem(parent), m_particle_count(0), m_running(true)
  , m_startTime(0), m_overwrite(false)
  , m_componentComplete(false)
{
    QSGParticleGroupData* gd = new QSGParticleGroupData;//Default group
    m_groupData.insert(0,gd);
    m_groupIds.insert("",0);
    m_nextGroupId = 1;

    connect(&m_painterMapper, SIGNAL(mapped(QObject*)),
            this, SLOT(loadPainter(QObject*)));
}

void QSGParticleSystem::registerParticlePainter(QSGParticlePainter* p)
{
    //TODO: a way to Unregister emitters, painters and affectors
    m_particlePainters << QPointer<QSGParticlePainter>(p);//###Set or uniqueness checking?
    connect(p, SIGNAL(particlesChanged(QStringList)),
            &m_painterMapper, SLOT(map()));
    loadPainter(p);
    p->update();//###Initial update here?
}

void QSGParticleSystem::registerParticleEmitter(QSGParticleEmitter* e)
{
    m_emitters << QPointer<QSGParticleEmitter>(e);//###How to get them out?
    connect(e, SIGNAL(particleCountChanged()),
            this, SLOT(emittersChanged()));
    connect(e, SIGNAL(particleChanged(QString)),
            this, SLOT(emittersChanged()));
    emittersChanged();
    e->reset();//Start, so that starttime factors appropriately
}

void QSGParticleSystem::registerParticleAffector(QSGParticleAffector* a)
{
    m_affectors << QPointer<QSGParticleAffector>(a);
}

void QSGParticleSystem::loadPainter(QObject *p)
{
    if(!m_componentComplete)
        return;

    QSGParticlePainter* painter = qobject_cast<QSGParticlePainter*>(p);
    Q_ASSERT(painter);//XXX
    foreach(QSGParticleGroupData* sg, m_groupData)
        sg->painters.remove(painter);
    int particleCount = 0;
    if(painter->particles().isEmpty()){//Uses default particle
        particleCount += m_groupData[0]->size;
        m_groupData[0]->painters << painter;
    }else{
        foreach(const QString &group, painter->particles()){
            particleCount += m_groupData[m_groupIds[group]]->size;
            m_groupData[m_groupIds[group]]->painters << painter;
        }
    }
    painter->setCount(particleCount);
    painter->update();//###Initial update here?
    return;
}

void QSGParticleSystem::emittersChanged()
{
    if(!m_componentComplete)
        return;

    m_emitters.removeAll(0);

    //Recalculate all counts, as emitter 'particle' may have changed as well
    //### Worth tracking previous 'particle' per emitter to do partial recalculations?
    m_particle_count = 0;

    int previousGroups = m_nextGroupId;
    QVector<int> previousSizes;
    previousSizes.resize(previousGroups);
    for(int i=0; i<previousGroups; i++)
        previousSizes[i] = m_groupData[i]->size;
    for(int i=0; i<previousGroups; i++)
        m_groupData[i]->size = 0;

    foreach(QSGParticleEmitter* e, m_emitters){//Populate groups and set sizes.
        if(!m_groupIds.contains(e->particle())
                || (!e->particle().isEmpty() && !m_groupIds[e->particle()])){//or it was accidentally inserted by a failed lookup earlier
            QSGParticleGroupData* gd = new QSGParticleGroupData;
            int id = m_nextGroupId++;
            m_groupIds.insert(e->particle(), id);
            m_groupData.insert(id, gd);
        }
        m_groupData[m_groupIds[e->particle()]]->size += e->particleCount();
        m_particle_count += e->particleCount();
        //###: Cull emptied groups?
    }

    foreach(QSGParticleGroupData* gd, m_groupData){//resize groups and update painters
        int id = m_groupData.key(gd);

        //TODO: Shrink back down! (but it has the problem of trying to remove the dead particles while maintaining integrity)
        gd->size = qMax(gd->size, id < previousGroups?previousSizes[id]:0);

        gd->data.resize(gd->size);
        if(id < previousGroups){
            for(int i=previousSizes[id]; i<gd->size; i++)
                gd->data[i] = 0;
            /*TODO:Consider salvaging partial updates, but have to batch changes to a single painter
            int delta = 0;
            delta = gd->size - previousSizes[id];
            foreach(QSGParticlePainter* painter, gd->painters){
                if(!painter->count() && delta){
                    painter->reset();
                    painter->update();
                }
                qDebug() << "Phi" << painter << painter->count() << delta;
                painter->setCount(painter->count() + delta);
            }
            */
        }
    }
    foreach(QSGParticlePainter *p, m_particlePainters)
        loadPainter(p);

    if(m_particle_count > 16000)//###Investigate if these limits are worth warning about?
        qWarning() << "Particle system arbitarily believes it has a vast number of particles (>16000). Expect poor performance";
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
    m_componentComplete = true;
    //if(!m_emitters.isEmpty() && !m_particlePainters.isEmpty())
    reset();
}

void QSGParticleSystem::reset()//TODO: Needed?
{
    if(!m_componentComplete)
        return;

    //Clear guarded pointers which have been deleted
    int cleared = 0;
    cleared += m_emitters.removeAll(0);
    cleared += m_particlePainters.removeAll(0);
    cleared += m_affectors.removeAll(0);
    //qDebug() << "Reset" << m_emitters.count() << m_particles.count() << "Cleared" << cleared;

    emittersChanged();

    //TODO: Reset data
//    foreach(QSGParticlePainter* p, m_particlePainters)
//        p->reset();
//    foreach(QSGParticleEmitter* e, m_emitters)
//        e->reset();
    //### Do affectors need reset too?

    if(!m_running)
        return;

    foreach(QSGParticlePainter *p, m_particlePainters){
        loadPainter(p);
        p->reset();
    }

    m_timestamp.start();//TODO: Better placement
    m_initialized = true;
}

QSGParticleData* QSGParticleSystem::newDatum(int groupId)
{

    Q_ASSERT(groupId < m_groupData.count());//XXX shouldn't really be an assert
    Q_ASSERT(m_groupData[groupId]->size);

    if( m_groupData[groupId]->nextIdx >= m_groupData[groupId]->size)
        m_groupData[groupId]->nextIdx = 0;
    int nextIdx = m_groupData[groupId]->nextIdx++;

    Q_ASSERT(nextIdx < m_groupData[groupId]->size);
    QSGParticleData* ret;
    if(m_groupData[groupId]->data[nextIdx]){//Recycle, it's faster.
        ret = m_groupData[groupId]->data[nextIdx];
        if(!m_overwrite && ret->stillAlive()){
            return 0;//Artificial longevity (or too fast emission) means this guy hasn't died. To maintain count, don't emit a new one
        }//###Reset?
    }else{
        ret = new QSGParticleData;
        m_groupData[groupId]->data[nextIdx] = ret;
    }

    ret->system = this;
    ret->index = nextIdx;
    ret->group = groupId;
    return ret;
}

void QSGParticleSystem::emitParticle(QSGParticleData* pd)
{// called from prepareNextFrame()->emitWindow - enforce?
    //Account for relative emitter position
    QPointF offset = this->mapFromItem(pd->e, QPointF(0, 0));
    if(!offset.isNull()){
        pd->x += offset.x();
        pd->y += offset.y();
    }

    foreach(QSGParticleAffector *a, m_affectors)
        if(a && a->m_needsReset)
            a->reset(pd);
    foreach(QSGParticlePainter* p, m_groupData[pd->group]->painters)
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
            foreach(QSGParticlePainter* p, m_groupData[d->group]->painters)
                if(p && d)
                    p->reload(d);
    }
    m_syncList << p;
    return m_timeInt;
}

//sets the x accleration without affecting the instantaneous x velocity or position
void QSGParticleData::setInstantaneousAX(qreal ax)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    qreal sx = (this->sx + t*this->ax) - t*ax;
    qreal ex = this->x + this->sx * t + 0.5 * this->ax * t * t;
    qreal x = ex - t*sx - 0.5 * t*t*ax;

    this->ax = ax;
    this->sx = sx;
    this->x = x;
}

//sets the x velocity without affecting the instantaneous x postion
void QSGParticleData::setInstantaneousSX(qreal vx)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    qreal sx = vx - t*this->ax;
    qreal ex = this->x + this->sx * t + 0.5 * this->ax * t * t;
    qreal x = ex - t*sx - 0.5 * t*t*this->ax;

    this->sx = sx;
    this->x = x;
}

//sets the instantaneous x postion
void QSGParticleData::setInstantaneousX(qreal x)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    this->x = x - t*this->sx - 0.5 * t*t*this->ax;
}

//sets the y accleration without affecting the instantaneous y velocity or position
void QSGParticleData::setInstantaneousAY(qreal ay)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    qreal sy = (this->sy + t*this->ay) - t*ay;
    qreal ey = this->y + this->sy * t + 0.5 * this->ay * t * t;
    qreal y = ey - t*sy - 0.5 * t*t*ay;

    this->ay = ay;
    this->sy = sy;
    this->y = y;
}

//sets the y velocity without affecting the instantaneous y position
void QSGParticleData::setInstantaneousSY(qreal vy)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    //qDebug() << t << (system->m_timeInt/1000.0) << this->x << this->sx << this->ax << this->x + this->sx * t + 0.5 * this->ax * t * t;
    qreal sy = vy - t*this->ay;
    qreal ey = this->y + this->sy * t + 0.5 * this->ay * t * t;
    qreal y = ey - t*sy - 0.5 * t*t*this->ay;

    this->sy = sy;
    this->y = y;
}

//sets the instantaneous Y position
void QSGParticleData::setInstantaneousY(qreal y)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    this->y = y - t*this->sy - 0.5 * t*t*this->ay;
}

qreal QSGParticleData::curX() const
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    return this->x + this->sx * t + 0.5 * this->ax * t * t;
}

qreal QSGParticleData::curSX() const
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    return this->sx + t*this->ax;
}

qreal QSGParticleData::curY() const
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    return y + sy * t + 0.5 * ay * t * t;
}

qreal QSGParticleData::curSY() const
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    return sy + t*ay;
}

void QSGParticleData::debugDump()
{
    qDebug() << "Particle" << group
             << "Pos: " << x << "," << y
             << "Vel: " << sx << "," << sy
             << "Acc: " << ax << "," << ay
             << "Size: " << size << "," << endSize
             << "Time: " << t << "," <<lifeSpan;
}

bool QSGParticleData::stillAlive()
{
    if(!system)
        return false;
    return (t + lifeSpan) > (system->m_timeInt/1000.0);
}

QT_END_NAMESPACE
