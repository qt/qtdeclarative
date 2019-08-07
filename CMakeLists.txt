cmake_minimum_required(VERSION 3.14.0)

project(QtDeclarative
    VERSION 6.0.0
    DESCRIPTION "Qt Declarative Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Gui Widgets Network)
find_package(Qt6 ${PROJECT_VERSION} CONFIG COMPONENTS Sql)
qt_build_repo()
