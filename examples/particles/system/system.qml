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
    \example particles/system
    \brief This is a collection of examples using Affectors in the QML particle system.
    \image qml-system-example.png

    This is a collection of small QML examples relating to using Affectors in the particle system.
    Each example is a small QML file emphasizing a particular element or feature.

    Dynamic comparison compares using the particle system to getting a similar effect with the following code that dynamically instantiates Image elements.
    \snippet examples/particles/system/content/dynamiccomparison.qml fake
    Note how the Image elements are not able to be randomly colorized.

    Start and Stop simply sets the running and paused states of a ParticleSystem. While the system does not perform any simulation when stopped or paused, a restart restarts the simulation from the beginning, while unpausing resumes the simulation from where it was.

    Timed group changes is an example that highlights the ParticleGroup element. While normally referring to groups with a string name is sufficent, additional effects can be
    done by setting properties on groups.
    The first group has a variable duration on it, but always transitions to the second group.
    \snippet examples/particles/system/content/timedgroupchanges.qml 0
    The second group has a TrailEmitter on it, and a fixed duration for emitting into the third group. By placing the TrailEmitter as a direct child of the ParticleGroup, it automatically selects that group to follow.
    \snippet examples/particles/system/content/timedgroupchanges.qml 1
    The third group has an Affector as a direct child, which makes the affector automatically target this group. The affector means that as soon as particles enter this group, a burst function can be called on another emitter, using the x,y positions of this particle.
    \snippet examples/particles/system/content/timedgroupchanges.qml 2

    If TrailEmitter does not suit your needs for multiple emitters, you can also dynamically create Emitters while still using the same ParticleSystem and image particle
    \snippet examples/particles/system/content/dynamicemitters.qml 0
    Note that this effect, a flurry of flying rainbow spears, would be better served with TrailEmitter. It is only done with dynamic emitters in this example to show the concept more simply.

    Multiple Painters shows how to control paint ordering of individual particles. While the paint ordering of particles within one ImagePainter is not strictly defined, ImageParticle elements follow the normal Z-ordering rules for QtQuick items. This example allow you to paint the inside of the particles above the black borders using a pair of ImageParticles each painting different parts of the same logical particle.

*/

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Dynamic Comparison", "Compares with dynamically created elements",  Qt.resolvedUrl("content/dynamiccomparison.qml"));
            addExample("StartStop", "Start and stop the simulation", Qt.resolvedUrl("content/startstop.qml"));
            addExample("Timed group changes", "Emit into managed groups",  Qt.resolvedUrl("content/timedgroupchanges.qml"));
            addExample("Dynamic Emitters", "Dynamically instantiated emitters with a single system",  Qt.resolvedUrl("content/dynamicemitters.qml"));
            addExample("Multiple Painters", "Several ParticlePainters on the same logical particles",  Qt.resolvedUrl("content/multiplepainters.qml"));
        }
    }
}
