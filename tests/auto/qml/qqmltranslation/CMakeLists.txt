# Generated from qqmltranslation.pro.

#####################################################################
## tst_qqmltranslation Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmltranslation
    SOURCES
        tst_qqmltranslation.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

# Resources:
set(translation_resource_files
    "data/qml_fr.qm"
    "data/translation.qml"
)

# special case begin
set_source_files_properties(
    data/translation.qml
    PROPERTIES
    QT_SKIP_QUICKCOMPILER 1
)
# special case end

qt_internal_add_resource(tst_qqmltranslation "translation"
    PREFIX
        "/"
    BASE
        "data"
    FILES
        ${translation_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmltranslation CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qqmltranslation CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
