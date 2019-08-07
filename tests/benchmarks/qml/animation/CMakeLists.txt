# Generated from animation.pro.

#####################################################################
## tst_animation Binary:
#####################################################################

add_qt_executable(tst_animation
    GUI
    SOURCES
        tst_animation.cpp
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

#### Keys ignored in scope 1:.:.:animation.pro:<TRUE>:
# CONFIG = "benchmark"
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:animation.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
