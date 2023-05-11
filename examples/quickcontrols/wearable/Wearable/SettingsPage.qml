// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import WearableStyle
import WearableSettings

Item {

    QQC2.SwipeView {
        id: svSettingsContainer

        anchors.fill: parent

        SwipeViewPage {
            id: settingsPage1

            property alias bluetoothSwitch: bluetoothSwitch
            property alias wirelessSwitch: wirelessSwitch

            Column {
                anchors.centerIn: parent
                spacing: 25

                Row {
                    spacing: 50
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: UIStyle.themeImagePath("settings-bluetooth")
                    }
                    QQC2.Switch {
                        id: bluetoothSwitch
                        anchors.verticalCenter: parent.verticalCenter
                        checked: WearableSettings.bluetooth
                        onToggled: WearableSettings.bluetooth = checked
                    }
                }
                Row {
                    spacing: 50
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: UIStyle.themeImagePath("settings-wifi")
                    }
                    QQC2.Switch {
                        id: wirelessSwitch
                        anchors.verticalCenter: parent.verticalCenter
                        checked: WearableSettings.wireless
                        onToggled: WearableSettings.wireless = checked
                    }
                }
            }
        }

        SwipeViewPage {
            id: settingsPage2

            property alias brightnessSlider: brightnessSlider
            property alias darkThemeSwitch: darkThemeSwitch

            Column {
                anchors.centerIn: parent
                spacing: 2

                Column {
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: UIStyle.themeImagePath("settings-brightness")
                    }
                    QQC2.Slider {
                        id: brightnessSlider
                        anchors.horizontalCenter: parent.horizontalCenter
                        from: 0
                        to: 5
                        stepSize: 1
                        value: WearableSettings.brightness
                        onMoved: WearableSettings.brightness = value
                    }
                }
                Column {
                    anchors.horizontalCenter: parent.horizontalCenter

                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: UIStyle.themeImagePath("settings-theme")
                    }
                    QQC2.Switch {
                        id: darkThemeSwitch
                        anchors.horizontalCenter: parent.horizontalCenter
                        checked: WearableSettings.darkTheme
                        onToggled: WearableSettings.darkTheme = checked
                    }
                }
            }
        }

        SwipeViewPage {
            id: settingsPage3

            Column {
                anchors.centerIn: parent

                Column {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 6

                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: UIStyle.themeImagePath("settings-demo-mode")
                    }
                    QQC2.Switch {
                        id: demoModeSwitch
                        anchors.horizontalCenter: parent.horizontalCenter
                        checked: WearableSettings.demoMode
                        onToggled: WearableSettings.demoMode = checked
                    }
                }
            }
        }
    }

    QQC2.PageIndicator {
        count: svSettingsContainer.count
        currentIndex: svSettingsContainer.currentIndex

        anchors.bottom: svSettingsContainer.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
