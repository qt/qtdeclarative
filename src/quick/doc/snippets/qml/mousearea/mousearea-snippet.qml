// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
import QtQuick

//! [parent begin]
Rectangle {
//! [parent begin]
    width: 500; height: 500
    color: "green"

    Column {
        //! [anchor fill]
        Rectangle {
            id: button
            width: 100; height: 100

            MouseArea {
                anchors.fill: parent
                onClicked: console.log("button clicked")
            }
            MouseArea {
                width:150; height: 75
                onClicked: console.log("irregular area clicked")
            }
        }
        //! [anchor fill]

        Rectangle {
            width: 100; height: 100

        //! [enable handlers]
            MouseArea {
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onEntered: console.log("mouse entered the area")
                onExited: console.log("mouse left the area")
            }
        //! [enable handlers]
        }

        Rectangle {
            width: 100; height: 100

        //! [signal handlers]
            MouseArea {
                anchors.fill: parent
                onClicked: console.log("area clicked")
                onDoubleClicked: console.log("area double clicked")
                onEntered: console.log("mouse entered the area")
                onExited: console.log("mouse left the area")
            }
        //! [signal handlers]
        }

    } //end of column
//! [parent end]
}
//! [parent end]
//! [document]
