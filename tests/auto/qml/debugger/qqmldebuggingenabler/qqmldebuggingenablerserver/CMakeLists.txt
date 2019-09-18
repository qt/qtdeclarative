# Generated from qqmldebuggingenablerserver.pro.

#####################################################################
## qqmldebuggingenablerserver Binary:
#####################################################################

add_qt_executable(qqmldebuggingenablerserver
    # special case begin
    # Remove OUTPUT_DIRECTORY and INSTALL_DIRECTORY with $$[QT_INSTALL_TESTS]
    # as these are invalid
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../qqmldebuggingenabler"
    # special case end
    SOURCES
        qqmldebuggingenablerserver.cpp
    DEFINES
        QT_QML_DEBUG_NO_WARNING
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qqmldebuggingenablerserver.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmldebuggingenablerserver.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
