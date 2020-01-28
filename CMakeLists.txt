cmake_minimum_required(VERSION 3.15.0)

project(QtDeclarative
    VERSION 6.0.0
    DESCRIPTION "Qt Declarative Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Gui Widgets Network OpenGL # special case
                                           OPTIONAL_COMPONENTS Sql Concurrent Test) # special case

# export QT6_ADD_QML_MODULE to this project
include(src/qml/Qt6QmlMacros.cmake)

qt_build_repo()
