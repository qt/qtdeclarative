# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_settings)
    add_subdirectory(settings)
endif()

add_subdirectory(models)

if(QT_FEATURE_qml_itemmodel)
    add_subdirectory(folderlistmodel)
endif()

if(TARGET Qt::Quick)
    add_subdirectory(animation)
endif()

if(TARGET Qt::QuickTemplates2)
    add_subdirectory(platform)
endif()

if(QT_FEATURE_quick_shadereffect AND TARGET Qt::Quick)
    add_subdirectory(wavefrontmesh)
endif()

if(QT_FEATURE_systemsemaphore AND TARGET Qt::Quick)
    add_subdirectory(sharedimage)
endif()

