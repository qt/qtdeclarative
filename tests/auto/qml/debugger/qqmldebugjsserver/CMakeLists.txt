# Generated from qqmldebugjsserver.pro.

#####################################################################
## qqmldebugjsserver Binary:
#####################################################################

# special case begin
# Remove INSTALL_DIRECTORY and INSTALL_DIRECTORY from this test a
# use the one from special case instead
# special case end
add_qt_executable(qqmldebugjsserver
    # special case begin
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    # special case end
    SOURCES
        qqmldebugjsserver.cpp
    DEFINES
        QT_QML_DEBUG_NO_WARNING
    INCLUDE_DIRECTORIES
        ../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmldebugjsserver.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
