# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(wearable LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

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
        ListHeaderItem.qml
        ListItem.qml
        Main.qml
        MenuHeader.qml
        NavigationPage.qml
        NavigationFallbackPage.qml
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
        icons/wind-light.svg
        icons/wind-dark.svg
        icons/wifi-light.svg
        icons/wifi-dark.svg
        icons/weather-light.svg
        icons/weather-dark.svg
        icons/uturnright-light.svg
        icons/uturnright-dark.svg
        icons/uturnleft-light.svg
        icons/uturnleft-dark.svg
        icons/thermometer-light.svg
        icons/thermometer-dark.svg
        icons/sunset-light.svg
        icons/sunset-dark.svg
        icons/sunrise-light.svg
        icons/sunrise-dark.svg
        icons/sun-light.svg
        icons/sun-dark.svg
        icons/settings-light.svg
        icons/settings-dark.svg
        icons/right-light.svg
        icons/right-dark.svg
        icons/pressure-light.svg
        icons/pressure-dark.svg
        icons/plus-light.svg
        icons/plus-dark.svg
        icons/notification-light.svg
        icons/notification-dark.svg
        icons/maps-light.svg
        icons/maps-dark.svg
        icons/lightright-light.svg
        icons/lightright-dark.svg
        icons/lightleft-light.svg
        icons/lightleft-dark.svg
        icons/left-light.svg
        icons/left-dark.svg
        icons/hearth-light.svg
        icons/hearth-dark.svg
        icons/forward-light.svg
        icons/forward-dark.svg
        icons/drop-light.svg
        icons/drop-dark.svg
        icons/demomode-light.svg
        icons/demomode-dark.svg
        icons/darkmode-light.svg
        icons/darkmode-dark.svg
        icons/clock-light.svg
        icons/clock-dark.svg
        icons/bluetooth-light.svg
        icons/bluetooth-dark.svg
        icons/bell-light.svg
        icons/bell-dark.svg
        icons/bearright-light.svg
        icons/bearright-dark.svg
        icons/bearleft-light.svg
        icons/bearleft-dark.svg
        icons/back-light.svg
        icons/back-dark.svg
        icons/weather-thundershower-light.svg
        icons/weather-thundershower-dark.svg
        icons/weather-sunny-very-few-clouds-light.svg
        icons/weather-sunny-very-few-clouds-dark.svg
        icons/weather-sunny-light.svg
        icons/weather-sunny-dark.svg
        icons/weather-storm-light.svg
        icons/weather-storm-dark.svg
        icons/weather-snow-light.svg
        icons/weather-snow-dark.svg
        icons/weather-sleet-light.svg
        icons/weather-sleet-dark.svg
        icons/weather-showers-scattered-light.svg
        icons/weather-showers-scattered-dark.svg
        icons/weather-showers-light.svg
        icons/weather-showers-dark.svg
        icons/weather-overcast-light.svg
        icons/weather-overcast-dark.svg
        icons/weather-icy-light.svg
        icons/weather-icy-dark.svg
        icons/weather-haze-light.svg
        icons/weather-haze-dark.svg
        icons/weather-fog-light.svg
        icons/weather-fog-dark.svg
        icons/weather-few-clouds-light.svg
        icons/weather-few-clouds-dark.svg
        images/fitness-man-walking-light.svg
        images/fitness-man-running-light.svg
        images/fitness-man-walking-dark.svg
        images/fitness-man-running-dark.svg
        images/fallbackmap.png
        fallbackroute.json
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
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/Wearable"
    LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}/Wearable"
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmldir
    DESTINATION "${CMAKE_INSTALL_BINDIR}/Wearable"
)
