# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(LabsFolderListModel
    URI "Qt.labs.folderlistmodel"
    VERSION "${PROJECT_VERSION}"
    DEPENDENCIES
        QtQml.Models/auto
    PLUGIN_TARGET qmlfolderlistmodelplugin
    CLASS_NAME QmlFolderListModelPlugin
    SOURCES
        fileinfothread.cpp fileinfothread_p.h
        fileproperty_p.h
        qquickfolderlistmodel.cpp qquickfolderlistmodel_p.h
        qquickfolderlistmodelglobal_p.h
    DEFINES
        QT_BUILD_LABSFOLDERMODEL_LIB
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::QmlModelsPrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)
