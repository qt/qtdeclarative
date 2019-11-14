# Generated from qquickframebufferobject.pro.

#####################################################################
## tst_qquickframebufferobject Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qquickframebufferobject
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qquickframebufferobject.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickframebufferobject.pro:<TRUE>:
# OTHER_FILES = "data/testStuff.qml"

## Scopes:
#####################################################################

qt_extend_target(tst_qquickframebufferobject CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_extend_target(tst_qquickframebufferobject CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
