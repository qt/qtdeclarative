# Generated from shell.pro.

#####################################################################
## shell Binary:
#####################################################################

add_qt_executable(shell
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qml/shell"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qml/shell"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Qml
)

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:shell.pro:WIN32:
# CONFIG = "console"

#### Keys ignored in scope 3:.:.:shell.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
