# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qmltyperegistrarprivate:
#####################################################################

qt_internal_add_module(QmlTypeRegistrarPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qqmljsstreamwriter.cpp qqmljsstreamwriter_p.h
        qmetatypesjsonprocessor.cpp qmetatypesjsonprocessor_p.h
        qqmltyperegistrar.cpp qqmltyperegistrar_p.h
        qqmltypesclassdescription.cpp qqmltypesclassdescription_p.h
        qqmltypescreator.cpp qqmltypescreator_p.h
    NO_UNITY_BUILD_SOURCES
        qqmltypescreator.cpp # S_NAME etc clash
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    PUBLIC_LIBRARIES
        Qt::Core
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)
