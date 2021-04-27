qt_internal_add_module(QmlLocalStorage
    SOURCES
        qqmllocalstorage.cpp qqmllocalstorage_p.h
        qqmllocalstorageglobal_p.h
    DEFINES
        QT_BUILD_QMLLOCALSTORAGE_LIB
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::Sql
)

set_target_properties(QmlLocalStorage PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick.LocalStorage
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQuick/LocalStorage"
)


qt6_qml_type_registration(QmlLocalStorage)
