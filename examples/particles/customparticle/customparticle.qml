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
    \title QtQuick.Particles Examples - CustomParticle
    \example particles/customparticle
    \brief This is a collection of examples using CustomParticle in the QML particle system.
    \image qml-customparticle-example.png

    This is a collection of small QML examples relating to using CustomParticle in the particle system.
    Each example is a small QML file emphasizing a different way to use CustomParticle.

    Blur Particles adds a blur effect to the particles, which increases over the particle's life time.
    It uses a custom vertex shader:
    \snippet examples/particles/customparticle/content/blurparticles.qml vertex
    to propagate life time simulation to a custom fragment shader:
    \snippet examples/particles/customparticle/content/blurparticles.qml fragment
    which has access to both the normal image sampler and a blurred sampler, the image plus a ShaderEffect.

    Fragment Shader just uses the particle system as a vertex delivery system.
    \snippet examples/particles/customparticle/content/fragmentshader.qml 0

    Image Colors uses CustomParticle to assign colors to particles based on their location in a picture.
    The vertex shader,
    \snippet examples/particles/customparticle/content/imagecolors.qml vertex
    passes along the starting position for each vertex to the fragment shader,
    \snippet examples/particles/customparticle/content/imagecolors.qml fragment
    which uses it to determine the color for that particle.

*/

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Blur Particles", "Particles that get blurred over time",  Qt.resolvedUrl("content/blurparticles.qml"));
            addExample("Fragment Shader", "Particles drawn with a custom fragment shader", Qt.resolvedUrl("content/fragmentshader.qml"));
            addExample("Image Colors", "An image explodes into colored particles", Qt.resolvedUrl("content/imagecolors.qml"));
        }
    }
}
