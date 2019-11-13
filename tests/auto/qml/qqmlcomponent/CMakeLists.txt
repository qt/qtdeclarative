# Generated from qqmlcomponent.pro.

#####################################################################
## tst_qqmlcomponent Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmlcomponent
    SOURCES
        ../../shared/testhttpserver.cpp ../../shared/testhttpserver.h
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlcomponent.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
        Qt::Quick
    TESTDATA ${test_data}
)

# Resources:
set(qmake_immediate_resource_files
    "data/QtObjectComponent.qml"
)

add_qt_resource(tst_qqmlcomponent "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


## Scopes:
#####################################################################

extend_target(tst_qqmlcomponent CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlcomponent CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
