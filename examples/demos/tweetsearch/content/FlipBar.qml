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

Item {
    id: container
    property int animDuration: 300
    property int wiggleDuration: 0 // 2x time spent each side to wiggle away before shrinking in
    property real wiggleRoom: 0 // Size on each side it moves away before shrinking during the transition
    property real squeezeFactor: 0.1 // More give a greater squeeze during transition (0.0 for none)
    property Item above: Item {}
    property Item front: Item {}
    property Item back: Item {}
    property real factor: 0.1 // amount the edges fold in for the 3D effect
    property alias delta: effect.delta

    property Item cur: frontShown ? front : back
    property Item noncur:frontShown ? back : front
    function swap() {
        var tmp = front;
        front = back;
        back = tmp;
        resync();
    }

    width: cur.width
    height: cur.height
    onFrontChanged: resync(); 
    onBackChanged: resync();
    function resync() {//TODO: Are the items ever actually visible?
        back.parent = container;
        front.parent = container;
        frontShown ? back.visible = false : front.visible = false;
    }
    property bool frontShown: true
    Rectangle {
        anchors.fill: parent
        color: "white"
    }
    function flipUp(start) {
        effect.visible = true;
        effect.sourceA = effect.source1
        effect.sourceB = effect.source2
        if (start == undefined)
            start = 1.0;
        deltaAnim.from = start;
        deltaAnim.to = 0.0
        dAnim.start();
        frontShown = false;
        sizeAnim.start();
    }
    function flipDown(start) {
        effect.visible = true;
        effect.sourceA = effect.source1
        effect.sourceB = effect.source2
        if (start == undefined)
            start = 0.0;
        deltaAnim.from = start;
        deltaAnim.to = 1.0
        dAnim.start();
        frontShown = true;
        sizeAnim.start();
    }
    SequentialAnimation on height {
        id: sizeAnim
        running: false
        //Note: front has already swapped around when we start
        NumberAnimation {
            duration: wiggleDuration
            to: noncur.height + wiggleRoom
        }
        NumberAnimation {
            duration: animDuration/2
            to: noncur.height - wiggleRoom * 2
        }
        NumberAnimation {
            duration: animDuration/2
            to: cur.height + wiggleRoom
        }
        NumberAnimation {
            duration: wiggleDuration
            to: cur.height
        }
    }
    Binding {
        target: above
        property: "y"
        value: -(container.height - effect.height) / 2
    }
    ShaderEffect {
        id: effect
        //Magic is a quadratic coefficient so that we get a down pointed parabola based on delta with value +1.0 for delta 0 and 1
        //property real magic_x: delta - 0.5
        //property real magic: (magic_x * magic_x) * 2 + 0.5
        width: cur.width
        height: cur.height// * magic*squeezeFactor
        property real factor: container.factor * width
        property real delta: 1.0
        mesh: GridMesh { resolution: Qt.size(8,2) }//1x2 is all that's needed for the rect, but proper contents interpolation wants more
        SequentialAnimation on delta {
            id: dAnim
            running: false
            PauseAnimation { duration: wiggleDuration }
            NumberAnimation {
            id: deltaAnim
            duration: animDuration//expose anim
            //easing.type: Easing.OutQuart
            }
        }
        property variant sourceA: source1
        property variant sourceB: source1
        property variant source1: ShaderEffectSource {
            sourceItem: front
            hideSource: effect.visible
        }
        property variant source2: ShaderEffectSource {
            sourceItem: back
            hideSource: effect.visible
        }
        
        fragmentShader: "
            uniform lowp float qt_Opacity;
            uniform sampler2D sourceA;
            uniform sampler2D sourceB;
            uniform highp float delta;
            varying highp vec2 qt_TexCoord0;
            void main() {
            /* Pre-Vertex
                highp vec4 tex = vec4(qt_TexCoord0.x, qt_TexCoord0.y / delta, qt_TexCoord0.x, (qt_TexCoord0.y-delta)/(1.0-delta));
                //highp float shade = (1.0 - qt_TexCoord0.y) * delta;
                //highp float shade = (1.0 - tex.w) * delta;
                highp float shade = vec4(delta,delta,delta,0.0) ;
                highp vec4 col;
                if (delta > qt_TexCoord0.y)
                    col = texture2D(sourceA, tex.xy);
                else
                    col = texture2D(sourceB, tex.zw) * (vec4(1.0,1.0,1.0,1.0) - shade);
                gl_FragColor = vec4(col.x, col.y, col.z, 1.0) * qt_Opacity;
                //gl_FragColor = vec4(0.0,1.0,delta,1.0) * qt_Opacity;
            */
                highp vec4 tex = vec4(qt_TexCoord0.x, qt_TexCoord0.y * 2.0, qt_TexCoord0.x, (qt_TexCoord0.y-0.5) * 2.0);
                highp float shade = clamp(delta*2.0, 0.5, 1.0);
                highp vec4 col;
                if (qt_TexCoord0.y < 0.5) {
                    col = texture2D(sourceA, tex.xy) * (shade);
                    //w governed by shade
                } else {
                    col = texture2D(sourceB, tex.zw) * (1.5 - shade);
                    col.w = 1.0;
                }
                gl_FragColor = col * qt_Opacity;
            }
        "
        property real h: height
        vertexShader: "
        uniform highp float delta;
        uniform highp float factor;
        uniform highp float h;
        uniform highp mat4 qt_Matrix;
        attribute highp vec4 qt_Vertex;
        attribute highp vec2 qt_MultiTexCoord0;
        varying highp vec2 qt_TexCoord0;
        void main() {
            highp vec4 pos = qt_Vertex;
            if (qt_MultiTexCoord0.y == 0.0)
                pos.x += factor * (1. - delta) * (qt_MultiTexCoord0.x * -2.0 + 1.0);
            else if (qt_MultiTexCoord0.y == 1.0)
                pos.x += factor * (delta) * (qt_MultiTexCoord0.x * -2.0 + 1.0);
            else // (qt_MultiTexCoord0.y == 0.5 )
                pos.y = delta * h;
            gl_Position = qt_Matrix * pos;
            //highp vec2 tex = qt_MultiTexCoord0;
            qt_TexCoord0 = qt_MultiTexCoord0;
        }"

    }
}
