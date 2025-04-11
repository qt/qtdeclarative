// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include "qquickparticlegroup_p.h"

/*!
    \qmltype ParticleGroup
    \inqmlmodule QtQuick.Particles
    \brief For setting attributes on a logical particle group.
    \ingroup qtquick-particles

    This element allows you to set timed transitions on particle groups.

    You can also use this element to group particle system elements related to the logical
    particle group. Emitters, Affectors and Painters set as direct children of a ParticleGroup
    will automatically apply to that logical particle group. TrailEmitters will automatically follow
    the group.

    If a ParticleGroup element is not defined for a group, the group will function normally as if
    none of the transition properties were set.
*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles::ParticleGroup::system
    This is the system which will contain the group.

    If the ParticleGroup is a direct child of a ParticleSystem, it will automatically be associated with it.
*/
/*!
    \qmlproperty string QtQuick.Particles::ParticleGroup::name
    This is the name of the particle group, and how it is generally referred to by other elements.

    If elements refer to a name which does not have an explicit ParticleGroup created, it will
    work normally (with no transitions specified for the group). If you do not need to assign
    duration based transitions to a group, you do not need to create a ParticleGroup with that name (although you may).
*/
/*!
    \qmlproperty int QtQuick.Particles::ParticleGroup::duration
    The time in milliseconds before the group will attempt to transition.

*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles::ParticleGroup::durationVariation
    The maximum number of milliseconds that the duration of the transition cycle varies per particle in the group.

    Default value is zero.
*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles::ParticleGroup::to
    The weighted list of transitions valid for this group.

    If the chosen transition stays in this group, another duration (+/- up to durationVariation)
    milliseconds will occur before another transition is attempted.
*/

QQuickParticleGroup::QQuickParticleGroup(QObject* parent)
    : QQuickStochasticState(parent)
    , m_system(nullptr)
{

}

void delayedRedirect(QQmlListProperty<QObject> *prop, QObject *value)
{
    QQuickParticleGroup* pg = qobject_cast<QQuickParticleGroup*>(prop->object);
    if (pg)
        pg->delayRedirect(value);
}

QQmlListProperty<QObject> QQuickParticleGroup::particleChildren()
{
    QQuickParticleSystem* system = qobject_cast<QQuickParticleSystem*>(parent());
    if (system) {
        return QQmlListProperty<QObject>(this, nullptr,
                                         &QQuickParticleSystem::statePropertyRedirect, nullptr,
                                         nullptr, nullptr, nullptr, nullptr);
    } else {
        return QQmlListProperty<QObject>(this, nullptr,
                                         &delayedRedirect, nullptr, nullptr,
                                         nullptr, nullptr, nullptr);
    }
}

void QQuickParticleGroup::setSystem(QQuickParticleSystem* arg)
{
    if (m_system != arg) {
        m_system = arg;
        m_system->registerParticleGroup(this);
        performDelayedRedirects();
        emit systemChanged(arg);
    }
}

void QQuickParticleGroup::delayRedirect(QObject *obj)
{
    m_delayedRedirects << obj;
}

void QQuickParticleGroup::performDelayedRedirects()
{
    if (!m_system)
        return;
    foreach (QObject* obj, m_delayedRedirects)
        m_system->stateRedirect(this, m_system, obj);

    m_delayedRedirects.clear();
}

void QQuickParticleGroup::componentComplete(){
    if (!m_system && qobject_cast<QQuickParticleSystem*>(parent()))
        setSystem(qobject_cast<QQuickParticleSystem*>(parent()));
}

#include "moc_qquickparticlegroup_p.cpp"
