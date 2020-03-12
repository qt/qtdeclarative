# Generated from qtdeclarative.pro.

cmake_minimum_required(VERSION 3.15.0)

project(QtDeclarative # special case
    VERSION 6.0.0
    DESCRIPTION "Qt Declarative Libraries" # special case
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Gui Widgets Network # special case
                                           OPTIONAL_COMPONENTS OpenGL Sql Concurrent Test) # special case

# special case begin
# export QT6_ADD_QML_MODULE to this project
include(src/qml/Qt6QmlMacros.cmake)
# special case end

if(NOT QT_FEATURE_commandlineparser)
    message(NOTICE "Skipping the build as the condition \"QT_FEATURE_commandlineparser\" is not met.")
    return()
endif()
if(NOT QT_FEATURE_temporaryfile)
    message(NOTICE "Skipping the build as the condition \"QT_FEATURE_temporaryfile\" is not met.")
    return()
endif()
qt_build_repo()
