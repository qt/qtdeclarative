// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import Wearable
import WearableStyle

Item {
    QQC2.SwipeView {
        id: svAlarmsContainer

        anchors.fill: parent

        Repeater {
            model: ListModel {
                ListElement { name: qsTr("Week Days"); state: true; time: "06:00 AM" }
                ListElement { name: qsTr("Week Ends"); state: false; time: "07:30 AM" }
            }

            SwipeViewPage {
                property alias stateSwitch: stateSwitch

                Column {
                    spacing: 30
                    anchors.centerIn: parent

                    QQC2.Switch {
                        id: stateSwitch
                        checked: model.state
                        anchors.left: nameLabel.right
                    }

                    Text {
                        text: model.time
                        anchors.horizontalCenter: parent.horizontalCenter
                        verticalAlignment: Text.AlignVCenter
                        height: UIStyle.fontSizeXL
                        font.bold: stateSwitch.checked
                        font.pixelSize: stateSwitch.checked ? UIStyle.fontSizeXL : UIStyle.fontSizeL
                        font.letterSpacing: 4
                        color: UIStyle.themeColorQtGray1
                    }

                    Text {
                        id: nameLabel
                        text: model.name
                        anchors.horizontalCenter: parent.horizontalCenter
                        font.pixelSize: UIStyle.fontSizeS
                        font.italic: true
                        font.bold: true
                        font.letterSpacing: 1
                        color: UIStyle.themeColorQtGray2
                    }
                }
            }
        }
    }

    QQC2.PageIndicator {
        count: svAlarmsContainer.count
        currentIndex: svAlarmsContainer.currentIndex

        anchors.bottom: svAlarmsContainer.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
