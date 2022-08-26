# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(LabsQmlModels
    URI "Qt.labs.qmlmodels"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET labsmodelsplugin
    CLASS_NAME QtQmlLabsModelsPlugin
    SOURCES
        qqmlmodelsglobal_p.h
    DEFINES
        QT_BUILD_LABSQMLMODELS_LIB
    PUBLIC_LIBRARIES
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
    DEPENDENCIES
        QtQml.Models/auto
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

qt_internal_extend_target(LabsQmlModels CONDITION QT_FEATURE_qml_table_model
    SOURCES
        qqmltablemodel.cpp qqmltablemodel_p.h
        qqmltablemodelcolumn.cpp qqmltablemodelcolumn_p.h
)

qt_internal_extend_target(LabsQmlModels CONDITION QT_FEATURE_qml_delegate_model
    SOURCES
        qqmldelegatecomponent.cpp qqmldelegatecomponent_p.h
)
