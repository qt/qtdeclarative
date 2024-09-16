// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

Window {
    id: theWindow
    width: 1024
    height: 768
    visible: true
    title: qsTr("Text Rendering")
    color: "white"

    Text {
        id: dummyTextRendering
        scale: scaleSlider.value
        anchors.centerIn: parent
        text: dummyText.text
        font.pixelSize: fontSize.value
        renderType: renderTypeCombo.currentIndex
        style: styleCombo.currentIndex
        styleColor: "indianred"
        color: "black"
        visible: renderTypeCombo.currentIndex <= 2
    }

    Shape {
        id: dummyShapeRendering
        anchors.centerIn: parent
        scale: scaleSlider.value
        visible: !dummyTextRendering.visible
        width: boundingRect.width
        height: boundingRect.height
        preferredRendererType: shapesRendererCombo.currentIndex === 0 ? Shape.GeometryRenderer : Shape.CurveRenderer

        ShapePath {
            id: shapePath
            fillColor: "black"
            strokeColor: styleCombo.currentIndex === 1 ? "indianred" : "transparent"
            strokeStyle: ShapePath.SolidLine
            strokeWidth: 1
            fillRule: ShapePath.WindingFill
            PathText {
                text: dummyText.text
                font.pixelSize: fontSize.value
            }
        }
    }

    RowLayout {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        TextField {
            id: dummyText
            text: "Foobar"
            Layout.fillWidth: false
        }

        Label {
            text: "Scale:"
        }

        Slider {
            id: scaleSlider
            from: 0.5
            to: 10
            value: 1
            Layout.fillWidth: false
        }

        Label {
            text: "Font size:"
        }

        Slider {
            id: fontSize
            from: 1
            to: 1000
            value: 100
            Layout.fillWidth: true
        }

        ComboBox {
            id: styleCombo
            Layout.fillWidth: false
            model: [ "Normal", "Outline", "Raised", "Sunken" ]
        }

        ComboBox {
            id: renderTypeCombo
            Layout.fillWidth: false
            model: [ "QtRendering", "NativeRendering", "CurveRendering", "Qt Quick Shapes" ]
        }

        ComboBox {
            id: shapesRendererCombo
            Layout.fillWidth: false
            model: [ "GeometryRenderer", "CurveRenderer" ]
            visible: renderTypeCombo.currentIndex > 2
        }
    }
}
