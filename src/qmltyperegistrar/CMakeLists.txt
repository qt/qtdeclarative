# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qmltyperegistrarprivate:
#####################################################################

qt_internal_add_module(QmlTypeRegistrarPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qanystringviewutils_p.h
        qmetatypesjsonprocessor.cpp qmetatypesjsonprocessor_p.h
        qqmljsstreamwriter.cpp qqmljsstreamwriter_p.h
        qqmltyperegistrar.cpp qqmltyperegistrar_p.h
        qqmltyperegistrarconstants_p.h
        qqmltypesclassdescription.cpp qqmltypesclassdescription_p.h
        qqmltypescreator.cpp qqmltypescreator_p.h
        qqmltyperegistrarutils_p.h qqmltyperegistrarutils.cpp
    NO_UNITY_BUILD_SOURCES
        qqmltypescreator.cpp # S_NAME etc clash
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    )

qt_internal_add_resource(QmlTypeRegistrarPrivate "jsRootMetaTypes"
    PREFIX
        "/qt-project.org/meta_types"
    FILES
        jsroot_metatypes.json
)
