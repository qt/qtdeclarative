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

#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include <QSGItem>
#include <QElapsedTimer>
#include <QVector>
#include <QHash>
#include <QPointer>
#include <QSignalMapper>
#include <QtDeclarative/private/qsgsprite_p.h>
#include "private/qabstractanimation2_p.h"
#include <QtDeclarative/qdeclarative.h>
#include <private/qv8engine_p.h> //For QDeclarativeV8Handle

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGParticleSystem;
class QSGParticleAffector;
class QSGParticleEmitter;
class QSGParticlePainter;
class QSGParticleData;
class QSGParticleSystemAnimation;
class QSGStochasticEngine;
class QSGSprite;
class QSGV8ParticleData;
class QSGParticleGroup;
class QSGImageParticle;

struct QSGParticleDataHeapNode{
    int time;//in ms
    QSet<QSGParticleData*> data;//Set ptrs instead?
};

class QSGParticleDataHeap {
    //Idea is to do a binary heap, but which also stores a set of int,Node* so that if the int already exists, you can
    //add it to the data* list. Pops return the whole list at once.
public:
    QSGParticleDataHeap();
    void insert(QSGParticleData* data);
    void insertTimed(QSGParticleData* data, int time);

    int top();

    QSet<QSGParticleData*> pop();

    void clear();

    bool contains(QSGParticleData*);//O(n), for debugging purposes only
private:
    void grow();
    void swap(int, int);
    void bubbleUp(int);
    void bubbleDown(int);
    int m_size;
    int m_end;
    QSGParticleDataHeapNode m_tmp;
    QVector<QSGParticleDataHeapNode> m_data;
    QHash<int,int> m_lookups;
};

class QSGParticleGroupData{
public:
    QSGParticleGroupData(int id, QSGParticleSystem* sys);
    ~QSGParticleGroupData();

    int size();
    QString name();

    void setSize(int newSize);

    int index;
    QSet<QSGParticlePainter*> painters;

    //TODO: Refactor particle data list out into a separate class
    QVector<QSGParticleData*> data;
    QSGParticleDataHeap dataHeap;
    QSet<int> reusableIndexes;
    bool recycle(); //Force recycling round, reutrns true if all indexes are now reusable

    void initList();
    void kill(QSGParticleData* d);

    //After calling this, initialize, then call prepareRecycler(d)
    QSGParticleData* newDatum(bool respectsLimits);

    //TODO: Find and clean up those that don't get added to the recycler (currently they get lost)
    void prepareRecycler(QSGParticleData* d);

private:
    int m_size;
    QSGParticleSystem* m_system;
};

struct Color4ub {
    uchar r;
    uchar g;
    uchar b;
    uchar a;
};

class QSGParticleData{
public:
    //TODO: QObject like memory management (without the cost, just attached to system)
    QSGParticleData(QSGParticleSystem* sys);

    //Convenience functions for working backwards, because parameters are from the start of particle life
    //If setting multiple parameters at once, doing the conversion yourself will be faster.

    //sets the x accleration without affecting the instantaneous x velocity or position
    void setInstantaneousAX(qreal ax);
    //sets the x velocity without affecting the instantaneous x postion
    void setInstantaneousVX(qreal vx);
    //sets the instantaneous x postion
    void setInstantaneousX(qreal x);
    //sets the y accleration without affecting the instantaneous y velocity or position
    void setInstantaneousAY(qreal ay);
    //sets the y velocity without affecting the instantaneous y postion
    void setInstantaneousVY(qreal vy);
    //sets the instantaneous Y postion
    void setInstantaneousY(qreal y);

    //TODO: Slight caching?
    qreal curX() const;
    qreal curVX() const;
    qreal curAX() const { return ax; }
    qreal curY() const;
    qreal curVY() const;
    qreal curAY() const { return ay; }

    int group;
    QSGParticleEmitter* e;//### Needed?
    QSGParticleSystem* system;
    int index;
    int systemIndex;

    //General Position Stuff
    float x;
    float y;
    float t;
    float lifeSpan;
    float size;
    float endSize;
    float vx;
    float vy;
    float ax;
    float ay;

    //Other stuff, now universally shared
    Color4ub color;
    float xx;
    float xy;
    float yx;
    float yy;
    float rotation;
    float rotationSpeed;
    float autoRotate;//Assume that GPUs prefer floats to bools
    float animIdx;
    float frameDuration;
    float frameCount;
    float animT;
    float r;
    QSGItem* delegate;
    int modelIndex;
    float update;//Used by custom affectors

    //Used by image particle
    QSGImageParticle* colorOwner;
    QSGImageParticle* rotationOwner;
    QSGImageParticle* deformationOwner;
    QSGImageParticle* animationOwner;

