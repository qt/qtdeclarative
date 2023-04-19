# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_library(Base STATIC)
qt_add_qml_module(Base
    URI "Base"
    QML_FILES
        qml/Red.qml
    SOURCES
        Base.cpp
)

target_link_libraries(Base PRIVATE Qt6::Quick)
