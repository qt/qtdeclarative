qt_internal_add_test(tst_qmltc_qprocess
    SOURCES
        tst_qmltc_qprocess.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::QuickTestUtilsPrivate
)
qt6_add_qml_module(tst_qmltc_qprocess
    VERSION 1.0
    URI QmltcQProcessTests
    QML_FILES
        data/dummy.qml
        data/inlineComponent.qml
        data/SingletonThing.qml
        data/erroneousFile.qml
)
# special setup for singleton files:
set_source_files_properties(SingletonThing.qml PROPERTIES QT_QML_SINGLETON_TYPE true)

add_dependencies(tst_qmltc_qprocess Qt::qmltc)

# fetch --resource arguments manually (mimics the logic of qmltc compilation
# command)
_qt_internal_genex_getjoinedproperty(qrc_args tst_qmltc_qprocess
    _qt_generated_qrc_files "" "_::_"
)
target_compile_definitions(tst_qmltc_qprocess PRIVATE
    TST_QMLTC_QPROCESS_RESOURCES="${qrc_args}"
)

qt_internal_extend_target(tst_qmltc_qprocess CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\":/data\\\"
)

qt_internal_extend_target(tst_qmltc_qprocess CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\\"
)
