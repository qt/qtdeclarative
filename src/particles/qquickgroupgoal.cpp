// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickgroupgoal_p.h"
#include <private/qquickspriteengine_p.h>
#include <private/qquicksprite_p.h>
#include "qquickimageparticle_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmltype GroupGoal
    \nativetype QQuickGroupGoalAffector
    \inqmlmodule QtQuick.Particles
    \ingroup qtquick-particles
    \inherits ParticleAffector
    \brief For changing the state of a group of a particle.

*/
/*!
    \qmlproperty string QtQuick.Particles::GroupGoal::goalState

    The name of the group which the affected particles should move to.

    Groups can have defined durations and transitions between them, setting goalState
    will cause it to disregard any path weightings (including 0) and head down the path
    which will reach the goalState quickest. It will pass through intermediate groups
    on that path for their respective durations.
*/
/*!
    \qmlproperty bool QtQuick.Particles::GroupGoal::jump

    If true, affected particles will jump directly to the target group instead of taking the
    shortest valid path to get there. They will also not finish their current state,
    but immediately move to the beginning of the goal state.

    Default is false.
*/

QQuickGroupGoalAffector::QQuickGroupGoalAffector(QQuickItem *parent) :
    QQuickParticleAffector(parent), m_jump(false)
{
    m_ignoresTime = true;
}

void QQuickGroupGoalAffector::setGoalState(const QString &arg)
{
    if (m_goalState != arg) {
        m_goalState = arg;
        emit goalStateChanged(arg);
    }
}

bool QQuickGroupGoalAffector::affectParticle(QQuickParticleData *d, qreal dt)
{
    Q_UNUSED(dt);
    QQuickStochasticEngine *engine = m_system->stateEngine;
    int index = d->systemIndex;
    int goalIdx = m_system->groupIds[m_goalState];
    if (!engine){//no stochastic states defined. So cut out the engine
        //TODO: It's possible to move to a group that is intermediate and not used by painters or emitters - but right now that will redirect to the default group
        m_system->moveGroups(d, goalIdx);
        return true;
    }else if (engine->curState(index) != goalIdx){
        engine->setGoal(goalIdx, index, m_jump);
        return true;
    }
    return false;
}

QT_END_NAMESPACE

#include "moc_qquickgroupgoal_p.cpp"
