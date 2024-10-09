# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickanimations.pro.

#####################################################################
## tst_qquickanimations Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickanimations LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickanimations
    SOURCES
        tst_qquickanimations.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlModelsPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickanimations.pro:<TRUE>:
# OTHER_FILES = "data/alwaysRunToEndInSequentialAnimationBug.qml" "data/animationJobSelfDestructionBug.qml" "data/attached.qml" "data/badproperty1.qml" "data/badproperty2.qml" "data/badtype1.qml" "data/badtype2.qml" "data/badtype3.qml" "data/badtype4.qml" "data/disabledTransition.qml" "data/dontAutoStart.qml" "data/dontStart.qml" "data/dontStart2.qml" "data/dotproperty.qml" "data/Double.qml" "data/doubleRegistrationBug.qml" "data/fastFlickingBug.qml" "data/looping.qml" "data/mixedtype1.qml" "data/mixedtype2.qml" "data/nonTransitionBug.qml" "data/parallelAnimationNullChildBug.qml" "data/pathAnimation.qml" "data/pathAnimation2.qml" "data/pathAnimationInOutBackCrash.qml" "data/pathAnimationNoStart.qml" "data/pathInterpolator.qml" "data/pathInterpolatorBack.qml" "data/pathInterpolatorBack2.qml" "data/pathTransition.qml" "data/pauseBindingBug.qml" "data/pauseBug.qml" "data/properties.qml" "data/properties2.qml" "data/properties3.qml" "data/properties4.qml" "data/properties5.qml" "data/propertiesTransition.qml" "data/propertiesTransition2.qml" "data/propertiesTransition3.qml" "data/propertiesTransition4.qml" "data/propertiesTransition5.qml" "data/propertiesTransition6.qml" "data/propertiesTransition7.qml" "data/reanchor.qml" "data/registrationBug.qml" "data/reparent.qml" "data/rotation.qml" "data/runningTrueBug.qml" "data/scriptActionBug.qml" "data/scriptActionCrash.qml" "data/sequentialAnimationNullChildBug.qml" "data/signals.qml" "data/signalorder.qml" "data/transitionAssignmentBug.qml" "data/valuesource.qml" "data/valuesource2.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickanimations CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickanimations CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
