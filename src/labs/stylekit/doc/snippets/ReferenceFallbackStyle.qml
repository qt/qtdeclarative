// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [1]
import QtQuick
import Qt.labs.StyleKit

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
                fillWidth: false
                fillHeight: false
                margins: 1
                color: palette.accent
                image.color: palette.accent
                /* Note: don't set implicit size here, since the DelegateContainer will (and should)
                 * fall back to use the size of the image if not set. So if we hard-code a size here,
                 * it cannot be unset to be the size of the image (if any) again from the Style. */
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

    busyIndicator {
        background {
            visible: false
            width: 48
            height: 48
        }
        indicator {
            fillWidth: true
            fillHeight: true
            color: __transparent
            border.width: 0
            foreground {
                radius: 255
                delegate: BusyIndicatorDelegate { }
            }
        }
    }

    checkBox {
        background.visible: false
        indicator {
            foreground {
                fillWidth: true
                fillHeight: true
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
                fillWidth: true
                fillHeight: true
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

    delayButton {
        padding: 0
        indicator {
            fillWidth: true
            fillHeight: true
            color: __transparent
            border.width: 0
            foreground {
                fillWidth: true
                fillHeight: true
                delegate: ProgressDelegate {}
            }
        }
    }

    dialog {
        text {
            padding: 12
            alignment: Qt.AlignVCenter | Qt.AlignLeft
        }
        background.color: __baseWhite
    }

    dialogButtonBox {
        background {
            height: 40
            radius: 0
            color: __transparent
            border.width: 0
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
            shadow.visible: false
        }
        hovered.background.color: __backgroundDefault
    }

    label {
        background.visible: false
    }

    menu {
        background {
            width: 200
            height: 40
        }
    }

    menuBar {
        background {
            radius: 0
            border.width: 0
            color: __backgroundDefault
        }
    }

    menuBarItem {
        text.alignment: Qt.AlignLeft | Qt.AlignVCenter
        background {
            width: 40
            height: 40
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
                color: __transparent
                image.color: __textDefault
                image.fillMode: Image.PreserveAspectFit
                alignment: Qt.AlignCenter
            }
            first {
                alignment: Qt.AlignLeft
                foreground {
                    width: 20
                    height: 20
                    image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
                }
            }
            second {
                alignment: Qt.AlignRight
                foreground {
                    width: 10
                    height: 10
                    image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/arrow-indicator.png"
                }
            }
        }
        hovered.background.color: __backgroundSubtle
    }

    menuSeparator {
        padding: 2
        background.visible: false
        indicator {
            width: 188
            height: 1
            border.width: 0
            color: __strokeMuted
            foreground.visible: false
        }
    }

    page {
        background.border.width: 0
    }

    pageIndicator {
        background.visible: false
        indicator {
            width: 5
            height: 5
            radius: 255
            border.width: 0
            color: __baseBlack
            opacity: 0.45
            foreground.visible: false
        }

        hovered.indicator {
            width: 6
            height: 6
            opacity: 0.7
        }

        pressed.indicator {
            width: 4
            height: 4
        }

        checked.indicator {
            width: 7
            height: 7
        }

        checked.hovered.indicator {
            width: 8
            height: 8
            opacity: 0.7
        }
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
        indicator {
            width: 150
            foreground {
                fillWidth: true
                fillHeight: true
                delegate: ProgressDelegate {}
            }
        }
    }

    radioButton {
        background.visible: false
        indicator {
            radius: 255
            foreground {
                fillWidth: true
                fillHeight: true
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
                fillWidth: true
                fillHeight: true
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
            height: 10
        }
        indicator {
            height: 10
            radius: 255
            foreground {
                fillWidth: true
                fillHeight: true
                radius: 255
                color: __backgroundDefault
            }
        }
        vertical {
            background.width: 10
            indicator.width: 10
        }
        hovered {
            indicator.foreground.color: __backgroundMuted
        }
    }

    scrollIndicator {
        background {
            height: 6
            visible: false
        }
        indicator {
            border.width: 0
            height: 6
            radius: 255
            foreground {
                fillWidth: true
                fillHeight: true
                margins: 0
                radius: 255
                color: __backgroundDefault
            }
        }
        vertical {
            background.width: 6
            indicator.width: 6
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
            foreground {
                radius: 7
                fillWidth: true
                fillHeight: true
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
                color: __transparent
                image.color: __textDefault
                image.fillMode: Image.PreserveAspectFit
                image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/arrow-indicator.png"

                // Note: since we set a rotation here, don't set fillWidth or fillHeight to true, since
                // the rotation is applied after the layout has been done based on the unrotated geometry
                // of the indicator. So we use a fixed implicit size instead, and let the layout stretch the
                // indicator by setting the alignment to AlignCenter. This way, the indicator will be
                // stretched in both dimensions, but still maintain its aspect ratio due to the image's fillMode
                // being PreserveAspectFit.
                width: 10
                height: 10
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

    swipeDelegate {
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        background {
            radius: 0
            color: __baseWhite
            border.width: 0
        }
        hovered.background.color: __backgroundDefault
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
                fillWidth: true
                fillHeight: true
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
                fillWidth: true
                fillHeight: true
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
    }
}
//! [1]
