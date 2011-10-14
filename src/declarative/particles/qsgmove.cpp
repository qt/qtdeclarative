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

#include "qsgmove_p.h"
#include <cmath>
QT_BEGIN_NAMESPACE
const qreal CONV = 0.017453292520444443;
/*!
    \qmlclass Move QSGMoveAffector
    \inqmlmodule QtQuick.Particles 2
    \inherits Affector
    \brief The Move element allows you to set a new position, speed or acceleration on particles

    You'll often want to set the 'once' property to true for efficiency in the case where you just
    want to set the parameter. Otherwise, the parameter will be needlessly set to the same thing
    every simulation cycle.
*/

/*!
    \qmlproperty StochasticDirection QtQuick.Particles2::Move::position

    Affected particles will have their position set to this direction,
    relative to the ParticleSystem. When interpreting directions as points,
    imagine it as an arrow with the base at the 0,0 of the ParticleSystem and the
    tip at where the specified position will be.
*/

/*!
    \qmlproperty StochasticDirection QtQuick.Particles2::Move::speed

    Affected particles will have their speed set to this direction.
*/


/*!
    \qmlproperty StochasticDirection QtQuick.Particles2::Move::acceleration

    Affected particles will have their acceleration set to this direction.
*/


/*!
    \qmlproperty bool QtQuick.Particles2::Move::relative

    Whether the affected particles have their existing position/speed/acceleration added
    to the new one.
*/

QSGMoveAffector::QSGMoveAffector(QQuickItem *parent)
    : QSGParticleAffector(parent)
    , m_position(&m_nullVector)
    , m_speed(&m_nullVector)
    , m_acceleration(&m_nullVector)
    , m_relative(false)
{
}

bool QSGMoveAffector::affectParticle(QSGParticleData *d, qreal dt)
{
    Q_UNUSED(dt);
    bool changed = false;
    QPointF curPos(d->curX(), d->curY());

    if (m_position != &m_nullVector){
        QPointF pos = m_position->sample(curPos);
        if (m_relative)
            pos += curPos;
        d->setInstantaneousX(pos.x());
        d->setInstantaneousY(pos.y());
        changed = true;
    }

    if (m_speed != &m_nullVector){
        QPointF pos = m_speed->sample(curPos);
        if (m_relative)
            pos += QPointF(d->curVX(), d->curVY());
        d->setInstantaneousVX(pos.x());
        d->setInstantaneousVY(pos.y());
        changed = true;
    }

    if (m_acceleration != &m_nullVector){
        QPointF pos = m_acceleration->sample(curPos);
        if (m_relative)
            pos += QPointF(d->curAX(), d->curAY());
        d->setInstantaneousAX(pos.x());
        d->setInstantaneousAY(pos.y());
        changed = true;
    }

    return changed;
}
QT_END_NAMESPACE
