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

#include "qsgparticlesystem_p.h"
#include <qsgnode.h>
#include "qsgparticleemitter_p.h"
#include "qsgparticleaffector_p.h"
#include "qsgparticlepainter_p.h"
#include "qsgspriteengine_p.h"
#include "qsgsprite_p.h"
#include "qsgv8particledata_p.h"

#include "qsgfollowemitter_p.h"//###For auto-follow on states, perhaps should be in emitter?
#include <private/qdeclarativeengine_p.h>
#include <cmath>
#include <QDebug>

QT_BEGIN_NAMESPACE
/*!
    \qmlclass ParticleSystem QSGParticleSystem
    \inqmlmodule QtQuick.Particles 2
    \brief The ParticleSystem brings together ParticlePainter, Emitter and Affector elements.

*/

/*!
    \qmlproperty bool QtQuick.Particles2::ParticleSystem::running

    If running is set to false, the particle system will not advance the simulation.
*/
/*!
    \qmlproperty int QtQuick.Particles2::ParticleSystem::startTime

    If start time is specified, then the system will simulate up to this time
    before the system starts playing. This allows you to appear to start with a
    fully populated particle system, instead of starting with no particles visible.
*/
/*!
    \qmlproperty list<Sprite> QtQuick.Particles2::ParticleSystem::particleStates

    You can define a sub-set of particle groups in this property in order to provide them
    with stochastic state transitions.

    Each QtQuick2::Sprite in this list is interpreted as corresponding to the particle group
    with ths same name. Any transitions defined in these sprites will take effect on the particle
    groups as well. Additionally FollowEmitters, Affectors and ParticlePainters definined
    inside one of these sprites are automatically associated with the corresponding particle group.
*/

const qreal EPSILON = 0.001;
//Utility functions for when within 1ms is close enough
bool timeEqualOrGreater(qreal a, qreal b){
    return (a+EPSILON >= b);
}

bool timeLess(qreal a, qreal b){
    return (a-EPSILON < b);
}

bool timeEqual(qreal a, qreal b){
    return (a+EPSILON > b) && (a-EPSILON < b);
}

int roundedTime(qreal a){// in ms
    return (int)qRound(a*1000.0);
}

QSGParticleDataHeap::QSGParticleDataHeap()
    : m_data(0)
{
    m_data.reserve(1000);
    clear();
}

void QSGParticleDataHeap::grow() //###Consider automatic growth vs resize() calls from GroupData
{
    m_data.resize(1 << ++m_size);
}

void QSGParticleDataHeap::insert(QSGParticleData* data)//TODO: Optimize 0 lifespan (or already dead) case
{
    int time = roundedTime(data->t + data->lifeSpan);
    if (m_lookups.contains(time)){
        m_data[m_lookups[time]].data << data;
        return;
    }
    if (m_end == (1 << m_size))
        grow();
    m_data[m_end].time = time;
    m_data[m_end].data.clear();
    m_data[m_end].data.insert(data);
    m_lookups.insert(time, m_end);
    bubbleUp(m_end++);
}

int QSGParticleDataHeap::top()
{
    if (m_end == 0)
        return 1 << 30;
    return m_data[0].time;
}

QSet<QSGParticleData*> QSGParticleDataHeap::pop()
{
    if (!m_end)
        return QSet<QSGParticleData*> ();
    QSet<QSGParticleData*> ret = m_data[0].data;
    m_lookups.remove(m_data[0].time);
    if (m_end == 1){
        --m_end;
    }else{
        m_data[0] = m_data[--m_end];
        bubbleDown(0);
    }
    return ret;
}

void QSGParticleDataHeap::clear()
{
    m_size = 0;
    m_end = 0;
    //m_size is in powers of two. So to start at 0 we have one allocated
    m_data.resize(1);
    m_lookups.clear();
}

bool QSGParticleDataHeap::contains(QSGParticleData* d)
{
    for (int i=0; i<m_end; i++)
        if (m_data[i].data.contains(d))
            return true;
    return false;
}

