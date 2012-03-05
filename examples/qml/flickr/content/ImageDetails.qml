/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Particles 2.0

Flipable {
    id: container

    property alias frontContainer: containerFront
    property string photoTitle: ""
    property string photoTags: ""
    property int photoWidth
    property int photoHeight
    property string photoType
    property string photoAuthor
    property string photoDate
    property string photoUrl
    property int rating: 2
    property variant prevScale: 1.0

    property int flipDuration: 1600

    signal closed

    transform: Rotation {
        id: itemRotation
        origin.x: container.width / 2;
        axis.y: 1; axis.z: 0
    }

    front: Item {
        id: containerFront; anchors.fill: container

        Rectangle {
            anchors.fill: parent
            color: "black"; opacity: 0.4
        }

        Column {
            spacing: 10
            anchors {
                left: parent.left; leftMargin: 10
                right: parent.right; rightMargin: 10
                top: parent.top; topMargin: 120
            }
            Text { font.bold: true; color: "white"; elide: Text.ElideRight; text: container.photoTitle; width: parent.width }
            Text { color: "white"; elide: Text.ElideRight; text: "Size: " + container.photoWidth + 'x' + container.photoHeight; width: parent.width }
            Text { color: "white"; elide: Text.ElideRight; text: "Type: " + container.photoType; width: parent.width }
            Text { color: "white"; elide: Text.ElideRight; text: "Author: " + container.photoAuthor; width: parent.width }
            Text { color: "white"; elide: Text.ElideRight; text: "Published: " + container.photoDate; width: parent.width }
            Text { color: "white"; elide: Text.ElideRight; text: container.photoTags == "" ? "" : "Tags: "; width: parent.width }
            Text { color: "white"; elide: Text.ElideRight; text: container.photoTags; width: parent.width }
        }
    }

    back: Item {
        anchors.fill: container

        Rectangle { anchors.fill: parent; color: "black"; opacity: 0.4 }

        Progress {
            anchors.centerIn: parent; width: 200; height: 22
            progress: bigImage.progress; visible: bigImage.status != Image.Ready
        }

        Flickable {
            id: flickable; anchors.fill: parent; clip: true
            contentWidth: imageContainer.width; contentHeight: imageContainer.height

            function updateMinimumScale() {
                if (bigImage.status == Image.Ready && bigImage.width != 0) {
                    slider.minimum = Math.min(flickable.width / bigImage.width, flickable.height / bigImage.height);
                    if (bigImage.width * slider.value > flickable.width) {
                        var xoff = (flickable.width/2 + flickable.contentX) * slider.value / prevScale;
                        flickable.contentX = xoff - flickable.width/2;
                    }
                    if (bigImage.height * slider.value > flickable.height) {
                        var yoff = (flickable.height/2 + flickable.contentY) * slider.value / prevScale;
                        flickable.contentY = yoff - flickable.height/2;
                    }
                    prevScale = slider.value;
                }
            }

            onWidthChanged: updateMinimumScale()
            onHeightChanged: updateMinimumScale()

            Item {
                id: imageContainer
                width: Math.max(bigImage.width * bigImage.scale, flickable.width);
                height: Math.max(bigImage.height * bigImage.scale, flickable.height);
                Image {
                    id: bigImage; source: container.photoUrl; scale: slider.value
                    anchors.centerIn: parent; smooth: !flickable.movingVertically
                    onStatusChanged : {
                        // Default scale shows the entire image.
                        if (bigImage.status == Image.Ready && bigImage.width != 0) {
                            slider.minimum = Math.min(flickable.width / bigImage.width, flickable.height / bigImage.height);
                            prevScale = Math.min(slider.minimum, 1);
                            slider.value = prevScale;
                        }
                        if (inBackState && bigImage.status == Image.Ready)
                            effectBox.imageInAnim();
                    }
                    property bool inBackState: false
                    onInBackStateChanged:{
                        if(inBackState && bigImage.status == Image.Ready)
                            effectBox.imageInAnim();
                        else if (!inBackState && bigImage.status == Image.Ready)
                            effectBox.imageOutAnim();
                    }
                }
                ShaderEffectSource{
                    id: pictureSource
                    sourceItem: bigImage 
                    smooth: true
                    //Workaround: Doesn't work below lines
                    width: bigImage.width
                    height: bigImage.width
                    visible: false
                }
                Turbulence{//only fill visible rect
                    id: turbulence
                    system: imageSystem
                    anchors.fill: parent 
                    strength: 240
                    enabled: false
                }

                Item{
                    id: effectBox
                    width: bigImage.width * bigImage.scale
                    height: bigImage.height * bigImage.scale
                    anchors.centerIn: parent
                    function imageInAnim(){
                        bigImage.visible = false;
                        noiseIn.visible = true;
                        endEffectTimer.start();
                    }
                    function imageOutAnim(){
                        bigImage.visible = false;
                        noiseIn.visible = false;
                        turbulence.enabled = true;
                        endEffectTimer.start();
                        pixelEmitter.burst(2048);
                    }
                    Timer{
                        id: endEffectTimer
                        interval: flipDuration
                        repeat: false
                        running: false
                        onTriggered:{
                            turbulence.enabled = false;
                            noiseIn.visible = false;
                            bigImage.visible = true;
                        }
                    }
                    ShaderEffect{
                        id: noiseIn
                        anchors.fill: parent
                        property real t: 0
                        visible: false
                        onVisibleChanged: tAnim.start()
                        NumberAnimation{
                            id: tAnim
                            target: noiseIn
                            property: "t"
                            from: 0.0 
                            to: 1.0
                            duration: flipDuration
                        }
                        property variant source: pictureSource
                        property variant noise: ShaderEffectSource{
                            sourceItem:Image{
                                source: "images/noise.png"
                            }
                            hideSource: true
                            smooth: false
                        }
                        fragmentShader:"
                            uniform sampler2D noise;
                            uniform sampler2D source;
                            uniform highp float t;
                            uniform lowp float qt_Opacity;
                            varying highp vec2 qt_TexCoord0;
                            void main(){
                                //Want to use noise2, but it always returns (0,0)?
                                if(texture2D(noise, qt_TexCoord0).w <= t)
                                    gl_FragColor = texture2D(source, qt_TexCoord0) * qt_Opacity;
                                else
                                    gl_FragColor = vec4(0.,0.,0.,0.);
                            }
                        "
                    }
                    ParticleSystem{
                        id: imageSystem
                    }
                    Emitter{
                        id: pixelEmitter
                        system: imageSystem
                        //anchors.fill: parent
                        width: Math.min(bigImage.width * bigImage.scale, flickable.width);
                        height: Math.min(bigImage.height * bigImage.scale, flickable.height);
                        anchors.centerIn: parent
                        size: 4
                        lifeSpan: flipDuration
                        emitRate: 2048
                        enabled: false
                    }
                    CustomParticle{
                        id: blowOut
                        system: imageSystem
                        property real maxWidth: effectBox.width
                        property real maxHeight: effectBox.height
                        vertexShader:"
                            uniform highp float maxWidth;
                            uniform highp float maxHeight;

                            varying highp vec2 fTex2;

                            void main() {
                                defaultMain();
                                fTex2 = vec2(qt_ParticlePos.x / maxWidth, qt_ParticlePos.y / maxHeight);
                            }
                        "
                        property variant pictureTexture: pictureSource
                        fragmentShader: "
                            uniform lowp float qt_Opacity;
                            uniform sampler2D pictureTexture;
                            varying highp vec2 fTex2;
                            void main() {
                                gl_FragColor = texture2D(pictureTexture, fTex2) * qt_Opacity;
                        }"
                    }



                }
            }
        }

        Text {
            text: "Image Unavailable"
            visible: bigImage.status == Image.Error
            anchors.centerIn: parent; color: "white"; font.bold: true
        }

        Slider {
            id: slider; visible: { bigImage.status == Image.Ready && maximum > minimum }
            anchors {
                bottom: parent.bottom; bottomMargin: 65
                left: parent.left; leftMargin: 25
                right: parent.right; rightMargin: 25
            }
            onValueChanged: {
                if (bigImage.width * value > flickable.width) {
                    var xoff = (flickable.width/2 + flickable.contentX) * value / prevScale;
                    flickable.contentX = xoff - flickable.width/2;
                }
                if (bigImage.height * value > flickable.height) {
                    var yoff = (flickable.height/2 + flickable.contentY) * value / prevScale;
                    flickable.contentY = yoff - flickable.height/2;
                }
                prevScale = value;
            }
        }
    }

    states: State {
        name: "Back"
        PropertyChanges { target: itemRotation; angle: 180 }
        PropertyChanges { target: toolBar; button2Visible: false }
        PropertyChanges { target: toolBar; button1Label: "Back" }
        PropertyChanges { target: bigImage; inBackState: true }
    }

    transitions: Transition {
        SequentialAnimation {
            PropertyAction { target: bigImage; property: "smooth"; value: false }
            NumberAnimation { easing.type: Easing.InOutQuad; properties: "angle"; duration: flipDuration }
            PropertyAction { target: bigImage; property: "smooth"; value: !flickable.movingVertically }
        }
    }
}
