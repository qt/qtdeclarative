# Generated from signalspy.pro.

#####################################################################
## tst_signalspy Test:
#####################################################################

add_qt_test(tst_signalspy
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        mypropertymap.cpp mypropertymap.h
        tst_signalspy.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
)

## Scopes:
#####################################################################

extend_target(tst_signalspy CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_signalspy CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
