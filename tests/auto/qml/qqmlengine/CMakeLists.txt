# Generated from qqmlengine.pro.

#####################################################################
## tst_qqmlengine Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmlengine
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlengine.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
    TESTDATA ${test_data}
)

# Resources:
set(qmake_immediate_resource_files
    "data/qrcurls.js"
    "data/qrcurls.qml"
)

add_qt_resource(tst_qqmlengine "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


## Scopes:
#####################################################################

extend_target(tst_qqmlengine CONDITION boot2qt
    DEFINES
        SKIP_GCCORRUPTION_TEST
)

extend_target(tst_qqmlengine CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlengine CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
