# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_internal_add_example(shapes)

if(TARGET Qt6::Svg)
    qt_internal_add_example(weatherforecast)
endif()