    void debugDump();
    bool stillAlive();
    float lifeLeft();
    float curSize();
    void clone(const QSGParticleData& other);//Not =, leaves meta-data like index
    QDeclarativeV8Handle v8Value();
    void extendLife(float time);
private:
    QSGV8ParticleData* v8Datum;
};

class QSGParticleSystem : public QSGItem
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY emptyChanged)

public:
    explicit QSGParticleSystem(QSGItem *parent = 0);
    ~QSGParticleSystem();

    //TODO: Hook up running and temporal manipulators to the animation
    bool isRunning() const
    {
        return m_running;
    }

    int count(){ return m_particle_count; }

    static const int maxLife = 600000;

signals:

    void systemInitialized();
    void runningChanged(bool arg);
    void pausedChanged(bool arg);
    void emptyChanged(bool arg);

public slots:
    void start(){setRunning(true);}
    void stop(){setRunning(false);}
    void restart(){setRunning(false);setRunning(true);}
    void pause(){setPaused(true);}
    void resume(){setPaused(false);}

    void reset();
    void setRunning(bool arg);
    void setPaused(bool arg);

    virtual int duration() const { return -1; }


protected:
    //This one only once per frame (effectively)
    void componentComplete();

private slots:
    void emittersChanged();
    void loadPainter(QObject* p);
    void createEngine(); //### method invoked by sprite list changing (in engine.h) - pretty nasty
    void particleStateChange(int idx);

public://###but only really for related class usage. Perhaps we should all be friends?
    //These can be called multiple times per frame, performance critical
    void emitParticle(QSGParticleData* p);
    QSGParticleData* newDatum(int groupId, bool respectLimits = true, int sysIdx = -1);//TODO: implement respectLimits in emitters (which means interacting with maxCount?)
    void finishNewDatum(QSGParticleData*);
    void moveGroups(QSGParticleData *d, int newGIdx);
    int nextSystemIndex();

    //This one only once per painter per frame
    int systemSync(QSGParticlePainter* p);

    QSet<QSGParticleData*> m_needsReset;
    QVector<QSGParticleData*> m_bySysIdx; //Another reference to the data (data owned by group), but by sysIdx
    QHash<QString, int> m_groupIds;
    QHash<int, QSGParticleGroupData*> m_groupData;
    QSGStochasticEngine* m_stateEngine;

    int m_timeInt;
    bool m_initialized;

    void registerParticlePainter(QSGParticlePainter* p);
    void registerParticleEmitter(QSGParticleEmitter* e);
    void registerParticleAffector(QSGParticleAffector* a);
    void registerParticleGroup(QSGParticleGroup* g);

    int m_particle_count;
    static void statePropertyRedirect(QDeclarativeListProperty<QObject> *prop, QObject *value);
    static void stateRedirect(QSGParticleGroup* group, QSGParticleSystem* sys, QObject *value);
    bool isPaused() const
    {
        return m_paused;
    }

    bool isEmpty() const
    {
        return m_empty;
    }

private:
    void initializeSystem();
    void initGroups();
    bool m_running;
    QList<QPointer<QSGParticleEmitter> > m_emitters;
    QList<QPointer<QSGParticleAffector> > m_affectors;
    QList<QPointer<QSGParticlePainter> > m_painters;
    QList<QPointer<QSGParticlePainter> > m_syncList;
    QList<QSGParticleGroup*> m_groups;
    int m_nextGroupId;
    int m_nextIndex;
    QSet<int> m_reusableIndexes;
    bool m_componentComplete;

    QSignalMapper m_painterMapper;
    QSignalMapper m_emitterMapper;
    friend class QSGParticleSystemAnimation;
    void updateCurrentTime( int currentTime );
    QSGParticleSystemAnimation* m_animation;
    bool m_paused;
    bool m_debugMode;
    bool m_allDead;
    bool m_empty;
};

// Internally, this animation drives all the timing. Painters sync up in their updatePaintNode
class QSGParticleSystemAnimation : public QAbstractAnimation2
{
public:
    QSGParticleSystemAnimation(QSGParticleSystem* system)
        : QAbstractAnimation2()
        , m_system(system)
    { }

    QSGParticleSystemAnimation(const QSGParticleSystemAnimation &other)
        : QAbstractAnimation2(other)
        , m_system(other.m_system)
    { }
protected:
    virtual void updateCurrentTime( int t )
    {
        m_system->updateCurrentTime(t);
    }

    virtual int duration() const
    {
        return -1;
    }

private:
    QSGParticleSystem* m_system;
};


QT_END_NAMESPACE

QT_END_HEADER

#endif // PARTICLESYSTEM_H


