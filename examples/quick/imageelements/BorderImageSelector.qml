// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: selector

    property int curIdx: 0
    property int maxIdx: 3
    property int gridWidth: 240
    property Flickable flickable
    width: parent.width
    height: 64
    function advance(steps) {
         const nextIdx = curIdx + steps
         if (nextIdx < 0 || nextIdx > maxIdx)
            return
         flickable.contentX += gridWidth * steps
         curIdx += steps
    }
    Image {
        source: "pics/arrow.png"
        MouseArea{
            anchors.fill: parent
            onClicked: selector.advance(-1)
        }
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        opacity: selector.curIdx == 0 ? 0.2 : 1.0
        Behavior on opacity {NumberAnimation{}}
    }
    Image {
        source: "pics/arrow.png"
        mirror: true
        MouseArea{
            anchors.fill: parent
            onClicked: selector.advance(1)
        }
        opacity: selector.curIdx == selector.maxIdx ? 0.2 : 1.0
        Behavior on opacity {NumberAnimation{}}
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
    }
    Repeater {
        model: [ "Scale", "Repeat", "Scale/Repeat", "Round" ]
        delegate: Text {
            required property string modelData
            required property int index

            text: modelData
            anchors.verticalCenter: parent.verticalCenter

            x: (index - selector.curIdx) * 80 + 140
            Behavior on x { NumberAnimation{} }

            opacity: selector.curIdx == index ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation{} }
        }
    }
}
