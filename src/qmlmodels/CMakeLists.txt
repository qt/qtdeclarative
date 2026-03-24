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
        QML/1.0
    SOURCES
        qqmlchangeset.cpp qqmlchangeset_p.h
        qqmlmodelsmodule.cpp qqmlmodelsmodule_p.h
        qtqmlmodelsglobal.h qtqmlmodelsglobal_p.h
    DEFINES
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
        qqmldelegatecomponent.cpp qqmldelegatecomponent_p.h
        qqmldelegatemodel.cpp qqmldelegatemodel_p.h
        qqmldelegatemodel_p_p.h
        qqmldmabstractitemmodeldata.cpp qqmldmabstractitemmodeldata_p.h
        qqmldmlistaccessordata.cpp qqmldmlistaccessordata_p.h
        qqmldmobjectdata.cpp qqmldmobjectdata_p.h
        qqmllistaccessor.cpp qqmllistaccessor_p.h
        qqmllistcompositor.cpp qqmllistcompositor_p.h
        qquickpackage.cpp qquickpackage_p.h
)

qt_internal_extend_target(QmlModels CONDITION QT_FEATURE_qml_sfpm_model AND QT_FEATURE_proxymodel
    SOURCES
        sfpm/qqmlsortfilterproxymodel_p.h sfpm/qqmlsortfilterproxymodel.cpp
        sfpm/qqmlsortfilterproxymodel_p_p.h sfpm/qsortfilterproxymodelhelper_p.h sfpm/qsortfilterproxymodelhelper.cpp
        # sfpm filter specific source files
        sfpm/filters/qqmlfilterbase_p.h sfpm/filters/qqmlfilterbase.cpp
        sfpm/filters/qqmlfiltercompositor_p.h sfpm/filters/qqmlfiltercompositor.cpp
        sfpm/filters/qqmlrolefilter_p.h sfpm/filters/qqmlrolefilter.cpp
        sfpm/filters/qqmlvaluefilter_p.h sfpm/filters/qqmlvaluefilter.cpp
        sfpm/filters/qqmlfunctionfilter_p.h sfpm/filters/qqmlfunctionfilter.cpp
        # sfpm sorter specific source files
        sfpm/sorters/qqmlsorterbase_p.h sfpm/sorters/qqmlsorterbase.cpp
        sfpm/sorters/qqmlsortercompositor_p.h sfpm/sorters/qqmlsortercompositor.cpp
        sfpm/sorters/qqmlstringsorter_p.h sfpm/sorters/qqmlstringsorter.cpp
        sfpm/sorters/qqmlrolesorter_p.h sfpm/sorters/qqmlrolesorter.cpp
        sfpm/sorters/qqmlfunctionsorter_p.h sfpm/sorters/qqmlfunctionsorter.cpp
)

qt_internal_add_docs(QmlModels
    doc/qtqmlmodels.qdocconf
)
