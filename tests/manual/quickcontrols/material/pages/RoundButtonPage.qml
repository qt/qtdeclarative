// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import ".."

Page {
    topPadding: 20

    property var backgroundColor

    header: RowLayout {
        CheckBox {
            id: iconCheckBox
            text: "Icon"
            Layout.fillWidth: false
        }

        CheckBox {
            id: disabledCheckBox
            text: "Disabled"
            Layout.fillWidth: false
        }

        Item {
            Layout.fillWidth: true
        }
    }

    component ElevationLayout: ColumnLayout {
        id: elevationLayout
        enabled: !disabledCheckBox.checked

        property bool allowFlat
        property var backgroundColor: undefined
        property var foregroundColor: undefined

        property int contentLeftMargin
        property int contentRightMargin

        RowLayout {
            enabled: elevationLayout.allowFlat

            CheckBox {
                id: flatCheckBox
                text: "Flat"

                Layout.leftMargin: elevationLayout.contentLeftMargin
                Layout.fillWidth: false
            }
        }

        ColumnLayout {
            spacing: Constants.spacing

            Repeater {
                model: 13

                RoundButton {
                    text: iconCheckBox.checked ? "" : modelData
                    flat: flatCheckBox.checked
                    icon.source: iconCheckBox.checked ? Constants.iconSource : ""

                    Material.background: elevationLayout.backgroundColor
                    Material.foreground: elevationLayout.foregroundColor
                    Material.elevation: modelData

                    Layout.leftMargin: elevationLayout.contentLeftMargin
                    Layout.fillWidth: false
                }
            }

            Layout.bottomMargin: Constants.spacing
        }
    }

    ScrollView {
        anchors.fill: parent

        RowLayout {
            spacing: Constants.spacing

            ElevationLayout {
                contentLeftMargin: Constants.spacing
                allowFlat: true
            }

            ElevationLayout {
                backgroundColor: Material.Teal
                foregroundColor: "white"
                contentRightMargin: Constants.spacing
            }
        }
    }
}
