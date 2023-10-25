// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Shapes
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

Window {
    id: theWindow
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
            text: "DashedStroke"
            source: "DashedStroke.qml"
        }
        ListElement {
            text: "Mussel"
            source: "Mussel.qml"
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
    }
    Rectangle {
        id: solidBackground
        anchors.fill: flickable
        color: controlPanel.backgroundColor
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
                         // position in scaled path:
                         let posX = event.x - controlPanel.pathMargin
                         let posY = event.y - controlPanel.pathMargin

                         let pathX = posX / scale
                         let pathY = posY / scale

                         if (event.angleDelta.y > 0)
                             scale = scale * 1.1
                         else
                             scale = scale / 1.1
                         controlPanel.setScale(scale)

                         scale = controlPanel.scale
                         let scaledPosX = pathX * scale
                         let scaledPosY = pathY * scale

                         flickable.contentX += scaledPosX - posX
                         flickable.contentY += scaledPosY - posY
                         flickable.returnToBounds()
                     }
        }

        Loader {
            x: controlPanel.pathMargin
            y: controlPanel.pathMargin
            id: loader
        }
        MouseArea {
            id: theMouseArea
            acceptedButtons: Qt.RightButton
            anchors.fill: parent
            onMouseXChanged: {
                let p = Qt.point(Math.round(mouseX), Math.round(mouseY))
                p = mapToItem(loader.item, p)
                zoomTarget.sourceRect.x = p.x - zoomTarget.sourceRect.width/2
            }
            onMouseYChanged: {
                let p = Qt.point(Math.round(mouseX), Math.round(mouseY))
                p = mapToItem(loader.item, p)
                zoomTarget.sourceRect.y = p.y - zoomTarget.sourceRect.height/2
            }
            hoverEnabled: true
        }
        ShaderEffectSource {
            id: zoomTarget
            sourceItem: loader.item
            sourceRect.width: 16
            sourceRect.height: 16
        }
    }


    Rectangle {
        anchors.top: flickable.top
        anchors.right: flickable.right
        anchors.margins: 5
        width: 256
        height: 256
        border.color: Qt.black
        ShaderEffect {
            anchors.fill: parent
            anchors.margins: 1
            property variant src: zoomTarget
            property real textureWidth: zoomTarget.sourceRect.width
            property real textureHeight: zoomTarget.sourceRect.height
            fragmentShader: "zoombox.frag.qsb"
        }
        Button {
            id: plusButton
            text: "+"
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 2
            width: 20
            height: 20
            onPressed: {
                zoomTarget.sourceRect.width = Math.max(zoomTarget.sourceRect.width / 2, 4)
                zoomTarget.sourceRect.height = Math.max(zoomTarget.sourceRect.height / 2, 4)
            }
        }
        Button {
            id: minusButton
            text: "-"
            anchors.top: plusButton.bottom
            anchors.right: parent.right
            anchors.margins: 2
            width: 20
            height: 20
            onPressed: {
                zoomTarget.sourceRect.width = Math.max(zoomTarget.sourceRect.width * 2, 4)
                zoomTarget.sourceRect.height = Math.max(zoomTarget.sourceRect.height * 2, 4)
            }
        }
        Text {
            text: "x"+parent.width / zoomTarget.sourceRect.width
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 2
        }
    }


    ControlPanel {
        id: controlPanel
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height / 4
    }
    Settings {
        property alias currentTab: comboBox.currentIndex
        property alias windowWidth: theWindow.width
        property alias windowHeight: theWindow.height
    }
}
