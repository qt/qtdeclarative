// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![import]
import QtQuick
//![import]

Column {

//![0]
Item {
    id: container
    width: 300; height: 300

    Rectangle {
        id: rect
        width: 100; height: 100
        color: "red"

        MouseArea {
           id: mouseArea
           anchors.fill: parent
        }

        states: State {
           name: "resized"; when: mouseArea.pressed
           PropertyChanges {
               rect {
                   color: "blue"
                   height: container.height
               }
           }
        }
    }
}
//![0]

//![reset]
Rectangle {
    width: 300; height: 200

    Text {
        id: myText
        width: 50
        wrapMode: Text.WordWrap
        text: "a text string that is longer than 50 pixels"

        states: State {
            name: "widerText"
            PropertyChanges { myText.width: undefined }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: myText.state = "widerText"
    }
}
//![reset]
}
