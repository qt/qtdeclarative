// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickparticlesystem_p.h"
#include <QtQuick/qsgnode.h>
#include "qquickparticleemitter_p.h"
#include "qquickparticleaffector_p.h"
#include "qquickparticlepainter_p.h"
#include <private/qquickspriteengine_p.h>
#include <private/qquicksprite_p.h>
#include "qquickv4particledata_p.h"
#include "qquickparticlegroup_p.h"

#include "qquicktrailemitter_p.h"//###For auto-follow on states, perhaps should be in emitter?
#include <private/qqmlengine_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <cmath>
#include <QDebug>

QT_BEGIN_NAMESPACE
//###Switch to define later, for now user-friendly (no compilation) debugging is worth it
DEFINE_BOOL_CONFIG_OPTION(qmlParticlesDebug, QML_PARTICLES_DEBUG)


/* \internal ParticleSystem internals documentation

   Affectors, Painters, Emitters and Groups all register themselves on construction as a callback
   from their setSystem (or componentComplete if they have a system from a parent).

   Particle data is stored by group, They have a group index (used by the particle system almost
   everywhere) and a global index (used by the Stochastic state engine powering stochastic group
   transitions). Each group has a recycling list/heap that stores the particle data.

   The recycling list/heap is a heap of particle data sorted by when they're expected to die. If
   they die prematurely then they are marked as reusable (and will probably still be alive when
   they exit the heap). If they have their life extended, then they aren't dead when expected.
   If this happens, they go back in the heap with the new estimate. If they have died on schedule,
   then the indexes are marked as reusable. If no indexes are reusable when new particles are
   requested, then the list is extended. This relatively complex datastructure is because memory
   allocation and deallocation on this scale proved to be a significant performance cost. In order
   to reuse the indexes validly (even when particles can have their life extended or cut short
   dynamically, or particle counts grow) this seemed to be the most efficient option for keeping
   track of which indices could be reused.

   When a new particle is emitted, the emitter gets a new datum from the group (through the
   system), and sets properties on it. Then it's passed back to the group briefly so that it can
   now guess when the particle will die. Then the painters get a change to initialize properties
   as well, since particle data includes shared data from painters as well as logical particle
   data.

   Every animation advance, the simulation advances by running all emitters for the elapsed
   duration, then running all affectors, then telling all particle painters to update changed
   particles. The ParticlePainter superclass stores these changes, and they are implemented
   when the painter is called to paint in the render thread.

   Particle group changes move the particle from one group to another by killing the old particle
   and then creating a new one with the same data in the new group.

   Note that currently groups only grow. Given that data is stored in vectors, it is non-trivial
   to pluck out the unused indexes when the count goes down. Given the dynamic nature of the
   system, it is difficult to tell if those unused data instances will be used again. Still,
   some form of garbage collection is on the long term plan.
*/

/*!
    \qmltype ParticleSystem
//!    \nativetype QQuickParticleSystem
    \inqmlmodule QtQuick.Particles
    \brief A system which includes particle painter, emitter, and affector types.
    \ingroup qtquick-particles

*/

/*!
    \qmlproperty bool QtQuick.Particles::ParticleSystem::running

    If running is set to false, the particle system will stop the simulation. All particles
    will be destroyed when the system is set to running again.

    It can also be controlled with the start() and stop() methods.
*/


/*!
    \qmlproperty bool QtQuick.Particles::ParticleSystem::paused

    If paused is set to true, the particle system will not advance the simulation. When
    paused is set to false again, the simulation will resume from the same point it was
    paused.

    The simulation will automatically pause if it detects that there are no live particles
    left, and unpause when new live particles are added.

    It can also be controlled with the pause() and resume() methods.
*/

/*!
    \qmlproperty bool QtQuick.Particles::ParticleSystem::empty

    empty is set to true when there are no live particles left in the system.

    You can use this to pause the system, keeping it from spending any time updating,
    but you will need to resume it in order for additional particles to be generated
    by the system.

    To kill all the particles in the system, use an Age affector.
*/

