// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [import]
import QtQuick
//! [import]

Rectangle {
    width: childrenRect.width
    height: childrenRect.height

    Row {
        //! [intro]
        Rectangle {
            width: 100; height: 100
            color: "green"

            MouseArea {
                anchors.fill: parent
                onClicked: { parent.color = 'red' }
            }
        }
        //! [intro]

        //! [intro-extended]
        Rectangle {
            width: 100; height: 100
            color: "green"

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: (mouse)=> {
                    if (mouse.button == Qt.RightButton)
                        parent.color = 'blue';
                    else
                        parent.color = 'red';
                }
            }
        }
        //! [intro-extended]

        //! [drag]
        Rectangle {
            id: container
            width: 600; height: 200

            Rectangle {
                id: rect
                width: 50; height: 50
                color: "red"
                opacity: (600.0 - rect.x) / 600

                MouseArea {
                    anchors.fill: parent
                    drag.target: rect
                    drag.axis: Drag.XAxis
                    drag.minimumX: 0
                    drag.maximumX: container.width - rect.width
                }
            }
        }
        //! [drag]

        //! [mousebuttons]
        Text {
            text: mouseArea.pressedButtons & Qt.RightButton ? "right" : ""
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
            }
        }
        //! [mousebuttons]

    }
}
