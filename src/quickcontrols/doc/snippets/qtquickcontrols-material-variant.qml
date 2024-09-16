// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

/*
    This file is used by tst_snippets to generate qtquickcontrols-material-variant-normal.png:

    SCREENSHOTS=1 ./tst_snippets verify:qtquickcontrols-material-variant

    and qtquickcontrols-material-variant-dense.png:

    SCREENSHOTS=1 QT_QUICK_CONTROLS_MATERIAL_VARIANT=Dense ./tst_snippets verify:qtquickcontrols-material-variant
*/

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Pane {
    id: root
    implicitWidth: 400
    implicitHeight: 600
    padding: 10

    readonly property color measurementColor: "darkorange"
    readonly property int barLeftMargin: 10
    readonly property int textTopMargin: 12

    Component {
        id: measurementComponent

        Rectangle {
            color: root.measurementColor
            width:  1
            height: parent.height

            Rectangle {
                width: 5
                height: 1
                color: root.measurementColor
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Rectangle {
                width: 5
                height: 1
                color: root.measurementColor
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
            }

            Text {
                x: 8
                text: parent.height
                height: parent.height
                color: root.measurementColor
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 20

        ColumnLayout {
            spacing: root.textTopMargin

            Button {
                id: button
                text: qsTr("Button")

                Loader {
                    sourceComponent: measurementComponent
                    height: parent.height
                    anchors.left: parent.right
                    anchors.leftMargin: root.barLeftMargin
                }

            }
            Text {
                text: "Roboto " + button.font.pixelSize
                color: root.measurementColor
            }
        }

        ColumnLayout {
            spacing: root.textTopMargin

            ItemDelegate {
                id: itemDelegate
                text: qsTr("ItemDelegate")

                Loader {
                    sourceComponent: measurementComponent
                    height: parent.height
                    anchors.left: parent.right
                    anchors.leftMargin: root.barLeftMargin
                }

            }
            Text {
                text: "Roboto " + itemDelegate.font.pixelSize
                color: root.measurementColor
            }
        }

        ColumnLayout {
            spacing: root.textTopMargin

            CheckDelegate {
                id: checkDelegate
                text: qsTr("CheckDelegate")

                Loader {
                    sourceComponent: measurementComponent
                    height: parent.height
                    anchors.left: parent.right
                    anchors.leftMargin: root.barLeftMargin
                }

            }
            Text {
                text: "Roboto " + checkDelegate.font.pixelSize
                color: root.measurementColor
            }
        }

        ColumnLayout {
            spacing: root.textTopMargin

            RadioDelegate {
                id: radioDelegate
                text: qsTr("RadioDelegate")

                Loader {
                    sourceComponent: measurementComponent
                    height: parent.height
                    anchors.left: parent.right
                    anchors.leftMargin: root.barLeftMargin
                }

            }
            Text {
                text: "Roboto " + radioDelegate.font.pixelSize
                color: root.measurementColor
            }
        }

        ColumnLayout {
            spacing: root.textTopMargin

            ComboBox {
                id: comboBox
                model: [ qsTr("ComboBox") ]

                Loader {
                    sourceComponent: measurementComponent
                    height: parent.height
                    anchors.left: parent.right
                    anchors.leftMargin: root.barLeftMargin
                }

            }
            Text {
                text: "Roboto " + comboBox.font.pixelSize
                color: root.measurementColor
            }
        }

        ColumnLayout {
            spacing: root.textTopMargin

            Item {
                implicitWidth: groupBox.implicitWidth
                implicitHeight: groupBox.implicitHeight

                GroupBox {
                    id: groupBox
                    title: qsTr("GroupBox")
                }
                Loader {
                    sourceComponent: measurementComponent
                    height: parent.height
                    anchors.left: parent.right
                    anchors.leftMargin: root.barLeftMargin
                }
            }
            Text {
                text: "Roboto " + groupBox.font.pixelSize
                color: root.measurementColor
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
