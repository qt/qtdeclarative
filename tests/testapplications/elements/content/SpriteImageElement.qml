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

Item {
    id: spriteimageelementtest
    anchors.fill: parent
    property string testtext: ""
    SpriteImage {
        id: spriteimage
        sprites: [Sprite {
            name: "happy"
            source: "pics/squarefacesprite2.png"
            frames: 6
            duration: 120
            to: {"silly": 1, "sad":0}
        }, Sprite {
            name: "silly"
            source: "pics/squarefacesprite.png"
            frames: 6
            duration: 120
            to: {"happy": 1, "sad": 0}
        }, Sprite {
            name: "sad"
            source: "pics/squarefacesprite3.png"
            frames: 6
            duration: 120
            to: {"evil": 0.5, "sad": 1, "cyclops" : 0}
        }, Sprite {
            name: "cyclops"
            source: "pics/squarefacesprite4.png"
            frames: 6
            duration: 120
            to: {"love": 0.1, "boggled": 0.1, "cyclops" : 0.1}
        }, Sprite {
            name: "evil"
            source: "pics/squarefacesprite5.png"
            frames: 6
            duration: 120
            to: {"sad": 1.0, "cyclops" : 0}
        }, Sprite {
            name: "love"
            source: "pics/squarefacesprite6.png"
            frames: 6
            duration: 120
            to: {"love": 0.1, "boggled": 0.1, "cyclops" : 0.1}
        }, Sprite {
            name: "boggled"
            source: "pics/squarefacesprite7.png"
            frames: 6
            duration: 120
            to: {"love": 0.1, "boggled": 0.1, "cyclops" : 0.1, "dying":0}
        }, Sprite {
            name: "dying"
            source: "pics/squarefacespriteX.png"
            frames: 4
            duration: 120
            to: {"dead":1.0}
        }, Sprite {
            name: "dead"
            source: "pics/squarefacespriteXX.png"
            frames: 1
            duration: 10000
        }]

        width: 300
        height: 300
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
    }


    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            StateChangeScript { script: spriteimage.jumpTo("happy"); }
            PropertyChanges { target: spriteimageelementtest
                testtext: "This is a SpriteImage element. It should be animating currently."+
                "It should alternate between winking and sticking out its tongue." }
        },
        State { name: "stochastic2"; when: statenum == 2
            StateChangeScript { script: spriteimage.jumpTo("sad"); }
            PropertyChanges { target: spriteimageelementtest
                testtext: "The sprite should now be animating between frowning and being evil."+
                "This should not be alternating, but mostly frowning with the occasional evil eyes."+
                "After an evil eyes animation, it should return to frowning at least once." }
        },
        State { name: "stochastic3"; when: statenum == 3
            StateChangeScript { script: spriteimage.jumpTo("cyclops"); }
            PropertyChanges { target: spriteimageelementtest
                testtext: "The sprite should now be animating fairly randomly between three animations where it does silly things with its eyes.\n"+
                "Next the sprite will animate into a static 'dead' state."+
                "When it does, it should first animate to and play through the 'big eyes' animation (if it is not currently playing that animation) before it enters the dying animation."}
        },
        State { name: "dead"; when: statenum == 4
            PropertyChanges { target: spriteimage; goalState: "dead" }
            PropertyChanges { target: spriteimageelementtest
                testtext: "After a brief dying animation, the image should now be static.\n"+
                "Advance to restart the test." }
        }
    ]
}
