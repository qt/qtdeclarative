// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.VectorImage

ApplicationWindow {
    id: topLevel
    width: 800
    height: 800
    visible: true
    title: qsTr("Vector Image Example")
    color: "lightpink"

    property real sourceSize: Math.min(topLevel.width / 12, topLevel.height / 12)

    Component {
        id: imageComponent
//! [image]
        Image {
            sourceSize: Qt.size(topLevel.sourceSize, topLevel.sourceSize)
            source: "heart.svg"
        }
//! [image]
    }

    Component {
        id: vectorImageComponent
//! [vectorimage]
        VectorImage {
            width: topLevel.sourceSize
            height: topLevel.sourceSize
            preferredRendererType: VectorImage.CurveRenderer
            source: "heart.svg"
        }
//! [vectorimage]
    }

    Component {
        id: svgtoqmlComponent
//! [svgtoqml]
        Heart {
            width: topLevel.sourceSize
            height: topLevel.sourceSize
        }
//! [svgtoqml]
    }

    GridLayout {
        id: grid
        anchors.fill: parent
        columns: 3
        uniformCellWidths: true
        rowSpacing: 0

        Label {
            id: label
            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            text: "Image"
            color: "black"
            font.pixelSize: 20
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            text: "VectorImage"
            color: "black"
            font.pixelSize: 20
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            text: "svgtoqml"
            color: "black"
            font.pixelSize: 20
            font.bold: true
        }

        Repeater {
            id: repeater
            property int count: grid.columns
            model: grid.columns * count
            Item {
                property int margin: 10
                Layout.preferredHeight: ((grid.height - label.height - margin) / repeater.count)
                Layout.preferredWidth: ((grid.height - label.height - margin) / repeater.count)
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                clip: true

                Rectangle {
                    anchors.fill: parent
                    border.width: 1
                    color: "lightcoral"
                    radius: 10
                }

                Item {
                    property int row: index / grid.columns
                    transformOrigin: Item.Center
                    anchors.fill: parent
                    Loader {
                        id: loader
                        property int column: index % grid.columns

                        anchors.centerIn: parent
                        sourceComponent: {
                            switch (column) {
                            case 0: return imageComponent
                            case 1: return vectorImageComponent
                            case 2: return svgtoqmlComponent
                            }
                        }
                    }
                    scale: 1 + row * 1.5
                }
            }
        }
    }
}
