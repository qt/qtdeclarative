# Generated from qqmlengine.pro.

#####################################################################
## tst_qqmlengine Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlengine
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlengine.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
        Qt::QmlPrivate
    TESTDATA ${test_data}
)

# Resources:
set(qmake_immediate_resource_files
    "data/qrcurls.js"
    "data/qrcurls.qml"
)

qt_internal_add_resource(tst_qqmlengine "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlengine CONDITION boot2qt
    DEFINES
        SKIP_GCCORRUPTION_TEST
)

qt_internal_extend_target(tst_qqmlengine CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qqmlengine CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
