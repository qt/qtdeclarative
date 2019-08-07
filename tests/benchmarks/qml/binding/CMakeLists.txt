# Generated from binding.pro.

#####################################################################
## tst_binding Binary:
#####################################################################

add_qt_executable(tst_binding
    GUI
    SOURCES
        testtypes.cpp testtypes.h
        tst_binding.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:binding.pro:<TRUE>:
# CONFIG = "benchmark"
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:binding.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
