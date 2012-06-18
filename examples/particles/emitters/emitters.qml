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
    \title QtQuick.Particles Examples - Emitters
    \example particles/emitters
    \brief This is a collection of examples using Emitters in the QML particle system.
    \image qml-emitters-example.png

    This is a collection of small QML examples relating to using Emitters in the particle system.
    Each example is a small QML file emphasizing a particular element or feature.

    Velocity from motion gives the effect of strong particle motion through primarily moving the emitters:
    \snippet examples/particles/emitters/content/velocityfrommotion.qml 0

    Burst and pulse calls the burst and pulse methods on two idential emitters.
    \snippet examples/particles/emitters/content/burstandpulse.qml 0
    Note how burst takes an argument of number of particles to emit, and pulse takes an argument of number of milliseconds to emit for.
    This gives a slightly different behaviour, which is easy to see in this example.

    Custom Emitter connects to the emitParticles signal to set arbitrary values on particle data as they're emitted;
    \snippet examples/particles/emitters/content/customemitter.qml 0
    This is used to emit curving particles in six rotating spokes.

    Emit mask sets an image mask on the Emitter, to emit out of an arbitrary shape.
    \snippet examples/particles/emitters/content/emitmask.qml 0

    Maximum emitted emits no more than a certain number of particles at a time. This example makes it easy to see what happens when the limit is reached.

    Shape and Direction emits particles out of an unfilled Ellipse shape, using a TargetDirection
    \snippet examples/particles/emitters/content/shapeanddirection.qml 0
    This sends the particles towards the center of the ellipse with proportional speed, keeping the ellipse outline as they move to the center.

    TrailEmitter uses that element to add smoke particles to trail the fire particles in the scene.
    \snippet examples/particles/emitters/content/trailemitter.qml 0

*/

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Velocity from Motion", "Particle motion just by moving emitters",  Qt.resolvedUrl("content/velocityfrommotion.qml"));
            addExample("Burst and Pulse", "Emit imperatively",  Qt.resolvedUrl("content/burstandpulse.qml"));
            addExample("Custom Emitter", "Custom starting state",  Qt.resolvedUrl("content/customemitter.qml"));
            addExample("Emit Mask", "Emit arbitrary shapes",  Qt.resolvedUrl("content/emitmask.qml"));
            addExample("Maximum Emitted", "Put a limit on emissions",  Qt.resolvedUrl("content/maximumemitted.qml"));
            addExample("Shape and Direction", "Creates a portal effect",  Qt.resolvedUrl("content/shapeanddirection.qml"));
            addExample("TrailEmitter", "Emit from other particles",  Qt.resolvedUrl("content/trailemitter.qml"));
        }
    }
}