/*!
    \qmlproperty list<Sprite> QtQuick.Particles::ParticleSystem::particleStates

    You can define a sub-set of particle groups in this property in order to provide them
    with stochastic state transitions.

    Each QtQuick::Sprite in this list is interpreted as corresponding to the particle group
    with the same name. Any transitions defined in these sprites will take effect on the particle
    groups as well. Additionally TrailEmitters, Affectors and ParticlePainters defined
    inside one of these sprites are automatically associated with the corresponding particle group.
*/

/*!
    \qmlmethod QtQuick.Particles::ParticleSystem::pause()

    Pauses the simulation if it is running.

    \sa resume, paused
*/

/*!
    \qmlmethod QtQuick.Particles::ParticleSystem::resume()

    Resumes the simulation if it is paused.

    \sa pause, paused
*/

/*!
    \qmlmethod QtQuick.Particles::ParticleSystem::start()

    Starts the simulation if it has not already running.

    \sa stop, restart, running
*/

/*!
    \qmlmethod QtQuick.Particles::ParticleSystem::stop()

    Stops the simulation if it is running.

    \sa start, restart, running
*/

/*!
    \qmlmethod QtQuick.Particles::ParticleSystem::restart()

    Stops the simulation if it is running, and then starts it.

    \sa start, stop, running
*/
/*!
    \qmlmethod QtQuick.Particles::ParticleSystem::reset()

    Discards all currently existing particles.

*/

static inline int roundedTime(qreal a)
{// in ms
    return (int)qRound(a*1000.0);
}

QQuickParticleDataHeap::QQuickParticleDataHeap()
    : m_data(0)
{
    m_data.reserve(1000);
    clear();
}

void QQuickParticleDataHeap::grow() //###Consider automatic growth vs resize() calls from GroupData
{
    m_data.resize(qsizetype(1) << ++m_size);
}

void QQuickParticleDataHeap::insert(QQuickParticleData* data)
{
    insertTimed(data, roundedTime(data->t + data->lifeSpan));
}

