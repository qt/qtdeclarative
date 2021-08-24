qt_internal_add_test(tst_registrationmacros
    SOURCES
        tst_registrationmacros.cpp
        types.h
        types.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::QmlPrivate
)

set_target_properties(tst_registrationmacros PROPERTIES
    QT_QML_MODULE_VERSION 6.2
    QT_QML_MODULE_URI test
)

_qt_internal_qml_type_registration(tst_registrationmacros)