void QSGParticleDataHeap::swap(int a, int b)
{
    m_tmp = m_data[a];
    m_data[a] = m_data[b];
    m_data[b] = m_tmp;
    m_lookups[m_data[a].time] = a;
    m_lookups[m_data[b].time] = b;
}

void QSGParticleDataHeap::bubbleUp(int idx)//tends to be called once
{
    if (!idx)
        return;
    int parent = (idx-1)/2;
    if (m_data[idx].time < m_data[parent].time){
        swap(idx, parent);
        bubbleUp(parent);
    }
}

void QSGParticleDataHeap::bubbleDown(int idx)//tends to be called log n times
{
    int left = idx*2 + 1;
    if (left >= m_end)
        return;
    int lesser = left;
    int right = idx*2 + 2;
    if (right < m_end){
        if (m_data[left].time > m_data[right].time)
            lesser = right;
    }
    if (m_data[idx].time > m_data[lesser].time){
        swap(idx, lesser);
        bubbleDown(lesser);
    }
}

QSGParticleGroupData::QSGParticleGroupData(int id, QSGParticleSystem* sys):index(id),m_size(0),m_system(sys)
{
    initList();
}

QSGParticleGroupData::~QSGParticleGroupData()
{
    foreach (QSGParticleData* d, data)
        delete d;
}

int QSGParticleGroupData::size()
{
    return m_size;
}

QString QSGParticleGroupData::name()//### Worth caching as well?
{
    return m_system->m_groupIds.key(index);
}

void QSGParticleGroupData::setSize(int newSize){
    if (newSize == m_size)
        return;
    Q_ASSERT(newSize > m_size);//XXX allow shrinking
    data.resize(newSize);
    for (int i=m_size; i<newSize; i++){
        data[i] = new QSGParticleData(m_system);
        data[i]->group = index;
        data[i]->index = i;
        reusableIndexes << i;
    }
    int delta = newSize - m_size;
    m_size = newSize;
    foreach (QSGParticlePainter* p, painters)
        p->setCount(p->count() + delta);
}

void QSGParticleGroupData::initList()
{
    dataHeap.clear();
}

void QSGParticleGroupData::kill(QSGParticleData* d){
    Q_ASSERT(d->group == index);
    d->lifeSpan = 0;//Kill off
    foreach (QSGParticlePainter* p, painters)
        p->reload(d);
    reusableIndexes << d->index;
}

QSGParticleData* QSGParticleGroupData::newDatum(bool respectsLimits){
    while (dataHeap.top() <= m_system->m_timeInt){
        foreach (QSGParticleData* datum, dataHeap.pop()){
            if (!datum->stillAlive()){
                reusableIndexes << datum->index;
            }else{
                prepareRecycler(datum); //ttl has been altered mid-way, put it back
            }
        }
    }

    while (!reusableIndexes.empty()){
        int idx = *(reusableIndexes.begin());
        reusableIndexes.remove(idx);
        if (data[idx]->stillAlive()){// ### This means resurrection of dead particles. Is that allowed?
            prepareRecycler(data[idx]);
            continue;
        }
        return data[idx];
    }
    if (respectsLimits)
        return 0;

    int oldSize = m_size;
    setSize(oldSize + 10);//###+1,10%,+10? Choose something non-arbitrarily
    reusableIndexes.remove(oldSize);
    return data[oldSize];
}

void QSGParticleGroupData::prepareRecycler(QSGParticleData* d){
    dataHeap.insert(d);
}

QSGParticleData::QSGParticleData(QSGParticleSystem* sys)
    : group(0)
    , e(0)
    , system(sys)
    , index(0)
    , systemIndex(-1)
    , v8Datum(0)
{
    x = 0;
    y = 0;
    t = -1;
    lifeSpan = 0;
    size = 0;
    endSize = 0;
    vx = 0;
    vy = 0;
    ax = 0;
    ay = 0;
    xx = 1;
    xy = 0;
    yx = 0;
    yy = 1;
    rotation = 0;
    rotationSpeed = 0;
    autoRotate = 0;
    animIdx = 0;
    frameDuration = 1;
    frameCount = 1;
    animT = -1;
    color.r = 255;
    color.g = 255;
    color.b = 255;
    color.a = 255;
    r = 0;
    delegate = 0;
    modelIndex = -1;
}

