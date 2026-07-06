// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

BaseStyle {
    id: style

    /* Properties and Controls left unspecified in a Style will be read from Style.fallbackStyle
     * instead (that is, this file, unless the fallback style is changed). Here we can give the
     * properties some sensible defaults. */

    /* Assign sensible light-theme colors to the controls. We intentionally avoid
     * binding to the OS palette (except for the accent color) because the fallback
     * style should provide a stable baseline for other styles and remain identical
     * and consistent across all platforms. Instead, it is the style developer’s
     * responsibility to bind individual colors to the palette if desired, and to
     * define separate themes for light and dark mode. */

    readonly property color __baseBlack: "black"
    readonly property color __baseWhite: "white"
    readonly property color __backgroundDefault: Qt.darker(__baseWhite, 1.1)
    readonly property color __backgroundMuted: Qt.darker(__baseWhite, 1.2)
    readonly property color __backgroundSubtle: Qt.darker(__baseWhite, 1.3)
    readonly property color __strokeStrong: Qt.darker(__baseWhite, 1.6)
    readonly property color __strokeMuted: Qt.darker(__baseWhite, 1.4)
    readonly property color __textDefault: Qt.darker(__baseWhite, 1.9)
    readonly property color __textSubtle: Qt.darker(__baseWhite, 1.6)
    readonly property color __transparent: "transparent"

    readonly property real indicatorSize: 24

    control {
        spacing: 5
        padding: 5

        background {
            radius: 2
            width: 100
            height: 40
            border.width: 1
            color: __backgroundDefault
            border.color: __strokeStrong
        }

        indicator {
            width: style.indicatorSize
            height: style.indicatorSize
            border.width: 1
            color: __baseWhite
            border.color: __strokeStrong
            foreground {
                fillWidth: true
                fillHeight: true
                margins: 1
                color: palette.accent
                image.color: palette.accent
            }
        }

        text {
            color: __baseBlack
        }

        handle {
            width: style.indicatorSize
            height: style.indicatorSize
            radius: style.indicatorSize / 2
            border.width: 1
            color: __backgroundDefault
            border.color: __strokeStrong
        }

        checked {
            background.color: __backgroundSubtle
        }

        hovered {
            handle.color: __backgroundMuted
        }

        disabled {
            background.color: __baseWhite
            text.color: __textSubtle
        }
    }

    abstractButton {
        hovered.background.color: __backgroundMuted
    }

    applicationWindow {
        background.color: __baseWhite
    }

    checkBox {
        background.visible: false
        indicator {
            foreground {
                color: __transparent
                visible: false
                image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
            }
        }
        text {
            alignment: Qt.AlignVCenter | Qt.AlignLeft
        }
        checked {
            indicator.foreground.visible: true
        }
    }

    checkDelegate {
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        background {
            radius: 0
            color: __baseWhite
            border.width: 0
        }
        indicator {
            alignment: Qt.AlignRight | Qt.AlignVCenter
            foreground {
                visible: false
                alignment: Qt.AlignCenter
                color: __transparent
                image.fillMode: Image.PreserveAspectFit
                image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
            }
        }
        hovered.background.color: __backgroundDefault
        checked {
            indicator.foreground.visible: true
        }
    }

    comboBox {
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        background.width: 150
        indicator {
            color: __transparent
            border.width: 0
            alignment: Qt.AlignRight | Qt.AlignVCenter
            foreground {
                fillWidth: false
                fillHeight: false
                width: 10
                height: 10
                alignment: Qt.AlignCenter
                color: __transparent
                image.color: __textDefault
                image.fillMode: Image.PreserveAspectFit
                image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/drop-indicator.png"
            }
        }
    }

    flatButton {
        background.visible: false
        hovered.background.visible: true
    }

    pane {
        padding: 12
        background {
            width: 200
            height: 200
            color: __baseWhite
        }
    }

    frame {
        background.color: Qt.darker(__baseWhite, 1.05)
    }

    groupBox {
        background.topMargin: 20
        background.height: 20
    }

    itemDelegate {
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        background {
            // Make it flat
            radius: 0
            color: __baseWhite
            border.width: 0
        }
        hovered.background.color: __backgroundDefault
    }

    label {
        background.visible: false
    }

    menu {
        background {
            width: 200
            height: 20
        }
    }

    menuBar {
        padding: 1
        spacing: 0
        background {
            height: 20
            radius: 0
            border.width: 0
            color: __backgroundDefault
        }
    }

    menuBarItem {
        text.alignment: Qt.AlignLeft | Qt.AlignVCenter
        background {
            width: 20
            height: 20
            radius: 0
            color: __transparent
            border.width: 0
        }
        hovered.background.color: __backgroundSubtle
    }

    menuItem {
        text.alignment: Qt.AlignLeft | Qt.AlignVCenter
        background {
            width: 200
            radius: 0
            border.width: 0
        }
        indicator {
            color: __transparent
            border.width: 0
            foreground {
                width: 10
                height: 10
                color: __transparent
                image.color: __textDefault
                image.fillMode: Image.PreserveAspectFit
                alignment: Qt.AlignCenter
            }
            first {
                alignment: Qt.AlignLeft
                foreground.image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
            }
            second {
                alignment: Qt.AlignRight
                foreground.image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/arrow-indicator.png"
            }
        }
        hovered.background.color: __backgroundSubtle
    }

    menuSeparator {
        padding: 0

        background {
            width: 188
            height: 1
            color: "transparent"
            border.width: 0
        }
        indicator {
            height: 1
            fillWidth: true
            border.width: 0
            color: __strokeMuted
            foreground.visible: false
        }
    }

    page {
        background.border.width: 0
    }

    popup {
        background {
            width: 200
            height: 200
            border.width: 1
        }
    }

    progressBar {
        background.visible: false
        indicator.width: 150
        indicator.foreground.delegate: ProgressDelegate {}
    }

    radioButton {
        background.visible: false
        indicator {
            radius: 255
            foreground {
                margins: 4
                visible: false
                radius: 255
            }
        }
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        checked.indicator.foreground.visible: true
    }

    radioDelegate {
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        background {
            radius: 0
            color: __baseWhite
            border.width: 0
        }
        indicator {
            alignment: Qt.AlignRight | Qt.AlignVCenter
            radius: 255
            foreground {
                margins: 4
                visible: false
                radius: 255
            }
        }
        hovered.background.color: __backgroundDefault
        checked.indicator.foreground.visible: true
    }

    roundButton {
        background.radius: 255
        text.alignment: Qt.AlignVCenter | Qt.AlignHCenter
    }

    scrollBar {
        padding: 0
        background {
            width: 12
            height: 12
            radius: 0
        }
        indicator {
            width: 12
            height: 12
            fillWidth: true
            radius: 0
            foreground.radius: 0
            foreground.color: __backgroundMuted
        }
        vertical {
            indicator {
                fillWidth: false
                fillHeight: true
            }
        }
        hovered {
            indicator.foreground.color: __backgroundSubtle
        }
    }

    scrollIndicator {
        background {
            width: 6
            height: 6
            visible: false
        }
        indicator {
            width: 6
            height: 6
            fillWidth: true
            radius: 255
            border.width: 0
            foreground {
                margins: 0
                radius: 255
                color: __backgroundMuted
            }
        }
        vertical {
            indicator {
                fillWidth: false
                fillHeight: true
            }
        }
    }

    searchField {
        text.padding: 5
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        text.color: __baseBlack
        background.width: 200

        indicator {
            fillHeight: true
            border.width: 0
            margins: 0
            color: __transparent
            foreground {
                fillWidth: false
                fillHeight: false
                width: 10
                height: 10
                alignment: Qt.AlignCenter
                color: __transparent
            }

            //search button
            first {
                alignment: Qt.AlignLeft
                image.color: __textDefault
                image.fillMode: Image.PreserveAspectFit
                image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/search-magnifier.png"
            }

            //clear button
            second {
                alignment: Qt.AlignRight
                image.color: __textDefault
                image.fillMode: Image.PreserveAspectFit
                image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/close_circle.png"
            }
        }
    }

    slider {
        background {
            visible: false
            width: 150
        }
        indicator {
            fillWidth: true
            height: 8
            radius: 8
            foreground.radius: 7
            foreground.delegate: ProgressDelegate {}
        }
        vertical {
            // Manually transpose the sizes
            background {
                width: 40
                height: 150
            }

            indicator {
                fillWidth: false
                fillHeight: true
                width: 8
                alignment: Qt.AlignHCenter
            }
        }
    }

    spinBox {
        text.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        indicator {
            fillHeight: true
            border.width: 0
            margins: 0
            color: __transparent
            foreground {
                width: 10
                height: 10
                fillWidth: false
                fillHeight: false
                color: __transparent
                image.color: __textDefault
                image.fillMode: Image.PreserveAspectFit
                image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/arrow-indicator.png"
                alignment: Qt.AlignCenter
            }
            // up button
            first {
                alignment: Qt.AlignRight
                foreground.rotation: -90
            }
            // down button
            second {
                alignment: Qt.AlignLeft
                foreground.rotation: 90
            }
        }
    }

    switchControl {
        background.visible: false
        text.alignment: Qt.AlignVCenter
        indicator {
            width: style.indicatorSize * 2
            height: style.indicatorSize
            alignment: Qt.AlignLeft | Qt.AlignVCenter
            radius: style.indicatorSize / 2
            foreground {
                radius: style.indicatorSize / 2
                color: __transparent
            }
        }
        checked {
            indicator.foreground.color: palette.accent
        }
    }

    switchDelegate {
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        background {
            radius: 0
            color: __baseWhite
            border.width: 0
        }
        indicator {
            width: style.indicatorSize * 2
            height: style.indicatorSize
            alignment: Qt.AlignRight | Qt.AlignVCenter
            radius: style.indicatorSize / 2
            foreground {
                radius: style.indicatorSize / 2
                color: __transparent
            }
        }
        hovered.background.color: __backgroundDefault
        checked {
            indicator.foreground.color: palette.accent
        }
    }

    tabBar {
        padding: 0
        spacing: -1 // let tabButtons overlap slightly
    }

    tabButton {
        background {
            radius: 0
            topLeftRadius: 2
            topRightRadius: 2
        }
    }

    textField {
        text.alignment: Qt.AlignVCenter
        background {
            width: 150
            gradient: null
        }
    }

    textInput {
        padding: 5
        background {
            width: 150
            height: 40
            border.width: 1
            color: __baseWhite
        }
        text {
            color: __baseBlack
        }
    }

    toolBar {
        background.height: 40
    }

    toolSeparator {
        padding: 2
        background.visible: false
        indicator {
            width: 30
            height: 1
            border.width: 0
            color: __strokeMuted
            foreground.visible: false
        }
        vertical {
            indicator {
                width: 1
                height: 30
                alignment: Qt.AlignHCenter
            }
        }
    }
}
