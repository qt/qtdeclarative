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

#include "qsgparticlegroup_p.h"

/*!
    \qmlclass ParticleGroup QSGParticleGroup
    \inqmlmodule QtQuick.Particles 2
    \brief ParticleGroup elements allow you to set attributes on a logical particle group.

    This element allows you to set timed transitions on particle groups.

    You can also use this element to group particle system elements related to the logical
    particle group. Emitters, Affectors and Painters set as direct children of a ParticleGroup
    will automatically apply to that logical particle group. TrailEmitters will automatically follow
    the group.

    If a ParticleGroup element is not defined for a group, the group will function normally as if
    none of the transition properties were set.
*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles2::ParticleGroup::system
    This is the system which will contain the group.

    If the ParticleGroup is a direct child of a ParticleSystem, it will automatically be associated with it.
*/
/*!
    \qmlproperty string QtQuick.Particles2::ParticleGroup::name
    This is the name of the particle group, and how it is generally referred to by other elements.

    If elements refer to a name which does not have an explicit ParticleGroup created, it will
    work normally (with no transitions specified for the group). If you do not need to assign
    duration based transitions to a group, you do not need to create a ParticleGroup with that name (although you may).
*/
/*!
    \qmlproperty int QtQuick.Particles2::ParticleGroup::duration
    The time in milliseconds before the group will attempt to transition.

*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles2::ParticleGroup::durationVariation
    The maximum number of milliseconds that the duration of the transition cycle varies per particle in the group.

    Default value is zero.
*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles2::ParticleGroup::to
    The weighted list of transitions valid for this group.

    If the chosen transition stays in this group, another duration (+/- up to durationVariation)
    milliseconds will occur before another transition is attempted.
*/

QSGParticleGroup::QSGParticleGroup(QObject* parent)
    : QSGStochasticState(parent)
    , m_system(0)
{

}

void delayedRedirect(QDeclarativeListProperty<QObject> *prop, QObject *value)
{
    QSGParticleGroup* pg = qobject_cast<QSGParticleGroup*>(prop->object);
    if (pg)
        pg->delayRedirect(value);
}

QDeclarativeListProperty<QObject> QSGParticleGroup::particleChildren()
{
    QSGParticleSystem* system = qobject_cast<QSGParticleSystem*>(parent());
    if (system)
        return QDeclarativeListProperty<QObject>(this, 0, &QSGParticleSystem::statePropertyRedirect);
    else
        return QDeclarativeListProperty<QObject>(this, 0, &delayedRedirect);
}

void QSGParticleGroup::setSystem(QSGParticleSystem* arg)
{
    if (m_system != arg) {
        m_system = arg;
        m_system->registerParticleGroup(this);
        performDelayedRedirects();
        emit systemChanged(arg);
    }
}

void QSGParticleGroup::delayRedirect(QObject *obj)
{
    m_delayedRedirects << obj;
}

void QSGParticleGroup::performDelayedRedirects()
{
    if (!m_system)
        return;
    foreach (QObject* obj, m_delayedRedirects)
        m_system->stateRedirect(this, m_system, obj);

    m_delayedRedirects.clear();
}

void QSGParticleGroup::componentComplete(){
    if (!m_system && qobject_cast<QSGParticleSystem*>(parent()))
        setSystem(qobject_cast<QSGParticleSystem*>(parent()));
}