void QSGParticleData::clone(const QSGParticleData& other)
{
    x = other.x;
    y = other.y;
    t = other.t;
    lifeSpan = other.lifeSpan;
    size = other.size;
    endSize = other.endSize;
    vx = other.vx;
    vy = other.vy;
    ax = other.ax;
    ay = other.ay;
    xx = other.xx;
    xy = other.xy;
    yx = other.yx;
    yy = other.yy;
    rotation = other.rotation;
    rotationSpeed = other.rotationSpeed;
    autoRotate = other.autoRotate;
    animIdx = other.animIdx;
    frameDuration = other.frameDuration;
    frameCount = other.frameCount;
    animT = other.animT;
    color.r = other.color.r;
    color.g = other.color.g;
    color.b = other.color.b;
    color.a = other.color.a;
    r = other.r;
    delegate = other.delegate;
    modelIndex = other.modelIndex;
}

QDeclarativeV8Handle QSGParticleData::v8Value()
{
    if (!v8Datum)
        v8Datum = new QSGV8ParticleData(QDeclarativeEnginePrivate::getV8Engine(qmlEngine(system)), this);
    return v8Datum->v8Value();
}
//sets the x accleration without affecting the instantaneous x velocity or position
void QSGParticleData::setInstantaneousAX(qreal ax)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    qreal vx = (this->vx + t*this->ax) - t*ax;
    qreal ex = this->x + this->vx * t + 0.5 * this->ax * t * t;
    qreal x = ex - t*vx - 0.5 * t*t*ax;

    this->ax = ax;
    this->vx = vx;
    this->x = x;
}

//sets the x velocity without affecting the instantaneous x postion
void QSGParticleData::setInstantaneousVX(qreal vx)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    qreal evx = vx - t*this->ax;
    qreal ex = this->x + this->vx * t + 0.5 * this->ax * t * t;
    qreal x = ex - t*evx - 0.5 * t*t*this->ax;

    this->vx = evx;
    this->x = x;
}

//sets the instantaneous x postion
void QSGParticleData::setInstantaneousX(qreal x)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    this->x = x - t*this->vx - 0.5 * t*t*this->ax;
}

//sets the y accleration without affecting the instantaneous y velocity or position
void QSGParticleData::setInstantaneousAY(qreal ay)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    qreal vy = (this->vy + t*this->ay) - t*ay;
    qreal ey = this->y + this->vy * t + 0.5 * this->ay * t * t;
    qreal y = ey - t*vy - 0.5 * t*t*ay;

    this->ay = ay;
    this->vy = vy;
    this->y = y;
}

//sets the y velocity without affecting the instantaneous y position
void QSGParticleData::setInstantaneousVY(qreal vy)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    qreal evy = vy - t*this->ay;
    qreal ey = this->y + this->vy * t + 0.5 * this->ay * t * t;
    qreal y = ey - t*evy - 0.5 * t*t*this->ay;

    this->vy = evy;
    this->y = y;
}

//sets the instantaneous Y position
void QSGParticleData::setInstantaneousY(qreal y)
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    this->y = y - t*this->vy - 0.5 * t*t*this->ay;
}

qreal QSGParticleData::curX() const
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    return this->x + this->vx * t + 0.5 * this->ax * t * t;
}

qreal QSGParticleData::curVX() const
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    return this->vx + t*this->ax;
}

qreal QSGParticleData::curY() const
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    return y + vy * t + 0.5 * ay * t * t;
}

qreal QSGParticleData::curVY() const
{
    qreal t = (system->m_timeInt / 1000.0) - this->t;
    return vy + t*ay;
}

void QSGParticleData::debugDump()
{
    qDebug() << "Particle" << systemIndex << group << "/" << index << stillAlive()
             << "Pos: " << x << "," << y
             //<< "Vel: " << vx << "," << sy
             //<< "Acc: " << ax << "," << ay
             << "Size: " << size << "," << endSize
             << "Time: " << t << "," <<lifeSpan << ";" << (system->m_timeInt / 1000.0) ;
}

