# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_internal_add_example(chapter1)
qt_internal_add_example(chapter2)
qt_internal_add_example(chapter3)
if (TARGET Qt6::Sql)
    qt_internal_add_example(chapter4)
    if(TARGET chattutorial-chapter4)
        set_target_properties(chattutorial-chapter4 PROPERTIES UNITY_BUILD OFF)
    endif()
    qt_internal_add_example(chapter5)
    if(TARGET chattutorial-chapter5)
        set_target_properties(chattutorial-chapter5 PROPERTIES UNITY_BUILD OFF)
    endif()
endif()
