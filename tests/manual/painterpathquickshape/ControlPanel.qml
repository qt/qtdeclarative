// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Shapes
import QtQuick.Dialogs

Item {
    property real scale: +scaleSlider.value.toFixed(4)
    property color outlineColor: enableOutline.checked ? Qt.rgba(outlineColor.color.r, outlineColor.color.g, outlineColor.color.b, pathAlpha) : Qt.rgba(0,0,0,0)
    property color fillColor: Qt.rgba(fillColor.color.r, fillColor.color.g, fillColor.color.b, pathAlpha)
    property alias pathAlpha: alphaSlider.value
    property alias outlineWidth: outlineWidth.value
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
    property alias rendererName: rendererLabel.text
    property alias preferCurve: rendererLabel.preferCurve

    property int subShape: pickSubShape.checked ? subShapeSelector.value : -1

    property real pathMargin: marginEdit.text

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
            CheckBox { id: enableDebug }
            Label {
                text: "Debug"
                color: "white"
            }
            CheckBox { id: enableWireframe }
            Label {
                text: "Wireframe"
                color: "white"
            }
            ComboBox {
                id: painterComparison
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
            }
            Label {
                text: "Alpha"
                color: "white"
            }
            CheckBox { id: pickSubShape }
            Label {
                text: "Pick SVG sub-shape"
                color: "white"
            }
            SpinBox {
                id: subShapeSelector
                visible: pickSubShape.checked
                value: 0
                to: 999
                editable: true
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
                to: 500.0
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
                model: [ "NoGradient", "LinearGradient", "RadialGradient", "ConicalGradient" ]
            }
            Label {
                text: "Path alpha:"
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
                text: "Outline width"
                color: "white"
            }
            Slider {
                id: outlineWidth
                Layout.fillWidth: true
                from: 0.0
                to: 100.0
                value: 10.0
            }
            }
        }

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
