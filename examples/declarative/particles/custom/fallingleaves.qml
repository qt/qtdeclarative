import QtQuick 2.0
import QtQuick.Particles 2.0

Item {
    width: 360
    height: 600

    Image{
        source: "content/backgroundLeaves.jpg"
        anchors.fill: parent
    }
    ParticleSystem{ id: sys }
    Emitter{
        system: sys
        width: parent.width
        emitRate: 6
        lifeSpan: 12000
        size: 80
        speed: PointDirection{ y: 60 }
    }
    Wander {
        system: sys
        anchors.fill: parent
        xVariance: 60
        pace: 60
    }
    CustomAffector{
        system: sys
        property real coefficient: 0.5
        property real speed: 10.0
        onAffectParticle:{
        /*  //Linear movement
            if (particle.r == 0){
                particle.r = Math.random() > 0.5 ? -1 : 1;
            }else if (particle.r == 1){
                particle.rotation += speed * dt;
                if(particle.rotation >= maxAngle)
                    particle.r = -1;
            }else if (particle.r == -1){
                particle.rotation -= speed * dt;
                if(particle.rotation <= -1 * maxAngle)
                    particle.r = 1;
            }
        */
            //Wobbly movement
            if (particle.r == 0.0){
                particle.r = Math.random() + 0.01;
            }
            particle.rotation += speed * particle.r * dt;
            particle.r -= particle.rotation * coefficient;
            if (particle.r == 0.0)
                particle.r -= particle.rotation * 0.000001;
        }
    }

    ImageParticle{
        anchors.fill: parent
        id: particles
        system: sys
        sprites: [Sprite{
                source: "content/realLeaf1.png"
                frames: 1
                duration: 1
                to: {"a":1, "b":1, "c":1, "d":1}
            }, Sprite{
                name: "a"
                source: "content/realLeaf1.png"
                frames: 1
                duration: 10000
            },
            Sprite{
                name: "b"
                source: "content/realLeaf2.png"
                frames: 1
                duration: 10000
            },
            Sprite{
                name: "c"
                source: "content/realLeaf3.png"
                frames: 1
                duration: 10000
            },
            Sprite{
                name: "d"
                source: "content/realLeaf4.png"
                frames: 1
                duration: 10000
            }
        ]

        width: 100
        height: 100
        x: 20
        y: 20
        z:4
    }
}
