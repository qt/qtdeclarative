# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_library(duck_track STATIC)

qt_add_qml_module(duck_track
    URI duck.track
    VERSION 1.0
    QML_FILES
      Track.qml
)

add_dependencies(duck_track duck_trick)
