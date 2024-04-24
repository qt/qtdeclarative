# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qqmlstandardpaths)
add_subdirectory(qqmlsysteminformation)
if(QT_FEATURE_settings)
    add_subdirectory(qqmlsettings)
endif()
