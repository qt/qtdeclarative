# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(QuickControlsTestUtilsPrivate
    CONFIG_MODULE_NAME quickcontrolstestutilsprivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        controlstestutils.cpp
        controlstestutils_p.h
        qtest_quickcontrols_p.h
    DEFINES
        QT_BUILD_SHARED_QUICK_CONTROLS_TEST_UTILS_LIB
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Test
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickPrivate
        Qt::QuickTemplates2
        Qt::QuickTemplates2Private
        Qt::QuickTestUtilsPrivate
    NO_GENERATE_CPP_EXPORTS
    QT_LICENSE_ID QT_COMMERCIAL_OR_GPL3
)
qt_internal_extend_target(QuickControlsTestUtilsPrivate CONDITION QT_FEATURE_quicktemplates2_container
    SOURCES
        dialogstestutils.cpp
        dialogstestutils_p.h
    PUBLIC_LIBRARIES
        Qt::QuickDialogs2QuickImplPrivate
)


# This is used by both C++ and QML tests, so we need it to be a library and a QML plugin,
# hence qt_internal_add_qml_module. We use it in addition to qt_internal_add_module,
# because otherwise syncqt complains that there is no "QtQuickControlsTestUtilsPrivate" module.
qt_internal_add_qml_module(QuickControlsTestUtilsPrivate
    URI "Qt.test.controls"
    VERSION "${PROJECT_VERSION}"
)
