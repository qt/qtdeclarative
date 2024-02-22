// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Controls.Universal

ApplicationWindow {
    id: window
    visible: true
    title: "Buttons"

    Component.onCompleted: {
        var pane = repeater.itemAt(0)
        width = pane.implicitWidth * 2 + flickable.leftMargin + flickable.rightMargin + flow.spacing
        height = header.height + pane.implicitHeight * 2 + flickable.topMargin + flickable.bottomMargin + flow.spacing
    }

    header: ToolBar {
        Row {
            spacing: 20
            anchors.right: parent.right
            CheckBox {
                id: hoverBox
                text: "Hover"
                checked: true
            }
            CheckBox {
                id: roundBox
                text: "Round"
                checked: false
            }
        }
    }

    Flickable {
        id: flickable
        anchors.fill: parent

        topMargin: 40
        leftMargin: 40
        rightMargin: 40
        bottomMargin: 40

        contentHeight: flow.implicitHeight

        Flow {
            id: flow
            spacing: 40
            width: flickable.width - flickable.leftMargin - flickable.rightMargin

            Repeater {
                id: repeater

                model: [
                    { title: "Normal", theme: Material.Light, flat: false },
                    { title: "Flat", theme: Material.Light, flat: true },
                    { title: "Normal", theme: Material.Dark, flat: false },
                    { title: "Flat", theme: Material.Dark, flat: true }
                ]

                Pane {
                    Material.elevation: 8
                    Material.theme: modelData.theme
                    Universal.theme: modelData.theme

                    GroupBox {
                        title: modelData.title
                        background.visible: false

                        Grid {
                            columns: 4
                            spacing: 20
                            padding: 20

                            ButtonLoader { text: "Normal";   flat: modelData.flat; hoverEnabled: hoverBox.checked; round: roundBox.checked }
                            ButtonLoader { text: "Disabled"; flat: modelData.flat; hoverEnabled: hoverBox.checked; enabled: false; round: roundBox.checked }
                            ButtonLoader { text: "Down";     flat: modelData.flat; hoverEnabled: hoverBox.checked; down: true; round: roundBox.checked }
                            ButtonLoader { text: "Disabled"; flat: modelData.flat; hoverEnabled: hoverBox.checked; down: true; enabled: false; round: roundBox.checked }

                            ButtonLoader { text: "Checked";  flat: modelData.flat; hoverEnabled: hoverBox.checked; checked: true; round: roundBox.checked }
                            ButtonLoader { text: "Disabled"; flat: modelData.flat; hoverEnabled: hoverBox.checked; checked: true; enabled: false; round: roundBox.checked }
                            ButtonLoader { text: "Down";     flat: modelData.flat; hoverEnabled: hoverBox.checked; checked: true; down: true; round: roundBox.checked }
                            ButtonLoader { text: "Disabled"; flat: modelData.flat; hoverEnabled: hoverBox.checked; checked: true; down: true; enabled: false; round: roundBox.checked }

                            ButtonLoader { text: "Highlighted"; flat: modelData.flat; hoverEnabled: hoverBox.checked; highlighted: true; round: roundBox.checked }
                            ButtonLoader { text: "Disabled";    flat: modelData.flat; hoverEnabled: hoverBox.checked; highlighted: true; enabled: false; round: roundBox.checked }
                            ButtonLoader { text: "Down";        flat: modelData.flat; hoverEnabled: hoverBox.checked; highlighted: true; down: true; round: roundBox.checked }
                            ButtonLoader { text: "Disabled";    flat: modelData.flat; hoverEnabled: hoverBox.checked; highlighted: true; down: true; enabled: false; round: roundBox.checked }

                            ButtonLoader { text: "Hi-checked"; flat: modelData.flat; hoverEnabled: hoverBox.checked; highlighted: true; checked: true; round: roundBox.checked }
                            ButtonLoader { text: "Disabled";   flat: modelData.flat; hoverEnabled: hoverBox.checked; highlighted: true; checked: true; enabled: false; round: roundBox.checked }
                            ButtonLoader { text: "Down";       flat: modelData.flat; hoverEnabled: hoverBox.checked; highlighted: true; checked: true; down: true; round: roundBox.checked }
                            ButtonLoader { text: "Disabled";   flat: modelData.flat; hoverEnabled: hoverBox.checked; highlighted: true; checked: true; down: true; enabled: false; round: roundBox.checked }
                        }
                    }
                }
            }
        }

        ScrollIndicator.vertical: ScrollIndicator { }
    }
}
