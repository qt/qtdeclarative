// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

import ".."

Page {
    id: root
    topPadding: 20

    property var backgroundColor

    header: RowLayout {
        CheckBox {
            id: iconCheckBox
            text: "Icon"
        }

        CheckBox {
            id: flatCheckBox
            text: "Flat"
        }
    }

    component RoundedScaleRepeater: Repeater {
        id: roundedScaleRepeater

        property var backgroundColor: undefined
        property var foregroundColor: undefined

        property int contentLeftMargin
        property int contentRightMargin

        model: ListModel {
            ListElement { displayName: "NotRounded"; roundedScale: Material.NotRounded }
            ListElement { displayName: "ExtraSmall"; roundedScale: Material.ExtraSmallScale }
            ListElement { displayName: "Small"; roundedScale: Material.SmallScale }
            ListElement { displayName: "Medium"; roundedScale: Material.MediumScale }
            ListElement { displayName: "Large"; roundedScale: Material.LargeScale }
            ListElement { displayName: "ExtraLarge"; roundedScale: Material.ExtraLargeScale }
            ListElement { displayName: "Full"; roundedScale: Material.FullScale }
        }

        // Workaround for QTBUG-98859.
        delegate: Component {
            ColumnLayout {
                id: scaleLayout
                spacing: Constants.spacing

                Layout.leftMargin: index === 0 ? roundedScaleRepeater.contentLeftMargin : 0
                Layout.rightMargin: index === roundedScaleRepeater.count - 1 ? roundedScaleRepeater.contentRightMargin : 0
                Layout.bottomMargin: Constants.spacing

                required property int index
                required property string displayName
                required property int roundedScale

                Label {
                    text: scaleLayout.displayName

                    Layout.alignment: Qt.AlignHCenter
                }

                Repeater {
                    model: 13

                    Button {
                        text: modelData
                        flat: flatCheckBox.checked
                        icon.source: iconCheckBox.checked ? Constants.iconSource : ""

                        Material.background: roundedScaleRepeater.backgroundColor
                        Material.foreground: roundedScaleRepeater.foregroundColor
                        Material.elevation: modelData
                        Material.roundedScale: scaleLayout.roundedScale
                    }
                }
            }
        }
    }

    ScrollView {
        anchors.fill: parent

        RowLayout {
            spacing: Constants.spacing

            RoundedScaleRepeater {
                contentLeftMargin: Constants.spacing
            }

            RoundedScaleRepeater {
                backgroundColor: Material.Teal
                foregroundColor: "white"
                contentRightMargin: Constants.spacing
            }
        }
    }
}
