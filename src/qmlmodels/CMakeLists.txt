# Generated from qmlmodels.pro.

#####################################################################
## QmlModels Module:
#####################################################################

add_qt_module(QmlModels
    SOURCES
        qqmlchangeset.cpp qqmlchangeset_p.h
        qqmlmodelsmodule.cpp qqmlmodelsmodule_p.h
        qtqmlmodelsglobal.h qtqmlmodelsglobal_p.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_INTEGER_EVENT_COORDINATES
        QT_NO_URL_CAST_FROM_STRING
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qmlmodels.pro:<TRUE>:
# _LOADED = "qt_module"

## Scopes:
#####################################################################

extend_target(QmlModels CONDITION QT_FEATURE_qml_object_model
    SOURCES
        qqmlinstantiator.cpp qqmlinstantiator_p.h
        qqmlinstantiator_p_p.h
        qqmlobjectmodel.cpp qqmlobjectmodel_p.h
)

extend_target(QmlModels CONDITION QT_FEATURE_qml_table_model
    SOURCES
        qqmltableinstancemodel.cpp qqmltableinstancemodel_p.h
        qqmltablemodel.cpp qqmltablemodel_p.h
        qqmltablemodelcolumn.cpp qqmltablemodelcolumn_p.h
)

extend_target(QmlModels CONDITION QT_FEATURE_qml_list_model
    SOURCES
        qqmllistmodel.cpp qqmllistmodel_p.h
        qqmllistmodel_p_p.h
        qqmllistmodelworkeragent.cpp qqmllistmodelworkeragent_p.h
)

extend_target(QmlModels CONDITION QT_FEATURE_qml_delegate_model
    SOURCES
        qqmladaptormodel.cpp qqmladaptormodel_p.h
        qqmldelegatecomponent.cpp qqmldelegatecomponent_p.h
        qqmldelegatemodel.cpp qqmldelegatemodel_p.h
        qqmldelegatemodel_p_p.h
        qqmllistaccessor.cpp qqmllistaccessor_p.h
        qqmllistcompositor.cpp qqmllistcompositor_p.h
        qquickpackage.cpp qquickpackage_p.h
)
