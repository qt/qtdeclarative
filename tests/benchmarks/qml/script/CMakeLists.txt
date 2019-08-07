# Generated from script.pro.

#####################################################################
## tst_script Binary:
#####################################################################

add_qt_executable(tst_script
    GUI
    SOURCES
        tst_script.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
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

#### Keys ignored in scope 1:.:.:script.pro:<TRUE>:
# CONFIG = "benchmark" "release"
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:script.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