bool QSGParticleData::stillAlive()
{
    if (!system)
        return false;
    //fprintf(stderr, "%.9lf %.9lf\n",((qreal)system->m_timeInt/1000.0), (t+lifeSpan));
    return (t + lifeSpan - EPSILON) > ((qreal)system->m_timeInt/1000.0);
}

float QSGParticleData::curSize()
{
    if (!system || !lifeSpan)
        return 0.0f;
    return size + (endSize - size) * (1 - (lifeLeft() / lifeSpan));
}

float QSGParticleData::lifeLeft()
{
    if (!system)
        return 0.0f;
    return (t + lifeSpan) - (system->m_timeInt/1000.0);
}

QSGParticleSystem::QSGParticleSystem(QSGItem *parent) :
    QSGItem(parent), m_particle_count(0), m_running(true)
  , m_startTime(0), m_nextIndex(0), m_componentComplete(false), m_spriteEngine(0)
{
    QSGParticleGroupData* gd = new QSGParticleGroupData(0, this);//Default group
    m_groupData.insert(0,gd);
    m_groupIds.insert("",0);
    m_nextGroupId = 1;

    connect(&m_painterMapper, SIGNAL(mapped(QObject*)),
            this, SLOT(loadPainter(QObject*)));
}

QSGParticleSystem::~QSGParticleSystem()
{
    foreach (QSGParticleGroupData* gd, m_groupData)
        delete gd;
}

QDeclarativeListProperty<QSGSprite> QSGParticleSystem::particleStates()
{
    return QDeclarativeListProperty<QSGSprite>(this, &m_states, spriteAppend, spriteCount, spriteAt, spriteClear);
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
    if (!m_componentComplete)
        return;

    QSGParticlePainter* painter = qobject_cast<QSGParticlePainter*>(p);
    Q_ASSERT(painter);//XXX
    foreach (QSGParticleGroupData* sg, m_groupData)
        sg->painters.remove(painter);
    int particleCount = 0;
    if (painter->particles().isEmpty()){//Uses default particle
        QStringList def;
        def << "";
        painter->setParticles(def);
        particleCount += m_groupData[0]->size();
        m_groupData[0]->painters << painter;
    }else{
        foreach (const QString &group, painter->particles()){
            if (group != QLatin1String("") && !m_groupIds[group]){//new group
                int id = m_nextGroupId++;
                QSGParticleGroupData* gd = new QSGParticleGroupData(id, this);
                m_groupIds.insert(group, id);
                m_groupData.insert(id, gd);
            }
            particleCount += m_groupData[m_groupIds[group]]->size();
            m_groupData[m_groupIds[group]]->painters << painter;
        }
    }
    painter->setCount(particleCount);
    painter->update();//###Initial update here?
    return;
}

