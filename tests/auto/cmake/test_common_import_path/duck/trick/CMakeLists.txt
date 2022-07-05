# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_add_library(duck_trick STATIC)

qt_add_qml_module(duck_trick
    URI duck.trick
    VERSION 1.0
    QML_FILES
      Trick.qml
)

