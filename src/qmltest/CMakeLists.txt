#####################################################################
## QuickTest Module:
#####################################################################

set_source_files_properties(testlogger.js PROPERTIES
    QT_QML_SKIP_QMLDIR_ENTRY TRUE
)

qt_internal_add_qml_module(QuickTest
    URI "QtTest"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 1
    DEPENDENCIES
        QtQuick.Window
    CONFIG_MODULE_NAME qmltest
    QMAKE_MODULE_CONFIG $${QT.testlib.CONFIG}
    SOURCES
        qtestoptions_p.h
        quicktest.cpp quicktest.h quicktest_p.h
        quicktestevent.cpp quicktestevent_p.h
        quicktestglobal.h quicktestglobal_p.h
        quicktestresult.cpp quicktestresult_p.h
        quicktestutil.cpp quicktestutil_p.h
    QML_FILES
        TestCase.qml
        SignalSpy.qml
        testlogger.js
    DEFINES
        QT_NO_FOREACH
        QT_NO_URL_CAST_FROM_STRING
        QT_BUILD_QMLTEST_LIB
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickPrivate
        Qt::TestPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Test
    PRIVATE_MODULE_INTERFACE
        Qt::TestPrivate
)

qt_internal_extend_target(QuickTest CONDITION QT_FEATURE_qml_debug
    DEFINES
        QT_QML_DEBUG_NO_WARNING
)
qt_internal_add_docs(QuickTest
    doc/qtqmltest.qdocconf
)
