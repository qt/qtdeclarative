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

#include "qsgfriction_p.h"
QT_BEGIN_NAMESPACE
/*!
    \qmlclass Friction QSGFrictionAffector
    \inqmlmodule QtQuick.Particles 2
    \inherits Affector
    \brief The Friction affector slows down movement proportional to the particle's current speed.

*/

/*!
    \qmlproperty real QtQuick.Particles2::Friction::factor

    A drag will be applied to moving objects which is this factor of their current velocity.
*/
/*!
    \qmlproperty real QtQuick.Particles2::Friction::threshold

    The drag will only be applied to objects with a velocity above the threshold velocity. The
    drag applied will bring objects down to the threshold velocity, but no further.

    The default threshold is 0
*/
static qreal sign(qreal a)
{
    return a >= 0 ? 1 : -1;
}

static const qreal epsilon = 0.00001;

QSGFrictionAffector::QSGFrictionAffector(QQuickItem *parent) :
    QSGParticleAffector(parent), m_factor(0.0), m_threshold(0.0)
{
}

bool QSGFrictionAffector::affectParticle(QSGParticleData *d, qreal dt)
{
    if (!m_factor)
        return false;
    qreal curVX = d->curVX();
    qreal curVY = d->curVY();
    if (!curVX && !curVY)
        return false;
    qreal newVX = curVX + (curVX * m_factor * -1 * dt);
    qreal newVY = curVY + (curVY * m_factor * -1 * dt);

    if (!m_threshold) {
        if (sign(curVX) != sign(newVX))
            newVX = 0;
        if (sign(curVY) != sign(newVY))
            newVY = 0;
    } else {
        qreal curMag = sqrt(curVX*curVX + curVY*curVY);
        if (curMag <= m_threshold + epsilon)
            return false;
        qreal newMag = sqrt(newVX*newVX + newVY*newVY);
        if (newMag <= m_threshold + epsilon || //went past the threshold, stop there instead
            sign(curVX) != sign(newVX) || //went so far past maybe it came out the other side!
            sign(curVY) != sign(newVY)) {
            qreal theta = atan2(curVY, curVX);
            newVX = m_threshold * cos(theta);
            newVY = m_threshold * sin(theta);
        }
    }

    d->setInstantaneousVX(newVX);
    d->setInstantaneousVY(newVY);
    return true;
}
QT_END_NAMESPACE
