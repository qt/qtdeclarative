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

#include "qsgparticleaffector_p.h"
#include <QDebug>
QT_BEGIN_NAMESPACE

/*!
    \qmlclass Affector QSGParticleAffector
    \inqmlmodule QtQuick.Particles 2
    \brief Affector elements can alter the attributes of logical particles at any point in their lifetime.

    The base Affector does not alter any attributes, but can be used to emit a signal
    when a particle meets certain conditions.

    If an affector has a defined size, then it will only affect particles within its size and position on screen.
*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles2::Affector::system
    This is the system which will be affected by the element.
    If the Affector is a direct child of a ParticleSystem, it will automatically be associated with it.
*/
/*!
    \qmlproperty list<string> QtQuick.Particles2::Affector::groups
    Which logical particle groups will be affected.

    If empty, it will affect all particles.
*/
/*!
    \qmlproperty list<string> QtQuick.Particles2::Affector::collisionParticles
    If any logical particle groups are specified here, then the affector
    will only be triggered if the particle being examined intersects with
    a particle of one of these groups.

    By default, no groups are specified.
*/
/*!
    \qmlproperty bool QtQuick.Particles2::Affector::enabled
    If enabled is set to false, this affector will not affect any particles.

    Usually this is used to conditionally turn an affector on or off.

    Default value is true.
*/
/*!
    \qmlproperty bool QtQuick.Particles2::Affector::once
    If once is set to true, this affector will only affect each particle
    once in their lifetimes.

    Default value is false.
*/
/*!
    \qmlproperty Shape QtQuick.Particles2::Affector::shape
    If a size has been defined, the shape property can be used to affect a
    non-rectangular area.
*/
/*!
    \qmlsignal QtQuick.Particles2::Affector::onAffected(x, y)

    This signal is emitted each time the affector actually affects a particle.

    x,y are the coordinates of the affected particle, relative to the ParticleSystem.

*/

/*!
    \qmlsignal QtQuick.Particles2::Affector::affectParticle(particle particle, real dt)

    This handler is called when particles are selected to be affected.

    dt is the time since the last time it was affected. Use dt to normalize
    trajectory manipulations to real time.

    Note that JS is slower to execute, so it is not recommended to use this in
    high-volume particle systems.
*/
/*!
    \qmlsignal QtQuick.Particles2::Affector::affected(real x, real y)

    This handler is called when a particle is selected to be affected. It will
    only be called if signal is set to true.

    x,y is the particles current position.
*/
QSGParticleAffector::QSGParticleAffector(QSGItem *parent) :
    QSGItem(parent), m_needsReset(false), m_system(0), m_enabled(true)
  , m_updateIntSet(false), m_shape(new QSGParticleExtruder(this))
{
}

bool QSGParticleAffector::isAffectedConnected()
{
    static int idx = QObjectPrivate::get(this)->signalIndex("affected(qreal,qreal)");
    return QObjectPrivate::get(this)->isSignalConnected(idx);
}


void QSGParticleAffector::componentComplete()
{
    if (!m_system && qobject_cast<QSGParticleSystem*>(parentItem()))
        setSystem(qobject_cast<QSGParticleSystem*>(parentItem()));
    QSGItem::componentComplete();
}

bool QSGParticleAffector::activeGroup(int g) {
    if (m_updateIntSet){
        m_groupIds.clear();
        foreach (const QString &p, m_groups)
            m_groupIds << m_system->m_groupIds[p];//###Can this occur before group ids are properly assigned?
        m_updateIntSet = false;
    }
    return m_groupIds.isEmpty() || m_groupIds.contains(g);
}

bool QSGParticleAffector::shouldAffect(QSGParticleData* d)
{
    if (!d)
        return false;
    if (activeGroup(d->group)){
        if ((m_onceOff && m_onceOffed.contains(qMakePair(d->group, d->index)))
                || !d->stillAlive())
            return false;
        //Need to have previous location for affected anyways
        if (width() == 0 || height() == 0
                || m_shape->contains(QRectF(m_offset.x(), m_offset.y(), width(), height()), QPointF(d->curX(), d->curY()))){
            if (m_whenCollidingWith.isEmpty() || isColliding(d)){
                return true;
            }
        }
    }
    return false;

}

void QSGParticleAffector::postAffect(QSGParticleData* d)
{
    m_system->m_needsReset << d;
    if (m_onceOff)
        m_onceOffed << qMakePair(d->group, d->index);
    if (isAffectedConnected())
        emit affected(d->curX(), d->curY());
}

void QSGParticleAffector::affectSystem(qreal dt)
{
    if (!m_enabled)
        return;
    //If not reimplemented, calls affect particle per particle
    //But only on particles in targeted system/area
    updateOffsets();//### Needed if an ancestor is transformed.
    foreach (QSGParticleGroupData* gd, m_system->m_groupData)
        if (activeGroup(m_system->m_groupData.key(gd)))
            foreach (QSGParticleData* d, gd->data)
                if (shouldAffect(d))
                    if (affectParticle(d, dt))
                        postAffect(d);
}

bool QSGParticleAffector::affectParticle(QSGParticleData *, qreal )
{
    return true;
}

void QSGParticleAffector::reset(QSGParticleData* pd)
{//TODO: This, among other ones, should be restructured so they don't all need to remember to call the superclass
    if (m_onceOff)
        if (m_groups.isEmpty() || m_groupIds.contains(pd->group))
            m_onceOffed.remove(qMakePair(pd->group, pd->index));
}

void QSGParticleAffector::updateOffsets()
{
    if (m_system)
        m_offset = m_system->mapFromItem(this, QPointF(0, 0));
}

bool QSGParticleAffector::isColliding(QSGParticleData *d)
{
    qreal myCurX = d->curX();
    qreal myCurY = d->curY();
    qreal myCurSize = d->curSize()/2;
    foreach (const QString &group, m_whenCollidingWith){
        foreach (QSGParticleData* other, m_system->m_groupData[m_system->m_groupIds[group]]->data){
            if (!other->stillAlive())
                continue;
            qreal otherCurX = other->curX();
            qreal otherCurY = other->curY();
            qreal otherCurSize = other->curSize()/2;
            if ((myCurX + myCurSize > otherCurX - otherCurSize
                 && myCurX - myCurSize < otherCurX + otherCurSize)
                 && (myCurY + myCurSize > otherCurY - otherCurSize
                     && myCurY - myCurSize < otherCurY + otherCurSize))
                return true;
        }
    }
    return false;
}

QT_END_NAMESPACE
