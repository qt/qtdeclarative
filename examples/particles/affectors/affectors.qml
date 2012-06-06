/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import "../../shared" as Examples

/*!
    \title QtQuick.Particles Examples - Affectors
    \example particles/affectors
    \brief This is a collection of examples using Affectors in the QML particle system.
    \image qml-affectors-example.png

    This is a collection of small QML examples relating to using Affectors in the particle system.
    Each example is a small QML file emphasizing a particular element or feature.

    Age demonstrates using an Age affector to prematurely end the lives of particles.
    \snippet examples/particles/affectors/content/age.qml 0

    As you move the affector around the screen, the particles inside it
    (which haven't already been affected) jump to a period near the end
    of their life. This gives them a short period to finish fading out,
    but changing lifeLeft to 0 (the default), would cause them to reach
    the end of their life instantly.

    Attractor demonstrates using an Attractor affector to simulate a black hole
    \snippet examples/particles/affectors/content/attractor.qml 0

    All particles in the scene, including the rocket ship's exhaust and pellets, are pulled
    towards the black hole. This effect is stronger closer to the black hole, so the
    asteroids near the top of the screen are barely affected at all, while the ones
    towards the middle sometimes curve drastically. To complete the effect, an Age
    affector covers the black hole to destroy particles which come in contact with it.

    Custom Affector manipulates the properties of the particles directly in javascript.
    One Affector is used to make the leaves rock back and forth as they fall, looking more
    leaf-like than just spinning in circles:
    \snippet examples/particles/affectors/content/customaffector.qml 0
    Another is used to provide a slightly varying friction to the leaves as they 'land',
    to look more natural:
    \snippet examples/particles/affectors/content/customaffector.qml 1

    Friction is similar to the falling leaves in the custom affector, except that it uses a
    flat friction the whole way down instead of custom affectors.
    \snippet examples/particles/affectors/content/friction.qml 0

    Gravity is a convenience affector for applying a constant acceleration to particles inside it
    \snippet examples/particles/affectors/content/gravity.qml 0

    GroupGoal sets up two particle groups for flaming and non-flaming balls, and gives you various
    ways to transition between them.
    \snippet examples/particles/affectors/content/groupgoal.qml unlit
    The non-flaming balls have a one in a hundred chance of lighting on their own each second, but they also
    have a GroupGoal set on the whole group. This affector affects all particles of the unlit group, when colliding
    with particles in the lit group, and cause them to move to the lighting group.
    \snippet examples/particles/affectors/content/groupgoal.qml lighting
    lighting is an intermediate group so that the glow builds up and the transition is less jarring. So it automatically
    moves into the lit group after 100ms.
    \snippet examples/particles/affectors/content/groupgoal.qml lit
    The lit group also has TrailEmitters on it for additional fire and smoke, but does not transition anywhere.
    There are two more GroupGoal elements that allow particles in the unlit group to transition to the lighting group
    (and then to the lit group).
    \snippet examples/particles/affectors/content/groupgoal.qml groupgoal-pilot
    The first is just an area bound to the location of an image of a pilot flame. When unlit balls pass through the flame,
    they go straight to lit because the pilot flame is so hot.
    \snippet examples/particles/affectors/content/groupgoal.qml groupgoal-ma
    The second is bound to the location of the last pointer interaction, so that touching or clicking on unlit balls (which
    is hard due to their constant movement) causes them to move to the lighting group.

    Move shows some simple effects you can get by altering trajectory midway.
    The red particles have an affector that affects their position, jumping them forwards by 120px.
    \snippet examples/particles/affectors/content/move.qml A
    The green particles have an affector that affects their velocity, but with some angle variation. By adding some random direction
    velocity to their existing forwards velocity, they begin to spray off in a cone.
    \snippet examples/particles/affectors/content/move.qml B
    The blue particles have an affector that affects their acceleration, and because it sets relative to false this resets the acceleration instead of
    adding to it. Once the blue particles reach the affector, their horizontal velocity stops increasing as their vertical velocity decreases.
    \snippet examples/particles/affectors/content/move.qml C

    SpriteGoal has an affector which interacts with the sprite engine of particles, if they are being drawn as sprites by ImageParticle.
    \snippet examples/particles/affectors/content/spritegoal.qml 0
    The SpriteGoal follows the image of the rocket ship on screen, and when it interacts with particles drawn by ImageParticle as sprites,
    it instructs them to move immediately to the "explode" state, which in this case is the animation of the asteroid breaking into many pieces.

    Turbulence has a flame with smoke, and both sets of particles being affected by a Turbulence affector. This gives a faint wind effect.
    \snippet examples/particles/affectors/content/turbulence.qml 0
    To make the wind change direction, subsitute a black and white noise image in the noiseSource parameter (it currently uses a default noise source).

    Wander uses a Wander affector to add some horizontal drift to snowflakes as they fall down.
    \snippet examples/particles/affectors/content/wander.qml 0
    There are different movements given by applying the Wander to different attributes of the trajectory, so the example makes it easy to play around and see the difference.
*/

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Age", "Kills off particles that enter the affector",  Qt.resolvedUrl("content/age.qml"));
            addExample("Attractor", "Simulates a small black hole", Qt.resolvedUrl("content/attractor.qml"));
            addExample("Custom Affector", "Custom falling leaves", Qt.resolvedUrl("content/customaffector.qml"));
            addExample("Friction", "Leaves that slow down as they fall", Qt.resolvedUrl("content/friction.qml"));
            addExample("Gravity", "Leaves that fall towards the earth as you move it", Qt.resolvedUrl("content/gravity.qml"));
            addExample("GroupGoal", "Balls that can be set on fire various ways", Qt.resolvedUrl("content/groupgoal.qml"));
            addExample("Move", "Some effects you can get by altering trajectory midway", Qt.resolvedUrl("content/move.qml"));
            addExample("SpriteGoal", "A ship that makes asteroids explode", Qt.resolvedUrl("content/spritegoal.qml"));
            addExample("Turbulence", "A candle with faint wind", Qt.resolvedUrl("content/turbulence.qml"));
            addExample("Wander", "Drifting snow flakes", Qt.resolvedUrl("content/wander.qml"));
        }
    }
}
