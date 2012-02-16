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
        group: "cannon"
        enabled: container.show
        system: container.system
        anchors.centerIn: parent
        lifeSpan: 2000
        emitRate: 1

        size: 4
        endSize: 0
    }

    function fireAt(targetArg, hardpoint) {
        target = container.mapFromItem(targetArg, targetArg.width/2, targetArg.height/2);
        if (container.hp <= 0 || targetArg.hp <= 0)
            return;
        //TODO: calculate hit and damage at target, which must be a Ship
        var hit = Math.random() > targetArg.dodge
        if (hit) {
            switch (targetArg.shipType) {
            case 1: hardpoint.damageDealt += 8; break;
            case 2: hardpoint.damageDealt += 10; break;
            case 3: hardpoint.damageDealt += 16; break;
            default: hardpoint.damageDealt += 1000;
            }
        }
        emitter.burst(1);
    }
    Emitter {
        id: emitter
        group: "cannon"
        enabled: false
        system: container.system
        anchors.centerIn: parent

        lifeSpan: 1000
        emitRate: 1
        size: 8
        endSize: 4
        speed: TargetDirection {
            id: blastVector
            targetX: target.x; targetY: target.y; magnitude: 1.1; proportionalMagnitude: true
        }
    }
}
