# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_policy(SET QTP0001 NEW)

qt_internal_add_test(tst_qml_common
    SOURCES
        tst_qml_common.cpp tst_qml_common.h
    LIBRARIES
        Qt::Qml
        Qt::QmlPrivate
)
