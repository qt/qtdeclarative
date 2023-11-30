// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma Singleton

import QtQuick
import WearableSettings

Item {
    id: uiStyle

    property font h1: Qt.font({
        family: fontLoaderSemibold.font.family,
        weight: fontLoaderSemibold.font.weight,
        pixelSize: 24
    })
    property int h1lineHeight: 28

    property font h2: Qt.font({
        family: fontLoaderSemibold.font.family,
        weight: fontLoaderSemibold.font.weight,
        pixelSize: 20
    })
    property int h2lineHeight: 24

    property font h3: Qt.font({
        family: fontLoaderRegular.font.family,
        weight: fontLoaderRegular.font.weight,
        pixelSize: 16
    })
    property int h3lineHeight: 20

    property font h4: Qt.font({
        family: fontLoaderBold.font.family,
        weight: fontLoaderBold.font.weight,
        pixelSize: 16
    })
    property int h4lineHeight: 20

    property font p1: Qt.font({
        family: fontLoaderRegular.font.family,
        weight: fontLoaderRegular.font.weight,
        pixelSize: 14
    })
    property int p1lineHeight: 16

    property font p2: Qt.font({
        family: fontLoaderRegular.font.family,
        weight: fontLoaderRegular.font.weight,
        pixelSize: 20
    })
    property int p2lineHeight: 24

    property font tumblerFont: Qt.font({
        family: fontLoaderRegular.font.family,
        weight: fontLoaderRegular.font.weight,
        pixelSize: 32
    })

    FontLoader {
        id: fontLoaderBold
        source: "qrc:/qt/qml/WearableStyle/fonts/TitilliumWeb-Bold.ttf"
    }

    FontLoader {
        id: fontLoaderSemibold
        source: "qrc:/qt/qml/WearableStyle/fonts/TitilliumWeb-SemiBold.ttf"
    }

    FontLoader {
        id: fontLoaderRegular
        source: "qrc:/qt/qml/WearableStyle/fonts/TitilliumWeb-Regular.ttf"
    }

    readonly property color colorRed: "#E91E63"

    readonly property color buttonGray: WearableSettings.darkTheme ? "#808080" : "#f3f3f4"
    readonly property color buttonGrayPressed: WearableSettings.darkTheme ? "#707070" : "#cecfd5"
    readonly property color buttonGrayOutLine: WearableSettings.darkTheme ? "#0D0D0D" : "#999999"

    readonly property color buttonBackground: WearableSettings.darkTheme ? "#262626" : "#CCCCCC"
    readonly property color buttonProgress: WearableSettings.darkTheme ? "#28C878" : "#19545C"

    readonly property color gradientOverlay1: "#00000000"
    readonly property color gradientOverlay2: "#1E000000"

    readonly property color background1: WearableSettings.darkTheme ? "#00414A" : "#ABF2CE"
    readonly property color background2: WearableSettings.darkTheme ? "#07243E" : "#E6E6E6"
    readonly property color background3: WearableSettings.darkTheme ? "#262626" : "#E6E6E6"

    readonly property color textColor: WearableSettings.darkTheme ? "#E6E6E6" : "#191919"
    readonly property color titletextColor: WearableSettings.darkTheme ? "#2CDE85" : "#191919"

    readonly property color highlightColor: WearableSettings.darkTheme ? "#33676E" : "#28C878"

    readonly property color pageIndicatorColor: WearableSettings.darkTheme ? "#00000000" : "#E6E6E6"
    readonly property color indicatorOutlineColor: WearableSettings.darkTheme ? "#707070" : "#999999"

    readonly property color listHeader1: WearableSettings.darkTheme ? "#9E00414A" : "#9E2CDE85"
    readonly property color listHeader2: WearableSettings.darkTheme ? "#9E0C1C1F" : "#9E00414A"

    readonly property color listItemBackground: WearableSettings.darkTheme ? "#00414A" : "#EAFCF3"

    function imagePath(baseImagePath) {
        return `qrc:/qt/qml/Wearable/images/${baseImagePath}.png`
    }

    function themeImagePath(baseImagePath) {
        return `qrc:/qt/qml/Wearable/images/${baseImagePath}${(WearableSettings.darkTheme ? "-dark" : "-light")}.svg`
    }
    function iconPath(baseImagePath) {
        return `qrc:/qt/qml/Wearable/icons/${baseImagePath}${(WearableSettings.darkTheme ? "-dark" : "-light")}.svg`
    }
}
