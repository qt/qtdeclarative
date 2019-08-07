# Generated from qmlplugindump.pro.

#####################################################################
## tst_qmlplugindump Test:
#####################################################################

add_qt_test(tst_qmlplugindump
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qmlplugindump.cpp
    DEFINES
        QT_QMLTEST_DIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 2:.:.:tst_qmlplugindump.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:tst_qmlplugindump.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qmlplugindump CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qmlplugindump CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
add_subdirectory(data/dumper/Dummy)
add_subdirectory(data/dumper/Imports)
add_subdirectory(data/dumper/Versions)
add_subdirectory(data/dumper/ExtendedType)
