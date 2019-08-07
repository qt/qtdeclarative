# Generated from qqmltranslation.pro.

#####################################################################
## tst_qqmltranslation Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_qqmltranslation
    GUI
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmltranslation.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
    TESTDATA ${test_data}
)

# Resources:
# special case begin
set_source_files_properties(
    data/translation.qml
    PROPERTIES
    QT_SKIP_QUICKCOMPILER 1
    alias translation.qml
)
set_source_files_properties(
    data/qml_fr.qm
    PROPERTIES
    alias qml_fr.qm
)
# special case end
add_qt_resource(tst_qqmltranslation "translation"
    PREFIX
        "/"
    BASE
        "data"
    FILES
        qml_fr.qm
        translation.qml
)


#### Keys ignored in scope 1:.:.:qqmltranslation.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qqmltranslation.pro:APPLE_OSX:
# CONFIG = "-app_bundle"

extend_target(tst_qqmltranslation CONDITION ANDROID OR APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

extend_target(tst_qqmltranslation CONDITION NOT ANDROID AND NOT APPLE_IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
