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
    id: me
    //Reflective Properties
    width: shipLoader.width
    height: shipLoader.height
    //Transfered Properties
    property int hp: 100//shipLoader.item.maxHP
    property real dodge: shipLoader.item.initialDodge
    property ParticleSystem system
    property int targetIdx: 0
    property Item target: targets[targetIdx] == undefined?null:targets[targetIdx]
    Connections {
        target: me.target
        onHpChanged: if (me.target.hp<=0) me.targetIdx++;
    }
    property list<Item> targets
    property string shipParticle: "default"//Per team colors?
    property int gunType: 0
    property int shipType: 0
    Component {
        id: sloopComp
        Sloop {
            system: me.system
            target: me.target
            shipParticle: me.shipParticle
            gunType: me.gunType
            hp: me.hp
            dodge: me.dodge
        }
    }
    Component {
        id: frigateComp
        Frigate {
            system: me.system
            target: me.target
            shipParticle: me.shipParticle
            gunType: me.gunType
            hp: me.hp
            dodge: me.dodge
        }
    }
    Component {
        id: cruiserComp
        Cruiser {
            system: me.system
            target: me.target
            shipParticle: me.shipParticle
            gunType: me.gunType
            hp: me.hp
            dodge: me.dodge
        }
    }
    Component {
        id: dumbComp
        Item {
            property int maxHP: 0
            property int initialDodge: 0
        }
    }
    Loader {
        id: shipLoader
        sourceComponent: {
            switch (shipType) {
            case 1: sloopComp; break;
            case 2: frigateComp; break;
            case 3: cruiserComp; break;
            default: dumbComp;
            }
        }
    }
}
