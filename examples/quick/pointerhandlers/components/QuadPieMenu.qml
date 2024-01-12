// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Shapes

TapHandler {
    property var labels: [ "upperLeft", "upperRight", "lowerRight", "lowerLeft" ]
    signal triggered(string text)

    id: menuTap
    acceptedButtons: Qt.RightButton
    gesturePolicy: TapHandler.DragWithinBounds
    onPressedChanged: if (pressed) {
        impl.x = point.position.x - impl.width / 2
        impl.y = point.position.y - impl.width / 2
    } else {
        if (impl.highlightedShape)
            menuTap.triggered(impl.highlightedShape.text)
    }

    property Item impl: Item {
        parent: menuTap.parent
        width: 100
        height: 100
        // with touchscreen or stylus, long-press slowly expands the menu to size
        // with mouse or touchpad right-click, it opens instantly
        scale: menuTap.point.device.pointerType === PointerDevice.Generic ?
                   1 : Math.min(1, Math.max(0, menuTap.timeHeld * 4))
        opacity: scale * 2
        visible: menuTap.pressed
        property Shape highlightedShape: null

        component PieSegment : Shape {
            id: shape
            property int orientation: Qt.TopRightCorner
            property alias text: text.text

            width: 100
            height: 100
            containsMode: Shape.FillContains

            property bool highlighted: menuTap.pressed &&
                    shape.contains(shape.mapFromItem(menuTap.parent, menuTap.point.position))
            onHighlightedChanged: {
                if (highlighted)
                    impl.highlightedShape = shape
                else if (impl.highlightedShape === shape)
                    impl.highlightedShape = null
            }

            ShapePath {
                fillColor: highlighted ? "darkturquoise" : "aliceblue"
                PathSvg {
                    id: svgPath
                    path: switch (orientation) {
                        case Qt.TopRightCorner:
                            return "M75,50 l 25,0 a50,50 0 0,0 -50,-50 l 0,25 a25,25 0 0,1 25,25";
                        case Qt.BottomRightCorner:
                            return "M75,50 l 25,0 a50,50 0 0,1 -50,50 l 0,-25 a25,25 0 0,0 25,-25";
                        case Qt.TopLeftCorner:
                            return "M50,25 l 0,-25 a50,50 0 0,0 -50,50 l 25,0 a25,25 0 0,1 25,-25";
                        case Qt.BottomLeftCorner:
                            return "M50,75 l 0,25 a50,50 0 0,1 -50,-50 l 25,0 a25,25 0 0,0 25,25";
                    }
                }
            }
            Text {
                id: text
                anchors {
                    centerIn: parent
                    horizontalCenterOffset: switch (orientation) {
                        case Qt.TopRightCorner:
                        case Qt.BottomRightCorner:
                            return 25;
                        default:
                            return -25;
                    }
                    verticalCenterOffset: switch (orientation) {
                        case Qt.BottomLeftCorner:
                        case Qt.BottomRightCorner:
                            return 25;
                        default:
                            return -25;
                    }
                }
                horizontalAlignment: Text.AlignHCenter
                rotation: switch (orientation) {
                    case Qt.TopRightCorner:
                    case Qt.BottomLeftCorner:
                        return 45;
                    default:
                        return -45;
                }
            }
        }

        PieSegment {
            orientation: Qt.TopLeftCorner
            text: labels[0]
        }
        PieSegment {
            orientation: Qt.TopRightCorner
            text: labels[1]
        }
        PieSegment {
            orientation: Qt.BottomRightCorner
            text: labels[2]
        }
        PieSegment {
            orientation: Qt.BottomLeftCorner
            text: labels[3]
        }
    }
}
