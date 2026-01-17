// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

pragma ComponentBehavior: Bound

Rectangle {
    id: root
    width: 1024
    height: 768

    property int requestedBackend: curveCheck.checkState === Qt.Checked ? Shape.CurveRenderer
                                                                        : Shape.UnknownRenderer

    readonly property color col: "lightsteelblue"
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: Qt.tint(root.col, "#20FFFFFF")
        }
        GradientStop {
            position: 0.1
            color: Qt.tint(root.col, "#20AAAAAA")
        }
        GradientStop {
            position: 0.9
            color: Qt.tint(root.col, "#20666666")
        }
        GradientStop {
            position: 1.0
            color: Qt.tint(root.col, "#20000000")
        }
    }

    readonly property int gridSpacing: 10

    Rectangle {
        anchors {
            fill: parent
            margins: 10
            topMargin: 40
        }
        color: "lightBlue"
        clip: true

        GridView {
            id: grid
            anchors {
                fill: parent
                margins: root.gridSpacing
            }
            cellWidth: 300
            cellHeight: 300
            delegate: Rectangle {
                id: gridDelegate

                required property string name
                required property string shapeUrl

                border.color: "purple"
                width: grid.cellWidth - root.gridSpacing
                height: grid.cellHeight - root.gridSpacing
                Column {
                    anchors.fill: parent
                    anchors.margins: 4
                    Item {
                        width: parent.width
                        height: parent.height - delegText.height
                        Loader {
                            source: Qt.resolvedUrl(gridDelegate.shapeUrl)
                            anchors.fill: parent
                        }
                    }
                    Text {
                        id: delegText
                        text: gridDelegate.name
                        font.pointSize: 16
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            model: ListModel {
                ListElement {
                    name: qsTr("Stroke and fill")
                    shapeUrl: "tapableTriangle.qml"
                }
                ListElement {
                    name: qsTr("Stroke or fill only")
                    shapeUrl: "strokeOrFill.qml"
                }
                ListElement {
                    name: qsTr("Dash pattern")
                    shapeUrl: "dashPattern.qml"
                }
                ListElement {
                    name: qsTr("Linear gradient")
                    shapeUrl: "linearGradient.qml"
                }
                ListElement {
                    name: qsTr("Radial gradient")
                    shapeUrl: "radialGradient.qml"
                }
                ListElement {
                    name: qsTr("Fill rules")
                    shapeUrl: "fillRules.qml"
                }
                ListElement {
                    name: qsTr("Join styles")
                    shapeUrl: "joinStyles.qml"
                }
                ListElement {
                    name: qsTr("Cap styles")
                    shapeUrl: "capStyles.qml"
                }
                ListElement {
                    name: qsTr("Quadratic curve")
                    shapeUrl: "quadraticCurve.qml"
                }
                ListElement {
                    name: qsTr("Cubic curve")
                    shapeUrl: "cubicCurve.qml"
                }
                ListElement {
                    name: qsTr("Elliptical arc")
                    shapeUrl: "ellipticalArcs.qml"
                }
                ListElement {
                    name: qsTr("Gradient spread modes")
                    shapeUrl: "gradientSpreadModes.qml"
                }
                ListElement {
                    name: qsTr("Arc direction")
                    shapeUrl: "arcDirection.qml"
                }
                ListElement {
                    name: qsTr("Large/small arc")
                    shapeUrl: "largeOrSmallArc.qml"
                }
                ListElement {
                    name: qsTr("Arc rotation")
                    shapeUrl: "arcRotation.qml"
                }
                ListElement {
                    name: qsTr("Tiger")
                    shapeUrl: "tigerLoader.qml"
                }
                ListElement {
                    name: qsTr("Text")
                    shapeUrl: "text.qml"
                }
                ListElement {
                    name: qsTr("Fill transform")
                    shapeUrl: "fillTransform.qml"
                }
                ListElement {
                    name: qsTr("Shape Rectangle")
                    shapeUrl: "rectangle.qml"
                }
                ListElement {
                    name: qsTr("Fill item")
                    shapeUrl: "fillItem.qml"
                }
                ListElement {
                    name: qsTr("Cosmetic stroke")
                    shapeUrl: "cosmetic.qml"
                }
            }
        }
    }

    CheckBox {
        id: curveCheck
        anchors.right: parent.right
        anchors.rightMargin: backendText.anchors.leftMargin
        anchors.top: parent.top
        anchors.margins: 10
        font.pointSize: 18
        text: "Request Curve Renderer"
    }

    Text {
        id: backendText
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: parent.top
        anchors.margins: 10
        // used only to get the renderer type
        Shape {
            id: dummyShape
            preferredRendererType: root.requestedBackend
            ShapePath { }
        }
        color: "darkBlue"
        font.pointSize: curveCheck.font.pointSize
        readonly property variant rendererStrings: [
            qsTr("Unknown"),
            qsTr("Geometry Renderer (QtGui triangulator)"),
            qsTr("GL_NV_path_rendering"),
            qsTr("Software (QPainter)"),
            qsTr("Curve Renderer")
        ]
        text: "Active Shape backend: " + rendererStrings[dummyShape.rendererType]
    }
}
