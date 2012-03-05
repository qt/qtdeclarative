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
    property real initialDodge: 0.5
    property real dodge: initialDodge
    property int blinkInterval: 800
    onHpChanged: if(hp <= 0) target = container;
    property ParticleSystem system//TODO: Ship abstraction
    property Item target: container
    property string shipParticle: "default"//Per team colors?
    property int gunType: 0
    width: 128
    height: 128
    Emitter {
        id: emitter
        //TODO: Cooler would be an 'orbiting' affector
        //TODO: On the subject, opacity and size should be grouped type 'overLife' if we can cram that in the particles
        system: container.system
        group: container.shipParticle
        shape: EllipseShape {}

        emitRate: hp > 0 ?  hp + 20 : 0 
        lifeSpan: blinkInterval
        maximumEmitted: (maxHP + 20)

        acceleration: AngleDirection {angleVariation: 360; magnitude: 8}

        size: 24
        endSize: 4
        sizeVariation: 8
        width: 16
        height: 16
        x: 64
        y: 64
        Behavior on x {NumberAnimation {duration:blinkInterval}}
        Behavior on y {NumberAnimation {duration:blinkInterval}}
        Timer {
            interval: blinkInterval
            running: true
            repeat: true
            onTriggered: {
                emitter.x = Math.random() * 48 + 32
                emitter.y = Math.random() * 48 + 32
            }
        }
    }
    Hardpoint {
        anchors.centerIn: parent
        id: gun2
        system: container.system
        show: container.hp > 0
        hardpointType: gunType
    }
    Timer {
        id: fireControl
        interval: 800
        running: root.readySetGo
        repeat: true
        onTriggered: {
                gun2.fireAt(container.target);
        }
    }

}
