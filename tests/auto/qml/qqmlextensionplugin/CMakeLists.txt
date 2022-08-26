# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlextensionplugin.pro.

#####################################################################
## tst_qqmlextensionplugin Test:
#####################################################################

qt_internal_add_test(tst_qqmlextensionplugin
    SOURCES
        tst_qqmlextensionplugin.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
)

# On Android we need to have some QML file, so that plugins are actually copied
# into the APK
if(ANDROID)
    add_compile_definitions(ANDROID_ARCH="${CMAKE_ANDROID_ARCH_ABI}")
    qt_add_resources(tst_qqmlextensionplugin "dummy_resources"
        PREFIX
            "/"
        FILES
            "./data/dummy.qml"
    )
endif()

## Scopes:
#####################################################################
