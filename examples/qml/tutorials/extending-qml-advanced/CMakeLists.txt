# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(NOT ANDROID)
    qt_internal_add_example(advanced1-Base-project)
    qt_internal_add_example(advanced2-Inheritance-and-coercion)
    qt_internal_add_example(advanced3-Default-properties)
endif(NOT ANDROID)
qt_internal_add_example(advanced4-Grouped-properties)
qt_internal_add_example(advanced5-Attached-properties)
qt_internal_add_example(advanced6-Property-value-source)
qt_internal_add_example(advanced7-Extension-objects)
