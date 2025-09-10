// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfriction_p.h"
#include <qmath.h>

QT_BEGIN_NAMESPACE
/*!
    \qmltype Friction
    \nativetype QQuickFrictionAffector
    \inqmlmodule QtQuick.Particles
    \ingroup qtquick-particles
    \inherits ParticleAffector
    \brief For applying friction proportional to the particle's current velocity.

*/

/*!
    \qmlproperty real QtQuick.Particles::Friction::factor

    A drag will be applied to moving objects which is this factor of their current velocity.
*/
/*!
    \qmlproperty real QtQuick.Particles::Friction::threshold

    The drag will only be applied to objects with a velocity above the threshold velocity. The
    drag applied will bring objects down to the threshold velocity, but no further.

    The default threshold is 0
*/
static qreal sign(qreal a)
{
    return a >= 0 ? 1 : -1;
}

static const qreal epsilon = 0.00001;

QQuickFrictionAffector::QQuickFrictionAffector(QQuickItem *parent) :
    QQuickParticleAffector(parent), m_factor(0.0), m_threshold(0.0)
{
}

bool QQuickFrictionAffector::affectParticle(QQuickParticleData *d, qreal dt)
{
    if (!m_factor)
        return false;
    qreal curVX = d->curVX(m_system);
    qreal curVY = d->curVY(m_system);
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
        qreal curMag = qSqrt(curVX*curVX + curVY*curVY);
        if (curMag <= m_threshold + epsilon)
            return false;
        qreal newMag = qSqrt(newVX*newVX + newVY*newVY);
        if (newMag <= m_threshold + epsilon || //went past the threshold, stop there instead
            sign(curVX) != sign(newVX) || //went so far past maybe it came out the other side!
            sign(curVY) != sign(newVY)) {
            qreal theta = qAtan2(curVY, curVX);
            newVX = m_threshold * qCos(theta);
            newVY = m_threshold * qSin(theta);
        }
    }

    d->setInstantaneousVX(newVX, m_system);
    d->setInstantaneousVY(newVY, m_system);
    return true;
}
QT_END_NAMESPACE

#include "moc_qquickfriction_p.cpp"
