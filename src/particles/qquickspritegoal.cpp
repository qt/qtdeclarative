// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickspritegoal_p.h"
#include <private/qquickspriteengine_p.h>
#include <private/qquicksprite_p.h>
#include "qquickimageparticle_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SpriteGoal
    \instantiates QQuickSpriteGoalAffector
    \inqmlmodule QtQuick.Particles
    \ingroup qtquick-images-sprites
    \inherits Affector
    \brief For changing the state of a sprite particle.

*/
/*!
    \qmlproperty string QtQuick.Particles::SpriteGoal::goalState

    The name of the Sprite which the affected particles should move to.

    Sprite states have defined durations and transitions between them, setting goalState
    will cause it to disregard any path weightings (including 0) and head down the path
    which will reach the goalState quickest. It will pass through intermediate states
    on that path.
*/
/*!
    \qmlproperty bool QtQuick.Particles::SpriteGoal::jump

    If true, affected sprites will jump directly to the goal state instead of taking the
    shortest valid path to get there. They will also not finish their current state,
    but immediately move to the beginning of the goal state.

    Default is false.
*/
/*!
    \qmlproperty bool QtQuick.Particles::SpriteGoal::systemStates

    deprecated, use GroupGoal instead
*/

QQuickSpriteGoalAffector::QQuickSpriteGoalAffector(QQuickItem *parent) :
    QQuickParticleAffector(parent),
    m_goalIdx(-1),
    m_lastEngine(nullptr),
    m_jump(false),
    m_systemStates(false),
    m_notUsingEngine(false)
{
    m_ignoresTime = true;
}

void QQuickSpriteGoalAffector::updateStateIndex(QQuickStochasticEngine* e)
{
    if (m_systemStates){
        m_goalIdx = m_system->groupIds[m_goalState];
    }else{
        m_lastEngine = e;
        for (int i=0; i<e->stateCount(); i++){
            if (e->state(i)->name() == m_goalState){
                m_goalIdx = i;
                return;
            }
        }
        m_goalIdx = -1;//Can't find it
    }
}

void QQuickSpriteGoalAffector::setGoalState(const QString &arg)
{
    if (m_goalState != arg) {
        m_goalState = arg;
        emit goalStateChanged(arg);
        if (m_goalState.isEmpty())
            m_goalIdx = -1;
        else
            m_goalIdx = -2;
    }
}

bool QQuickSpriteGoalAffector::affectParticle(QQuickParticleData *d, qreal dt)
{
    Q_UNUSED(dt);
    QQuickStochasticEngine *engine = nullptr;
    if (!m_systemStates){
        //TODO: Affect all engines
        for (QQuickParticlePainter *p : m_system->groupData[d->groupId]->painters) {
            if (qobject_cast<QQuickImageParticle*>(p))
                engine = qobject_cast<QQuickImageParticle*>(p)->spriteEngine();
        }
    } else {
        engine = m_system->stateEngine;
        if (!engine)
            m_notUsingEngine = true;
    }
    if (!engine && !m_notUsingEngine)
        return false;

    if (m_goalIdx == -2 || engine != m_lastEngine)
        updateStateIndex(engine);
    int index = d->index;
    if (m_systemStates)
        index = d->systemIndex;
    if (m_notUsingEngine){//systemStates && no stochastic states defined. So cut out the engine
        //TODO: It's possible to move to a group that is intermediate and not used by painters or emitters - but right now that will redirect to the default group
        m_system->moveGroups(d, m_goalIdx);
    }else if (engine->curState(index) != m_goalIdx){
        engine->setGoal(m_goalIdx, index, m_jump);
        return true; //Doesn't affect particle data, but necessary for onceOff
    }
    return false;
}

QT_END_NAMESPACE

#include "moc_qquickspritegoal_p.cpp"
