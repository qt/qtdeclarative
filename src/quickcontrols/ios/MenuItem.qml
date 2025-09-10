// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS.impl

T.MenuItem {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    leftPadding: 12
    rightPadding: 12
    topPadding: 11
    bottomPadding: 11
    spacing: 9

    icon.width: 19
    icon.height: 19
    icon.color: control.palette.text

    readonly property bool __isSingleItem: control.menu && control.menu.count === 1
    readonly property bool __isFirstItem: !__isSingleItem && control.menu && control.menu.itemAt(0) === control ? true : false
    readonly property bool __isLastItem: !__isSingleItem && control.menu && control.menu.itemAt(control.menu.count - 1) === control ? true : false
    readonly property real __indicatorWidth: 12

    contentItem: IconLabel {
        readonly property real padding: control.__indicatorWidth + control.spacing
        leftPadding: !control.mirrored ? padding : 0
        rightPadding: control.mirrored ? padding : 0

        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: Qt.AlignLeft

        icon: control.icon
        text: control.text
        font: control.font
        color: control.palette.text
    }

    arrow: ColorImage {
        x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 7
        height: 12
        rotation: control.subMenu && (control.down || control.subMenu.visible) ? 90 : 0

        visible: control.subMenu
        opacity: control.enabled ? 1 : 0.5
        mirror: control.mirrored
        color: control.palette.text
        source: control.subMenu ? IOS.url + "arrow-indicator-light.png" : ""

        Behavior on rotation { RotationAnimation { duration: 100 } }
    }

    indicator: ColorImage {
        x: control.mirrored ? control.width - width - control.rightPadding : control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        scale: 0.8

        visible: control.checked
        source: control.checked ? IOS.url + "radiodelegate-indicator-light.png" : ""
        color: control.palette.text
    }

    background: Item {
        implicitHeight: 44
        implicitWidth: 250
        NinePatchImage {
            y: control.__isLastItem ? -1 : 0
            width: parent.width
            height: control.__isLastItem ? parent.height + 1 : parent.height
            rotation: control.__isLastItem ? 180 : 0
            visible: !(control.__isSingleItem && !control.down)
            source: IOS.url + "menuitem-background"
            NinePatchImageSelector on source {
                states: [
                    {"edge": control.__isFirstItem || control.__isLastItem},
                    {"single": control.__isSingleItem},
                    {"light": Application.styleHints.colorScheme === Qt.Light},
                    {"dark": Application.styleHints.colorScheme === Qt.Dark},
                    {"pressed": control.down}
                ]
            }
        }
    }

    states: [
        State {
            name: "submenu-opened"
            when: control.subMenu && control.subMenu.visible
            PropertyChanges { target: control.menu; scale: 0.9 }
        }
    ]
}

