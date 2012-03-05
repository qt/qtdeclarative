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
    property real initialDodge: 0.2
    property real dodge: initialDodge
    onHpChanged: if(hp <= 0) target = container;
    property ParticleSystem system//TODO: Ship abstraction
    property Item target: container
    property string shipParticle: "default"//Per team colors?
    property int gunType: 0
    width: 128
    height: 128
    Emitter {
        system: container.system
        group: "frigateShield"
        anchors.centerIn: parent
        size: 92
        emitRate: 1
        lifeSpan: 4800
        enabled: hp > 0
    }
    Emitter {
        system: container.system
        group: container.shipParticle
        anchors.centerIn: parent
        width: 64 
        height: 16
        shape: EllipseShape {}

        size: 16
        sizeVariation: 8
        endSize: 8
        emitRate: hp > 0 ?  hp * 1 + 20 : 0 
        lifeSpan: 1200
        maximumEmitted: (maxHP * 1 + 20)*2
    }
    Timer {
        id: fireControl
        property int next: Math.floor(Math.random() * 2) + 1
        interval: 800
        running: root.readySetGo
        repeat: true
        onTriggered: {
            if (next == 1) {
                gun1.fireAt(container.target);
                next = Math.floor(Math.random() * 2) + 1;
            } else if (next == 2) {
                gun2.fireAt(container.target);
                next = Math.floor(Math.random() * 2) + 1;
            }
        }
    }

    Hardpoint {
        x: 128 - 32 - 12
        y: 64 - 12
        id: gun1
        system: container.system
        show: hp > 0
        hardpointType: gunType
    }
    Hardpoint {
        x: 0 + 32 - 12
        y: 64 - 12
        id: gun2
        system: container.system
        show: hp > 0
        hardpointType: gunType
    }
}
