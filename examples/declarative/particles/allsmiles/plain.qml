import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle{
    color: "goldenrod"
    width: 2000
    height: 2000
    ParticleSystem{id: sys}
    ImageParticle{
        id: up
        system: sys
        source: "content/singlesmile.png"
    }
    Emitter{
        anchors.centerIn: parent
        system: sys
        emitRate: 1000
        size: 20
        lifeSpan: 10000
        speed: AngleDirection{angleVariation: 360; magnitudeVariation: 100;}
    }
    MouseArea{
        anchors.fill: parent
        onClicked: up.autoRotation = !up.autoRotation
    }
}
