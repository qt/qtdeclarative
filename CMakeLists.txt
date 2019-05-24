cmake_minimum_required(VERSION 3.14.0)

project(QtDeclarative
    VERSION 5.14.0
    DESCRIPTION "Qt Declarative Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt5 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Gui Widgets Network)
qt_build_repo()
