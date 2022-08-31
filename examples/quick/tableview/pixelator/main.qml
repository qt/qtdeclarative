// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

import QtQuick.Window

import ImageModel

Window {
    visible: true
    width: 900
    height: 900
    title: qsTr("TableView Pixelator")

    //! [pixelcomponent]
    Component {
        id: pixelDelegate

        Item {
            required property real display

            readonly property real gray: display / 255.0
            readonly property real size: 16

            implicitWidth: size
            implicitHeight: size
            //! [pixelcomponent]

            //! [rectshape]
            Rectangle {
                id: rect
                anchors.centerIn: parent
                color: "#09102b"
                radius: parent.size - parent.gray * parent.size
                implicitWidth: radius
                implicitHeight: radius
                //! [rectshape]

                //! [animation]
                ColorAnimation on color {
                    id: colorAnimation
                    running: false
                    to: "#41cd52"
                    duration: 1500
                }
                //! [animation]
            }

            //! [interaction]
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: rect.color = "#cecfd5"
                onExited: colorAnimation.start()
            }
            //! [interaction]
        }
    }

    //! [tableview]
    TableView {
        id: tableView
        anchors.fill: parent
        model: ImageModel {
            source: ":/qt.png"
        }

        delegate: pixelDelegate
    }
    //! [tableview]
}
