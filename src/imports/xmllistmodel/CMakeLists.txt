qt_internal_add_qml_module(qmlxmllistmodelplugin
    URI "QtQml.XmlListModel"
    VERSION "${CMAKE_PROJECT_VERSION}"
    CLASSNAME QtQmlXmlListModelPlugin
    SKIP_TYPE_REGISTRATION
    DEPENDENCIES
        QtQml
    PLUGIN_OPTIONAL
    SOURCES
        plugin.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::QmlXmlListModelPrivate
)
