// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

Item {
    id: rootItem

    property real showAnimationProgress: 0
    property int itemWidth: 256 * dp
    property int lineSpacing1: 0
    property int lineWidth1: 20 * dp
    property int lineSpacing2: 10 * dp
    property int lineWidth2: 10 * dp

    readonly property int cx: itemWidth / 2
    readonly property int cy: itemWidth / 2

    signal animationFinished

    function startShow() {
        hideAnimationItem.stop();
        showAnimationItem.restart();
    }
    function stopShow() {
        showAnimationItem.stop();
        hideAnimationItem.restart();
    }

    function ringProgress() {
        return showAnimationProgress * (2 * Math.PI) - (Math.PI / 2);
    }

    function useLargeArcFunc() {
        return (ringProgress() > Math.PI / 2);
    }

    anchors.fill: parent
    visible: opacity
    opacity: 0

    SequentialAnimation {
        id: showAnimationItem
        PauseAnimation {
            duration: 400
        }
        ScriptAction {
            script: {
                rootItem.showAnimationProgress = 0;
            }
        }
        NumberAnimation {
            target: rootItem
            property: "opacity"
            to: 1
            duration: 400
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            target: rootItem
            property: "showAnimationProgress"
            to: 1
            duration: 2000
            easing.type: Easing.InOutQuad
        }
        ScriptAction {
            script: rootItem.animationFinished();
        }
    }
    SequentialAnimation {
        id: hideAnimationItem
        NumberAnimation {
            target: rootItem
            property: "opacity"
            to: 0
            duration: 400
            easing.type: Easing.InOutQuad
        }
        ScriptAction {
            script: {
                rootItem.showAnimationProgress = 0;
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.6
    }

    Shape {
        id: shapeItem
        x: (parent.width / 2) - (itemWidth / 2)
        y: (parent.height / 2) - (itemWidth / 2)
        opacity: rootItem.showAnimationProgress
        width: itemWidth
        height: itemWidth
        rotation: rootItem.showAnimationProgress * 360
        ShapePath {
            strokeColor: "#606060"
            fillColor:  "transparent"
            strokeWidth: lineWidth1
            capStyle: ShapePath.RoundCap
            PathMove {
                x: itemWidth / 2
                y: pathArc1.spacing
            }
            PathArc {
                id: pathArc1
                property real spacing: lineSpacing1 + lineWidth1
                x: cx + ((itemWidth / 2 - spacing) * Math.cos(ringProgress()))
                y: cy + ((itemWidth / 2 - spacing) * Math.sin(ringProgress()))
                radiusX: itemWidth / 2 - spacing
                radiusY: itemWidth / 2 - spacing
                useLargeArc: useLargeArcFunc()
            }
        }

        ShapePath {
            strokeColor: "#808080"
            fillColor: "transparent"
            strokeWidth: lineWidth2
            capStyle: ShapePath.RoundCap
            PathMove {
                x: itemWidth / 2
                y: pathArc2.spacing
            }
            PathArc {
                id: pathArc2
                property real spacing: lineSpacing2 + lineWidth2
                x: cx + ((itemWidth / 2 - spacing) * Math.cos(ringProgress()))
                y: cy + ((itemWidth / 2 - spacing) * Math.sin(ringProgress()))
                radiusX: itemWidth / 2 - spacing
                radiusY: itemWidth / 2 - spacing
                useLargeArc: useLargeArcFunc()
            }
        }
    }
    Text {
        id: textItem
        anchors.centerIn: shapeItem
        font.pixelSize: 32 * dp
        color: "#d0d0d0"
        text: showAnimationProgress < 1 ? qsTr("Reset to default settings?") : qsTr("Reseted!")
        Behavior on text {
            SequentialAnimation {
                NumberAnimation {
                    target: textItem
                    properties: "scale, opacity"
                    to: 0
                    duration: 200
                    easing.type: Easing.InOutQuad
                }
                PropertyAction {
                    target: textItem
                    property: "text"
                }
                NumberAnimation {
                    target: textItem
                    properties: "scale, opacity"
                    to: 1.0
                    duration: 200
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }
}
