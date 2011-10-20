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

#include "qsggravity_p.h"
#include <cmath>
QT_BEGIN_NAMESPACE
const qreal CONV = 0.017453292520444443;
/*!
    \qmlclass Gravity QSGGravityAffector
    \inqmlmodule QtQuick.Particles 2
    \inherits Affector
    \brief The Gravity element allows you to set a constant accleration in an angle

    This element will set the acceleration of all affected particles to a vector of
    the specified magnitude in the specified angle. If the angle or acceleration is
    not varying, it is more efficient to set the specified acceleration on the Emitter.

    This element models the gravity of a massive object whose center of
    gravity is far away (and thus the gravitational pull is effectively constant
    across the scene). To model the gravity of an object near or inside the scene,
    use PointAttractor.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Gravity::acceleration

    Pixels per second that objects will be accelerated by.
*/
/*!
    \qmlproperty real QtQuick.Particles2::Gravity::angle

    Angle of acceleration.
*/

QSGGravityAffector::QSGGravityAffector(QQuickItem *parent) :
    QSGParticleAffector(parent), m_acceleration(-10), m_angle(90), m_xAcc(0), m_yAcc(0)
{
    connect(this, SIGNAL(accelerationChanged(qreal)),
            this, SLOT(recalc()));
    connect(this, SIGNAL(angleChanged(qreal)),
            this, SLOT(recalc()));
    recalc();
}

void QSGGravityAffector::recalc()
{
    qreal theta = m_angle * CONV;
    m_xAcc = m_acceleration * cos(theta);
    m_yAcc = m_acceleration * sin(theta);
}

bool QSGGravityAffector::affectParticle(QSGParticleData *d, qreal dt)
{
    Q_UNUSED(dt);
    bool changed = false;
    if (d->ax != m_xAcc){
        d->setInstantaneousAX(m_xAcc);
        changed = true;
    }
    if (d->ay != m_yAcc){
        d->setInstantaneousAY(m_yAcc);
        changed = true;
    }
    return changed;
}
QT_END_NAMESPACE
