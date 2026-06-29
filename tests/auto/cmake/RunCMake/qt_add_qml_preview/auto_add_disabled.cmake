# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# auto_add, but opted out per target via QT_QML_NO_QML_PREVIEW: no preview target.
include(${CMAKE_CURRENT_LIST_DIR}/auto_add.cmake)

set_target_properties(app PROPERTIES QT_QML_NO_QML_PREVIEW ON)
