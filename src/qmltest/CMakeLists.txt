# Generated from qmltest.pro.

#####################################################################
## QuickTest Module:
#####################################################################

add_qt_module(QuickTest
    QMAKE_MODULE_CONFIG $${QT.testlib.CONFIG}
    SOURCES
        qtestoptions_p.h
        quicktest.cpp quicktest.h
        quicktestevent.cpp quicktestevent_p.h
        quicktestglobal.h
        quicktestresult.cpp quicktestresult_p.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_URL_CAST_FROM_STRING
    LIBRARIES
        Qt::TestPrivate
        Qt::Quick # special case
        Qt::QuickPrivate # special case
        Qt::QmlPrivate # special case
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qmltest.pro:<TRUE>:
# MODULE_CONFIG = "$${QT.testlib.CONFIG}"
# QT_PRIVATE = "quick" "quick-private" "qml-private" "gui" "core-private" "gui-private"
# _LOADED = "qt_module"

## Scopes:
#####################################################################

extend_target(QuickTest CONDITION TARGET Qt::Widgets
    DEFINES
        QT_QMLTEST_WITH_WIDGETS
    PUBLIC_LIBRARIES
        Qt::Widgets
)

extend_target(QuickTest CONDITION QT_FEATURE_qml_debug
    DEFINES
        QT_QML_DEBUG_NO_WARNING
)
add_qt_docs(
    doc/qtqmltest.qdocconf
)
