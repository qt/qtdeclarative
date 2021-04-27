# Generated from qmltest.pro.

#####################################################################
## QuickTest Module:
#####################################################################

qt_internal_add_module(QuickTest
    CONFIG_MODULE_NAME qmltest
    QMAKE_MODULE_CONFIG $${QT.testlib.CONFIG}
    SOURCES
        qtestoptions_p.h
        quicktest.cpp quicktest.h quicktest_p.h
        quicktestevent.cpp quicktestevent_p.h
        quicktestglobal.h quicktestglobal_p.h
        quicktestresult.cpp quicktestresult_p.h
        quicktestutil.cpp quicktestutil_p.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_URL_CAST_FROM_STRING
        QT_BUILD_QMLTEST_LIB # special case
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

#### Keys ignored in scope 1:.:.:qmltest.pro:<TRUE>:
# MODULE_CONFIG = "$${QT.testlib.CONFIG}"
# QMLTYPES_FILENAME = "plugins.qmltypes"
# QMLTYPES_INSTALL_DIR = "$$[QT_INSTALL_QML]/QtTest"
# QML_IMPORT_NAME = "QtTest"
# QML_IMPORT_VERSION = "$$QT_VERSION"

## Scopes:
#####################################################################

qt_internal_extend_target(QuickTest CONDITION QT_FEATURE_qml_debug
    DEFINES
        QT_QML_DEBUG_NO_WARNING
)
qt_internal_add_docs(QuickTest
    doc/qtqmltest.qdocconf
)


set_target_properties(QuickTest PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI QtTest
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtTest"
)

qt6_qml_type_registration(QuickTest)
