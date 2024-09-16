# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_internal_add_example(shapes)
qt_internal_add_example(weatherforecast)

# qt_examples_build_end() misses at least some of these due to some
# source subdirectories being added multiple times. See QTBUG-96159.
set(reused_dir_targets shapes_shared weatherforecast_shared)

foreach(target IN LISTS reused_dir_targets)
    if(TARGET ${target})
        qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS moc rcc)
        if(TARGET Qt6::Widgets)
            qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS uic)
        endif()
    endif()
endforeach()
