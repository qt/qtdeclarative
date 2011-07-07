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
QSGParticleAffector::QSGParticleAffector(QSGItem *parent) :
    QSGItem(parent), m_needsReset(false), m_system(0), m_active(true)
  , m_updateIntSet(false), m_shape(new QSGParticleExtruder(this)), m_signal(false)
{
    connect(this, SIGNAL(systemChanged(QSGParticleSystem*)),
            this, SLOT(updateOffsets()));
    connect(this, SIGNAL(xChanged()),
            this, SLOT(updateOffsets()));
    connect(this, SIGNAL(yChanged()),
            this, SLOT(updateOffsets()));//TODO: in componentComplete and all relevant signals
}

void QSGParticleAffector::componentComplete()
{
    if (!m_system && qobject_cast<QSGParticleSystem*>(parentItem()))
        setSystem(qobject_cast<QSGParticleSystem*>(parentItem()));
    if (!m_system)
        qWarning() << "Affector created without a particle system specified";//TODO: useful QML warnings, like line number?
    QSGItem::componentComplete();
}

void QSGParticleAffector::affectSystem(qreal dt)
{
    if (!m_active)
        return;
    //If not reimplemented, calls affect particle per particle
    //But only on particles in targeted system/area
    if (m_updateIntSet){
        m_groups.clear();
        foreach (const QString &p, m_particles)
            m_groups << m_system->m_groupIds[p];//###Can this occur before group ids are properly assigned?
        m_updateIntSet = false;
    }
    foreach (QSGParticleGroupData* gd, m_system->m_groupData){
        foreach (QSGParticleData* d, gd->data){
            if (!d)
                continue;
            if (m_groups.isEmpty() || m_groups.contains(d->group)){
                if ((m_onceOff && m_onceOffed.contains(qMakePair(d->group, d->index)))
                        || !d->stillAlive())
                    continue;
                //Need to have previous location for affected. if signal || shape might be faster?
                QPointF curPos = QPointF(d->curX(), d->curY());
                if (width() == 0 || height() == 0
                        || m_shape->contains(QRectF(m_offset.x(), m_offset.y(), width(), height()),curPos)){
                    if (m_collisionParticles.isEmpty() || isColliding(d)){
                        if (affectParticle(d, dt)){
                            m_system->m_needsReset << d;
                            if (m_onceOff)
                                m_onceOffed << qMakePair(d->group, d->index);
                            if (m_signal)
                                emit affected(curPos.x(), curPos.y());
                        }
                    }
                }
            }
        }
    }
}

bool QSGParticleAffector::affectParticle(QSGParticleData *d, qreal dt)
{
    Q_UNUSED(d);
    Q_UNUSED(dt);
    return m_signal;//If signalling, then we always 'null affect' it.
}

void QSGParticleAffector::reset(QSGParticleData* pd)
{//TODO: This, among other ones, should be restructured so they don't all need to remember to call the superclass
    if (m_onceOff)
        if (m_groups.isEmpty() || m_groups.contains(pd->group))
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
    foreach (const QString &group, m_collisionParticles){
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
