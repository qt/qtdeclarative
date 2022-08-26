# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from httpserver.pro.

#####################################################################
## httpserver Binary:
#####################################################################

qt_internal_add_manual_test(qml_httpserver_manual # special case
    SOURCES
        ../../auto/shared/testhttpserver.cpp ../../auto/shared/testhttpserver.h
        main.cpp
    INCLUDE_DIRECTORIES
        ../../auto/shared
    LIBRARIES
        Qt::Network
        Qt::Test # special case
        Qt::Qml # special case
)

#### Keys ignored in scope 1:.:.:httpserver.pro:<TRUE>:
# TEMPLATE = "app"
