# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(Qt6 REQUIRED COMPONENTS ExamplesAssetDownloaderPrivate)

qt_internal_add_qml_module(QmlAssetDownloader
    URI "Qt.labs.assetdownloader"
    VERSION "${PROJECT_VERSION}"
    STATIC
    SOURCES
        qqmlassetdownloader_p.h
        qqmlassetdownloader.cpp
    LIBRARIES
        Qt6::Qml
        Qt6::ExamplesAssetDownloaderPrivate
    )
