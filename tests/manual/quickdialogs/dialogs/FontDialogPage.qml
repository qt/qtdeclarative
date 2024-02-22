// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import "."

ColumnLayout {
    property alias dialog: fontDialog

    // Put it all in another ColumnLayout so we can easily add margins.
    ColumnLayout {
        Layout.leftMargin: 12
        Layout.rightMargin: 12
        Layout.topMargin: 12
        Layout.bottomMargin: 12

        GroupBox {
            title: qsTr("Dialog properties")

            Layout.fillWidth: true
            Layout.fillHeight: false

            GridLayout {
                columns: 2
                anchors.fill: parent

                Label {
                    text: qsTr("modality")

                    Layout.alignment: Qt.AlignTop
                    Layout.minimumWidth: ApplicationWindow.window.width * 0.2
                    Layout.maximumWidth: ApplicationWindow.window.width * 0.2
                }
                ButtonGroup {
                    id: modalityButtonGroup
                    buttons: modalityColumnLayout.children
                }
                ColumnLayout {
                    id: modalityColumnLayout

                    RadioButton {
                        text: qsTr("Qt.NonModal")
                        Layout.fillWidth: false

                        readonly property int modality: Qt.NonModal
                    }
                    RadioButton {
                        text: qsTr("Qt.WindowModal")
                        checked: true
                        Layout.fillWidth: false

                        readonly property int modality: Qt.WindowModal
                    }
                    RadioButton {
                        text: qsTr("Qt.ApplicationModal")
                        Layout.fillWidth: false

                        readonly property int modality: Qt.ApplicationModal
                    }
                }

                Label {
                    text: qsTr("result")
                }
                TextField {
                    id: resultTextField
                    text: fontDialog.result === 1 ? qsTr("Accepted") : qsTr("Rejected")
                    readOnly: true
                    enabled: false
                    Layout.fillWidth: false
                }

                Label {
                    text: qsTr("title")
                }
                TextField {
                    id: titleTextField
                    text: qsTr("Pick a font")
                    Layout.fillWidth: false
                }
            }
        }

        GroupBox {
            title: qsTr("FontDialog properties")

            Layout.fillWidth: true
            Layout.fillHeight: false

            GridLayout {
                columns: 2
                anchors.fill: parent

                Label {
                    Layout.minimumWidth: ApplicationWindow.window.width * 0.2
                    Layout.maximumWidth: ApplicationWindow.window.width * 0.2
                    text: qsTr("currentFont")
                }
                TextField {
                    id: currentFontTextField
                    text: qsTr("AaBbYyZz")
                    font: fontDialog.currentFont
                    readOnly: true
                    selectByMouse: true

                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("selectedFont")
                }
                TextField {
                    id: selectedFontTextField
                    text: qsTr("AaBbYyZz")
                    font: fontDialog.selectedFont
                    readOnly: true
                    selectByMouse: true

                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("fontOptions")

                    Layout.alignment: Qt.AlignTop
                }
                ColumnLayout {
                    id: fontOptionsColumnLayout

                    CheckBox {
                        id: noButtons
                        text: qsTr("NoButtons")
                        Layout.fillWidth: false

                        readonly property int fontOption: checked ? FontDialog.NoButtons : 0
                    }
                    CheckBox {
                        id: scalableFonts
                        text: qsTr("ScalableFonts")
                        Layout.fillWidth: false

                        readonly property int fontOption: checked ? FontDialog.ScalableFonts : 0
                    }
                    CheckBox {
                        id: nonScalableFonts
                        text: qsTr("NonScalableFonts")
                        Layout.fillWidth: false

                        readonly property int fontOption: checked ? FontDialog.NonScalableFonts : 0
                    }
                    CheckBox {
                        id: monospacedFonts
                        text: qsTr("MonospacedFonts")
                        Layout.fillWidth: false

                        readonly property int fontOption: checked ? FontDialog.MonospacedFonts : 0
                    }
                    CheckBox {
                        id: proportionalFonts
                        text: qsTr("ProportionalFonts")
                        Layout.fillWidth: false

                        readonly property int fontOption: checked ? FontDialog.ProportionalFonts : 0
                    }
                }
            }
        }

        FontDialog {
            id: fontDialog

            modality: modalityButtonGroup.checkedButton.modality
            title: titleTextField.text
            options: noButtons.fontOption
                | scalableFonts.fontOption
                | nonScalableFonts.fontOption
                | monospacedFonts.fontOption
                | proportionalFonts.fontOption
        }
    }
}
