# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qqcstylegenerator App:
#####################################################################

qt_internal_add_app(qqcstylegenerator
    TARGET_DESCRIPTION "Quick Controls Style Generator"
    EXCEPTIONS
    SOURCES
        src/main.cpp
    LIBRARIES
        Qt::GuiPrivate
)
