# Generated from compilation.pro.

#####################################################################
## tst_compilation Binary:
#####################################################################

add_qt_executable(tst_compilation
    GUI
    SOURCES
        tst_compilation.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:compilation.pro:<TRUE>:
# CONFIG = "benchmark" "release"
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:compilation.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
