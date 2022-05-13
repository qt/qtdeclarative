// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: textelementtest
    anchors.fill: parent
    property string testtext: ""

    Text {
        id: textelement
        property int pseudopointsize: 12
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 15
        height: 200; width: parent.width *.8; wrapMode: Text.WordWrap; font.pointSize: 12
        text: "Hello, my name is Text"; horizontalAlignment: Text.AlignHCenter; textFormat: Text.PlainText
        Behavior on font.pointSize { NumberAnimation { duration: 1000 } }
        Behavior on color { ColorAnimation { duration: 1000 } }
    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: textelementtest
                testtext: "This is a Text element. At present it should be saying hello.\n"+
                "Next, it will animatedly increase the text to twice its size" }
        },
        State { name: "fontlarge"; when: statenum == 2
            PropertyChanges { target: textelement; font.pointSize: 24 }
            PropertyChanges { target: textelementtest
                testtext: "The font should have increased in size, and wrapped once too large for the screen width.\n"+
                "Next, let's change the color to blue." }
            PropertyChanges { target: bugpanel; bugnumber: "19848" }
        },
        State { name: "fontlargeblue"; when: statenum == 3
            PropertyChanges { target: textelement; font.pointSize: 24; color: "blue" }
            PropertyChanges { target: textelementtest
                testtext: "The font should now be blue.\n"+
                "Next, let's add some formatting." }
        },
        State { name: "fontlargeblueitalicsbold"; when: statenum == 4
            PropertyChanges { target: textelement; font.pointSize: 24; color: "blue"; font.bold: true; font.italic: true }
            PropertyChanges { target: textelementtest
                testtext: "The font should now be blue, bold and italic.\n"+
                "Next, let's use rich text." }
            PropertyChanges { target: bugpanel; bugnumber: "19848" }
        },
        State { name: "fontlargerichtext"; when: statenum == 5
            PropertyChanges {
                target: textelement; font.pointSize: 24; color: "blue"; font.bold: true; font.italic: true
                text: "<font color=\"green\">I</font> am <font color=\"green\">now</font> multicolored!"; textFormat: Text.RichText
            }
            PropertyChanges { target: textelementtest
                testtext: "The font should now be bold and italic, and alternating in color.\n"+
                "The test will now return to the start." }
        }
    ]

}
