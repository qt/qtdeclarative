/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qquickgroupgoal_p.h"
#include <private/qquickspriteengine_p.h>
#include <private/qquicksprite_p.h>
#include "qquickimageparticle_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmltype GroupGoal
    \instantiates QQuickGroupGoalAffector
    \inqmlmodule QtQuick.Particles
    \ingroup qtquick-particles
    \inherits Affector
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
    bool notUsingEngine = false;
    if (!engine)
        notUsingEngine = true;

    int index = d->systemIndex;
    int goalIdx = m_system->groupIds[m_goalState];
    if (notUsingEngine){//no stochastic states defined. So cut out the engine
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
