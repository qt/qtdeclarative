# Generated from qqmlmetaobject.pro.

#####################################################################
## tst_qqmlmetaobject Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qqmlmetaobject
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlmetaobject.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_extend_target(tst_qqmlmetaobject CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qqmlmetaobject CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
