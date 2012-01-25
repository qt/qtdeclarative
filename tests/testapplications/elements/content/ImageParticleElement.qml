/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Particles 2.0

Item {
    id: imageparticleelementtest
    anchors.fill: parent
    property string testtext: ""

    ParticleSystem {
        id: particlesystemelement
        anchors.fill: parent
        ImageParticle {
            id: imageparticle
            source: "pics/smile.png"
            color: "red"
            Behavior on color { ColorAnimation { duration: 3000 } }
        }
        Emitter {
            id: particleemitter
            anchors.centerIn: parent
            emitRate: 50
            lifeSpan: 3000
            speed: AngleDirection { angle: 0; angleVariation: 360; magnitude: 60 }
        }
    }


    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: imageparticleelementtest
                testtext: "This is an ImageParticle element. It should be emitting particles "+
                "from the center of the display.\n"+
                "Next, let's change the color of the particles." }
        },
        State { name: "green"; when: statenum == 2
            PropertyChanges { target: imageparticle; color: "lightgreen" }
            PropertyChanges { target: imageparticleelementtest
                testtext: "The particles should now be green.\n"+
                "Next, let's get them spinning." }
        },
        State { name: "spinning"; when: statenum == 3
            PropertyChanges { target: imageparticle; color: "lightgreen"; rotation: 360; rotationSpeed: 100 }
            PropertyChanges { target: imageparticleelementtest
                testtext: "The particles should now be green and spinning.\n"+
                "Next, let's get them popping in and out." }
        },
        State { name: "scaling"; when: statenum == 4
            PropertyChanges { target: imageparticle; color: "lightgreen"; rotation: 360; rotationSpeed: 100; entryEffect: ImageParticle.Scale }
            PropertyChanges { target: imageparticleelementtest
                testtext: "The particles should now be scaling in and out.\n"+
                "Advance to restart the test." }
        }
    ]
}