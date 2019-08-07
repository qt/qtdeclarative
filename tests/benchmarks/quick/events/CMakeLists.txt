# Generated from events.pro.

#####################################################################
## tst_qevents Binary:
#####################################################################

add_qt_executable(tst_qevents
    GUI
    SOURCES
        ../../../auto/quick/shared/geometrytestutil.cpp ../../../auto/quick/shared/geometrytestutil.h
        ../../../auto/quick/shared/viewtestutil.cpp ../../../auto/quick/shared/viewtestutil.h
        ../../../auto/quick/shared/visualtestutil.cpp ../../../auto/quick/shared/visualtestutil.h
        ../../../auto/shared/util.cpp ../../../auto/shared/util.h
        tst_events.cpp
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
    INCLUDE_DIRECTORIES
        ../../../auto/shared
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

#### Keys ignored in scope 1:.:.:events.pro:<TRUE>:
# CONFIG = "benchmark"
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:events.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qevents CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qevents CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
