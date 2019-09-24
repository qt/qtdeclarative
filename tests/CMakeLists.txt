# Generated from tests.pro.

if(NOT TARGET Qt::Test)
    cmake_minimum_required(VERSION 3.15.0)
    project(QtQuickControls2Tests VERSION 6.0.0 LANGUAGES C CXX)
    find_package(Qt6 ${PROJECT_VERSION} REQUIRED COMPONENTS
        BuildInternals Core Qml Quick QmlDevTools QmlDebug QuickTest
        QuickControls2  QuickTemplates2
    )
    qt_set_up_standalone_tests_build()
endif()
qt_build_tests()

