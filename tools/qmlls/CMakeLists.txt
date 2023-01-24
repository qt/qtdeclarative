# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qmlls Tool:
#####################################################################

qt_internal_add_app(qmlls
    TARGET_DESCRIPTION "QML Language Server"
    SOURCES
        qmllanguageservertool.cpp
    DEFINES
        QT_USE_QSTRINGBUILDER
    LIBRARIES
        Qt::QmlLSPrivate
        Qt::QmlToolingSettingsPrivate
)
set_target_properties(qmlls PROPERTIES WIN32_EXECUTABLE FALSE)
