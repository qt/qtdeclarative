import QtQuick 2.0
import QtQuick.Particles 2.0

ParticleSystem{
    id: sys
    width: 360
    height: 600
    Rectangle{
        z: -1
        anchors.fill: parent
        color: "black"
        Text{
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 42
            color: "white"
            text: "It's all in QML."
        }
    }
    property real petalLength: 180
    property real petalRotation: 0
    NumberAnimation on petalRotation{
        from: 0;
        to: 360;
        loops: -1;
        running: true
        duration: 24000
    }
    function convert(a){return a*(Math.PI/180);}
    Emitter{
        lifeSpan: 4000
        emitRate: 120
        size: 12
        anchors.centerIn: parent
        onEmitParticle:{
            particle.size = Math.max(12,Math.min(492,Math.tan(particle.t/2)*24));
            var theta = Math.floor(Math.random() * 6.0) / 6.0;
            theta *= 2.0*Math.PI;
            theta += sys.convert(sys.petalRotation);
            particle.vx = petalLength * Math.cos(theta);
            particle.vy = petalLength * Math.sin(theta);
            particle.ax = particle.vx * -0.5;
            particle.ay = particle.vy * -0.5;
        }
    }
    CustomParticle{
        vertexShader:"
            uniform lowp float qt_Opacity;
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
                fPos = vec2(pos.x/360.0, pos.y/600.0);
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
