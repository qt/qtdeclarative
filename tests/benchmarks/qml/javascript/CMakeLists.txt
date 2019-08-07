# Generated from javascript.pro.

#####################################################################
## tst_javascript Binary:
#####################################################################

add_qt_executable(tst_javascript
    GUI
    SOURCES
        testtypes.cpp testtypes.h
        tst_javascript.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:javascript.pro:<TRUE>:
# CONFIG = "benchmark"
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:javascript.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
