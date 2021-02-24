file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlxmllistmodel
    SOURCES
        ../../shared/util.cpp ../../shared/util.h
        tst_qqmlxmllistmodel.cpp
    INCLUDE_DIRECTORIES
        ../../shared
    LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::QmlXmlListModelPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qqmlxmllistmodel CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qqmlxmllistmodel CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
