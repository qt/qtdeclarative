# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_internal_add_example(rendercontrol_opengl)
if(WIN32)
    qt_internal_add_example(rendercontrol_d3d11)
endif()
if(TARGET Qt6::Widgets)
    qt_internal_add_example(rendercontrol_rhi)
endif()
