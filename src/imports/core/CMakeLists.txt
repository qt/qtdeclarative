qt_internal_add_qml_module(qtqmlcoreplugin
    URI "QtCore"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    CLASS_NAME QtQmlCorePlugin
    SKIP_TYPE_REGISTRATION
    DEPENDENCIES
        QtQml
    PLUGIN_OPTIONAL
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::QmlCore
        Qt::QmlCorePrivate
)
