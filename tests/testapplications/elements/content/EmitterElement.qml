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
    id: emitterelementtest
    anchors.fill: parent
    property string testtext: ""

    ParticleSystem {
        id: particlesystem
        anchors.fill: parent
        ImageParticle {
            id: imageparticle
            source: "pics/smile.png"
            color: "red"
            Behavior on color { ColorAnimation { duration: 3000 } }
        }
        Emitter {
            id: emitterelement
            anchors.centerIn: parent
            property int emitspeed: 10
            emitRate: 5
            lifeSpan: 1000
            speed: AngleDirection { angle: 0; angleVariation: 360; magnitude: emitterelement.emitspeed }
        }
    }


    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: emitterelementtest
                testtext: "This is an Emitter element, visualized by an ImageParticle. It should be emitting particles "+
                "slowly from the center of the display.\n"+
                "Next, let's change the emission speed of the particles." }
        },
        State { name: "fast"; when: statenum == 2
            PropertyChanges { target: emitterelement; emitspeed: 50 }
            PropertyChanges { target: emitterelementtest
                testtext: "The particles emitted should be moving more quickly.\n"+
                "Next, let's increase the number of particles emitted." }
        },
        State { name: "many"; when: statenum == 3
            PropertyChanges { target: emitterelement; emitspeed: 50; emitRate: 100 }
            PropertyChanges { target: emitterelementtest
            testtext: "The particles should now be quick and numerous.\n"+
                "Next, let's allow them to survive longer." }
        },
        State { name: "enduring"; when: statenum == 4
            PropertyChanges { target: emitterelement; emitspeed: 50; emitRate: 100; lifeSpan: 3000 }
            PropertyChanges { target: emitterelementtest
                testtext: "The particles should now be enduring to the edges of the display.\n"+
                "Next, let's have them changing their size." }
        },
        State { name: "sized"; when: statenum == 5
            PropertyChanges { target: emitterelement; emitspeed: 50; emitRate: 100; lifeSpan: 3000; size: 20; endSize: 5 }
            PropertyChanges { target: emitterelementtest
                testtext: "The particles should now be starting large and ending small.\n"+
                "Advance to restart the test." }
        }
    ]
}