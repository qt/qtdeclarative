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
    id: container
    property int maxHP: 100
    property int hp: maxHP
    property real initialDodge: 0.01
    property real dodge: initialDodge
    onHpChanged: if(hp <= 0) target = container;
    property ParticleSystem system//TODO: Ship abstraction
    property Item target: container
    property string shipParticle: "default"//Per team colors?
    property int gunType: 0
    width: 128
    height: 128
    Emitter {
        //TODO: Cooler would be an 'orbiting' affector
        //TODO: On the subject, opacity and size should be grouped type 'overLife' if we can cram that in the particles
        system: container.system
        group: container.shipParticle
        anchors.centerIn: parent
        width: 64
        height: 64
        shape: EllipseShape {}

        emitRate: hp > 0 ?  hp * 1 + 20 : 0 
        lifeSpan: 2400
        maximumEmitted: (maxHP * 1 + 20)*2.4

        size: 48
        sizeVariation: 16
        endSize: 16

        speed: AngleDirection {angleVariation:360; magnitudeVariation: 32}
    }
    Emitter {
        system: container.system
        group: "cruiserArmor"
        anchors.fill: parent
        shape: EllipseShape { fill: false }
        enabled: hp>0
        
        emitRate: 16
        lifeSpan: 2000

        size: 48
        sizeVariation: 24

        SpriteGoal {
            id: destructor
            system: container.system
            enabled: container.hp <=0
            anchors.fill: parent
            groups: ["cruiserArmor"]
            goalState: "death"
//            jump: true
            once: true
        }
    }

    Timer {
        id: fireControl
        property int next: Math.floor(Math.random() * 3) + 1
        interval: 800
        running: root.readySetGo
        repeat: true
        onTriggered: {
            if (next == 1) {
                gun1.fireAt(container.target);
                next = Math.floor(Math.random() * 3) + 1;
            } else if (next == 2) {
                gun2.fireAt(container.target);
                next = Math.floor(Math.random() * 3) + 1;
            } else if (next == 3) {
                gun3.fireAt(container.target);
                next = Math.floor(Math.random() * 3) + 1;
            }
        }
    }

    Hardpoint {//TODO: Hardpoint abstraction
        x: 112 - 12 - 8*2
        y: 128 - 12 - 12*2
        id: gun1
        system: container.system
        show: hp > 0
        hardpointType: gunType
    }
    Hardpoint {
        x: 64 - 12
        y: 0 - 12 + 12*2
        id: gun2
        system: container.system
        show: hp > 0
        hardpointType: gunType
    }
    Hardpoint {
        x: 16 - 12 + 8*2
        y: 128 - 12 - 12*2
        id: gun3
        system: container.system
        show: hp > 0
        hardpointType: gunType
    }
}
