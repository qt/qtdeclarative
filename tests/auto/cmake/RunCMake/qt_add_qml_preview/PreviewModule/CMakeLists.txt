# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Its generated .qrc is what qt_add_qml_preview() hands to the qmlpreview
# target of an executable that only links against it.
qt_add_library(PreviewModule STATIC)
qt_add_qml_module(PreviewModule
    URI PreviewModule
    VERSION 1.0
    QML_FILES Thing.qml
)
