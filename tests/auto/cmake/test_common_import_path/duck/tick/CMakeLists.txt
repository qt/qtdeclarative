# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_library(duck_tick STATIC)

qt_add_qml_module(duck_tick
    URI duck.tick
    VERSION 1.0
    QML_FILES
      Tick.qml
    RESOURCE_PREFIX /
)

