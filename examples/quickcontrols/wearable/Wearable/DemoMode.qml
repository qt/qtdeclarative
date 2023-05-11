// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

import WearableSettings
import WearableStyle

Item {
    property StackView stackView

    SequentialAnimation {
        id: demoModeAnimation
        running: WearableSettings.demoMode

        // Set brightness back to normal.
        ScriptAction { script: WearableSettings.brightness = 0 }

        // Go back to the launcher page.
        PauseAnimation { duration: 1000 }
        ScriptAction { script: stackView.pop(null) }
        PauseAnimation { duration: 2000 }

        // Open the world clock page.
        ScriptAction { script: stackView.currentItem.launched("WorldClockPage.qml") }
        PauseAnimation { duration: 2000 }

        // Swipe across a few times.
        SequentialAnimation {
            loops: 6

            ScriptAction { script: stackView.currentItem.children[0].incrementCurrentIndex() }
            PauseAnimation { duration: 2500 }
        }


        // Go back to the launcher page.
        ScriptAction { script: stackView.pop(null) }
        PauseAnimation { duration: 2000 }

        // Open the navigation page.
        ScriptAction { script: stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: stackView.currentItem.launched("NavigationPage.qml") }
        PauseAnimation { duration: 2000 }

        // Flick down a few times.
        SequentialAnimation {
            loops: 6

            ScriptAction { script: stackView.currentItem.routeListView.incrementCurrentIndex() }
            PauseAnimation { duration: 2000 }
        }


        // Go back to the launcher page.
        ScriptAction { script: stackView.pop(null) }
        PauseAnimation { duration: 2000 }

        // Open the weather page.
        ScriptAction { script: stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: stackView.currentItem.launched("WeatherPage.qml") }
        PauseAnimation { duration: 2000 }

        // Swipe across a few times.
        SequentialAnimation {
            loops: 4

            ScriptAction { script: stackView.currentItem.children[0].incrementCurrentIndex() }
            PauseAnimation { duration: 2000 }
        }


        // Go back to the launcher page.
        ScriptAction { script: stackView.pop(null) }
        PauseAnimation { duration: 2000 }

        // Open the fitness page.
        ScriptAction { script: stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: stackView.currentItem.launched("FitnessPage.qml") }
        PauseAnimation { duration: 2000 }

        // Swipe across a few times.
        SequentialAnimation {
            loops: 2

            ScriptAction { script: stackView.currentItem.children[0].incrementCurrentIndex() }
            PauseAnimation { duration: 2000 }
        }


        // Go back to the launcher page.
        ScriptAction { script: stackView.pop(null) }
        PauseAnimation { duration: 2000 }

        // Open the notifications page.
        ScriptAction { script: stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: stackView.currentItem.launched("NotificationsPage.qml") }

        // Flick down a few times.
        SequentialAnimation {
            loops: 3

            PauseAnimation { duration: 2000 }
            ScriptAction { script: stackView.currentItem.incrementCurrentIndex() }
        }


        // Go back to the launcher page.
        ScriptAction { script: stackView.pop(null) }
        PauseAnimation { duration: 2000 }

        // Open the alarms page.
        ScriptAction { script: stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: stackView.currentItem.launched("AlarmsPage.qml") }
        PauseAnimation { duration: 2000 }

        // Toggle the switch.
        ScriptAction { script: stackView.currentItem.children[0].currentItem.stateSwitch.toggle() }
        PauseAnimation { duration: 2000 }

        // Go to the next alarm.
        ScriptAction { script: stackView.currentItem.children[0].incrementCurrentIndex() }
        PauseAnimation { duration: 2000 }

        // Toggle the switch there too.
        ScriptAction { script: stackView.currentItem.children[0].currentItem.stateSwitch.toggle() }
        PauseAnimation { duration: 2000 }


        // Go back to the launcher page.
        ScriptAction { script: stackView.pop(null) }
        PauseAnimation { duration: 2000 }

        // Open the settings page.
        ScriptAction { script: stackView.currentItem.incrementCurrentIndex() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: stackView.currentItem.launched("SettingsPage.qml") }
        PauseAnimation { duration: 3000 }

        // Toggle the switches.
        ScriptAction { script: stackView.currentItem.children[0].currentItem.bluetoothSwitch.toggle() }
        PauseAnimation { duration: 1000 }
        ScriptAction { script: stackView.currentItem.children[0].currentItem.wirelessSwitch.toggle() }
        PauseAnimation { duration: 3000 }

        // Go to the next page.
        ScriptAction { script: stackView.currentItem.children[0].incrementCurrentIndex() }

        // Play with the brightness slider.
        // First, set it to full brightness so we start in the correct state.
        ScriptAction {
            script: {
                var brightnessSlider = stackView.currentItem.children[0].currentItem.brightnessSlider
                brightnessSlider.value = 0
                // increase()/decrease() are not a result of user interaction and
                // hence moved() will not be emitted, so we do it ourselves.
                brightnessSlider.moved()
            }
        }

        // Decrease the brightness.
        SequentialAnimation {
            loops: 3

            PauseAnimation { duration: 1000 }
            ScriptAction {
                script: {
                    var brightnessSlider = stackView.currentItem.children[0].currentItem.brightnessSlider
                    brightnessSlider.decrease()
                    brightnessSlider.moved()
                }
            }
        }

        // Increase the brightness back to full.
        PauseAnimation { duration: 3000 }
        SequentialAnimation {
            loops: 3

            PauseAnimation { duration: 1000 }
            ScriptAction {
                script: {
                    var brightnessSlider = stackView.currentItem.children[0].currentItem.brightnessSlider
                    brightnessSlider.increase()
                    brightnessSlider.moved()
                }
            }
        }

        // Toggle the dark theme switch.
        PauseAnimation { duration: 2000 }
        ScriptAction {
            script: {
                var darkThemeSwitch = stackView.currentItem.children[0].currentItem.darkThemeSwitch
                darkThemeSwitch.toggle()
                // As above, only proper user interaction results in toggled() being emitted,
                // so we do it ourselves.
                darkThemeSwitch.toggled()
            }
        }
        PauseAnimation { duration: 4000 }

        // Go back to the launcher page.
        ScriptAction { script: stackView.pop(null) }
    }
}
