# Generated from qquickscreen.pro.

#####################################################################
## tst_qquickscreen Test:
#####################################################################

qt_internal_add_test(tst_qquickscreen
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickscreen.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickscreen CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qquickscreen CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
