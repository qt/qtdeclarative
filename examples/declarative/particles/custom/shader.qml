import QtQuick 2.0
import QtQuick.Particles 2.0

ParticleSystem{
    id: root
    width: 1024
    height: 768
    Rectangle{
        z: -1
        anchors.fill: parent
        color: "black"
        Text{
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 36
            color: "white"
            text: "It's all in the fragment shader."
        }
    }
    Emitter{
        emitRate: 400
        lifeSpan: 8000
        size: 24
        sizeVariation: 16
        speed: PointDirection{x: root.width/10; y: root.height/10;}
        //acceleration: AngledDirection{angle:225; magnitude: root.width/36; angleVariation: 45; magnitudeVariation: 80}
        acceleration: PointDirection{x: -root.width/40; y: -root.height/40; xVariation: -root.width/20; yVariation: -root.width/20}
    }
    CustomParticle{
        //TODO: Someway that you don't have to rewrite the basics for a simple addition
        vertexShader:"
            attribute highp vec2 vPos;
            attribute highp vec2 vTex;
            attribute highp vec4 vData; //  x = time,  y = lifeSpan, z = size,  w = endSize
            attribute highp vec4 vVec; // x,y = constant speed,  z,w = acceleration
            attribute highp float r;

            uniform highp mat4 qt_Matrix;
            uniform highp float timestamp;
            uniform lowp float qt_Opacity;

            varying highp vec2 fTex;                                
            varying lowp float fFade;
            varying highp vec2 fPos;

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

                gl_Position = qt_Matrix * vec4(pos.x, pos.y, 0, 1);

                highp float fadeIn = min(t * 20., 1.);
                highp float fadeOut = 1. - max(0., min((t - 0.75) * 4., 1.));

                fFade = fadeIn * fadeOut * qt_Opacity;
                fPos = vec2(pos.x/1024., pos.y/768.);
            }
        "
        fragmentShader: "
            varying highp vec2 fPos;
            varying lowp float fFade;
            varying highp vec2 fTex;
            void main() {//*2 because this generates dark colors mostly
                highp vec2 circlePos = fTex*2.0 - vec2(1.0,1.0);
                highp float dist = length(circlePos);
                highp float circleFactor = max(min(1.0 - dist, 1.0), 0.0);
                gl_FragColor = vec4(fPos.x*2.0 - fPos.y, fPos.y*2.0 - fPos.x, fPos.x*fPos.y*2.0, 0.0) * circleFactor * fFade;
            }"

    }
}
