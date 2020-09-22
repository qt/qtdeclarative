# Generated from qqmldebugjsserver.pro.

#####################################################################
## qqmldebugjsserver Binary:
#####################################################################

qt_internal_add_executable(qqmldebugjsserver
    # special case begin
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    # INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/qqmldebugjsserver"
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
