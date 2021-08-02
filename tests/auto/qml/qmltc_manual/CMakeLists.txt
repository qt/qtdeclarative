# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmltc_manual
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        testclasses.h
        tst_qmltc_manual.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::Quick
)

qt6_add_qml_module(tst_qmltc_manual
    VERSION 1.0
    URI "QmltcManualTests"
    QML_FILES
        ${test_data}
)

qt_internal_extend_target(tst_qmltc_manual CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qmltc_manual CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
