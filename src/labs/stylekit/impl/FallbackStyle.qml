// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

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

    applicationWindow {
        background.color: __baseWhite
    }

    control {
        spacing: 5
        padding: 5

        background {
            radius: 2
            implicitWidth: 100
            implicitHeight: 40
            border.width: 1
            color: __backgroundDefault
            border.color: __strokeStrong
        }

        indicator {
            implicitWidth: style.indicatorSize
            implicitHeight: style.indicatorSize
            border.width: 1
            color: __baseWhite
            border.color: __strokeStrong
            foreground {
                implicitWidth: Style.Stretch
                implicitHeight: Style.Stretch
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
            implicitWidth: style.indicatorSize
            implicitHeight: style.indicatorSize
            radius: style.indicatorSize / 2
            border.width: 1
            color: __backgroundDefault
            border.color: __strokeStrong
        }

        checked {
            background.color: __backgroundSubtle
        }

        hovered {
            background.color: __backgroundMuted
            handle.color: __backgroundMuted
        }

        disabled {
            background.color: __baseWhite
            text.color: __textSubtle
        }
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

    comboBox {
        text.alignment: Qt.AlignVCenter
        background.implicitWidth: 150
        indicator {
            color: __transparent
            border.width: 0
            foreground {
                implicitWidth: 10
                implicitHeight: 10
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
            implicitWidth: 200
            implicitHeight: 200
            color: __baseWhite
        }
    }

    frame {
        background.color: Qt.darker(__baseWhite, 1.05)
    }

    groupBox {
        background.topMargin: 20
        background.implicitHeight: 20
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

    page {
        background.border.width: 0
    }

    popup {
        background {
            implicitWidth: 200
            implicitHeight: 200
            border.width: 1
        }
    }

    progressBar {
        background.visible: false
        indicator.implicitWidth: 150
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

    scrollBar {
        padding: 0
        background {
            implicitHeight: 10
            visible: false
        }
        indicator {
            implicitHeight: 10
            radius: 255
            foreground.radius: 255
            foreground.color: __backgroundDefault
        }
        vertical {
            background.implicitWidth: 10
            indicator.implicitWidth: 10
        }
        hovered {
            indicator.foreground.color: __backgroundMuted
        }
    }

    scrollIndicator {
        background {
            implicitHeight: 6
            visible: false
        }
        indicator {
            border.width: 0
            implicitHeight: 6
            radius: 255
            foreground {
                margins: 0
                radius: 255
                color: __backgroundDefault
            }
        }
        vertical {
            background.implicitWidth: 6
            indicator.implicitWidth: 6
        }
    }

    slider {
        background {
            visible: false
            implicitWidth: 150
        }
        indicator {
            implicitWidth: Style.Stretch
            implicitHeight: 8
            radius: 8
            foreground {
                radius: 7
            }
        }
    }

    spinBox {
        text.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        indicator {
            implicitHeight: Style.Stretch
            border.width: 0
            margins: 0
            color: __transparent
            foreground {
                color: __transparent
                image.color: __textDefault
                image.fillMode: Image.PreserveAspectFit
                image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/arrow-indicator.png"

                // Note: by setting a rotation, we cannot at the same time set the implicit size to
                // Stretch, since Stretch will be based on the unrotated geometry of the indicator.
                // So if the height of the indicator is different from the width, things will look wrong.
                implicitWidth: 10
                implicitHeight: 10
                alignment: Qt.AlignCenter
            }
            down {
                alignment: Qt.AlignLeft
                foreground.rotation: 90
            }
            up {
                alignment: Qt.AlignRight
                foreground.rotation: -90
            }
        }
    }

    switchControl {
        background.visible: false
        text.alignment: Qt.AlignVCenter
        indicator {
            implicitWidth: style.indicatorSize * 2
            implicitHeight: style.indicatorSize
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
            implicitWidth: 150
            gradient: null
        }
    }

    textInput {
        padding: 5
        background {
            implicitWidth: 150
            implicitHeight: 40
            border.width: 1
            color: __baseWhite
        }
        text {
            color: __baseBlack
        }
    }

    toolBar {
        background.implicitHeight: 40
    }

    toolSeparator {
        padding: 2
        background.visible: false
        indicator {
            implicitWidth: 30
            implicitHeight: 1
            border.width: 0
            color: __strokeMuted
            foreground.visible: false
        }
    }
}
