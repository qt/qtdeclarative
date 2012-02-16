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
import QtQuick.Particles 2.0
import "content/launcher.js" as Util
import "content"

Rectangle {
    color: "black"
    width: 360
    height: 600
    Shell {
        z: 1
        id: shell
        anchors.fill: parent
    }

    VisualDataModel {//TODO: Transitions between modes (and a second mode...)
        id: vdm
        model: [
            "../../affectors/attractor.qml",
            "../../affectors/customaffector.qml",
            "../../affectors/groupgoal.qml",
            "../../affectors/spritegoal.qml",
            "../../affectors/turbulence.qml",
            "../../affectors/wander.qml",
            "../../customparticle/blurparticles.qml",
            "../../customparticle/fragmentshader.qml",
            "../../customparticle/imagecolors.qml",
            "../../emitters/customemitter.qml",
            "../../emitters/emitmask.qml",
            "../../emitters/maximumemitted.qml",
            "../../emitters/shapeanddirection.qml",
            "../../emitters/timedgroupchanges.qml",
            "../../emitters/trailemitter.qml",
            "../../emitters/velocityfrommotion.qml",
            "../../imageparticle/allatonce.qml",
            "../../imageparticle/colortable.qml",
            "../../imageparticle/deformation.qml",
            "../../imageparticle/rotation.qml",
            "../../imageparticle/sprites.qml",
            "../../itemparticle/delegates.qml",
            "../../itemparticle/particleview.qml",
            "../../simple/dynamicemitters.qml",
            "../../simple/multiplepainters.qml",
            "../../simple/startstop.qml",
            "../../plasmapatrol/plasmapatrol.qml"
        ]
        delegate: Rectangle {
            color: "white"
            width: 96
            height: 96
            Image {
                width: 72
                height: 72
                anchors.centerIn: parent
                source: Util.iconFromPath(modelData)
            }
            Text {
                text: Util.nameFromPath(modelData)
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: 8
            }
            MouseArea {
                anchors.fill: parent
                onClicked: shell.setDemo(modelData)
            }
        }
    }

    GridView {
        anchors.fill: parent
        cellWidth: 120
        cellHeight: 120
        model: vdm
    }
}
