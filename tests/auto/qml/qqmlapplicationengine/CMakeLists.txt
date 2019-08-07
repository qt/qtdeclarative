# Generated from qqmlapplicationengine.pro.

#####################################################################
## tst_qqmlapplicationengine Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmlapplicationengine
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlapplicationengine.cpp
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
add_qt_resource(tst_qqmlapplicationengine "tst_qqmlapplicationengine"
    PREFIX
        "/"
    FILES
        data/i18n/qml.qm
        data/loadTranslation.qml
)


#### Keys ignored in scope 2:.:.:tst_qqmlapplicationengine.pro:<TRUE>:
# CONFIG = "testcase"
# TRANSLATIONS = "data/i18n/qml_ja.ts"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:tst_qqmlapplicationengine.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmlapplicationengine CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmlapplicationengine CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
add_subdirectory(testapp)
