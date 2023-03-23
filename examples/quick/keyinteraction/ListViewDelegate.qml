// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: container
    required property int index

    width: ListView.view.width
    height: 60
    anchors.leftMargin: 10
    anchors.rightMargin: 10

    Rectangle {
        id: content

        anchors.centerIn: parent
        width: container.width - 40
        height: container.height - 10
        color: "transparent"
        antialiasing: true
        radius: 10

        Rectangle {
            anchors.fill: parent
            anchors.margins: 3
            color: "#91AA9D"
            antialiasing: true
            radius: 8
        }
    }

    Text {
        id: label

        anchors.centerIn: content
        text: qsTr("List element ") + (container.index + 1)
        color: "#193441"
        font.pixelSize: 14
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            container.ListView.view.currentIndex = container.index
            container.forceActiveFocus()
        }
    }

    states: State {
        name: "active"
        when: container.activeFocus
        PropertyChanges {
            content {
                color: "#FCFFF5"
                scale: 1.1
            }
            label.font.pixelSize: 16
        }
    }

    transitions: Transition {
        NumberAnimation {
            properties: "scale"
            duration: 100
        }
    }
}
