/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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
    id: particlesystemelementtest
    anchors.fill: parent
    property string testtext: ""
    Rectangle { x: 50; y: 200; height: 300; width: 260; color: "transparent"; border.color: "lightgray" }
    ParticleSystem {
        id: particlesystemelement
        anchors.fill: parent
        ImageParticle { source: "pics/star.png"; color: "red" }
        Emitter {
            id: particleemitter
            x: 50; y: 200
            emitRate: 100
            speed: AngleDirection { angle: 0; angleVariation: 360; magnitude: 100 }
            Rectangle { anchors.centerIn: parent; height: 1; width: 1; color: "black" }
            SequentialAnimation {
                running: true; paused: particlesystemelement.paused; loops: Animation.Infinite
                NumberAnimation { target: particleemitter; properties: "x"; to: 310; duration: 3000 }
                NumberAnimation { target: particleemitter; properties: "y"; to: 500; duration: 3000 }
                NumberAnimation { target: particleemitter; properties: "x"; to: 50;  duration: 3000 }
                NumberAnimation { target: particleemitter; properties: "y"; to: 200; duration: 3000 }
            }
        }
    }


    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: particlesystemelementtest
                testtext: "This is an ParticleSystem element. It is represented by an ImageParticle, "+
                "tracing around the display.\n"+
                "Next, it should pause simulation." }
        },
        State { name: "paused"; when: statenum == 2
            PropertyChanges { target: particlesystemelement; paused: true }
            PropertyChanges { target: particlesystemelementtest
                testtext: "The simulation should now be paused.\n"+
                "Advance to resume simulation." }
        },
        State { name: "resumed"; when: statenum == 3
            // PropertyChanges { target: bugpanel; bugnumber: "21539" } FIXED
            PropertyChanges { target: particlesystemelement; paused: false }
            PropertyChanges { target: particlesystemelementtest
                testtext: "The simulation should now be active.\n"+
                "Advance to restart the test" }
        }
    ]
}