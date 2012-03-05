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
    //ReflectiveProperties
    //TransferredProperties
    property variant target: {"y": -90, "x":12}
    property ParticleSystem system
    property bool show: true
    property int hardpointType: 0 //default is pea shooter - always bad.

    property Item targetObj: null
    property int damageDealt: 0
    onDamageDealtChanged: dealDamageTimer.start();
    Timer {
        id: dealDamageTimer
        interval: 16
        running: false
        repeat: false
        onTriggered: {targetObj.hp -= damageDealt; damageDealt = 0;}
    }
    width: 24
    height: 24
    function fireAt(targetArg) {//Each implement own
        if (targetArg != null) {
            hardpointLoader.item.fireAt(targetArg, container);
            targetObj = targetArg;
        }
    }
    Loader {
        id: hardpointLoader
        sourceComponent:  {
            switch (hardpointType) {
            case 1: laserComponent; break;
            case 2: blasterComponent; break;
            case 3: cannonComponent; break;
            default: emptyComponent;
            }
        }
    }
    Component {
        id: laserComponent
        LaserHardpoint {
            target: container.target
            system: container.system
            show: container.show
        }
    }
    Component {
        id: blasterComponent
        BlasterHardpoint {
            target: container.target
            system: container.system
            show: container.show
        }
    }
    Component {
        id: cannonComponent
        CannonHardpoint {
            target: container.target
            system: container.system
            show: container.show
        }
    }
    Component {
        id: emptyComponent
        Item {
            function fireAt(obj) {
                console.log("Firing null weapon. It hurts.");
            }
        }
    }
}
