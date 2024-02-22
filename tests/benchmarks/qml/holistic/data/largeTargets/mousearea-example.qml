// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    id: box
    width: 350; height: 250

    Rectangle {
        id: redSquare
        width: 80; height: 80
        anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 10
        color: "red"

        Text { text: "Click"; font.pixelSize: 16; anchors.centerIn: parent }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            onEntered: info.text = 'Entered'
            onExited: info.text = 'Exited (pressed=' + pressed + ')'

            onPressed: {
                info.text = 'Pressed (button=' + (mouse.button == Qt.RightButton ? 'right' : 'left')
                    + ' shift=' + (mouse.modifiers & Qt.ShiftModifier ? 'true' : 'false') + ')'
                var posInBox = redSquare.mapToItem(box, mouse.x, mouse.y)
                posInfo.text = + mouse.x + ',' + mouse.y + ' in square'
                        + ' (' + posInBox.x + ',' + posInBox.y + ' in window)'
            }

            onReleased: {
                info.text = 'Released (isClick=' + mouse.isClick + ' wasHeld=' + mouse.wasHeld + ')'
                posInfo.text = ''
            }

            onPressAndHold: info.text = 'Press and hold'
            onClicked: info.text = 'Clicked (wasHeld=' + mouse.wasHeld + ')'
            onDoubleClicked: info.text = 'Double clicked'
        }
    }

    Rectangle {
        id: blueSquare
        width: 80; height: 80
        x: box.width - width - 10; y: 10    // making this item draggable, so don't use anchors
        color: "blue"

        Text { text: "Drag"; font.pixelSize: 16; color: "white"; anchors.centerIn: parent }

        MouseArea {
            anchors.fill: parent
            drag.target: blueSquare
            drag.axis: Drag.XAndYAxis
            drag.minimumX: 0
            drag.maximumX: box.width - parent.width
            drag.minimumY: 0
            drag.maximumY: box.height - parent.width
        }
    }

    Text {
        id: info
        anchors.bottom: posInfo.top; anchors.horizontalCenter: parent.horizontalCenter; anchors.margins: 30

        onTextChanged: console.log(text)
    }

    Text {
        id: posInfo
        anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; anchors.margins: 30
    }
}
