# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("quickcontrols2-basic" PRIVATE
    LABEL "Basic"
)
qt_feature("quickcontrols2-fusion" PRIVATE
    SECTION "Quick Controls 2"
    LABEL "Fusion"
    PURPOSE "Provides the platform agnostic desktop-oriented Fusion style."
    CONDITION QT_FEATURE_quickcontrols2_basic
)
qt_feature("quickcontrols2-imagine" PRIVATE
    SECTION "Quick Controls 2"
    LABEL "Imagine"
    PURPOSE "Provides a style based on configurable image assets."
    CONDITION QT_FEATURE_quickcontrols2_basic
)
qt_feature("quickcontrols2-material" PRIVATE
    SECTION "Quick Controls 2"
    LABEL "Material"
    PURPOSE "Provides a style based on the Material Design guidelines."
    CONDITION QT_FEATURE_quickcontrols2_basic
)
qt_feature("quickcontrols2-universal" PRIVATE
    SECTION "Quick Controls 2"
    LABEL "Universal"
    PURPOSE "Provides a style based on the Universal Design guidelines."
    CONDITION QT_FEATURE_quickcontrols2_basic
)
qt_feature("quickcontrols2-macos" PRIVATE
    SECTION "Quick Controls 2"
    LABEL "macOS"
    PURPOSE "Provides a native macOS desktop style."
    CONDITION QT_FEATURE_quickcontrols2_basic AND MACOS
)
qt_feature("quickcontrols2-ios" PRIVATE
    SECTION "Quick Controls 2"
    LABEL "iOS"
    PURPOSE "Provides a native-looking iOS style."
    CONDITION QT_FEATURE_quickcontrols2_basic AND (IOS OR MACOS)
)
qt_feature("quickcontrols2-windows" PRIVATE
    SECTION "Quick Controls 2"
    LABEL "Windows"
    PURPOSE "Provides a native Windows desktop style."
    CONDITION QT_FEATURE_quickcontrols2_basic AND WIN32
)
qt_configure_add_summary_section(NAME "Qt Quick Controls 2")
qt_configure_add_summary_entry(
    TYPE "featureList"
    ARGS "quickcontrols2-basic quickcontrols2-fusion quickcontrols2-imagine quickcontrols2-ios quickcontrols2-material quickcontrols2-universal quickcontrols2-macos quickcontrols2-windows"
    MESSAGE "Styles"
)
qt_configure_end_summary_section() # end of "Qt Quick Controls 2" section
