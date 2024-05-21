#!/bin/bash
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

$HOME/bauhaus-suite/setup.sh --non-interactive
export PATH=/home/qt/bauhaus-suite/bin:$PATH
export BAUHAUS_CONFIG=$(cd $(dirname $(readlink -f $0)) && pwd)
export AXIVION_VERSION_NAME=$(git rev-parse HEAD)
export EXCLUDE_FILES="build/*:src/3rdparty/*"
export AXIVION_NUM_JOBS_COMPILE="4"
export AXIVION_NUM_JOBS_LINK="1"
export CAFECC_BASEPATH="/home/qt/work/qt/$TESTED_MODULE_COIN"
gccsetup --cc gcc --cxx g++ --config "$BAUHAUS_CONFIG"
cd "$CAFECC_BASEPATH"
BAUHAUS_IR_COMPRESSION=none COMPILE_ONLY=1 cmake -G Ninja -DAXIVION_ANALYSIS_TOOLCHAIN_FILE=/home/qt/bauhaus-suite/profiles/cmake/axivion-launcher-toolchain.cmake -DCMAKE_PREFIX_PATH=/home/qt/work/qt/qtdeclarative/build -DCMAKE_PROJECT_INCLUDE_BEFORE=/home/qt/bauhaus-suite/profiles/cmake/axivion-before-project-hook.cmake -B build -S . --fresh
cmake --build build -j4
for MODULE in qtqml qtquick qtquickcontrols qtquickdialogs qtquicklayouts qtquicktest; do
    export MODULE
    export PLUGINS=""
    export IRNAME=build/$MODULE.ir
    if [ "$MODULE" == "qtqml" ]
    then
        export TARGET_NAME="build/lib/libQt6Qml.so.*.ir"
        export PLUGINGS="build/qml/*.so.ir:build/plugins/*.so.ir"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtquick" ]
    then
        export TARGET_NAME="build/lib/libQt6Quick.so.*.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/*:src/qmlmodels/*"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtquickcontrols" ]
    then
        export TARGET_NAME="build/lib/libQt6QuickControls2.so.*.ir:build/lib/libQt6QuickControls2Impl.so.*.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/*:src/quick/*:src/qmlmodels/*:src/quicktemplates/*"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtquickdialogs" ]
    then
        export TARGET_NAME="build/lib/libQt6QuickDialogs2.so.*.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/*:src/quick/*:src/quicktemplates/*:src/quickcontrolsimpl/*:src/qmlmodels/*"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtquicklayouts" ]
    then
        export TARGET_NAME="build/lib/libQt6QuickLayouts.so.*.ir"
        export PACKAGE="Essentials"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/*:src/quick/*:src/qmlmodels/*"
    elif [ "$MODULE" == "qtquicktest" ]
    then
        export TARGET_NAME="build/lib/libQt6QuickTest.so.*.ir"
        export PACKAGE="Essentials"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/*:src/quick/*:src/qmlmodels/*:src/qmltests/*"
    fi
    axivion_ci "$@"
done
