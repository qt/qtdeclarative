# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_how-to-cpp-enum-js
    SOURCES
        backend.cpp
        backend.h
        tst_how-to-cpp-enum-js.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
)

qt_add_resources(tst_how-to-cpp-enum-js "js"
    PREFIX
        /
    FILES
        script.mjs
)
