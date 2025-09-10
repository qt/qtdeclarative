// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/qqmlinfo.h>
#include <QtCore/QtMath>
#include "qquickgravity_p.h"
QT_BEGIN_NAMESPACE

/*!
    \qmltype Gravity
    \nativetype QQuickGravityAffector
    \inqmlmodule QtQuick.Particles
    \ingroup qtquick-particles
    \inherits ParticleAffector
    \brief For applying acceleration in an angle.

    This element will accelerate all affected particles to a vector of
    the specified magnitude in the specified angle. If the angle and acceleration do
    not vary, it is more efficient to set the specified acceleration on the Emitter.

    This element models the gravity of a massive object whose center of
    gravity is far away (and thus the gravitational pull is effectively constant
    across the scene). To model the gravity of an object near or inside the scene,
    use PointAttractor.
*/

/*!
    \qmlproperty real QtQuick.Particles::Gravity::magnitude

    Pixels per second that objects will be accelerated by.
*/
void QQuickGravityAffector::setMagnitude(qreal arg)
{
    if (m_magnitude != arg) {
        m_magnitude = arg;
        m_needRecalc = true;
        emit magnitudeChanged(arg);
    }
}

qreal QQuickGravityAffector::magnitude() const
{
    return m_magnitude;
}


/*!
    \qmlproperty real QtQuick.Particles::Gravity::acceleration
    \deprecated

    \warning The name for this property has changed to magnitude, use it instead.
*/
void QQuickGravityAffector::setAcceleration(qreal arg)
{
    qmlWarning(this) << "The acceleration property is deprecated. Please use magnitude instead.";
    setMagnitude(arg);
}

/*!
    \qmlproperty real QtQuick.Particles::Gravity::angle

    Angle of acceleration.
*/
void QQuickGravityAffector::setAngle(qreal arg)
{
    if (m_angle != arg) {
        m_angle = arg;
        m_needRecalc = true;
        emit angleChanged(arg);
    }
}

qreal QQuickGravityAffector::angle() const
{
    return m_angle;
}

QQuickGravityAffector::QQuickGravityAffector(QQuickItem *parent) :
    QQuickParticleAffector(parent), m_magnitude(-10), m_angle(90), m_needRecalc(true)
{
}

bool QQuickGravityAffector::affectParticle(QQuickParticleData *d, qreal dt)
{
    if (!m_magnitude)
        return false;
    if (m_needRecalc) {
        m_needRecalc = false;
        m_dx = m_magnitude * qCos(qDegreesToRadians(m_angle));
        m_dy = m_magnitude * qSin(qDegreesToRadians(m_angle));
    }

    d->setInstantaneousVX(d->curVX(m_system) + m_dx*dt, m_system);
    d->setInstantaneousVY(d->curVY(m_system) + m_dy*dt, m_system);
    return true;
}



QT_END_NAMESPACE

#include "moc_qquickgravity_p.cpp"
