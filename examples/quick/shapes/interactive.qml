/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

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
                            createStartEndHandles(this, pathSegment);
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
                        strokeColor: root.palette.windowText
                        strokeWidth: widthSlider.value
                        fillColor: fillSwitch.checked ? "green" : "transparent"
                        PathQuad {
                            id: pathSegment
                            x: quadShapePath.startx + 1
                            y: quadShapePath.startY + 1
                            controlX: quadShapePath.startX + 50
                            controlY: quadShapePath.startY + 50
                        }
                        function finishCreation() {
                            createStartEndHandles(this, pathSegment);
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
                        strokeColor: root.palette.windowText
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
                            createStartEndHandles(this, pathSegment);
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
            visible: showHandlesSwitch.checked
            color: hh.hovered  ? "yellow" : idleColor
            border.color: "grey"

            property real halfWidth: width / 2
            property bool complete: false
            Binding { target: rect.target; property: xprop; value: x + halfWidth; when: complete }
            Binding { target: rect.target; property: yprop; value: y + halfWidth; when: complete }

            DragHandler { }

            HoverHandler { id: hh }

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
