# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_example(adding)
qt_internal_add_example(coercion)
qt_internal_add_example(default)
qt_internal_add_example(properties)
qt_internal_add_example(methods)
if(TARGET Qt::Widgets)
    qt_internal_add_example(extended)
endif()
if(TARGET Qt::Quick)
    qt_internal_add_example(attached)
    qt_internal_add_example(binding)
    qt_internal_add_example(grouped)
    qt_internal_add_example(signal)
    qt_internal_add_example(valuesource)
endif()
