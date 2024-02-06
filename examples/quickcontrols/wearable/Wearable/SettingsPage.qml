// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import WearableStyle
import WearableSettings

Item {
    id: settingspage

    property alias listView: listViewItem

    component SettingsItem: ListItem {
        id: settingsItem

        property string title
        property string icon

        Image {
            id: itemIcon
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 15
            source: UIStyle.iconPath(settingsItem.icon)
            height: 40
            width: 40
        }
        Text {
            anchors.left: itemIcon.right
            anchors.verticalCenter: itemIcon.verticalCenter
            anchors.margins: 15
            text: settingsItem.title
            color: UIStyle.textColor
            font: UIStyle.h3
        }
    }

    component SettingsBoolItem: SettingsItem {
        height: 70

        property alias checked: onSwitchItem.checked

        Switch {
            id: onSwitchItem
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 15
            onCheckedChanged: parent.onCheckedChanged()
        }
        function toggle() { onSwitchItem.toggle() }
        signal onCheckedChanged()

    }

    component SettingsIntItem: SettingsItem {
        height: 110

        property alias value: valueSliderItem.value

        Slider {
            id: valueSliderItem
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 15
            from: 0
            to: 100
            onValueChanged: parent.onValueChanged()
        }

        signal onValueChanged()
    }

    Flickable {
        id: listViewItem

        anchors.fill: parent
        anchors.margins: 15
        anchors.topMargin: 40 + 15
        contentHeight: content.height

        // aliases for the demo mode
        property alias brightnessItem: brightnessItem
        property alias bluetoothItem: bluetoothItem
        property alias wifiItem: wifiItem
        property alias darkmodeItem: darkmodeItem
        property alias demomodeItem: demomodeItem

        Column {
            id: content
            spacing: 10
            width: parent.width

            SettingsIntItem {
                id: brightnessItem
                width: parent.width
                title: qsTr("Brightness")
                icon: "sun"
                value: WearableSettings.brightness
                onValueChanged: WearableSettings.brightness = value
            }
            SettingsBoolItem {
                id: bluetoothItem
                width: parent.width
                title: qsTr("Bluetooth")
                icon: "bluetooth"
                checked: WearableSettings.bluetooth
                onCheckedChanged: WearableSettings.bluetooth = checked
            }
            SettingsBoolItem {
                id: wifiItem
                width: parent.width
                title: qsTr("Wi-Fi")
                icon: "wifi"
                checked: WearableSettings.wireless
                onCheckedChanged: WearableSettings.wireless = checked
            }
            SettingsBoolItem {
                id: darkmodeItem
                width: parent.width
                title: qsTr("Change theme")
                icon: "darkmode"
                checked: WearableSettings.darkTheme
                onCheckedChanged: WearableSettings.darkTheme = checked
            }
            SettingsBoolItem {
                id: demomodeItem
                width: parent.width
                title: qsTr("Demo mode")
                icon: "demomode"
                checked: WearableSettings.demoMode
                onCheckedChanged: WearableSettings.demoMode = checked
            }
        }
    }
}
