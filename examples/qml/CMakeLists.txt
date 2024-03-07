# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_subdirectory(tutorials)
if(TARGET Qt6::Quick)
    if (TARGET Qt6::lupdate)
        qt_internal_add_example(qml-i18n)
    endif()
endif()

