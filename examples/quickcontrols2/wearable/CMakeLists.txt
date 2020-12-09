# Generated from wearable.pro.

cmake_minimum_required(VERSION 3.14)
project(wearable LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols2/wearable")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS QuickControls2)

qt_add_executable(wearable
    wearable.cpp
)
set_target_properties(wearable PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(wearable PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickControls2
)
# special case begin
if(UNIX AND NOT APPLE AND CMAKE_CROSSCOMPILING)
    find_package(Qt6 COMPONENTS QuickTemplates2)
     # Work around QTBUG-86533
    target_link_libraries(wearable PRIVATE Qt::QuickTemplates2)
endif()
# special case end

# Resources:
set(wearable_resource_files
    "icons/wearable/36x36/alarms.png"
    "icons/wearable/36x36/fitness.png"
    "icons/wearable/36x36/navigation.png"
    "icons/wearable/36x36/notifications.png"
    "icons/wearable/36x36/settings.png"
    "icons/wearable/36x36/weather.png"
    "icons/wearable/36x36/worldclock.png"
    "icons/wearable/36x36@2/alarms.png"
    "icons/wearable/36x36@2/fitness.png"
    "icons/wearable/36x36@2/navigation.png"
    "icons/wearable/36x36@2/notifications.png"
    "icons/wearable/36x36@2/settings.png"
    "icons/wearable/36x36@2/weather.png"
    "icons/wearable/36x36@2/worldclock.png"
    "icons/wearable/index.theme"
    "images/back.png"
    "images/back@2x.png"
    "images/background-dark.png"
    "images/background-light.png"
    "images/home.png"
    "images/home@2x.png"
    "qml/Alarms/AlarmsPage.qml"
    "qml/DemoMode.qml"
    "qml/DemoModeIndicator.qml"
    "qml/Fitness/FitnessPage.qml"
    "qml/Fitness/fitness.js"
    "qml/Fitness/images/man-running-dark.png"
    "qml/Fitness/images/man-running-dark@2x.png"
    "qml/Fitness/images/man-running-light.png"
    "qml/Fitness/images/man-running-light@2x.png"
    "qml/Fitness/images/man-walking-dark.png"
    "qml/Fitness/images/man-walking-dark@2x.png"
    "qml/Fitness/images/man-walking-light.png"
    "qml/Fitness/images/man-walking-light@2x.png"
    "qml/LauncherPage.qml"
    "qml/NaviButton.qml"
    "qml/Navigation/NavigationPage.qml"
    "qml/Navigation/RouteElement.qml"
    "qml/Navigation/images/end.png"
    "qml/Navigation/images/end@2x.png"
    "qml/Navigation/images/leftturn-dark.png"
    "qml/Navigation/images/leftturn-dark@2x.png"
    "qml/Navigation/images/leftturn-light.png"
    "qml/Navigation/images/leftturn-light@2x.png"
    "qml/Navigation/images/navigation-dark.png"
    "qml/Navigation/images/navigation-dark@2x.png"
    "qml/Navigation/images/navigation-light.png"
    "qml/Navigation/images/navigation-light@2x.png"
    "qml/Navigation/images/rightturn-dark.png"
    "qml/Navigation/images/rightturn-dark@2x.png"
    "qml/Navigation/images/rightturn-light.png"
    "qml/Navigation/images/rightturn-light@2x.png"
    "qml/Navigation/images/start.png"
    "qml/Navigation/images/start@2x.png"
    "qml/Navigation/images/straight-dark.png"
    "qml/Navigation/images/straight-dark@2x.png"
    "qml/Navigation/images/straight-light.png"
    "qml/Navigation/images/straight-light@2x.png"
    "qml/Navigation/images/uturn.png"
    "qml/Navigation/images/uturn@2x.png"
    "qml/Navigation/navigation.js"
    "qml/Navigation/walk_route.json"
    "qml/Notifications/NotificationsPage.qml"
    "qml/Notifications/images/avatarf-dark.png"
    "qml/Notifications/images/avatarf-dark@2x.png"
    "qml/Notifications/images/avatarf-light.png"
    "qml/Notifications/images/avatarf-light@2x.png"
    "qml/Notifications/images/avatarm-dark.png"
    "qml/Notifications/images/avatarm-dark@2x.png"
    "qml/Notifications/images/avatarm-light.png"
    "qml/Notifications/images/avatarm-light@2x.png"
    "qml/Notifications/images/missedcall-dark.png"
    "qml/Notifications/images/missedcall-dark@2x.png"
    "qml/Notifications/images/missedcall-light.png"
    "qml/Notifications/images/missedcall-light@2x.png"
    "qml/Notifications/notifications.js"
    "qml/Settings/SettingsPage.qml"
    "qml/Settings/images/bluetooth-dark.png"
    "qml/Settings/images/bluetooth-dark@2x.png"
    "qml/Settings/images/bluetooth-light.png"
    "qml/Settings/images/bluetooth-light@2x.png"
    "qml/Settings/images/brightness-dark.png"
    "qml/Settings/images/brightness-dark@2x.png"
    "qml/Settings/images/brightness-light.png"
    "qml/Settings/images/brightness-light@2x.png"
    "qml/Settings/images/demo-mode-dark.png"
    "qml/Settings/images/demo-mode-dark@2x.png"
    "qml/Settings/images/demo-mode-light.png"
    "qml/Settings/images/demo-mode-light@2x.png"
    "qml/Settings/images/demo-mode-white.png"
    "qml/Settings/images/demo-mode-white@2x.png"
    "qml/Settings/images/theme-dark.png"
    "qml/Settings/images/theme-dark@2x.png"
    "qml/Settings/images/theme-light.png"
    "qml/Settings/images/theme-light@2x.png"
    "qml/Settings/images/wifi-dark.png"
    "qml/Settings/images/wifi-dark@2x.png"
    "qml/Settings/images/wifi-light.png"
    "qml/Settings/images/wifi-light@2x.png"
    "qml/Style/PageIndicator.qml"
    "qml/Style/Slider.qml"
    "qml/Style/Switch.qml"
    "qml/Style/UIStyle.qml"
    "qml/Style/qmldir"
    "qml/SwipeViewPage.qml"
    "qml/Weather/WeatherPage.qml"
    "qml/Weather/images/humidity-dark.png"
    "qml/Weather/images/humidity-dark@2x.png"
    "qml/Weather/images/humidity-light.png"
    "qml/Weather/images/humidity-light@2x.png"
    "qml/Weather/images/pressure-dark.png"
    "qml/Weather/images/pressure-dark@2x.png"
    "qml/Weather/images/pressure-light.png"
    "qml/Weather/images/pressure-light@2x.png"
    "qml/Weather/images/sunrise-dark.png"
    "qml/Weather/images/sunrise-dark@2x.png"
    "qml/Weather/images/sunrise-light.png"
    "qml/Weather/images/sunrise-light@2x.png"
    "qml/Weather/images/sunset-dark.png"
    "qml/Weather/images/sunset-dark@2x.png"
    "qml/Weather/images/sunset-light.png"
    "qml/Weather/images/sunset-light@2x.png"
    "qml/Weather/images/temperature-dark.png"
    "qml/Weather/images/temperature-dark@2x.png"
    "qml/Weather/images/temperature-light.png"
    "qml/Weather/images/temperature-light@2x.png"
    "qml/Weather/images/wind-dark.png"
    "qml/Weather/images/wind-dark@2x.png"
    "qml/Weather/images/wind-light.png"
    "qml/Weather/images/wind-light@2x.png"
    "qml/Weather/weather.js"
    "qml/Weather/weather.json"
    "qml/WorldClock/Clock.qml"
    "qml/WorldClock/WorldClockPage.qml"
    "qml/WorldClock/images/center.png"
    "qml/WorldClock/images/center@2x.png"
    "qml/WorldClock/images/second.png"
    "qml/WorldClock/images/second@2x.png"
    "qml/WorldClock/images/swissdaydial.png"
    "qml/WorldClock/images/swissdaydial@2x.png"
    "qml/WorldClock/images/swissdayhour.png"
    "qml/WorldClock/images/swissdayhour@2x.png"
    "qml/WorldClock/images/swissdayminute.png"
    "qml/WorldClock/images/swissdayminute@2x.png"
    "qml/WorldClock/images/swissnightdial.png"
    "qml/WorldClock/images/swissnightdial@2x.png"
    "qml/WorldClock/images/swissnighthour.png"
    "qml/WorldClock/images/swissnighthour@2x.png"
    "qml/WorldClock/images/swissnightminute.png"
    "qml/WorldClock/images/swissnightminute@2x.png"
    "wearable.qml"
)

qt6_add_resources(wearable "wearable"
    PREFIX
        "/"
    FILES
        ${wearable_resource_files}
)

install(TARGETS wearable
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