void QQuickParticleDataHeap::insertTimed(QQuickParticleData* data, int time)
{
    //TODO: Optimize 0 lifespan (or already dead) case
    if (m_lookups.contains(time)) {
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

int QQuickParticleDataHeap::top()
{
    Q_ASSERT(!isEmpty());
    return m_data[0].time;
}

QSet<QQuickParticleData*> QQuickParticleDataHeap::pop()
{
    if (!m_end)
        return QSet<QQuickParticleData*> ();
    QSet<QQuickParticleData*> ret = m_data[0].data;
    m_lookups.remove(m_data[0].time);
    if (m_end == 1) {
        --m_end;
    } else {
        m_data[0] = m_data[--m_end];
        bubbleDown(0);
    }
    return ret;
}

void QQuickParticleDataHeap::clear()
{
    m_size = 0;
    m_end = 0;
    //m_size is in powers of two. So to start at 0 we have one allocated
    m_data.resize(1);
    m_lookups.clear();
}

bool QQuickParticleDataHeap::contains(QQuickParticleData* d)
{
    for (int i=0; i<m_end; i++)
        if (m_data[i].data.contains(d))
            return true;
    return false;
}

void QQuickParticleDataHeap::swap(int a, int b)
{
    m_tmp = m_data[a];
    m_data[a] = m_data[b];
    m_data[b] = m_tmp;
    m_lookups[m_data[a].time] = a;
    m_lookups[m_data[b].time] = b;
}

void QQuickParticleDataHeap::bubbleUp(int idx)//tends to be called once
{
    if (!idx)
        return;
    int parent = (idx-1)/2;
    if (m_data[idx].time < m_data[parent].time) {
        swap(idx, parent);
        bubbleUp(parent);
    }
}

void QQuickParticleDataHeap::bubbleDown(int idx)//tends to be called log n times
{
    int left = idx*2 + 1;
    if (left >= m_end)
        return;
    int lesser = left;
    int right = idx*2 + 2;
    if (right < m_end) {
        if (m_data[left].time > m_data[right].time)
            lesser = right;
    }
    if (m_data[idx].time > m_data[lesser].time) {
        swap(idx, lesser);
        bubbleDown(lesser);
    }
}

QQuickParticleGroupData::QQuickParticleGroupData(const QString &name, QQuickParticleSystem* sys)
    : index(sys->registerParticleGroupData(name, this))
    , m_size(0)
    , m_system(sys)
{
    initList();
}

QQuickParticleGroupData::~QQuickParticleGroupData()
{
    for (QQuickParticleData *d : std::as_const(data))
        delete d;
}

QString QQuickParticleGroupData::name() const//### Worth caching as well?
{
    return m_system->groupIds.key(index);
}

void QQuickParticleGroupData::setSize(int newSize)
{
    if (newSize == m_size)
        return;
    Q_ASSERT(newSize > m_size);//XXX allow shrinking
    data.resize(newSize);
    freeList.resize(newSize);
    for (int i=m_size; i<newSize; i++) {
        data[i] = new QQuickParticleData;
        data[i]->groupId = index;
        data[i]->index = i;
    }
    int delta = newSize - m_size;
    m_size = newSize;
    for (QQuickParticlePainter *p : std::as_const(painters))
        p->setCount(p->count() + delta);
}

void QQuickParticleGroupData::initList()
{
    dataHeap.clear();
}

void QQuickParticleGroupData::kill(QQuickParticleData* d)
{
    Q_ASSERT(d->groupId == index);
    d->lifeSpan = 0;//Kill off
    for (QQuickParticlePainter *p : std::as_const(painters))
        p->reload(d);
    freeList.free(d->index);
}

QQuickParticleData* QQuickParticleGroupData::newDatum(bool respectsLimits)
{
    //recycle();//Extra recycler round to be sure?

    while (freeList.hasUnusedEntries()) {
        int idx = freeList.alloc();
        if (data[idx]->stillAlive(m_system)) {// ### This means resurrection of 'dead' particles. Is that allowed?
            prepareRecycler(data[idx]);
            continue;
        }
        return data[idx];
    }
    if (respectsLimits)
        return nullptr;

    int oldSize = m_size;
    setSize(oldSize + 10);//###+1,10%,+10? Choose something non-arbitrarily
    int idx = freeList.alloc();
    Q_ASSERT(idx == oldSize);
    return data[idx];
}

bool QQuickParticleGroupData::recycle()
{
    m_latestAliveParticles.clear();

    while (!dataHeap.isEmpty() && dataHeap.top() <= m_system->timeInt) {
        for (QQuickParticleData *datum : dataHeap.pop()) {
            if (!datum->stillAlive(m_system)) {
                freeList.free(datum->index);
            } else {
                m_latestAliveParticles.push_back(datum);
            }
        }
    }

    for (auto particle : m_latestAliveParticles)
        prepareRecycler(particle); //ttl has been altered mid-way, put it back

    //TODO: If the data is clear, gc (consider shrinking stack size)?
    return freeList.count() == 0;
}

void QQuickParticleGroupData::prepareRecycler(QQuickParticleData* d)
{
    if (d->lifeSpan*1000 < m_system->maxLife) {
        dataHeap.insert(d);
    } else {
        int extend = 2 * m_system->maxLife / 3;
        while ((roundedTime(d->t) + extend) <= m_system->timeInt)
            d->extendLife(m_system->maxLife / 3000.0, m_system);
        dataHeap.insertTimed(d, roundedTime(d->t) + extend);
    }
}

QQuickV4ParticleData QQuickParticleData::v4Value(QQuickParticleSystem *particleSystem)
{
    return QQuickV4ParticleData(this, particleSystem);
}

void QQuickParticleData::debugDump(QQuickParticleSystem* particleSystem) const
{
    qDebug() << "Particle" << systemIndex << groupId << "/" << index << stillAlive(particleSystem)
             << "Pos: " << x << "," << y
             << "Vel: " << vx << "," << vy
             << "Acc: " << ax << "," << ay
             << "Size: " << size << "," << endSize
             << "Time: " << t << "," <<lifeSpan << ";" << (particleSystem->timeInt / 1000.0) ;
}

void QQuickParticleData::extendLife(float time, QQuickParticleSystem* particleSystem)
{
    qreal newX = curX(particleSystem);
    qreal newY = curY(particleSystem);
    qreal newVX = curVX(particleSystem);
    qreal newVY = curVY(particleSystem);

    t += time;
    animT += time;

    qreal elapsed = (particleSystem->timeInt / 1000.0) - t;
    qreal evy = newVY - elapsed*ay;
    qreal ey = newY - elapsed*evy - 0.5 * elapsed*elapsed*ay;
    qreal evx = newVX - elapsed*ax;
    qreal ex = newX - elapsed*evx - 0.5 * elapsed*elapsed*ax;

    x = ex;
    vx = evx;
    y = ey;
    vy = evy;
}

QQuickParticleSystem::QQuickParticleSystem(QQuickItem *parent) :
    QQuickItem(parent),
    stateEngine(nullptr),
    nextFreeGroupId(0),
    m_animation(nullptr),
    m_running(true),
    initialized(0),
    particleCount(0),
    m_nextIndex(0),
    m_componentComplete(false),
    m_paused(false),
    m_empty(true)
{
    m_debugMode = qmlParticlesDebug();
}

QQuickParticleSystem::~QQuickParticleSystem()
{
    for (QQuickParticleGroupData *gd : std::as_const(groupData))
        delete gd;
}

void QQuickParticleSystem::initGroups()
{
    m_reusableIndexes.clear();
    m_nextIndex = 0;

    qDeleteAll(groupData);
    groupData.clear();
    groupIds.clear();
    nextFreeGroupId = 0;

    for (auto e : std::as_const(m_emitters)) {
        e->reclaculateGroupId();
    }
    for (QQuickParticlePainter *p : std::as_const(m_painters)) {
        p->recalculateGroupIds();
    }

    QQuickParticleGroupData *pd = new QQuickParticleGroupData(QString(), this); // Default group
    Q_ASSERT(pd->index == 0);
    Q_UNUSED(pd);
}

void QQuickParticleSystem::registerParticlePainter(QQuickParticlePainter* p)
{
    if (m_debugMode)
        qDebug() << "Registering Painter" << p << "to" << this;
    //TODO: a way to Unregister emitters, painters and affectors
    m_painters << QPointer<QQuickParticlePainter>(p);//###Set or uniqueness checking?

    connect(p, &QQuickParticlePainter::groupsChanged, this, [this, p] { this->loadPainter(p); }, Qt::QueuedConnection);
    loadPainter(p);
}

void QQuickParticleSystem::registerParticleEmitter(QQuickParticleEmitter* e)
{
    if (m_debugMode)
        qDebug() << "Registering Emitter" << e << "to" << this;
    m_emitters << QPointer<QQuickParticleEmitter>(e);//###How to get them out?
}

void QQuickParticleSystem::finishRegisteringParticleEmitter(QQuickParticleEmitter* e)
{
    connect(e, &QQuickParticleEmitter::particleCountChanged,
            this, &QQuickParticleSystem::emittersChanged);
    connect(e, &QQuickParticleEmitter::groupChanged,
            this, &QQuickParticleSystem::emittersChanged);
    if (m_componentComplete)
        emitterAdded(e);
    e->reset();//Start, so that starttime factors appropriately
}

void QQuickParticleSystem::registerParticleAffector(QQuickParticleAffector* a)
{
    if (m_debugMode)
        qDebug() << "Registering Affector" << a << "to" << this;
    if (!m_affectors.contains(a))
        m_affectors << QPointer<QQuickParticleAffector>(a);
}

void QQuickParticleSystem::registerParticleGroup(QQuickParticleGroup* g)
{
    if (m_debugMode)
        qDebug() << "Registering Group" << g << "to" << this;
    m_groups << QPointer<QQuickParticleGroup>(g);
    createEngine();
}

void QQuickParticleSystem::setRunning(bool arg)
{
    if (m_running != arg) {
        m_running = arg;
        emit runningChanged(arg);
        setPaused(false);
        if (m_animation)//Not created until componentCompleted
            m_running ? m_animation->start() : m_animation->stop();
        reset();
    }
}

void QQuickParticleSystem::setPaused(bool arg) {
    if (m_paused != arg) {
        m_paused = arg;
        if (m_animation && m_animation->state() != QAbstractAnimation::Stopped)
            m_paused ? m_animation->pause() : m_animation->resume();
        if (!m_paused) {
            for (QQuickParticlePainter *p : std::as_const(m_painters)) {
                if (p) {
                    p->update();
                }
            }
        }
        emit pausedChanged(arg);
    }
}

void QQuickParticleSystem::statePropertyRedirect(QQmlListProperty<QObject> *prop, QObject *value)
{
    //Hooks up automatic state-associated stuff
    QQuickParticleSystem* sys = qobject_cast<QQuickParticleSystem*>(prop->object->parent());
    QQuickParticleGroup* group = qobject_cast<QQuickParticleGroup*>(prop->object);
    if (!group || !sys || !value)
        return;
    stateRedirect(group, sys, value);
}

void QQuickParticleSystem::stateRedirect(QQuickParticleGroup* group, QQuickParticleSystem* sys, QObject *value)
{
    QStringList list;
    list << group->name();
    QQuickParticleAffector* a = qobject_cast<QQuickParticleAffector*>(value);
    if (a) {
        a->setParentItem(sys);
        a->setGroups(list);
        a->setSystem(sys);
        return;
    }
    QQuickTrailEmitter* fe = qobject_cast<QQuickTrailEmitter*>(value);
    if (fe) {
        fe->setParentItem(sys);
        fe->setFollow(group->name());
        fe->setSystem(sys);
        return;
    }
    QQuickParticleEmitter* e = qobject_cast<QQuickParticleEmitter*>(value);
    if (e) {
        e->setParentItem(sys);
        e->setGroup(group->name());
        e->setSystem(sys);
        return;
    }
    QQuickParticlePainter* p = qobject_cast<QQuickParticlePainter*>(value);
    if (p) {
        p->setParentItem(sys);
        p->setGroups(list);
        p->setSystem(sys);
        return;
    }
    qWarning() << value << " was placed inside a particle system state but cannot be taken into the particle system. It will be lost.";
}


int QQuickParticleSystem::registerParticleGroupData(const QString &name, QQuickParticleGroupData *pgd)
{
    Q_ASSERT(!groupIds.contains(name));
    int id;
    if (nextFreeGroupId >= groupData.size()) {
        groupData.push_back(pgd);
        nextFreeGroupId = groupData.size();
        id = nextFreeGroupId - 1;
    } else {
        id = nextFreeGroupId;
        groupData[id] = pgd;
        searchNextFreeGroupId();
    }
    groupIds.insert(name, id);
    return id;
}

void QQuickParticleSystem::searchNextFreeGroupId()
{
    ++nextFreeGroupId;
    for (int ei = groupData.size(); nextFreeGroupId != ei; ++nextFreeGroupId) {
        if (groupData[nextFreeGroupId] == nullptr) {
            return;
        }
    }
}

void QQuickParticleSystem::componentComplete()

{
    QQuickItem::componentComplete();
    m_componentComplete = true;
    m_animation = new QQuickParticleSystemAnimation(this);
    reset();//restarts animation as well
}

void QQuickParticleSystem::reset()
{
    if (!m_componentComplete)
        return;

    timeInt = 0;
    //Clear guarded pointers which have been deleted
    m_emitters.removeAll(nullptr);
    m_painters.removeAll(nullptr);
    m_affectors.removeAll(nullptr);

    bySysIdx.resize(0);
    initGroups();//Also clears all logical particles

    if (!m_running)
        return;

    for (QQuickParticleEmitter *e : std::as_const(m_emitters))
        e->reset();

    emittersChanged();

    for (QQuickParticlePainter *p : std::as_const(m_painters)) {
        loadPainter(p);
        p->reset();
    }

    //### Do affectors need reset too?
    if (m_animation) {//Animation is explicitly disabled in benchmarks
        //reset restarts animation (if running)
        if ((m_animation->state() == QAbstractAnimation::Running))
            m_animation->stop();
        m_animation->start();
        if (m_paused)
            m_animation->pause();
    }

    initialized = true;
}


void QQuickParticleSystem::loadPainter(QQuickParticlePainter *painter)
{
    if (!m_componentComplete || !painter)
        return;

    for (QQuickParticleGroupData *sg : groupData) {
        sg->painters.removeOne(painter);
    }

    int particleCount = 0;
    if (painter->groups().isEmpty()) {//Uses default particle
        static QStringList def = QStringList() << QString();
        painter->setGroups(def);
        particleCount += groupData[0]->size();
        groupData[0]->painters << painter;
    } else {
        for (auto groupId : painter->groupIds()) {
            QQuickParticleGroupData *gd = groupData[groupId];
            particleCount += gd->size();
            gd->painters << painter;
        }
    }
    painter->setCount(particleCount);
    painter->update();//Initial update here
    return;
}

void QQuickParticleSystem::emittersChanged()
{
    if (!m_componentComplete)
        return;

    QVector<int> previousSizes;
    QVector<int> newSizes;
    previousSizes.reserve(groupData.size());
    newSizes.reserve(groupData.size());
    for (int i = 0, ei = groupData.size(); i != ei; ++i) {
        previousSizes << groupData[i]->size();
        newSizes << 0;
    }

    // Populate groups and set sizes.
    for (int i = 0; i < m_emitters.size(); ) {
        QQuickParticleEmitter *e = m_emitters.at(i);
        if (!e) {
            m_emitters.removeAt(i);
            continue;
        }

        int groupId = e->groupId();
        if (groupId == QQuickParticleGroupData::InvalidID) {
            groupId = (new QQuickParticleGroupData(e->group(), this))->index;
            previousSizes << 0;
            newSizes << 0;
        }
        newSizes[groupId] += e->particleCount();
        //###: Cull emptied groups?

        ++i;
    }

    //TODO: Garbage collection?
    particleCount = 0;
    for (int i = 0, ei = groupData.size(); i != ei; ++i) {
        groupData[i]->setSize(qMax(newSizes[i], previousSizes[i]));
        particleCount += groupData[i]->size();
    }

    postProcessEmitters();
}

void QQuickParticleSystem::postProcessEmitters()
{
    if (m_debugMode)
        qDebug() << "Particle system emitters changed. New particle count: " << particleCount << "in" << groupData.size() << "groups.";

    if (particleCount > bySysIdx.size())//New datum requests haven't updated it
        bySysIdx.resize(particleCount);

    for (QQuickParticleAffector *a : std::as_const(m_affectors)) {//Groups may have changed
        if (a) {
            a->m_updateIntSet = true;
        }
    }

    for (QQuickParticlePainter *p : std::as_const(m_painters))
        loadPainter(p);

    if (!m_groups.isEmpty())
        createEngine();

}

void QQuickParticleSystem::emitterAdded(QQuickParticleEmitter *e)
{
    if (!m_componentComplete)
        return;

    // Populate group and set size.
    const int groupId = e->groupId();
    if (groupId == QQuickParticleGroupData::InvalidID) {
        QQuickParticleGroupData *group = new QQuickParticleGroupData(e->group(), this);
        group->setSize(e->particleCount());
    } else {
        QQuickParticleGroupData *group = groupData[groupId];
        group->setSize(group->size() + e->particleCount());
    }

    // groupData can have changed independently, so we still have to iterate it all
    // to count the particles.
    particleCount = 0;
    for (int i = 0, ei = groupData.size(); i != ei; ++i)
        particleCount += groupData[i]->size();

    postProcessEmitters();
}

void QQuickParticleSystem::createEngine()
{
    if (!m_componentComplete)
        return;
    if (stateEngine && m_debugMode)
        qDebug() << "Resetting Existing Sprite Engine...";
    //### Solve the losses if size/states go down
    for (QQuickParticleGroup *group : std::as_const(m_groups)) {
        bool exists = false;
        for (auto it = groupIds.keyBegin(), end = groupIds.keyEnd(); it != end; ++it) {
            if (group->name() == *it) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            new QQuickParticleGroupData(group->name(), this);
        }
    }

    if (m_groups.size()) {
        //Reorder groups List so as to have the same order as groupData
        // TODO: can't we just merge the two lists?
        QList<QQuickParticleGroup*> newList;
        for (int i = 0, ei = groupData.size(); i != ei; ++i) {
            bool exists = false;
            QString name = groupData[i]->name();
            for (QQuickParticleGroup *existing : std::as_const(m_groups)) {
                if (existing->name() == name) {
                    newList << existing;
                    exists = true;
                }
            }
            if (!exists) {
                newList << new QQuickParticleGroup(this);
                newList.back()->setName(name);
            }
        }
        m_groups = newList;
        QList<QQuickStochasticState*> states;
        states.reserve(m_groups.size());
        for (QQuickParticleGroup *g : std::as_const(m_groups))
            states << (QQuickStochasticState*)g;

        if (!stateEngine)
            stateEngine = new QQuickStochasticEngine(this);
        stateEngine->setCount(particleCount);
        stateEngine->m_states = states;

        connect(stateEngine, &QQuickStochasticEngine::stateChanged,
                this, &QQuickParticleSystem::particleStateChange);

    } else {
        if (stateEngine)
            delete stateEngine;
        stateEngine = nullptr;
    }

}

void QQuickParticleSystem::particleStateChange(int idx)
{
    moveGroups(bySysIdx[idx], stateEngine->curState(idx));
}

void QQuickParticleSystem::moveGroups(QQuickParticleData *d, int newGIdx)
{
    if (!d || newGIdx == d->groupId)
        return;

    QQuickParticleData *pd = newDatum(newGIdx, false, d->systemIndex, d);
    if (!pd)
        return;

    finishNewDatum(pd);

    d->systemIndex = -1;
    groupData[d->groupId]->kill(d);
}

int QQuickParticleSystem::nextSystemIndex()
{
    if (!m_reusableIndexes.isEmpty()) {
        int ret = *(m_reusableIndexes.begin());
        m_reusableIndexes.remove(ret);
        return ret;
    }
    if (m_nextIndex >= bySysIdx.size()) {
        bySysIdx.resize(bySysIdx.size() < 10 ? 10 : bySysIdx.size()*1.1);//###+1,10%,+10? Choose something non-arbitrarily
        if (stateEngine)
            stateEngine->setCount(bySysIdx.size());

    }
    return m_nextIndex++;
}

QQuickParticleData *QQuickParticleSystem::newDatum(
        int groupId, bool respectLimits, int sysIndex,
        const QQuickParticleData *cloneFrom)
{
    Q_ASSERT(groupId < groupData.size());//XXX shouldn't really be an assert

    QQuickParticleData *ret = groupData[groupId]->newDatum(respectLimits);
    if (!ret)
        return nullptr;

    if (cloneFrom) {
        // We need to retain the "identity" information of the new particle data since it may be
        // "recycled" and still be tracked.
        const int retainedIndex = ret->index;
        const int retainedGroupId = ret->groupId;
        const int retainedSystemIndex = ret->systemIndex;
        *ret = *cloneFrom;
        ret->index = retainedIndex;
        ret->groupId = retainedGroupId;
        ret->systemIndex = retainedSystemIndex;
    }

    if (sysIndex == -1) {
        if (ret->systemIndex == -1)
            ret->systemIndex = nextSystemIndex();
    } else {
        if (ret->systemIndex != -1) {
            if (stateEngine)
                stateEngine->stop(ret->systemIndex);
            m_reusableIndexes << ret->systemIndex;
            bySysIdx[ret->systemIndex] = 0;
        }
        ret->systemIndex = sysIndex;
    }
    bySysIdx[ret->systemIndex] = ret;

    if (stateEngine)
        stateEngine->start(ret->systemIndex, ret->groupId);

    m_empty = false;
    return ret;
}

void QQuickParticleSystem::emitParticle(QQuickParticleData* pd, QQuickParticleEmitter* particleEmitter)
{// called from prepareNextFrame()->emitWindow - enforce?
    //Account for relative emitter position
    bool okay = false;
    QTransform t = particleEmitter->itemTransform(this, &okay);
    if (okay) {
        qreal tx,ty;
        t.map(pd->x, pd->y, &tx, &ty);
        pd->x = tx;
        pd->y = ty;
    }

    finishNewDatum(pd);
}

void QQuickParticleSystem::finishNewDatum(QQuickParticleData *pd)
{
    Q_ASSERT(pd);
    groupData[pd->groupId]->prepareRecycler(pd);

    for (QQuickParticleAffector *a : std::as_const(m_affectors))
        if (a && a->m_needsReset)
            a->reset(pd);
    for (QQuickParticlePainter *p : std::as_const(groupData[pd->groupId]->painters))
        if (p)
            p->load(pd);
}

void QQuickParticleSystem::updateCurrentTime( int currentTime )
{
    if (!initialized)
        return;//error in initialization

    //### Elapsed time never shrinks - may cause problems if left emitting for weeks at a time.
    qreal dt = timeInt / 1000.;
    timeInt = currentTime;
    qreal time =  timeInt / 1000.;
    dt = time - dt;
    needsReset.clear();

    m_emitters.removeAll(nullptr);
    m_painters.removeAll(nullptr);
    m_affectors.removeAll(nullptr);

    bool oldClear = m_empty;
    m_empty = true;
    for (QQuickParticleGroupData *gd : std::as_const(groupData))//Recycle all groups and see if they're out of live particles
        m_empty = gd->recycle() && m_empty;

    if (stateEngine)
        stateEngine->updateSprites(timeInt);

    for (QQuickParticleEmitter *emitter : std::as_const(m_emitters))
        emitter->emitWindow(timeInt);
    for (QQuickParticleAffector *a : std::as_const(m_affectors))
        a->affectSystem(dt);
    for (QQuickParticleData *d : needsReset)
        for (QQuickParticlePainter *p : std::as_const(groupData[d->groupId]->painters))
            p->reload(d);

    if (oldClear != m_empty)
        emptyChanged(m_empty);
}

int QQuickParticleSystem::systemSync(QQuickParticlePainter* p)
{
    if (!m_running)
        return 0;
    if (!initialized)
        return 0;//error in initialization
    p->performPendingCommits();
    return timeInt;
}


QT_END_NAMESPACE

#include "moc_qquickparticlesystem_p.cpp"
