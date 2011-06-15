/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle{
    color: "white"
    width: 240
    height: 360
    ParticleSystem{
        id: sys
    }
    Emitter{
        system:sys
        height: parent.height
        emitRate: 1
        lifeSpan: 12000
        speed: PointDirection{x:20;}
        size: 64
    }
    ShaderEffectSource{
        id: theSource
        sourceItem: theItem
        hideSource: true
    }
    Image{
        id: theItem
        source: "content/smile.png"
    }

    CustomParticle{
        system: sys 
        //TODO: Someway that you don't have to rewrite the basics for a simple addition
        vertexShader:"
            attribute highp vec2 vPos;
            attribute highp vec2 vTex;
            attribute highp vec4 vData; //  x = time,  y = lifeSpan, z = size,  w = endSize
            attribute highp vec4 vVec; // x,y = constant speed,  z,w = acceleration
            attribute highp float r;

            uniform highp mat4 qt_ModelViewProjectionMatrix;                              
            uniform highp float timestamp;
            uniform lowp float qt_Opacity;

            varying highp vec2 fTex;                                
            varying lowp float fFade;
            varying lowp float fBlur;

            void main() {                                           
                fTex = vTex;                                        
                highp float size = vData.z;
                highp float endSize = vData.w;

                highp float t = (timestamp - vData.x) / vData.y;

                highp float currentSize = mix(size, endSize, t * t);

                if (t < 0. || t > 1.)
                currentSize = 0.;

                highp vec2 pos = vPos
                - currentSize / 2. + currentSize * vTex          // adjust size
                + vVec.xy * t * vData.y         // apply speed vector..
                + 0.5 * vVec.zw * pow(t * vData.y, 2.);

                gl_Position = qt_ModelViewProjectionMatrix * vec4(pos.x, pos.y, 0, 1);

                highp float fadeIn = min(t * 10., 1.);
                highp float fadeOut = 1. - max(0., min((t - 0.75) * 4., 1.));

                fFade = fadeIn * fadeOut * qt_Opacity;
                fBlur = max(0.2 * t, t * r);
            }
        "
        property variant source: theSource
        property variant blurred: ShaderEffectSource {
        smooth: true
        sourceItem: ShaderEffectItem {
            width: theItem.width
            height: theItem.height
            property variant delta: Qt.size(0.0, 1.0 / height)
            property variant source: ShaderEffectSource {
                smooth: true
                sourceItem: ShaderEffectItem {
                    width: theItem.width
                    height: theItem.height
                    property variant delta: Qt.size(1.0 / width, 0.0)
                    property variant source: theSource
                    fragmentShader: "
                        uniform sampler2D source;
                        uniform highp vec2 delta;
                        varying highp vec2 qt_TexCoord0;
                        void main() {
                            gl_FragColor = 0.0538 * texture2D(source, qt_TexCoord0 - 3.182 * delta)
                                         + 0.3229 * texture2D(source, qt_TexCoord0 - 1.364 * delta)
                                         + 0.2466 * texture2D(source, qt_TexCoord0)
                                         + 0.3229 * texture2D(source, qt_TexCoord0 + 1.364 * delta)
                                         + 0.0538 * texture2D(source, qt_TexCoord0 + 3.182 * delta);
                        }"
                }
            }
            fragmentShader: "
                uniform sampler2D source;
                uniform highp vec2 delta;
                varying highp vec2 qt_TexCoord0;
                void main() {
                    gl_FragColor = 0.0538 * texture2D(source, qt_TexCoord0 - 3.182 * delta)
                                 + 0.3229 * texture2D(source, qt_TexCoord0 - 1.364 * delta)
                                 + 0.2466 * texture2D(source, qt_TexCoord0)
                                 + 0.3229 * texture2D(source, qt_TexCoord0 + 1.364 * delta)
                                 + 0.0538 * texture2D(source, qt_TexCoord0 + 3.182 * delta);
                }"
            }
        }
        fragmentShader: "
            uniform sampler2D source;
            uniform sampler2D blurred;
            varying highp vec2 fTex;
            varying highp float fBlur;
            varying highp float fFade;
            void main() {
                gl_FragColor = mix(texture2D(source, fTex), texture2D(blurred, fTex), min(1.0,fBlur*3.0)) * fFade;
            }"

    }
}

