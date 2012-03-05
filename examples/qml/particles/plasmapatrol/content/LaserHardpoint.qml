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
    property variant target: {"y": -90, "x":12}
    property ParticleSystem system
    property bool show: true

    width: 24
    height: 24
    Emitter {
        id: visualization
        group: "laser"
        system: container.system
        anchors.fill: parent
        enabled: container.show
        shape: EllipseShape {}
        speed: TargetDirection { targetX: width/2; targetY: width/2; magnitude: -1; proportionalMagnitude: true }
        lifeSpan: 1000
        emitRate: 64

        size: 24
        sizeVariation: 8
        endSize: 8
    }

    function fireAt(targetArg, hardpoint) {
        if (targetArg.hp <= 0)
            return;
        //TODO: calculate hit and damage at target, which must be a Ship
        var offset = 0;
        if (Math.random() < 0.99) {
            switch (targetArg.shipType) {
            case 1: hardpoint.damageDealt += 16; break;
            case 2: hardpoint.damageDealt += 4; break;
            case 3: hardpoint.damageDealt += 8; break;
            default: hardpoint.damageDealt += 500; //Really effective against unregistered vessels
            }
        } else {//Misses with Lasers are really rare
            offset = Math.random() * 100;
        }
        target = container.mapFromItem(targetArg, offset + targetArg.width/2, offset + targetArg.height/2);
        emitter.pulse(100);
 //       console.log("Fire box: " +  Math.min(container.width/2, target.x) + "," + Math.min(container.height/2, target.y) + " " + (Math.max(container.width/2, target.x) - Math.min(container.width/2, target.x)) + "," + (Math.max(container.height/2, target.y) - Math.min(container.height/2, target.y)));
    }
    Emitter {
        id: emitter
        group: "laser"
        enabled: false
        system: container.system
        x: Math.min(container.width/2, target.x);
        width: Math.max(container.width/2, target.x) - x;
        y: Math.min(container.height/2, target.y);
        height: Math.max(container.height/2, target.y) - y;
        shape: LineShape {
            mirrored: (emitter.y < 0 || emitter.x < 0) && !(emitter.y < 0 && emitter.x < 0 )//I just want XOR
        }

        lifeSpan: 1000
        emitRate: 8000
        maximumEmitted: 800
        size: 16
        endSize: 0

        speed: PointDirection {xVariation: 4; yVariation: 4}
    }
}
