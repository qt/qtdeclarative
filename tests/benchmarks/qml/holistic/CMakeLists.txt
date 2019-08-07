# Generated from holistic.pro.

#####################################################################
## tst_holistic Binary:
#####################################################################

add_qt_executable(tst_holistic
    GUI
    SOURCES
        testtypes.cpp testtypes.h
        tst_holistic.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:holistic.pro:<TRUE>:
# CONFIG = "benchmark" "release"
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:holistic.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
