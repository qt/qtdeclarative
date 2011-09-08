/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of QtUiTest.
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

Item {
    id: textinputelementtest
    anchors.fill: parent
    property string testtext: ""

    Rectangle {
        id: textinputelementbackground
        color: "green"; height: 50; width: parent.width *.8; border.color: "gray"; opacity: 0.7; radius: 5
        anchors.centerIn: parent
        TextInput {
            id: textinputelement
            font.pointSize: 12; width: parent.width; text: ""; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            Behavior on font.pointSize { NumberAnimation { duration: 1000 } }
            Behavior on color { ColorAnimation { duration: 1000 } }
        }
    }

    Rectangle {
        id: secondarybackground
        color: "lightgray"; border.color: "gray"; opacity: 0.7; radius: 5; height: 50; width: parent.width *.8
        anchors { top: textinputelementbackground.bottom; topMargin: 100; horizontalCenter: parent.horizontalCenter }

        TextInput {
            id: secondary
            property string ignoretext: "Nothing to see here"
            font.pointSize: 12; text: ""; opacity: text == ignoretext ? .3 : 1; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent; width: parent.width
        }
    }
    Rectangle {
        id: shadowrect
        color: "lightgray"; height: 50; width: parent.width *.8; border.color: "gray"; opacity: 0; radius: 5
        anchors.horizontalCenter: textinputelementbackground.horizontalCenter;
        anchors.verticalCenter: textinputelementbackground.verticalCenter;
        Text {
            id: shadowtext
            font.pointSize: 12; width: parent.width; text: ""; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
        }
    }
    transitions: Transition {
        AnchorAnimation { targets: shadowrect; duration: 1000 }
    }

    SequentialAnimation { id: copypaste
        ScriptAction { script: { secondary.text = ""; shadowtext.text = textinputelement.selectedText; textinputelement.copy(); } }
        NumberAnimation { target: shadowrect; property: "opacity"; to: 0.5; duration: 100 }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: { secondary.paste(); } }
        NumberAnimation { target: shadowrect; property: "opacity"; to: 0; duration: 100 }
        NumberAnimation { target: shadowrect; property: "x"; to: textinputelementbackground.x; duration: 0 }
    }

    BugPanel { id: bugpanel }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }

    states: [
        State { name: "start"; when: statenum == 1
            StateChangeScript {
                script: {
                    textinputelement.text = "Hello, my name is TextInput"
                    secondary.text = "Nothing to see here";
                }
            }
            PropertyChanges { target: textinputelementtest
                testtext: "This is a TextInput element. At present it should be saying hello.\n"+
                "Next, it will select the TextInput portion of the displayed text" }
        },
        State { name: "highlight"; when: statenum == 2
            StateChangeScript { script: textinputelement.select(18, 27); }
            PropertyChanges { target: textinputelementtest
                testtext: "TextInput should now be highlighted.\nNext, copy this to the other TextInput." }
        },
        State { name: "copypaste"; when: statenum == 3
            PropertyChanges { target: copypaste; running: true }
            AnchorChanges { target: shadowrect; anchors.verticalCenter: secondarybackground.verticalCenter }
            PropertyChanges { target: textinputelementtest
                testtext: "The second TextInput should now be showing the selected text of the top TextInput.\n"+
                "Next, let's change the way the entered text is displayed." }
        },
        State { name: "passwordmode"; when: statenum == 4
            StateChangeScript { script: textinputelement.deselect(); }
            PropertyChanges { target: textinputelement; echoMode: TextInput.Password }
            PropertyChanges { target: textinputelementtest
                testtext: "The TextInput should now be showing asterisks (*) for every character in the top TextInput - all "+
                textinputelement.text.length+" of them.\n"+
                "Next, let's return to the start." }
        }
    ]


}