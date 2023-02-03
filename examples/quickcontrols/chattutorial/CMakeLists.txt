# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_example(chapter1)
qt_internal_add_example(chapter2)
qt_internal_add_example(chapter3)
if (TARGET Qt::Sql)
    qt_internal_add_example(chapter4)
    set_target_properties(chattutorial-chapter4 PROPERTIES UNITY_BUILD OFF)
    qt_internal_add_example(chapter5)
    set_target_properties(chattutorial-chapter5 PROPERTIES UNITY_BUILD OFF)
endif()
