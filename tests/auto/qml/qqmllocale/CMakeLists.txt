# Generated from qqmllocale.pro.

#####################################################################
## tst_qqmllocale Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmllocale
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmllocale.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

extend_target(tst_qqmllocale CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmllocale CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
