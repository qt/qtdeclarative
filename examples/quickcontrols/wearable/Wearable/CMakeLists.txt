# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(wearable LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols/wearable/Wearable")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)

qt_policy(SET QTP0001 NEW)
qt_add_qml_module(wearable
    URI Wearable
    PLUGIN_TARGET wearable
    QML_FILES
        AlarmsPage.qml
        Clock.qml
        DemoMode.qml
        DemoModeIndicator.qml
        FitnessPage.qml
        LauncherPage.qml
        Main.qml
        NaviButton.qml
        NavigationPage.qml
        NotificationsPage.qml
        RouteElement.qml
        SettingsPage.qml
        SwipeViewPage.qml
        WeatherPage.qml
        WorldClockPage.qml
        fitness.js
        navigation.js
        notifications.js
        weather.js
    RESOURCES
        icons/wearable/36x36/alarms.png
        icons/wearable/36x36/fitness.png
        icons/wearable/36x36/navigation.png
        icons/wearable/36x36/notifications.png
        icons/wearable/36x36/settings.png
        icons/wearable/36x36/weather.png
        icons/wearable/36x36/worldclock.png
        icons/wearable/36x36@2/alarms.png
        icons/wearable/36x36@2/fitness.png
        icons/wearable/36x36@2/navigation.png
        icons/wearable/36x36@2/notifications.png
        icons/wearable/36x36@2/settings.png
        icons/wearable/36x36@2/weather.png
        icons/wearable/36x36@2/worldclock.png
        icons/wearable/index.theme
        images/back.png
        images/back@2x.png
        images/background-dark.png
        images/background-light.png
        images/fitness-man-running-dark.png
        images/fitness-man-running-dark@2x.png
        images/fitness-man-running-light.png
        images/fitness-man-running-light@2x.png
        images/fitness-man-walking-dark.png
        images/fitness-man-walking-dark@2x.png
        images/fitness-man-walking-light.png
        images/fitness-man-walking-light@2x.png
        images/home.png
        images/home@2x.png
        images/navigation-dark.png
        images/navigation-dark@2x.png
        images/navigation-end.png
        images/navigation-end@2x.png
        images/navigation-leftturn-dark.png
        images/navigation-leftturn-dark@2x.png
        images/navigation-leftturn-light.png
        images/navigation-leftturn-light@2x.png
        images/navigation-light.png
        images/navigation-light@2x.png
        images/navigation-rightturn-dark.png
        images/navigation-rightturn-dark@2x.png
        images/navigation-rightturn-light.png
        images/navigation-rightturn-light@2x.png
        images/navigation-start.png
        images/navigation-start@2x.png
        images/navigation-straight-dark.png
        images/navigation-straight-dark@2x.png
        images/navigation-straight-light.png
        images/navigation-straight-light@2x.png
        images/navigation-uturn.png
        images/navigation-uturn@2x.png
        images/notifications-avatarf-dark.png
        images/notifications-avatarf-dark@2x.png
        images/notifications-avatarf-light.png
        images/notifications-avatarf-light@2x.png
        images/notifications-avatarm-dark.png
        images/notifications-avatarm-dark@2x.png
        images/notifications-avatarm-light.png
        images/notifications-avatarm-light@2x.png
        images/notifications-missedcall-dark.png
        images/notifications-missedcall-dark@2x.png
        images/notifications-missedcall-light.png
        images/notifications-missedcall-light@2x.png
        images/settings-bluetooth-dark.png
        images/settings-bluetooth-dark@2x.png
        images/settings-bluetooth-light.png
        images/settings-bluetooth-light@2x.png
        images/settings-brightness-dark.png
        images/settings-brightness-dark@2x.png
        images/settings-brightness-light.png
        images/settings-brightness-light@2x.png
        images/settings-demo-mode-dark.png
        images/settings-demo-mode-dark@2x.png
        images/settings-demo-mode-light.png
        images/settings-demo-mode-light@2x.png
        images/settings-demo-mode-white.png
        images/settings-demo-mode-white@2x.png
        images/settings-theme-dark.png
        images/settings-theme-dark@2x.png
        images/settings-theme-light.png
        images/settings-theme-light@2x.png
        images/settings-wifi-dark.png
        images/settings-wifi-dark@2x.png
        images/settings-wifi-light.png
        images/settings-wifi-light@2x.png
        images/weather-humidity-dark.png
        images/weather-humidity-dark@2x.png
        images/weather-humidity-light.png
        images/weather-humidity-light@2x.png
        images/weather-pressure-dark.png
        images/weather-pressure-dark@2x.png
        images/weather-pressure-light.png
        images/weather-pressure-light@2x.png
        images/weather-sunrise-dark.png
        images/weather-sunrise-dark@2x.png
        images/weather-sunrise-light.png
        images/weather-sunrise-light@2x.png
        images/weather-sunset-dark.png
        images/weather-sunset-dark@2x.png
        images/weather-sunset-light.png
        images/weather-sunset-light@2x.png
        images/weather-temperature-dark.png
        images/weather-temperature-dark@2x.png
        images/weather-temperature-light.png
        images/weather-temperature-light@2x.png
        images/weather-wind-dark.png
        images/weather-wind-dark@2x.png
        images/weather-wind-light.png
        images/weather-wind-light@2x.png
        images/world-clock-center.png
        images/world-clock-center@2x.png
        images/world-clock-second.png
        images/world-clock-second@2x.png
        images/world-clock-swissdaydial.png
        images/world-clock-swissdaydial@2x.png
        images/world-clock-swissdayhour.png
        images/world-clock-swissdayhour@2x.png
        images/world-clock-swissdayminute.png
        images/world-clock-swissdayminute@2x.png
        images/world-clock-swissnightdial.png
        images/world-clock-swissnightdial@2x.png
        images/world-clock-swissnighthour.png
        images/world-clock-swissnighthour@2x.png
        images/world-clock-swissnightminute.png
        images/world-clock-swissnightminute@2x.png
        walk_route.json
        weather.json
)

target_link_libraries(wearable PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
)

if(UNIX AND NOT APPLE AND CMAKE_CROSSCOMPILING)
    find_package(Qt6 REQUIRED COMPONENTS QuickTemplates2)

    # Work around QTBUG-86533
    target_link_libraries(wearable PRIVATE Qt6::QuickTemplates2)
endif()

install(TARGETS wearable
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmldir
    DESTINATION "${INSTALL_EXAMPLEDIR}"
)
