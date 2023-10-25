// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <math.h>
#include "qquickv4particledata_p.h"
#include <QDebug>
#include <private/qv4engine_p.h>
#include <private/qv4functionobject_p.h>
#include <QtCore/private/qnumeric_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Particle
    \inqmlmodule QtQuick.Particles
    \brief Represents particles manipulated by emitters and affectors.
    \ingroup qtquick-particles

    Particle elements are always managed internally by the ParticleSystem and cannot be created in QML.
    However, sometimes they are exposed via signals so as to allow arbitrary changes to the particle state
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::initialX
    The x coordinate of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::initialVX
    The x velocity of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::initialAX
    The x acceleration of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::initialY
    The y coordinate of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::initialVY
    The y velocity of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::initialAY
    The y acceleration of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::x
    The current x coordinate of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::vx
    The current x velocity of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::ax
    The current x acceleration of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::y
    The current y coordinate of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::vy
    The current y velocity of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::ay
    The current y acceleration of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::t
    The time, in seconds since the beginning of the simulation, that the particle was born.
*/


/*!
    \qmlproperty real QtQuick.Particles::Particle::startSize
    The size in pixels that the particle image is at the start
    of its life.
*/


/*!
    \qmlproperty real QtQuick.Particles::Particle::endSize
    The size in pixels that the particle image is at the end
    of its life. If this value is less than 0, then it is
    disregarded and the particle will have its startSize for the
    entire lifetime.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::lifeSpan
    The time in seconds that the particle will live for.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::rotation
    Degrees clockwise that the particle image is rotated at
    the beginning of its life.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::rotationVelocity
    Degrees clockwise per second that the particle image is rotated at while alive.
*/
/*!
    \qmlproperty bool QtQuick.Particles::Particle::autoRotate
    If autoRotate is true, then the particle's rotation will be
    set so that it faces the direction of travel, plus any
    rotation from the rotation or rotationVelocity properties.
*/

/*!
    \qmlproperty bool QtQuick.Particles::Particle::update

    Inside an Affector, the changes made to the particle will only be
    applied if update is set to true.
*/
/*!
    \qmlproperty real QtQuick.Particles::Particle::xDeformationVectorX

    The x component of the deformation vector along the X axis. ImageParticle
    can draw particles across non-square shapes. It will draw the texture rectangle
    across the parallelogram drawn with the x and y deformation vectors.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::yDeformationVectorX

    The y component of the deformation vector along the X axis. ImageParticle
    can draw particles across non-square shapes. It will draw the texture rectangle
    across the parallelogram drawn with the x and y deformation vectors.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::xDeformationVectorY

    The x component of the deformation vector along the X axis. ImageParticle
    can draw particles across non-square shapes. It will draw the texture rectangle
    across the parallelogram drawn with the x and y deformation vectors.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::yDeformationVectorY

    The y component of the deformation vector along the Y axis. ImageParticle
    can draw particles across non-square shapes. It will draw the texture rectangle
    across the parallelogram drawn with the x and y deformation vectors.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::red

    ImageParticle can draw colorized particles. When it does so, red is used
    as the red channel of the color applied to the source image.

    Values are from 0.0 to 1.0.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::green

    ImageParticle can draw colorized particles. When it does so, green is used
    as the green channel of the color applied to the source image.

    Values are from 0.0 to 1.0.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::blue

    ImageParticle can draw colorized particles. When it does so, blue is used
    as the blue channel of the color applied to the source image.

    Values are from 0.0 to 1.0.
*/

/*!
    \qmlproperty real QtQuick.Particles::Particle::alpha

    ImageParticle can draw colorized particles. When it does so, alpha is used
    as the alpha channel of the color applied to the source image.

    Values are from 0.0 to 1.0.
*/
/*!
    \qmlproperty real QtQuick.Particles::Particle::lifeLeft
    The time in seconds that the particle has left to live at
    the current point in time.
*/
/*!
    \qmlproperty real QtQuick.Particles::Particle::currentSize
    The currentSize of the particle, interpolating between startSize and endSize based on the currentTime.
*/

QT_END_NAMESPACE
