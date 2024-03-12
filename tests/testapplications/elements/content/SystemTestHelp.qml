// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id: helpbubble
    visible: false
    property bool showtext: false
    property bool showadvance: true
    onShowadvanceChanged: { advancearrow.visible = showadvance; }

    width: 300; radius: 15; color: "white"; border.color: "black"; border.width: 3
    height: showadvance ? bubbletext.height + advancearrow.height + 25 : bubbletext.height + 25
    Behavior on height {
        SequentialAnimation {
            ScriptAction { script: { bubbletext.visible = false; advancearrow.visible = false } }
            NumberAnimation { duration: 200 }
            ScriptAction { script: { bubbletext.visible = true; advancearrow.visible = showadvance && true } }
        }
    }
    Behavior on width {
        SequentialAnimation {
            ScriptAction { script: { bubbletext.visible = false; advancearrow.visible = false } }
            NumberAnimation { duration: 200 }
            ScriptAction { script: { bubbletext.visible = true; advancearrow.visible = true } }
        }
    }
    Text { id: bubbletext; width: parent.width - 30; text: testtext; wrapMode: Text.WordWrap
            anchors { top: parent.top; topMargin: 15; left: parent.left; leftMargin: 20 }
    }
    Image { id: advancearrow; source: "pics/arrow.png"; height: 30; width: 30; visible: showadvance
        anchors { right: parent.right; bottom: parent.bottom; rightMargin: 5; bottomMargin: 5 }
        MouseArea { enabled: showadvance; anchors.fill: parent; onClicked: { advance(); } }
    }
}
