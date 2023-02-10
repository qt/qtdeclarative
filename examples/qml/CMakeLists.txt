# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(referenceexamples)
add_subdirectory(tutorials)
qt_internal_add_example(shell)
if(TARGET Qt::Quick)
    qt_internal_add_example(qmlextensionplugins)
    qt_internal_add_example(xmlhttprequest)
    if (TARGET Qt::lupdate)
        qt_internal_add_example(qml-i18n)
    endif()
    qt_internal_add_example(dynamicscene)
endif()

