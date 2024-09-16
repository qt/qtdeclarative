// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

import WearableSettings

Item {
    id: demoItem
    required property StackView stackView

    SequentialAnimation {
        id: demoModeAnimation
        loops: 2 //darkMode and lightMode
        running: WearableSettings.demoMode

        // Go back to the launcher page.
        PauseAnimation { duration: 1000 }
        ScriptAction { script: demoItem.stackView.pop(null) }
        PauseAnimation { duration: 2000 }


        // Open the world clock page.
        ScriptAction { script: demoItem.stackView.currentItem.currentIndex = 0 }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: demoItem.stackView.currentItem.launched(qsTr("World Clock"), "WorldClockPage.qml", "") }
        PauseAnimation { duration: 2000 }

        // Swipe across a few times.
        SequentialAnimation {
            loops: 6

            ScriptAction { script: demoItem.stackView.currentItem.children[0].incrementCurrentIndex() }
            PauseAnimation { duration: 2500 }
        }

        // Go back to the launcher page.
        ScriptAction { script: demoItem.stackView.pop(null) }
        PauseAnimation { duration: 2000 }



        // Open the navigation page.
        ScriptAction { script: demoItem.stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: demoItem.stackView.currentItem.launched(qsTr("Navigation"), "NavigationPage.qml", "NavigationFallbackPage.qml") }
        PauseAnimation { duration: 2000 }

        // Scroll through the route.
        SequentialAnimation {
            loops: 23

            ScriptAction { script: demoItem.stackView.currentItem.incrementPoint() }
            PauseAnimation { duration: 2000 }
        }

        // Go back to the launcher page.
        ScriptAction { script: demoItem.stackView.pop(null) }
        PauseAnimation { duration: 2000 }



        // Open the weather page.
        ScriptAction { script: demoItem.stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: demoItem.stackView.currentItem.launched(qsTr("Weather"), "WeatherPage.qml", "") }
        PauseAnimation { duration: 2000 }

        // Flick down..
        SequentialAnimation {
            loops: 3
            ScriptAction { script: demoItem.stackView.currentItem.children[0].flick(0, - 800) }
            PauseAnimation { duration: 2000 }
        }

        // Go back to the launcher page.
        ScriptAction { script: demoItem.stackView.pop(null) }
        PauseAnimation { duration: 2000 }



        // Open the fitness page.
        ScriptAction { script: demoItem.stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: demoItem.stackView.currentItem.launched(qsTr("Fitness"), "FitnessPage.qml", "") }
        PauseAnimation { duration: 2000 }

        // Swipe across once.
        SequentialAnimation {
            loops: 1

            ScriptAction { script: demoItem.stackView.currentItem.children[0].incrementCurrentIndex() }
            PauseAnimation { duration: 2000 }
        }

        // Go back to the launcher page.
        ScriptAction { script: demoItem.stackView.pop(null) }
        PauseAnimation { duration: 2000 }

        // Open the notifications page.
        ScriptAction { script: demoItem.stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: demoItem.stackView.currentItem.launched(qsTr("Notifications"), "NotificationsPage.qml", "") }
        PauseAnimation { duration: 2000 }

        // Go back to the launcher page.
        ScriptAction { script: demoItem.stackView.pop(null) }
        PauseAnimation { duration: 2000 }



        // Open the alarms page.
        ScriptAction { script: demoItem.stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: demoItem.stackView.currentItem.launched(qsTr("Alarm"), "AlarmsPage.qml", "") }
        PauseAnimation { duration: 2000 }

        // Toggle the switch.
        ScriptAction { script: demoItem.stackView.currentItem.alarmList.itemAtIndex(0).onSwitch.toggle() }
        PauseAnimation { duration: 500 }
        ScriptAction { script: demoItem.stackView.currentItem.alarmList.itemAtIndex(1).onSwitch.toggle() }
        PauseAnimation { duration: 2000 }
        ScriptAction { script: demoItem.stackView.currentItem.alarmList.itemAtIndex(0).hourTumbler.currentIndex = 8 }
        PauseAnimation { duration: 200 }
        ScriptAction { script: demoItem.stackView.currentItem.alarmList.itemAtIndex(0).minuteTumbler.currentIndex = 15 }
        PauseAnimation { duration: 300 }
        ScriptAction { script: demoItem.stackView.currentItem.alarmList.itemAtIndex(1).hourTumbler.currentIndex = 9 }
        PauseAnimation { duration: 200 }
        ScriptAction { script: demoItem.stackView.currentItem.alarmList.itemAtIndex(1).minuteTumbler.currentIndex = 45 }
        PauseAnimation { duration: 2000 }
        ScriptAction { script: demoItem.stackView.currentItem.addAlarm() }
        PauseAnimation { duration: 2000 }
        ScriptAction { script: demoItem.stackView.currentItem.alarmList.itemAtIndex(2).onSwitch.toggle() }
        PauseAnimation { duration: 100 }
        ScriptAction { script: demoItem.stackView.currentItem.alarmList.itemAtIndex(0).onSwitch.toggle() }
        PauseAnimation { duration: 100 }
        ScriptAction { script: demoItem.stackView.currentItem.alarmList.itemAtIndex(1).onSwitch.toggle() }
        PauseAnimation { duration: 2000 }
        ScriptAction { script: demoItem.stackView.currentItem.alarmList.itemAtIndex(2).hourTumbler.currentIndex = 10 }
        PauseAnimation { duration: 200 }
        ScriptAction { script: demoItem.stackView.currentItem.alarmList.itemAtIndex(2).minuteTumbler.currentIndex = 30 }
        PauseAnimation { duration: 2000 }

        // Go back to the launcher page.
        ScriptAction { script: demoItem.stackView.pop(null) }
        PauseAnimation { duration: 2000 }


        // Open the settings page.
        ScriptAction { script: demoItem.stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: demoItem.stackView.currentItem.launched(qsTr("Settings"), "SettingsPage.qml", "") }
        PauseAnimation { duration: 3000 }

        // Toggle the switches.
        ScriptAction { script: demoItem.stackView.currentItem.listView.brightnessItem.value = 80 }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: demoItem.stackView.currentItem.listView.bluetoothItem.toggle() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: demoItem.stackView.currentItem.listView.wifiItem.toggle() }
        PauseAnimation { duration: 1000 }
        SequentialAnimation {
            loops: 2
            ScriptAction { script: demoItem.stackView.currentItem.children[0].flick(0, - 800) }
            PauseAnimation { duration: 500 }
        }
        ScriptAction { script: demoItem.stackView.currentItem.listView.darkmodeItem.toggle() }
        PauseAnimation { duration: 3000 }

        // Go back to the launcher page.
        ScriptAction { script: demoItem.stackView.pop(null) }
    }
}
