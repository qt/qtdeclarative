# Generated from signalspy.pro.

#####################################################################
## tst_signalspy Test:
#####################################################################

qt_internal_add_test(tst_signalspy
    SOURCES
        mypropertymap.cpp mypropertymap.h
        tst_signalspy.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickTestUtilsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_signalspy CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_signalspy CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
