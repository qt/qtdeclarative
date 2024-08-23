// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes
import QtQuick.Dialogs
import QtCore

Item {
    property real scale: +scaleSlider.value.toFixed(4)
    property color backgroundColor: setBackground.checked ? Qt.rgba(bgColor.color.r, bgColor.color.g, bgColor.color.b, 1.0) : Qt.rgba(0,0,0,0)

    property color outlineColor: enableOutline.checked ? Qt.rgba(outlineColor.color.r, outlineColor.color.g, outlineColor.color.b, outlineAlpha) : Qt.rgba(0,0,0,0)
    property color fillColor: Qt.rgba(fillColor.color.r, fillColor.color.g, fillColor.color.b, pathAlpha)
    property alias pathAlpha: alphaSlider.value
    property alias fillRule: fillRule.currentValue
    property alias outlineAlpha: outlineAlphaSlider.value
    property real outlineWidth: cosmeticPen.checked ? outlineWidthEdit.text / scale : outlineWidthEdit.text
    property alias outlineStyle: outlineStyle.currentValue
    property alias capStyle: capStyle.currentValue
    property alias joinStyle: joinStyle.currentValue
    property alias debugCurves: enableDebug.checked
    property alias debugWireframe: enableWireframe.checked
    property alias painterComparison: painterComparison.currentIndex
    property alias painterComparisonColor: painterComparisonColor.color
    property alias painterComparisonAlpha: painterComparisonColorAlpha.value
    property alias outlineEnabled: enableOutline.checked
    property alias gradientType: gradientType.currentIndex
    property alias fillScaleX: fillTransformSlider.value
    property alias rendererName: rendererLabel.text
    property alias preferCurve: rendererLabel.preferCurve
    property alias async: enableAsync.checked

    property int subShape: pickSubShape.checked ? subShapeSelector.value : -1
    property bool subShapeGreaterThan : pickSubShapeGreaterThan.checked

    property real pathMargin: marginEdit.text

    Settings {
        property alias enableOutline: enableOutline.checked
        property alias outlineColor: outlineColor.color
        property alias outlineWidth: outlineWidthEdit.text
        property alias outlineAlpha: outlineAlphaSlider.value
        property alias outlineStyle: outlineStyle.currentIndex
        property alias joinStyle: joinStyle.currentIndex
        property alias capStyle: capStyle.currentIndex
        property alias cosmeticPen: cosmeticPen.checked

        property alias pathAlpha: alphaSlider.value
        property alias fillRule: fillRule.currentIndex
        property alias fillColor: fillColor.color

        property alias setBackground: setBackground.checked
        property alias backgroundColor: bgColor.color
    }

    function setScale(x) {
        scaleSlider.value = x
    }

    signal pathChanged
    function updatePath()
    {
        pathChanged()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        RowLayout {
            Label {
                text: "Renderer:"
                color: "white"
            }
            Label {
                id: rendererLabel
                property bool preferCurve: true
                color: "white"
                text: "Unknown"

                TapHandler {
                    onTapped: {
                        rendererLabel.preferCurve = !rendererLabel.preferCurve
                    }
                }
            }
            CheckBox {
                id: enableAsync
                Layout.fillWidth: false
            }
            Label {
                text: "Asynchronous"
                color: "white"
            }
            CheckBox {
                id: enableDebug
                Layout.fillWidth: false
            }
            Label {
                text: "Debug"
                color: "white"
            }
            CheckBox {
                id: enableWireframe
                Layout.fillWidth: false
            }
            Label {
                text: "Wireframe"
                color: "white"
            }
            ComboBox {
                id: painterComparison
                Layout.fillWidth: false
                model: [
                    "No QPainter comparison",
                    "Overlaid QPainter comparison",
                    "Underlaid QPainter comparison"
                ]
            }
            Rectangle {
                id: painterComparisonColor
                color: "red"
                width: 20
                height: 20
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        painterComparisonColorDialog.open()
                    }
                }
            }
            Slider {
                id: painterComparisonColorAlpha
                from: 0.0
                to: 1.0
                value: 1.0
                Layout.fillWidth: false
            }
            Label {
                text: "Alpha"
                color: "white"
            }
            CheckBox {
                text: "Pick SVG sub-shape"
                id: pickSubShape
                palette.windowText: "white"
                Layout.fillWidth: false
            }
            SpinBox {
                id: subShapeSelector
                visible: pickSubShape.checked
                value: 0
                to: 999
                editable: true
                Layout.fillWidth: false
            }
            CheckBox {
                id: pickSubShapeGreaterThan
                visible: pickSubShape.checked
                text: "show greater than"
                palette.windowText: "white"
                Layout.fillWidth: false
            }
            CheckBox {
                id: setBackground
                text: "Solid background"
                palette.windowText: "white"
                Layout.fillWidth: false
            }
            RowLayout {
                visible: setBackground.checked
                Rectangle {
                    id: bgColor
                    property color selectedColor: "#a9a9a9"
                    color: selectedColor
                    border.color: "black"
                    border.width: 2
                    width: 21
                    height: 21
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            bgColorDialog.open()
                        }
                    }
                }
            }
        }
        RowLayout {
            Label {
                text: "Margin:"
                color: "white"
            }
            TextField {
                id: marginEdit
                text: "150"
                validator: DoubleValidator{ bottom: 0.0 }
            }
            Label {
                text: "Scale:"
                color: "white"
            }
            TextField {
                id: scaleEdit
                text: scaleSlider.value.toFixed(4)
                Layout.fillWidth: false
                onEditingFinished: {
                    let val = +text
                    if (val > 0)
                        scaleSlider.value = val
                }
            }
            Slider {
                id: scaleSlider
                Layout.fillWidth: true
                from: 0.01
                to: 50.0
                value: 0.2
                onValueChanged: scaleEdit.text = value.toFixed(4)
            }
        }
        RowLayout {
            Label {
                text: "Fill color"
                color: "white"
            }
            Rectangle {
                id: fillColor
                color: "#ffffff"
                width: 20
                height: 20
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        fillColorDialog.open()
                    }
                }
            }
            ComboBox {
                id: gradientType
                Layout.fillWidth: false
                model: [ "NoGradient", "LinearGradient", "RadialGradient", "ConicalGradient" ]
            }
            Label {
                text: "Fill rule:"
                color: "white"
            }
            ComboBox {
                id: fillRule
                textRole: "text"
                valueRole: "style"
                Layout.fillWidth: false
                model: ListModel {
                    ListElement {
                        text: "WindingFill"
                        style: ShapePath.WindingFill
                    }
                    ListElement {
                        text: "OddEvenFill"
                        style: ShapePath.OddEvenFill
                    }
                }
            }
            Label {
                text: "Fill transform (scale x: " + fillTransformSlider.value.toFixed(2) + "):"
                color: "white"
                visible: gradientType.currentIndex != 0
            }
            Slider {
                id: fillTransformSlider
                Layout.fillWidth: true
                from: 0.2
                to: 5.0
                value: 1.0
                visible: gradientType.currentIndex != 0
            }
            Label {
                text: "Fill alpha(" + Math.round(alphaSlider.value*100)/100 + "):"
                color: "white"
            }
            Slider {
                id: alphaSlider
                Layout.fillWidth: true
                from: 0.0
                to: 1.0
                value: 1.0
            }
        }
        RowLayout {
            CheckBox {
                id: enableOutline
                text: "Enable outline"
                palette.windowText: "white"
                Layout.fillWidth: false
            }
            RowLayout {
                opacity: enableOutline.checked ? 1 : 0
            Label {
                text: "Outline color:"
                color: "white"
            }
            Rectangle {
                id: outlineColor
                property color selectedColor: "#33ccbb"
                color: selectedColor
                width: 20
                height: 20
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        outlineColorDialog.open()
                    }
                }
            }
            ComboBox {
                id: outlineStyle
                textRole: "text"
                valueRole: "style"
                Layout.fillWidth: false
                model: ListModel {
                    ListElement {
                        text: "Solid line"
                        style: ShapePath.SolidLine
                    }
                    ListElement {
                        text: "Dash line"
                        style: ShapePath.DashLine
                    }
                }
            }
            ComboBox {
                id: joinStyle
                textRole: "text"
                valueRole: "style"
                Layout.fillWidth: false
                model: ListModel {
                    ListElement {
                        text: "Miter join"
                        style: ShapePath.MiterJoin
                    }
                    ListElement {
                        text: "Bevel join"
                        style: ShapePath.BevelJoin
                    }
                    ListElement {
                        text: "Round join"
                        style: ShapePath.RoundJoin
                    }
                }
            }
            ComboBox {
                id: capStyle
                textRole: "text"
                valueRole: "style"
                Layout.fillWidth: false
                model: ListModel {
                    ListElement {
                        text: "Square cap"
                        style: ShapePath.SquareCap
                    }
                    ListElement {
                        text: "Round cap"
                        style: ShapePath.RoundCap
                    }
                    ListElement {
                        text: "Flat cap"
                        style: ShapePath.FlatCap
                    }
                }
            }
            Label {
                text: "Outline width:"
                color: "white"
            }
            TextField {
                id: outlineWidthEdit
                text: (cosmeticPen.checked ? outlineWidthSlider.value: outlineWidthSlider.value ** 2).toFixed(2)
                Layout.fillWidth: false
                onEditingFinished: {
                    let val = +text
                    if (val > 0) {
                        if (cosmeticPen.checked)
                            outlineWidth.value = val * scale
                        else
                            outlineWidth.value = Math.sqrt(val)
                    }
                }
            }
            Slider {
                id: outlineWidthSlider
                Layout.fillWidth: true
                from: 0.0
                to: 10.0
                value: Math.sqrt(10)
            }
            CheckBox {
                id: cosmeticPen
                text: "Cosmetic pen"
                palette.windowText: "white"
                Layout.fillWidth: false
            }
            Label {
                text: "Outline alpha (" + Math.round(outlineAlphaSlider.value*100)/100 + "):"
                color: "white"
            }
            Slider {
                id: outlineAlphaSlider
                Layout.fillWidth: true
                from: 0.0
                to: 1.0
                value: 1.0
            }
            }
        }

    }
    ColorDialog {
        id: bgColorDialog
        selectedColor: bgColor.selectedColor
        onAccepted: bgColor.selectedColor = selectedColor
    }
    ColorDialog {
        id: outlineColorDialog
        selectedColor: outlineColor.selectedColor
        onAccepted: outlineColor.selectedColor = selectedColor
    }
    ColorDialog {
        id: fillColorDialog
        selectedColor: fillColor.color
        onAccepted: fillColor.color = selectedColor
    }
    ColorDialog {
        id: painterComparisonColorDialog
        selectedColor: painterComparisonColor.color
        onAccepted: painterComparisonColor.color = selectedColor
    }
}
