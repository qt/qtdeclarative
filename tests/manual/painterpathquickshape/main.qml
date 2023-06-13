// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Shapes
import QtQuick.Controls
import QtQuick.Layouts

Window {
    width: 1024
    height: 768
    visible: true
    title: qsTr("Hello World")
    color: "#666"

    ListModel {
        id: sampleList

        ListElement {
            text: "Small polygon"
            source: "SmallPolygon.qml"
        }
        ListElement {
            text: "Text"
            source: "TextShape.qml"
        }
        ListElement {
            text: "SVG"
            source: "SvgShape.qml"
        }
        ListElement {
            text: "QuadShape"
            source: "SimpleShape.qml"
        }
        ListElement {
            text: "Squircle"
            source: "Squircle.qml"
        }
        ListElement {
            text: "CubicShape"
            source: "CubicShape.qml"
        }
        ListElement {
            text: "Arc Direction"
            source: "arcDirection.qml"
        }

        ListElement {
            text: "Arc Rotation"
            source: "arcRotation.qml"
        }

        ListElement {
            text: "Cap Styles"
            source: "capStyles.qml"
        }

        ListElement {
            text: "Cubic Curve"
            source: "cubicCurve.qml"
        }

        ListElement {
            text: "Dash Pattern"
            source: "dashPattern.qml"
        }

        ListElement {
            text: "Elliptical Arcs"
            source: "ellipticalArcs.qml"
        }

        ListElement {
            text: "Fill rules"
            source: "fillRules.qml"
        }

        ListElement {
            text: "Gradient spread modes"
            source: "gradientSpreadModes.qml"
        }

        ListElement {
            text: "Join styles"
            source: "joinStyles.qml"
        }

        ListElement {
            text: "Large or small arc"
            source: "largeOrSmallArc.qml"
        }

        ListElement {
            text: "Linear gradient"
            source: "linearGradient.qml"
        }

        ListElement {
            text: "Quadratic curve"
            source: "quadraticCurve.qml"
        }

        ListElement {
            text: "Radial gradient"
            source: "radialGradient.qml"
        }

        ListElement {
            text: "Stroke or fill"
            source: "strokeOrFill.qml"
        }

        ListElement {
            text: "Qt! text"
            source: "text.qml"
        }

        ListElement {
            text: "Tiger"
            source: "tiger.qml"
        }
    }

    ComboBox {
        id: comboBox
        model: sampleList
        textRole: "text"
        valueRole: "source"
        onCurrentValueChanged: {
            loader.source = currentValue
        }
    }
    Image {
        id: background
        anchors.fill: flickable
        fillMode: Image.Tile
        source: "qrc:/background.png"
        smooth: true
    }

    Flickable {
        id: flickable
        clip: true
        contentWidth: loader.item ? loader.item.boundingRect.right * controlPanel.scale + controlPanel.pathMargin * 2 : 1
        contentHeight: loader.item ? loader.item.boundingRect.bottom * controlPanel.scale + controlPanel.pathMargin * 2 : 1
        anchors.top: comboBox.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: controlPanel.top

        WheelHandler {
            onWheel: (event)=> {
                         let scale = controlPanel.scale
                         let posX = event.x
                         let posY = event.y
                         let xOff = posX - flickable.contentX
                         let yOff = posY - flickable.contentY

                         let pathX = posX / scale
                         let pathY = posY / scale

                         if (event.angleDelta.y > 0)
                             scale = scale * 1.1
                         else
                             scale = scale / 1.1
                         controlPanel.setScale(scale)

                         flickable.contentX = pathX * controlPanel.scale - xOff
                         flickable.contentY = pathY * controlPanel.scale - yOff
                         flickable.returnToBounds()
                     }
        }

        Loader {
            x: controlPanel.pathMargin
            y: controlPanel.pathMargin
            id: loader
        }
    }

    ControlPanel {
        id: controlPanel
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height / 4
    }
}
