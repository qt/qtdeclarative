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

#include "qsgage_p.h"
#include "qsgparticleemitter_p.h"
QT_BEGIN_NAMESPACE
/*!
    \qmlclass Age QSGAgeAffector
    \inqmlmodule QtQuick.Particles 2
    \inherits Affector
    \brief The Age affector allows you to prematurely age particles

    The Age affector allows you to alter where the particle is in its lifecycle. Common uses
    are to expire particles prematurely, possibly giving them time to animate out.

    The Age affector only applies to particles which are still alive.
*/
/*!
    \qmlproperty int QtQuick.Particles2::Age::lifeLeft

    The amount of life to set the particle to have. Affected particles
    will advance to a point in their life where they will have this many
    milliseconds left to live.
*/

/*!
    \qmlproperty bool QtQuick.Particles2::Age::advancePosition

    advancePosition determines whether position, veclocity and acceleration are included in
    the simulated aging done by the affector. If advancePosition is false,
    then the position, velocity and acceleration will remain the same and only
    other attributes (such as opacity) will advance in the simulation to where
    it would normally be for that point in the particle's life. With advancePosition set to
    true the position, velocity and acceleration will also advance to where it would
    normally be by that point in the particle's life, making it advance its position
    on screen.

    Default value is true.
*/

QSGAgeAffector::QSGAgeAffector(QSGItem *parent) :
    QSGParticleAffector(parent), m_lifeLeft(0), m_advancePosition(true)
{
}


bool QSGAgeAffector::affectParticle(QSGParticleData *d, qreal dt)
{
    Q_UNUSED(dt);
    if (d->stillAlive()){
        qreal curT = (qreal)m_system->timeInt/1000.0;
        qreal ttl = (qreal)m_lifeLeft/1000.0;
        if (!m_advancePosition && ttl > 0){
            qreal x = d->curX();
            qreal vx = d->curVX();
            qreal ax = d->curAX();
            qreal y = d->curY();
            qreal vy = d->curVY();
            qreal ay = d->curAY();
            d->t = curT - (d->lifeSpan - ttl);
            d->setInstantaneousX(x);
            d->setInstantaneousVX(vx);
            d->setInstantaneousAX(ax);
            d->setInstantaneousY(y);
            d->setInstantaneousVY(vy);
            d->setInstantaneousAY(ay);
        } else {
            d->t = curT - (d->lifeSpan - ttl);
        }
        return true;
    }
    return false;
}
QT_END_NAMESPACE
