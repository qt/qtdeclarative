# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QmlModels Module:
#####################################################################

qt_internal_add_qml_module(QmlModels
    URI "QtQml.Models"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    PLUGIN_TARGET modelsplugin
    CLASS_NAME QtQmlModelsPlugin
    DEPENDENCIES
        QtQml.Base/auto
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
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::QmlPrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

qt_internal_extend_target(QmlModels CONDITION QT_FEATURE_qml_itemmodel
    SOURCES
        qqmlmodelindexvaluetype.cpp qqmlmodelindexvaluetype_p.h
)

qt_internal_extend_target(QmlModels CONDITION QT_FEATURE_qml_object_model
    SOURCES
        qqmlinstantiator.cpp qqmlinstantiator_p.h
        qqmlinstantiator_p_p.h
        qqmlobjectmodel.cpp qqmlobjectmodel_p.h
)

qt_internal_extend_target(QmlModels CONDITION QT_FEATURE_qml_table_model
    SOURCES
        qqmltableinstancemodel.cpp qqmltableinstancemodel_p.h
        qqmltreemodeltotablemodel.cpp qqmltreemodeltotablemodel_p_p.h
)

qt_internal_extend_target(QmlModels CONDITION QT_FEATURE_qml_list_model
    SOURCES
        qqmllistmodel.cpp qqmllistmodel_p.h
        qqmllistmodel_p_p.h
        qqmllistmodelworkeragent.cpp qqmllistmodelworkeragent_p.h
)

qt_internal_extend_target(QmlModels CONDITION QT_FEATURE_qml_delegate_model
    SOURCES
        qqmlabstractdelegatecomponent.cpp qqmlabstractdelegatecomponent_p.h
        qqmladaptormodel.cpp qqmladaptormodel_p.h
        qqmladaptormodelenginedata.cpp qqmladaptormodelenginedata_p.h
        qqmldelegatemodel.cpp qqmldelegatemodel_p.h
        qqmldelegatemodel_p_p.h
        qqmldmabstractitemmodeldata.cpp qqmldmabstractitemmodeldata_p.h
        qqmldmlistaccessordata.cpp qqmldmlistaccessordata_p.h
        qqmldmobjectdata.cpp qqmldmobjectdata_p.h
        qqmllistaccessor.cpp qqmllistaccessor_p.h
        qqmllistcompositor.cpp qqmllistcompositor_p.h
        qquickpackage.cpp qquickpackage_p.h
)
qt_internal_add_docs(QmlModels
    doc/qtqmlmodels.qdocconf
)
