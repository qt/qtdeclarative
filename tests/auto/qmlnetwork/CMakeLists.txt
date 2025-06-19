# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qqmlnetworkinformation)
if(QT_FEATURE_qml_ssl)
    add_subdirectory(qqmlsslconfiguration)
    add_subdirectory(qqmlsslconfiguration_cpp)
    add_subdirectory(qqmlsslkey)
endif()
