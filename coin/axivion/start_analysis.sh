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

ROOT_DIR=src
MAGIC="Qt-Security score:critical"

INCLUDE_FILES=""

while IFS= read -r file; do
    if [ -z "$INCLUDE_FILES" ]; then
        INCLUDE_FILES="$file"
    else
        INCLUDE_FILES="$INCLUDE_FILES:$file"
    fi
done <<EOF
$(grep -rl "$MAGIC" "$ROOT_DIR")
EOF

if [ -z "$INCLUDE_FILES" ]; then
    echo "No Security Critical files found; running analysis with an explicit fallback sentinel."
    INCLUDE_FILES="__NO_FILES_FOUND__"
fi

export INCLUDE_FILES

# Module list below is grouped as:
#   1) the 6 "core" Essentials modules. These now also absorb the
#      private/internal libraries that back them directly (QmlMeta,
#      QmlLocalStorage, QmlDesignSupport under qtqml; QuickShapes/
#      QuickVectorImage under qtquick; the per-style Controls2
#      implementations and QuickControlsTestUtils under qtquickcontrols;
#      QuickDialogs2Utils/QuickImpl under qtquickdialogs; QuickTestUtils
#      under qtquicktest) instead of living in a separate catch-all module.
#   2) 10 modules promoted 1:1 to an official Qt6 module listed on
#      https://doc.qt.io/qt-6/qtmodules.html
#   3) 2 catch-all buckets for libraries that have no single obvious
#      Essentials/Add-Ons module to fold into: qtlabsexperimental (the
#      Qt.labs.* namespaces that never got their own module page) and
#      qtqmltooling (internal support libs for qmllint/qmltc/qmlformat/
#      qmlls/qmldom/qmlpreview and the QML debug protocol).
for MODULE in \
    qtqml qtquick qtquickcontrols qtquickdialogs qtquicklayouts qtquicktest \
    qtqmlcore qtqmlworkerscript qtqmlnetwork qtqmlxmllistmodel qtqmlcompiler \
    qtquickparticles qtquickeffects qtquickwidgets qtlabsplatform qtlabsstylekit \
    qtlabsexperimental qtqmltooling \
