# Generated from qtquickcontrols2.pro.

cmake_minimum_required(VERSION 3.15.0)

include(.cmake.conf)
project(QtQuickControls2
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt QuickControls2 Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

# special case begin
# Make sure we only use latest private CMake API, aka no compatibility wrappers.
set(QT_NO_INTERNAL_COMPATIBILITY_FUNCTIONS TRUE)
# special case end

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Gui Network Qml Quick) # special case
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Widgets QuickTest) # special case
if(NOT TARGET Qt::Quick)
    message(NOTICE "Skipping the build as the condition \"TARGET Qt::Quick\" is not met.")
    return()
endif()
qt_build_repo()
