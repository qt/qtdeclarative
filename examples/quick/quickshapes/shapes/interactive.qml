// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

pragma ComponentBehavior: Bound

Rectangle {
    id: root
    width: 1024
    height: 768
    color: palette.window

    RowLayout {
        id: topRow
        x: 20
        y: 10
        spacing: 20

        ButtonGroup {
            id: toolButtons
            buttons: drawingTools.children
        }

        RowLayout {
            id: drawingTools
            ToolButton {
                text: qsTr("Line")
                checkable: true
                checked: true

                property Component shapeType: Component {
                    ShapePath {
                        id: lineShapePath
                        strokeColor: root.palette.windowText
                        strokeWidth: widthSlider.value
                        fillColor: "transparent"
                        PathLine {
                            id: pathSegment
                            x: lineShapePath.startX + 1
                            y: lineShapePath.startY + 1
                        }
                        function finishCreation() {
                            root.createStartEndHandles(this, pathSegment);
                        }
                    }
                }
            }
            ToolButton {
                text: qsTr("Quadratic")
                checkable: true

                property Component shapeType: Component {
                    ShapePath {
                        id: quadShapePath
                        strokeColor: strokeSwitch.checked ? root.palette.windowText : "transparent"
                        strokeWidth: widthSlider.value
                        fillColor: fillSwitch.checked ? "green" : "transparent"
                        PathQuad {
                            id: pathSegment
                            x: quadShapePath.startX + 1
                            y: quadShapePath.startY + 1
                            controlX: quadShapePath.startX + 50
                            controlY: quadShapePath.startY + 50
                        }
                        function finishCreation() {
                            root.createStartEndHandles(this, pathSegment);
                            pointDragHandle.createObject(canvas, {
                                idleColor: "blue",
                                target: pathSegment, xprop: "controlX", yprop: "controlY"
                            });
                        }
                    }
                }
            }
            ToolButton {
                text: qsTr("Cubic")
                checkable: true

                property Component shapeType: Component {
                    ShapePath {
                        id: cubicShapePath
                        strokeColor: strokeSwitch.checked ? root.palette.windowText : "transparent"
                        strokeWidth: widthSlider.value
                        fillColor: fillSwitch.checked ? "green" : "transparent"
                        PathCubic {
                            id: pathSegment
                            x: cubicShapePath.startX + 1
                            y: cubicShapePath.startY + 1
                            control1X: cubicShapePath.startX + 50;
                            control1Y: cubicShapePath.startY + 50;
                            control2X: cubicShapePath.startX + 150;
                            control2Y: cubicShapePath.startY + 50;
                        }
                        function finishCreation() {
                            root.createStartEndHandles(this, pathSegment);
                            pointDragHandle.createObject(canvas, {
                                idleColor: "blue",
                                target: pathSegment, xprop: "control1X", yprop: "control1Y"
                            });
                            pointDragHandle.createObject(canvas, {
                                idleColor: "lightBlue",
                                target: pathSegment, xprop: "control2X", yprop: "control2Y"
                            });
                        }
                    }
                }
            }
            ToolButton {
                id: modifyButton
                text: qsTr("Modify")
                checkable: true
                onCheckedChanged: {
                    if (checked)
                        showHandlesSwitch.checked = true;
                }
            }
        }

        Label {
            text: qsTr("Width")
        }
        Slider {
            id: widthSlider
            from: 1
            to: 60
            value: 4
        }

        Switch {
            id: showHandlesSwitch
            text: qsTr("Handles")
        }

        Switch {
            id: fillSwitch
            text: qsTr("Fill")
        }

        Switch {
            id: strokeSwitch
            text: qsTr("Stroke")
            checked: true
        }
    }

    Component {
        id: pointDragHandle

        Rectangle {
            id: rect
            property variant target
            property string xprop
            property string yprop
            property color idleColor: "red"

            width: 20
            height: width
            radius: halfWidth
            visible: showHandlesSwitch.checked
            color: hh.hovered  ? "yellow" : idleColor
            border.color: "grey"
            opacity: 0.75

            property real halfWidth: width / 2
            property bool complete: false
            Binding {
                target: rect.target
                property: rect.xprop
                value: rect.x + rect.halfWidth
                when: rect.complete
            }
            Binding {
                target: rect.target
                property: rect.yprop
                value: rect.y + rect.halfWidth
                when: rect.complete
            }

            DragHandler { }

            HoverHandler {
                id: hh
            }

            Component.onCompleted: {
                x = target[xprop] - halfWidth;
                y = target[yprop] - halfWidth;
                complete = true;
            }
        }
    }

    function createStartEndHandles(shapePath, pathSegment) {
        pointDragHandle.createObject(canvas, {
            idleColor: "red",
            target: shapePath, xprop: "startX", yprop: "startY"
        });
        pointDragHandle.createObject(canvas, {
            idleColor: "red",
            target: pathSegment, xprop: "x", yprop: "y"
        });
    }

    Rectangle {
        id: canvas
        color: palette.base
        width: root.width - 40
        height: root.height - y - 20
        x: 20
        anchors.top: topRow.bottom
        anchors.topMargin: 20

        DragHandler {
            target: null
            grabPermissions: DragHandler.TakeOverForbidden
            property ShapePath activePath: null
            onActiveChanged: {
                const tool = toolButtons.checkedButton;
                if (tool != modifyButton) {
                    if (active) {
                        activePath = tool.shapeType.createObject(root, {
                            startX: centroid.position.x, startY: centroid.position.y
                        });
                        shape.data.push(activePath);
                    } else {
                        activePath.finishCreation();
                        activePath = null;
                    }
                }
            }
            onCentroidChanged: if (activePath) {
                var pathObj = activePath.pathElements[0];
                pathObj.x = centroid.position.x;
                pathObj.y = centroid.position.y;
            }
        }

        Shape {
            id: shape
            anchors.fill: parent
        }
    }
}