; do
    export MODULE
    export PLUGINS=""
    export IRNAME=build/$MODULE.ir
    if [ "$MODULE" == "qtqml" ]
    then
        # Qt Qml (Essentials). QmlMeta/QmlLocalStorage back Qt Qml directly;
        # QmlDesignSupport (Qt Design Studio only) has no better home.
        TARGET_NAME="build/lib/libQt6Qml.so.*.ir:build/lib/libQt6QmlModels.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QmlMeta.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QmlLocalStorage.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QmlDesignSupport.so.*.ir"
        export TARGET_NAME
        # Recursive (**) globs are required: the actual plugin binaries are
        # nested one level deeper, e.g. build/qml/QtQml/Models/libmodelsplugin.so
        export PLUGINS="build/qml/**/*.so.ir:build/plugins/**/*.so.ir"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtquick" ]
    then
        # Qt Quick (Essentials). QuickShapes/QuickVectorImage ship as part
        # of Qt Quick and have no module page of their own.
        TARGET_NAME="build/lib/libQt6Quick.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickShapes.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickShapesDesignHelpers.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickVectorImage.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickVectorImageHelpers.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickVectorImageGenerator.so.*.ir"
        export TARGET_NAME
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtquickcontrols" ]
    then
        # Qt Quick Controls (Essentials). Folds in every per-style
        # implementation (Basic/Fusion/Imagine/Material/Universal/
        # FluentWinUI3/Native) and the private QuickControlsTestUtils
        # helper - Qt6 doc only documents "Qt Quick Controls" as a whole.
        TARGET_NAME="build/lib/libQt6QuickControls2.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2Impl.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickTemplates2.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2Basic.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2BasicStyleImpl.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2Fusion.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2FusionStyleImpl.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2Imagine.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2ImagineStyleImpl.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2Material.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2MaterialStyleImpl.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2Universal.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2UniversalStyleImpl.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2FluentWinUI3.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2FluentWinUI3StyleImpl.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControls2Native.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickControlsTestUtils.a.ir"
        export TARGET_NAME
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtquickdialogs" ]
    then
        # Qt Quick Dialogs (Essentials). QuickDialogs2Utils/QuickImpl back
        # this module directly and have no module page of their own.
        TARGET_NAME="build/lib/libQt6QuickDialogs2.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickDialogs2Utils.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QuickDialogs2QuickImpl.so.*.ir"
        export TARGET_NAME
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        export EXCLUDE_FILES
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtquicklayouts" ]
    then
        # Qt Quick Layouts (Essentials)
        export TARGET_NAME="build/lib/libQt6QuickLayouts.so.*.ir"
        export PACKAGE="Essentials"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtquicktest" ]
    then
        # Qt Quick Test (Essentials). QuickTestUtils backs this module
        # directly and has no module page of its own.
        export TARGET_NAME="build/lib/libQt6QuickTest.so.*.ir:build/lib/libQt6QuickTestUtils.a.ir"
        export PACKAGE="Essentials"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtqmlcore" ]
    then
        # Qt Qml Core (Essentials)
        export TARGET_NAME="build/lib/libQt6QmlCore.so.*.ir"
        export PACKAGE="Essentials"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtqmlworkerscript" ]
    then
        # Qt Qml WorkerScript (Essentials)
        export TARGET_NAME="build/lib/libQt6QmlWorkerScript.so.*.ir"
        export PACKAGE="Essentials"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**:src/core/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtqmlnetwork" ]
    then
        # Qt Qml Network (Add-Ons)
        export TARGET_NAME="build/lib/libQt6QmlNetwork.so.*.ir"
        export PACKAGE="Add-ons"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**:src/core/**:src/qmlworkerscript/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtqmlxmllistmodel" ]
    then
        # Qt Qml XmlListModel (Add-Ons)
        export TARGET_NAME="build/lib/libQt6QmlXmlListModel.so.*.ir"
        export PACKAGE="Add-ons"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**:src/core/**:src/qmlworkerscript/**:src/qmlnetwork/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtqmlcompiler" ]
    then
        # Qt Qml Compiler (Add-Ons)
        export TARGET_NAME="build/lib/libQt6QmlCompiler.so.*.ir"
        export PACKAGE="Add-ons"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**:src/core/**:src/qmlworkerscript/**:src/qmlnetwork/**:src/qmlxmllistmodel/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtquickparticles" ]
    then
        # Qt Quick Particles (Add-Ons)
        export TARGET_NAME="build/lib/libQt6QuickParticles.so.*.ir"
        export PACKAGE="Add-ons"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**:src/core/**:src/qmlworkerscript/**:src/qmlnetwork/**:src/qmlxmllistmodel/**:src/qmlcompiler/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtquickeffects" ]
    then
        # Qt Quick Effects (Add-Ons)
        export TARGET_NAME="build/lib/libQt6QuickEffects.so.*.ir"
        export PACKAGE="Add-ons"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**:src/core/**:src/qmlworkerscript/**:src/qmlnetwork/**:src/qmlxmllistmodel/**:src/qmlcompiler/**:src/particles/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtquickwidgets" ]
    then
        # Qt Quick Widgets (Add-Ons)
        export TARGET_NAME="build/lib/libQt6QuickWidgets.so.*.ir"
        export PACKAGE="Add-ons"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**:src/core/**:src/qmlworkerscript/**:src/qmlnetwork/**:src/qmlxmllistmodel/**:src/qmlcompiler/**:src/particles/**:src/effects/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtlabsplatform" ]
    then
        # Qt Labs Platform (Technology Preview Add-On)
        export TARGET_NAME="build/lib/libQt6LabsPlatform.so.*.ir"
        export PACKAGE="Add-ons"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**:src/core/**:src/qmlworkerscript/**:src/qmlnetwork/**:src/qmlxmllistmodel/**:src/qmlcompiler/**:src/particles/**:src/effects/**:src/quickwidgets/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtlabsstylekit" ]
    then
        # Qt Labs StyleKit (Technology Preview Add-On)
        export TARGET_NAME="build/lib/libQt6LabsStyleKit.so.*.ir:build/lib/libQt6LabsStyleKitImpl.so.*.ir"
        export PACKAGE="Add-ons"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**:src/core/**:src/qmlworkerscript/**:src/qmlnetwork/**:src/qmlxmllistmodel/**:src/qmlcompiler/**:src/particles/**:src/effects/**:src/quickwidgets/**:src/labs/platform/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtlabsexperimental" ]
    then
        # Experimental Qt.labs.* QML namespaces that (unlike Qt Labs Platform
        # and Qt Labs StyleKit) never got their own module page.
        TARGET_NAME="build/lib/libQt6LabsSynchronizer.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6LabsSettings.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6LabsQmlModels.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6LabsFolderListModel.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6LabsAnimation.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6LabsWavefrontMesh.so.*.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6LabsSharedImage.so.*.ir"
        export TARGET_NAME
        export PACKAGE="Add-ons"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**:src/core/**:src/qmlworkerscript/**:src/qmlnetwork/**:src/qmlxmllistmodel/**:src/qmlcompiler/**:src/particles/**:src/effects/**:src/quickwidgets/**:src/labs/platform/**:src/labs/stylekit/**"
        export EXCLUDE_FILES
    elif [ "$MODULE" == "qtqmltooling" ]
    then
        # Internal libraries backing the qmllint/qmltc/qmlformat/qmlls/qmldom/
        # qmlpreview command-line tools and the QML debug-connection protocol.
        # None of these are documented as importable/linkable Qt6 modules.
        TARGET_NAME="build/lib/libQt6QmlDom.a.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QmlLint.a.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6Qmltc.a.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QmlFormat.a.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QmlLS.a.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6PacketProtocol.a.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QmlDebug.a.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QmlToolingSettings.a.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QmlTypeRegistrar.a.ir"
        TARGET_NAME="$TARGET_NAME:build/lib/libQt6QmlPreview.a.ir"
        export TARGET_NAME
        export PACKAGE="Add-ons"
        EXCLUDE_FILES="build/*:src/3rdparty/*:src/qml/**:src/qmlmodels/**:src/qmlmeta/**:src/qmllocalstorage/**:src/qmldesignsupport/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quick/**:src/quickshapes/**:src/quickvectorimage/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickcontrolsimpl/**:src/quicktemplates/**:src/quickcontrols/**:src/quickcontrolstestutils/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/quickdialogs/**:src/quicklayouts/**:src/qmltest/**:src/quicktestutils/**:src/core/**:src/qmlworkerscript/**:src/qmlnetwork/**:src/qmlxmllistmodel/**:src/qmlcompiler/**:src/particles/**:src/effects/**:src/quickwidgets/**:src/labs/platform/**:src/labs/stylekit/**"
        EXCLUDE_FILES="$EXCLUDE_FILES:src/labs/synchronizer/**:src/labs/settings/**:src/labs/models/**:src/labs/folderlistmodel/**:src/labs/animation/**:src/labs/wavefrontmesh/**:src/labs/sharedimage/**"
        export EXCLUDE_FILES
    fi
    axivion_ci "$@"
done
