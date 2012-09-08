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
    \example particles/imageparticle
    \brief This is a collection of examples using Affectors in the QML particle system.
    \image qml-imageparticle-example.png

    This is a collection of small QML examples relating to using Affectors in the particle system.
    Each example is a small QML file emphasizing a particular element or feature.

    All at once shows off several of the features of ImageParticle at the same time.
    \snippet examples/particles/imageparticle/content/allatonce.qml 0

    Colored shows a simple ImageParticle with some color variation.
    \snippet examples/particles/imageparticle/content/colored.qml 0

    Color Table sets the color over life on the particles to provide a fixed rainbow effect.
    \snippet examples/particles/imageparticle/content/colortable.qml 0

    Deformation spins and squishes a starfish particle.
    \snippet examples/particles/imageparticle/content/colortable.qml spin
    \snippet examples/particles/imageparticle/content/colortable.qml deform

    Rotation demonstrates the autoRotate property, so that particles rotate in the direction that they travel.

    Sharing demonstrates what happens when multiple ImageParticles try to render the same particle.
    The following ImageParticle renders the particles inside the ListView:
    \snippet examples/particles/imageparticle/content/sharing.qml 0
    The following ImageParticle is placed inside the list highlight, and renders the particles above the other ImageParticle.
    \snippet examples/particles/imageparticle/content/sharing.qml 1
    Note that because it sets the color and alpha in this ImageParticle, it renders the particles in a different color.
    Since it doesn't specify anything about the rotation, it shares the rotation with the other ImageParticle so that the flowers are rotated the same way in both.
    Note that you can undo rotation in another ImageParticle, you just need to explicitly set rotationVariation to 0.

    Sprites demonstrates using an image particle to render animated sprites instead of static images for each particle.
*/

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("All at once", "Uses all ImageParticle features",  Qt.resolvedUrl("content/allatonce.qml"));
            addExample("Colored", "Colorized image particles",  Qt.resolvedUrl("content/colored.qml"));
            addExample("Color Table", "Color-over-life rainbow particles",  Qt.resolvedUrl("content/colortable.qml"));
            addExample("Deformation", "Deformed particles",  Qt.resolvedUrl("content/deformation.qml"));
            addExample("Rotation", "Rotated particles",  Qt.resolvedUrl("content/rotation.qml"));
            addExample("Sharing", "Multiple ImageParticles on the same particles",  Qt.resolvedUrl("content/sharing.qml"));
            addExample("Sprites", "Partiles rendered with sprites",  Qt.resolvedUrl("content/sprites.qml"));
        }
    }
}
