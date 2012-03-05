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

Item {
    property ParticleSystem sys
    ImageParticle {
        system: sys
        groups: ["default"]
        source: "pics/blur-circle3.png"
        color: "#003A3A3A"
        colorVariation: 0.1
        z: 0
    }
    ImageParticle {
        system: sys
        groups: ["redTeam"]
        source: "pics/blur-circle3.png"
        color: "#0028060A"
        colorVariation: 0.1
        z: 0
    }
    ImageParticle {
        system: sys
        groups: ["greenTeam"]
        source: "pics/blur-circle3.png"
        color: "#0006280A"
        colorVariation: 0.1
       z: 0
    }
    ImageParticle {
        system: sys
        groups: ["blaster"]
        source: "pics/star2.png"
        //color: "#0F282406"
        color: "#0F484416"
        colorVariation: 0.2
        z: 2
    }
    ImageParticle {
        system: sys
        groups: ["laser"]
        source: "pics/star3.png"
        //color: "#00123F68"
        color: "#00428FF8"
        colorVariation: 0.2
        z: 2
    }
    ImageParticle {
        system: sys
        groups: ["cannon"]
        source: "pics/particle.png"
        color: "#80FFAAFF"
        colorVariation: 0.1
        z: 2
    }
    ImageParticle {
        system: sys
        groups: ["cannonCore"]
        source: "pics/particle.png"
        color: "#00666666"
        colorVariation: 0.8
        z: 1
    }
    ImageParticle {
        system: sys
        groups: ["cannonWake"]
        source: "pics/star.png"
        color: "#00CCCCCC"
        colorVariation: 0.2
        z: 1
    }
    ImageParticle {
        system: sys
        groups: ["frigateShield"]
        source: "pics/blur-circle2.png"
        color: "#00000000"
        colorVariation: 0.05
        blueVariation: 0.5
        greenVariation: 0.1
        z: 3
    }
    ImageParticle {
        system: sys
        groups: ["cruiserArmor"]
        z: 1
        sprites:[Sprite {
                id: spinState
                name: "spinning"
                source: "pics/meteor.png"
                frameCount: 35
                frameDuration: 40
                to: {"death":0, "spinning":1}
            },Sprite {
                name: "death"
                source: "pics/meteor_explo.png"
                frameCount: 22
                frameDuration: 40
                to: {"null":1}
            }, Sprite {
                name: "null"
                source: "pics/nullRock.png"
                frameCount: 1
                frameDuration: 1000
            }
        ]
    }
    TrailEmitter {
        system: sys
        group: "cannonWake"
        follow: "cannon"
        emitRatePerParticle: 64
        lifeSpan: 600
        speed: AngleDirection { angleVariation: 360; magnitude: 48}
        size: 16
        endSize: 8
        sizeVariation: 2
        enabled: true
        width: 1000//XXX: Terrible hack
        height: 1000
    }
    TrailEmitter {
        system: sys
        group: "cannonCore"
        follow: "cannon"
        emitRatePerParticle: 256
        lifeSpan: 128
        size: 24
        endSize: 8
        enabled: true
        width: 1000//XXX: Terrible hack
        height: 1000
    }
}