void QSGParticleSystem::emittersChanged()
{
    if (!m_componentComplete)
        return;

    m_emitters.removeAll(0);


    QList<int> previousSizes;
    QList<int> newSizes;
    for (int i=0; i<m_nextGroupId; i++){
        previousSizes << m_groupData[i]->size();
        newSizes << 0;
    }

    foreach (QSGParticleEmitter* e, m_emitters){//Populate groups and set sizes.
        if (!m_groupIds.contains(e->particle())
                || (!e->particle().isEmpty() && !m_groupIds[e->particle()])){//or it was accidentally inserted by a failed lookup earlier
            int id = m_nextGroupId++;
            QSGParticleGroupData* gd = new QSGParticleGroupData(id, this);
            m_groupIds.insert(e->particle(), id);
            m_groupData.insert(id, gd);
            previousSizes << 0;
            newSizes << 0;
        }
        newSizes[m_groupIds[e->particle()]] += e->particleCount();
        //###: Cull emptied groups?
    }

    //TODO: Garbage collection?
    m_particle_count = 0;
    for (int i=0; i<m_nextGroupId; i++){
        m_groupData[i]->setSize(qMax(newSizes[i], previousSizes[i]));
        m_particle_count += m_groupData[i]->size();
    }

    Q_ASSERT(m_particle_count >= m_bySysIdx.size());//XXX when GC done right
    m_bySysIdx.resize(m_particle_count);

    foreach (QSGParticlePainter *p, m_particlePainters)
        loadPainter(p);

    if (!m_states.isEmpty())
        createEngine();

    if (m_particle_count > 16000)//###Investigate if these limits are worth warning about?
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

void QSGParticleSystem::stateRedirect(QDeclarativeListProperty<QObject> *prop, QObject *value)
{
    //Hooks up automatic state-associated stuff
    QSGParticleSystem* sys = qobject_cast<QSGParticleSystem*>(prop->object->parent());
    QSGSprite* sprite = qobject_cast<QSGSprite*>(prop->object);
    if (!sprite || !sys)
        return;
    QStringList list;
    list << sprite->name();
    QSGParticleAffector* a = qobject_cast<QSGParticleAffector*>(value);
    if (a){
        a->setParentItem(sys);
        a->setParticles(list);
        a->setSystem(sys);
        return;
    }
    QSGFollowEmitter* e = qobject_cast<QSGFollowEmitter*>(value);
    if (e){
        e->setParentItem(sys);
        e->setFollow(sprite->name());
        e->setSystem(sys);
        return;
    }
    QSGParticlePainter* p = qobject_cast<QSGParticlePainter*>(value);
    if (p){
        p->setParentItem(sys);
        p->setParticles(list);
        p->setSystem(sys);
        return;
    }
    qWarning() << value << " was placed inside a particle system state but cannot be taken into the particle system. It will be lost.";
}

void QSGParticleSystem::componentComplete()

{
    QSGItem::componentComplete();
    m_componentComplete = true;
    m_animation = new QSGParticleSystemAnimation(this);
    reset();//restarts animation as well
}

void QSGParticleSystem::reset()//TODO: Needed? Or just in component complete?
{
    if (!m_componentComplete)
        return;

    m_timeInt = 0;
    //Clear guarded pointers which have been deleted
    int cleared = 0;
    cleared += m_emitters.removeAll(0);
    cleared += m_particlePainters.removeAll(0);
    cleared += m_affectors.removeAll(0);

    emittersChanged();

    //TODO: Reset data
//    foreach (QSGParticlePainter* p, m_particlePainters)
//        p->reset();
//    foreach (QSGParticleEmitter* e, m_emitters)
//        e->reset();
    //### Do affectors need reset too?

    if (!m_running)
        return;

    foreach (QSGParticlePainter *p, m_particlePainters){
        loadPainter(p);
        p->reset();
    }

    if (m_animation){
        m_animation->stop();
        m_animation->start();
    }
    m_initialized = true;
}

void QSGParticleSystem::createEngine()
{
    if (!m_componentComplete)
        return;
    //### Solve the losses if size/states go down
    foreach (QSGSprite* sprite, m_states){
        bool exists = false;
        foreach (const QString &name, m_groupIds.keys())
            if (sprite->name() == name)
                exists = true;
        if (!exists){
            int id = m_nextGroupId++;
            QSGParticleGroupData* gd = new QSGParticleGroupData(id, this);
            m_groupIds.insert(sprite->name(), id);
            m_groupData.insert(id, gd);
        }
    }

    if (m_states.count()){
        //Reorder Sprite List so as to have the same order as groups
        QList<QSGSprite*> newList;
        for (int i=0; i<m_nextGroupId; i++){
            bool exists = false;
            QString name = m_groupData[i]->name();
            foreach (QSGSprite* existing, m_states){
                if (existing->name() == name){
                    newList << existing;
                    exists = true;
                }
            }
            if (!exists){
                newList << new QSGSprite(this);
                newList.back()->setName(name);
            }
        }
        m_states = newList;

        if (!m_spriteEngine)
            m_spriteEngine = new QSGSpriteEngine(this);
        m_spriteEngine->setCount(m_particle_count);
        m_spriteEngine->m_states = m_states;

        connect(m_spriteEngine, SIGNAL(stateChanged(int)),
                this, SLOT(particleStateChange(int)));

    }else{
        if (m_spriteEngine)
            delete m_spriteEngine;
        m_spriteEngine = 0;
    }

}

void QSGParticleSystem::particleStateChange(int idx)
{
    moveGroups(m_bySysIdx[idx], m_spriteEngine->spriteState(idx));
}

void QSGParticleSystem::moveGroups(QSGParticleData *d, int newGIdx)
{
    QSGParticleData* pd = newDatum(newGIdx, false, d->systemIndex);
    pd->clone(*d);
    finishNewDatum(pd);

    d->systemIndex = -1;
    m_groupData[d->group]->kill(d);
}

int QSGParticleSystem::nextSystemIndex()
{
    if (!m_reusableIndexes.isEmpty()){
        int ret = *(m_reusableIndexes.begin());
        m_reusableIndexes.remove(ret);
        return ret;
    }
    if (m_nextIndex >= m_bySysIdx.size()){
        m_bySysIdx.resize(m_bySysIdx.size() < 10 ? 10 : m_bySysIdx.size()*1.1);//###+1,10%,+10? Choose something non-arbitrarily
        if (m_spriteEngine)
            m_spriteEngine->setCount(m_bySysIdx.size());

    }
    return m_nextIndex++;
}

QSGParticleData* QSGParticleSystem::newDatum(int groupId, bool respectLimits, int sysIndex)
{
    Q_ASSERT(groupId < m_groupData.count());//XXX shouldn't really be an assert

    QSGParticleData* ret = m_groupData[groupId]->newDatum(respectLimits);
    if (!ret){
        return 0;
    }
    if (sysIndex == -1){
        if (ret->systemIndex == -1)
            ret->systemIndex = nextSystemIndex();
    }else{
        if (ret->systemIndex != -1){
            if (m_spriteEngine)
                m_spriteEngine->stopSprite(ret->systemIndex);
            m_reusableIndexes << ret->systemIndex;
            m_bySysIdx[ret->systemIndex] = 0;
        }
        ret->systemIndex = sysIndex;
    }
    m_bySysIdx[ret->systemIndex] = ret;

    if (m_spriteEngine)
        m_spriteEngine->startSprite(ret->systemIndex, ret->group);

    return ret;
}

void QSGParticleSystem::emitParticle(QSGParticleData* pd)
{// called from prepareNextFrame()->emitWindow - enforce?
    //Account for relative emitter position
    QPointF offset = this->mapFromItem(pd->e, QPointF(0, 0));
    if (!offset.isNull()){
        pd->x += offset.x();
        pd->y += offset.y();
    }

    finishNewDatum(pd);
}

void QSGParticleSystem::finishNewDatum(QSGParticleData *pd){
    Q_ASSERT(pd);
    m_groupData[pd->group]->prepareRecycler(pd);

    foreach (QSGParticleAffector *a, m_affectors)
        if (a && a->m_needsReset)
            a->reset(pd);
    foreach (QSGParticlePainter* p, m_groupData[pd->group]->painters)
        if (p)
            p->load(pd);
}

void QSGParticleSystem::updateCurrentTime( int currentTime )
{
    if (!m_running)
        return;
    if (!m_initialized)
        return;//error in initialization

    //### Elapsed time never shrinks - may cause problems if left emitting for weeks at a time.
    qreal dt = m_timeInt / 1000.;
    m_timeInt = currentTime + m_startTime;
    qreal time =  m_timeInt / 1000.;
    dt = time - dt;
    m_needsReset.clear();
    if (m_spriteEngine)
        m_spriteEngine->updateSprites(m_timeInt);

    foreach (QSGParticleEmitter* emitter, m_emitters)
        if (emitter)
            emitter->emitWindow(m_timeInt);
    foreach (QSGParticleAffector* a, m_affectors)
        if (a)
            a->affectSystem(dt);
    foreach (QSGParticleData* d, m_needsReset)
        foreach (QSGParticlePainter* p, m_groupData[d->group]->painters)
            if (p && d)
                p->reload(d);
}

int QSGParticleSystem::systemSync(QSGParticlePainter* p)
{
    if (!m_running)
        return 0;
    if (!m_initialized)
        return 0;//error in initialization
    p->performPendingCommits();
    return m_timeInt;
}


QT_END_NAMESPACE
