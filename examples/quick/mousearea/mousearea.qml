// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: box
    width: 320
    height: 480

    Rectangle {
        id: redSquare
        width: 120
        height: 120
        anchors {
            top: parent.top
            left: parent.left
            margins: 10
        }
        color: "red"
        opacity: redSquareMouseArea.containsPress ? 0.6 : 1.0

        Text {
            text: qsTr("Click")
            font.pixelSize: 16
            anchors.centerIn: parent
        }

        MouseArea {
            id: redSquareMouseArea
            anchors.fill: parent
            hoverEnabled: true
            property string buttonID

            acceptedButtons: Qt.AllButtons
            // Value 'All.Buttons' is eqivalent to:
            // 'Qt::LeftButton | Qt::RightButton | Qt::MiddleButton  .... | Qt::ExtraButton24'

            onEntered: info.text = qsTr('Entered')
            onExited: info.text = qsTr('Exited (pressed=') + pressed + ')'

            onPressed: (mouse) => {
                if (mouse.button == Qt.LeftButton)
                    buttonID = qsTr('LeftButton')
                else if (mouse.button == Qt.RightButton)
                    buttonID = qsTr('RightButton')
                else if (mouse.button == Qt.MiddleButton)
                    buttonID = qsTr('MiddleButton')
                else if (mouse.button == Qt.BackButton)
                    buttonID = qsTr('BackButton')
                else if (mouse.button == Qt.ForwardButton)
                    buttonID = qsTr('ForwardButton')
                else if (mouse.button == Qt.TaskButton)
                    buttonID = qsTr('TaskButton')
                else if (mouse.button == Qt.ExtraButton4)
                    buttonID = qsTr('ExtraButton4')
                else if (mouse.button == Qt.ExtraButton5)
                    buttonID = qsTr('ExtraButton5')
                else if (mouse.button == Qt.ExtraButton6)
                    buttonID = qsTr('ExtraButton6')
                else if (mouse.button == Qt.ExtraButton7)
                    buttonID = qsTr('ExtraButton7')
                else if (mouse.button == Qt.ExtraButton8)
                    buttonID = qsTr('ExtraButton8')
                else if (mouse.button == Qt.ExtraButton9)
                    buttonID = qsTr('ExtraButton9')
                else if (mouse.button == Qt.ExtraButton10)
                    buttonID = qsTr('ExtraButton10')
                else if (mouse.button == Qt.ExtraButton11)
                    buttonID = qsTr('ExtraButton11')
                else if (mouse.button == Qt.ExtraButton12)
                    buttonID = qsTr('ExtraButton12')
                else if (mouse.button == Qt.ExtraButton13)
                    buttonID = qsTr('ExtraButton13')
                else if (mouse.button == Qt.ExtraButton14)
                    buttonID = qsTr('ExtraButton14')
                else if (mouse.button == Qt.ExtraButton15)
                    buttonID = qsTr('ExtraButton15')
                else if (mouse.button == Qt.ExtraButton16)
                    buttonID = qsTr('ExtraButton16')
                else if (mouse.button == Qt.ExtraButton17)
                    buttonID = qsTr('ExtraButton17')
                else if (mouse.button == Qt.ExtraButton18)
                    buttonID = qsTr('ExtraButton18')
                else if (mouse.button == Qt.ExtraButton19)
                    buttonID = qsTr('ExtraButton19')
                else if (mouse.button == Qt.ExtraButton20)
                    buttonID = qsTr('ExtraButton20')
                else if (mouse.button == Qt.ExtraButton21)
                    buttonID = qsTr('ExtraButton21')
                else if (mouse.button == Qt.ExtraButton22)
                    buttonID = qsTr('ExtraButton22')
                else if (mouse.button == Qt.ExtraButton23)
                    buttonID = qsTr('ExtraButton23')
                else if (mouse.button == Qt.ExtraButton24)
                    buttonID = qsTr('ExtraButton24')

                info.text = qsTr('Pressed (') + buttonID + qsTr(' shift=')
                    + (mouse.modifiers & Qt.ShiftModifier ? qsTr('true') : qsTr('false')) + ')'
                const posInBox = redSquare.mapToItem(box, mouse.x, mouse.y)
                posInfo.text = + mouse.x + ',' + mouse.y + qsTr(' in square')
                    + ' (' + posInBox.x + ',' + posInBox.y + qsTr(' in window)')
            }

            onReleased: (mouse) => {
                btn.text = qsTr('Released (isClick=') + mouse.isClick + qsTr(' wasHeld=') + mouse.wasHeld + ')'
                posInfo.text = ''
            }

            //! [clicks]
            onPressAndHold: btn.text = qsTr('Press and hold')
            onClicked: (mouse) => { btn.text = qsTr('Clicked (wasHeld=') + mouse.wasHeld + ')' }
            onDoubleClicked: btn.text = qsTr('Double clicked')
            //! [clicks]
        }
    }

    Rectangle {
        id: blueSquare
        width: 120
        height: 120
        x: box.width - width - 10
        y: 10    // making this item draggable, so don't use anchors
        color: "blue"

        Text {
            text: qsTr("Drag")
            font.pixelSize: 16
            color: "white"
            anchors.centerIn: parent
        }

        MouseArea {
            anchors.fill: parent
            //! [drag]
            drag.target: blueSquare
            drag.axis: Drag.XAndYAxis
            drag.minimumX: 0
            drag.maximumX: box.width - parent.width
            drag.minimumY: 0
            drag.maximumY: box.height - parent.width
            //! [drag]
        }
    }

    Text {
        id: info
        anchors{
            bottom: btn.top
            horizontalCenter: parent.horizontalCenter
            margins: 20
        }

        onTextChanged: console.log(text)
    }

    Text {
        id: btn
        anchors {
            bottom: posInfo.top
            horizontalCenter: parent.horizontalCenter
            margins: 20
        }
    }

    Text {
        id: posInfo
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            margins: 20
        }
    }
}
