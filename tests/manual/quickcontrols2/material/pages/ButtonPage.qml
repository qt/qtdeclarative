// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
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
        }

        CheckBox {
            id: disabledCheckBox
            text: "Disabled"
        }

        Item {
            Layout.fillWidth: true
        }
    }

    component RoundedScaleLayout: ColumnLayout {
        id: roundedScaleLayout
        enabled: !disabledCheckBox.checked

        property bool allowFlat
        property var backgroundColor: undefined
        property var foregroundColor: undefined

        property int contentLeftMargin
        property int contentRightMargin

        RowLayout {
            enabled: roundedScaleLayout.allowFlat

            CheckBox {
                id: flatCheckBox
                text: "Flat"

                Layout.leftMargin: roundedScaleLayout.contentLeftMargin
            }
        }

        RowLayout {
            spacing: Constants.spacing

            Repeater {
                id: roundedScaleRepeater
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

                        required property int index
                        required property string displayName
                        required property int roundedScale

                        Layout.leftMargin: index === 0 ? roundedScaleLayout.contentLeftMargin : 0
                        Layout.rightMargin: index === roundedScaleRepeater.count - 1 ? roundedScaleLayout.contentRightMargin : 0
                        Layout.bottomMargin: Constants.spacing

                        Label {
                            text: scaleLayout.displayName

                            Layout.alignment: Qt.AlignHCenter
                        }

                        Repeater {
                            model: 13

                            Button {
                                text: modelData
                                flat: flatCheckBox.checked
                                icon.source: iconCheckBox.checked ? "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png" : ""

                                Material.background: roundedScaleLayout.backgroundColor
                                Material.foreground: roundedScaleLayout.foregroundColor
                                Material.elevation: modelData
                                Material.roundedScale: scaleLayout.roundedScale
                            }
                        }
                    }
                }
            }
        }
    }

    ScrollView {
        anchors.fill: parent

        RowLayout {
            spacing: Constants.spacing

            RoundedScaleLayout {
                contentLeftMargin: Constants.spacing
                allowFlat: true
            }

            RoundedScaleLayout {
                backgroundColor: Material.Teal
                foregroundColor: "white"
                contentRightMargin: Constants.spacing
            }
        }
    }
}
