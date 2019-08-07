# Generated from creation.pro.

#####################################################################
## tst_creation Binary:
#####################################################################

add_qt_executable(tst_creation
    GUI
    SOURCES
        tst_creation.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Test
)

#### Keys ignored in scope 1:.:.:creation.pro:<TRUE>:
# CONFIG = "benchmark"
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:creation.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
