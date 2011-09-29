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

#include "qsgspritegoal_p.h"
#include "private/qsgspriteengine_p.h"
#include "private/qsgsprite_p.h"
#include "qsgimageparticle_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass SpriteGoal QSGSpriteGoalAffector
    \inqmlmodule QtQuick.Particles 2
    \inherits Affector
    \brief The SpriteGoal Affector allows you to change the state of a sprite or group of a particle.

*/
/*!
    \qmlproperty string QtQuick.Particles2::SpriteGoal::goalState
*/
/*!
    \qmlproperty bool QtQuick.Particles2::SpriteGoal::jump
*/
/*!
    \qmlproperty bool QtQuick.Particles2::SpriteGoal::systemStates
*/

QSGSpriteGoalAffector::QSGSpriteGoalAffector(QSGItem *parent) :
    QSGParticleAffector(parent), m_goalIdx(-1), m_jump(false), m_systemStates(false), m_lastEngine(0), m_notUsingEngine(false)
{
}

void QSGSpriteGoalAffector::updateStateIndex(QSGStochasticEngine* e)
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

void QSGSpriteGoalAffector::setGoalState(QString arg)
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

bool QSGSpriteGoalAffector::affectParticle(QSGParticleData *d, qreal dt)
{
    Q_UNUSED(dt);
    QSGStochasticEngine *engine = 0;
    if (!m_systemStates){
        //TODO: Affect all engines
        foreach (QSGParticlePainter *p, m_system->groupData[d->group]->painters)
            if (qobject_cast<QSGImageParticle*>(p))
                engine = qobject_cast<QSGImageParticle*>(p)->spriteEngine();
    }else{
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
        emit affected(QPointF(d->curX(), d->curY()));//###Expensive if unconnected? Move to Affector?
        return true; //Doesn't affect particle data, but necessary for onceOff
    }
    return false;
}

QT_END_NAMESPACE
