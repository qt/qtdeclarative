// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQuick

Item {
    id: page

    property real effectiveOpacity: 1.0
    property real ratio: width / 320 < height / 440 ? width / 320 : height / 440
    property int smallSize: 45 * ratio
    property int bigSize: 2 * smallSize
    property int elementSpacing: 0.14 * smallSize

    anchors.fill: parent

    Timer {
        interval: 2000
        running: true
        repeat: true
        onTriggered: page.effectiveOpacity = (page.effectiveOpacity == 1.0 ? 0.0 : 1.0)
    }

    Column {
        anchors {
            left: parent.left
            leftMargin: page.width / 32
            top: parent.top
            topMargin: page.height / 48
        }
        spacing: page.elementSpacing

        populate: Transition {
            NumberAnimation {
                properties: "x,y"
                from: 200
                duration: 100
                easing.type: Easing.OutBounce
            }
        }
        add: Transition {
            NumberAnimation {
                properties: "y"
                easing.type: Easing.OutQuad
            }
        }
        move: Transition {
            NumberAnimation {
                properties: "y"
                easing.type: Easing.OutBounce
            }
        }

        Rectangle {
            color: "#80c342"
            width: page.bigSize
            height: page.smallSize
        }

        Rectangle {
            id: greenV1

            visible: opacity != 0
            width: page.bigSize
            height: page.smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: page.effectiveOpacity
        }

        Rectangle {
            color: "#14aaff"
            width: page.bigSize
            height: page.smallSize
        }

        Rectangle {
            id: greenV2

            visible: opacity != 0
            width: page.bigSize
            height: page.smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: page.effectiveOpacity
        }

        Rectangle {
            color: "#6400aa"
            width: page.bigSize
            height: page.smallSize
        }

        Rectangle {
            color: "#80c342"
            width: page.bigSize
            height: page.smallSize
        }
    }

    Row {
        anchors {
            left: page.left
            leftMargin: page.width / 32
            bottom: page.bottom
            bottomMargin: page.height / 48
        }
        spacing: page.elementSpacing

        populate: Transition {
            NumberAnimation {
                properties: "x,y"
                from: 200
                duration: 100
                easing.type: Easing.OutBounce
            }
        }
        add: Transition {
            NumberAnimation {
                properties: "x"
                easing.type: Easing.OutQuad
            }
        }
        move: Transition {
            NumberAnimation {
                properties: "x"
                easing.type: Easing.OutBounce
            }
        }

        Rectangle {
            color: "#80c342"
            width: page.smallSize
            height: page.bigSize
        }

        Rectangle {
            id: blueH1

            visible: opacity != 0
            width: page.smallSize
            height: page.bigSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: page.effectiveOpacity
        }

        Rectangle {
            color: "#14aaff"
            width: page.smallSize
            height: page.bigSize
        }

        Rectangle {
            id: greenH2

            visible: opacity != 0
            width: page.smallSize
            height: page.bigSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: page.effectiveOpacity
        }

        Rectangle {
            color: "#6400aa"
            width: page.smallSize
            height: page.bigSize
        }

        Rectangle {
            color: "#80c342"
            width: page.smallSize
            height: page.bigSize
        }
    }

    Grid {
        anchors.top: parent.top
        anchors.topMargin: page.height / 48
        anchors.left: flowItem.left
        columns: 3
        spacing: page.elementSpacing

        populate: Transition {
            NumberAnimation {
                properties: "x,y"
                from: 200
                duration: 100
                easing.type: Easing.OutBounce
            }
        }
        add: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutBounce
            }
        }
        move: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutBounce
            }
        }

        Rectangle {
            color: "#80c342"
            width: page.smallSize
            height: page.smallSize
        }

        Rectangle {
            id: greenG1

            visible: opacity != 0
            width: page.smallSize
            height: page.smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: page.effectiveOpacity
        }

        Rectangle {
            color: "#14aaff"
            width: page.smallSize
            height: page.smallSize
        }

        Rectangle {
            id: greenG2

            visible: opacity != 0
            width: page.smallSize
            height:page. smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: page.effectiveOpacity
        }

        Rectangle {
            color: "#6400aa"
            width: page.smallSize
            height: page.smallSize
        }

        Rectangle {
            id: greenG3

            visible: opacity != 0
            width: page.smallSize
            height: page.smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: page.effectiveOpacity
        }

        Rectangle {
            color: "#80c342"
            width: page.smallSize
            height: page.smallSize
        }

        Rectangle {
            color: "#14aaff"
            width: page.smallSize
            height: page.smallSize
        }

        Rectangle {
            color: "#6400aa"
            width: page.smallSize
            height: page.smallSize
        }
    }

    Flow {
        id: flowItem

        anchors.right: page.right
        anchors.rightMargin: page.width / 32
        y: 2 * page.bigSize
        width: 1.8 * page.bigSize
        spacing: page.elementSpacing

        //! [move]
        move: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutBounce
            }
        }
        //! [move]

        //! [add]
        add: Transition {
            NumberAnimation {
                properties: "x,y"
                easing.type: Easing.OutBounce
            }
        }
        //! [add]

        //! [populate]
        populate: Transition {
            NumberAnimation {
                properties: "x,y"
                from: 200
                duration: 100
                easing.type: Easing.OutBounce
            }
        }
        //! [populate]

        Rectangle {
            color: "#80c342"
            width: page.smallSize
            height: page.smallSize
        }

        Rectangle {
            id: greenF1

            visible: opacity != 0
            width: 0.6 * page.bigSize
            height: page.smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: page.effectiveOpacity
        }

        Rectangle {
            color: "#14aaff"
            width: 0.3 * page.bigSize
            height: page.smallSize
        }

        Rectangle {
            id: greenF2

            visible: opacity != 0
            width: 0.6 * page.bigSize
            height: page.smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: page.effectiveOpacity
        }

        Rectangle {
            color: "#6400aa"
            width: page.smallSize
            height: page.smallSize
        }

        Rectangle {
            id: greenF3

            visible: opacity != 0
            width: 0.4 * page.bigSize
            height: page.smallSize
            color: "#006325"
            border.color: "transparent"
            Behavior on opacity { NumberAnimation {} }
            opacity: page.effectiveOpacity
        }

        Rectangle {
            color: "#80c342"
            width: 0.8 * page.bigSize
            height: page.smallSize
        }
    }
}
